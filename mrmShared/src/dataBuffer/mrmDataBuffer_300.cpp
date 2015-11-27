#include <stdio.h>
#include <string.h>     // for memcpy

#include <epicsMMIO.h>
#include <epicsGuard.h>
#include <errlog.h>

#include "mrmShared.h"
#include "mrmDataBufferUser.h"

#include <epicsExport.h>
#include "mrmDataBuffer_300.h"


bool mrmDataBuffer_300::send(epicsUInt8 startSegment, epicsUInt16 length, epicsUInt8 *data){
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
        dbgPrintf(1, "Too much data to send from offset %d (%d bytes). ", offset, length);
        length = DataBuffer_len_max - offset;
        dbgPrintf(1, "Sending only %d bytes!\n", length);
    }

    if (length % 4 != 0) {
        dbgPrintf(1, "Data length (%d) is not a multiple of 4, cropping to", length);
        length &= DataTxCtrl_len_mask;
        dbgPrintf(1, " %d\n", length);
    }

    /* Send data */
    if (!waitWhileTxRunning()) return false;

    memcpy((epicsUInt8 *)(dataRegTx+base+offset), data, length);

    dbgPrintf(3, "Triggering transmision: 0x%x => ", (epicsUInt32)length|DataTxCtrl_trig|((epicsUInt32)startSegment << DataTxCtrl_saddr_shift));

    reg = nat_ioread32(base+ctrlRegTx);
    reg &= ~(DataTxCtrl_len_mask); //clear length
    reg &= ~(DataTxCtrl_saddr_mask); // clear segment address
    reg |= (epicsUInt32)length|DataTxCtrl_trig|((epicsUInt32)startSegment << DataTxCtrl_saddr_shift); // set length and segment addres and trigger sending.
    nat_iowrite32(base+ctrlRegTx, reg);

    dbgPrintf(3, "0x%x\n", nat_ioread32(base+ctrlRegTx));

    return true;

}

void mrmDataBuffer_300::receive()
{
    epicsUInt16 i, segment, length;
    //epicsUInt32 sts = nat_ioread32(base+ctrlRegRx);


    memcpy(m_overflows, (epicsUInt8 *)(base+DataBufferFlags_overflow), 16);
    memcpy(m_checksums, (epicsUInt8 *)(base+DataBufferFlags_cheksum),  16);
    memcpy(m_rx_flags,  (epicsUInt8 *)(base+DataBufferFlags_rx),       16);

    if(mrfioc2_dataBufferDebug >= 5){
        printFlags("Segment", base+DataBuffer_SegmentIRQ);
        printFlags("Checksum", (epicsUInt8 *)m_checksums);
        printFlags("Overflow", (epicsUInt8 *)m_overflows);
        printFlags("Rx", (epicsUInt8 *)m_rx_flags);
    }

    /*length = sts & DataRxCtrl_len_mask;
    segment = getFirstReceivedSegment();
    length -= segment*DataBuffer_segment_length;    // length is always reported from the start of the buffer
    */

    /**
     * length is not reported correctly in 200+ series FW.
     * Example:
     * - send data on segment 6
     * - send data on segment 2
     * - enable IRQ
     * - length reported will be for last received segment == segment 2. We do not know the length of data that was previously received on segment 6
     * For this reason we should always read entire buffer.
     * To improve preformance we instead calculate the max length based on the registered interest. The rest is rubbish to us anyway.
     * This behavior is about to change in future (RX bit will be set for each received segment, not just the first one, and length will not be used anymore)
     **/
    segment = 0;
    length = m_max_length;

    if(checksumError()) {
        memcpy((epicsUInt8 *)(base+DataBufferFlags_rx), m_rx_flags, 16);        // clear Rx flags for the data we have just received. We are skipping reception because of checksum.
    } else {
        overflowOccured();

        dbgPrintf(2, "Rx segment+len: %d + %d\n", segment, length);

        // Dispatch the buffer to users
        if(m_users.size() > 0) {
            memcpy(&m_rx_buff[segment*DataBuffer_segment_length], (epicsUInt8 *)(base + dataRegRx + segment*DataBuffer_segment_length), length);    // copy the data to local buffer

            if(mrfioc2_dataBufferDebug >= 2){
                for(i=segment*DataBuffer_segment_length; i<length+segment*DataBuffer_segment_length; i++) {
                    if(!(i%16)) printf(" | ");
                    else if(!(i%4)) printf(", ");
                    printf("%d ", m_rx_buff[i]);
                }
                printf("\n");
            }

            // clear Rx flags for the data we have just received
            memcpy((epicsUInt8 *)(base+DataBufferFlags_rx), m_rx_flags, 16);

            for(i=0; i<m_users.size(); i++) {
                m_users[i]->user->updateSegment(segment, m_rx_buff, length);
            }
        }
    }

    //printFlags("IRQ", (epicsUInt8 *)m_irq_flags);
    memcpy((epicsUInt8 *)(base+DataBuffer_SegmentIRQ), m_irq_flags, 16);    // set which segments will trigger interrupt when data is received
}
