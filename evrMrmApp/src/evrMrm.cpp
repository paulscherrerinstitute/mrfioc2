/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <cstdio>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <sstream>

#include <epicsMath.h>
#include <errlog.h>
#include <epicsMath.h>
#include <dbDefs.h>
#include <dbScan.h>
#include <epicsInterrupt.h>

#include "evrRegMap.h"

#include "mrfFracSynth.h"

#include <mrfCommon.h>
#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#include "evrIocsh.h"
#include "evrMrm.h"

#include "support/util.h"
#include "mrf/version.h"

#if defined(__linux__) || defined(_WIN32)
#  include "devLibPCI.h"
#endif

#include <epicsExport.h>

int evrDebug, evrEventDebug=0;
extern "C" {
 epicsExportAddress(int, evrDebug);
 epicsExportAddress(int, evrEventDebug);
}

using namespace std;

#define CBINIT(ptr, prio, fn, valptr) \
do { \
  callbackSetPriority(prio, ptr); \
  callbackSetCallback(fn, ptr);   \
  callbackSetUser(valptr, ptr);   \
  (ptr)->timer=NULL;              \
} while(0)

/*Note: All locking involving the ISR done by disabling interrupts
 *      since the OSI library doesn't provide more efficient
 *      constructs like a ISR safe spinlock.
 */

extern "C" {
    /* Arbitrary throttleing of FIFO thread.
     * The FIFO thread has to run at a high priority
     * so the callbacks have low latency.  At the same
     * time we want to prevent starvation of lower
     * priority tasks if too many events are received.
     * This would cause the CA server to be starved
     * preventing remote correction of the problem.
     *
     * This should be less than the time between
     * the highest frequency event needed for
     * database processing.
     *
     * Note that the actual rate will be limited by the
     * time needed for database processing.
     *
     * Set to 0.0 to disable
     *
     * No point in making this shorter than the system tick
     */
    double mrmEvrFIFOPeriod = 1.0/ 1000.0; /* 1/rate in Hz */

    epicsExportAddress(double,mrmEvrFIFOPeriod);
}

/* Number of good updates before the time is considered valid */
#define TSValidThreshold 5

// Fractional synthesizer reference clock frequency
static
const double fracref=24.0; // MHz



long get_ioint_info_statusChange(int dir,dbCommon* prec,IOSCANPVT* io)
{
    IOStatus* stat=static_cast<IOStatus*>(prec->dpvt);

    if(!stat) return 1;

    *io=stat->statusChange((dir != 0));

    return 0;
}


EVRMRM::EVRMRM(const std::string& n,
               bus_configuration& busConfig,
               volatile epicsUInt8* b,
               epicsUInt32 bl)
  :mrf::ObjectInst<EVRMRM>(n)
  ,evrLock()
  ,id(n)
  ,base(b)
  ,baselen(bl)
  ,count_recv_error(0)
  ,count_hardware_irq(0)
  ,count_heartbeat(0)
  ,shadowIRQEna(0)
  ,count_FIFO_overflow(0)
  ,busConfiguration(busConfig)
  ,outputs()
  ,prescalers()
  ,pulsers()
  ,shortcmls()
  ,gpio_(*this)
  ,drain_fifo_method(*this)
  ,drain_fifo_task(drain_fifo_method, "EVRFIFO",
                   epicsThreadGetStackSize(epicsThreadStackBig),
                   epicsThreadPriorityHigh )
  // 3 because 2 IRQ events, and 1 shutdown event
  ,drain_fifo_wakeup(3,sizeof(int))
  ,count_FIFO_sw_overrate(0)
  ,stampClock(0.0)
  ,shadowSourceTS(TSSourceInternal)
  ,shadowCounterPS(0)
  ,timestampValid(0)
  ,lastInvalidTimestamp(0)
  ,lastValidTimestamp(0)
  ,m_remoteFlash(n+":Flash",b)

