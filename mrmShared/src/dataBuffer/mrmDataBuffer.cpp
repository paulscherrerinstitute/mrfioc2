#include <vector>
#include <stdio.h>
#include <algorithm>  // for remove()
#include <string.h>     // for memcpy

#include <epicsGuard.h>
#include <callback.h>
#include <epicsExport.h>
#include <epicsMMIO.h>
#include <errlog.h>

#include "mrmShared.h"
#include "mrmDataBuffer.h"

#include "../../evgMrmApp/src/evgMrm.h"
#include "../../evrMrmApp/src/evrMrm.h"


int drvMrfiocDataBufferDebug = 0;
epicsExportAddress(int, drvMrfiocDataBufferDebug);

#define TX_WAIT_MAX_ITERATIONS 1000     // guard against infinite loop when waiting for Tx to complete / waiting while Tx is running.


mrmDataBuffer::mrmDataBuffer(volatile epicsUInt8 *parentBaseAddress,
                             epicsUInt32 controlRegisterTx,
                             epicsUInt32 controlRegisterRx,
                             epicsUInt32 dataRegisterTx,
                             epicsUInt32 dataRegisterRx)
    :base(parentBaseAddress)
    ,ctrlRegTx(controlRegisterTx)
    ,ctrlRegRx(controlRegisterRx)
    ,dataRegTx(dataRegisterTx)
    ,dataRegRx(dataRegisterRx)
{
    epicsUInt16 i;

    enableRx(1);
    enableTx(1);

    for (i=0; i<4; i++) {
        m_irq_flags[i] = 0;
    }

    m_rx_irq_handled = true;
}

mrmDataBuffer::~mrmDataBuffer() {
    size_t i;

    enableRx(0);
    enableTx(0);

    for (i=0; i<m_users.size(); i++) {
        delete m_users[i];
    }
}

void mrmDataBuffer::enableRx(bool en)
{
    epicsUInt32 reg;

    if(supportsRx()) {
        epicsGuard<epicsMutex> g(m_rx_lock);
        reg = nat_ioread32(base+ctrlRegRx);
        if(en) {
            reg |= DataRxCtrl_mode|DataRxCtrl_rx; // Set mode to DBUS+data buffer and set up buffer for reception
        } else {
            reg |= DataRxCtrl_stop;    // stop reception
            reg &= ~DataRxCtrl_mode;   // set mode to DBUS only (no effect on firmware 200+)
        }
        nat_iowrite32(base+ctrlRegRx, reg);

        clearFlags(base+DataBufferFlags_rx);    // also clear Rx flags (and consequently checksum+overflow flags)
    }
}

bool mrmDataBuffer::enabledRx()
{
    if(supportsRx()) return (nat_ioread32(base + ctrlRegRx) & DataRxCtrl_mode) != 0;    // check if in DBUS+data buffer mode
    return 0;
}

void mrmDataBuffer::enableTx(bool en)
{
    epicsUInt32 reg, mask;

    if(supportsTx()) {
        epicsGuard<epicsMutex> g(m_tx_lock);

        mask = DataTxCtrl_ena|DataTxCtrl_mode;  // enable Tx engine and set DBUS+data buffer mode

        reg = nat_ioread32(base+ctrlRegTx);
        if(en)
            reg |= mask;
        else
            reg &= ~mask;
        nat_iowrite32(base+ctrlRegTx, reg);
    }
}

bool mrmDataBuffer::enabledTx()
{
    if(supportsTx()) return (nat_ioread32(base+ctrlRegTx) & (DataTxCtrl_ena|DataTxCtrl_mode)) != 0;
    return 0;
}

bool mrmDataBuffer::supportsRx()
{
    return (ctrlRegRx > 0) && (dataRegRx > 0);
}

bool mrmDataBuffer::supportsTx()
{
    return (ctrlRegTx > 0) && (dataRegTx > 0);
}

bool mrmDataBuffer::waitForTxComplete(){
    epicsUInt32 i=0;

    // Actual sending is so fast that we can use busy wait here
    while (!(nat_ioread32(base+ctrlRegTx)&DataTxCtrl_done) && i < TX_WAIT_MAX_ITERATIONS) {
        i++;
    }
    dataBuffer_debug(1, "Waiting for TX to complete took %d iterations (waiting for maximum of %d iterations)\n", i, TX_WAIT_MAX_ITERATIONS);
    if (i >= TX_WAIT_MAX_ITERATIONS) {
        return false;
    }
    return true;
}

