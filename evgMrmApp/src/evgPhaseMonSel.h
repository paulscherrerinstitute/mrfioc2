#ifndef EVG_PHASE_MONITOR_SELECT_H
#define EVG_PHASE_MONITOR_SELECT_H

#include <string>

#include <epicsTypes.h>
#include "mrf/object.h"

class evgPhaseMonSel : public mrf::ObjectInst<evgPhaseMonSel> {
public:
    enum PhSel {
        PhSel_0Deg = 0,
        PhSel_90Deg,
        PhSel_180Deg,
        PhSel_270Deg,
    };

    enum PhMonRi {
        PhMonRi_reset = 0x0,
        PhMonRi_0to90 = 0x7,
        PhMonRi_90to180 = 0x3,
        PhMonRi_180to270 = 0x1,
        PhMonRi_270to0 = 0xF,
        PhMonRi_invalid = 0xFFFF
    };

    enum PhMonFa {
        PhMonFa_reset = 0xF,
        PhMonFa_0to90 = 0x8,
        PhMonFa_90to180 = 0xC,
        PhMonFa_180to270 = 0xE,
        PhMonFa_270to0 = 0x0,
        PhMonFa_invalid = 0xFFFF
    };

    evgPhaseMonSel(const std::string& name, volatile epicsUInt8* const pPhReg);
    ~evgPhaseMonSel();   

    virtual void lock() const {}
    virtual void unlock() const {}

    // phase monitoring
    epicsUInt16 getRiEdgeRaw() const;
    epicsUInt16 getFaEdgeRaw() const;

    PhMonRi getRiEdge() const;
    PhMonFa getFaEdge() const;

    void setMonReset(bool isReset);
    bool getMonReset() const;

    // phase select (for sampling)
    void setPhaseSelRaw(epicsUInt16 phSel);
    epicsUInt16 getPhaseSelRaw() const;

    void setPhaseSel(PhSel phSel);
    PhSel getPhaseSel() const;

private:
    volatile epicsUInt8* const m_pPhReg;
};

#endif

