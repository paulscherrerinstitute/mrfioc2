/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef EVRMRML_H_INC
#define EVRMRML_H_INC

#define MRMREMOTEFLASH_H_LEVEL2
#define SFP_H_LEVEL2
#define DATABUF_H_INC_LEVEL2

#include <string>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <utility>

#include <dbScan.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsMessageQueue.h>
#include <callback.h>
#include <epicsMutex.h>

#include "evrInput.h"
#include "evrOutput.h"
#include "evrPrescaler.h"
#include "evrPulser.h"
#include "evrCML.h"
#include "evrDelayModule.h"
#include "evrRxBuf.h"

#include "evrGpio.h"

#include "mrmDataBufTx.h"
#include "sfp.h"
#include "configurationInfo.h"

#include "mrmremoteflash.h"

enum TSSource {
  TSSourceInternal=0,
  TSSourceEvent=1,
  TSSourceDBus4=2
};

/* PLL Bandwidth Select (see Silicon Labs Si5317 datasheet)
 *  000 – Si5317, BW setting HM (lowest loop bandwidth)
 *  001 – Si5317, BW setting HL
 *  010 – Si5317, BW setting MH
 *  011 – Si5317, BW setting MM
 *  100 – Si5317, BW setting ML (highest loop bandwidth)
 */
enum PLLBandwidth {
    PLLBandwidth_HM=0,
    PLLBandwidth_HL=1,
    PLLBandwidth_MH=2,
    PLLBandwidth_MM=3,
    PLLBandwidth_ML=4,
    PLLBandwidth_MAX=PLLBandwidth_ML
};


//! @brief Helper to allow one class to have several runable methods
template<class C,void (C::*Method)()>
class epicsThreadRunableMethod : public epicsThreadRunable
{
    C& owner;
public:
    epicsThreadRunableMethod(C& o)
        :owner(o)
    {}
    ~epicsThreadRunableMethod(){}
    void run()
    {
        (owner.*Method)();
    }
};

class EVRMRM;
typedef void (*eventCallback)(void* userarg, epicsUInt32 event);
struct eventCode {
    epicsUInt8 code; // constant
    EVRMRM* owner;

    // For efficiency events will only
    // be mapped into the FIFO when this
    // counter is non-zero.
    size_t interested;

    epicsUInt32 last_sec;
    epicsUInt32 last_evt;

    IOSCANPVT occured;

    typedef std::list<std::pair<eventCallback,void*> > notifiees_t;
    notifiees_t notifiees;

    CALLBACK done;
    size_t waitingfor;
    bool again;

    eventCode():owner(0), interested(0), last_sec(0)
            ,last_evt(0), notifiees(), waitingfor(0), again(false)
    {
        scanIoInit(&occured);
        // done - initialized in EVRMRM::EVRMRM()
    }
};


/**@brief Modular Register Map Event Receivers
 *
 * 
 */
class epicsShareClass EVRMRM : public mrf::ObjectInst<EVRMRM>
{
public:    
    /** @brief Guards access to instance
   *  All callers must take this lock before any operations on
   *  this object.
   */
    mutable epicsMutex evrLock;


    EVRMRM(const std::string& n, bus_configuration& busConfig, volatile epicsUInt8 *, epicsUInt32);

    ~EVRMRM();
private:
    void cleanup();
public:

    void lock() const{evrLock.lock();};
    void unlock() const{evrLock.unlock();};

    //! Hardware model
    epicsUInt32 model() const;
    epicsUInt32 fpgaFirmware();
    formFactor getFormFactor();
    std::string formFactorStr();

    //! Firmware Version
    epicsUInt32 version() const;

    //! Software Version -> from version.h
    std::string versionSw() const;

    //! Position of EVR device in enclosure.
    std::string position() const;
    bus_configuration* getBusConfiguration();


    bool enabled() const;
    void enable(bool v);

    EvrPulser* pulser(epicsUInt32) const;

    EvrOutput* output(OutputType,epicsUInt32 o) const;

    EvrDelayModule* delay(epicsUInt32 i);

    EvrInput* input(epicsUInt32) const;

    EvrPrescaler* prescaler(epicsUInt32) const;

    EvrCML* cml(epicsUInt32) const;

    EvrGPIO* gpio();

    void setDelayCompensationEnabled(bool enabled);
    bool isDelayCompensationEnabled() const;
    epicsUInt32 delayCompensationTarget() const;
    void setDelayCompensationTarget(epicsUInt32 target);
    epicsUInt32 delayCompensationRxValue() const;
    epicsUInt32 delayCompensationIntValue() const;
    epicsUInt32 delayCompensationStatus() const;