{
try{
    epicsUInt32 v, evr,ver;

    v = fpgaFirmware();
    evr=v&FWVersion_type_mask;
    evr>>=FWVersion_type_shift;

    if(evr!=0x1)
        throw std::runtime_error("Address does not correspond to an EVR");

    ver = version();
    firmwareVersion = ver;
    if(ver<3)
        throw std::runtime_error("Firmware versions < 3 not supported");

    scanIoInit(&IRQmappedEvent);
    scanIoInit(&IRQheartbeat);
    scanIoInit(&IRQrxError);
    scanIoInit(&IRQfifofull);
    scanIoInit(&timestampValidChange);

    CBINIT(&drain_log_cb   , priorityMedium, &EVRMRM::drain_log , this);
    CBINIT(&poll_link_cb   , priorityMedium, &EVRMRM::poll_link , this);

    if(ver>=5) {
        std::ostringstream name;
        name<<id<<":SFP0";
        sfp.reset(new SFP(name.str(), base + U32_SFPEEPROM_base));
    }

    /*
     * Create subunit instances
     */

    formFactor form = getFormFactor();

    size_t nPul=16; // number of pulsers
    size_t nPS=3;   // number of prescalers
    // # of outputs (Front panel, FP Universal, Rear transition module)
    size_t nOFP=0, nOFPUV=0, nORB=0;
    size_t nOFPDly=0;  // # of slots== # of delay modules. Some of the FP Universals have GPIOs. Each FPUV==2 GPIO pins, 2 FPUVs in one slot = 4 GPIO pins. One dly module uses 4 GPIO pins.
    // # of CML outputs
    size_t nCML=0;
    EvrCML::outkind kind=EvrCML::typeCML;
    // # of FP inputs
    size_t nIFP=0;

    printf("%s: ", formFactorStr().c_str());
    switch(form){
    case formFactor_CPCI:
        nOFPUV=4;
        nOFPDly=2;
        nIFP=2;
        nORB=6;
        break;
    case formFactor_PMC:
        nOFP=3;
        nIFP=1;
        break;
    case formFactor_VME64:
        if(ver >= 200){  //This is for vme300
            nOFP=0;
            nCML=4; // FP univ out 6-9 are CML
            nOFPDly=4;
            nOFPUV=10;
            nORB=16;
            nIFP=2;
            nPul = 32;
            nPS = 8;
        }else{
            nOFP=7;
            nCML=3; // OFP 4-6 are CML
            nOFPDly=2;
            nOFPUV=4;
            nORB=16;
            nIFP=2;
        }
        break;
    case formFactor_CPCIFULL:
        nOFPUV=4;
        kind=EvrCML::typeTG300;
        break;
    case formFactor_PCIe:
        nOFPUV=16;
        break;
    default:
        printf("Unknown EVR form factor %d\n",v);
    }
    printf("Out FP:%u FPUNIV:%u RB:%u IFP:%u GPIO:%u\n",
           (unsigned int)nOFP,(unsigned int)nOFPUV,
           (unsigned int)nORB,(unsigned int)nIFP,
           (unsigned int)nOFPDly);

    // Special output for mapping bus interrupt
    //outputs[std::make_pair(OutputInt,0)]=new EvrOutput(base+U16_IRQPulseMap);

    inputs.resize(nIFP);
    for(size_t i=0; i<nIFP; i++){
        std::ostringstream name;
        name<<id<<":FPIn"<<i;
        inputs[i]=new EvrInput(name.str(), base,i);
    }

    for(size_t i=0; i<nOFP; i++){
        std::ostringstream name;
        name<<id<<":FrontOut"<<i;
        outputs[std::make_pair(OutputFP,(epicsUInt32)i)]=new EvrOutput(name.str(), this, OutputFP, i);
    }

    for(size_t i=0; i<nOFPUV; i++){
        std::ostringstream name;
        name<<id<<":FrontUnivOut"<<i;
        outputs[std::make_pair(OutputFPUniv,(epicsUInt32)i)]=new EvrOutput(name.str(), this, OutputFPUniv, i);
    }

    for(size_t i=0; i<nORB; i++){
        std::ostringstream name;
        name<<id<<":RearUniv"<<i;
        outputs[std::make_pair(OutputRB,(epicsUInt32)i)]=new EvrOutput(name.str(), this, OutputRB, i);
    }

    delays.resize(nOFPDly);
    for(size_t i=0; i<nOFPDly; i++){
        std::ostringstream name;
        name<<id<<":UnivDlyModule"<<i;
        delays[i]=new EvrDelayModule(name.str(), this, i);
    }

    prescalers.resize(nPS);
    for(size_t i=0; i<nPS; i++){
        std::ostringstream name;
        name<<id<<":PS"<<i;
        prescalers[i]=new EvrPrescaler(name.str(), base, i);
    }

    pulsers.resize(nPul);
    for(size_t i=0; i<nPul; i++){
        std::ostringstream name;
        name<<id<<":Pul"<<i;
        pulsers[i]=new EvrPulser(name.str(), *this, i);
    }

    if(v==formFactor_CPCIFULL) {
        shortcmls.resize(8);
        for(size_t i=4; i<8; i++) {
            std::ostringstream name;
            name<<id<<":FrontOut"<<i;
            outputs[std::make_pair(OutputFP,(epicsUInt32)i)]=new EvrOutput(name.str(), this, OutputFP, i);
        }
        for(size_t i=0; i<4; i++)
            shortcmls[i]=0;
        shortcmls[4]=new EvrCML(id+":CML4", 4,*this,EvrCML::typeCML,form);
        shortcmls[5]=new EvrCML(id+":CML5", 5,*this,EvrCML::typeCML,form);
        shortcmls[6]=new EvrCML(id+":CML6", 6,*this,EvrCML::typeTG300,form);
        shortcmls[7]=new EvrCML(id+":CML7", 7,*this,EvrCML::typeTG300,form);

    } else if(nCML && ver>=4){
        shortcmls.resize(nCML);
        for(size_t i=0; i<nCML; i++){
            std::ostringstream name;
            name<<id<<":CML"<<i;
            shortcmls[i]=new EvrCML(name.str(), i, *this, kind, form);
        }

    }else if(nCML){
        printf("CML outputs not supported with this firmware\n");
    }

    for(epicsUInt32 i=0; i<NELEMENTS(this->events); i++) {
        events[i].code=i;
        events[i].owner=this;
        CBINIT(&events[i].done, priorityLow, &EVRMRM::sentinel_done , &events[i]);
    }

    // TODO: use define for version number
    if(ver < MIN_FW_SEGMENTED_DBUFF) {
        m_dataBuffer = new mrmNonSegmentedDataBuffer(base, U32_DataTxCtrlEvr, U32_DataRxCtrlEvr, U32_DataTxBaseEvr, U32_DataRxBaseEvr);
    } else {
        m_dataBuffer = new mrmDataBuffer(base, U32_DataTxCtrlEvr, U32_DataRxCtrlEvr, U32_DataTxBaseEvr, U32_DataRxBaseEvr);
    }
    CBINIT(&dataBufferRx_cb, priorityHigh, &mrmDataBuffer::handleDataBufferRxIRQ, &*m_dataBuffer);

    SCOPED_LOCK(evrLock);

    memset(_mapped, 0, sizeof(_mapped));
    // restore mapping ram to a clean state
    // needed when the IOC is started w/o a device reset (ie Linux)
    //TODO: find a way to do this that doesn't require clearing
    //      mapping which will shortly be set again...
    for(size_t i=0; i<255; i++) {
        WRITE32(base, MappingRam(0, i, Internal), 0);
        WRITE32(base, MappingRam(0, i, Trigger), 0);
        WRITE32(base, MappingRam(0, i, Set), 0);
        WRITE32(base, MappingRam(0, i, Reset), 0);
    }

    // restore default special mappings
    // These may be replaced later
    specialSetMap(MRF_EVENT_TS_SHIFT_0,     96, true);
    specialSetMap(MRF_EVENT_TS_SHIFT_1,     97, true);
    specialSetMap(MRF_EVENT_TS_COUNTER_INC, 98, true);
    specialSetMap(MRF_EVENT_TS_COUNTER_RST, 99, true);
    specialSetMap(MRF_EVENT_HEARTBEAT,      101, true);

    // Except for Prescaler reset, which is set with a record
    specialSetMap(MRF_EVENT_RST_PRESCALERS, 100, false);

    eventClock=FracSynthAnalyze(READ32(base, FracDiv),
                                fracref,0)*1e6;

    shadowCounterPS=READ32(base, CounterPS);

    if(tsDiv()!=0) {
        shadowSourceTS=TSSourceInternal;
    } else {
        bool usedbus4=(READ32(base, Control) & Control_tsdbus) != 0;

        if(usedbus4)
            shadowSourceTS=TSSourceDBus4;
        else
            shadowSourceTS=TSSourceEvent;
    }

    eventNotifyAdd(MRF_EVENT_TS_COUNTER_RST, &seconds_tick, (void*)this);

    drain_fifo_task.start();

} catch (std::exception& e) {
    printf("Aborting EVR initializtion: %s\n", e.what());
    cleanup();
    throw;
}
}

