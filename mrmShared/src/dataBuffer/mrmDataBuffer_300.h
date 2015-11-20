#ifndef MRMDATABUFFER_300_H
#define MRMDATABUFFER_300_H

#include "mrmDataBuffer.h"

class epicsShareClass mrmDataBuffer_300 : public mrmDataBuffer
{
public:
    mrmDataBuffer_300(const char *parentName,
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
                      dataRegisterRx) {}
private:
    bool send(epicsUInt8 startSegment, epicsUInt16 length, epicsUInt8 *data);
    void receive();
};


#endif // MRMDATABUFFER_300_H
