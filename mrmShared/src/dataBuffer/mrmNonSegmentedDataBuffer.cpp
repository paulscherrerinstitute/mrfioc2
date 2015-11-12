#include <stdio.h>
#include <string.h>     // for memcpy

#include <epicsMMIO.h>
#include <errlog.h>

#include "mrmShared.h"
#include "mrmNonSegmentedDataBuffer.h"


/*void mrmNonSegmentedDataBuffer::setTxLength(epicsUInt8 *startSegment, epicsUInt16 *length)
{
    // We will receive the buffer from the start, so increase the length that will be sent down the wire.
    *length += (epicsUInt16)*startSegment * DataBuffer_segment_length;
    *startSegment = 0;
}*/

/*void mrmNonSegmentedDataBuffer::clearFlags(volatile epicsUInt8 *flagRegister)
{
    // There are no flags to clear.
}*/

void mrmNonSegmentedDataBuffer::receive() {
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

        dataBuffer_debug(1, "Rx len: %d\n", length);

        // Dispatch the buffer to users
        if(m_users.size() > 0) {
            memcpy(&m_rx_buff[0], (epicsUInt8 *)(base + dataRegRx), length);    // copy the data to local buffer

            if(drvMrfiocDataBufferDebug){
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