EVRMRM::~EVRMRM()
{
    cleanup();
}

void
EVRMRM::cleanup()
{
    printf("%s shuting down... ", name().c_str());
    int wakeup=1;
    drain_fifo_wakeup.send(&wakeup, sizeof(wakeup));
    drain_fifo_task.exitWait();

    for(outputs_t::iterator it=outputs.begin();
        it!=outputs.end(); ++it)
    {
        delete it->second;
    }
    outputs.clear();

    delete m_dataBuffer;

#define CLEANVEC(TYPE, VAR) \
    for(TYPE::iterator it=VAR.begin(); it!=VAR.end(); ++it) \
    { delete (*it); } \
    VAR.clear();

    CLEANVEC(inputs_t, inputs);
    CLEANVEC(prescalers_t, prescalers);
    CLEANVEC(pulsers_t, pulsers);
    CLEANVEC(shortcmls_t, shortcmls);

#undef CLEANVEC
    printf("complete\n");
}

std::string
EVRMRM::versionSw() const
{
    return MRF_VERSION;
}


bus_configuration *
EVRMRM::getBusConfiguration(){
    return &busConfiguration;
}

std::string
EVRMRM::position() const
{
    std::ostringstream position;

    if(busConfiguration.busType == busType_pci) position << busConfiguration.pci.bus << ":" << busConfiguration.pci.device << "." << busConfiguration.pci.function;
    else if(busConfiguration.busType == busType_vme) position << "Slot #" << busConfiguration.vme.slot;
    else position << "Unknown position";

    return position.str();
}

epicsUInt32
EVRMRM::model() const
{
    epicsUInt32 v = READ32(base, FWVersion);

    return (v&FWVersion_form_mask)>>FWVersion_form_shift;
}

epicsUInt32
EVRMRM::fpgaFirmware(){
    return READ32(base, FWVersion);
}

epicsUInt32
EVRMRM::version() const
{
    epicsUInt32 v = READ32(base, FWVersion);

    return (v&FWVersion_ver_mask)>>FWVersion_ver_shift;
}

formFactor
EVRMRM::getFormFactor(){
    epicsUInt32 form = model();

    /**
     * Removing 'formFactor_CPCI <= form' from the if condition since
     * 'form' is unsigned and 'formFactor_CPCI' is 0. 'form' can never
     * be less than 0 which makes this comparison always true and
     * therefore superfluous.
     *
     * Changed by: jkrasna
     */
    if(form <= formFactor_PCIe) return (formFactor)form;
    else return formFactor_unknown;
}

std::string
EVRMRM::formFactorStr(){
    std::string text;
    formFactor form;

    form = getFormFactor();
    switch(form){
    case formFactor_CPCI:
        text = "CompactPCI 3U";
        break;

    case formFactor_CPCIFULL:
        text = "CompactPCI 6U";
        break;

    case formFactor_CRIO:
        text = "CompactRIO";
        break;

    case formFactor_PCIe:
        text = "PCIe";
        break;

    case formFactor_PXIe:
        text = "PXIe";
        break;

    case formFactor_PMC:
        text = "PMC";
        break;

    case formFactor_VME64:
        text = "VME 64";
        break;

    default:
        text = "Unknown form factor";
    }

    return text;
}

bool
EVRMRM::enabled() const
{
    epicsUInt32 v = READ32(base, Control);
    return (v&Control_enable) != 0;
}

