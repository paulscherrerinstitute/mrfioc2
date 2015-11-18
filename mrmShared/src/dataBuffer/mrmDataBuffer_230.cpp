#include <stdio.h>
#include <string.h>     // for memcpy

#include <epicsMMIO.h>
#include <epicsGuard.h>
#include <errlog.h>

#include "mrmShared.h"
#include "mrmDataBufferUser.h"

#include <epicsExport.h>
#include "mrmDataBuffer_230.h"


bool mrmDataBuffer_230::send(epicsUInt8 startSegment, epicsUInt16 length, epicsUInt8 *data){
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

    length += startSegment * DataBuffer_segment_length;
    dbgPrintf(1, "Triggering transmision: 0x%x => ", (epicsUInt32)length|DataTxCtrl_trig);

    reg = nat_ioread32(base+ctrlRegTx);
    reg &= ~(DataTxCtrl_len_mask); //clear length
    reg |= (epicsUInt32)length|DataTxCtrl_trig; // set length and trigger sending.
    nat_iowrite32(base+ctrlRegTx, reg);

    dbgPrintf(1, "0x%x\n", nat_ioread32(base+ctrlRegTx));

    return true;
}

void mrmDataBuffer_230::receive() {
    epicsUInt16 i, length;
    epicsUInt32 sts = nat_ioread32(base+ctrlRegRx);

    if (sts & DataRxCtrl_rx) {
        errlogPrintf("Interrupt triggered but Rx not completed. Should never happen, fatal error!\n  Control register status: 0x%x\n", sts);
    }
    else if (sts&DataRxCtrl_sumerr) {
        errlogPrintf("RX: Checksum error\n");
    }
    else {
        length = sts & DataRxCtrl_len_mask;

        dbgPrintf(1, "Rx len: %d\n", length);

        // Dispatch the buffer to users
        if(m_users.size() > 0) {
            memcpy(&m_rx_buff[0], (epicsUInt8 *)(base + dataRegRx), length);    // copy the data to local buffer

            if(mrfioc2_dataBufferDebug){
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
}
