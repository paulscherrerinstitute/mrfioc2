#ifndef evgFct_H
#define evgFct_H

#include <epicsTypes.h>
#include "mrf/object.h"
#include "sfp.h"

class evgFct : public mrf::ObjectInst<evgFct> {
public:
    evgFct(const std::string&, volatile epicsUInt8* const, std::vector<SFP*>&);
    ~evgFct();

    /* locking done internally */
    virtual void lock() const{};
    virtual void unlock() const{};


    epicsUInt32 getUpstreamDC() const;
    epicsUInt32 getFIFODC() const;
    epicsUInt32 getInternalDC() const;

    epicsUInt16 getPortStatus() const;
    epicsUInt16 getPortViolation() const;
    void clearPortViolation(epicsUInt16 port);

    epicsUInt32 getPortDelayValue(epicsUInt16 port) const;

    // helpers for creating objects (in evg.cpp) //
    inline epicsUInt32 getPort1DelayValue() const{return getPortDelayValue(1);}
    inline epicsUInt32 getPort2DelayValue() const{return getPortDelayValue(2);}
    inline epicsUInt32 getPort3DelayValue() const{return getPortDelayValue(3);}
    inline epicsUInt32 getPort4DelayValue() const{return getPortDelayValue(4);}
    inline epicsUInt32 getPort5DelayValue() const{return getPortDelayValue(5);}
    inline epicsUInt32 getPort6DelayValue() const{return getPortDelayValue(6);}
    inline epicsUInt32 getPort7DelayValue() const{return getPortDelayValue(7);}
    inline epicsUInt32 getPort8DelayValue() const{return getPortDelayValue(8);}

private:
    volatile epicsUInt8* const m_fctReg;
    std::vector<SFP*> m_sfp;
};


#endif // evgFct_H