void
EVRMRM::enable(bool v)
{
    SCOPED_LOCK(evrLock);
    if(v)
        BITSET(NAT,32,base, Control, Control_enable|Control_mapena|Control_outena|Control_evtfwd);
    else
        BITCLR(NAT,32,base, Control, Control_enable|Control_mapena|Control_outena|Control_evtfwd);
}

EvrPulser*
EVRMRM::pulser(epicsUInt32 i) const
{
    if(i>=pulsers.size())
        throw std::out_of_range("Pulser id is out of range");
    return pulsers[i];
}

EvrOutput*
EVRMRM::output(OutputType otype,epicsUInt32 idx) const
{
    outputs_t::const_iterator it=outputs.find(std::make_pair(otype,idx));
    if(it==outputs.end())
        return 0;
    else
        return it->second;
}

EvrDelayModule*
EVRMRM::delay(epicsUInt32 i){
    if(i>=delays.size())
        throw std::out_of_range("Delay Module id is out of range.");
    return delays[i];
}

EvrInput*
EVRMRM::input(epicsUInt32 i) const
{
    if(i>=inputs.size())
        throw std::out_of_range("Input id is out of range");
    return inputs[i];
}

EvrPrescaler*
EVRMRM::prescaler(epicsUInt32 i) const
{
    if(i>=prescalers.size())
        throw std::out_of_range("PreScaler id is out of range");
    return prescalers[i];
}

EvrCML*
EVRMRM::cml(epicsUInt32 i) const
{
    if(i>=shortcmls.size() || !shortcmls[i])
        throw std::out_of_range("CML Short id is out of range");
    return shortcmls[i];
}

EvrGPIO*
EVRMRM::gpio(){
    return &gpio_;
}

void
EVRMRM::setDelayCompensationEnabled(bool enabled){
    if(enabled){
        BITCLR(NAT,32, base, Control, Control_dlyComp_disable);
    }else{
        BITSET(NAT, 32, base, Control, Control_dlyComp_disable);
    }
}

bool
EVRMRM::isDelayCompensationEnabled() const{
    return (READ32(base, Control) & Control_dlyComp_disable) == 0;
}

epicsUInt32
EVRMRM::delayCompensationTarget() const{
    return READ32(base, DCTarget);
}

void
EVRMRM::setDelayCompensationTarget(epicsUInt32 target){
    WRITE32(base, DCTarget, target);
}

epicsUInt32
EVRMRM::delayCompensationRxValue() const{
    return READ32(base, DCRxValue);
}

epicsUInt32
EVRMRM::delayCompensationIntValue() const{
    return READ32(base, DCIntValue);
}

epicsUInt32
EVRMRM::delayCompensationStatus() const{
    return READ32(base, DCStatus);
}

bool
EVRMRM::specialMapped(epicsUInt32 code, epicsUInt32 func) const
{
    if(code>255)
        throw std::out_of_range("Event code is out of range (0-255)");
    if(func>127 || func<96 ||
        (func<=121 && func>=102) )
    {
        throw std::out_of_range("Special function code is out of range. Valid ranges: 96-101 and 122-127");
    }

    if(code==0)
        return false;

    SCOPED_LOCK(evrLock);

    return _ismap(code,func-96);
}

void
EVRMRM::specialSetMap(epicsUInt32 code, epicsUInt32 func,bool v)
{
    if(code>255)
        throw std::out_of_range("Event code is out of range");
    /* The special function codes are the range 96 to 127, excluding 102 to 121
     */
    if(func>127 || func<96 ||
        (func<=121 && func>=102) )
    {
        errlogPrintf("EVR %s code %02x func %3d out of range. Code range is 0-255, where function rangs are 96-101 and 122-127\n",
            id.c_str(), code, func);
        throw std::out_of_range("Special function code is out of range.  Valid ranges: 96-101 and 122-127");
    }

    if(code==0)
      return;

    /* The way the latch timestamp is implimented in hardware (no status bit)
     * makes it impossible to use the latch mapping and the latch control register
     * bits at the same time.  We use the control register bits.
     * However, there is not much loss of functionality since all events
     * can be timestamped in the FIFO.
     */
    if(func==126)
        throw std::out_of_range("Use of latch timestamp special function code is not allowed");

    epicsUInt32 bit  =func%32;
    epicsUInt32 mask=1<<bit;

    SCOPED_LOCK(evrLock);

    epicsUInt32 val=READ32(base, MappingRam(0, code, Internal));

    if (v == _ismap(code,func-96)) {
        // mapping already set defined

    } else if(v) {
        _map(code,func-96);
        WRITE32(base, MappingRam(0, code, Internal), val|mask);
    } else {
        _unmap(code,func-96);
        WRITE32(base, MappingRam(0, code, Internal), val&~mask);
    }
}

void
EVRMRM::clockSet(double freq)
{
    double err;
    // Set both the fractional synthesiser and microsecond
    // divider.
    printf("Set EVR clock %f\n",freq);

    freq/=1e6;

    epicsUInt32 newfrac=FracSynthControlWord(
                        freq, fracref, 0, &err);

    if(newfrac==0)
        throw std::out_of_range("New frequency can't be used");

    SCOPED_LOCK(evrLock);

    epicsUInt32 oldfrac=READ32(base, FracDiv);

    if(newfrac!=oldfrac){
        // Changing the control word disturbes the phase
        // of the synthesiser which will cause a glitch.
        // Don't change the control word unless needed.

        WRITE32(base, FracDiv, newfrac);

        eventClock=FracSynthAnalyze(READ32(base, FracDiv),
                                    fracref,0)*1e6;
    }

    // USecDiv is accessed as a 32 bit register, but
    // only 16 are used.
    epicsUInt16 oldudiv=READ32(base, USecDiv);
    epicsUInt16 newudiv=(epicsUInt16)freq;

    if(newudiv!=oldudiv){
        WRITE32(base, USecDiv, newudiv);
    }
}

