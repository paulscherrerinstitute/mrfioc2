#include <stdexcept>

#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#include "evrRegMap.h"

#include <epicsExport.h>
#include "evrSequencer.h"


#define MAX_SEQUENCE_SIZE 2048
#define EVENT_END_OF_SEQUENCE 0x7F  // this event signalizes end of sequence
#define TIMESTAMP_END_OF_SEQUENCE 5 // number of ticks behind last timestamp to insert end of sequence event if not user provided


extern "C" {
    int mrfioc2_sequencerDebug = 0;
    epicsExportAddress(int, mrfioc2_sequencerDebug);
}

#define dbgPrintf(level,M, ...) if(mrfioc2_sequencerDebug >= level) fprintf(stderr, "mrfioc2_sequencerDebug: " M,##__VA_ARGS__)

EvrSequencer::EvrSequencer(const std::string& n, volatile epicsUInt8 * b)
    :mrf::ObjectInst<EvrSequencer>(n)
    ,base(b)
    ,m_irqSosCount(0)
    ,m_irqEosCount(0)
{
    scanIoInit(&m_irqSos);
    scanIoInit(&m_irqEos);
    scanIoInit(&m_sequence.irqValid);

    m_sequence.noContinuationEvents = 0;
    m_sequence.valid = false;
    m_sequence.userProvidedEosEvent = false;
    m_sequence.timestamp.reserve(MAX_SEQUENCE_SIZE);
    m_sequence.eventCode.reserve(MAX_SEQUENCE_SIZE);
}

void EvrSequencer::setSequenceEvents(const epicsUInt16 *waveform, epicsUInt32 len)
{
    m_sequence.eventCode.clear();
    m_sequence.userProvidedEosEvent = false;
    m_sequence.valid = false;
    epicsUInt8 event;
    bool sizeOK = true;

    for(epicsUInt32 i=0; i<len; i++) {
        event = (epicsUInt8)waveform[i];

        // check if user provided end of sequence event
        if(event == EVENT_END_OF_SEQUENCE) {
            dbgPrintf(1,"EOS event detected\n");
            m_sequence.userProvidedEosEvent = true;
            m_sequence.eventCode.push_back(event);
            break;
        }

        if(i > MAX_SEQUENCE_SIZE) {
            dbgPrintf(1,"Too many events\n");
            sizeOK = false;
            break;
        }

        m_sequence.eventCode.push_back(event);
    }

    if(!sizeOK) {
        m_sequence.valid = false;
        scanIoRequest(m_sequence.irqValid);
    }
    else {
        checkSequenceSize();
    }
}

epicsUInt32 EvrSequencer::getSequenceEvents(epicsUInt16 *waveform, epicsUInt32 len) const
{
    for(epicsUInt32 i = 0; i<len; i++) {
        waveform[i] = READ8(base, EVR_SeqRamEvent(0,i));
    }

    return len;
}

void EvrSequencer::setSequenceTimestamp(const double *waveform, epicsUInt32 len)
{
    m_sequence.timestamp.clear();
    m_sequence.noContinuationEvents = 0;
    epicsUInt64 timestamp;
    bool sorted = true, sizeOK = true;

    for(epicsUInt32 i=0; i<len; i++) {
        timestamp = (epicsUInt64)waveform[i];   // TODO conversion from user units
        if(i < len-1 && (timestamp >= (epicsUInt64)waveform[i+1])) { // check if the timestamps are sorted and unique
            dbgPrintf(1,"Timestamps not sorted! Timestamp at %u:%llu, timestamp at %u: %llu\n", i, timestamp, i+1, (epicsUInt64)waveform[i+1]);
            sorted = false;
            break;
        }

        if(timestamp > 0xFFFFFFFF) {
            m_sequence.noContinuationEvents += timestamp % 0xFFFFFFFF;
        }

        if(len + m_sequence.noContinuationEvents > MAX_SEQUENCE_SIZE) {
            dbgPrintf(1,"Too many timestamps (including continuation events)\n");
            sizeOK = false;
            break;
        }

        m_sequence.timestamp.push_back(timestamp);
    }

    if(!sorted || !sizeOK) {
        m_sequence.valid = false;
        scanIoRequest(m_sequence.irqValid);
    }
    else {
        checkSequenceSize();
    }
}

