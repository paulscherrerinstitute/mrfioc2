#include <vector>
#include <stdio.h>
#include <algorithm>  // for remove()
#include <string.h>     // for memcpy

#include <epicsGuard.h>
#include <callback.h>
#include <epicsExport.h>
#include <epicsMMIO.h>
#include <errlog.h>

#include "mrmDataBuffer.h"
#include "mrmDataBufferUser.h"



int drvMrfiocDataBufferDebug = 0;
epicsExportAddress(int, drvMrfiocDataBufferDebug);

#if defined __GNUC__ && __GNUC__ < 3
#define dbgPrintf(args...)  if(drvMrfiocDataBufferDebug) printf(args);
#else
#define dbgPrintf(...)  if(drvMrfiocDataBufferDebug) printf(__VA_ARGS__);
#endif

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
    enableRx(1);
    enableTx(1);
    /*if(ctrlRegRx != 0 && dataRegRx != 0){

        // Enable segment Rx IRQ
        nat_iowrite32(base+DataBuffer_SegmentIRQ, 0x7FFFFFFF);
        nat_iowrite32(base+DataBuffer_SegmentIRQ+4, 0xFFFFFFFF);
        nat_iowrite32(base+DataBuffer_SegmentIRQ+8, 0xFFFFFFFF);
        nat_iowrite32(base+DataBuffer_SegmentIRQ+12, 0xFFFFFFFF);

        clearFlags(base+DataBufferFlags_rx);

    }*/
}

mrmDataBuffer::~mrmDataBuffer() {
    enableRx(0);
    enableTx(0);
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
    dbgPrintf("Waiting for TX to complete took %d iterations (waiting for maximum of %d iterations)\n", i, TX_WAIT_MAX_ITERATIONS);
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
    dbgPrintf("Waiting while TX is running took %d iterations (waiting for maximum of %d iterations)\n", i, TX_WAIT_MAX_ITERATIONS);
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
        dbgPrintf("Too much data to send from offset %d (%d bytes). ", offset, length);
        length = DataBuffer_len_max - offset;
        dbgPrintf("Sending only %d bytes!\n", length);
    }

    if (length % 4 != 0) {
        dbgPrintf("Data length (%d) is not a multiple of 4, cropping to", length);
        length &= DataTxCtrl_len_mask;
        dbgPrintf(" %d\n", length);
    }

    /* TODO: Check if delay compensation is active? Does it affect length? Will see when data buffer HW is fixed...*/

    /* Send data */
    if (!waitWhileTxRunning()) return false;

    memcpy((epicsUInt8 *)(dataRegTx+base+offset), data, length);

    setTxLength(&startSegment, &length);    // This function can be overriden to calculate different length (used for non-segmented implementation)
    dbgPrintf("Triggering transmision: 0x%x => ", (epicsUInt32)length|DataTxCtrl_trig|((epicsUInt32)startSegment << DataTxCtrl_saddr_shift));

    reg = nat_ioread32(base+ctrlRegTx);
    reg &= ~(DataTxCtrl_len_mask); //clear length
    reg &= ~(DataTxCtrl_saddr_mask); // clear segment address
    reg |= (epicsUInt32)length|DataTxCtrl_trig|((epicsUInt32)startSegment << DataTxCtrl_saddr_shift); // set length and segment addres and trigger sending.
    nat_iowrite32(base+ctrlRegTx, reg);

    dbgPrintf("0x%x\n", nat_ioread32(base+ctrlRegTx));

    return true;
}

void mrmDataBuffer::setTxLength(epicsUInt8 *startSegment, epicsUInt16 *length)
{
    // Length is ok. Non segmented implementation uses different length...
}

void mrmDataBuffer::registerUser(mrmDataBufferUser *user){
    epicsGuard<epicsMutex> g(m_rx_lock);
    m_users.push_back(user);
}

void mrmDataBuffer::removeUser(mrmDataBufferUser *user)
{
    epicsGuard<epicsMutex> g(m_rx_lock);
    m_users.erase(std::remove(m_users.begin(), m_users.end(), user), m_users.end());
}

/*void mrmDataBuffer::setSegmentIRQ(epicsUInt8 i, epicsUInt32 mask)
{
    printFlags("Segment a", base+DataBuffer_SegmentIRQ);
    nat_iowrite32(base+DataBuffer_SegmentIRQ + 4*i, mask);
    printFlags("Segment b", base+DataBuffer_SegmentIRQ);
}

void mrmDataBuffer::receive()
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
}*/

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

    return 0;// TODO because v300 hardware does not work correctly yet

    for(i=0; i<4; i++){
        if(m_rx_flags[i] != 0){  // do we have Rx flag in this 32 segment bits?
            while(!(m_rx_flags[i] & 0x80000000)) {   // do a bit search for the Rx flag, since we know at least one bit is set
                m_rx_flags[i] <<= 1;
                firstSegment++;
            }
        } else {    // read next 32 bits and check for Rx flag there.
            firstSegment += 32;
        }
    }

    if(firstSegment >= 128) {    // no Rx flag set, but interrupt triggered. We are probbably on firmware version <200, so return segment/address 0
        return 0;
    }

    return firstSegment;
}