epicsUInt32
EVRMRM::uSecDiv() const
{
    return READ32(base, USecDiv);
}

bool
EVRMRM::extInhib() const
{
    epicsUInt32 v = READ32(base, Control);
    return (v&Control_GTXio) != 0;
}

void
EVRMRM::setExtInhib(bool v)
{
    SCOPED_LOCK(evrLock);
    if(v)
        BITSET(NAT,32,base, Control, Control_GTXio);
    else
        BITCLR(NAT,32,base, Control, Control_GTXio);
}

bool
EVRMRM::cgLocked() const
{
    return (READ32(base, ClkCtrl) & ClkCtrl_cglock) != 0;
}

bool
EVRMRM::pllLocked() const
{
    return (READ32(base, ClkCtrl) & ClkCtrl_plllock) != 0;
}

void
EVRMRM::setPllBandwidth(PLLBandwidth pllBandwidth)
{
    epicsUInt32 clkCtrl;
    epicsUInt32 bw;

    if(pllBandwidth > PLLBandwidth_MAX){
        throw std::out_of_range("PLL bandwith you selected is not available.");
    }

    bw = (epicsUInt32)pllBandwidth;
    bw = bw << ClkCtrl_bwsel_shift;

    clkCtrl = READ32(base, ClkCtrl);
    clkCtrl = clkCtrl & ~ClkCtrl_bwsel;
    clkCtrl = clkCtrl | bw;
    WRITE32(base, ClkCtrl, clkCtrl);
}

PLLBandwidth
EVRMRM::pllBandwidth() const
{
    epicsUInt32 bw;

    bw = (READ32(base, ClkCtrl) & ClkCtrl_bwsel);
    bw = bw >> ClkCtrl_bwsel_shift;

    return (PLLBandwidth)bw;
}

bool
EVRMRM::linkStatus() const
{
    return !(READ32(base, Status) & Status_legvio);
}

void
EVRMRM::setSourceTS(TSSource src)
{
    double clk=clockTS(), eclk=clock();
    epicsUInt16 div=0;

    if(clk<=0 || !isfinite(clk))
        throw std::out_of_range("TS Clock rate invalid");

    switch(src){
    case TSSourceInternal:
    case TSSourceEvent:
    case TSSourceDBus4:
        break;
    default:
        throw std::out_of_range("TS source invalid");
    }

    SCOPED_LOCK(evrLock);

    switch(src){
    case TSSourceInternal:
        // div!=0 selects src internal
        div=(epicsUInt16)(eclk/clk);
        break;
    case TSSourceEvent:
        BITCLR(NAT,32, base, Control, Control_tsdbus);
        // div=0
        break;
    case TSSourceDBus4:
        BITSET(NAT,32, base, Control, Control_tsdbus);
        // div=0
        break;
    }
    WRITE32(base, CounterPS, div);
    shadowCounterPS=div;
    shadowSourceTS=src;
}

double
EVRMRM::clockTS() const
{
    //Note: acquires evrLock 3 times.

    TSSource src=SourceTS();

    if(src!=TSSourceInternal)
        return stampClock;

    epicsUInt16 div=tsDiv();

    return clock()/div;
}

void
EVRMRM::clockTSSet(double clk)
{
    if(clk<0.0 || !isfinite(clk))
        throw std::out_of_range("TS Clock rate invalid");

    TSSource src=SourceTS();
    double eclk=clock();

    if(clk>eclk || clk==0.0)
        clk=eclk;

    SCOPED_LOCK(evrLock);

    if(src==TSSourceInternal){
        epicsUInt16 div=roundToUInt(eclk/clk, 0xffff);
        WRITE32(base, CounterPS, div);

        shadowCounterPS=div;
    }

    stampClock=clk;
}

bool
EVRMRM::interestedInEvent(epicsUInt32 event,bool set)
{
    if (!event || event>255) return false;

    eventCode *entry=&events[event];

    SCOPED_LOCK(evrLock);

    if (   (set  && entry->interested==0) // first interested
        || (!set && entry->interested==1) // or last un-interested
    ) {
        specialSetMap(event, ActionFIFOSave, set);
    }

    if (set)
        entry->interested++;
    else
        entry->interested--;

    return true;
}

bool
EVRMRM::TimeStampValid() const
{
    SCOPED_LOCK(evrLock);
    return timestampValid>=TSValidThreshold;
}

bool
EVRMRM::getTimeStamp(epicsTimeStamp *ts,epicsUInt32 event)
{
    if(!ts) throw std::runtime_error("Invalid argument");

    SCOPED_LOCK(evrLock);
    if(timestampValid<TSValidThreshold) return false;

    if(event>0 && event<=255) {
        // Get time of last event code #

        eventCode *entry=&events[event];

        // Fail if event is not mapped
        if (!entry->interested ||
            ( entry->last_sec==0 &&
              entry->last_evt==0) )
        {
            return false;
        }

        ts->secPastEpoch=entry->last_sec;
        ts->nsec=entry->last_evt;


    } else {
        // Get current absolute time

        epicsUInt32 ctrl=READ32(base, Control);

        // Latch timestamp
        WRITE32(base, Control, ctrl|Control_tsltch);

        ts->secPastEpoch=READ32(base, TSSecLatch);
        ts->nsec=READ32(base, TSEvtLatch);

        /* BUG: There was a firmware bug which occasionally
         * causes the previous write to fail with a VME bus
         * error, and 0 the Control register.
         *
         * This issues has been fixed in VME firmwares EVRv 5
         * pre2 and EVG v3 pre2.  Feb 2011
         */
        epicsUInt32 ctrl2=READ32(base, Control);
        if (ctrl2!=ctrl) { // tsltch bit is write-only
            printf("Get timestamp: control register write fault. Written: %08x, readback: %08x\n",ctrl,ctrl2);
            WRITE32(base, Control, ctrl);
        }

    }

    if(!convertTS(ts))
        return false;

    return true;
}

