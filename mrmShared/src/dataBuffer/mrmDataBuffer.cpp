#include <vector>
#include <stdio.h>
#include <algorithm>
#include <stdexcept>

#include <epicsGuard.h>
#include <callback.h>

//#include <mrfCommonIO.h>
//#include <mrfCommon.h>
#include <epicsMMIO.h>



#include "mrmDataBuffer.h"
#include "mrmDataBufferUser.h"


#define DataTxCtrl_done 0x100000
#define DataTxCtrl_run  0x080000
#define DataTxCtrl_trig 0x040000
#define DataTxCtrl_ena  0x020000
#define DataTxCtrl_mode 0x010000


#define DataTxCtrl_saddr_mask 0xFF000000
#define DataTxCtrl_saddr_shift 24

#define DataBuffer_SegmentIRQ  0x780   //32 bit
#define DataBufferFlags_cheksum 0x7A0   //32 bit, each bit for one segment. 0 = Checksum OK
#define DataBufferFlags_overflow    0x7C0   //32 bit, each bit for one segment.
#define DataBufferFlags_rx  0x7E0   //32 bit



#include "evrRegMap.h"
//#include "evgRegMap.h"


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
    if(ctrlRegRx != 0 && dataRegRx != 0){


        // Enable segment Rx IRQ
        /*nat_iowrite32(base+DataBuffer_SegmentIRQ, 0x7FFFFFFF);
        nat_iowrite32(base+DataBuffer_SegmentIRQ+4, 0xFFFFFFFF);
        nat_iowrite32(base+DataBuffer_SegmentIRQ+8, 0xFFFFFFFF);
        nat_iowrite32(base+DataBuffer_SegmentIRQ+12, 0xFFFFFFFF);

        clearRxFlags();*/

    }
}

mrmDataBuffer::~mrmDataBuffer() {
    /*epicsUInt32 reg;

    reg = nat_ioread32(base+ctrlRegRx);
    reg |= DataBufCtrl_stop;    // stop reception
    nat_iowrite32(base+ctrlRegRx, reg);*/
    enableRx(0);
    enableTx(0);
}

void mrmDataBuffer::enableRx(bool en)
{
    // TODO: Should :
    //          - lock the section
    //          - maybe wait for tx/rx complete

    epicsUInt32 reg;

    if(supportsRx()) {
        //epicsGuard<epicsMutex> g(m_rx_lock);
        reg = nat_ioread32(base+ctrlRegRx);
        if(en) {
            reg |= DataBufCtrl_mode|DataBufCtrl_rx; // Set mode to DBUS+data buffer and set up buffer for reception
        } else {
            reg |= DataBufCtrl_stop;    // stop reception
            reg &= ~DataBufCtrl_mode;   // set mode to DBUS only (no effect on firmware 200+)
        }
        nat_iowrite32(base+ctrlRegRx, reg);
    }
}

bool mrmDataBuffer::enabledRx()
{
    if(supportsRx()) return (nat_ioread32(base + ctrlRegRx) & DataBufCtrl_mode) != 0;    // check if in DBUS+data buffer mode
    return 0;
}

