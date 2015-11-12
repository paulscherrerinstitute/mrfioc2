#ifndef MRMDATABUFFER_H
#define MRMDATABUFFER_H

#include <vector>

#include <epicsTypes.h>
#include <epicsMutex.h>
#include <callback.h>

#include "mrmDataBufferUser.h"


/**
 * @brief drvMrfiocDataBufferDebug Defines debug level (verbosity of debug printout)
 */
extern int drvMrfiocDataBufferDebug;
#define dataBuffer_debug(level,M, ...) if(drvMrfiocDataBufferDebug >= level) fprintf(stderr, "DataBuffer_DEBUG: (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

/**
 * @brief The mrmDataBuffer class part that resides directly in evm/evg/evr driver.
 * This class deals directly with the HW, thus it can be extended for different HW data buffer implementations.
 * The class accepts users which will receive updates on the data buffer.
 */
class mrmDataBuffer {
public:
    bool m_rx_irq_handled;  // guards against running multiple Rx callbacks from main ISR at the same time.

    mrmDataBuffer(volatile epicsUInt8 *parentBaseAddress,
                  epicsUInt32 controlRegisterTx,
                  epicsUInt32 controlRegisterRx,
                  epicsUInt32 dataRegisterTx,
                  epicsUInt32 dataRegisterRx);
    ~mrmDataBuffer();
    
    /**
     * @brief enableRx is used to enable or disable receiving for this data buffer
     * @param en enables receiving when true, disables when false
     */
    void enableRx(bool en);

    /**
     * @brief enabledRx tells if the data buffer reception is enabled
     * @return true if enabled, false if disabled
     */
    bool enabledRx();

    /**
     * @brief enableTx is used to enable or disable transmission for this data buffer
     * @param en enables transmission when true, disables when false
     */
    void enableTx(bool en);

    /**
     * @brief enabledTx tells if the data buffer transmission is enabled
     * @return true if enabled, false if disabled
     */
    bool enabledTx();

    /**
     * @brief supportsRx tells if this data buffer supports reception
     * @return true if the data buffer supports reception, false otherwise
     */
    bool supportsRx();

    /**
     * @brief supportsTx tells if this data buffer can send data
     * @return true if the data buffer supports transmission, false otherwise
     */
    bool supportsTx();

    /**
    * @brief waitForTxComplete busy waits until data buffer transmission is complete (pools the TXCPT bit).
    * @return true on success, false otherwise
    */
    bool waitForTxComplete(); // inline?

    /**
     * @brief send starts the transmission of the data buffer
     * @param startSegment is the starting segment to be sent out
     * @param length is the length from the starting segment to be sent out
     * @param data is the payload to be sent out
     * @return true on success, false otherwise
     */
    bool send(epicsUInt8 startSegment, epicsUInt16 length, epicsUInt8 *data);

    /**
     * @brief registerUser will register a data buffer user, which will receive data buffer updates
     * @param user is the data buffer user to register
     */
    void registerUser(mrmDataBufferUser* user);

    /**
     * @brief removeUser will remove a data buffer user from a list of registered users. The user will no longer receive and data buffer updates.
     * @param user is the data buffer user to de-register
     */
    void removeUser(mrmDataBufferUser* user);

    void setInterest(mrmDataBufferUser* user, epicsUInt32 *interest);

    /**
     * @brief handleDataBufferRxIRQ is called from the ISR when the data buffer IRQ arrives (through a CB High scheduled callback)
     */
    static void handleDataBufferRxIRQ(CALLBACK*);

    static mrmDataBuffer* getDataBufferFromDevice(const char *device);

    // TODO test functions
    void setSegmentIRQ(epicsUInt8 i, epicsUInt32 mask);
    void ctrlReceive();
    void stop();
    void printRegs();
    void setRx(epicsUInt8 i, epicsUInt32 mask);

protected:
    volatile epicsUInt8 * const base;   // Base address of the EVR/EVG card
    epicsUInt32 const ctrlRegTx;        // Tx control register offset
    epicsUInt32 const ctrlRegRx;        // Rx control register offset
    epicsUInt32 const dataRegTx;        // Tx data register offset
    epicsUInt32 const dataRegRx;        // Rx data register offset

    epicsUInt8 m_rx_buff[2048];         // Always up-to-date copy of rx buffer

    //Registered users
    struct Users{
        mrmDataBufferUser *user;
        epicsUInt32 segments[4];                // segment mask in which the user is interested
    };
    std::vector<Users*> m_users;    // a list of users who are accessing the data buffer

private:
    epicsMutex m_tx_lock;               // This lock must be held while send is in progress
    epicsMutex m_rx_lock;               // The lock prevents adding/removing users while data is being dispatched to users.

    epicsUInt32 m_checksums[4];         // stores the received checksum error register
    epicsUInt32 m_overflows[4];         // stores the received overflow flag register
    epicsUInt32 m_rx_flags[4];          // stores the received segment flags register
    epicsUInt32 m_irq_flags[4];         // used to set the segment IRQ flags register

    //std::vector<mrmDataBufferUser*> m_users;    // a list of users who are accessing the data buffer


    /**
     * @brief setTxLength calculates the length of the data package to send. It is dependant on the hardware implementation of the data buffer (thus it can be overriden).
     * @param startSegment is the number of the segment where data is to be written
     * @param length is the length of the data to write
     */
    //virtual void setTxLength(epicsUInt8 *startSegment, epicsUInt16 *length);

    /**
     * @brief setRxLength calculates the length of the received data package. It is dependant on the hardware implementation of the data buffer (thus it can be overriden).
     * @param startSegment is the number of the received segment
     * @param length is the length of the received data
     */
    //void setRxLength(epicsUInt16 *startSegment, epicsUInt16 *length);

    /**
     * @brief getFirstReceivedSegment will check the Rx flag register and return the first received segment number.
     * @return First received segment number. If no Rx flags are set, returns zero.
     */
    epicsUInt16 getFirstReceivedSegment();

    /**
     * @brief overflowOccured checks if the overflow flag is set for any segment
     * @return True if overflow occured, false otherwise
     */
    bool overflowOccured();

    /**
     * @brief checksumError checks if the checksum error flag is set for any segment
     * @return True if checksum error is detected, false otherwise
     */
    bool checksumError();

    /**
     * @brief clearFlags clears all the flags for the specified flag register, by writing '1' to each flag bit
     * @param flagRegister is the starting address of a 4x32 bit flag register to clear.
     */
    void clearFlags(volatile epicsUInt8* flagRegister);

    /**
     * @brief waitWhileTxRunning is busy waiting while data buffer transmission is running (pools the TXRUN bit)
     * @return true on success, false otherwise
     */
    bool waitWhileTxRunning();

    // TODO: remove theese
    void printBinary(const char *preface, epicsUInt32 n);
    void printFlags(const char *preface, volatile epicsUInt8* flagRegister);
    virtual void receive();
};

#endif // MRMDATABUFFER_H
