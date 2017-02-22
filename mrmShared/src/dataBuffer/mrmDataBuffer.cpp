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


static std::map<std::string, mrmDataBuffer*> data_buffers[2];

mrmDataBuffer::mrmDataBuffer(const char * parentName, mrmDataBufferType::type_t type,
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
    ,m_type(type)
{
    epicsUInt16 i;

    rx_complete_callback.fptr = NULL;
    rx_complete_callback.pvt = NULL;

    enableTx(true);

    for (i=0; i<4; i++) {
        m_irq_flags[i] = 0;
    }

    for (i=0; i<128; i++) {
        m_overflow_count[i] = 0;
        m_checksum_count[i] = 0;
    }

    // init interest flags
    setInterest(NULL, NULL);

    data_buffers[m_type][parentName] = this;
}

mrmDataBuffer::~mrmDataBuffer() {
    size_t i;

    for (i=0; i<m_users.size(); i++) {
        delete m_users[i];
    }

    // reset intrest
    setInterest(NULL, NULL);

    // data_buffers are destroyed when the app is destroyed...
}

void mrmDataBuffer::enableRx(bool en)
{
    m_enabled_rx = en;
}

bool mrmDataBuffer::enabledRx()
{
    if(supportsRx()){
        return m_enabled_rx;
    }
    else{
        return false;
    }
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
    return (dataRegRx > 0);
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

    for(i=0; i<4; i++) {        // set which segments will trigger interrupt when data is received
        nat_iowrite32(base+DataBuffer_SegmentIRQ + 4 * i, m_irq_flags[i]);
    }
}

void mrmDataBuffer::registerRxComplete(rxCompleteCallback_t fptr, void *pvt)
{
    rx_complete_callback.fptr = fptr;
    rx_complete_callback.pvt = pvt;
}

void mrmDataBuffer::clearFlags(volatile epicsUInt8 *flagRegister) {
    int i;

    for(i=0; i<=12; i+=4) {
        nat_iowrite32(flagRegister+i, 0xFFFFFFFF);
    }
}


void mrmDataBuffer::handleDataBufferRxIRQ(CALLBACK *cb) {
    void *vptr;
    callbackGetUser(vptr,cb);
    mrmDataBuffer* parent = static_cast<mrmDataBuffer*>(vptr);


    if(parent->m_enabled_rx) {

        parent->m_rx_lock.lock();
        parent->receive();
        parent->m_rx_lock.unlock();

        if(parent->rx_complete_callback.fptr != NULL){
            parent->rx_complete_callback.fptr(parent, parent->rx_complete_callback.pvt);
        }
    }
}

mrmDataBuffer* mrmDataBuffer::getDataBufferFromDevice(const char *device, mrmDataBufferType::type_t type) {
    // locking not needed because all data buffer instances are created before someone can use them

    if(data_buffers[type].count(device)){
        return data_buffers[type][device];
    }

    return NULL;
}

epicsUInt32 mrmDataBuffer::getOverflowCount(epicsUInt32 **overflowCount)
{
    *overflowCount = m_overflow_count;
    return (epicsUInt32)(sizeof (m_overflow_count) / sizeof (epicsUInt32));
}

epicsUInt32 mrmDataBuffer::getChecksumCount(epicsUInt32 **checksumCount)
{
    *checksumCount = m_checksum_count;
    return (epicsUInt32)(sizeof (m_checksum_count) / sizeof (epicsUInt32));
}

mrmDataBufferType::type_t mrmDataBuffer::getType()
{
    return m_type;
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
    printFlags("Segment flags before", base+DataBuffer_SegmentIRQ);
    nat_iowrite32(base+DataBuffer_SegmentIRQ + 4*i, mask);
    printFlags("Segment flags after", base+DataBuffer_SegmentIRQ);
}

void mrmDataBuffer::setRx(epicsUInt8 i, epicsUInt32 mask)
{
    printFlags("Rx flags before", base+DataBufferFlags_rx);
    nat_iowrite32(base+DataBufferFlags_rx + 4*i, mask);
    printFlags("Rx flags after", base+DataBufferFlags_rx);
}

void mrmDataBuffer::printRegs()
{
    printFlags("Segment", base+DataBuffer_SegmentIRQ);
    printFlags("Checksum", base + DataBufferFlags_checksum);
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
