#ifndef EVRSEQUENCER_H
#define EVRSEQUENCER_H

class evrSequencer
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

    IOSCANPVT m_irq_sos;                    // start of sequence interrupt
    volatile epicsUInt32 m_irq_sos_count;   // number of times SOS IRQ occured
    IOSCANPVT m_irq_eos;                    // end of sequence interrupt
    volatile epicsUInt32 m_irq_eos_count;   // number of times EOS IRQ occured
    IOSCANPVT m_irq_synced;                 // IO interrupt which occurs when settings are synced to HW
    bool m_isSynced;                        // indicates if settings need to be synced to HW
    epicsMutex m_synced_lock;               // Protects race condition between EOS IRQ and user thread when trying to save new sequence to HW

    evrSequencer();

    /**
     * @brief setSequenceEvents sets the events in the sequence. It needs to be commited after it is set.
     * @param waveform of events
     * @param len is the length of the waveform
     */
    void setSequenceEvents(const char* waveform, epicsUInt32 len);

    /**
     * @brief getSequenceEvents returns the currently commited events in the sequence.
     * @param waveform of commited events
     * @param len is the length of the waveform
     * @return the length of the waveform (should be the same as len, or there was an error in copying data)
     */
    epicsUInt32 getSequenceEvents(char* waveform,epicsUInt32 len) const;



    /**
     * @brief setSequenceTimestamp sets the event's timestamps in the sequence. It needs to be commited after it is set.
     * @param waveform of event timestamps
     * @param len is the length of the waveform
     */
    void setSequenceTimestamp(const char* waveform, epicsUInt32 len);

    /**
     * @brief getSequenceTimestamp returns the currently commited event timestamps in the sequence.
     * @param waveform of commited event timestamps
     * @param len is the length of the waveform
     * @return the length of the waveform (should be the same as len, or there was an error in copying data)
     */
    epicsUInt32 getSequenceTimestamp(char* waveform,epicsUInt32 len) const;



    /**
     * @brief setTriggerSource sets how the sequence will be triggered. It needs to be commited after it is set.
     * @param source is the same source mapping as used for EVR output configuration
     */
    void setTriggerSource(epicsUInt32 source);

    /**
     * @brief getTriggerSource returns the currently commited trigger source.
     * @return the same source mapping as used for EVR output configuration
     */
    epicsUInt32 getTriggerSource() const;



    /**
     * @brief setRunMode sets the mode in which the sequence runs. It needs to be committed after it is set.
     * @param mode Normal (not single, not recycle), Auto (not single, recycle), Single (single)
     */
    void setRunMode(enum SeqRunMode);

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
     * @brief running checks if the sequence is currently running.
     * @return true if running, false otherwise
     */
    bool running() const;

    /**
     * @brief commit will validate() the settings and then save() them to HW if validation is OK (on next EOS IRQ if sequence is currently running, otherwise right away)
     * @return true if command was successfull, false otherwise (eg. error in settings)
     */
    bool commit() const;

    /**
     * @brief stop will finish the current sequence and then stop (sets mode to single and trigger source to force low).
     * @return always true
     */
    bool stop() const;

    /**
     * @brief isSynced is called whenever settings are saved to HW.
     * @return always true
     */
    bool isSynced() const{return true;}
    IOSCANPVT isSynced() const{return m_irq_synced;}


    /**
     * @brief sos should be called on start of sequence interrupt
     */
    void sos() {
        m_irq_sos_count++;
        scanIoRequest(m_irq_sos);
    }

    epicsUInt32 sosOccured() const{return m_irq_sos_count;}
    IOSCANPVT sosOccured() const{return m_irq_sos;}

    /**
     * @brief eos should be called on end of sequence interrupt
     */
    void eos() {
        m_irq_eos_count++;
        scanIoRequest(m_irq_eos);

        if (m_synced_lock.tryLock()) {  // TODO does this throw exception?
            if(!m_isSynced) {
                save();
                m_isSynced = true;
                scanIoRequest(m_irq_synced);
            }
            m_synced_lock.unlock();
        }
    }

    epicsUInt32 eosOccured() const{return m_irq_eos_count;}
    IOSCANPVT eosOccured() const{return m_irq_eos;}


    /** helper for EPICS object access **/
    void setRunModeRaw(epicsUInt16 r){setRunMode((enum SeqRunMode)r);}
    epicsUInt16 getRunModeRaw() const{return (epicsUInt16)getRunMode();}

private:
    /**
     * @brief reset the sequence to start from beginning
     * @return always true
     */
    bool reset() const;

    /**
     * @brief validate will check if settings are ok (event and timestamp waveforms of the same length, timestamps monotonically increasing, ...)
     * @return true if settings are ok, false otherwise
     */
    bool validate();

    /**
     * @brief save settings (trigger source, trigger mode, events with timestamps) to HW.
     */
    void save();
};

#endif // EVRSEQUENCER_H