epicsUInt32 EvrSequencer::getSequenceTimestamp(double *waveform, epicsUInt32 len) const
{
    for(epicsUInt32 i = 0; i<len; i++) {
        waveform[i] = (double)READ32(base, EVR_SeqRamTS(0,i));  // TODO conversion to user units
    }

    return len;
}

void EvrSequencer::setTriggerSource(epicsUInt32 source)
{
    if(!isSourceValid(source)) {
        throw std::out_of_range("Trigger source code is out of range");
    }
    dbgPrintf(1,"Setting trigger source %u\n", source);

    epicsUInt32 reg = READ32(base, EVR_SeqRamCtrl);
    reg &= ~EVR_SeqRamCtrl_TSEL_mask;
    reg |= source;

    WRITE32(base, EVR_SeqRamCtrl, reg);
}

epicsUInt32 EvrSequencer::getTriggerSource() const
{
    dbgPrintf(1,"Getting trigger source: %u\n", READ32(base, EVR_SeqRamCtrl) & EVR_SeqRamCtrl_TSEL_mask);
    return READ32(base, EVR_SeqRamCtrl) & EVR_SeqRamCtrl_TSEL_mask;
}

void EvrSequencer::setRunMode(EvrSequencer::SeqRunMode mode)
{
    switch (mode) {
        case(Normal):
            BITCLR32(base, EVR_SeqRamCtrl, EVR_SeqRamCtrl_SNG);
            BITCLR32(base, EVR_SeqRamCtrl, EVR_SeqRamCtrl_REC);
            break;
        case(Auto):
            BITCLR32(base, EVR_SeqRamCtrl, EVR_SeqRamCtrl_SNG);
            BITSET32(base, EVR_SeqRamCtrl, EVR_SeqRamCtrl_REC);
            break;
        case(Single):
            BITSET32(base, EVR_SeqRamCtrl, EVR_SeqRamCtrl_SNG);
            break;
        default:
            throw std::out_of_range("Unknown sequence run mode");
    }
}

EvrSequencer::SeqRunMode EvrSequencer::getRunMode() const
{
    epicsUInt32 reg = READ32(base, EVR_SeqRamCtrl);

    if(reg & EVR_SeqRamCtrl_SNG) {
        return Single;
    }

    if(reg & EVR_SeqRamCtrl_REC) {
        return Auto;
    }

    return Normal;
}

void EvrSequencer::enable(bool ena)
{
    if(ena) {
        BITSET32(base, EVR_SeqRamCtrl, EVR_SeqRamCtrl_EN);
        BITCLR32(base, EVR_SeqRamCtrl, EVR_SeqRamCtrl_DIS);
    }
    else {
        BITSET32(base, EVR_SeqRamCtrl, EVR_SeqRamCtrl_DIS);
        BITCLR32(base, EVR_SeqRamCtrl, EVR_SeqRamCtrl_EN);
    }
}

bool EvrSequencer::enabled() const
{
    return READ32(base, EVR_SeqRamCtrl) & EVR_SeqRamCtrl_ENA;
}

bool EvrSequencer::softTrigger() const
{
    if(!enabled() || running()) {
        dbgPrintf(1,"Soft trigger command not sent, because sequence is running or is not enabled\n");
        return false;
    }

    BITSET32(base, EVR_SeqRamCtrl, EVR_SeqRamCtrl_SWT);
    dbgPrintf(1,"Soft trigger command sent.\n");

    return true;
}

bool EvrSequencer::running() const
{
    dbgPrintf(1,"Sequence running: %u\n", (bool)(READ32(base, EVR_SeqRamCtrl) & EVR_SeqRamCtrl_RUN));
    return READ32(base, EVR_SeqRamCtrl) & EVR_SeqRamCtrl_RUN;
}

bool EvrSequencer::reset() const
{
    dbgPrintf(1,"Reset sequence command sent.\n");
    BITSET32(base, EVR_SeqRamCtrl, EVR_SeqRamCtrl_RES);
    return true;
}

