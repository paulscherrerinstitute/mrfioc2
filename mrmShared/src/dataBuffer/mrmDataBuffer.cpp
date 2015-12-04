// for htons() et al.
#ifdef _WIN32
 #include <Winsock2.h>
#endif

#include <vector>
#include <stdio.h>
#include <algorithm>  // for remove()

#include <epicsGuard.h>
#include <callback.h>
#include <epicsMMIO.h>
#include <errlog.h>

#include "mrmShared.h"

#include "mrmDataBufferUser.h"

#include <epicsExport.h>
#include "mrmDataBuffer.h"


extern "C" {
    int mrfioc2_dataBufferDebug = 0;
    epicsExportAddress(int, mrfioc2_dataBufferDebug);
}

#define TX_WAIT_MAX_ITERATIONS 1000     // guard against infinite loop when waiting for Tx to complete / waiting while Tx is running.


static std::map<std::string, mrmDataBuffer*> data_buffers;

mrmDataBuffer::mrmDataBuffer(const char * parentName,
                             volatile epicsUInt8 *parentBaseAddress,
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

    data_buffers[parentName] = this;
}

mrmDataBuffer::~mrmDataBuffer() {
    size_t i;

    enableRx(0);
    enableTx(0);

    for (i=0; i<m_users.size(); i++) {
        delete m_users[i];
    }

    // data_buffers are destroyed when the app is destroyed...
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
    dbgPrintf(4, "Waiting for TX to complete took %d iterations (waiting for maximum of %d iterations)\n", i, TX_WAIT_MAX_ITERATIONS);
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
    dbgPrintf(4, "Waiting while TX is running took %d iterations (waiting for maximum of %d iterations)\n", i, TX_WAIT_MAX_ITERATIONS);
    if (i >= TX_WAIT_MAX_ITERATIONS) {
        errlogPrintf("Waiting while Tx running takes too long. Forced exit...\n");
        return false;
    }
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

    calcMaxInterestedLength();

    for(i=0; i<4; i++) {        // set which segments will trigger interrupt when data is received
        nat_iowrite32(base+DataBuffer_SegmentIRQ + 4 * i, m_irq_flags[i]);
    }
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

    calcMaxInterestedLength();
    for(i=0; i<4; i++) {        // set which segments will trigger interrupt when data is received
        nat_iowrite32(base+DataBuffer_SegmentIRQ + 4 * i, m_irq_flags[i]);
    }
}

void mrmDataBuffer::clearFlags(volatile epicsUInt8 *flagRegister) {
    int i;

    for(i=0; i<=12; i+=4) {
        nat_iowrite32(flagRegister+i, 0xFFFFFFFF);
    }
}

void mrmDataBuffer::calcMaxInterestedLength()
{
    epicsInt16 j, bits;
    epicsUInt32 irqFlags;

    for (j=3; j>=0; j--) {  // search from the end
        irqFlags = m_irq_flags[j];
        bits = 32;
        while(bits && irqFlags) {   // skip if irqFlags are all 0
            if (irqFlags & 0x1) {   // we found an irq flag == this is the furthest segment we are interested in
                m_max_length = j * 32 * DataBuffer_segment_length + bits * DataBuffer_segment_length;
                j = -1;   // end the for loop
                bits = 0;  // end the while loop
            }
            else {  // no flag yet
                irqFlags >>= 1;
                bits--;
            }
        }
    }
    if (m_max_length > DataBuffer_len_max) m_max_length = DataBuffer_len_max;

    dbgPrintf(1, "Fetching max %d bytes from the data buffer\n", m_max_length);
}

void mrmDataBuffer::handleDataBufferRxIRQ(CALLBACK *cb) {
    void *vptr;
    callbackGetUser(vptr,cb);
    mrmDataBuffer* parent = static_cast<mrmDataBuffer*>(vptr);


    epicsGuard<epicsMutex> g(parent->m_rx_lock);

    parent->receive();
    parent->m_rx_irq_handled = true;
}

mrmDataBuffer* mrmDataBuffer::getDataBufferFromDevice(const char *device) {
    // locking not needed because all data buffer instances are created before someone can use them

    if(data_buffers.count(device)){
        return data_buffers[device];
    }

    return NULL;
}


// ///////////////////
// Helper functions
// ///////////////////

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




// ///////////////////////////////////////////////////////
//
// The following helper functions are invoked from the
// mrmDataBuffer_test.cpp (iocsh test functions)
//
// ///////////////////////////////////////////////////////

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

void mrmDataBuffer::read(size_t offset, size_t length) {
    epicsUInt8 buff[2048];
    size_t i;

    for(i = 0; i < length; i+=4){
        *(epicsUInt32*)(buff+i + offset) = be_ioread32(base + dataRegRx + i + offset);
    }

    for(i=offset; i<offset+length; i++) {
        if(!(i%16)) printf(" | ");
        else if(!(i%4)) printf(", ");
        printf("%x ", buff[i]);
    }
    printf("\n");
}
