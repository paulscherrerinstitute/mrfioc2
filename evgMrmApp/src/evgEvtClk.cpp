#include "evgEvtClk.h"

#include <stdio.h>
#include <errlog.h>
#include <stdexcept>

#include <mrfCommonIO.h>
#include <mrfCommon.h>
#include <mrfFracSynth.h>

#include "evgRegMap.h"
#include "mrmDeviceInfo.h"
#include "evgMrm.h"

evgEvtClk::evgEvtClk(const std::string& name, volatile epicsUInt8* const pReg, evgMrm *evg):
mrf::ObjectInst<evgEvtClk>(name),
m_pReg(pReg),
m_parent(evg),
m_RFref(0.0f),
m_fracSynFreq(0.0f),
m_deviceInfo(evg->getDeviceInfo())
{
}

evgEvtClk::~evgEvtClk() {
}

epicsFloat64
evgEvtClk::getFrequency() const {
    epicsUInt16 source = getSource();

    switch ((RFClockReference) source) {
        case RFClockReference_Internal:
        case RFClockReference_Recovered:
            return m_fracSynFreq;
            break;

        case RFClockReference_RecoverHalved:
            return m_fracSynFreq/2;
            break;

        default:
            return getRFFreq()/getRFDiv();
            break;
    }

}

void
evgEvtClk::setRFFreq (epicsFloat64 RFref) {
    if(RFref < 50.0f || RFref > 1600.0f) {
        char err[80];
        sprintf(err, "Cannot set RF frequency to %f MHz. Valid range is 50 - 1600.", RFref);
        std::string strErr(err);
        throw std::runtime_error(strErr);
    }

    m_RFref = RFref;
}

epicsFloat64
evgEvtClk::getRFFreq() const {
    return m_RFref;
}

void
evgEvtClk::setRFDiv(epicsUInt32 rfDiv) {
    if(rfDiv < 1    || rfDiv > 32) {
        char err[80];
        sprintf(err, "Invalid RF Divider %d. Valid range is 1 - 32", rfDiv);
        std::string strErr(err);
        throw std::runtime_error(strErr);
    }

    WRITE8(m_pReg, RfDiv, rfDiv-1);
}

epicsUInt32
evgEvtClk::getRFDiv() const {
    return READ8(m_pReg, RfDiv) + 1;
}

void
evgEvtClk::setFracSynFreq(epicsFloat64 freq) {
    epicsUInt32 controlWord, oldControlWord;
    epicsFloat64 error;

    controlWord = FracSynthControlWord (freq, MRF_FRAC_SYNTH_REF, 0, &error);
    if ((!controlWord) || (error > 100.0)) {
        char err[80];
        sprintf(err, "Cannot set event clock speed to %f MHz.\n", freq);
        std::string strErr(err);
        throw std::runtime_error(strErr);
    }

    oldControlWord=READ32(m_pReg, FracSynthWord);

    /* Changing the control word disturbes the phase of the synthesiser
     which will cause a glitch. Don't change the control word unless needed.*/
    if(controlWord != oldControlWord) {
        epicsUInt16 uSecDivider;

        if ((RFClockReference)getSource() == RFClockReference_RecoverHalved) {
            uSecDivider = (epicsUInt16)(freq / 2);
        }
        else {
            uSecDivider = (epicsUInt16)freq;
        }

        // uSedDiv must be written before fract synth (see section 3.2 Programmable Reference clock in the documentation)
        WRITE16(m_pReg, uSecDiv, uSecDivider);
        WRITE32(m_pReg, FracSynthWord, controlWord);
    }

    m_fracSynFreq = FracSynthAnalyze(READ32(m_pReg, FracSynthWord), 24.0, 0);
}

epicsFloat64
evgEvtClk::getFracSynFreq() const {
    return FracSynthAnalyze(READ32(m_pReg, FracSynthWord), 24.0, 0);
}

bool
evgEvtClk::getPllLocked() const {
    return (READ8(m_pReg, ClockSource) & EVG_CLK_PLLLOCK) != 0;
}