    /** Hook to handle general event mapping table manipulation.
     *  Allows 'special' events only (ie heartbeat, log, led, etc)
     *  Normal mappings (pulsers, outputs) must be made through the
     *  appropriate class (Pulser, Output).
     *
     * Note: this is one place where Device Support will have some depth.
     */
    bool specialMapped(epicsUInt32 code, epicsUInt32 func) const;
    void specialSetMap(epicsUInt32 code, epicsUInt32 func,bool);

    /**Set LO frequency
     *@param clk Clock rate in Hz
     */
    double clock() const
        {SCOPED_LOCK(evrLock);return eventClock;}
    void clockSet(double);

    //! Internal PLL Status and bandwidth
    bool pllLocked() const;
    void setPllBandwidth(PLLBandwidth pllBandwidth);
    PLLBandwidth pllBandwidth() const;

    epicsUInt32 irqCount() const{return count_hardware_irq;}

    bool linkStatus() const;
    IOSCANPVT linkChanged() const{return IRQrxError;}
    epicsUInt32 recvErrorCount() const{return count_recv_error;}

    //! Approximate divider from event clock period to 1us
    epicsUInt32 uSecDiv() const;
    /*@}*/

    //! Using external hardware input for inhibit?
    bool extInhib() const;
    void setExtInhib(bool);

    //!When using internal TS source gives the divider from event clock period to TS period
    epicsUInt32 tsDiv() const
        {SCOPED_LOCK(evrLock);return shadowCounterPS;}

    //!Select source which increments TS counter
    void setSourceTS(TSSource);
    TSSource SourceTS() const
        {SCOPED_LOCK(evrLock);return shadowSourceTS;}

    /**Find current TS settings
     *@returns Clock rate in Hz
     */
    double clockTS() const;

    /**Set TS frequency
     *@param clk Clock rate in Hz
     */
    void clockTSSet(double);

    /** Indicate (lack of) interest in a particular event code.
     *  This allows an EVR to ignore event codes which are not needed.
     */
    bool interestedInEvent(epicsUInt32 event,bool set);

    bool TimeStampValid() const;
    IOSCANPVT TimeStampValidEvent() const{return timestampValidChange;}

    /** Gives the current time stamp as sec+nsec
     *@param ts This pointer will be filled in with the current time
     *@param event N<=0 Return the current wall clock time
     *@param event N>0  Return the time the most recent event # N was received.
     *@return true When ts was updated
     *@return false When ts could not be updated
     */
    bool getTimeStamp(epicsTimeStamp *ts,epicsUInt32 event);

    /** Returns the current value of the Timestamp Event Counter
     *@param tks Pointer to be filled with the counter value
     *@return false if the counter value is not valid
     */
    bool getTicks(epicsUInt32 *tks);

    IOSCANPVT eventOccurred(epicsUInt32 event) const;
    void eventNotifyAdd(epicsUInt32, eventCallback, void*);
    void eventNotifyDel(epicsUInt32, eventCallback, void*);

    bool convertTS(epicsTimeStamp* ts);

    epicsUInt16 dbus() const;
    epicsUInt32 dbusToPulserMapping(epicsUInt8 dbus) const;
    void setDbusToPulserMapping(epicsUInt8 dbus, epicsUInt32 pulsers);

    epicsUInt32 heartbeatTIMOCount() const{return count_heartbeat;}
    IOSCANPVT heartbeatTIMOOccured() const{return IRQheartbeat;}

    epicsUInt32 FIFOFullCount() const
    {SCOPED_LOCK(evrLock);return count_FIFO_overflow;}
    epicsUInt32 FIFOOverRate() const
    {SCOPED_LOCK(evrLock);return count_FIFO_sw_overrate;}
    epicsUInt32 FIFOEvtCount() const{return count_fifo_events;}
    epicsUInt32 FIFOLoopCount() const{return count_fifo_loops;}

    void enableIRQ(void);

    static void isr(void*);
    static void isr_pci(void*);
    static void isr_vme(void*);
#if defined(__linux__) || defined(_WIN32)
    const void *isrLinuxPvt;
#endif

    const std::string id;
    volatile unsigned char * const base;
    epicsUInt32 baselen;
    mrmDataBufTx buftx;
    mrmBufRx bufrx;
    std::auto_ptr<SFP> sfp;

    /**\defgroup devhelp Device Support Helpers
     *
     * These functions exists to make life easier for device support
     */
    /*@{*/
    void setSourceTSraw(epicsUInt32 r){setSourceTS((TSSource)r);}
    epicsUInt32 SourceTSraw() const{return (epicsUInt32)SourceTS();}

    void setPllBandwidthRaw(epicsUInt16 r){setPllBandwidth((PLLBandwidth)r);}
    epicsUInt16 pllBandwidthRaw() const{return (epicsUInt16)pllBandwidth();}

