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
                  parentBaseAddress,
                  controlRegisterTx,
                  controlRegisterRx,
                  dataRegisterTx,
                  dataRegisterRx)
{
    enableRx(true);
    pulseID = 0;

    txcount = regcount = rxcount = 0;
}

void mrmDataBuffer_300::enableRx(bool en)
{
    epicsUInt32 reg;

    if(supportsRx() && en) {
        epicsGuard<epicsMutex> g(m_rx_lock);
        reg = nat_ioread32(base+ctrlRegRx);

        reg |= DataRxCtrl_mode|DataRxCtrl_rx; // Set mode to DBUS+data buffer and set up buffer for reception
        nat_iowrite32(base+ctrlRegRx, reg);

        clearFlags(base+DataBufferFlags_rx);    // also clear Rx flags (and consequently checksum+overflow flags)
    }
}

inline double time_get(int clock_type=CLOCK_REALTIME){
    struct timespec t;
    clock_gettime(clock_type,&t);
    return t.tv_sec + t.tv_nsec/1e9;
}
double receiveTime=0, receiveDiff;
double sendTime=0, sendDiff;

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
    if (!waitWhileTxRunning()) return false;

    // Using big endian write (instead of memcopy for example), because the data is always in big endian on the network. Thus we always
    // need to write using big endian.
    // Length cannot be less than 4, and it must be dividable by 4. Thus we can use 32 bit access.
    /*for(i = 0; i < length; i+=4){
        be_iowrite32(dataRegTx+base+i+offset, *(epicsUInt32*)(data+i) );
    }*/

    dbgPrintf(3, "Triggering transmision: 0x%x => ", (epicsUInt32)length|DataTxCtrl_trig|((epicsUInt32)startSegment << DataTxCtrl_saddr_shift));

    reg = nat_ioread32(base+ctrlRegTx);
    reg &= ~(DataTxCtrl_len_mask);      // clear length
    reg &= ~(DataTxCtrl_saddr_mask);    // clear segment address
    reg |= (epicsUInt32)length|DataTxCtrl_trig|((epicsUInt32)startSegment << DataTxCtrl_saddr_shift); // set length and segment addres and trigger sending.
    nat_iowrite32(base+ctrlRegTx, reg);

    dbgPrintf(3, "0x%x\n", nat_ioread32(base+ctrlRegTx));

    sendReg = (epicsUInt32)length|DataTxCtrl_trig|((epicsUInt32)startSegment << DataTxCtrl_saddr_shift);
    if(sendReg != 0x2040020) {
        printf("Send parameters not ok! length=%u, startSegment=%u, sendCmd=%x, reg=%x\n", length, startSegment, sendReg, reg);
    }
    //if(length !=16 || startSegment != 2) printf("Send parameters not ok! length=%u, startSegment=%u, sendCmd=%x, reg=%x\n", length, startSegment, sendReg, reg);

    sendDiff = time_get() - sendTime;
    if(mrfioc2_dataBufferDebug >= 1) {
        if(sendDiff > 0.011 || sendDiff < 0.009) {
            time ( &rawtime1 );
            timeinfo1 = localtime ( &rawtime1 );
            txcount++;
            printf("send timeout %f. Dbuf length=%u, startSeg=%u at %s. %u times.\n", sendDiff*1e3, length, startSegment, asctime (timeinfo1), txcount);
            fflush(stdout);
        }
    }
    sendTime = time_get();

    reg = nat_ioread32(base+ctrlRegTx);
    if (!(reg & DataTxCtrl_done) || (reg & DataTxCtrl_run)) {
        time ( &rawtime1 );
        timeinfo1 = localtime ( &rawtime1 );
        regcount++;
        printf("Tx done: %x at %s. %u times.\n", reg, asctime (timeinfo1), regcount);
    }



    return true;
}



void mrmDataBuffer_300::receive()
{
    epicsUInt16 i, segment, length;

    for(i=0; i<4; i++) {
        m_checksums[i] = nat_ioread32(base+DataBufferFlags_cheksum + 4 * i);
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

            receiveDiff = time_get();
            receiveDiff -= receiveTime;
            if(mrfioc2_dataBufferDebug >= 1) {
                double newPulseID = (*((double*) &m_rx_buff[32]));
                if (newPulseID != pulseID+1) {
                    time ( &rawtime );
                    timeinfo = localtime ( &rawtime );
                    rxcount++;
                    printf("PulseID skip: expected %f, found %f, diff: %f, at %s. %u times.\n", pulseID+1, newPulseID, receiveDiff*1e3, asctime (timeinfo), rxcount);
                    fflush(stdout);
                }
                pulseID = newPulseID;
            }
            receiveTime = time_get();

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

bool mrmDataBuffer_300::overflowOccured() {
    bool overflow = false;
    epicsUInt16 i, segment;

    m_overflows[0] &= 0x7FFFFFFF;  // we don't care about segment 0 == delay compensation
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

    m_checksums[0] &= 0x7FFFFFFF;  // we don't care about segment 0 == delay compensation
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
