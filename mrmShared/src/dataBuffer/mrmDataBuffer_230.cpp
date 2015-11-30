#include <stdio.h>

#include <epicsMMIO.h>
#include <epicsGuard.h>
#include <errlog.h>

#include "mrmShared.h"
#include "mrmDataBufferUser.h"

#include <epicsExport.h>
#include "mrmDataBuffer_230.h"


bool mrmDataBuffer_230::send(epicsUInt8 startSegment, epicsUInt16 length, epicsUInt8 *data){
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
    for(i = 0; i < length; i+=4){
        be_iowrite32(dataRegTx+base+i+offset, *(epicsUInt32*)(data+i) );
    }

    length += startSegment * DataBuffer_segment_length;
    dbgPrintf(3, "Triggering transmision: 0x%x => ", (epicsUInt32)length|DataTxCtrl_trig);

    reg = nat_ioread32(base+ctrlRegTx);
    reg &= ~(DataTxCtrl_len_mask); //clear length
    reg |= (epicsUInt32)length|DataTxCtrl_trig; // set length and trigger sending.
    nat_iowrite32(base+ctrlRegTx, reg);

    dbgPrintf(3, "0x%x\n", nat_ioread32(base+ctrlRegTx));

    return true;
}

void mrmDataBuffer_230::receive() {
    epicsUInt16 i, length;
    epicsUInt32 sts = nat_ioread32(base+ctrlRegRx);

    if (sts & DataRxCtrl_rx) {
        errlogPrintf("DBRX bit active (data buffer receiving). Skipping reception.\n\tControl register status: 0x%x\n", sts);
        return;
    }
    else if (sts&DataRxCtrl_sumerr) {
        errlogPrintf("RX: Checksum error. Skipping reception.\n");
    }
    else {
        length = sts & DataRxCtrl_len_mask;

        dbgPrintf(2, "Rx len: %d\n", length);

        // Dispatch the buffer to users
        if(m_users.size() > 0) {
            // Using big endian read (instead of memcopy for example), because the data is always in big endian on the network. Thus we always
            // need to read using big endian.
            for(i=0; i<length; i+=4) {
                *(epicsUInt32*)(m_rx_buff+i) = be_ioread32(base + dataRegRx + i);
            }

            if(mrfioc2_dataBufferDebug >= 2){
                for(i=0; i<length; i++) {
                    if(!(i%16)) printf(" | ");
                    else if(!(i%4)) printf(", ");
                    printf("%d ", m_rx_buff[i]);
                }
                printf("\n");
            }

            for(i=0; i<m_users.size(); i++) {
                m_users[i]->user->updateSegment(0, m_rx_buff, length);
            }
        }
    }
    sts = nat_ioread32(base+ctrlRegRx);
    sts |= DataRxCtrl_rx;
    nat_iowrite32(base+ctrlRegRx, sts);
}
