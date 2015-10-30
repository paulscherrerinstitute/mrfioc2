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

    memset(m_rx_buff, 0, 2048);// TODO make define here
    memset(m_tx_buff, 0, 2048);// TODO make define here

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
                                        ,epicsThreadPriorityLow    // TODO expose to user
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

size_t mrmDataBufferUser::registerInterest(size_t offset, size_t length, dataBufferRXCallback_t fptr, void *pvt) {
    RxCallback *cb = NULL;
    size_t id;
    epicsUInt16 segment, i, segmentOffset, noOfSegmentsUpdated;

    epicsGuard<epicsMutex> g(m_rx_lock);

    /* Check input arguments */
    if(length <= 0) {
        errlogPrintf("Invalid length %zu provided. Offset + length should be in interval [0, %d].\n", length, DataBuffer_len_max-1);
        return 0;
    }

    if(!supportsRx()) {
        errlogPrintf("This data buffer does not support reception of data, or reception thread could not be run. Cannot register interest.\n");
        return 0;
    }

    if (offset >= DataBuffer_len_max) {
        errlogPrintf("Trying to register on offset %zu, which is out of range (max offset: %d).\n", offset, DataBuffer_len_max-1);
        return 0;
    }

    if(offset + length > DataBuffer_len_max) {
        dbgPrintf("Trying to register %zu bytes on offset %zu, which is longer than buffer size (%d). ", length, offset, DataBuffer_len_max);
        length = DataBuffer_len_max - offset;
        dbgPrintf("Cropped length to %zu\n", length);
    }  


    if (m_rx_callbacks.size() > 0) {
        cb = m_rx_callbacks.back();
        id = cb->id + 1;
        if(id <= 0) {   // id overflowed
            errlogPrintf("Maximum number of registered interests reached. Canceled operation.\n");
            return 0;
        }
    } else id = 1;  // this is the first ID. We start with 1...


    cb = new RxCallback;
    cb->fptr = fptr;
    cb->pvt = pvt;
    cb->id = id;

    segment = offset / DataBuffer_segment_length;
    segmentOffset = offset - segment * DataBuffer_segment_length;
    noOfSegmentsUpdated = ((length - 1 + segmentOffset) / DataBuffer_segment_length) + 1;

    // Mark which segments were updated
    for(i=segment; i<segment+noOfSegmentsUpdated; i++){
        cb->segments[i / 32] |= 0x80000000 >> i % 32;
    }

    m_rx_callbacks.push_back(cb);

    return id;
}

void mrmDataBufferUser::removeInterest(size_t id) {
    size_t size, i;

    epicsGuard<epicsMutex> g(m_rx_lock);

    size = m_rx_callbacks.size();
    if(id < size && id > 0) {   // are we in range?
        for (i=0; i<size; i++){
            if (m_rx_callbacks[i]->id == id) {   // found a callback with specified ID
                m_rx_callbacks.erase(m_rx_callbacks.begin() + i);
                break;
            }
        }

    } else {
        errlogPrintf("Invalid registered interest ID (%zu). Valid IDs are in range [1, %zu]\n", id, size-1);
    }
}

void mrmDataBufferUser::put(size_t offset, size_t length, void *buffer) {
    epicsUInt16 segment, i, noOfSegmentsUpdated, segmentOffset;;

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
    noOfSegmentsUpdated = ((length - 1 + segmentOffset) / DataBuffer_segment_length) + 1;

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

bool mrmDataBufferUser::send(bool wait)
{
    epicsUInt32 segments;
    epicsUInt8 i, bits, startSegment=0;
    epicsUInt16 length = 0;

    if (m_data_buffer != NULL) {
        { // brackets here to enforce m_tx_lock scope
            epicsGuard<epicsMutex> g(m_tx_lock);

            for(i=0; i<4; i++){
                segments = m_tx_segments[i];
                bits = 32;
                while(bits){
                    if (segments & 0x80000000) {    // we found segment that needs sending
                        if (length == 0) startSegment = 32 * i + 32 - bits;
                        length += DataBuffer_segment_length;;

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

        if(wait) {
            if (!m_data_buffer->waitForTxComplete()) {
                errlogPrintf("Data sent, but waiting for Tx complete takes too long. Exiting prematurely...\n");
                return false;
            }
        }
    } else {
        errlogPrintf("Cannot send since data buffer is not available!\n");
    }

    return true;
}

void mrmDataBufferUser::updateSegment(epicsUInt16 segment, epicsUInt8 *data, epicsUInt16 length) {
    epicsUInt32 segmentMask, segmentsReceived[4]={0, 0, 0, 0};
    epicsUInt32 i;
    epicsUInt8 noOfSegmentsUpdated;

    // TODO give user option to use lock/try lock here
    if (m_rx_lock.tryLock()) {  // can throw exception, user should decide how to handle it...

        noOfSegmentsUpdated = ((length - 1) / DataBuffer_segment_length) + 1;
        for(i=segment; i<segment+noOfSegmentsUpdated; i++){ // detect overflow
            segmentMask = (0x80000000 >> (i % 32));
            if (m_rx_segments[i / 32] & segmentMask) {
                errlogPrintf("SW overflow occured for segment %d. Discarding data buffer update\n", i);
                m_rx_lock.unlock();
                return;
            }
            segmentsReceived[i / 32] |= segmentMask;   // Mark segment as received
        }

        for (i=0; i<4; i++) {
            m_rx_segments[i] |= segmentsReceived[i];   // Set segment received flags
        }

        // Copy received segment(s) to local buffer. If the the tryLock fails, the buffer is out of sync...
        memcpy(&m_rx_buff[segment*DataBuffer_segment_length], &data[segment*DataBuffer_segment_length], length);

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
    epicsUInt32 segments;
    size_t i;
    epicsUInt8 j, bits;
    mrmDataBufferUser* parent = static_cast<mrmDataBufferUser*>(args);
    RxCallback *userCallback;
    epicsUInt16 startOffset=0, length=0;

    epicsEventWait(parent->m_thread_sync);
    while (!parent->m_thread_stop) {

        parent->m_rx_lock.lock();

        for (i=0; i<parent->m_rx_callbacks.size(); i++) {
            userCallback = parent->m_rx_callbacks[i];

            for (j=0; j<4; j++) {
                // check if there is any data this user is interested in
                segments = parent->m_rx_segments[j] & userCallback->segments[j];
                bits = 32;

                while (bits) {
                    if (segments & 0x80000000) {
                        if (length == 0) startOffset = (32 * j + 32 - bits) * DataBuffer_segment_length; // first segment found. Mark it's offset.
                        length += DataBuffer_segment_length;    // user is interested in this segment, so increase the length
                    }
                    else if (length > 0) {
                        userCallback->fptr(startOffset, length, userCallback->pvt); // call the function that the user registered
                        length = 0;
                    }
                    segments <<= 1;
                    bits--;
                }
            }

            if (length > 0) {   // user is interested in data on startOffset + length.
                userCallback->fptr(startOffset, length, userCallback->pvt); // call the function that the user registered
            }

            length = 0;
        }

        // iterate through segments and clear received segment flag. This is used to check SW overflow when receiving.
        // note that only when all users handle all the data, the overflow flags are cleared. Since we are holding a lock here, new reception can't start anyway...
        for(j=0; j<4; j++) {
            parent->m_rx_segments[j] = 0;  // mark the segments as handled
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
