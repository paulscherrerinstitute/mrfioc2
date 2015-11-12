#include "mrmNonSegmentedDataBuffer.h"
#include "mrmShared.h"

void mrmNonSegmentedDataBuffer::setTxLength(epicsUInt8 *startSegment, epicsUInt16 *length)
{
    // We will receive the buffer from the start, so increase the length that will be sent down the wire.
    *length += (epicsUInt16)*startSegment * DataBuffer_segment_length;
}

void mrmNonSegmentedDataBuffer::setRxLength(epicsUInt16 *startSegment, epicsUInt16 *length)
{
    // length is ok.
}

epicsUInt16 mrmNonSegmentedDataBuffer::getFirstReceivedSegment()
{
    // Always start reception from the beginning of the buffer
    return 0;
}

bool mrmNonSegmentedDataBuffer::overflowOccured()
{
    return false;   // this firmware does not provide any overflow check. Return success.
}

bool mrmNonSegmentedDataBuffer::checksumError()
{
    return false; // this firmware only supports global ckecksum error checking, which is already integrated in IRQ handler.
}

void mrmNonSegmentedDataBuffer::clearFlags(volatile epicsUInt8 *flagRegister)
{
    // There are no flags to clear.
}