void mrmDataBuffer::enableTx(bool en)
{
    epicsUInt32 reg, mask;

    if(supportsTx()) {
        mask = DataTxCtrl_ena|DataTxCtrl_mode;  // enable Tx engine and set DBUS+data buffer mode

        epicsGuard<epicsMutex> g(m_tx_lock);
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

void mrmDataBuffer::waitForTxComplete(){
    //while TXCPT!=0
    //clock_nanosleep(0); //release CPU and allow reschedule

    // TODO: check statement below
    // Reading flushes output queue of VME bridge
    // Actual sending is so fast that we can use busy wait here
    // Measurements showed that we loop up to 17 times
    int i = 0;
    while (!(nat_ioread32(base+ctrlRegTx)&DataTxCtrl_done)) {
        i++;
    }
    printf("Waited for TX complete for %d iterations\n",i);
}

void mrmDataBuffer::waitWhileTxRunning(){
    int i = 0;
    while ((nat_ioread32(base+ctrlRegTx)&DataTxCtrl_run)) {
        i++;
    }
    printf("TX was running for %d iterations\n",i);
}

void mrmDataBuffer::send(epicsUInt8 startSegment, epicsUInt16 length, epicsUInt8 *data){
    epicsUInt32 i;
    epicsUInt32 startAddress;

    /* Check length */
    startAddress = startSegment*DataTxCtrl_segment_bytes;
    if (length + startAddress > DataTxCtrl_len_max) throw std::out_of_range("Too much data to send for the selected segment.");
    if (length % 4 != 0) {
        printf("Data length (%d) is not a multiple of 4, cropping to", length);   // TODO how to write it? errlogPrintf?
        length &= DataTxCtrl_len_mask;
        printf(" %d\n", length);
    }

    printf("Sending %d bytes from address %d\n", length, startAddress);

    /* Check if delay compensation is active? */

    /* Send data */
    epicsGuard<epicsMutex> g(m_tx_lock);
    waitWhileTxRunning();
    for(i = 0; i < length; i+=4){
        nat_iowrite32(dataRegTx+base+i+startAddress, *(epicsUInt32*)(data+i) );
    }


    setTxLength(&startSegment, &length);
    printf("Triggering transmision: 0x%x => ", (epicsUInt32)length|DataTxCtrl_trig|DataTxCtrl_ena|DataTxCtrl_mode|((epicsUInt32)startSegment << DataTxCtrl_saddr_shift));
    nat_iowrite32(base+ctrlRegTx, (epicsUInt32)length|DataTxCtrl_trig|DataTxCtrl_ena|DataTxCtrl_mode|((epicsUInt32)startSegment << DataTxCtrl_saddr_shift));// TODO do not force enable TX, rather check if it is enabled
    printf("0x%x\n", nat_ioread32(base+ctrlRegTx));

    printf("Tx done! and lock released\n");
}

void mrmDataBuffer::setTxLength(epicsUInt8 *startSegment, epicsUInt16 *length)
{
    // Length is ok. Non segmented implementation uses different length...
}

void mrmDataBuffer::registerUser(mrmDataBufferUser *user){
    m_users.push_back(user);
}

void mrmDataBuffer::removeUser(mrmDataBufferUser *user)
{
    m_users.erase(std::remove(m_users.begin(), m_users.end(), user), m_users.end());
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
    epicsUInt32 segments;
    epicsUInt16 firstSegment=0;
    epicsUInt8 i;

    for(i=0; i<4; i++){
        segments = nat_ioread32(base+DataBufferFlags_rx+i*4);//receivedSegments[i];
        if(segments != 0){  // do we have Rx flag in this 32 segment bits?
            while(!(segments & 0x80000000)) {   // do a bit search for the Rx flag, since we know at least one bit is set
                segments <<= 1;
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
    epicsUInt32 overflows;

    registerOffset = (segment / 32) * 4;  // decimal parts are discarded because of variable type
    segmentOffset = (segment % 32); // the bit number of the segment in the register

    overflows = nat_ioread32(base+DataBufferFlags_overflow+registerOffset);

    return (overflows & (0x80000000 >> segmentOffset)) != 0;
}

bool mrmDataBuffer::checksumError(epicsUInt16 segment) {
    epicsUInt8 registerOffset, segmentOffset;
    epicsUInt32 checksums;

    registerOffset = (segment / 32) * 4;  // decimal parts are discarded because of variable type
    segmentOffset = (segment % 32); // the bit number of the segment in the register

    checksums = nat_ioread32(base+DataBufferFlags_cheksum+registerOffset);

    return (checksums & (0x80000000 >> segmentOffset)) != 0;
}

void mrmDataBuffer::handleDataBufferRxIRQ(CALLBACK *cb) {
    void *vptr;
    callbackGetUser(vptr,cb);
    mrmDataBuffer& self=*static_cast<mrmDataBuffer*>(vptr);


    epicsUInt32 sts=nat_ioread32(self.base+self.ctrlRegRx);
    if((sts & DataBufCtrl_len_mask) > 8)self.printBinary("Control", sts);

    // Configured to send?
    if (!(sts & DataBufCtrl_mode)){
        printf("RX: Mode is not Dbuff+DBus\n");
        return;
    }

    // Still receiving?
    if (sts&DataBufCtrl_rx) {
        printf("RX: Still receiving\n");    // TODO is this overflow detection? how is it connected with overflow bits?
        return;
    }

    if (sts&DataBufCtrl_sumerr) {
        printf("RX: Checksum error\n"); // TODO when is global / segment checksum error bit set?? Should be moved to checksumError()

    } else if((sts & DataBufCtrl_len_mask) > 8){

        /*self.printFlags("Segment", self.base+DataBuffer_SegmentIRQ);
        self.printFlags("Checksum", self.base+DataBufferFlags_cheksum);
        self.printFlags("Overflow", self.base+DataBufferFlags_overflow);
        self.printFlags("Rx", self.base+DataBufferFlags_rx);*/

        epicsUInt16 length=sts & DataBufCtrl_len_mask;


        /* keep buffer in big endian mode (as sent by EVM/EVR) */
        printf("RX len: %d\n", length);
        epicsUInt16 i, segment;

        //epicsGuard<epicsMutex> g(self.m_rx_lock); // prevent getDataBufferReg user from accessing the buffer while it is updated

        segment = self.getFirstReceivedSegment();

        if(self.checksumError(segment) == 0) {   // This segment does not have a checksum error, continue...
            if(self.overflowOccured(segment) !=0) printf("HW overflow occured for segment %d\n", segment);

            /*Temporary hack for testing */
            printf("Rx segment: %d\n", segment);
            //segment = 0;
            //length += 8;
            /*---------------------------*/

            // Dispatch the buffer to users
            if(self.m_users.size() <= 0) {
                return;
            }
            else {
                for(i=segment*DataTxCtrl_segment_bytes; i<length+segment*DataTxCtrl_segment_bytes; i+=4) {
                    *(epicsUInt32*)(self.m_rx_buff+i) = nat_ioread32(self.base + self.dataRegRx + i);

                    printf("%d ", self.m_rx_buff[i]);
                    printf("%d ", self.m_rx_buff[i+1]);
                    printf("%d ", self.m_rx_buff[i+2]);
                    printf("%d,  ", self.m_rx_buff[i+3]);
                }
                printf("\n");

                for(i=0; i<self.m_users.size(); i++) {
                    self.m_users[i]->updateSegment(&segment, self.m_rx_buff, &length);
                }
            }
        } else {
            printf("Checksum error occured for segment %d.\n", segment);
            return;
        }

    }

    self.clearFlags(self.base+DataBufferFlags_rx);
    nat_iowrite32(self.base+self.ctrlRegRx, sts|DataBufCtrl_rx);    // enable for next reception
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

/*void mrmDataBuffer::printBinary(const char *preface, epicsUInt32 n) {
    printf("%s: 0x%x =", preface, n);
    int i = 0;
    while (n) {
        if(i%4 == 0) printf(" ");
        if (n & 1)
            printf("1");
        else
            printf("0");

        n >>= 1;
        i++;
    }
    printf("\n");
}*/
