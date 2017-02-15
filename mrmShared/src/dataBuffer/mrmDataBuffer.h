#ifndef MRMDATABUFFER_H
#define MRMDATABUFFER_H

#include <vector>
#include <map>

#include <epicsTypes.h>
#include <epicsMutex.h>
#include <callback.h>

#include "mrmDataBufferType.h"


class mrmDataBufferUser;    // Windows: use forward decleration to avoid export problems for mrmDataBufferUser class

#ifdef _WIN32
/**
 * Removes warning on windows for m_users: needs to have dll-interface to be used by clients of class 'mrmDataBuffer'
 * This is safe, because m_users is not used outside the boundary of the DLL file.
 */
#pragma warning( disable: 4251 )
#endif


/**
 * @brief drvMrfiocDataBufferDebug Defines debug level (verbosity of debug printout)
 */
extern "C"{
    extern int mrfioc2_dataBufferDebug;
    #define dbgPrintf(level,M, ...) if(mrfioc2_dataBufferDebug >= level) fprintf(stderr, "mrfioc2_dataBufferDebug: (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
}

/**
 * @brief The mrmDataBuffer class part that resides directly in evm/evg/evr driver.
 * This class deals directly with the HW, thus it can be extended for different HW data buffer implementations.
 * The class accepts users which will receive updates on the data buffer.
 */
class epicsShareClass mrmDataBuffer {
public:

    mrmDataBuffer(const char *parentName,
                  mrmDataBufferType::type_t type,
                  volatile epicsUInt8 *parentBaseAddress,
                  epicsUInt32 controlRegisterTx,
                  epicsUInt32 controlRegisterRx,
                  epicsUInt32 dataRegisterTx,
                  epicsUInt32 dataRegisterRx);
    virtual ~mrmDataBuffer();

    /**
     * Definition of a callback function. Used in registerRxComplete()
     */
    typedef void(*rxCompleteCallback_t)(mrmDataBuffer *dataBuffer, void* pvt);

    /**
     * @brief enableRx is used to enable or disable receiving for this data buffer.
     * @param en enables receiving when true, disables when false
     */
    virtual void enableRx(bool en);

    /**
     * @brief enabledRx tells if the data buffer reception is enabled
     * @return true if enabled, false if disabled
     */
    virtual bool enabledRx();

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
     * @brief send starts the transmission of the data buffer. Function implementations are in separate classes.
     * @param startSegment is the starting segment to be sent out
     * @param length is the length from the starting segment to be sent out
     * @param data is the payload to be sent out
     * @return true on success, false otherwise
     */
    virtual bool send(epicsUInt8 startSegment, epicsUInt16 length, epicsUInt8 *data) = 0;

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

    /**
     * @brief setInterest will set the interrupt flags for hardware segments based on the segments users are interested in
     * @param user A registered user who wishes to set interest
     * @param interest mask of segments the user is interested in. Must be array: interest[4]
     */
    void setInterest(mrmDataBufferUser* user, epicsUInt32 *interest);

    /**
     * @brief registerRxComplete registers a callback that is called when data buffer reception completes
     * @param fptr is the callback function to invoke when data buffer reception is complete
     * @param pvt is pointer to the callback function private structure
     */
    void registerRxComplete(rxCompleteCallback_t fptr, void* pvt);

    /**
     * @brief handleDataBufferRxIRQ is called from the ISR when the data buffer IRQ arrives (through a CB High scheduled callback)
     */
    static void handleDataBufferRxIRQ(CALLBACK*);

    static mrmDataBuffer* getDataBufferFromDevice(const char *device, mrmDataBufferType::type_t type);

    /**
     * @brief getOverflowCount sets the pointer to the internal counter for overflows on each segment. Use the pointer for reading only!
     * @param overflowCount will be set to the pointer to the internal counter
     * @return number of items in the couter array (number of segments)
     */
    epicsUInt32 getOverflowCount(epicsUInt32 **overflowCount);

    /**
     * @brief getChecksumCount sets the pointer to the internal counter for checksums on each segment. Use the pointer for reading only!
     * @param checksumCount will be set to the pointer to the internal counter
     * @return number of items in the couter array (number of segments)
     */
    epicsUInt32 getChecksumCount(epicsUInt32 **checksumCount);

    mrmDataBufferType::type_t getType();

    // test functions (used from mrmDataBuffer_test.cpp)
    void setSegmentIRQ(epicsUInt8 i, epicsUInt32 mask);
    void printRegs();
    void setRx(epicsUInt8 i, epicsUInt32 mask);
    void read(size_t offset, size_t length);

protected:
    volatile epicsUInt8 * const base;   // Base address of the EVR/EVG card
    epicsUInt32 const ctrlRegTx;        // Tx control register offset
    epicsUInt32 const ctrlRegRx;        // Rx control register offset
    epicsUInt32 const dataRegTx;        // Tx data register offset
    epicsUInt32 const dataRegRx;        // Rx data register offset

    epicsMutex m_tx_lock;               // This lock must be held while send is in progress
    epicsMutex m_rx_lock;               // The lock prevents adding/removing users while data is being dispatched to users.

    epicsUInt8 m_rx_buff[2048];         // Always up-to-date copy of rx buffer

    epicsUInt32 m_checksums[4];         // stores the received checksum error register
    epicsUInt32 m_overflows[4];         // stores the received overflow flag register
    epicsUInt32 m_rx_flags[4];          // stores the received segment flags register
    epicsUInt32 m_irq_flags[4];         // used to set the segment IRQ flags register
    epicsUInt16 m_max_length;           // maximum buffer length that we are interested in (based on m_irq_flags)

    epicsUInt32 m_overflow_count[128];  // count the total number of overflows that occured for each segment (4*32 = 128 segments)
    epicsUInt32 m_checksum_count[128];  // count the total number of checksum errors that occured for each segment (4*32 = 128 segments)
    epicsUInt32 m_rx_length[128];       // stores the received segment size (data length)
    bool        m_enabledRx;            // is reception enabled?

    //Registered users
    struct Users{
        mrmDataBufferUser *user;
        epicsUInt32 segments[4];        // segment mask in which the user is interested
    };
    std::vector<Users*> m_users;        // a list of users who are accessing the data buffer

    mrmDataBufferType::type_t m_type;  // data buffer type id

    /**
     * @brief waitWhileTxRunning is busy waiting while data buffer transmission is running (pools the TXRUN bit)
     * @return true on success, false otherwise
     */
    bool waitWhileTxRunning();


    /**
     * @brief clearFlags clears all the flags for the specified flag register, by writing '1' to each flag bit
     * @param flagRegister is the starting address of a 4x32 bit flag register to clear.
     */
    void clearFlags(volatile epicsUInt8* flagRegister);

    // helper functions
    void printBinary(const char *preface, epicsUInt32 n);
    void printFlags(const char *preface, volatile epicsUInt8* flagRegister);

    struct RxCompleteCallback{
        rxCompleteCallback_t fptr;  // callback function pointer
        void* pvt;                  // callback private
    } rx_complete_callback;

private:
        /**
     * @brief calcMaxInterestedLength Uses m_irq_flags to set new value for m_max_length
     */
    void calcMaxInterestedLength();

    /**
     * @brief receive is invoked by handleDataBufferRxIRQ. Function implementations are in separate classes.
     */
    virtual void receive() = 0;
};

#endif // MRMDATABUFFER_H
