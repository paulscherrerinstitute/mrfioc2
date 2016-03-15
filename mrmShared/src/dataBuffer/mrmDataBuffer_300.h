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
                      epicsUInt32 dataRegisterRx);

    /**
     * @brief enableRx can only enable reception. Disabling reception has no effect on this HW.
     * @param en when true enables reception, otherwise does nothing
     */
    void enableRx(bool en);

private:
    bool send(epicsUInt8 startSegment, epicsUInt16 length, epicsUInt8 *data);
    void receive();

    /**
     * @brief overflowOccured checks if the overflow flag is set for any segment
     * @return True if overflow occured, false otherwise
     */
    bool overflowOccured();

    /**
     * @brief checksumError checks if the checksum error flag is set for any segment
     * @return True if checksum error is detected, false otherwise
     */
    bool checksumError();
};


#endif // MRMDATABUFFER_300_H