bool mrmDataBuffer::waitWhileTxRunning(){
    epicsUInt32 i=0;

    // Actual sending is so fast that we can use busy wait here
    while ((nat_ioread32(base+ctrlRegTx)&DataTxCtrl_run)  && i < TX_WAIT_MAX_ITERATIONS) {
        i++;
    }
    dataBuffer_debug(1, "Waiting while TX is running took %d iterations (waiting for maximum of %d iterations)\n", i, TX_WAIT_MAX_ITERATIONS);
    if (i >= TX_WAIT_MAX_ITERATIONS) {
        errlogPrintf("Waiting while Tx running takes too long. Forced exit...\n");
        return false;
    }
    return true;
}

bool mrmDataBuffer::send(epicsUInt8 startSegment, epicsUInt16 length, epicsUInt8 *data){
    epicsUInt32 offset, reg;

    epicsGuard<epicsMutex> g(m_tx_lock);

    if (!enabledTx()) {
        errlogPrintf("Sending is not enabled, aborting...\n");
        return false;
    }

    /* Check input arguments */
    offset = startSegment*DataBuffer_segment_length;
    if (offset >= DataBuffer_len_max) {
        errlogPrintf("Trying to send on offset %d, which is out of range (max offset: %d). Sending aborted!\n", offset, DataBuffer_len_max-1);
        return false;
    }

    if (length + offset > DataBuffer_len_max) {
        dataBuffer_debug(1, "Too much data to send from offset %d (%d bytes). ", offset, length);
        length = DataBuffer_len_max - offset;
        dataBuffer_debug(1, "Sending only %d bytes!\n", length);
    }

    if (length % 4 != 0) {
        dataBuffer_debug(1, "Data length (%d) is not a multiple of 4, cropping to", length);
        length &= DataTxCtrl_len_mask;
        dataBuffer_debug(1, " %d\n", length);
    }

    /* TODO: Check if delay compensation is active? Does it affect length? Will see when data buffer HW is fixed...*/
    // TODO check if length=0 means 0 or 4 bytes

    /* Send data */
    if (!waitWhileTxRunning()) return false;

    memcpy((epicsUInt8 *)(dataRegTx+base+offset), data, length);

    length += startSegment * DataBuffer_segment_length;   // This is for 230 series. Segmented data buffer doesn't use length anyway...
    //setTxLength(&startSegment, &length);    // This function can be overriden to calculate different length (used for non-segmented implementation)
    dataBuffer_debug(1, "Triggering transmision: 0x%x => ", (epicsUInt32)length|DataTxCtrl_trig|((epicsUInt32)startSegment << DataTxCtrl_saddr_shift));

    reg = nat_ioread32(base+ctrlRegTx);
    reg &= ~(DataTxCtrl_len_mask); //clear length
    reg &= ~(DataTxCtrl_saddr_mask); // clear segment address
    reg |= (epicsUInt32)length|DataTxCtrl_trig|((epicsUInt32)startSegment << DataTxCtrl_saddr_shift); // set length and segment addres and trigger sending.
    nat_iowrite32(base+ctrlRegTx, reg);

    dataBuffer_debug(1, "0x%x\n", nat_ioread32(base+ctrlRegTx));

    return true;
}

void mrmDataBuffer::registerUser(mrmDataBufferUser *user){
    epicsUInt16 i;

    epicsGuard<epicsMutex> g(m_rx_lock);

    Users *newUser = new Users;
    newUser->user = user;
    for (i=0; i<4; i++) {
        newUser->segments[i] = 0;
    }
    m_users.push_back(newUser);
}

void mrmDataBuffer::removeUser(mrmDataBufferUser *user)
{
    size_t size, i;
    epicsUInt16 j;

    epicsGuard<epicsMutex> g(m_rx_lock);
    //m_users.erase(std::remove(m_users.begin(), m_users.end(), user), m_users.end());

    for (j=0; j<4; j++) {
        m_irq_flags[j] = 0;
    }

    size = m_users.size();
    for (i=0; i<size; i++){
        if (m_users[i]->user == user) {   // found a user to remove
            m_users.erase(m_users.begin() + i);

        } else {
            for (j=0; j<4; j++) {
                m_irq_flags[j] |= m_users[i]->segments[j];
            }
        }

    }
    memcpy((epicsUInt8 *)(base+DataBuffer_SegmentIRQ), m_irq_flags, 16);    // set which segments will trigger interrupt when data is received
}

void mrmDataBuffer::setInterest(mrmDataBufferUser *user, epicsUInt32 *interest)
{
    size_t size, i;
    epicsUInt16 j;

    epicsGuard<epicsMutex> g(m_rx_lock);

    for (j=0; j<4; j++) {
        m_irq_flags[j] = 0;
    }

    size = m_users.size();
    for (i=0; i<size; i++){
        if (m_users[i]->user == user) {   // found a user to set segment interest for
            for (j=0; j<4; j++) {
                m_users[i]->segments[j] = interest[j];  // we trust the length of interest is OK
                m_irq_flags[j] |= interest[j];
            }
        } else {
            for (j=0; j<4; j++) {
                m_irq_flags[j] |= m_users[i]->segments[j];
            }
        }
    }
    //printFlags("set interest IRQ", (epicsUInt8 *)m_irq_flags);
    memcpy((epicsUInt8 *)(base+DataBuffer_SegmentIRQ), m_irq_flags, 16);    // set which segments will trigger interrupt when data is received
}

