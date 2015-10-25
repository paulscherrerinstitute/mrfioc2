#ifndef MRMm_data_bufferUSER_H
#define MRMm_data_bufferUSER_H

#include <vector>
#include <epicsTypes.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>

#include "mrmDataBuffer.h"


typedef void(*dataBufferRXCallback_t)(size_t updated_offset, size_t length, void* pvt);


/**
 * @brief The mrmDataBufferUser class
 *
 * Interface towards MRF EVM and EVR data buffer
 *
 * Each user of the data buffer creates 1 instance of this class. On every segment update the data is
 * copied into all registered mrmDataBufferUser instances. Each instance is than responsible for invoking
 * its own callbacks, which users can subscribe to.
 *
 * Note that if users callbacks (registerd via registerInterest function) are slow (slower than updates), data will be lost!
 *
 */
class mrmDataBufferUser {
public:
    mrmDataBufferUser();
    ~mrmDataBufferUser();

    /**
     * @brief init will connect to the data buffer based on device name
     * @param deviceName is the name of the device which holds the data buffer (eg. EVR0, EVG0, ...)
     * @return returns 0 on success
     */
    epicsUInt8 init(const char *deviceName);

    /**
     * @brief registerInterest Register callback for part (or whole) data buffer.
     * @param offset is the starting offset to register to
     * @param len is the length to register to
     * @param fptr is the callback function to invoke when data buffer on registered addresses is updated
     * @param pvt is pointer to the callback function private structure
     * @return the ID of the registered interest
     */
    epicsUInt16 registerInterest(size_t offset, size_t length, dataBufferRXCallback_t fptr, void* pvt);

    /**
     * @brief removeInterest Remove previously registred interest
     * @param id is the ID of the previously registered interest
     */
    void removeInterest(epicsUInt16 id);

    /**
     * @brief put data to buffer, but do not send it yet
     * @param offset location in data buffer where the new data should be placed
     * @param len length of new data
     * @param buffer new data
     */
    void put(size_t offset, size_t length, void* buffer);

    /**
     * @brief get retrive data by copying it into a destination buffer
     * User has to ensure that size of dest > len
     * @param offset Start location
     * @param len
     * @param dest
     */
    void get(size_t offset, size_t length, void *buffer);

    /**
     * @brief send Send the updated segments of data buffer that were set via put();
     * If the send operation is already in progress the function will block untill data can be sent.
     * If wait is true the function will block until the segment was actually sent by MRF HW.
     */
    void send(bool wait);


    /**
     * @brief trySend Send the updated segments of data buffer that were set via put();
     * If the send operation is already in progress the function will return false.
     * If wait is true the function will block until the segment was actually sent by MRF HW.
     * @return true if data was sent, false if another operation is already in progress
     */
    bool trySend(bool wait);

    /**
     * @brief getBuffer return pointer to local buffer. Note that the m_lock
     * has to be held by client. The user should never write to this buffer and use
     * a put function instead. User has to enusre that time spent within locked section
     * is minimized.
     *
     * In general the user should avoid using this function
     * @return pointer to internal m_data_buffer buffer
     */
    /*void* getBuffer(){
        return NULL;
    }*/


    /**
     * @brief updateSegment is called from the underlying data buffer class when data is received.
     * @param segment is the first segment that was updated
     * @param data is the updated data
     * @param length is the length of the updated data from the segment offset
     */
    void updateSegment(epicsUInt16 segment, epicsUInt8* data, epicsUInt16 length);

    /**
     * @brief supportsRx Checks if the underlying data buffer supports reception
     * @return True if the data buffer supports reception, false otherwise
     */
    bool supportsRx();

    /**
     * @brief supportsTx Checks if the underlying data buffer supports transmission
     * @return True if the data buffer supports transmission, false otherwise
     */
    bool supportsTx();

    /**
     * @brief lock will lock the binary semaphore protecting data races on the internal buffer
     */
    //void lock() {epicsEventWait(m_lock);}

    /**
     * @brief unlock will unlock the binary semaphore protecting data races on the internal buffer
     */
    //void unlock() {epicsEventSignal(m_lock);}


private:
    epicsUInt8 m_rx_buff[2048];    // Always up-to-date copy of buffer
    epicsUInt8 m_tx_buff[2048];    // Always up-to-date copy of buffer

    epicsUInt32 m_tx_segments[4];  // Semgent mask for updated segments,  e.g. segments that are updated in a local buffer but were not sent out yet
    epicsUInt32 m_rx_segments[4];  // Semgent mask for updates that were received but not yet dispatched
    epicsMutex m_tx_lock;          // Protects race condition put and send
    epicsMutexId m_rx_lock;        // Protects race condition between level 1 callback and consumer

    mrmDataBuffer *m_data_buffer;  // Reference to the underlying data buffer

    epicsThreadId m_thread_id;
    bool m_thread_stop;             // used to signal the user update thread that it should exit
    epicsEventId m_thread_stopped;  // used to signal when the user update thread exited
    epicsEventId m_thread_sync;     // used for synchronisation between updateSegment and updateUserThread


    //Registered callbacks
    struct RxCallback{
        epicsUInt32 offset;
        epicsUInt32 length;
        dataBufferRXCallback_t fptr;         //callback function pointer
        void* pvt;                              //callback private
    };
    std::vector<RxCallback*> m_rx_callbacks;    // a list of registered users and their segments of interest


    /**
     * @brief userUpdateThread is the thread responsible for informing users (m_rx_callbacks) that of new data received based on their registered interest in segments
     * @param args are the arguments passed to the thread
     */
    static void userUpdateThread(void *args);

    /**
     * @brief sendDataBuffer Preforms the sending of the updated segments (based on m_tx_segments) using the underlying data buffer
     */
    void sendDataBuffer();

    /**
     * @brief getDataBufferFromDevice searches for the data buffer instance in a specified device instance (eg. EVR0, EVG0, ...)
     * @param device is the device name to search for
     * @return A pointer to the underlying data buffer class, or NULL if not found
     */
    mrmDataBuffer *getDataBufferFromDevice(const char *device);
};

#endif // MRMm_data_bufferUSER_H
