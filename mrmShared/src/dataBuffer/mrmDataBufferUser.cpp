#include <errlog.h>
#include <epicsExport.h>

#include "../../evgMrmApp/src/evgMrm.h"
#include "../../evrMrmApp/src/evrMrm.h"

#include "mrmDataBufferUser.h"

int drvMrfiocDataBufferUserDebug = 1;
epicsExportAddress(int, drvMrfiocDataBufferUserDebug);

#if defined __GNUC__ && __GNUC__ < 3
#define dbgPrintf(args...)  if(drvMrfiocDataBufferUserDebug) printf(args);
#else
#define dbgPrintf(...)  if(drvMrfiocDataBufferUserDebug) printf(__VA_ARGS__);
#endif




mrmDataBufferUser::mrmDataBufferUser() {
    epicsUInt32 i;

    for (i=0; i<4; i++){
        m_rx_segments[i] = 0;
        m_tx_segments[i] = 0;
    }
    for (i=0; i<2048; i++) {    // TODO make define here
        m_rx_buff[i] = 0;
        m_tx_buff[i] = 0;
    }
    m_data_buffer = NULL;
}

epicsUInt8 mrmDataBufferUser::init(const char* deviceName) {
    m_data_buffer = getDataBufferFromDevice(deviceName);
    if(m_data_buffer == NULL) {
        errlogPrintf("Data buffer for %s not found.\n", deviceName);
        return 1;
    }

    if (m_data_buffer->supportsRx()) {
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
            return 3;
        }

        m_data_buffer->registerUser(this);
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

        if (supportsRx()) {
            m_data_buffer->removeUser(this);
            m_thread_stop = true;
            epicsEventSignal(m_thread_sync);
            epicsEventWait(m_thread_stopped);
            {
                epicsGuard<epicsMutex> gr(m_rx_lock);
                epicsEventDestroy(m_thread_stopped);
                epicsEventDestroy(m_thread_sync);

                for(size_t i = 0; i < m_rx_callbacks.size(); i++) {
                    delete m_rx_callbacks[i];
                }
            }

        }
    }
}

epicsUInt32 mrmDataBufferUser::registerInterest(size_t offset, size_t length, dataBufferRXCallback_t fptr, void *pvt) {
    RxCallback *cb = NULL;
    epicsUInt32 id;

    epicsGuard<epicsMutex> g(m_rx_lock);

    /* Check input arguments */
    if(length <= 0) {
        errlogPrintf("Invalid length %zu provided. Offset + length should be in interval [0, %d].\n", length, DataBuffer_len_max-1);
        return -1;
    }

    if(!supportsRx()) {
        errlogPrintf("This data buffer does not support reception of data, or reception thread could not be run. Cannot register interest.\n");
        return -1;
    }

    if (offset >= DataBuffer_len_max) {
        errlogPrintf("Trying to register on offset %zu, which is out of range (max offset: %d).\n", offset, DataBuffer_len_max-1);
        return -1;
    }

    if(offset + length > DataBuffer_len_max) {
        dbgPrintf("Trying to register %zu bytes on offset %zu, which is longer than buffer size (%d). ", length, offset, DataBuffer_len_max);
        length = DataBuffer_len_max - offset;
        dbgPrintf("Cropped length to %zu\n", length);
    }  


    cb = new RxCallback;
    cb->fptr = fptr;
    cb->pvt = pvt;
    cb->offset = offset;
    cb->length = length;

    m_rx_callbacks.push_back(cb);
    id = m_rx_callbacks.size()-1;

    return id;
}

void mrmDataBufferUser::removeInterest(epicsUInt32 id) {
    epicsUInt32 size;

    epicsGuard<epicsMutex> g(m_rx_lock);

    size = m_rx_callbacks.size();
    if(id < size) {
        m_rx_callbacks.erase(m_rx_callbacks.begin() + id);
    } else {
        errlogPrintf("Invalid registered interest ID (%d). Valid IDs are in range [0, %d]\n", id, size-1);
    }
}

