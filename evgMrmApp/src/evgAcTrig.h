#ifndef EVG_AC_TRIG_H
#define EVG_AC_TRIG_H

#include <epicsTypes.h>
#include "mrf/object.h"

class evgAcTrig : public mrf::ObjectInst<evgAcTrig> {
public:
    evgAcTrig(const std::string&, volatile epicsUInt8* const);
    ~evgAcTrig();

    typedef enum trigSrc {
        trigSrc_eventClock,
        trigSrc_mxc7,
        trigSrc_fpin1,
        trigSrc_fpin2
    } triggerSourceT;

    /* locking done internally */
    virtual void lock() const{};
    virtual void unlock() const{};

    void setDivider(epicsUInt32);
    epicsUInt32 getDivider() const;

    void setPhase(epicsFloat64);
    epicsFloat64 getPhase() const;

    void setBypass(bool);
    bool getBypass() const;

    void setSyncSrc(triggerSourceT);
    triggerSourceT getSyncSrc() const;

    void setTrigEvtMap(epicsUInt16, bool);
    epicsUInt32 getTrigEvtMap() const;

    /** helper for EPICS object access **/
    void setSyncSrcEpics(epicsUInt16 r){setSyncSrc((triggerSourceT)r);}
    epicsUInt16 getSyncSrcEpics() const{return (epicsUInt16)getSyncSrc();}

private:
    volatile epicsUInt8* const m_pReg;
};

#endif //EVG_AC_TRIG_H

