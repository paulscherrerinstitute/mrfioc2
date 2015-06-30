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

    void setPLLBandWidth(epicsUInt16);
    epicsUInt16 getPLLBandWidth() const;

    void setSource(epicsUInt16 source);
    epicsUInt16 getSource() const;

private:
    volatile epicsUInt8* const m_pReg;
    epicsFloat64               m_RFref;       // In MHz
    epicsFloat64               m_fracSynFreq; // In MHz
};

#endif //EVG_EVTCLK_H