/** @brief In place conversion between raw posix sec+ticks to EPICS sec+nsec.
 @returns false if conversion failed
 */
bool
EVRMRM::convertTS(epicsTimeStamp* ts)
{
    // First validate the input

    //Has it been initialized?
    if(ts->secPastEpoch==0 || ts->nsec==0){
        return false;
    }

    // 1 sec. reset is late
    if(ts->nsec>=1000000000) {
        SCOPED_LOCK(evrLock);
        timestampValid=0;
        lastInvalidTimestamp=ts->secPastEpoch;
        scanIoRequest(timestampValidChange);
        return false;
    }

    // recurrence of an invalid time
    if(ts->secPastEpoch==lastInvalidTimestamp) {
        timestampValid=0;
        scanIoRequest(timestampValidChange);
        return false;
    }

    /* Reported seconds timestamp should be no more
     * then 1sec in the future.
     * reporting values in the past should be caught by
     * generalTime
     */
    if(ts->secPastEpoch > lastValidTimestamp+1)
    {
        errlogPrintf("EVR ignoring invalid TS %08x %08x (expect %08x)\n",
                     ts->secPastEpoch, ts->nsec, lastValidTimestamp);
        timestampValid=0;
        scanIoRequest(timestampValidChange);
        return false;
    }

    //Link seconds counter is POSIX time
    ts->secPastEpoch-=POSIX_TIME_AT_EPICS_EPOCH;

    // Convert ticks to nanoseconds
    double period=1e9/clockTS(); // in nanoseconds

    if(period<=0 || !isfinite(period))
        return false;

    ts->nsec=(epicsUInt32)(ts->nsec*period);
    return true;
}

bool
EVRMRM::getTicks(epicsUInt32 *tks)
{
    *tks=READ32(base, TSEvt);
    return true;
}

IOSCANPVT
EVRMRM::eventOccurred(epicsUInt32 event) const
{
    if (event>0 && event<=255)
        return events[event].occured;
    else
        return NULL;
}

void
EVRMRM::eventNotifyAdd(epicsUInt32 event, eventCallback cb, void* arg)
{
    if (event==0 || event>255)
        throw std::out_of_range("Invalid event number");

    SCOPED_LOCK2(evrLock, guard);

    events[event].notifiees.push_back( std::make_pair(cb,arg));

    interestedInEvent(event, true);
}

void
EVRMRM::eventNotifyDel(epicsUInt32 event, eventCallback cb, void* arg)
{
    if (event==0 || event>255)
        throw std::out_of_range("Invalid event number");

    SCOPED_LOCK2(evrLock, guard);

    events[event].notifiees.remove(std::make_pair(cb,arg));

    interestedInEvent(event, false);
}

epicsUInt16
EVRMRM::dbus() const
{
    return (READ32(base, Status) & Status_dbus_mask) >> Status_dbus_shift;
}

epicsUInt32
EVRMRM::dbusToPulserMapping(epicsUInt8 dbus) const{
    if(dbus > 7){
        throw std::out_of_range("Invalid DBus bit selected. Max: 7");
    }

    return READ32(base, DBusTrigger(dbus));
}

void
EVRMRM::setDbusToPulserMapping(epicsUInt8 dbus, epicsUInt32 pulsers){

    size_t noOfPulsers;

    noOfPulsers = this->pulsers.size();
    if(pulsers >> noOfPulsers){
        throw std::out_of_range("Invalid pulsers selected.");
    }

    if(dbus > 7){
        throw std::out_of_range("Invalid DBus bit selected. Max: 7");
    }

    return WRITE32(base, DBusTrigger(dbus), pulsers);
}

mrmDataBuffer *EVRMRM::getDataBuffer()
{
    return m_dataBuffer;
}

void
EVRMRM::enableIRQ(void)
{
    int key=epicsInterruptLock();

    shadowIRQEna =  IRQ_Enable
                   |IRQ_RXErr    |IRQ_BufFull
                   |IRQ_Heartbeat|IRQ_HWMapped
                   |IRQ_Event    |IRQ_FIFOFull;

    // IRQ PCIe enable flag should not be changed. Possible RACER here
    shadowIRQEna |= (IRQ_PCIee & (READ32(base, IRQEnable)));

    WRITE32(base, IRQEnable, shadowIRQEna);

    epicsInterruptUnlock(key);
}

void
EVRMRM::isr_pci(void *arg) {
    EVRMRM *evr=static_cast<EVRMRM*>(arg);

    // Calling the default platform-independent interrupt routine
    evr->isr(arg);

    if(devPCIEnableInterrupt((const epicsPCIDevice*)evr->isrLinuxPvt)) {
        printf("Failed to re-enable interrupt.  Stuck...\n");
    }
}

void
EVRMRM::isr_vme(void *arg) {
    EVRMRM *evr=static_cast<EVRMRM*>(arg);

    // Calling the default platform-independent interrupt routine
    evr->isr(arg);
}

