#include <errlog.h>
#include <epicsExport.h>

#include "evgMrm.h"
#include "drvem.h"

#include "mrmDataBufferUser.h"

int drvMrfiocDataBufferUserDebug = 1;
epicsExportAddress(int, drvMrfiocDataBufferUserDebug);

#if defined __GNUC__ && __GNUC__ < 3
#define dbgPrintf(args...)  if(drvMrfiocDataBufferUserDebug) printf(args);
#else
#define dbgPrintf(...)  if(drvMrfiocDataBufferUserDebug) printf(__VA_ARGS__);
#endif




mrmDataBufferUser::mrmDataBufferUser() {
    epicsUInt8 i;

    for (i=0; i<4; i++){
        m_rx_segments[i] = 0;
        m_tx_segments[i] = 0;
    }
    m_data_buffer = NULL;
}

epicsUInt8 mrmDataBufferUser::init(const char* deviceName) {
    m_data_buffer = getDataBufferFromDevice(deviceName);
    if(m_data_buffer == NULL) {
        errlogPrintf("Data buffer for %s not found.\n", deviceName);
        return 1;
    }


    m_rx_lock = epicsMutexCreate();
    if (!m_rx_lock) {
        errlogPrintf("Unable to create mutex for %s.\n", deviceName);
        return 2;
    }

    if (m_data_buffer->supportsRx()) {
        m_data_buffer->registerUser(this);
        m_thread_sync = epicsEventCreate(epicsEventEmpty);
        m_thread_stopped = epicsEventCreate(epicsEventEmpty);
        if (!m_thread_sync | !m_thread_stopped) {
            errlogPrintf("Unable to create semaphore for %s.\n", deviceName);
            return 2;
        }
        m_thread_stop = false;
        m_thread_id = epicsThreadCreate("MRF DBUFF UPDATE"
                                        ,epicsThreadPriorityLow
                                        ,epicsThreadGetStackSize(epicsThreadStackMedium)
                                        ,&mrmDataBufferUser::userUpdateThread
                                        ,this);
        if(!m_thread_id) {
            errlogPrintf("Unable to create data buffer Rx update thread for %s.\n", deviceName);
            m_thread_stop = true;
            return 3;
        }
    }

    return 0;
}

mrmDataBuffer* mrmDataBufferUser::getDataBufferFromDevice(const char *device) {
    evgMrm* evg = dynamic_cast<evgMrm*>(mrf::Object::getObject(device));
    if(evg){
        return evg->getDataBuffer();
    } else {
        EVRMRM* evr = dynamic_cast<EVRMRM*>(mrf::Object::getObject(device));
        if(evr){
            return evr->getDataBuffer();
        }
    }

    return NULL;
}

mrmDataBufferUser::~mrmDataBufferUser()
{
    if(m_data_buffer != NULL) {
        epicsGuard<epicsMutex> g(m_tx_lock);

        bool rx = m_data_buffer->supportsRx();

        if (rx) {
            m_data_buffer->removeUser(this);
            m_thread_stop = true;
            epicsEventSignal(m_thread_sync);
            //printf("thread dead\n");
            epicsEventWait(m_thread_stopped);
            //printf("locking rx...");

            epicsMutexLock(m_rx_lock);
            //printf("rx locked\n");
            epicsEventDestroy(m_thread_stopped);
            epicsEventDestroy(m_thread_sync);

            for(size_t i = 0; i < m_rx_callbacks.size(); i++) {
                delete m_rx_callbacks[i];
            }
            //printf("unlocking rx...");
            epicsMutexUnlock(m_rx_lock);
            //printf("unlocked rx, destroying rx...");
            epicsMutexDestroy(m_rx_lock);
            //printf("rx destroyed\n");

        }

        //printf("destructor ended\n");
    }
}

