#ifndef EVG_EVTCLK_H
#define EVG_EVTCLK_H

#include <epicsTypes.h>
#include "mrf/object.h"
#include "mrmShared.h"


enum RFClockReference {
    RFClockReference_Internal = 0,   // Use internal reference (fractional synthesizer)
    RFClockReference_External,       // Use external RF reference (front panel input through divider)
    RFClockReference_PXIe100,        // PXIe 100 MHz clock
    RFClockReference_Recovered,      // Use recovered RX clock, Fan-Out mode
    RFClockReference_PXIe10,         // PXIe 10 MHz clock through clock multiplier
    RFClockReference_ExtDownrate,    // use external RF reference for downstream ports, internal reference for upstream port, Fan-Out mode, event rate down conversion
    RFClockReference_RecoverHalved   // recovered clock /2 decimate mode, event rate is halved
};

class evgMrm;
class mrmDeviceInfo;

class evgEvtClk : public mrf::ObjectInst<evgEvtClk> {
public:
    evgEvtClk(const std::string&, volatile epicsUInt8* const, evgMrm *evg);
    ~evgEvtClk();

    /* locking done internally */
    virtual void lock() const{};
    virtual void unlock() const{};

    epicsFloat64 getFrequency() const;

    void setRFFreq(epicsFloat64);
    epicsFloat64 getRFFreq() const;

    void setRFDiv(epicsUInt32);
    epicsUInt32 getRFDiv() const;

    void setFracSynFreq(epicsFloat64);
    epicsFloat64 getFracSynFreq() const;

    bool getPllLocked() const;

    void setPLLBandwidth(PLLBandwidth pllBandwidth);
    PLLBandwidth getPLLBandwidth() const;

    void setSource(epicsUInt16 source);
    epicsUInt16 getSource() const;

    /** helper for object access **/
    void setPLLBandwidthRaw(epicsUInt16 r){setPLLBandwidth((PLLBandwidth)r);}
    epicsUInt16 getPLLBandwidthRaw() const{return (epicsUInt16)getPLLBandwidth();}

private:
    volatile epicsUInt8* const m_pReg;
    evgMrm *                   m_parent;
    epicsFloat64               m_RFref;       // In MHz
    epicsFloat64               m_fracSynFreq; // In MHz
    mrmDeviceInfo*             m_deviceInfo;
};

#endif //EVG_EVTCLK_H
