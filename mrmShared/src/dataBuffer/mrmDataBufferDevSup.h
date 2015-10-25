/*
 * Windows
 * When mrmShared is compiled the class should be set as exported (included in dll).
 * But when for example evgMrm is compiled, class should be set as imported. This means that evgMrm uses the previously exported class from the mrmShared dll.
 */
#ifdef MRMDATABUFFERDEVSUP_H_LEVEL2
 #ifdef epicsExportSharedSymbols
  #define MRMDATABUFFERDEVSUP_H_epicsExportSharedSymbols
  #undef epicsExportSharedSymbols
  #include "shareLib.h"
 #endif
#endif

#ifndef MRMDATABUFFERDEVSUP_H
#define MRMDATABUFFERDEVSUP_H

#include <epicsTypes.h>
#include "mrf/object.h"

class mrmDataBuffer;

class epicsShareClass mrmDataBufferDevSup : public mrf::ObjectInst<mrmDataBufferDevSup>
{
public:
    mrmDataBufferDevSup(const std::string& name, mrmDataBuffer *dataBuffer);

    bool supportsRx() const;
    bool supportsTx() const;

    bool enabledRx() const;
    bool enabledTx() const;

    void enableRx(bool enable);
    void enableTx(bool enable);


private:
    mrmDataBuffer *m_data_buffer;  // Reference to the underlying data buffer

    /* no locking needed */
    virtual void lock() const{}
    virtual void unlock() const{}
};

#endif // MRMDATABUFFERDEVSUP_H

#ifdef MRMDATABUFFERDEVSUP_H_epicsExportSharedSymbols
 #undef MRMDATABUFFERDEVSUP_H_LEVEL2
 #define epicsExportSharedSymbols
 #include "shareLib.h"
#endif
