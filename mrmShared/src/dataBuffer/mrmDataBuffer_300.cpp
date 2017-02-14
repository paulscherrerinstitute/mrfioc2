// for htons() et al.
#ifdef _WIN32
 #include <Winsock2.h>
#endif

#include <stdio.h>

#include <epicsMMIO.h>
#include <epicsGuard.h>
#include <errlog.h>

#include "mrmShared.h"
#include "mrmDataBufferUser.h"

#include <epicsExport.h>
#include "mrmDataBuffer_300.h"


mrmDataBuffer_300::mrmDataBuffer_300(const char *parentName,
                                     volatile epicsUInt8 *parentBaseAddress,
                                     epicsUInt32 controlRegisterTx,
                                     epicsUInt32 controlRegisterRx,
                                     epicsUInt32 dataRegisterTx,
                                     epicsUInt32 dataRegisterRx):
    mrmDataBuffer(parentName,
                  mrmDataBufferType::type_300,
                  parentBaseAddress,
                  controlRegisterTx,
                  controlRegisterRx,
                  dataRegisterTx,
                  dataRegisterRx)
{
    enableRx(true);
    errlogPrintf("Data buffer 300: %s\n", parentName);
}

void mrmDataBuffer_300::enableRx(bool en)
{
    if(en) {
        clearFlags(base+DataBufferFlags_rx);    // clear Rx flags (and consequently checksum+overflow flags)

        m_enabledRx = true;
        if(rx_complete_callback.fptr != NULL){
            rx_complete_callback.fptr(this, rx_complete_callback.pvt);
        }
    }
    else {
        m_enabledRx = false;
    }
}

bool mrmDataBuffer_300::enabledRx()
{
    return mrmDataBuffer::enabledRx();
}

bool mrmDataBuffer_300::send(epicsUInt8 startSegment, epicsUInt16 length, epicsUInt8 *data){
    epicsUInt32 offset, reg;
    epicsUInt16 i;

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
    if (!waitWhileTxRunning()) {
        dbgPrintf(1, "Data sending skipped...waiting while Tx running took too long.\n");
        return false;
    }

    // Using big endian write (instead of memcopy for example), because the data is always in big endian on the network. Thus we always
    // need to write using big endian.
    // Length cannot be less than 4, and it must be dividable by 4. Thus we can use 32 bit access.
    for(i = 0; i < length; i+=4){
        be_iowrite32(dataRegTx+base+i+offset, *(epicsUInt32*)(data+i) );
    }

    dbgPrintf(3, "Triggering transmision: 0x%x => ", (epicsUInt32)length|DataTxCtrl_trig|((epicsUInt32)startSegment << DataTxCtrl_saddr_shift));

    reg = nat_ioread32(base+ctrlRegTx);
    reg &= ~(DataTxCtrl_len_mask);      // clear length
    reg &= ~(DataTxCtrl_saddr_mask);    // clear segment address
    reg |= (epicsUInt32)length|DataTxCtrl_trig|((epicsUInt32)startSegment << DataTxCtrl_saddr_shift); // set length and segment addres and trigger sending.
    nat_iowrite32(base+ctrlRegTx, reg);

    dbgPrintf(3, "0x%x\n", nat_ioread32(base+ctrlRegTx));

    return true;

}