void mrmDataBufferUser::put(size_t offset, size_t length, void *buffer) {
    epicsUInt16 segment, i;
    epicsUInt8 noOfSegmentsUpdated, segmentOffset;

    epicsGuard<epicsMutex> g(m_tx_lock);

    /* Check input arguments */
    if(length <= 0) {
        errlogPrintf("Invalid length %zu provided. Offset + length should be in interval [0, %d].\n", length, DataBuffer_len_max-1);
        return;
    }

    if (offset >= DataBuffer_len_max) {
        errlogPrintf("Trying to send on offset %zu, which is out of range (max offset: %d).\n", offset, DataBuffer_len_max-1);
        return;
    }

    if(offset + length > DataBuffer_len_max) {
        dbgPrintf("Too much data to send from offset %zu (%zu bytes). ", offset, length);
        length = DataBuffer_len_max - offset;
        dbgPrintf("Cropped length to %zu\n", length);
    }

    segment = offset / DataBuffer_segment_length;
    segmentOffset = offset - segment * DataBuffer_segment_length;
    noOfSegmentsUpdated = ((length - 1 + segmentOffset) / DataBuffer_segment_length) + 1 - (1 / DataBuffer_segment_length);

    // Update buffer at the offset segment
    memcpy(&m_tx_buff[offset], buffer, length);

    // Mark which segments were updated
    for(i=segment; i<segment+noOfSegmentsUpdated; i++){
        m_tx_segments[i / 32] |= 0x80000000 >> i % 32;
    }
}

void mrmDataBufferUser::get(size_t offset, size_t length, void *buffer) {
    epicsGuard<epicsMutex> g(m_rx_lock);

    /* Check input arguments */
    if(length <= 0) {
        errlogPrintf("Invalid length %zu provided. Offset + length should be in interval [0, %d].\n", length, DataBuffer_len_max-1);
        return;
    }

    if (offset >= DataBuffer_len_max) {
        errlogPrintf("Trying to receive from offset %zu, which is out of range (max offset: %d).\n", offset, DataBuffer_len_max-1);
        return;
    }

    if(offset + length > DataBuffer_len_max) {
        dbgPrintf("Too much data to receive from offset %zu (%zu bytes). ", offset, length);
        length = DataBuffer_len_max - offset;
        dbgPrintf("Cropped length to %zu\n", length);
    }


    // Copy from offset to user buffer
    memcpy(buffer, &m_rx_buff[offset], length);
}

void mrmDataBufferUser::send(bool wait)
{
    epicsUInt32 segments;
    epicsUInt8 i, bits, startSegment=0;
    epicsUInt16 length = 0;

    if (m_data_buffer != NULL) {
        { // { here to enforce m_tx_lock scope
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

        if(wait) m_data_buffer->waitForTxComplete();
    } else {
        errlogPrintf("Cannot send since data buffer is not available!\n");
    }
}

void mrmDataBufferUser::updateSegment(epicsUInt16 segment, epicsUInt8 *data, epicsUInt16 length) {
    epicsUInt32 segmentMask;
    epicsUInt32 i;
    epicsUInt8 noOfSegmentsUpdated;

    if (m_rx_lock.tryLock()) {  //TODO can throw exception
        // Copy received segment(s) to local buffer. If the the tryLock fails, the buffer is out of sync...
        memcpy(&m_rx_buff[segment*DataBuffer_segment_length], &data[segment*DataBuffer_segment_length], length);

        // Set segment received flags
        noOfSegmentsUpdated = ((length - 1) / DataBuffer_segment_length) + 1 - (1 / DataBuffer_segment_length);

        for(i=segment; i<segment+noOfSegmentsUpdated; i++){
            segmentMask = (0x80000000 >> i % 32);
            if (m_rx_segments[i / 32] & segmentMask) {
                errlogPrintf("SW overflow occured for segment %d\n", i);
                m_rx_segments[i / 32] &= ~segmentMask;
            }
            m_rx_segments[i / 32] |= segmentMask;
        }

        m_rx_lock.unlock();

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
    epicsUInt16 startOffset=0, maxOffset, minLength, length=0;

    epicsEventWait(parent->m_thread_sync);
    while (!parent->m_thread_stop) {

        parent->m_rx_lock.lock();

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

                        // iterate through segments and clear received segment flag. This is used to check SW overflow when receiving.
                        for(j=0; j<=i; j++) {
                            //printf("clear segments 0x%x\n", segmentsReceived[j]);
                            parent->m_rx_segments[j] &= ~segmentsReceived[j];  // mark the segments as handled
                            segmentsReceived[j] = 0;
                        }

                        length = 0;
                    } // end if (length > 0)
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

        parent->m_rx_lock.unlock();
        epicsEventWait(parent->m_thread_sync);
    }
    epicsEventSignal(parent->m_thread_stopped);
}

bool mrmDataBufferUser::supportsRx() {
    if(m_data_buffer != NULL) return m_data_buffer->supportsRx() && m_thread_id != 0;
    return false;
}

bool mrmDataBufferUser::supportsTx() {
    if(m_data_buffer != NULL) return m_data_buffer->supportsTx();
    return false;
}