epicsUInt16 mrmDataBufferUser::registerInterest(size_t offset, size_t length, dataBufferRXCallback_t fptr, void *pvt) {
    RxCallback *cb = NULL;
    epicsUInt32 id;

    /* Check input arguments */
    if(length <= 0) {
        errlogPrintf("Invalid length %d provided. Offset + length should be in interval [0, %d].\n", length, DataBuffer_len_max-1);
        return -1;
    }

    if (offset >= DataBuffer_len_max) {
        errlogPrintf("Trying to register on offset %d, which is out of range (max offset: %d).\n", offset, DataBuffer_len_max-1);
        return -1;
    }

    if(offset + length > DataBuffer_len_max) {
        dbgPrintf("Trying to register %d bytes on offset %d, which is longer than buffer size (%d). ", length, offset, DataBuffer_len_max);
        length = DataBuffer_len_max - offset;
        dbgPrintf("Cropped length to %d\n", length);
    }

    if(!supportsRx())dbgPrintf("This data buffer does not support reception of data. Cannot register interest.\n");


    epicsMutexLock(m_rx_lock);

    cb = new RxCallback;
    cb->fptr = fptr;
    cb->pvt = pvt;
    cb->offset = offset;
    cb->length = length;

    m_rx_callbacks.push_back(cb);
    id = m_rx_callbacks.size()-1;

    epicsMutexUnlock(m_rx_lock);
    return id;
}

void mrmDataBufferUser::removeInterest(epicsUInt16 id) {
    epicsUInt32 size;

    epicsMutexLock(m_rx_lock);

    size = m_rx_callbacks.size();
    if(id < size) {
        m_rx_callbacks.erase(m_rx_callbacks.begin() + id);
    } else {
        errlogPrintf("Invalid registered interest ID (%d). Valid IDs are in range [0, %d]\n", id, size-1);
    }

    epicsMutexUnlock(m_rx_lock);
}

void mrmDataBufferUser::put(size_t offset, size_t length, void *buffer) {
    epicsUInt16 segment, i;
    epicsUInt8 noOfSegmentsUpdated, segmentOffset;

    epicsGuard<epicsMutex> g(m_tx_lock);


    /* Check input arguments */
    if(length <= 0) {
        errlogPrintf("Invalid length %d provided. Offset + length should be in interval [0, %d].\n", length, DataBuffer_len_max-1);
        return;
    }

    if (offset >= DataBuffer_len_max) {
        errlogPrintf("Trying to send on offset %d, which is out of range (max offset: %d).\n", offset, DataBuffer_len_max-1);
        return;
    }

    if(offset + length > DataBuffer_len_max) {
        dbgPrintf("Too much data to send from offset %d (%d bytes). ", offset, length);
        length = DataBuffer_len_max - offset;
        dbgPrintf("Cropped length to %d\n", length);
    }

    segment = offset / DataBuffer_segment_length;
    segmentOffset = offset - segment * DataBuffer_segment_length;
    noOfSegmentsUpdated = ((length - 1 + segmentOffset) / DataBuffer_segment_length) + 1 - (1 / DataBuffer_segment_length);

    // Update buffer at the offset segment
    for(i=0; i<length; i++) {
        m_tx_buff[offset + i] = ((epicsUInt8 *)buffer)[i];
    }

    // Mark which segments were updated
    for(i=segment; i<segment+noOfSegmentsUpdated; i++){
        m_tx_segments[i / 32] |= 0x80000000 >> i % 32;
    }
}

