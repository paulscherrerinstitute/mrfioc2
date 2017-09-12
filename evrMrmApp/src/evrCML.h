/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef EVRCML_H_INC
#define EVRCML_H_INC

#include <epicsTypes.h>

#include "evr/cml.h"

class EVRMRM;
class mrmDeviceInfo;

class EvrCML : public CML
{
public:
    enum outkind { typeCML, typeTG300, typeTG203 };

    EvrCML(const std::string&, size_t, EVRMRM&, outkind);
    ~EvrCML();

    void lock() const;
    void unlock() const;

    cmlMode mode() const;
    void setMode(cmlMode);

    bool enabled() const;
    void enable(bool);

    bool inReset() const;
    void reset(bool);

    bool powered() const;
    void power(bool);

    //! Speed of CML clock as a multiple of the event clock
    epicsUInt32 freqMultiple() const{return mult;}

    //! delay by fraction of one event clock period.  Units of sec
    double fineDelay() const;
    void setFineDelay(double);

    // For Frequency mode

    //! Trigger level
    bool polarityInvert() const;
    void setPolarityInvert(bool);

    epicsUInt32 countHigh() const;
    epicsUInt32 countLow () const;
    epicsUInt32 countInit () const;
    void setCountHigh(epicsUInt32);
    void setCountLow (epicsUInt32);
    void setCountInit (epicsUInt32);
    double timeHigh() const;
    double timeLow () const;
    double timeInit () const;
    void setTimeHigh(double);
    void setTimeLow (double);
    void setTimeInit (double);

    // For Pattern mode

    bool recyclePat() const;
    void setRecyclePat(bool);

    // waveform and 4x pattern modes

    epicsUInt32 lenPattern(pattern) const;
    epicsUInt32 lenPatternMax(pattern) const;
    epicsUInt32 getPattern(pattern, unsigned char*, epicsUInt32) const;
    void setPattern(pattern, const unsigned char*, epicsUInt32);

    // Helpers

    template<pattern P>
    epicsUInt32 lenPattern() const{return lenPattern(P);}
    template<pattern P>
    epicsUInt32 lenPatternMax() const{return lenPatternMax(P);}

    template<pattern P>
    epicsUInt32
    getPattern(unsigned char* b, epicsUInt32 l) const{return this->getPattern(P,b,l);};

    template<pattern P>
    void
    setPattern(const unsigned char* b, epicsUInt32 l){this->setPattern(P,b,l);};

    void setModRaw(epicsUInt16 r){setMode((cmlMode)r);};
    epicsUInt16 modeRaw() const{return (epicsUInt16)mode();};

private:

    epicsUInt32 mult, wordlen;
    volatile unsigned char *base;
    const size_t N;
    EVRMRM& owner;

    epicsUInt32 shadowEnable;

    epicsUInt32 *shadowPattern[5]; // 5 is wavefrom + 4x pattern
    epicsUInt32  shadowWaveformlength;

    void syncPattern(pattern);

    outkind kind;
    mrmDeviceInfo* deviceInfo;
};

#endif // EVRCML_H_INC