bool mrmDataBuffer::overflowOccured(epicsUInt16 segment) {
    epicsUInt8 registerOffset, segmentOffset;

    return 0;   // TODO because v300 hardware does not work correctly yet
    registerOffset = segment / 32;
    segmentOffset  = segment % 32; // the bit number of the segment in the register

    return (m_overflows[registerOffset] & (0x80000000 >> segmentOffset)) != 0;
}

bool mrmDataBuffer::checksumError(epicsUInt16 segment) {
    epicsUInt8 registerOffset, segmentOffset;

    return 0;   // TODO because v300 hardware does not work correctly yet
    registerOffset = segment / 32;
    segmentOffset  = segment % 32; // the bit number of the segment in the register

    return (m_checksums[registerOffset] & (0x80000000 >> segmentOffset)) != 0;
}

/**
 * No need to lock in this function, since we enable the reception at the end.
 * Thus, another reception cannot occur before this function is finished
 */
void mrmDataBuffer::handleDataBufferRxIRQ(CALLBACK *cb) {
    void *vptr;
    callbackGetUser(vptr,cb);
    mrmDataBuffer* parent = static_cast<mrmDataBuffer*>(vptr);
    epicsUInt16 i, segment, length;
    epicsUInt32 sts = nat_ioread32(parent->base+parent->ctrlRegRx);

    epicsGuard<epicsMutex> g(parent->m_rx_lock);

    if (sts & DataRxCtrl_rx) {
        errlogPrintf("Interrupt triggered but Rx not completed. Should never happen, fatal error!\n  Control register status: 0x%x\n", sts);
        return;
    }

    memcpy(parent->m_overflows, (epicsUInt8 *)(parent->base+DataBufferFlags_overflow), 16);
    memcpy(parent->m_checksums, (epicsUInt8 *)(parent->base+DataBufferFlags_cheksum),  16);
    memcpy(parent->m_rx_flags,  (epicsUInt8 *)(parent->base+DataBufferFlags_rx),       16);

    //if((sts & DataRxCtrl_len_mask) > 8)parent->printBinary("Control", sts); // applies to fw201. If len=8 we are (likely) only receiving dly.comp. If we send some actual data len > 8.

    if (sts&DataRxCtrl_sumerr) {
        errlogPrintf("RX: Checksum error\n"); // TODO when is global / segment checksum error bit set?? Should be moved to checksumError()

    } else if((sts & DataRxCtrl_len_mask) > 8){    // TODO: only temporary. Do not receive if only delay compensation is sent.

        /*parent->printFlags("Segment", parent->base+DataBuffer_SegmentIRQ);
        parent->printFlags("Checksum", (epicsUInt8 *)parent->m_checksums);
        parent->printFlags("Overflow", (epicsUInt8 *)parent->m_overflows);
        parent->printFlags("Rx", (epicsUInt8 *)parent->m_rx_flags);*/

        length = sts & DataRxCtrl_len_mask;
        segment = parent->getFirstReceivedSegment();

        if (parent->checksumError(segment) != 0) {
            errlogPrintf("Checksum error occured for segment %d.\n", segment);
            return;
        }

        if(parent->overflowOccured(segment) != 0) errlogPrintf("HW overflow occured for segment %d\n", segment);

        dbgPrintf("Rx segment+len: %d + %d\n", segment, length);

        // Dispatch the buffer to users
        if(parent->m_users.size() <= 0) {
            return;
        }
        else {
            // copy the data to local buffer
            memcpy(&parent->m_rx_buff[segment*DataBuffer_segment_length], (epicsUInt8 *)(parent->base + parent->dataRegRx + segment*DataBuffer_segment_length), length);

            if(drvMrfiocDataBufferDebug){
                for(i=segment*DataBuffer_segment_length; i<length+segment*DataBuffer_segment_length; i++) {
                    if(!(i%16)) printf(" | ");
                    else if(!(i%4)) printf(", ");
                    printf("%d ", parent->m_rx_buff[i]);
                }
                printf("\n");
            }


            // clear Rx flags for the data we have just received
            memcpy((epicsUInt8 *)(parent->base+DataBufferFlags_rx), parent->m_rx_flags, 16);

            for(i=0; i<parent->m_users.size(); i++) {
                parent->m_users[i]->updateSegment(segment, parent->m_rx_buff, length);
            }
        }
    }

    nat_iowrite32(parent->base+parent->ctrlRegRx, sts|DataRxCtrl_rx);    // enable for next reception
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