void mrmDataBuffer_300::receive1()
{
    epicsUInt16 i, segment, length;

    for(i=0; i<4; i++) {
        m_checksums[i] = nat_ioread32(base+DataBufferFlags_checksum + 4 * i);
    }
    for(i=0; i<4; i++) {
        m_overflows[i] = nat_ioread32(base+DataBufferFlags_overflow + 4 * i);
    }
    for(i=0; i<4; i++) {
        m_rx_flags[i]  = nat_ioread32(base+DataBufferFlags_rx + 4 * i);
    }

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
        for(i=0; i<4; i++) {        // clear Rx flags for the data we have just received. We are skipping reception because of checksum.
            nat_iowrite32(base+DataBufferFlags_rx + 4 * i, m_rx_flags[i]);
        }
    } else {
        overflowOccured();

        dbgPrintf(2, "Rx segment+len: %d + %d\n", segment, length);

        // Dispatch the buffer to users
        if(m_users.size() > 0) {
            // Using big endian read (instead of memcopy for example), because the data is always in big endian on the network. Thus we always
            // need to read using big endian.
            for(i=segment*DataBuffer_segment_length; i<length+segment*DataBuffer_segment_length; i+=4) {
                *(epicsUInt32*)(m_rx_buff+i) = be_ioread32(base + dataRegRx + i);
            }

            if(mrfioc2_dataBufferDebug >= 2){
                for(i=segment*DataBuffer_segment_length; i<length+segment*DataBuffer_segment_length; i++) {
                    if(!(i%16)) printf(" | ");
                    else if(!(i%4)) printf(", ");
                    printf("%d ", m_rx_buff[i]);
                }
                printf("\n");
            }

            // clear Rx flags for the data we have just received
            for(i=0; i<4; i++) {
                nat_iowrite32(base+DataBufferFlags_rx + 4 * i, m_rx_flags[i]);
            }

            for(i=0; i<m_users.size(); i++) {
                m_users[i]->user->updateSegment(segment, m_rx_buff, length);
            }
        }
    }
}


#include "string.h"
void mrmDataBuffer_300::receive()
{
    epicsUInt16 i;

    for(i=0; i<4; i++) {
        m_checksums[i] = nat_ioread32(base+DataBufferFlags_checksum + 4 * i);
    }
    for(i=0; i<4; i++) {
        m_overflows[i] = nat_ioread32(base+DataBufferFlags_overflow + 4 * i);
    }
    for(i=0; i<4; i++) {
        m_rx_flags[i]  = nat_ioread32(base+DataBufferFlags_rx + 4 * i);
    }
    m_rx_flags[3] &= 0xFFFFFFFE;    // we don't care about the last segment == delay compensation data

    // copy over individual segment lengths. Last one is delay conpensation so we don't copy it.
    size_t sizeofUInt32 = sizeof(epicsUInt32);
    for(size_t addr=0; addr<127; addr++) {
        *(epicsUInt32*)(m_rx_length+addr) = be_ioread32(base + DataBuffer_RXSize(0) + addr*sizeofUInt32);
    }

    if(mrfioc2_dataBufferDebug >= 5){
        printFlags("Segment", base+DataBuffer_SegmentIRQ);
        printFlags("Checksum", (epicsUInt8 *)m_checksums);
        printFlags("Overflow", (epicsUInt8 *)m_overflows);
        printFlags("Rx", (epicsUInt8 *)m_rx_flags);
    }

    if(checksumError()) {
        // clear Rx flags for the checkum errors
        for(i=0; i<4; i++) {
            nat_iowrite32(base+DataBufferFlags_rx + 4 * i, m_checksums[i]);
            m_rx_flags[i] &= ~m_checksums[i];
        }
    }

    if(overflowOccured()) {
        // clear overflow flags for the overflowed data
        for(i=0; i<4; i++) {
            nat_iowrite32(base+DataBufferFlags_overflow + 4 * i, m_overflows[i]);
        }
    }

    handleConsecutiveSegments(&mrmDataBuffer_300::copyDataLocally);

    for(i=0; i<4; i++) {
        m_overflows[i] = nat_ioread32(base+DataBufferFlags_overflow + 4 * i);   // update overflow flags
    }
    for(i=0; i<4; i++) {
        nat_iowrite32(base+DataBufferFlags_rx + 4 * i, m_rx_flags[i]);          // clear Rx flags for the data we have just received
    }

    // we do not check if overflowed data has maybe overwritten the next valid received segment.
    // This can happen if overflowed data length is greater than original data length
    if(overflowOccured()) {
        for(i=0; i<4; i++) {
            m_rx_flags[i] &= ~m_overflows[i];
        }
    }

    handleConsecutiveSegments(&mrmDataBuffer_300::copyDataToUser);
}

