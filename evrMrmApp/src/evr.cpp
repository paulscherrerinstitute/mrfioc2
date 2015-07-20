/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <epicsExport.h>
#include "drvem.h"
#include "drvemInput.h"
#include "drvemOutput.h"
#include "drvemPulser.h"
#include "drvemPrescaler.h"
#include "drvemCML.h"
#include "evrDelayModule.h"



OBJECT_BEGIN(EVRMRM) {

    OBJECT_PROP1("Model", &EVRMRM::model);

    OBJECT_PROP1("Version", &EVRMRM::version);
    OBJECT_PROP1("Sw Version", &EVRMRM::versionSw);

    OBJECT_PROP1("Position", &EVRMRM::position);

    OBJECT_PROP1("Event Clock TS Div", &EVRMRM::uSecDiv);

    OBJECT_PROP1("Receive Error Count", &EVRMRM::recvErrorCount);
    OBJECT_PROP1("Receive Error Count", &EVRMRM::linkChanged);

    OBJECT_PROP1("FIFO Overflow Count", &EVRMRM::FIFOFullCount);

    OBJECT_PROP1("FIFO Over rate", &EVRMRM::FIFOOverRate);
    OBJECT_PROP1("FIFO Event Count", &EVRMRM::FIFOEvtCount);
    OBJECT_PROP1("FIFO Loop Count", &EVRMRM::FIFOLoopCount);

    OBJECT_PROP1("HB Timeout Count", &EVRMRM::heartbeatTIMOCount);
    OBJECT_PROP1("HB Timeout Count", &EVRMRM::heartbeatTIMOOccured);

    OBJECT_PROP1("Timestamp Prescaler", &EVRMRM::tsDiv);

    OBJECT_PROP2("Timestamp Source", &EVRMRM::SourceTSraw, &EVRMRM::setSourceTSraw);

    OBJECT_PROP2("Clock", &EVRMRM::clock, &EVRMRM::clockSet);

    OBJECT_PROP2("Timestamp Clock", &EVRMRM::clockTS, &EVRMRM::clockTSSet);

    OBJECT_PROP2("Enable", &EVRMRM::enabled, &EVRMRM::enable);

    OBJECT_PROP2("External Inhibit", &EVRMRM::extInhib, &EVRMRM::setExtInhib);

    OBJECT_PROP2("dc enabled", &EVRMRM::isDelayCompensationEnabled, &EVRMRM::setDelayCompensationEnabled);
    OBJECT_PROP2("dc tv", &EVRMRM::delayCompensationTarget, &EVRMRM::setDelayCompensationTarget);
    OBJECT_PROP1("dc tpd", &EVRMRM::delayCompensationRxValue);
    OBJECT_PROP1("dc id", &EVRMRM::delayCompensationIntValue);
    OBJECT_PROP1("dc s", &EVRMRM::delayCompensationStatus);

    OBJECT_PROP1("PLL Lock Status", &EVRMRM::pllLocked);
    OBJECT_PROP2("PLL Bandwidth", &EVRMRM::pllBandwidthRaw, &EVRMRM::setPllBandwidthRaw);

    OBJECT_PROP1("Interrupt Count", &EVRMRM::irqCount);

    OBJECT_PROP1("Link Status", &EVRMRM::linkStatus);
    OBJECT_PROP1("Link Status", &EVRMRM::linkChanged);

    OBJECT_PROP1("Timestamp Valid", &EVRMRM::TimeStampValid);
    OBJECT_PROP1("Timestamp Valid", &EVRMRM::TimeStampValidEvent);

    OBJECT_PROP1("DBus status", &EVRMRM::dbus);
    OBJECT_PROP2("DBus Pulser Map 0", &EVRMRM::dbusToPulserMapping0, &EVRMRM::setDbusToPulserMapping0);
    OBJECT_PROP2("DBus Pulser Map 1", &EVRMRM::dbusToPulserMapping1, &EVRMRM::setDbusToPulserMapping1);
    OBJECT_PROP2("DBus Pulser Map 2", &EVRMRM::dbusToPulserMapping2, &EVRMRM::setDbusToPulserMapping2);
    OBJECT_PROP2("DBus Pulser Map 3", &EVRMRM::dbusToPulserMapping3, &EVRMRM::setDbusToPulserMapping3);
    OBJECT_PROP2("DBus Pulser Map 4", &EVRMRM::dbusToPulserMapping4, &EVRMRM::setDbusToPulserMapping4);
    OBJECT_PROP2("DBus Pulser Map 5", &EVRMRM::dbusToPulserMapping5, &EVRMRM::setDbusToPulserMapping5);
    OBJECT_PROP2("DBus Pulser Map 6", &EVRMRM::dbusToPulserMapping6, &EVRMRM::setDbusToPulserMapping6);
    OBJECT_PROP2("DBus Pulser Map 7", &EVRMRM::dbusToPulserMapping7, &EVRMRM::setDbusToPulserMapping7);

} OBJECT_END(EVRMRM)


