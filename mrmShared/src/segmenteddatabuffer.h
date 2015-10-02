#ifndef SEGMENTEDDATABUFFER_H
#define SEGMENTEDDATABUFFER_H

#include <vector>
#include <epicsTypes.h>
#include <epicsMutex.h>


typedef void(*SegmentedDataBufferRXCallback_t)(size_t updated_offset, void* pvt);


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
class SegmentedDataBuffer
{

public:
    /**
     * @brief SegmentedDataBuffer Create SegmentedDataBuffer instance based on device name
     * @param deviceName
     */
    SegmentedDataBuffer(const char* deviceName);

    /**
     * @brief registerInterest Register callback for part (or whole) databuffer. Note that
     * when the callback function specified by fptr is invoked a SegmentedDataBuffer lock is
     * already held.
     * @param offset
     * @param len
     * @param fptr
     * @param pvt
     */
    void registerInterest(size_t offset, size_t len,SegmentedDataBufferRXCallback_t fptr, void* pvt){
        //Append to m_callbacks;
    }

    /**
     * @brief put data to buffer, but do not send it yet
     * @param offset location in databuffer where the new data should be placed
     * @param len length of new data
     * @param buffer new data
     */
    void put(size_t offset, size_t len, void* buffer){
        //Acquire m_lock
        //Copy from buffer to m_buff
        //Update m_updated_segments
        //Relase m_lock
    }

    /**
     * @brief get retrive data by copying it into a destination buffer
     * User has to ensure that size of dest > len
     * @param offset Start location
     * @param len
     * @param dest
     */
    void get(size_t offset, size_t len, void*& dest){
        //Acquire m_lock
        //copy to dest
        //relase m_lock
    }

    /**
     * @brief send_databuffer Send the updated segments of data buffer that were set via put();
     * If the send operation is already in progress the function will block untill data can be sent.
     * If wait_send is true the function will block until the segment was actually sent by MRF HW.
     * @param asyn
     * @return
     */
    void send(bool wait_send){
        //mrmDataBuffer->m_tx_lock take a lock to prevent other threads from trashing data in buffer
        //
        //For each segment
        //  get length (check consquitive segments that can be sentout)
        //  mrmDataBuffer->send(segment id, len) //This blocks if something is already being sentout
        //
        //release mrmDataBuffer->m_tx_lock

    }


    /**
     * @brief send_databuffer Send the updated segments of data buffer that were set via put();
     * If the send operation is already in progress the function will return false.
     * If wait_send is true the function will block until the segment was actually sent by MRF HW.
     * @param asyn
     * @return true if data was sent, false if another operation is already in progress
     */
    bool try_send(bool wait_send){
        //same as send, but with a try_lock at the beginning
    }

    /**
     * @brief get_buffer return pointer to local buffer. Note that the m_lock
     * has to be held by client. The user should never write to this buffer and use
     * a put function instead. User has to enusre that time spent within locked section
     * is minimized.
     *
     * In general the user should avoid using this function
     * @return pointer to internal SegmentedDataBuffer  buffer
     */
    void* get_buffer(){

    }

    void lock();
    void unlock();


private:

    epicsMutex m_lock;          //Protects race condition between level 1 callback and consumer
    epicsUInt8 m_buff[2048];    //Always up-to-date copy of buffer

    epicsUInt8 m_tx_segments[16]; //Semgent mask for updated segments,
    //e.g. segments that are updated in a local buffer but were not sent out yet
    epicsUInt8 m_rx_segments[16]; //Semgent mask for updates that were received but not yet dispatched



    //Registered callbacks
    struct RxCallback{
        epicsUInt8 segment_interest_mask[16]; //16*8 = 128 == number of segments. each set bit indicates registered interest
        SegmentedDataBufferRXCallback_t fptr; //callback function pointer
        void* pvt;                            //callback private
    };
    std::vector<RxCallback> m_rx_callbacks;


    /* This is called from level 1 callback, e.g. callback created from IRQ thread
     */
    void update_segment(epicsUInt8 segment_interest_mask[16],epicsUInt8* data){
        //lock m_lock with timeout?
        //
        //  Copy new segments into m_buff
        //
        //  check m_rx_segments and segment_interest_mask on overlap issue a warning about data loss for segments
        //  m_rx_segments |= segment_interest_mask
        //
        //
        //  If this->callbacks.size()
        //      Schedule callback for this->dispatch_updates {or release thread}
        //     m_callback_in_progress = true
        //
    }


    void dispatch_updates(){
        //lock m_lock
        //Iterate across callbacks, invoke callbacks whose interest mask matches the set interest mask
        //m_callback_in_progress = false
    }

};

/**
 * @brief The mrmDataBuffer class part that resides directly in evm/evg/evr driver.
 * Parts of this can be perhaps embedded driectly in EVR/EVM parts respectivly. m_tx_lock
 * has to be. Consider extending this class for different HW
 */
class mrmDataBuffer{
    epicsUInt8 m_rx_buff[2048];    //Always up-to-date copy of buffer
    epicsMutex m_tx_lock;           //This lock must be held while send is in progress

    std::vector<SegmentedDataBuffer*> m_users;
public:

    void enable(bool);

    //Return pointer to HW memory
    epicsUInt8* getDatabufferReg();


    inline void wait_for_tx_cmplt(){
        //while TXCPT!=0
        //clock_nanosleep(0); //release CPU and allow reschedule
    }

    void send(epicsUInt8 start_segment, size_t len, void* data){
        //waif_for_tx_cmplt
        //Copy data into HW
        //Set start segment and len
        //begin tx (TRIG=1)
    }

    void registerUser(SegmentedDataBuffer* usr){
        users.push_back(usr);
    }

    void removeUser(SegmentedDataBuffer* usr);

    /**
     * @brief handleDataBufferIRQ IRQ schedules CB high to invoke this function
     */
    void handleDataBufferIRQ(){
        //Check overflow flags
        //Check RX segment flags

        //if(m_usres.size()==0) return
        //if(m_users.size() > 1) copy data to m_rx_buff; data=m_rx_buff
        //else data=memmappded data

        //for user in m_users
        // invoke update_segment(flags,data)

    }

};



#endif // SEGMENTEDDATABUFFER_H
