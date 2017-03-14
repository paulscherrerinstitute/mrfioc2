#include "evgAcTrig.h"

#include <iostream>
#include <stdexcept>

#include <mrfCommonIO.h>
#include <mrfCommon.h>

#include "evgRegMap.h"

evgAcTrig::evgAcTrig(const std::string& name, volatile epicsUInt8* const pReg):
mrf::ObjectInst<evgAcTrig>(name),
m_pReg(pReg) {
}

evgAcTrig::~evgAcTrig() {
}

void
evgAcTrig::setDivider(epicsUInt32 divider) {
    if(divider > 255)
        throw std::runtime_error("EVG AC Trigger divider out of range. Range: 0 - 255"); // 0: divide by 1, 1: divide by 2, ... 255: divide by 256

    WRITE8(m_pReg, AcTrigDivider, divider);
}

epicsUInt32
evgAcTrig::getDivider() const {
    return READ8(m_pReg, AcTrigDivider);
}

void
evgAcTrig::setPhase(epicsFloat64 phase) {
    if(phase < 0 || phase > 25.5)
        throw std::runtime_error("EVG AC Trigger phase out of range. Delay range 0 ms - 25.5 ms in 0.1 ms steps");

    WRITE8(m_pReg, AcTrigPhase, (epicsUInt8)phase);
}

epicsFloat64
evgAcTrig::getPhase() const {
    return READ8(m_pReg, AcTrigPhase);
}

void
evgAcTrig::setBypass(bool byp) {
    if(byp)
        BITSET8(m_pReg, AcTrigControl, EVG_AC_TRIG_BYP);
    else
        BITCLR8(m_pReg, AcTrigControl, EVG_AC_TRIG_BYP);
}

bool
evgAcTrig::getBypass() const {
    return (READ8(m_pReg, AcTrigControl) & EVG_AC_TRIG_BYP) != 0;
}


void
evgAcTrig::setSyncSrc(triggerSourceT syncSrc) {
    epicsUInt8 reg = READ8(m_pReg, AcTrigControl);

    reg &= ~EVG_AC_TRIG_SYNC_MASK;

    switch(syncSrc) {
    case trigSrc_eventClock:
        reg |= EVG_AC_TRIG_SYNC_EVTCLK;
        break;

    case trigSrc_mxc7:
        reg |= EVG_AC_TRIG_SYNC_MXC7;
        break;

    case trigSrc_fpin1:
        reg |= EVG_AC_TRIG_SYNC_FPIN1;
        break;

    case trigSrc_fpin2:
        reg |= EVG_AC_TRIG_SYNC_FPIN2;
        break;

    default:
        throw std::runtime_error("EVG: Trying to set invalid AC trigger source. Ignoring.");
    }

    WRITE8(m_pReg, AcTrigControl, reg);
}

evgAcTrig::triggerSourceT evgAcTrig::getSyncSrc() const {
    epicsUInt8 reg = READ8(m_pReg, AcTrigControl);
    reg &= EVG_AC_TRIG_SYNC_MASK;
    triggerSourceT trigSrc;

    switch(reg) {
    case EVG_AC_TRIG_SYNC_EVTCLK:
        trigSrc = trigSrc_eventClock;
        break;

    case EVG_AC_TRIG_SYNC_MXC7:
        trigSrc = trigSrc_mxc7;
        break;

    case EVG_AC_TRIG_SYNC_FPIN1:
        trigSrc = trigSrc_fpin1;
        break;

    case EVG_AC_TRIG_SYNC_FPIN2:
        trigSrc = trigSrc_fpin2;
        break;

    default:
        throw std::runtime_error("EVG: Invalid AC trigger source read-back");
    }

    return trigSrc;
}

void
evgAcTrig::setTrigEvtMap(epicsUInt16 trigEvt, bool ena) {
    if(trigEvt > 7)
        throw std::runtime_error("EVG Trig Event ID too large. Max : 7");

    epicsUInt8    mask = 1 << trigEvt;
    //Read-Modify-Write
    epicsUInt8 map = READ8(m_pReg, AcTrigEvtMap);

    if(ena)
        map = map | mask;
    else
        map = map & ~mask;

    WRITE8(m_pReg, AcTrigEvtMap, map);
}

epicsUInt32
evgAcTrig::getTrigEvtMap() const {
    return READ8(m_pReg, AcTrigEvtMap);
}
