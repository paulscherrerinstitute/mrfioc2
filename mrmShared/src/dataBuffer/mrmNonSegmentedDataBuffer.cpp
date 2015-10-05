#include "mrmNonSegmentedDataBuffer.h"

/*mrmNonSegmentedDataBuffer::mrmNonSegmentedDataBuffer()
{
}*/

void mrmNonSegmentedDataBuffer::setTxLength(epicsUInt8 *startSegment, epicsUInt16 *length)
{
    // We will receive the buffer from the start, so increase the length that will be sent down the wire.
    *length += (epicsUInt16)*startSegment * DataTxCtrl_segment_bytes;
}

epicsUInt16 mrmNonSegmentedDataBuffer::getFirstReceivedSegment()
{
    // Always start reception from the beginning of the buffer
    return 0;
}

void mrmNonSegmentedDataBuffer::clearFlags(volatile epicsUInt8 *flagRegister)
{
    // There are no flags to clear.
}
