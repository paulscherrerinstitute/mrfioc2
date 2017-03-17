#ifdef MRMSOFTEVENT_H_LEVEL2
 #ifdef epicsExportSharedSymbols
  #define MRMSOFTEVENT_H_epicsExportSharedSymbols
  #undef epicsExportSharedSymbols
  #include "shareLib.h"
 #endif
#endif

#ifndef MRMSOFTEVENT_H
#define MRMSOFTEVENT_H

#include <epicsTypes.h>
#include <epicsMutex.h>
#include "mrf/object.h"

class epicsShareClass mrmSoftEvent : public mrf::ObjectInst<mrmSoftEvent>
{
public:
    mrmSoftEvent(const std::string&, volatile epicsUInt8* const);

    /* locking done internally */
    virtual void lock() const{};
    virtual void unlock() const{};

    void enable(bool);
    bool enabled() const;

    bool pend() const;

    void setEvtCode(epicsUInt32);
    epicsUInt32 getEvtCode() const;

private:
    volatile epicsUInt8* const m_base;
    epicsMutex                 m_lock;
};

#endif // MRMSOFTEVENT_H

#ifdef MRMSOFTEVENT_H_epicsExportSharedSymbols
 #undef MRMSOFTEVENT_H_LEVEL2
 #define epicsExportSharedSymbols
 #include "shareLib.h"
#endif