    void setDbusToPulserMapping0(epicsUInt32 pulsers){setDbusToPulserMapping(0, pulsers);}
    epicsUInt32 dbusToPulserMapping0() const{return dbusToPulserMapping(0);}
    void setDbusToPulserMapping1(epicsUInt32 pulsers){setDbusToPulserMapping(1, pulsers);}
    epicsUInt32 dbusToPulserMapping1() const{return dbusToPulserMapping(1);}
    void setDbusToPulserMapping2(epicsUInt32 pulsers){setDbusToPulserMapping(2, pulsers);}
    epicsUInt32 dbusToPulserMapping2() const{return dbusToPulserMapping(2);}
    void setDbusToPulserMapping3(epicsUInt32 pulsers){setDbusToPulserMapping(3, pulsers);}
    epicsUInt32 dbusToPulserMapping3() const{return dbusToPulserMapping(3);}
    void setDbusToPulserMapping4(epicsUInt32 pulsers){setDbusToPulserMapping(4, pulsers);}
    epicsUInt32 dbusToPulserMapping4() const{return dbusToPulserMapping(4);}
    void setDbusToPulserMapping5(epicsUInt32 pulsers){setDbusToPulserMapping(5, pulsers);}
    epicsUInt32 dbusToPulserMapping5() const{return dbusToPulserMapping(5);}
    void setDbusToPulserMapping6(epicsUInt32 pulsers){setDbusToPulserMapping(6, pulsers);}
    epicsUInt32 dbusToPulserMapping6() const{return dbusToPulserMapping(6);}
    void setDbusToPulserMapping7(epicsUInt32 pulsers){setDbusToPulserMapping(7, pulsers);}
    epicsUInt32 dbusToPulserMapping7() const{return dbusToPulserMapping(7);}

    /*@}*/

private:

    // Set by ISR
    volatile epicsUInt32 count_recv_error;
    volatile epicsUInt32 count_hardware_irq;
    volatile epicsUInt32 count_heartbeat;
    volatile epicsUInt32 count_fifo_events;
    volatile epicsUInt32 count_fifo_loops;

    epicsUInt32 shadowIRQEna;

    // Guarded by evrLock
    epicsUInt32 count_FIFO_overflow;

    // scanIoRequest() from ISR or callback
    IOSCANPVT IRQmappedEvent; // Hardware mapped IRQ
    IOSCANPVT IRQheartbeat;   // Heartbeat timeout
    IOSCANPVT IRQrxError;     // Rx link state change
    IOSCANPVT IRQfifofull;    // Fifo overflow

    // Software events
    IOSCANPVT timestampValidChange;

    // Set by ctor, not changed after

    bus_configuration busConfiguration;

    typedef std::vector<EvrInput*> inputs_t;
    inputs_t inputs;

    typedef std::map<std::pair<OutputType,epicsUInt32>,EvrOutput*> outputs_t;
    outputs_t outputs;

    std::vector<EvrDelayModule*> delays;

    typedef std::vector<EvrPrescaler*> prescalers_t;
    prescalers_t prescalers;

    typedef std::vector<EvrPulser*> pulsers_t;
    pulsers_t pulsers;

    typedef std::vector<EvrCML*> shortcmls_t;
    shortcmls_t shortcmls;

    EvrGPIO gpio_;

    // run when FIFO not-full IRQ is received
    void drain_fifo();
    epicsThreadRunableMethod<EVRMRM, &EVRMRM::drain_fifo> drain_fifo_method;
    epicsThread drain_fifo_task;
    epicsMessageQueue drain_fifo_wakeup;
    static void sentinel_done(CALLBACK*);

    epicsUInt32 count_FIFO_sw_overrate;

    eventCode events[256];

    // Buffer received
    CALLBACK data_rx_cb;

    // Called when the Event Log is stopped
    CALLBACK drain_log_cb;
    static void drain_log(CALLBACK*);

    // Periodic callback to detect when link state goes from down to up
    CALLBACK poll_link_cb;
    static void poll_link(CALLBACK*);

    // Set by clockTSSet() with IRQ disabled
    double stampClock;
    TSSource shadowSourceTS;
    epicsUInt32 shadowCounterPS;
    double eventClock; //!< Stored in Hz

    epicsUInt32 timestampValid;
    epicsUInt32 lastInvalidTimestamp;
    epicsUInt32 lastValidTimestamp;
    static void seconds_tick(void*, epicsUInt32);

    // bit map of which event #'s are mapped
    // used as a safty check to avoid overloaded mappings
    epicsUInt32 _mapped[256];

    void _map(epicsUInt8 evt, epicsUInt8 func)   { _mapped[evt] |=    1<<(func);  }
    void _unmap(epicsUInt8 evt, epicsUInt8 func) { _mapped[evt] &= ~( 1<<(func) );}
    bool _ismap(epicsUInt8 evt, epicsUInt8 func) const { return (_mapped[evt] & 1<<(func)) != 0; }


    mrmRemoteFlash m_remoteFlash;

}; // class EVRMRM

#endif // EVRMRML_H_INC
