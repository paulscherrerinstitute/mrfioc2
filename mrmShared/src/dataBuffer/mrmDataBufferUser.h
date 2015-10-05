#ifndef MRMDATABUFFERUSER_H
#define MRMDATABUFFERUSER_H

#include <vector>
#include <epicsTypes.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>

#include "mrmDataBuffer.h"


typedef void(*DataBufferRXCallback_t)(size_t updated_offset, void* pvt);


/**
 * @brief The SegmentedDataBuffer class
 *
 * Interface towards MRF EVM and EVR segmented databuffer
 *
 * Each user of databuffer creates 1 instance of this class. On every segment update the data is
 * copied into all registered SegmentedDataBuffer instances. Each instance is than responsible for invoking
 * its own callbacks.
 *
 * Note that if users callbacks (registerd via registerInteres functions) are slow (slower than updates)
 *
 */
class mrmDataBufferUser
{
public:
    struct {
        bool updateInProgress;
        epicsThreadId id;
        bool running;
        bool stop;
    } userThread;

    /**
     * @brief SegmentedDataBuffer Create SegmentedDataBuffer instance based on device name
     * @param deviceName
     */
    mrmDataBufferUser(const char* deviceName);
    ~mrmDataBufferUser();

    /**
     * @brief registerInterest Register callback for part (or whole) databuffer. Note that
     * when the callback function specified by fptr is invoked a SegmentedDataBuffer lock is
     * already held.
     * @param offset
     * @param len
     * @param fptr
     * @param pvt
     */
    void registerInterest(size_t offset, size_t length, DataBufferRXCallback_t fptr, void* pvt);

    void deRegisterInterest(size_t offset, size_t length, DataBufferRXCallback_t fptr);

    /**
     * @brief put data to buffer, but do not send it yet
     * @param offset location in databuffer where the new data should be placed
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
     * @brief send_databuffer Send the updated segments of data buffer that were set via put();
     * If the send operation is already in progress the function will block untill data can be sent.
     * If wait_send is true the function will block until the segment was actually sent by MRF HW.
     * @param asyn
     * @return
     */
    void send(bool wait);


    /**
     * @brief send_databuffer Send the updated segments of data buffer that were set via put();
     * If the send operation is already in progress the function will return false.
     * If wait_send is true the function will block until the segment was actually sent by MRF HW.
     * @param asyn
     * @return true if data was sent, false if another operation is already in progress
     */
    bool trySend(bool wait);

    /**
     * @brief get_buffer return pointer to local buffer. Note that the m_lock
     * has to be held by client. The user should never write to this buffer and use
     * a put function instead. User has to enusre that time spent within locked section
     * is minimized.
     *
     * In general the user should avoid using this function
     * @return pointer to internal SegmentedDataBuffer  buffer
     */
    /*void* getBuffer(){
        return NULL;
    }*/

    /* This is called from level 1 callback, e.g. callback created from IRQ thread
     */
    void updateSegment(epicsUInt16 *segment, epicsUInt8* data, epicsUInt16 *length);

    bool supportsRx();
    bool supportsTx();

    void lock();
    void unlock();

private:
    epicsEventId m_lock;          //Protects race condition between level 1 callback and consumer
    epicsUInt8 m_buff[2048];    //Always up-to-date copy of buffer

    epicsUInt32 m_tx_segments[4]; //Semgent mask for updated segments,
    //e.g. segments that are updated in a local buffer but were not sent out yet
    epicsUInt32 m_rx_segments[4]; //Semgent mask for updates that were received but not yet dispatched
    epicsUInt32 globalSegmentInterestMask[4];  // Stores interests from all registered callbacks, which helps to determine if SW overflow occured
    epicsMutex segmentInterest_lock;    // Protects registering and de-registering segment interests

    mrmDataBuffer *dataBuffer;


    //Registered callbacks
    struct RxCallback{
        epicsUInt32 segment_interest_mask[4]; //4*32 = 128 == number of segments. each set bit indicates registered interest
        epicsUInt32 length;
        DataBufferRXCallback_t fptr; //callback function pointer
        void* pvt;                            //callback private
    };
    std::vector<RxCallback*> m_rx_callbacks;

    static void userUpdateThread(void *args);
    void sendDataBuffer();

    mrmDataBuffer *getDataBufferFromDevice(const char *device);
};

#endif // MRMDATABUFFERUSER_H