void mrmDataBufferUser::get(size_t offset, size_t length, void *buffer) {
    //epicsUInt32 i;

    /* Check input arguments */
    if(length <= 0) {
        errlogPrintf("Invalid length %d provided. Offset + length should be in interval [0, %d].\n", length, DataBuffer_len_max-1);
        return;
    }

    if (offset >= DataBuffer_len_max) {
        errlogPrintf("Trying to receive from offset %d, which is out of range (max offset: %d).\n", offset, DataBuffer_len_max-1);
        return;
    }

    if(offset + length > DataBuffer_len_max) {
        dbgPrintf("Too much data to receive from offset %d (%d bytes). ", offset, length);
        length = DataBuffer_len_max - offset;
        dbgPrintf("Cropped length to %d\n", length);
    }


    epicsMutexLock(m_rx_lock);
    // Copy from offset to user buffer
    /*for(i=0; i<length; i++) {
        ((epicsUInt8 *)buffer)[i] = m_rx_buff[offset + i];
    }*/
    memcpy(buffer, &m_rx_buff[offset], length);

    epicsMutexUnlock(m_rx_lock);
}

void mrmDataBufferUser::send(bool wait)
{
    if (m_data_buffer != NULL) {
        sendDataBuffer();
        if(wait) m_data_buffer->waitForTxComplete();
    } else {
        errlogPrintf("Cannot send since data buffer is not available!\n");
    }
}

void mrmDataBufferUser::updateSegment(epicsUInt16 segment, epicsUInt8 *data, epicsUInt16 length) {
    epicsUInt32 segmentMask;
    epicsUInt32 i;
    epicsUInt8 noOfSegmentsUpdated;

    if (epicsMutexTryLock(m_rx_lock) == epicsMutexLockOK) {
        // Copy received segment(s) to local buffer. If the the tryLock fails, the buffer is out of sync...
        for(i=segment*DataBuffer_segment_length; i<length+segment*(epicsUInt32)DataBuffer_segment_length; i+=4) {
            *(epicsUInt32*)(m_rx_buff + i) = *(epicsUInt32*)(data + i);
        }

        // Set segment received flags
        noOfSegmentsUpdated = ((length - 1) / DataBuffer_segment_length) + 1 - (1 / DataBuffer_segment_length);

        for(i=segment; i<segment+noOfSegmentsUpdated; i++){
            segmentMask = (0x80000000 >> i % 32);
            if (m_rx_segments[i / 32] & segmentMask) {
                dbgPrintf("SW overflow occured for segment %d\n", i);
                m_rx_segments[i / 32] &= ~segmentMask;
            }
            m_rx_segments[i / 32] |= segmentMask;
        }

        epicsMutexUnlock(m_rx_lock);

        // Wake up thread for updating user data, if semaphore is valid.
        if (m_thread_sync) {
            epicsEventSignal(m_thread_sync);
        }
    } else {
        errlogPrintf("Loosing data: Could not acquire exclusive access to the buffer. User operations on the buffer take too long?\n");
    }
}

