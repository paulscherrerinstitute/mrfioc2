#ifndef EVG_EVTCLK_H
#define EVG_EVTCLK_H

#include <epicsTypes.h>
#include "mrf/object.h"

const epicsUInt16 ClkSrcInternal = 0; // Event clock is generated internally
const epicsUInt16 ClkSrcRF = 1;  // Event clock is derived from the RF input

enum RFClockReference {
    RFClockReference_Internal = 0,   // Use internal reference (fractional synthesizer)
    RFClockReference_External = 1,   // Use external RF reference (front panel input through divider)
    RFClockReference_PXIe100 = 2,    // PXIe 100 MHz clock
    RFClockReference_Recovered = 3,  // Use recovered RX clock, Fan-Out mode
    RFClockReference_PXIe10 = 4      // PXIe 10 MHz clock through clock multiplier
};

/* PLL Bandwidth Select (see Silicon Labs Si5317 datasheet)
 *  000 – Si5317, BW setting HM (lowest loop bandwidth)
 *  001 – Si5317, BW setting HL
 *  010 – Si5317, BW setting MH
 *  011 – Si5317, BW setting MM
 *  100 – Si5317, BW setting ML (highest loop bandwidth)
 */
enum PLLBandwidthEvg {
    PLLBandwidthEvg_HM=0,
    PLLBandwidthEvg_HL=1,
    PLLBandwidthEvg_MH=2,
    PLLBandwidthEvg_MM=3,
    PLLBandwidthEvg_ML=4,
    PLLBandwidthEvg_MAX=PLLBandwidthEvg_ML
};

class evgEvtClk : public mrf::ObjectInst<evgEvtClk> {
public:
    evgEvtClk(const std::string&, volatile epicsUInt8* const);
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

    void setPLLBandwidth(PLLBandwidthEvg pllBandwidth);
    PLLBandwidthEvg getPLLBandwidth() const;

    void setSource(epicsUInt16 source);
    epicsUInt16 getSource() const;

    /** helper for object access **/
    void setPLLBandwidthRaw(epicsUInt16 r){setPLLBandwidth((PLLBandwidthEvg)r);}
    epicsUInt16 getPLLBandwidthRaw() const{return (epicsUInt16)getPLLBandwidth();}

private:
    volatile epicsUInt8* const m_pReg;
    epicsFloat64               m_RFref;       // In MHz
    epicsFloat64               m_fracSynFreq; // In MHz
};

#endif //EVG_EVTCLK_H
