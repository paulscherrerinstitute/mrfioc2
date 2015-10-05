#ifndef MRMDATABUFFER_H
#define MRMDATABUFFER_H

#include <vector>

#include <epicsTypes.h>
#include <epicsMutex.h>
#include <callback.h>

#define DataTxCtrl_segment_bytes 16

#define DataTxCtrl_len_mask 0x0007fc
#define DataTxCtrl_len_max  DataTxCtrl_len_mask



class mrmDataBufferUser;


/**
 * @brief The mrmDataBuffer class part that resides directly in evm/evg/evr driver.
 * Parts of this can be perhaps embedded driectly in EVR/EVM parts respectivly. m_tx_lock
 * has to be. Consider extending this class for different HW
 */
class mrmDataBuffer {
    epicsUInt8 m_rx_buff[2048];    //Always up-to-date copy of buffer
    epicsMutex m_tx_lock;           //This lock must be held while send is in progress
    //epicsMutex m_rx_lock;           // The lock is held during access to Rx data buffer registry (and when reading Rx, overflow and checksum flags)

    std::vector<mrmDataBufferUser*> m_users;

public:
    mrmDataBuffer(volatile epicsUInt8 *parentBaseAddress,
                  epicsUInt32 controlRegisterTx,
                  epicsUInt32 controlRegisterRx,
                  epicsUInt32 dataRegisterTx,
                  epicsUInt32 dataRegisterRx);
    ~mrmDataBuffer();
    
    void enableRx(bool en);
    bool enabledRx();
    void enableTx(bool en);
    bool enabledTx();
    bool supportsRx();
    bool supportsTx();

    //Return pointer to HW memory
    //epicsUInt8* getDataBufferReg();

    /**
    * @brief waitForTxComplete pools the TXCPT bit (busy wait).
    */
    void waitForTxComplete(); // inline?

    void send(epicsUInt8 startSegment, epicsUInt16 length, epicsUInt8 *data);

    void registerUser(mrmDataBufferUser* user);

    void removeUser(mrmDataBufferUser* user);


    /**
     * @brief handleDataBreg = nat_ioread32(base+ctrlRegRx);ufferIRQ IRQ schedules CB high to invoke this function
     */
    static void handleDataBufferRxIRQ(CALLBACK*);

    void printBinary(const char *preface, epicsUInt32 n);
    void printFlags(const char *preface, volatile epicsUInt8* flagRegister);


private:
    volatile epicsUInt8 * const base;
    epicsUInt32 const ctrlRegTx;
    epicsUInt32 const ctrlRegRx;
    epicsUInt32 const dataRegTx;
    epicsUInt32 const dataRegRx;

    virtual void setTxLength(epicsUInt8 *startSegment, epicsUInt16 *length);

    /**
     * @brief getFirstReceivedSegment will check the Rx flag register and return the first received segment number.
     * @return First received segment number. If no Rx flags are set, returns zero.
     */
    virtual epicsUInt16 getFirstReceivedSegment();

    virtual bool overflowOccured(epicsUInt16 segment);
    virtual bool checksumError(epicsUInt16 segment);

    virtual void clearFlags(volatile epicsUInt8* flagRegister);

    void waitWhileTxRunning();
};

#endif // MRMDATABUFFER_H
