#ifndef MRMDATABUFFEROBJ_H
#define MRMDATABUFFEROBJ_H


#include <epicsTypes.h>

#include "mrmDataBuffer.h"
#include "mrf/object.h"

class epicsShareClass mrmDataBufferObj: public mrf::ObjectInst<mrmDataBufferObj>
{
public:
    mrmDataBufferObj(const std::string& parentName, mrmDataBuffer &dataBuffer);
    static const char *OBJECT_NAME;

    /* locking done internally */
    virtual void lock() const{}
    virtual void unlock() const{}

    /**
     * @brief getOverflowCountWF is a waveform wrapper for getting the overflow count for each segment.
     */
    epicsUInt32 getOverflowCount(epicsUInt32* wf, epicsUInt32 l) const;

    /**
     * @brief getOverflowCountSum returns the sum of overflow counts for all segments
     * @return sum of overflow counts for all segments
     */
    epicsUInt32 getOverflowCountSum() const;

    /**
     * @brief getChecksumCount is a waveform wrapper for getting the checksum count for each segment.
     */
    epicsUInt32 getChecksumCount(epicsUInt32* wf, epicsUInt32 l) const;

    /**
     * @brief getChecksumCountSum returns the sum of checksum counts for all segments
     * @return sum of overflow counts for all segments
     */
    epicsUInt32 getChecksumCountSum() const;

    bool supportsTx() const;
    bool supportsRx() const;

    void enableRx(bool en);
    bool enabledRx() const;

    void report() const;

private:
    mrmDataBuffer &m_data_buffer;   // reference to the base data buffer class (hardware interface to the data buffer)
    /**
     * @brief report prints basic data buffer information to iocsh
     */

    epicsUInt32 getCountSum(epicsUInt32 *count, epicsUInt32 nElems) const;
};

#endif // MRMDATABUFFEROBJ_H