void mrmDataBuffer::setSegmentIRQ(epicsUInt8 i, epicsUInt32 mask)
{
    //printFlags("Segment a", base+DataBuffer_SegmentIRQ);
    nat_iowrite32(base+DataBuffer_SegmentIRQ + 4*i, mask);
    //printFlags("Segment b", base+DataBuffer_SegmentIRQ);
}

void mrmDataBuffer::setRx(epicsUInt8 i, epicsUInt32 mask)
{
    printFlags("Rx a", base+DataBufferFlags_rx);
    nat_iowrite32(base+DataBufferFlags_rx + 4*i, mask);
    printFlags("Rx b", base+DataBufferFlags_rx);
}

void mrmDataBuffer::ctrlReceive()
{
    epicsUInt32 reg = nat_ioread32(base+ctrlRegRx);
    printf("Ctrl: %x\n", reg);
    nat_iowrite32(base+ctrlRegRx, reg|DataRxCtrl_rx);
    printf("Ctrl: %x\n", nat_ioread32(base+ctrlRegRx));
    printf("\n");
}

void mrmDataBuffer::stop()
{
    epicsUInt32 reg = nat_ioread32(base+ctrlRegRx);
    printf("Ctrl: %x\n", reg);
    nat_iowrite32(base+ctrlRegRx, reg|DataRxCtrl_stop);
    printf("Ctrl: %x\n", nat_ioread32(base+ctrlRegRx));
    printf("\n");
}

void mrmDataBuffer::printRegs()
{
    printFlags("Segment", base+DataBuffer_SegmentIRQ);
    printFlags("Checksum", base + DataBufferFlags_cheksum);
    printFlags("Overflow", base + DataBufferFlags_overflow);
    printFlags("Rx", base + DataBufferFlags_rx);
}

void mrmDataBuffer::clearFlags(volatile epicsUInt8 *flagRegister) {
    int i;

    for(i=0; i<=12; i+=4) {
        nat_iowrite32(flagRegister+i, 0xFFFFFFFF);
    }
}

void mrmDataBuffer::printFlags(const char *preface, volatile epicsUInt8* flagRegister) {
    int i;
    epicsUInt32 segmentIrq;

    printf("\n");
    for(i=0; i<=12; i+=4) {
        segmentIrq = nat_ioread32(flagRegister+i);
        printBinary(preface, segmentIrq);
    }
    printf("\n");
}

epicsUInt16 mrmDataBuffer::getFirstReceivedSegment() {
    epicsUInt16 firstSegment=0;
    epicsUInt8 i;
    epicsUInt32 mask;

    for(i=0; i<4; i++){
        mask = 0x80000000;
        if(m_rx_flags[i] != 0){  // do we have Rx flag in this 32 segment bits?
            while(!(m_rx_flags[i] & mask)) {   // do a bit search for the Rx flag, since we know at least one bit is set
                mask >>= 1;
                firstSegment++;
            }
            //firstSegmentMask[i] = mask;
            break;
        } else {    // read next 32 bits and check for Rx flag there.
            firstSegment += 32;
        }
    }

    return firstSegment;
}

bool mrmDataBuffer::overflowOccured() {
    /*epicsUInt8 registerOffset, segmentOffset;

    registerOffset = segment / 32;
    segmentOffset  = segment % 32; // the bit number of the segment in the register

    return (m_overflows[registerOffset] & (0x80000000 >> segmentOffset)) != 0;*/

    bool overflow = false;
    epicsUInt16 i, segment;

    m_overflows[0] &= 0x7FFFFFFF;  // we don't care about segment 0 == delay compensation
    for (i=0; i<4; i++) {
        segment = 0;
        while (m_overflows[i] !=0 ) {   // overflow occured.
            overflow = true;
            if (m_overflows[i] & 0x80000000) errlogPrintf("HW overflow occured for segment %d\n", i*32 + segment);
            m_overflows[i] <<= 1;
            segment ++;
        }
    }

    return overflow;
}

bool mrmDataBuffer::checksumError() {
    bool checksum = false;
    epicsUInt16 i, segment;

    m_checksums[0] &= 0x7FFFFFFF;  // we don't care about segment 0 == delay compensation
    for (i=0; i<4; i++) {
        segment = 0;
        while (m_checksums[i] !=0 ) {   // checksum occured.
            checksum = true;
            if (m_checksums[i] & 0x80000000) errlogPrintf("Checksum error occured for segment %d.\n", i*32 + segment);
            m_checksums[i] <<= 1;
            segment ++;
        }
    }

    return checksum;
}

