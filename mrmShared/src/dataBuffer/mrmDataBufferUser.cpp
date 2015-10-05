#include <time.h>

#include "evgMrm.h"
#include "drvem.h"

#include "mrmDataBufferUser.h"

mrmDataBufferUser::mrmDataBufferUser(const char* deviceName)
{
    /*char * updateThreadName;
    char updateThreadPrefix[] = "MRF DBUFF UPDATE ";*/
    epicsUInt8 i;

    for (i=0; i<4; i++){
        globalSegmentInterestMask[i] = 0;
        m_rx_segments[i] = 0;
        m_tx_segments[i] = 0;
    }
    dataBuffer = getDataBufferFromDevice(deviceName);
    if(dataBuffer == NULL) {
        printf("Data buffer for %s not found.\n", deviceName);
    } else {
        dataBuffer->registerUser(this);

        m_lock = epicsEventCreate(epicsEventFull);  // TODO this call can fail...

        if (dataBuffer->supportsRx()) {
            //updateThreadName = (char *) malloc((18 + strlen(deviceName)) * sizeof(char));
            userThread.running = false;
            userThread.stop = false;
            /*strcat(updateThreadName, updateThreadPrefix);
            strcat(updateThreadName, deviceName);*/
            userThread.id = epicsThreadCreate("MRF DBUFF UPDATE"
                                              ,epicsThreadPriorityLow
                                              ,epicsThreadGetStackSize(epicsThreadStackMedium)
                                              ,&mrmDataBufferUser::userUpdateThread
                                              ,this); // TODO this can fail...
        }
    }
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

    userThread.stop = true;
    if(userThread.running) epicsThreadResume(userThread.id);
    if(dataBuffer != NULL) dataBuffer->removeUser(this);

    for(size_t i = 0; i < m_rx_callbacks.size(); i++) {
        delete m_rx_callbacks[i];
    }

    while(userThread.running) nanosleep(&t, &dummy);
}

void mrmDataBufferUser::registerInterest(size_t offset, size_t length, DataBufferRXCallback_t fptr, void *pvt) {
    RxCallback *cb = NULL;
    epicsUInt16 segment, noOfSegmentsUpdated, segmentOffset;
    epicsUInt32 i;
    epicsUInt8 j;


    if (length > 0) {
        segment = offset / DataTxCtrl_segment_bytes;
        segmentOffset = offset - segment * DataTxCtrl_segment_bytes;
        length += segmentOffset;
        noOfSegmentsUpdated = ((length - 1) / DataTxCtrl_segment_bytes) + 1 - (1 / DataTxCtrl_segment_bytes);

        epicsGuard<epicsMutex> g(segmentInterest_lock);
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
            globalSegmentInterestMask[j / 32] |= 0x80000000 >> j % 32;
        }

    }
}

void mrmDataBufferUser::deRegisterInterest(size_t offset, size_t length, DataBufferRXCallback_t fptr) {
    RxCallback *cb;
    epicsUInt16 segment, noOfSegmentsUpdated, segmentOffset;
    epicsUInt32 i;
    epicsUInt8 j;
    bool empty;


    if (length > 0) {
        segment = offset / DataTxCtrl_segment_bytes;
        segmentOffset = offset - segment * DataTxCtrl_segment_bytes;
        length += segmentOffset;
        noOfSegmentsUpdated = ((length - 1) / DataTxCtrl_segment_bytes) + 1 - (1 / DataTxCtrl_segment_bytes);

        printf("Deregistering...");
        epicsGuard<epicsMutex> g(segmentInterest_lock);
        for (i=0; i<m_rx_callbacks.size(); i++) {   // find the callback with the specified registered function
            cb = m_rx_callbacks[i];

            if(cb->fptr == fptr) { // found the same function callback.
                // de-register intrest
                for (j=segment; j<segment+noOfSegmentsUpdated; j++) {
                    printf("segment %d, ",j/32);
                    cb->segment_interest_mask[j / 32] &= ~(0x80000000 >> j % 32);
                    globalSegmentInterestMask[j / 32] &= ~(0x80000000 >> j % 32);
                }
                printf("...");

                // check if interest mask in now empty
                empty = true;
                for (j=0; j<4; j++) {
                    if(cb->segment_interest_mask[j] != 0) empty = false;
                }
                if (empty) {    // interest mask is empty so remove the callback
                    printf("removing callback");
                    m_rx_callbacks.erase(m_rx_callbacks.begin() + i);
                }
            }
        }
        printf("\n");
    }
}

void mrmDataBufferUser::put(size_t offset, size_t length, void *buffer) {
    epicsUInt16 segment;
    epicsUInt8 i, noOfSegmentsUpdated, segmentOffset;

    if(length <= 0) return;

    segment = offset / DataTxCtrl_segment_bytes;
    segmentOffset = offset - segment * DataTxCtrl_segment_bytes;
    //length += segmentOffset;
    noOfSegmentsUpdated = ((length - 1 + segmentOffset) / DataTxCtrl_segment_bytes) + 1 - (1 / DataTxCtrl_segment_bytes);

    epicsEventWait(m_lock);

    // Update buffer at the offset segment
    for(i=0; i<length; i++) {
        m_buff[offset + i] = ((epicsUInt8 *)buffer)[i];
    }
    /*for(i=0; i<length; i+=4) {
        *(epicsUInt32*)(m_buff + offset + i) = *(epicsUInt32*)((epicsUInt8 *)buffer + i);
        //printf("put: %d %d %d %d, ", m_buff + offset + i,m_buff + offset + i+1, m_buff + offset + i+2, m_buff + offset + i +3);
    }*/
    //printf("\n");

    // Mark which segments were updated
    for(i=segment; i<segment+noOfSegmentsUpdated; i++){
        m_tx_segments[i / 32] |= 0x80000000 >> i % 32;
    }

    epicsEventSignal(m_lock);
}