void
evgEvtClk::setPLLBandwidth(PLLBandwidth pllBandwidth) {
    epicsUInt8 clkCtrl;
    epicsUInt8 bw;

    if(pllBandwidth > PLLBandwidth_MAX){
        throw std::out_of_range("PLL bandwith you selected is not available.");
    }

    bw = (epicsUInt8)pllBandwidth;
    bw = bw << EVG_CLK_BW_shift;            // shift appropriately

    clkCtrl = READ8(m_pReg, ClockSource);   // read register content
    clkCtrl &= ~EVG_CLK_BW;                 // clear bw_sel
    clkCtrl |= bw;                          // OR bw sel value
    WRITE8(m_pReg, ClockSource, clkCtrl);   // write the new value to the register
}

PLLBandwidth
evgEvtClk::getPLLBandwidth() const {
    epicsUInt8 bw;

    bw = (READ8(m_pReg, ClockSource) & EVG_CLK_BW);    // read and mask out the PLL BW value
    bw = bw >> EVG_CLK_BW_shift;                       // shift appropriately

    return (PLLBandwidth)bw;
}

void
evgEvtClk::setSource (epicsUInt16 source) {
    epicsUInt8 clkReg, regMap = 0;

    if ((RFClockReference) source >= RFClockReference_PXIe100 && m_deviceInfo->getFirmwareId() < mrmDeviceInfo::firmwareId_delayCompensation) {
        throw std::out_of_range("RF clock source you selected is not supported in this firmware version.");
    }

    switch ((RFClockReference) source) {
    case RFClockReference_Internal:
        regMap = EVG_CLK_SRC_INTERNAL;
        break;

    case RFClockReference_External:
        regMap = EVG_CLK_SRC_EXTERNAL;
        break;

    case RFClockReference_PXIe100:
        regMap = EVG_CLK_SRC_PXIE100;
        break;

    case RFClockReference_PXIe10:
        regMap = EVG_CLK_SRC_PXIE10;
        break;

    case RFClockReference_Recovered:
        regMap = EVG_CLK_SRC_RECOVERED;
        break;

    case RFClockReference_ExtDownrate:
        regMap = EVG_CLK_SRC_EXTDOWNRATE;
        break;

    case RFClockReference_RecoverHalved:
        regMap = EVG_CLK_SRC_RECOVERHALVED;
        break;

    default:
        throw std::out_of_range("RF clock source you selected does not exist.");
        break;
    }



    clkReg = READ8(m_pReg, ClockSource);    // read register content
    clkReg &= ~EVG_CLK_SRC_SEL; // clear old clock source
    clkReg |= regMap;  // set new clock source
    WRITE8(m_pReg, ClockSource, clkReg);    // write the new value to the register
}

epicsUInt16
evgEvtClk::getSource() const {
    epicsUInt8 clkReg, source;

    clkReg = READ8(m_pReg, ClockSource);
    clkReg &= EVG_CLK_SRC_SEL;

    switch (clkReg) {
    case EVG_CLK_SRC_INTERNAL:
       source = (epicsUInt8)RFClockReference_Internal;
        break;

    case EVG_CLK_SRC_EXTERNAL:
       source = (epicsUInt8)RFClockReference_External;
        break;

    case EVG_CLK_SRC_PXIE100:
       source = (epicsUInt8)RFClockReference_PXIe100;
        break;

    case EVG_CLK_SRC_PXIE10:
       source = (epicsUInt8)RFClockReference_PXIe10;
        break;

    case EVG_CLK_SRC_RECOVERED:
       source = (epicsUInt8)RFClockReference_Recovered;
        break;

    case EVG_CLK_SRC_EXTDOWNRATE:
        source = (epicsUInt8)RFClockReference_ExtDownrate;
        break;

    case EVG_CLK_SRC_RECOVERHALVED:
        source = (epicsUInt8)RFClockReference_RecoverHalved;
        break;

    default:
        throw std::out_of_range("Cannot read valid RF clock source.");
        break;
    }

    return source;
}

void evgEvtClk::setToggleDBus(bool bit) {
    epicsUInt8 regVal;

    regVal = READ8(m_pReg, RfDiv);

    regVal &= ~EVG_CLK_PH_TOGG_mask;
    regVal |= bit << EVG_CLK_PH_TOGG_shift;

    WRITE8(m_pReg, RfDiv, regVal);
}

bool evgEvtClk::getToggleDBus() const {
  return READ8(m_pReg, RfDiv) & EVG_CLK_PH_TOGG_mask;
}