// TODO: the way non-segmented implementation overrides stuff is not clean
/**
 * No need to lock in this function, since we enable the reception at the end.
 * Thus, another reception cannot occur before this function is finished
 */
void mrmDataBuffer::handleDataBufferRxIRQ(CALLBACK *cb) {
    void *vptr;
    callbackGetUser(vptr,cb);
    mrmDataBuffer* parent = static_cast<mrmDataBuffer*>(vptr);


    epicsGuard<epicsMutex> g(parent->m_rx_lock);

    parent->receive();
    parent->m_rx_irq_handled = true;
}

void mrmDataBuffer::receive() {
    epicsUInt16 i, segment, length;
    epicsUInt32 sts = nat_ioread32(base+ctrlRegRx);

    //if((sts & DataRxCtrl_len_mask) > 8)printBinary("Control", sts); // applies to fw201+. If len=8 we are (likely) only receiving dly.comp. If we send some actual data len > 8.

    if (sts & DataRxCtrl_rx) {
        errlogPrintf("Interrupt triggered but Rx not completed. Should never happen, fatal error!\n  Control register status: 0x%x\n", sts);
    }
    else {
        memcpy(m_overflows, (epicsUInt8 *)(base+DataBufferFlags_overflow), 16);
        memcpy(m_checksums, (epicsUInt8 *)(base+DataBufferFlags_cheksum),  16);
        memcpy(m_rx_flags,  (epicsUInt8 *)(base+DataBufferFlags_rx),       16);

        /*printFlags("Segment", base+DataBuffer_SegmentIRQ);
        printFlags("Checksum", (epicsUInt8 *)m_checksums);
        printFlags("Overflow", (epicsUInt8 *)m_overflows);
        printFlags("Rx", (epicsUInt8 *)m_rx_flags);*/


        /*length = sts & DataRxCtrl_len_mask;
        segment = getFirstReceivedSegment();
        length -= segment*DataBuffer_segment_length;    // length is always reported from the start of the buffer
        */

        /**
         * length is not reported correctly in 200+ series FW.
         * Example:
         * - send data on segment 6
         * - send data on segment 2
         * - enable IRQ
         * - length reported will be for last received segment == segment 2. We do not know the length of data that was previously received on segment 6
         * For this reason we always read entire buffer.
         **/
        segment = 0;
        length = 0;

        if (checksumError() != 0) { // TODO when is global / segment checksum error bit set??
            memcpy((epicsUInt8 *)(base+DataBufferFlags_rx), m_rx_flags, 16);        // clear Rx flags for the data we have just received
            memcpy((epicsUInt8 *)(base+DataBuffer_SegmentIRQ), m_irq_flags, 16);    // set which segments will trigger interrupt when data is received
            return;
        }

        overflowOccured();

        dataBuffer_debug(1, "Rx segment+len: %d + %d\n", segment, length);

        // Dispatch the buffer to users
        if(m_users.size() > 0) {
            memcpy(&m_rx_buff[segment*DataBuffer_segment_length], (epicsUInt8 *)(base + dataRegRx + segment*DataBuffer_segment_length), length);    // copy the data to local buffer

            if(drvMrfiocDataBufferDebug){
                for(i=segment*DataBuffer_segment_length; i<length+segment*DataBuffer_segment_length; i++) {
                    if(!(i%16)) printf(" | ");
                    else if(!(i%4)) printf(", ");
                    printf("%d ", m_rx_buff[i]);
                }
                printf("\n");
            }

            // clear Rx flags for the data we have just received
            memcpy((epicsUInt8 *)(base+DataBufferFlags_rx), m_rx_flags, 16);

            for(i=0; i<m_users.size(); i++) {
                m_users[i]->user->updateSegment(segment, m_rx_buff, length);
            }
        }
    }
    //printFlags("IRQ", (epicsUInt8 *)m_irq_flags);
    memcpy((epicsUInt8 *)(base+DataBuffer_SegmentIRQ), m_irq_flags, 16);    // set which segments will trigger interrupt when data is received
}

mrmDataBuffer* getDataBufferFromDevice(const char *device) {
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

void mrmDataBuffer::printBinary(const char *preface, epicsUInt32 n) {
    printf("%s: 0x%x =", preface, n);
    int i = 32;

    while (i) {
        if(i%4 == 0) printf(" ");
        if (n & 0x80000000)
            printf("1");
        else
            printf("0");

        n <<= 1;
        i--;
    }
    printf("\n");
}