void mrmDataBufferUser::get(size_t offset, size_t length, void *buffer) {
    epicsUInt16 segment;
    epicsUInt8 i, noOfSegmentsUpdated, segmentOffset;

    if(length <= 0) return;

    segment = offset / DataTxCtrl_segment_bytes;
    segmentOffset = offset - segment * DataTxCtrl_segment_bytes;
    //length += segmentOffset;
    noOfSegmentsUpdated = ((length - 1 + segmentOffset) / DataTxCtrl_segment_bytes) + 1 - (1 / DataTxCtrl_segment_bytes);

    epicsEventWait(m_lock);

    // Copy from offset to user buffer
    for(i=0; i<length; i++) {
        ((epicsUInt8 *)buffer)[i] = m_buff[offset + i];
    }
    /*for(i=0; i<length; i+=4) {
        *(epicsUInt32*)((epicsUInt8 *)buffer + i) = *(epicsUInt32*)(m_buff + offset + i);
    }*/

    // Mark which segments were read
    for(i=segment; i<segment+noOfSegmentsUpdated; i++){
        m_rx_segments[i / 32] &= ~(0x80000000 >> i % 32);
    }

    epicsEventSignal(m_lock);
}

void mrmDataBufferUser::send(bool wait)
{
    if (dataBuffer != NULL) {
        epicsEventWait(m_lock);
        sendDataBuffer();
        epicsEventSignal(m_lock);

        if(wait) dataBuffer->waitForTxComplete();
    } else {
        printf("Cannot send since data buffer is not available!\n");
    }
}

bool mrmDataBufferUser::trySend(bool wait)
{
    if (dataBuffer != NULL) {
        if (epicsEventTryWait(m_lock) == epicsEventWaitOK) {
            sendDataBuffer();
            epicsEventSignal(m_lock);

            if(wait && dataBuffer != NULL) dataBuffer->waitForTxComplete();
            return true;
        }
    } else {
        printf("Cannot send since data buffer is not available!\n");
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
            if (m_rx_segments[i / 32] & segmentMask & globalSegmentInterestMask[i / 32]) {
                printf("SW overflow occured for segment %d\n", i);
            }
            m_rx_segments[i / 32] |= segmentMask;
        }

        epicsEventSignal(m_lock);

        /*if(!userThread.running) {
            userThread.id = epicsThreadCreate("MRF DBUFF UPDATE",epicsThreadPriorityLow,epicsThreadGetStackSize(epicsThreadStackMedium),&mrmDataBufferUser::userUpdateThread,this);
        }*/
        // Wake up thread for updating user data.
        epicsThreadResume(userThread.id);
    } else {
        printf("Loosing data: Timed-out while waiting for access to the buffer. User operations on the buffer take too long?\n");
    }
}

bool mrmDataBufferUser::supportsRx()
{
    if(dataBuffer != NULL) return dataBuffer->supportsRx();
    return false;
}

bool mrmDataBufferUser::supportsTx()
{
    if(dataBuffer != NULL) return dataBuffer->supportsTx();
    return false;
}

void mrmDataBufferUser::userUpdateThread(void* args){

    epicsUInt32 i, segments;
    epicsUInt8 j, bits, segmentNumber=0;
    mrmDataBufferUser* parent = static_cast<mrmDataBufferUser*>(args);

    parent->userThread.running = true;
    printf("User update thread started\n");

    epicsThreadSuspendSelf();
    while(!parent->userThread.stop){
        printf("User update thread running callbacks (max %d)\n", parent->m_rx_callbacks.size());
        for(i=0; i<parent->m_rx_callbacks.size(); i++){
            RxCallback *userCallback = parent->m_rx_callbacks[i];

            for(j=0; j<4; j++){
                segments = userCallback->segment_interest_mask[j] & parent->m_rx_segments[j];
                //printf("callback: seg: 0x%x = 0x%x & 0x%x\n", segments, userCallback->segment_interest_mask[j], parent->m_rx_segments[j]);
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
        printf("User update thread going to sleep\n");
        //TODO if triggered again while doing callbacks the threadwill suspend here, instead of rechecking all the callbacks...probbably
        epicsThreadSuspendSelf();
    }
    parent->userThread.running = false;
    printf("User update thread exited\n");
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
                    dataBuffer->send(startSegment, length, &m_buff[startSegment*DataTxCtrl_segment_bytes]);
                }
                length = 0;
            }
            segments <<= 1;
            bits--;
        }
    }

    if (length > 0) {   // segment(s) at the end of the buffer were changed. Send them.
        dataBuffer->send(startSegment, length, &m_buff[startSegment*DataTxCtrl_segment_bytes]);
    }
}
