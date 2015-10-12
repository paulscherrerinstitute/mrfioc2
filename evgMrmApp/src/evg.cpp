#define DATABUF_H_INC_LEVEL2

#include <epicsThread.h>
#include <epicsTime.h>
#include <generalTimeSup.h>

#include <epicsExport.h>

#include "evgOutput.h"
#include "evgAcTrig.h"
#include "evgDbus.h"
#include "evgInput.h"
#include "evgSoftEvt.h"
#include "evgTrigEvt.h"
#include "evgMxc.h"
#include "evgEvtClk.h"
#include "evgMrm.h"
#include "evgFct.h"

OBJECT_BEGIN(evgAcTrig) {
    OBJECT_PROP2("Divider", &evgAcTrig::getDivider, &evgAcTrig::setDivider);
    OBJECT_PROP2("Phase",   &evgAcTrig::getPhase,   &evgAcTrig::setPhase);
    OBJECT_PROP2("Bypass",  &evgAcTrig::getBypass,  &evgAcTrig::setBypass);
    OBJECT_PROP2("SyncSrc", &evgAcTrig::getSyncSrc, &evgAcTrig::setSyncSrc);
} OBJECT_END(evgAcTrig)

OBJECT_BEGIN(evgDbus) {
    OBJECT_PROP2("Source", &evgDbus::getSource, &evgDbus::setSource);
} OBJECT_END(evgDbus)

OBJECT_BEGIN(evgEvtClk) {
    OBJECT_PROP2("Source",         &evgEvtClk::getSource, &evgEvtClk::setSource);
    OBJECT_PROP2("RFFreq",         &evgEvtClk::getRFFreq, &evgEvtClk::setRFFreq);
    OBJECT_PROP2("RFDiv",          &evgEvtClk::getRFDiv,  &evgEvtClk::setRFDiv);
    OBJECT_PROP1("PLL Lock Status",&evgEvtClk::getPllLocked);
    OBJECT_PROP2("PLL Bandwidth",  &evgEvtClk::getPLLBandwidthRaw,  &evgEvtClk::setPLLBandwidthRaw);
    OBJECT_PROP2("FracSynFreq",    &evgEvtClk::getFracSynFreq, &evgEvtClk::setFracSynFreq);
    OBJECT_PROP1("Frequency",      &evgEvtClk::getFrequency);
} OBJECT_END(evgEvtClk)

OBJECT_BEGIN(evgInput) {
    OBJECT_PROP2("IRQ", &evgInput::getExtIrq, &evgInput::setExtIrq);
    OBJECT_PROP2("SQMK", &evgInput::getSeqMask, &evgInput::setSeqMask);
} OBJECT_END(evgInput)

OBJECT_BEGIN(evgMxc) {
    OBJECT_PROP1("Status",    &evgMxc::getStatus);
    OBJECT_PROP2("Polarity",  &evgMxc::getPolarity,  &evgMxc::setPolarity);
    OBJECT_PROP2("Frequency", &evgMxc::getFrequency, &evgMxc::setFrequency);
    OBJECT_PROP2("Prescaler", &evgMxc::getPrescaler, &evgMxc::setPrescaler);
} OBJECT_END(evgMxc)

OBJECT_BEGIN(evgOutput) {
    OBJECT_PROP2("Source", &evgOutput::getSource, &evgOutput::setSource);
} OBJECT_END(evgOutput)

OBJECT_BEGIN(evgSoftEvt) {
    OBJECT_PROP2("Enable",  &evgSoftEvt::enabled,    &evgSoftEvt::enable);
    OBJECT_PROP2("EvtCode", &evgSoftEvt::getEvtCode, &evgSoftEvt::setEvtCode);
} OBJECT_END(evgSoftEvt)

OBJECT_BEGIN(evgTrigEvt) {
    OBJECT_PROP2("Enable",  &evgTrigEvt::enabled,    &evgTrigEvt::enable);
    OBJECT_PROP2("EvtCode", &evgTrigEvt::getEvtCode, &evgTrigEvt::setEvtCode);
} OBJECT_END(evgTrigEvt)

OBJECT_BEGIN(evgMrm) {
    OBJECT_PROP2("Enable",     &evgMrm::enabled,      &evgMrm::enable);
    OBJECT_PROP1("DbusStatus", &evgMrm::getDbusStatus);
    OBJECT_PROP2("Seq mask", &evgMrm::getSWSequenceMask,   &evgMrm::setSWSequenceMask);
    OBJECT_PROP2("Seq enable", &evgMrm::getSWSequenceEnable,   &evgMrm::setSWSequenceEnable);
    OBJECT_PROP2("DlyCompens beacon", &evgMrm::dlyCompBeaconEnabled,   &evgMrm::dlyCompBeaconEnable);
    OBJECT_PROP2("DlyCompens master", &evgMrm::dlyCompMasterEnabled,   &evgMrm::dlyCompMasterEnable);
    OBJECT_PROP1("Version",    &evgMrm::getFwVersion);
    OBJECT_PROP1("Sw Version", &evgMrm::getSwVersion);
} OBJECT_END(evgMrm)

OBJECT_BEGIN(evgFct) {
    OBJECT_PROP1("DlyCompens upstream", &evgFct::getUpstreamDC);
    OBJECT_PROP1("DlyCompens fifo", &evgFct::getFIFODC);
    OBJECT_PROP1("DlyCompens internal", &evgFct::getInternalDC);
    OBJECT_PROP1("Status", &evgFct::getPortStatus);
    OBJECT_PROP2("Violation", &evgFct::getPortViolation, &evgFct::clearPortViolation);
    OBJECT_PROP1("LoopDelay port1", &evgFct::getPort1DelayValue);
    OBJECT_PROP1("LoopDelay port2", &evgFct::getPort2DelayValue);
    OBJECT_PROP1("LoopDelay port3", &evgFct::getPort3DelayValue);
    OBJECT_PROP1("LoopDelay port4", &evgFct::getPort4DelayValue);
    OBJECT_PROP1("LoopDelay port5", &evgFct::getPort5DelayValue);
    OBJECT_PROP1("LoopDelay port6", &evgFct::getPort6DelayValue);
    OBJECT_PROP1("LoopDelay port7", &evgFct::getPort7DelayValue);
    OBJECT_PROP1("LoopDelay port8", &evgFct::getPort8DelayValue);
} OBJECT_END(evgFct)