// A place to write to which will keep the read
// at the end of the ISR from being optimized out.
// This value should never be used anywhere else.
volatile epicsUInt32 evrMrmIsrFlagsTrashCan;

void
EVRMRM::isr(void *arg)
{
    EVRMRM *evr=static_cast<EVRMRM*>(arg);

    epicsUInt32 flags=READ32(evr->base, IRQFlag);

    epicsUInt32 active=flags&evr->shadowIRQEna;

    if(!active)
        return;

    if(active&IRQ_RXErr){
        evr->count_recv_error++;
        scanIoRequest(evr->IRQrxError);

        evr->shadowIRQEna &= ~IRQ_RXErr;
        callbackRequest(&evr->poll_link_cb);
    }
    if(active&IRQ_BufFull){
        if (evr->m_dataBuffer->m_rx_irq_handled) {
            if (evr->firmwareVersion < MIN_FW_SEGMENTED_DBUFF) {
                // Silence interrupt. DataRxCtrl_stop is actually an interrupt flag, so we need to write to it in order to clear it.
                BITSET(NAT,32,evr->base, DataRxCtrlEvr, DataRxCtrl_stop);

                // Check if the data buffer engine is initialized. This bit should not be set, since we have just acknowledged the interrupt.
                // This is needed because 201 series firmware keeps triggering the interrupt with bogous data, and we do not wish
                // to schedule callback requests for this. It prevents cbHigh queue to fill up when starting the driver. This only happens at start-up.
                if ( !(READ32(evr->base, DataRxCtrlEvr) & DataRxCtrl_rdy) ) {
                    evr->m_dataBuffer->m_rx_irq_handled = false;
                    callbackRequest(&evr->dataBufferRx_cb);
                } else {
                    errlogPrintf("Could not acknowledge data buffer IRQ. Data buffer engine not yet initalized? Not issuing callbacks...\n"); // TODO printing in isr is ugly....but this should never be printed (except at startup on 201 firmware)
                }
            } else {
                memset((epicsUInt8 *)(evr->base+DataBuffer_SegmentIRQ), 0, 16); // clear segment IRQ flags
                evr->m_dataBuffer->m_rx_irq_handled = false;
                callbackRequest(&evr->dataBufferRx_cb);
            }
        }
    }
    if(active&IRQ_HWMapped){
        evr->shadowIRQEna &= ~IRQ_HWMapped;
        //TODO: think of a way to use this feature...
    }
    if(active&IRQ_Event){
        //FIFO not-empty
        evr->shadowIRQEna &= ~IRQ_Event;
        int wakeup=0;
        evr->drain_fifo_wakeup.trySend(&wakeup, sizeof(wakeup));
    }
    if(active&IRQ_Heartbeat){
        evr->count_heartbeat++;
        scanIoRequest(evr->IRQheartbeat);
    }
    if(active&IRQ_FIFOFull){
        evr->shadowIRQEna &= ~IRQ_FIFOFull;
        int wakeup=0;
        evr->drain_fifo_wakeup.trySend(&wakeup, sizeof(wakeup));

        scanIoRequest(evr->IRQfifofull);
    }
    evr->count_hardware_irq++;

    // IRQ PCIe enable flag should not be changed. Possible RACER here
    evr->shadowIRQEna |= (IRQ_PCIee & (READ32(evr->base, IRQEnable)));

    WRITE32(evr->base, IRQFlag, flags);
    WRITE32(evr->base, IRQEnable, evr->shadowIRQEna);
    // Ensure IRQFlags is written before returning.
    evrMrmIsrFlagsTrashCan=READ32(evr->base, IRQFlag);
}


// Caller must hold evrLock
static
void
eventInvoke(eventCode& event)
{
    scanIoRequest(event.occured);

    for(eventCode::notifiees_t::const_iterator it=event.notifiees.begin();
        it!=event.notifiees.end();
        ++it)
    {
        (*it->first)(it->second, event.code);
    }
}

