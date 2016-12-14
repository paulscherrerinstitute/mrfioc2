#ifndef EVRSEQUENCER_H
#define EVRSEQUENCER_H

#include <stdio.h>
#include <vector>

#include "mrf/object.h"
#include <epicsTypes.h>
#include "dbScan.h"

#include "mrmShared.h"

/**
 * @brief mrfioc2_sequencerDebug defines debug level (verbosity of debug printout)
 */
extern "C" {
    extern int mrfioc2_sequencerDebug;
}

class EvrSequencer : public mrf::ObjectInst<EvrSequencer>
{
public:

    /**
     * @brief The SeqRunMode enum contains sequence run modes: Normal (not single, not recycle), Auto (not single, recycle), Single (single)
     */
    enum SeqRunMode {
        Normal = 0,
        Auto,
        Single
    };

    EvrSequencer(const std::string& n, volatile epicsUInt8 * b);

    /**
     * @brief setSequenceEvents sets the events in the sequence. It needs to be commited after it is set.
     * @param waveform of events
     * @param len is the length of the waveform
     */
    void setSequenceEvents(const epicsUInt16 *waveform, epicsUInt32 len);

    /**
     * @brief getSequenceEvents returns the currently commited events (reads from HW) in the sequence.
     * @param waveform of commited events
     * @param len is the length of the waveform
     * @return the length of the waveform (the same as len)
     */
    epicsUInt32 getSequenceEvents(epicsUInt16 *waveform, epicsUInt32 len) const;



    /**
     * @brief setSequenceTimestamp sets the event's timestamps in the sequence. It needs to be commited after it is set.
     * @param waveform of event timestamps
     * @param len is the length of the waveform
     */
    void setSequenceTimestamp(const double* waveform, epicsUInt32 len);

    /**
     * @brief getSequenceTimestamp returns the currently commited event timestamps (reads from HW) in the sequence.
     * @param waveform of commited event timestamps
     * @param len is the length of the waveform
     * @return the length of the waveform (the same as len)
     */
    epicsUInt32 getSequenceTimestamp(double *waveform, epicsUInt32 len) const;



    /**
     * @brief setTriggerSource sets how the sequence will be triggered.
     * @param source is the same source mapping as used for EVR output configuration
     */
    void setTriggerSource(epicsUInt32 source);

    /**
     * @brief getTriggerSource returns the currently commited trigger source.
     * @return the same source mapping as used for EVR output configuration
     */
    epicsUInt32 getTriggerSource() const;



    /**
     * @brief setRunMode sets the mode in which the sequence runs.
     * @param mode Normal (not single, not recycle), Auto (not single, recycle), Single (single)
     */
    void setRunMode(enum SeqRunMode mode);

    /**
     * @brief getRunMode returns the currently committed mode
     * @return Normal (not single, not recycle), Auto (not single, recycle), Single (single)
     */
    enum SeqRunMode getRunMode() const;



    /**
     * @brief enable or disable the sequence
     * @param ena true to enable, false to disable
     */
    void enable(bool ena);

    /**
     * @brief enabled checks if the sequence is currently enabled
     * @return true if enabled, false otherwise
     */
    bool enabled() const;



    /**
     * @brief softTrigger will start the sequence send procedure
     * @return true if command was sent, false othwerwise (eg. sequence was already running, sequence was disabled)
     */
    bool softTrigger() const;

    /**
     * @brief reset the sequence to start from beginning
     * @return always true
     */
    bool reset() const;

    /**
     * @brief commit will expand the event sequence to HW, if user provided sequence is valid.
     * @return true if command was successfull, false otherwise (eg. user provided sequence is not valid)
     */
    bool commit() const;

    /**
     * @brief sequenceValid returns status of the user provided events and event timestamps validity
     * @return true if user provided sequence is valid, false otherwise
     */
    bool sequenceValid() const;
    IOSCANPVT sequenceValidOccured() const{return m_sequence.irqValid;}


    /**
     * @brief sos should be called on start of sequence interrupt. This code should be quick since it's a part of the ISR!
     */
    void sos() {
        m_irqSosCount++;
        scanIoRequest(m_irqSos);
    }

    epicsUInt32 sosCount() const{return m_irqSosCount;}
    IOSCANPVT sosOccured() const{return m_irqSos;}

    /**
     * @brief eos should be called on end of sequence interrupt. This code should be quick since it's a part of the ISR!
     */
    void eos() {
        m_irqEosCount++;
        scanIoRequest(m_irqEos);
    }

    epicsUInt32 eosCount() const{return m_irqEosCount;}
    IOSCANPVT eosOccured() const{return m_irqEos;}

    /**
     * @brief lock should be called whenever accessing this class starts
     */
    void lock() const{guard.lock();}

    /**
     * @brief unlock should be called whenever accessing this class is finished
     */
    void unlock() const{guard.unlock();}


    /** helper for EPICS object access **/
    void setRunModeRaw(epicsUInt16 r){setRunMode((enum SeqRunMode)r);}
    epicsUInt16 getRunModeRaw() const{return (epicsUInt16)getRunMode();}

private:
    volatile epicsUInt8* const base;    // start of memory-mapped address space

    IOSCANPVT m_irqSos;                   // start of sequence interrupt
    volatile epicsUInt32 m_irqSosCount;   // number of times SOS IRQ occured
    IOSCANPVT m_irqEos;                   // end of sequence interrupt
    volatile epicsUInt32 m_irqEosCount;   // number of times EOS IRQ occured

    struct {
        std::vector<epicsUInt8> eventCode;  // user provided event codes
        std::vector<epicsUInt64> timestamp; // user provided timestamps (in Event Clock Ticks)
        size_t noContinuationEvents;        // how many continuation events we need to add
        bool userProvidedEosEvent;          // did user provide EOS event or not
        bool valid;                         // is the current user provided sequence valid?
        IOSCANPVT irqValid;                 // notifies EPICS record when valid status is updated
    } m_sequence;

    mutable epicsMutex guard;   // guards access to this class

    /**
     * @brief running checks if the sequence is currently running.
     * @return true if running, false otherwise
     */
    bool running() const;

    /**
     * @brief checkSequenceSize will check if number of events and timestamps is OK
     * @return true if settings are ok, false otherwise
     */
    bool checkSequenceSize();

    /**
     * @brief isSourceValid checks the source variable for correct range
     * @param source is the value to check
     * @return true if source is valid, false otherwise
     */
    bool isSourceValid(epicsUInt32 source) const;
};

#endif // EVRSEQUENCER_H