void mrmDataBuffer_300::handleConsecutiveSegments(consecutiveSegmentFunct_t fptr) {
    epicsUInt16 startSegment=0, thisSegment=0, nextSegment = 0, nextSegmentIndex = 0, i;
    epicsUInt32 length=0;

    for(i=0; i<4; i++) {
        epicsUInt32 rxFlag = m_rx_flags[i];
        while(rxFlag != 0) {
            dbgPrintf(5, "Handling segment %d, which is ", thisSegment);
            if(rxFlag & 0x80000000) {    // segment received
                if(length == 0) {   // we don't have consecutive segments
                    startSegment = thisSegment;
                    dbgPrintf(5, "not ");
                }
                dbgPrintf(5, "consecutive (length=%u)\n", length);
                length += m_rx_length[thisSegment];
                nextSegment = thisSegment + ((m_rx_length[thisSegment] / DataBuffer_segment_length));
                nextSegmentIndex = nextSegment / 32;
                nextSegment -= nextSegmentIndex*32;

                dbgPrintf(5, "Next segment %u has index %u and offset %u (segment %u + length %u). 0x%x OR 0x80000000\n", thisSegment + ((m_rx_length[thisSegment] / DataBuffer_segment_length)), nextSegmentIndex, nextSegment, thisSegment, m_rx_length[thisSegment], (m_rx_flags[nextSegmentIndex] << nextSegment) );
                if(nextSegmentIndex < 4 && nextSegment <= 32) {
                    if(!((m_rx_flags[nextSegmentIndex] << nextSegment) & 0x80000000 )) {    // we don't have consecutive segments. Send the data and move on
                        dbgPrintf(5, "Calling fptr...\n");
                        (this->*fptr)(startSegment, length);
                        length = 0;
                    }
                    rxFlag = m_rx_flags[nextSegmentIndex];
                    rxFlag <<= nextSegment;
                    thisSegment = nextSegmentIndex*32 + nextSegment;
                    dbgPrintf(5, "Going to next segment %u with flag 0x%x and length %u\n", thisSegment, rxFlag, length);
                }
                else {
                    // exit loop
                    i = 0xFFFF;
                    rxFlag = 0;
                }
            }
            else {  // segment not received. Check the next one
                length = 0;
                rxFlag <<= 1;
                thisSegment++;
                dbgPrintf(5, "not interesting. Going to next segment %u with flag [%u]0x%x and length %u\n", thisSegment, i, rxFlag, length);
            }
        }
    }
}

void mrmDataBuffer_300::copyDataLocally(epicsUInt16 startSegment, epicsUInt32 length)
{
    epicsUInt32 addr;

    for(addr=startSegment*DataBuffer_segment_length; addr<length+startSegment*DataBuffer_segment_length; addr+=4) {
        *(epicsUInt32*)(m_rx_buff+addr) = be_ioread32(base + dataRegRx + addr);
    }
}

void mrmDataBuffer_300::copyDataToUser(epicsUInt16 startSegment, epicsUInt32 length)
{
    epicsUInt16 i;
    for(i=0; i<m_users.size(); i++) {
        m_users[i]->user->updateSegment(startSegment, m_rx_buff, length);
    }
}

bool mrmDataBuffer_300::overflowOccured() {
    bool overflow = false;
    epicsUInt16 i, segment;

    m_overflows[3] &= 0xFFFFFFFE;  // we don't care about segment 127 == delay compensation
    for (i=0; i<4; i++) {
        segment = 0;
        while (m_overflows[i] !=0 ) {   // overflow occured.
            overflow = true;
            if (m_overflows[i] & 0x80000000) {
                dbgPrintf(1, "HW overflow occured for segment %d\n", i*32 + segment);
                m_overflow_count[i*32 + segment]++;
            }
            m_overflows[i] <<= 1;
            segment ++;
        }
    }

    return overflow;
}

bool mrmDataBuffer_300::checksumError() {   // DBCS bit is not checked, since segmented data buffer uses its own checksum error registers
    bool checksum = false;
    epicsUInt16 i, segment;

    m_checksums[3] &= 0xFFFFFFFE;  // we don't care about segment 127 == delay compensation
    for (i=0; i<4; i++) {
        segment = 0;
        while (m_checksums[i] !=0 ) {   // checksum occured.
            checksum = true;
            if (m_checksums[i] & 0x80000000) {
                dbgPrintf(1, "Checksum error occured for segment %d.\n", i*32 + segment);
                m_checksum_count[i*32 + segment]++;
            }
            m_checksums[i] <<= 1;
            segment ++;
        }
    }

    return checksum;
}