OBJECT_BEGIN(EvrInput) {

    OBJECT_PROP2("Active Level", &EvrInput::levelHigh, &EvrInput::levelHighSet);

    OBJECT_PROP2("Active Edge", &EvrInput::edgeRise, &EvrInput::edgeRiseSet);

    OBJECT_PROP2("External Code", &EvrInput::extEvt, &EvrInput::extEvtSet);

    OBJECT_PROP2("Backwards Code", &EvrInput::backEvt, &EvrInput::backEvtSet);

    OBJECT_PROP2("External Mode", &EvrInput::extModeraw, &EvrInput::extModeSetraw);

    OBJECT_PROP2("Backwards Mode", &EvrInput::backModeraw, &EvrInput::backModeSetraw);

    OBJECT_PROP2("DBus Mask", &EvrInput::dbus, &EvrInput::dbusSet);

} OBJECT_END(EvrInput)


OBJECT_BEGIN(EvrOutput) {

    OBJECT_PROP2("Map", &EvrOutput::source, &EvrOutput::setSource);
    OBJECT_PROP2("MapAlt", &EvrOutput::source2, &EvrOutput::setSource2);

    OBJECT_PROP2("Enable", &EvrOutput::enabled, &EvrOutput::enable);

} OBJECT_END(EvrOutput)


OBJECT_BEGIN(MRMPulser) {

    OBJECT_PROP2("Delay", &MRMPulser::delay, &MRMPulser::setDelay);
    OBJECT_PROP2("Delay", &MRMPulser::delayRaw, &MRMPulser::setDelayRaw);

    OBJECT_PROP2("Width", &MRMPulser::width, &MRMPulser::setWidth);
    OBJECT_PROP2("Width", &MRMPulser::widthRaw, &MRMPulser::setWidthRaw);

    OBJECT_PROP2("Enable", &MRMPulser::enabled, &MRMPulser::enable);

    OBJECT_PROP2("Polarity", &MRMPulser::polarityInvert, &MRMPulser::setPolarityInvert);

    OBJECT_PROP2("Prescaler", &MRMPulser::prescaler, &MRMPulser::setPrescaler);

    OBJECT_PROP2("Gate mask", &MRMPulser::gateMask, &MRMPulser::setGateMask);
    OBJECT_PROP2("Gate enable", &MRMPulser::gateEnable, &MRMPulser::setGateEnable);

} OBJECT_END(MRMPulser)


OBJECT_BEGIN(MRMPreScaler) {

    OBJECT_PROP2("Divide", &MRMPreScaler::prescaler, &MRMPreScaler::setPrescaler);
    OBJECT_PROP2("Pulser mapping", &MRMPreScaler::pulserMapping, &MRMPreScaler::setPulserMapping);

} OBJECT_END(MRMPreScaler)


OBJECT_BEGIN(EvrCML) {

    OBJECT_PROP2("Enable", &EvrCML::enabled, &EvrCML::enable);

    OBJECT_PROP2("Reset", &EvrCML::inReset, &EvrCML::reset);

    OBJECT_PROP2("Power", &EvrCML::powered, &EvrCML::power);

    OBJECT_PROP2("Freq Trig Lvl", &EvrCML::polarityInvert, &EvrCML::setPolarityInvert);

    OBJECT_PROP2("Pat Recycle", &EvrCML::recyclePat, &EvrCML::setRecyclePat);

    OBJECT_PROP2("Counts High", &EvrCML::timeHigh, &EvrCML::setTimeHigh);
    OBJECT_PROP2("Counts High", &EvrCML::countHigh, &EvrCML::setCountHigh);

    OBJECT_PROP2("Counts Low", &EvrCML::timeLow, &EvrCML::setTimeLow);
    OBJECT_PROP2("Counts Low", &EvrCML::countLow, &EvrCML::setCountLow);

    OBJECT_PROP2("Counts Init", &EvrCML::timeInit, &EvrCML::setTimeInit);
    OBJECT_PROP2("Counts Init", &EvrCML::countInit, &EvrCML::setCountInit);

    OBJECT_PROP2("Fine Delay", &EvrCML::fineDelay, &EvrCML::setFineDelay);

    OBJECT_PROP1("Freq Mult", &EvrCML::freqMultiple);

    OBJECT_PROP2("Mode", &EvrCML::modeRaw, &EvrCML::setModRaw);

    OBJECT_PROP2("Waveform", &EvrCML::getPattern<EvrCML::patternWaveform>,
                             &EvrCML::setPattern<EvrCML::patternWaveform>);

    OBJECT_PROP2("Pat Rise", &EvrCML::getPattern<EvrCML::patternRise>,
                             &EvrCML::setPattern<EvrCML::patternRise>);

    OBJECT_PROP2("Pat High", &EvrCML::getPattern<EvrCML::patternHigh>,
                             &EvrCML::setPattern<EvrCML::patternHigh>);

    OBJECT_PROP2("Pat Fall", &EvrCML::getPattern<EvrCML::patternFall>,
                             &EvrCML::setPattern<EvrCML::patternFall>);

    OBJECT_PROP2("Pat Low", &EvrCML::getPattern<EvrCML::patternLow>,
                            &EvrCML::setPattern<EvrCML::patternLow>);

} OBJECT_END(EvrCML)

OBJECT_BEGIN(EvrDelayModule) {

    OBJECT_PROP2("Enable", &EvrDelayModule::enabled, &EvrDelayModule::setState);
    OBJECT_PROP2("Delay0", &EvrDelayModule::getDelay0, &EvrDelayModule::setDelay0);
    OBJECT_PROP2("Delay1", &EvrDelayModule::getDelay1, &EvrDelayModule::setDelay1);

} OBJECT_END(EvrDelayModule)
