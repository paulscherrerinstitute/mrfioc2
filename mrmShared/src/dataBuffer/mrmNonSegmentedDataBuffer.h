#ifndef MRMNONSEGMENTEDDATABUFFER_H
#define MRMNONSEGMENTEDDATABUFFER_H


#include "mrmDataBuffer.h"


class mrmNonSegmentedDataBuffer : public mrmDataBuffer
{
public:
    mrmNonSegmentedDataBuffer(volatile epicsUInt8 *parentBaseAddress,
                              epicsUInt32 controlRegisterTx,
                              epicsUInt32 controlRegisterRx,
                              epicsUInt32 dataRegisterTx,
                              epicsUInt32 dataRegisterRx):
                mrmDataBuffer(parentBaseAddress,
                              controlRegisterTx,
                              controlRegisterRx,
                              dataRegisterTx,
                              dataRegisterRx) {}

private:
    void setTxLength(epicsUInt8 *startSegment, epicsUInt16 *length);

    epicsUInt16 getFirstReceivedSegment();

    //bool overflowOccured(epicsUInt16 segment);
    //bool checksumError(epicsUInt16 segment);

    void clearFlags(volatile epicsUInt8* flagRegister);
};

#endif // MRMNONSEGMENTEDDATABUFFER_H