void
EVRMRM::drain_fifo()
{
    size_t i;
    printf("EVR FIFO task start\n");

    SCOPED_LOCK2(evrLock, guard);

    while(true) {
        int code, err;

        guard.unlock();

        err=drain_fifo_wakeup.receive(&code, sizeof(code));

        if (err<0) {
            errlogPrintf("FIFO wakeup error %d\n",err);
            epicsThreadSleep(0.1); // avoid message flood
            guard.lock();
            continue;

        } else if(code==1) {
            // Request thread stop
            guard.lock();
            break;
        }

        guard.lock();

        count_fifo_loops++;

        epicsUInt32 status;

        if(evrEventDebug > 0) printf("Draining FIFO!\n");
        // Bound the number of events taken from the FIFO
        // at one time.
        for(i=0; i<512; i++) {

            status=READ32(base, IRQFlag);
            if (!(status&IRQ_Event))
                break;
            if (status&IRQ_RXErr)
                break;

            epicsUInt32 evt=READ32(base, EvtFIFOCode);
            if (!evt)
                break;

            if (evt>NELEMENTS(events)) {
                // BUG: we get occasional corrupt VME reads of this register
                // Fixed in firmware.  Feb 2011
                epicsUInt32 evt2=READ32(base, EvtFIFOCode);
                if (evt2>NELEMENTS(events)) {
                    printf("Really weird event 0x%08x 0x%08x\n", evt, evt2);
                    break;
                } else
                    evt=evt2;
            }
            evt &= 0xff; // (in)santity check

            count_fifo_events++;

            events[evt].last_sec=READ32(base, EvtFIFOSec);
            events[evt].last_evt=READ32(base, EvtFIFOEvt); // timestamp register

            if(evrEventDebug > 0) printf("%u.%u: %s received event: %d\n", events[evt].last_sec, events[evt].last_evt, id.c_str(), evt);


            if (events[evt].again) {
                // ignore extra events in buffer.
            } else if (events[evt].waitingfor>0) {
                // already queued, but occured again before
                // callbacks finished so disable event
                events[evt].again=true;
                specialSetMap(evt, ActionFIFOSave, false);
                count_FIFO_sw_overrate++;
            } else {
                // needs to be queued
                eventInvoke(events[evt]);
                events[evt].waitingfor=NUM_CALLBACK_PRIORITIES;
                for(int p=0; p<NUM_CALLBACK_PRIORITIES; p++) {
                    events[evt].done.priority=p;
                    callbackRequest(&events[evt].done);
                }
            }

        }

        if (status&IRQ_FIFOFull) {
            count_FIFO_overflow++;
        }

        if (status&(IRQ_FIFOFull|IRQ_RXErr)) {
            // clear fifo if link lost or buffer overflow
            BITSET(NAT,32, base, Control, Control_fiforst);
        }

        int iflags=epicsInterruptLock();

        //*
        shadowIRQEna |= IRQ_Event | IRQ_FIFOFull;
        // IRQ PCIe enable flag should not be changed. Possible RACER here
        shadowIRQEna |= (IRQ_PCIee & (READ32(base, IRQEnable)));

        WRITE32(base, IRQEnable, shadowIRQEna);

        epicsInterruptUnlock(iflags);

        // wait a fixed interval before checking again
        // Prevents this thread from starving others
        // if a high frequency event is accidentally
        // mapped into the FIFO.
        if(mrmEvrFIFOPeriod>0.0) {
            guard.unlock();
            epicsThreadSleep(mrmEvrFIFOPeriod);
            guard.lock();
        }
    }

    printf("FIFO task exiting\n");
}

void
EVRMRM::sentinel_done(CALLBACK* cb)
{
try {
    void *vptr;
    callbackGetUser(vptr,cb);
    eventCode *sent=static_cast<eventCode*>(vptr);

    SCOPED_LOCK2(sent->owner->evrLock, guard);

    // Is this the last callback queue?
    if (--sent->waitingfor)
        return;

    bool run=sent->again;
    sent->again=false;

    // Re-enable mapping if disabled
    if (run && sent->interested) {
        sent->owner->specialSetMap(sent->code, ActionFIFOSave, true);
    }
} catch(std::exception& e) {
    epicsPrintf("exception in sentinel_done callback: %s\n", e.what());
}
}


void
EVRMRM::drain_log(CALLBACK*)
{
}

void
EVRMRM::poll_link(CALLBACK* cb)
{
try {
    void *vptr;
    callbackGetUser(vptr,cb);
    EVRMRM *evr=static_cast<EVRMRM*>(vptr);

    epicsUInt32 flags=READ32(evr->base, IRQFlag);

    if(flags&IRQ_RXErr){
        // Still down
        callbackRequestDelayed(&evr->poll_link_cb, 0.1); // poll again in 100ms
        {
            SCOPED_LOCK2(evr->evrLock, guard);
            evr->timestampValid=0;
            evr->lastInvalidTimestamp=evr->lastValidTimestamp;
            scanIoRequest(evr->timestampValidChange);
        }
        WRITE32(evr->base, IRQFlag, IRQ_RXErr);
    }else{
        scanIoRequest(evr->IRQrxError);
        int iflags=epicsInterruptLock();
        evr->shadowIRQEna |= IRQ_RXErr;
        // IRQ PCIe enable flag should not be changed. Possible RACER here
        evr->shadowIRQEna |= (IRQ_PCIee & (READ32(evr->base, IRQEnable)));
        WRITE32(evr->base, IRQEnable, evr->shadowIRQEna);
        epicsInterruptUnlock(iflags);
    }
} catch(std::exception& e) {
    epicsPrintf("exception in poll_link callback: %s\n", e.what());
}
}

void
EVRMRM::seconds_tick(void *raw, epicsUInt32)
{
    EVRMRM *evr=static_cast<EVRMRM*>(raw);

    SCOPED_LOCK2(evr->evrLock, guard);

    // Don't bother to latch since we are only reading the seconds
    epicsUInt32 newSec=READ32(evr->base, TSSec);

    bool valid=true;

    /* Received a known bad value */
    if(evr->lastInvalidTimestamp==newSec)
        valid=false;

    /* Received a value which is inconsistent with a previous value */
    if(evr->timestampValid>0
       &&  evr->lastValidTimestamp!=(newSec-1) )
        valid=false;

    /* received the previous value again */
    else if(evr->lastValidTimestamp==newSec)
        valid=false;


    if (!valid)
    {
        if (evr->timestampValid>0) {
            errlogPrintf("TS reset w/ old or invalid seconds %08x (%08x %08x)\n",
                         newSec, evr->lastValidTimestamp, evr->lastInvalidTimestamp);
            scanIoRequest(evr->timestampValidChange);
        }
        evr->timestampValid=0;
        evr->lastInvalidTimestamp=newSec;

    } else {
        evr->timestampValid++;
        evr->lastValidTimestamp=newSec;

        if (evr->timestampValid == TSValidThreshold) {
            errlogPrintf("TS becomes valid after fault %08x\n",newSec);
            scanIoRequest(evr->timestampValidChange);
        }
    }


}