bool EvrSequencer::commit() const
{
    if(!m_sequence.valid) {
        dbgPrintf(1,"Sequence is not valid. Not commiting.\n");
        return false;
    }

    dbgPrintf(1,"Starting commit...\n");

    // by now both sizes are equal, since we did validations before.
    size_t j = 0;
    epicsUInt64 timestamp;
    for(size_t i = 0; i < m_sequence.eventCode.size(); i++, j++) {
        timestamp = m_sequence.timestamp[i];

        // take care of continuation events
        while(timestamp > 0xFFFFFFFF) {
            WRITE32(base, EVR_SeqRamTS(0,j), 0xFFFFFFFF);
            WRITE8(base, EVR_SeqRamEvent(0,j), 0);

            timestamp -= 0xFFFFFFFF;
            j++;
        }

        WRITE32(base, EVR_SeqRamTS(0,j), timestamp);
        WRITE8(base, EVR_SeqRamEvent(0,j), m_sequence.eventCode[i]);
    }

    // user did not provide EOS event. Let's do it for him...
    // note, that we know that the sizes are OK, since we did validations before.
    if(!m_sequence.userProvidedEosEvent) {
        if(m_sequence.timestamp.size() > 0) {
            timestamp = m_sequence.timestamp.back() + TIMESTAMP_END_OF_SEQUENCE;

            // check if continuation events need to be inserted
            while(timestamp > 0xFFFFFFFF) {
                WRITE32(base, EVR_SeqRamTS(0,j), 0xFFFFFFFF);
                WRITE8(base, EVR_SeqRamEvent(0,j), 0);

                timestamp -= 0xFFFFFFFF;
                j++;
            }
            WRITE32(base, EVR_SeqRamTS(0,j), timestamp);
            WRITE8(base, EVR_SeqRamEvent(0,j), EVENT_END_OF_SEQUENCE);
        }
        else {
            WRITE32(base, EVR_SeqRamTS(0,j), 0);
            WRITE8(base, EVR_SeqRamEvent(0,j), EVENT_END_OF_SEQUENCE);
        }
    }

    dbgPrintf(1,"Commit finished!\n");
    return true;
}

bool EvrSequencer::sequenceValid() const
{
    dbgPrintf(1,"Sequence valid read-back: %d\n", m_sequence.valid);
    return m_sequence.valid;
}

bool EvrSequencer::checkSequenceSize()
{
    if(m_sequence.eventCode.size() > m_sequence.timestamp.size()) {
        m_sequence.valid = false;
        dbgPrintf(1,"Sequence not valid: there is more events than timestamps\n");
    }
    else if(m_sequence.eventCode.size() < m_sequence.timestamp.size()) {
        m_sequence.valid = false;
        dbgPrintf(1,"Sequence not valid: there is more timestamps than events\n");
    }
    else {
        m_sequence.valid = true;
    }

    if(!m_sequence.userProvidedEosEvent) {
        if(m_sequence.eventCode.size() > MAX_SEQUENCE_SIZE + 1) {
            dbgPrintf(1,"Too many events (including EOS event)\n");
            m_sequence.valid = false;
        }

        if(m_sequence.timestamp.size() > 0) {
            epicsUInt64 lastTimestamp = m_sequence.timestamp.back() + TIMESTAMP_END_OF_SEQUENCE;

            if(lastTimestamp < m_sequence.timestamp.back()) {
                dbgPrintf(1,"Timestamp overflow when adding end of sequence event\n");
                m_sequence.valid = false;
            }

            if(lastTimestamp > 0xFFFFFFFF) {
                if(m_sequence.noContinuationEvents + m_sequence.timestamp.size() + (lastTimestamp % 0xFFFFFFFF) > MAX_SEQUENCE_SIZE) {
                    dbgPrintf(1,"Adding end of sequence events results in a continuation event, which results in too many timestamps\n");
                    m_sequence.valid = false;
                }
            }
        }
    }

    scanIoRequest(m_sequence.irqValid);
    return m_sequence.valid;
}

bool EvrSequencer::isSourceValid(epicsUInt32 source) const
{
    if(   (source<=63 && source>=62) ||
          (source<=47 && source>=32) ||
          (source<=31)               ||
          (source==61)               )
    {
        return true;
    }
    else {
        return false;
    }
}
