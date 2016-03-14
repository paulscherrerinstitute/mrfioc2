#ifndef MRMDATABUFFER_230_H
#define MRMDATABUFFER_230_H

#include "mrmDataBuffer.h"


class epicsShareClass mrmDataBuffer_230 : public mrmDataBuffer
{
public:
    mrmDataBuffer_230(const char *parentName,
                              volatile epicsUInt8 *parentBaseAddress,
                              epicsUInt32 controlRegisterTx,
                              epicsUInt32 controlRegisterRx,
                              epicsUInt32 dataRegisterTx,
                              epicsUInt32 dataRegisterRx);


    void enableRx(bool en);

private:
    bool send(epicsUInt8 startSegment, epicsUInt16 length, epicsUInt8 *data);
    void receive();
};

#endif // MRMDATABUFFER_230_H
