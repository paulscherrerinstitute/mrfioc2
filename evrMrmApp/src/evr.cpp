/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include "drvem.h"
#include "drvemInput.h"
#include "drvemOutput.h"
#include "drvemPulser.h"
#include "drvemPrescaler.h"
#include "drvemCML.h"
#include "delayModule.h"



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


OBJECT_BEGIN(MRMInput) {

    OBJECT_PROP2("Active Level", &MRMInput::levelHigh, &MRMInput::levelHighSet);

    OBJECT_PROP2("Active Edge", &MRMInput::edgeRise, &MRMInput::edgeRiseSet);

    OBJECT_PROP2("External Code", &MRMInput::extEvt, &MRMInput::extEvtSet);

    OBJECT_PROP2("Backwards Code", &MRMInput::backEvt, &MRMInput::backEvtSet);

    OBJECT_PROP2("External Mode", &MRMInput::extModeraw, &MRMInput::extModeSetraw);

    OBJECT_PROP2("Backwards Mode", &MRMInput::backModeraw, &MRMInput::backModeSetraw);

    OBJECT_PROP2("DBus Mask", &MRMInput::dbus, &MRMInput::dbusSet);

} OBJECT_END(MRMInput)


OBJECT_BEGIN(MRMOutput) {

    OBJECT_PROP2("Map", &MRMOutput::source, &MRMOutput::setSource);
    OBJECT_PROP2("MapAlt", &MRMOutput::source2, &MRMOutput::setSource2);

    OBJECT_PROP2("Enable", &MRMOutput::enabled, &MRMOutput::enable);

} OBJECT_END(MRMOutput)


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


OBJECT_BEGIN(MRMCML) {

    OBJECT_PROP2("Enable", &MRMCML::enabled, &MRMCML::enable);

    OBJECT_PROP2("Reset", &MRMCML::inReset, &MRMCML::reset);

    OBJECT_PROP2("Power", &MRMCML::powered, &MRMCML::power);

    OBJECT_PROP2("Freq Trig Lvl", &MRMCML::polarityInvert, &MRMCML::setPolarityInvert);

    OBJECT_PROP2("Pat Recycle", &MRMCML::recyclePat, &MRMCML::setRecyclePat);

    OBJECT_PROP2("Counts High", &MRMCML::timeHigh, &MRMCML::setTimeHigh);
    OBJECT_PROP2("Counts High", &MRMCML::countHigh, &MRMCML::setCountHigh);

    OBJECT_PROP2("Counts Low", &MRMCML::timeLow, &MRMCML::setTimeLow);
    OBJECT_PROP2("Counts Low", &MRMCML::countLow, &MRMCML::setCountLow);

    OBJECT_PROP2("Counts Init", &MRMCML::timeInit, &MRMCML::setTimeInit);
    OBJECT_PROP2("Counts Init", &MRMCML::countInit, &MRMCML::setCountInit);

    OBJECT_PROP2("Fine Delay", &MRMCML::fineDelay, &MRMCML::setFineDelay);

    OBJECT_PROP1("Freq Mult", &MRMCML::freqMultiple);

    OBJECT_PROP2("Mode", &MRMCML::modeRaw, &MRMCML::setModRaw);

    OBJECT_PROP2("Waveform", &MRMCML::getPattern<MRMCML::patternWaveform>,
                             &MRMCML::setPattern<MRMCML::patternWaveform>);

    OBJECT_PROP2("Pat Rise", &MRMCML::getPattern<MRMCML::patternRise>,
                             &MRMCML::setPattern<MRMCML::patternRise>);

    OBJECT_PROP2("Pat High", &MRMCML::getPattern<MRMCML::patternHigh>,
                             &MRMCML::setPattern<MRMCML::patternHigh>);

    OBJECT_PROP2("Pat Fall", &MRMCML::getPattern<MRMCML::patternFall>,
                             &MRMCML::setPattern<MRMCML::patternFall>);

    OBJECT_PROP2("Pat Low", &MRMCML::getPattern<MRMCML::patternLow>,
                            &MRMCML::setPattern<MRMCML::patternLow>);

} OBJECT_END(MRMCML)

OBJECT_BEGIN(DelayModule) {

    OBJECT_PROP2("Enable", &DelayModule::enabled, &DelayModule::setState);
    OBJECT_PROP2("Delay0", &DelayModule::getDelay0, &DelayModule::setDelay0);
    OBJECT_PROP2("Delay1", &DelayModule::getDelay1, &DelayModule::setDelay1);

} OBJECT_END(DelayModule)
