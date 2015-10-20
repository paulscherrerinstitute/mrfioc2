#include <time.h>

#include <errlog.h>
#include <epicsExport.h>

#include "evgMrm.h"
#include "drvem.h"

#include "mrmDataBufferUser.h"

int drvMrfiocm_data_bufferUserDebug = 0;
epicsExportAddress(int, drvMrfiocm_data_bufferUserDebug);

#if defined __GNUC__ && __GNUC__ < 3
#define dbgPrintf(args...)  if(drvMrfiocm_data_bufferUserDebug) printf(args);
#else
#define dbgPrintf(...)  if(drvMrfiocm_data_bufferUserDebug) printf(__VA_ARGS__);
#endif




mrmDataBufferUser::mrmDataBufferUser() {
    epicsUInt8 i;

    for (i=0; i<4; i++){
        m_global_segment_interest_mask[i] = 0;
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

    m_data_buffer->registerUser(this);

    m_lock = epicsEventCreate(epicsEventFull);
    if (!m_lock) {
        errlogPrintf("Unable to create semaphore lock for %s.\n", deviceName);
        return -2;
    }

    if (m_data_buffer->supportsRx()) {
        //updateThreadName = (char *) malloc((18 + strlen(deviceName)) * sizeof(char));
        m_user_thread.running = false;
        m_user_thread.stop = false;
        /*strcat(updateThreadName, updateThreadPrefix);
        strcat(updateThreadName, deviceName);*/
        m_user_thread.id = epicsThreadCreate("MRF DBUFF UPDATE"
                                          ,epicsThreadPriorityLow
                                          ,epicsThreadGetStackSize(epicsThreadStackMedium)
                                          ,&mrmDataBufferUser::userUpdateThread
                                          ,this);
        if(!m_user_thread.id) {
            errlogPrintf("Unable to create data buffer Rx update thread for %s.\n", deviceName);
            return -3;
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
    struct timespec t, dummy;
    t.tv_nsec = 1000;
    t.tv_sec = 0;

    m_user_thread.stop = true;
    epicsMutexMustLock(m_user_thread.lock);
    if(m_user_thread.running) epicsThreadResume(m_user_thread.id);
    epicsMutexUnlock(m_user_thread.lock);

    if(m_data_buffer != NULL) m_data_buffer->removeUser(this);
    epicsEventDestroy(m_lock);

    for(size_t i = 0; i < m_rx_callbacks.size(); i++) {
        delete m_rx_callbacks[i];
    }

    while(m_user_thread.running) nanosleep(&t, &dummy);
}

void mrmDataBufferUser::registerInterest(size_t offset, size_t length, dataBufferRXCallback_t fptr, void *pvt) {
    RxCallback *cb = NULL;
    epicsUInt16 segment, noOfSegmentsUpdated, segmentOffset;
    epicsUInt32 i;
    epicsUInt8 j;

    /* Check input arguments */
    if(length <= 0) {
        errlogPrintf("Invalid length %d provided. Offset + length should be in interval [0, %d].\n", length, DataTxCtrl_len_max-1);
        return;
    }

    if (offset >= DataTxCtrl_len_max) {
        errlogPrintf("Trying to register on offset %d, which is out of range (max offset: %d).\n", offset, DataTxCtrl_len_max-1);
        return;
    }

    if(offset + length > DataTxCtrl_len_max) {
        dbgPrintf("Trying to register %d bytes on offset %d, which is longer than buffer size (%d). ", length, offset, DataTxCtrl_len_max);
        length = DataTxCtrl_len_max - offset;
        dbgPrintf("Cropped length to %d\n", length);
    }

    segment = offset / DataTxCtrl_segment_bytes;
    segmentOffset = offset - segment * DataTxCtrl_segment_bytes;
    length += segmentOffset;
    noOfSegmentsUpdated = ((length - 1) / DataTxCtrl_segment_bytes) + 1 - (1 / DataTxCtrl_segment_bytes);


    epicsGuard<epicsMutex> g(m_segment_interest_lock);
    for (i=0; i<m_rx_callbacks.size(); i++) {   // is the callback function already registered?
        cb = m_rx_callbacks[i];

        if(cb->fptr == fptr) { // found the same function callback. Stop searching.
            break;
        }
    }

    if(cb == NULL) {    // if callback is not yet registered create a new one.
        cb = new RxCallback;
        cb->fptr = fptr;
        cb->pvt = pvt;
        for (j=0; j<4; j++) {
            cb->segment_interest_mask[j] = 0;
        }

        m_rx_callbacks.push_back(cb);
    }

    // finally update the interest segments
    for (j=segment; j<segment+noOfSegmentsUpdated; j++) {
        cb->segment_interest_mask[j / 32] |= 0x80000000 >> j % 32;
        m_global_segment_interest_mask[j / 32] |= 0x80000000 >> j % 32;
    }
}

void mrmDataBufferUser::deRegisterInterest(size_t offset, size_t length, dataBufferRXCallback_t fptr) {
    RxCallback *cb;
    epicsUInt16 segment, noOfSegmentsUpdated, segmentOffset;
    epicsUInt32 i;
    epicsUInt8 j;
    bool empty;

    /* Check input arguments */
    if(length <= 0) {
        errlogPrintf("Invalid length %d provided. Offset + length should be in interval [0, %d].\n", length, DataTxCtrl_len_max-1);
        return;
    }

    if (offset >= DataTxCtrl_len_max) {
        errlogPrintf("Trying to de-register on offset %d, which is out of range (max offset: %d).\n", offset, DataTxCtrl_len_max-1);
        return;
    }

    if(offset + length > DataTxCtrl_len_max) {
        dbgPrintf("Trying to de-register %d bytes on offset %d, which is longer than buffer size (%d). ", length, offset, DataTxCtrl_len_max);
        length = DataTxCtrl_len_max - offset;
        dbgPrintf("Cropped length to %d\n", length);
    }

    segment = offset / DataTxCtrl_segment_bytes;
    segmentOffset = offset - segment * DataTxCtrl_segment_bytes;
    length += segmentOffset;
    noOfSegmentsUpdated = ((length - 1) / DataTxCtrl_segment_bytes) + 1 - (1 / DataTxCtrl_segment_bytes);

    epicsGuard<epicsMutex> g(m_segment_interest_lock);
    for (i=0; i<m_rx_callbacks.size(); i++) {   // find the callback with the specified registered function
        cb = m_rx_callbacks[i];

        if(cb->fptr == fptr) { // found the same function callback.
            // de-register interest
            for (j=segment; j<segment+noOfSegmentsUpdated; j++) {
                cb->segment_interest_mask[j / 32] &= ~(0x80000000 >> j % 32);
                m_global_segment_interest_mask[j / 32] &= ~(0x80000000 >> j % 32);
            }

            // check if interest mask in now empty
            empty = true;
            for (j=0; j<4; j++) {
                if(cb->segment_interest_mask[j] != 0) empty = false;
            }
            if (empty) {    // interest mask is empty so remove the callback
                m_rx_callbacks.erase(m_rx_callbacks.begin() + i);
            }
        }
    }
}

void mrmDataBufferUser::put(size_t offset, size_t length, void *buffer) {
    epicsUInt16 segment;
    epicsUInt8 i, noOfSegmentsUpdated, segmentOffset;

    /* Check input arguments */
    if(length <= 0) {
        errlogPrintf("Invalid length %d provided. Offset + length should be in interval [0, %d].\n", length, DataTxCtrl_len_max-1);
        return;
    }

    if (offset >= DataTxCtrl_len_max) {
        errlogPrintf("Trying to send on offset %d, which is out of range (max offset: %d).\n", offset, DataTxCtrl_len_max-1);
        return;
    }

    if(offset + length > DataTxCtrl_len_max) {
        dbgPrintf("Too much data to send from offset %d (%d bytes). ", offset, length);
        length = DataTxCtrl_len_max - offset;
        dbgPrintf("Cropped length to %d\n", length);
    }

    segment = offset / DataTxCtrl_segment_bytes;
    segmentOffset = offset - segment * DataTxCtrl_segment_bytes;
    noOfSegmentsUpdated = ((length - 1 + segmentOffset) / DataTxCtrl_segment_bytes) + 1 - (1 / DataTxCtrl_segment_bytes);


    epicsEventWait(m_lock);

    // Update buffer at the offset segment
    for(i=0; i<length; i++) {
        m_buff[offset + i] = ((epicsUInt8 *)buffer)[i];
    }

    // Mark which segments were updated
    for(i=segment; i<segment+noOfSegmentsUpdated; i++){
        m_tx_segments[i / 32] |= 0x80000000 >> i % 32;
    }

    epicsEventSignal(m_lock);
}

void mrmDataBufferUser::get(size_t offset, size_t length, void *buffer) {
    epicsUInt16 segment;
    epicsUInt8 i, noOfSegmentsUpdated, segmentOffset;

    /* Check input arguments */
    if(length <= 0) {
        errlogPrintf("Invalid length %d provided. Offset + length should be in interval [0, %d].\n", length, DataTxCtrl_len_max-1);
        return;
    }

    if (offset >= DataTxCtrl_len_max) {
        errlogPrintf("Trying to receive from offset %d, which is out of range (max offset: %d).\n", offset, DataTxCtrl_len_max-1);
        return;
    }

    if(offset + length > DataTxCtrl_len_max) {
        dbgPrintf("Too much data to receive from offset %d (%d bytes). ", offset, length);
        length = DataTxCtrl_len_max - offset;
        dbgPrintf("Cropped length to %d\n", length);
    }

    segment = offset / DataTxCtrl_segment_bytes;
    segmentOffset = offset - segment * DataTxCtrl_segment_bytes;
    noOfSegmentsUpdated = ((length - 1 + segmentOffset) / DataTxCtrl_segment_bytes) + 1 - (1 / DataTxCtrl_segment_bytes);


    epicsEventWait(m_lock);

    // Copy from offset to user buffer
    for(i=0; i<length; i++) {
        ((epicsUInt8 *)buffer)[i] = m_buff[offset + i];
    }

    // Mark which segments were read
    for(i=segment; i<segment+noOfSegmentsUpdated; i++){
        m_rx_segments[i / 32] &= ~(0x80000000 >> i % 32);
    }

    epicsEventSignal(m_lock);
}

void mrmDataBufferUser::send(bool wait)
{
    if (m_data_buffer != NULL) {
        epicsEventWait(m_lock);
        sendDataBuffer();
        epicsEventSignal(m_lock);

        if(wait) m_data_buffer->waitForTxComplete();
    } else {
        errlogPrintf("Cannot send since data buffer is not available!\n");
    }
}

bool mrmDataBufferUser::trySend(bool wait)
{
    if (m_data_buffer != NULL) {
        if (epicsEventTryWait(m_lock) == epicsEventWaitOK) {
            sendDataBuffer();
            epicsEventSignal(m_lock);

            if(wait) m_data_buffer->waitForTxComplete();
            return true;
        }
    } else {
        errlogPrintf("Cannot send since data buffer is not available!\n");
    }
    return false;
}

void mrmDataBufferUser::updateSegment(epicsUInt16 *segment, epicsUInt8 *data, epicsUInt16 *length) {
    epicsUInt32 segmentMask;
    epicsUInt16 i;
    epicsUInt8 noOfSegmentsUpdated;

    if (epicsEventWaitWithTimeout(m_lock, 1e-6) == epicsEventWaitOK) {

        // Copy received segment(s) to local buffer. If the epicsEventWaitWithTimeout expires the buffer is out of sync...
        for(i=*segment*DataTxCtrl_segment_bytes; i<*length+*segment*DataTxCtrl_segment_bytes; i+=4) {
            *(epicsUInt32*)(m_buff + i) = *(epicsUInt32*)(data + i);
        }

        // This destroys data inserted by put() and not yet sent.
        // Copy entire buffer, because we might have mised and update because of m_lock time out.
        // could be (allmost) solved with a boolean...
        /*for(i=0; i<DataTxCtrl_len_max; i+=4) {
            *(epicsUInt32*)(m_buff + i) = *(epicsUInt32*)(data + i);
        }*/

        // Set segment received flags
        noOfSegmentsUpdated = ((*length - 1) / DataTxCtrl_segment_bytes) + 1 - (1 / DataTxCtrl_segment_bytes);

        for(i=*segment; i<*segment+noOfSegmentsUpdated; i++){
            segmentMask = (0x80000000 >> i % 32);
            if (m_rx_segments[i / 32] & segmentMask & m_global_segment_interest_mask[i / 32]) {
                dbgPrintf("SW overflow occured for segment %d\n", i);
            }
            m_rx_segments[i / 32] |= segmentMask;
        }

        epicsEventSignal(m_lock);

        // Wake up thread for updating user data.
        if (!m_user_thread.stop) {
            m_user_thread.update = true;
            epicsThreadResume(m_user_thread.id);
        }
    } else {
        errlogPrintf("Loosing data: Timed-out while waiting for access to the buffer. User operations on the buffer take too long?\n");
    }
}

bool mrmDataBufferUser::supportsRx()
{
    if(m_data_buffer != NULL) return m_data_buffer->supportsRx();
    return false;
}

bool mrmDataBufferUser::supportsTx()
{
    if(m_data_buffer != NULL) return m_data_buffer->supportsTx();
    return false;
}

void mrmDataBufferUser::userUpdateThread(void* args) {
    epicsUInt32 i, segments;
    epicsUInt8 j, bits, segmentNumber=0;
    mrmDataBufferUser* parent = static_cast<mrmDataBufferUser*>(args);

    parent->m_user_thread.running = true;

    epicsThreadSuspendSelf();
    while (!parent->m_user_thread.stop) {
        parent->m_user_thread.update = false;

        for (i=0; i<parent->m_rx_callbacks.size(); i++) {
            RxCallback *userCallback = parent->m_rx_callbacks[i];

            for (j=0; j<4; j++) {
                segments = userCallback->segment_interest_mask[j] & parent->m_rx_segments[j];
                bits = 32;
                while(bits){
                    if (segments & 0x80000000) {    // user is interested in this segment
                        segmentNumber = 32 * i + 32 - bits;
                        userCallback->fptr(segmentNumber * DataTxCtrl_segment_bytes, userCallback->pvt);
                    }
                    segments <<= 1;
                    bits--;
                }
            }
        }

        if (parent->m_user_thread.update == false) {
            epicsThreadSuspendSelf();
        }
    }
    epicsMutexMustLock(parent->m_user_thread.lock);
    parent->m_user_thread.running = false;
    epicsMutexUnlock(parent->m_user_thread.lock);
}

void mrmDataBufferUser::sendDataBuffer()
{
    epicsUInt32 segments;
    epicsUInt8 i, bits, startSegment;
    epicsUInt16 length = 0;

    for(i=0; i<4; i++){
        segments = m_tx_segments[i];
        bits = 32;
        while(bits){
            if (segments & 0x80000000) {    // we found segment that needs sending
                if (length == 0) startSegment = 32 * i + 32 - bits;
                length += 16;

            } else {    // this segment will not be sent
                if (length > 0) {   // previous segment was last consecutive segment to send.
                    m_data_buffer->send(startSegment, length, &m_buff[startSegment*DataTxCtrl_segment_bytes]);
                }
                length = 0;
            }
            segments <<= 1;
            bits--;
        }
    }

    if (length > 0) {   // segment(s) at the end of the buffer were changed. Send them.
        m_data_buffer->send(startSegment, length, &m_buff[startSegment*DataTxCtrl_segment_bytes]);
    }
}
