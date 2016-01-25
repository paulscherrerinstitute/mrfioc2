#include <string.h>     // for memcpy
#include <stdio.h>

#include <errlog.h>
#include <epicsGuard.h>

#include "mrmShared.h"
#include "mrmDataBuffer.h"

#include <epicsExport.h>
#include "mrmDataBufferUser.h"



mrmDataBufferUser::mrmDataBufferUser() {
    epicsUInt32 i;

    for (i=0; i<4; i++){
        m_rx_segments[i] = 0;
        m_tx_segments[i] = 0;
        m_segments_interested[i] = 0;
    }

    memset(m_rx_buff, 0, 2048);
    memset(m_tx_buff, 0, 2048);

    m_data_buffer = NULL;
    m_user_offset = 0;
    m_strict_mode = false;
}

bool mrmDataBufferUser::init(const char* deviceName, size_t userOffset, bool strictMode, unsigned int userUpdateThreadPriority) {
    if (m_data_buffer != NULL) {
        errlogPrintf("Data buffer for %s already initialized.\n", deviceName);
        return false;
    }

    if (userOffset > DataBuffer_len_max) {
        errlogPrintf("User offset too big. Should be [0, %d]. Init aborted.\n", DataBuffer_len_max);
        return false;
    }

    m_data_buffer = mrmDataBuffer::getDataBufferFromDevice(deviceName);
    if(m_data_buffer == NULL) {
        errlogPrintf("Data buffer for %s not found.\n", deviceName);
        return false;
    }

    m_user_offset = userOffset;
    m_strict_mode = strictMode;

    if (m_data_buffer->supportsRx()) {
        m_thread_sync = epicsEventCreate(epicsEventEmpty);
        m_thread_stopped = epicsEventCreate(epicsEventEmpty);
        if (!m_thread_sync | !m_thread_stopped) {
            errlogPrintf("Unable to create semaphore for %s.\n", deviceName);
            return false;
        }
        m_thread_stop = false;
        m_thread_id = epicsThreadCreate("MRF DBUFF UPDATE"
                                        ,userUpdateThreadPriority
                                        ,epicsThreadGetStackSize(epicsThreadStackMedium)
                                        ,&mrmDataBufferUser::userUpdateThread
                                        ,this);
        if(!m_thread_id) {
            errlogPrintf("Unable to create data buffer Rx update thread for %s.\n", deviceName);
            return false;
        }

        m_data_buffer->registerUser(this);
    }

    return true;
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

size_t mrmDataBufferUser::registerInterest(size_t offset, size_t length, dataBufferRxCallback_t fptr, void *pvt) {
    RxCallback *cb = NULL;
    size_t id;
    epicsUInt16 segment, i, segmentOffset, noOfSegmentsUpdated;

    epicsGuard<epicsMutex> g(m_rx_lock);

    if (m_data_buffer == NULL) {
        errlogPrintf("Data buffer not initialized. Did you call init() function?\n");
        return 0;
    }

    /* Check input arguments */
    if(length <= 0) {
        errlogPrintf("Invalid length %zu provided. Offset + length should be in interval [1, %d].\n", length, DataBuffer_len_max-1);
        return 0;
    }

    if(!supportsRx()) {
        errlogPrintf("This data buffer does not support reception of data, or reception thread could not be run. Cannot register interest.\n");
        return 0;
    }

    offset += m_user_offset;
    if (offset >= DataBuffer_len_max) {
        errlogPrintf("Trying to register on offset %zu, which is out of range (max offset: %d).\n", offset, DataBuffer_len_max-1);
        return 0;
    }

    if(offset + length > DataBuffer_len_max) {
        dbgPrintf(1, "Trying to register %zu bytes on offset %zu, which is longer than buffer size (%d). ", length, offset, DataBuffer_len_max);
        length = DataBuffer_len_max - offset;
        dbgPrintf(1, "Cropped length to %zu\n", length);
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

    for (i=0; i<4; i++) {
        cb->segments[i] = 0;
    }

    segment = (epicsUInt16)(offset / DataBuffer_segment_length);
    segmentOffset = (epicsUInt16)(offset - segment * DataBuffer_segment_length);
    noOfSegmentsUpdated = (epicsUInt16)(((length - 1 + segmentOffset) / DataBuffer_segment_length) + 1);

    // Mark which segments were updated
    for(i=segment; i<segment+noOfSegmentsUpdated; i++){
        cb->segments[i / 32] |= 0x80000000 >> (i % 32);
        m_segments_interested[i / 32] |= cb->segments[i / 32];  // mark segment interest in global segment mask
    }

    m_rx_callbacks.push_back(cb);

    m_data_buffer->setInterest(this, m_segments_interested);

    return id;
}

bool mrmDataBufferUser::removeInterest(size_t id) {
    size_t i;
    epicsUInt16 j;

    epicsGuard<epicsMutex> g(m_rx_lock);

    if (m_data_buffer == NULL) {
        errlogPrintf("Data buffer not initialized. Did you call init() function?\n");
        return false;
    }

    if(id > 0) {   // are we in range?
        for (j=0; j<4; j++) {
            m_segments_interested[j] = 0;
        }

        for (i=0; i<m_rx_callbacks.size(); i++){
            if (m_rx_callbacks[i]->id == id) {   // found a callback with specified ID
                m_rx_callbacks.erase(m_rx_callbacks.begin() + i);

            } else {
                for (j=0; j<4; j++) {
                    m_segments_interested[j] |= m_rx_callbacks[i]->segments[j];
                }
            }

        }
        m_data_buffer->setInterest(this, m_segments_interested);
    } else {
        RxCallback * cb = m_rx_callbacks.back();
        if(cb == NULL) errlogPrintf("No registered callbacks exist. Cannot remove...\n");
        else errlogPrintf("Invalid registered interest ID (%zu). Valid IDs are in range [1, %zu]\n", id, cb->id);
        return false;
    }

    return true;
}

epicsUInt8 *mrmDataBufferUser::requestTxBuffer() {
    m_tx_lock.lock();

    return &m_tx_buff[m_user_offset];
}

// TODO separate marking of updated segments. This would allow user to write multiple non-consecutive areas of the buffer without the need to reacquire locks for each area.
void mrmDataBufferUser::releaseTxBuffer(size_t offset, size_t length) {
    epicsUInt16 segment, i, noOfSegmentsUpdated, segmentOffset;

    offset += m_user_offset;
    segment = (epicsUInt16)(offset / DataBuffer_segment_length);
    segmentOffset = (epicsUInt16)(offset - segment * DataBuffer_segment_length);
    noOfSegmentsUpdated = (epicsUInt16)(((length - 1 + segmentOffset) / DataBuffer_segment_length) + 1);

    // Mark which segments were updated
    for(i=segment; i<segment+noOfSegmentsUpdated; i++){
        m_tx_segments[i / 32] |= 0x80000000 >> i % 32;
    }

    m_tx_lock.unlock();
}

void mrmDataBufferUser::put(size_t offset, size_t length, void *buffer) {
    /* Check input arguments */
    if(length <= 0) {
        errlogPrintf("Invalid length %zu provided. Offset + length should be in interval [1, %d].\n", length, DataBuffer_len_max-1);
        return;
    }

    offset += m_user_offset;
    if (offset >= DataBuffer_len_max) {
        errlogPrintf("Trying to transfer on offset %zu, which is out of range (max offset: %d).\n", offset, DataBuffer_len_max-1);
        return;
    }

    if(offset + length > DataBuffer_len_max) {
        dbgPrintf(1, "Too much data to transfer on offset %zu (%zu bytes). ", offset, length);
        length = DataBuffer_len_max - offset;
        dbgPrintf(1, "Cropped length to %zu\n", length);
    }

    m_tx_lock.lock();

    // Update buffer at the offset segment
    memcpy(&m_tx_buff[offset], buffer, length);

    releaseTxBuffer(offset-m_user_offset, length);
}


epicsUInt8 *mrmDataBufferUser::requestRxBuffer() {
    m_rx_lock.lock();

    return &m_rx_buff[m_user_offset];
}

void mrmDataBufferUser::releaseRxBuffer()
{
    m_rx_lock.unlock();
}

void mrmDataBufferUser::get(size_t offset, size_t length, void *buffer) {
    epicsGuard<epicsMutex> g(m_rx_lock);

    /* Check input arguments */
    if(length <= 0) {
        errlogPrintf("Invalid length %zu provided. Offset + length should be in interval [1, %d].\n", length, DataBuffer_len_max-1);
        return;
    }

    offset += m_user_offset;
    if (offset >= DataBuffer_len_max) {
        errlogPrintf("Trying to receive from offset %zu, which is out of range (max offset: %d).\n", offset, DataBuffer_len_max-1);
        return;
    }

    if(offset + length > DataBuffer_len_max) {
        dbgPrintf(1, "Too much data to receive from offset %zu (%zu bytes). ", offset, length);
        length = DataBuffer_len_max - offset;
        dbgPrintf(1, "Cropped length to %zu\n", length);
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
    epicsUInt16 noOfSegmentsUpdated;

    if(m_strict_mode) {
        m_rx_lock.lock();
    } else if (!m_rx_lock.tryLock()) {  // can throw exception, user should decide how to handle it...
        errlogPrintf("Loosing data: Could not acquire exclusive access to the buffer. User operations on the buffer take too long?\n");
        return;
    }

    noOfSegmentsUpdated = ((length - 1) / DataBuffer_segment_length) + 1;
    for(i=segment; i<(epicsUInt32)(segment+noOfSegmentsUpdated); i++){ // detect overflow
        segmentMask = (0x80000000 >> (i % 32));
        if (m_rx_segments[i / 32] & segmentMask) {
            errlogPrintf("SW overflow occured for segment %d. Discarding data buffer update\n", i);
            m_rx_lock.unlock();
            return;
        }
        segmentsReceived[i / 32] |= segmentMask;   // Mark segment as received
    }

    for (i=0; i<4; i++) {
        m_rx_segments[i] |= segmentsReceived[i] & m_segments_interested[i];   // Set segment received flags. We only care for segments someone is interested in.
    }

    // Copy received segment(s) to local buffer. If the the tryLock fails, the buffer is out of sync...
    memcpy(&m_rx_buff[segment*DataBuffer_segment_length], &data[segment*DataBuffer_segment_length], length);

    m_rx_lock.unlock();

    // Wake up thread for updating user data, if semaphore is valid.
    if (m_thread_sync) {
        epicsEventSignal(m_thread_sync);
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
                        userCallback->fptr(startOffset-parent->m_user_offset, length, userCallback->pvt); // call the function that the user registered
                        length = 0;
                    }
                    segments <<= 1;
                    bits--;
                }
            }

            // there is data spanning to the end of the buffer. Send it.
            if (length > 0) {   // user is interested in data on startOffset + length.
                if (startOffset * DataBuffer_segment_length + length > DataBuffer_len_max) {
                    length = (length - 1) & DataBuffer_len_max; // Length of the entire buffer is greater than max length that can be send (because of the 4 byte increment). This means that the last segment is actually smaller than 16 bytes. Trim the length...
                }
                userCallback->fptr(startOffset-parent->m_user_offset, length, userCallback->pvt); // call the function that the user registered
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

size_t mrmDataBufferUser::getMaxLength()
{
    return DataBuffer_len_max - m_user_offset;
}
