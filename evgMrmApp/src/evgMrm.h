#ifndef EVG_MRM_H
#define EVG_MRM_H

/*
 * Windows
 * Theese flip the flags for importing from dll, rather then exporting
 */
#define MRMREMOTEFLASH_H_LEVEL2
#define SFP_H_LEVEL2

#include <vector>
#include <map>
#include <string>

#include <stdio.h>
#include <dbScan.h>
#include <callback.h>
#include <epicsMutex.h>
#include <epicsTimer.h>
#include <errlog.h>
#include <epicsTime.h>
#include <generalTimeSup.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsMutex.h>

#include <devLibPCI.h>

#include "evgAcTrig.h"
#include "evgEvtClk.h"
#include "evgSoftEvt.h"
#include "evgTrigEvt.h"
#include "evgMxc.h"
#include "evgDbus.h"
#include "evgInput.h"
#include "evgOutput.h"
#include "evgSequencer/evgSeqRamManager.h"
#include "evgSequencer/evgSoftSeqManager.h"

#include "mrmShared.h"
#include "sfp.h"
#include "evgFct.h"
#include "mrmremoteflash.h"
#include "dataBuffer/mrmDataBuffer.h"
#include "dataBuffer/mrmNonSegmentedDataBuffer.h"

/*********
 * Each EVG will be represented by the instance of class 'evgMrm'. Each evg 
 * object maintains a list to all the evg sub-componets i.e. Event clock,
 * Software Events, Trigger Events, Distributed bus, Multiplex Counters, 
 * Input, Output etc.
 */
class wdTimer;
class wdTimer1;

enum ALARM_TS {TS_ALARM_NONE, TS_ALARM_MINOR, TS_ALARM_MAJOR};

class evgMrm : public mrf::ObjectInst<evgMrm> {
public:
    evgMrm(const std::string& id, bus_configuration& busConfig, volatile epicsUInt8* const, volatile epicsUInt8* const, const epicsPCIDevice* pciDevice);
    ~evgMrm();

    /* locking done internally */
    virtual void lock() const{};
    virtual void unlock() const{};

    /** EVG    **/
    const std::string getId() const;
    volatile epicsUInt8* getRegAddr() const;
    epicsUInt32 getFwVersion() const;
    epicsUInt32 getFwVersionID();
    formFactor getFormFactor();
    std::string getFormFactorStr();
    std::string getSwVersion() const;

    void enable(bool);
    bool enabled() const;

    void resetMxc(bool reset);
    epicsUInt16 getDbusStatus() const;

    /* From sequence ram control register */
    void setSWSequenceMask(epicsUInt16 mask);
    epicsUInt16 getSWSequenceMask() const;

    void setSWSequenceEnable(epicsUInt16 enable);
    epicsUInt16 getSWSequenceEnable() const;
    /*************************************/

    void dlyCompBeaconEnable(bool ena);
    bool dlyCompBeaconEnabled() const;

    void dlyCompMasterEnable(bool ena);
    bool dlyCompMasterEnabled() const;

    /**    Interrupt and Callback    **/
    static void isr(void*);
    static void isr_pci(void*);
    static void isr_vme(void*);
    static void init_cb(CALLBACK*, int, void(*)(CALLBACK*), void*);
    static void process_eos0_cb(CALLBACK*);
    static void process_eos1_cb(CALLBACK*);
    static void process_sos0_cb(CALLBACK*);
    static void process_sos1_cb(CALLBACK*);
    static void process_inp_cb(CALLBACK*);

    /** TimeStamp    **/
    epicsUInt32 sendTimestamp();
    epicsTimeStamp getTimestamp() const;
    void syncTimestamp();
    void syncTsRequest();
    void incrTimestamp();
    
    /**    Access    functions     **/
    evgAcTrig* getAcTrig();
    evgEvtClk* getEvtClk();
    evgSoftEvt* getSoftEvt();
    evgTrigEvt* getTrigEvt(epicsUInt32);
    evgMxc* getMuxCounter(epicsUInt32);
    evgDbus* getDbus(epicsUInt32);
    evgInput* getInput(epicsUInt32, InputType);

    evgOutput* getOutput(epicsUInt32, evgOutputType);
    evgSeqRamMgr* getSeqRamMgr();
    evgSoftSeqMgr* getSoftSeqMgr();
    epicsEvent* getTimerEvent();
    bus_configuration* getBusConfiguration();
    std::vector<SFP *> *getSFP();
    mrmDataBuffer* getDataBuffer();

    typedef std::map< std::pair<epicsUInt32, InputType>, evgInput*> Input_t;
    Input_t                       m_input;  // TODO the rest of the sub-units are private...(evgSeqRam needs this)

    CALLBACK                      irqStop0_cb;
    CALLBACK                      irqStop1_cb;
    CALLBACK                      irqStart0_cb;
    CALLBACK                      irqStart1_cb;
    CALLBACK                      irqExtInp_cb;

#ifdef __linux__
    void* isrLinuxPvt;
#endif
    // flags for CB rate limiting
    unsigned char irqStop0_queued;
    unsigned char irqStop1_queued;
    unsigned char irqStart0_queued;
    unsigned char irqStart1_queued;
    unsigned char irqExtInp_queued;

    IOSCANPVT                     ioScanTimestamp;
    bool                          m_syncTimestamp;
    ALARM_TS                      m_alarmTimestamp;

    void show(int lvl);

    const epicsPCIDevice*         m_pciDevice;

private:
    const std::string             m_id;
    volatile epicsUInt8* const    m_pReg;   // EVG function register map
    volatile epicsUInt8* const    m_fctReg; // FCT function register map
    bus_configuration             busConfiguration;

    evgAcTrig                     m_acTrig;
    evgEvtClk                     m_evtClk;
    evgSoftEvt                    m_softEvt;

    mrmRemoteFlash                m_remoteFlash;

    typedef std::vector<evgTrigEvt*> TrigEvt_t;
    TrigEvt_t                     m_trigEvt;

    typedef std::vector<evgMxc*> MuxCounter_t;
    MuxCounter_t                  m_muxCounter;

    typedef std::vector<evgDbus*> Dbus_t;
    Dbus_t                        m_dbus;

    typedef std::map< std::pair<epicsUInt32, evgOutputType>, evgOutput*> Output_t;
    Output_t                      m_output;

    evgFct*                       m_fct;

    evgSeqRamMgr                  m_seqRamMgr;
    evgSoftSeqMgr                 m_softSeqMgr;

    epicsTimeStamp                m_timestamp;

    wdTimer*                      m_wdTimer;
    epicsEvent*                   m_timerEvent;

    std::vector<SFP*>             m_sfp;    // upstream + fanout transceivers. Transceiver indexed 0 is upstream transceiver.

    mrmDataBuffer*                m_dataBuffer;
};


/*Creating a timer thread bcz epicsTimer uses epicsGeneralTime and when
  generalTime stops working even the timer stop working*/
class wdTimer : public epicsThreadRunable {
public:
    wdTimer(const char *name, evgMrm* evg);

    virtual void run();

    void decrPilotCount();

    bool getPilotCount();

    epicsMutex  m_lock;

private:
    epicsThread m_thread;
    evgMrm*     m_evg;
    epicsInt32  m_pilotCount;
};

#endif //EVG_MRM_H