void mrmDataBufferUser::userUpdateThread(void* args) {
    epicsUInt32 j, segments, segmentsReceived[4]={0, 0, 0, 0};
    epicsUInt8 i, bits;
    mrmDataBufferUser* parent = static_cast<mrmDataBufferUser*>(args);
    RxCallback *userCallback;
    epicsUInt16 startOffset, maxOffset, minLength, length=0;

    epicsEventWait(parent->m_thread_sync);
    while (!parent->m_thread_stop) {

        epicsMutexLock(parent->m_rx_lock);

        for (i=0; i<4; i++) {
            segments = parent->m_rx_segments[i];
            bits = 32;

            while(bits){
                if (segments & 0x80000000) {    // we found a received segment
                    if (length == 0) startOffset = (32 * i + 32 - bits) * DataBuffer_segment_length;
                    length += DataBuffer_segment_length;
                    segmentsReceived[i] |= 0x80000000 >> (32 - bits);
                } else {    // this segment was not received
                    if (length > 0) {   // we have received data on startSegment + length.
                        for (j=0; j<parent->m_rx_callbacks.size(); j++) {
                            userCallback = parent->m_rx_callbacks[j];
                            // Check if anyone is interested in what we received
                            if ((userCallback->offset >= startOffset && userCallback->offset <= startOffset+length) ||
                                (userCallback->offset + userCallback->length >= startOffset && userCallback->offset + userCallback->length <= startOffset+length)) {

                                // Calculate the offset and length that is sent out to each interested user, based on what was received and what he is interested in.
                                if(userCallback->offset >= startOffset) {
                                    maxOffset = userCallback->offset;
                                } else {
                                    maxOffset = startOffset;
                                }
                                if(userCallback->length <= length) {
                                    minLength = userCallback->length;
                                } else {
                                    minLength = length;
                                }

                                userCallback->fptr(maxOffset, minLength, userCallback->pvt); // call the function that the user registered
                            }
                        }
                        // iterate through segments and clear received segment flag. This is used to check SW overflow.
                        for(j=0; j<=i; j++) {
                            //printf("clear segments 0x%x\n", segmentsReceived[j]);
                            parent->m_rx_segments[j] &= ~segmentsReceived[j];  // mark the segments as handled
                            segmentsReceived[j] = 0;
                        }
                        length = 0;

                    }
                }
                segments <<= 1;
                bits--;
            }
        }

        if (length > 0) {   // we have received data at the end of the buffer. Check if anyone is interested in it
            for (j=0; j<parent->m_rx_callbacks.size(); j++) {
                userCallback = parent->m_rx_callbacks[j];
                if ((userCallback->offset >= startOffset && userCallback->offset <= startOffset+length) ||
                    (userCallback->offset + userCallback->length >= startOffset && userCallback->offset + userCallback->length <= startOffset+length)) {

                    if(userCallback->offset >= startOffset) {
                        maxOffset = userCallback->offset;
                    } else {
                        maxOffset = startOffset;
                    }
                    if(userCallback->length <= length) {
                        minLength = userCallback->length;
                    } else {
                        minLength = length;
                    }
                    userCallback->fptr(maxOffset, minLength, userCallback->pvt);
                }
            }
            for(j=0; j<4; j++) {
                //printf("clear segments 0x%x\n", segmentsReceived[j]);
                parent->m_rx_segments[j] &= ~segmentsReceived[j];  // mark the segments as handled
                segmentsReceived[j] = 0;
            }
        }

        epicsMutexUnlock(parent->m_rx_lock);

        epicsEventWait(parent->m_thread_sync);
    }
    epicsEventSignal(parent->m_thread_stopped);
}

bool mrmDataBufferUser::supportsRx() {
    if(m_data_buffer != NULL) return m_data_buffer->supportsRx();
    return false;
}

bool mrmDataBufferUser::supportsTx() {
    if(m_data_buffer != NULL) return m_data_buffer->supportsTx();
    return false;
}

void mrmDataBufferUser::sendDataBuffer() {
    epicsUInt32 segments;
    epicsUInt8 i, bits, startSegment;
    epicsUInt16 length = 0;

    epicsGuard<epicsMutex> g(m_tx_lock);

    for(i=0; i<4; i++){
        segments = m_tx_segments[i];
        bits = 32;
        while(bits){
            if (segments & 0x80000000) {    // we found segment that needs sending
                if (length == 0) startSegment = 32 * i + 32 - bits;
                length += 16;

            } else {    // this segment will not be sent
                if (length > 0) {   // previous segment was last consecutive segment to send.
                    m_data_buffer->send(startSegment, length, &m_tx_buff[startSegment*DataBuffer_segment_length]);
                }
                length = 0;
            }
            segments <<= 1;
            bits--;
        }
    }

    if (length > 0) {   // segment(s) at the end of the buffer were changed. Send them.
        if (length > DataBuffer_len_max) length = DataBuffer_len_max; // Sometimes we need to send the entire buffer. Since we are increasing length by 16, 128 segments represent 2048 bytes, which is not dividable by 4. Mask it...
        m_data_buffer->send(startSegment, length, &m_tx_buff[startSegment*DataBuffer_segment_length]);
    }
}
