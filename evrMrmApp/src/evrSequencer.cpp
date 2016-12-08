#include <stdio.h>

#include <stdexcept>

#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#include "evrRegMap.h"
#include "evrSequencer.h"


#define MAX_SEQUENCE_SIZE 2048
#define EVENT_END_OF_SEQUENCE 0x7F
#define TIMESTAMP_END_OF_SEQUENCE 5



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
        printf("%u -> %u\n", i, (epicsUInt8)waveform[i]);
    }
    printf("\n");
    for(epicsUInt32 i=0; i<len; i++) {
        event = (epicsUInt8)waveform[i];
        // check if user provided end of sequence event
        if(event == EVENT_END_OF_SEQUENCE) {
            printf("EOS event detected\n");
            m_sequence.userProvidedEosEvent = true;
            m_sequence.eventCode.push_back(event);
            break;
        }

        if(i > MAX_SEQUENCE_SIZE) {
            printf("Too many events\n");
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
        printf("%u -> %llu\n", i, (epicsUInt64)waveform[i]);
    }
    printf("\n");
    for(epicsUInt32 i=0; i<len; i++) {
        timestamp = (epicsUInt64)waveform[i];   // TODO conversion from user units
        if(i < len-1 && (timestamp >= (epicsUInt64)waveform[i+1])) { // check if the timestamps are sorted and unique
            printf("Timestamps not sorted! Timestamp at %u:%llu, timestamp at %u: %llu\n", i, timestamp, i+1, (epicsUInt64)waveform[i+1]);
            sorted = false;
            break;
        }

        if(timestamp > 0xFFFFFFFF) {
            m_sequence.noContinuationEvents += timestamp % 0xFFFFFFFF;
        }

        if(len + m_sequence.noContinuationEvents > MAX_SEQUENCE_SIZE) {
            printf("Too many timestamps (including continuation events)\n");
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
    printf("Setting trigger source %u\n", source);

    epicsUInt32 reg = READ32(base, EVR_SeqRamCtrl);
    reg &= ~EVR_SeqRamCtrl_TSEL_mask;
    reg |= source;

    WRITE32(base, EVR_SeqRamCtrl, reg);
}

epicsUInt32 EvrSequencer::getTriggerSource() const
{
    printf("Getting trigger source: %u\n", READ32(base, EVR_SeqRamCtrl) & EVR_SeqRamCtrl_TSEL_mask);
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
            throw std::runtime_error("Unknown sequence run mode");
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
    printf("Soft trigger...");
    if(!enabled() || running()) {
        return false;
    }

    BITSET32(base, EVR_SeqRamCtrl, EVR_SeqRamCtrl_SWT);
    printf("sent!\n");

    return true;
}

bool EvrSequencer::running() const
{
    printf("Running: %u\n", (bool)(READ32(base, EVR_SeqRamCtrl) & EVR_SeqRamCtrl_RUN));
    return READ32(base, EVR_SeqRamCtrl) & EVR_SeqRamCtrl_RUN;
}

bool EvrSequencer::reset() const
{
    printf("Reset\n");
    BITSET32(base, EVR_SeqRamCtrl, EVR_SeqRamCtrl_RES);
    return true;
}

bool EvrSequencer::commit() const
{
    printf("Commit. Is valid: %d\n", m_sequence.valid);
    if(!m_sequence.valid) {
        return false; // TODO throw?
    }

    // by now both sizes are equal, since we did validations before.
    size_t j = 0;
    epicsUInt64 lastTimestamp;
    for(size_t i = 0; i < m_sequence.eventCode.size(); i++, j++) {
        // take care of continuation events
        printf("i=%zu, j=%zu\n", i, j);
        lastTimestamp = m_sequence.timestamp[i];
        while(lastTimestamp > 0xFFFFFFFF) {
            WRITE32(base, EVR_SeqRamTS(0,j), 0xFFFFFFFF);
            WRITE8(base, EVR_SeqRamEvent(0,j), 0);

            lastTimestamp -= 0xFFFFFFFF;
            j++;
            printf("i=%zu, j=%zu\n", i, j);
        }

        WRITE32(base, EVR_SeqRamTS(0,j), lastTimestamp);
        WRITE8(base, EVR_SeqRamEvent(0,j), m_sequence.eventCode[i]);
    }

    printf("i=/, j=%zu (before adding EOS)\n", j);
    // user did not provide EOS event. Let's do it for him...
    // note, that we know that the sizes are OK, since we did validations before.
    if(!m_sequence.userProvidedEosEvent) {
        if(m_sequence.timestamp.size() > 0) {
            lastTimestamp = m_sequence.timestamp.back() + TIMESTAMP_END_OF_SEQUENCE;
            while(lastTimestamp > 0xFFFFFFFF) {
                WRITE32(base, EVR_SeqRamTS(0,j), 0xFFFFFFFF);
                WRITE8(base, EVR_SeqRamEvent(0,j), 0);

                lastTimestamp -= 0xFFFFFFFF;
                j++;
                printf("i=/, j=%zu (EOS: continuation)\n", j);
            }
            WRITE32(base, EVR_SeqRamTS(0,j), lastTimestamp);
            WRITE8(base, EVR_SeqRamEvent(0,j), EVENT_END_OF_SEQUENCE);
        }
        else {
            printf("i=/, j=%zu (EOS: empty sequence)\n", j);
            WRITE32(base, EVR_SeqRamTS(0,j), 0);
            WRITE8(base, EVR_SeqRamEvent(0,j), EVENT_END_OF_SEQUENCE);
        }
    }

    return true;
}

bool EvrSequencer::sequenceValid() const
{
    printf("Sequence valid: %d\n", m_sequence.valid);
    return m_sequence.valid;
}

bool EvrSequencer::checkSequenceSize()
{
    if(m_sequence.eventCode.size() > m_sequence.timestamp.size()) {
        m_sequence.valid = false;
        printf("Sequence not valid: there is more events than timestamps\n");
    }
    else if(m_sequence.eventCode.size() < m_sequence.timestamp.size()) {
        m_sequence.valid = false;
        printf("Sequence not valid: there is more timestamps than events\n");
    }
    else {
        m_sequence.valid = true;
    }

    if(!m_sequence.userProvidedEosEvent) {
        if(m_sequence.eventCode.size() > MAX_SEQUENCE_SIZE + 1) {
            printf("Too many events (including EOS event)\n");
            m_sequence.valid = false;
        }

        if(m_sequence.timestamp.size() > 0) {
            epicsUInt64 lastTimestamp = m_sequence.timestamp.back() + TIMESTAMP_END_OF_SEQUENCE;

            if(lastTimestamp < m_sequence.timestamp.back()) {
                printf("Timestamp overflow when adding end of sequence event\n");
                m_sequence.valid = false;
            }

            if(lastTimestamp > 0xFFFFFFFF) {
                if(m_sequence.noContinuationEvents + m_sequence.timestamp.size() + (lastTimestamp % 0xFFFFFFFF) > MAX_SEQUENCE_SIZE) {
                    printf("Adding end of sequence events results in a continuation event, which results in too many timestamps\n");
                    m_sequence.valid = false;
                }
            }
        }
    }

    if(!m_sequence.userProvidedEosEvent) {
        if(m_sequence.eventCode.size() > MAX_SEQUENCE_SIZE + 1) {
            printf("Too many events (including EOS event)\n");
            m_sequence.valid = false;
        }
    }

    scanIoRequest(m_sequence.irqValid);
    return m_sequence.valid;

    /*
    printf("Validate...\n");
    m_sequenceValid = false;
    m_eventCodeCt.clear();
    m_timestampCt.clear();

    printf("Size: %zu and %zu\n", m_eventCode.size(), m_timestamp.size());

    if(m_timestamp.size() >= MAX_SEQUENCE_SIZE || m_eventCode.size() > MAX_SEQUENCE_SIZE) {
        //TODO throw std::runtime_error("Sequence too long (>2047)");
        printf("Sequence too long!\n");
        return false;
    }

    bool sorted = true;
    bool eosEvent = false;
    bool tooBig = false;
    size_t vectorSize = 0;
    if(m_eventCode.size() > m_timestamp.size()) {
        vectorSize = m_timestamp.size();
    }
    else {
        vectorSize = m_eventCode.size();
    }
    for(size_t i = 0; i < vectorSize; i++) {
        printf("%zu: %u -> %llu\n", i, m_eventCode[i], m_timestamp[i]);

        // check if user provided end of sequence event
        if(m_eventCode[i] == EVENT_END_OF_SEQUENCE) {
            printf("EOS event detected\n");
            eosEvent = true;
            m_eventCode.erase(m_eventCode.begin()+i, m_eventCode.end());    // cut the vector where last event is
            if(vectorSize > m_eventCode.size()) vectorSize = m_eventCode.size();    // update iteration length
            break;
        }

        if(i < vectorSize-1) {
            // check if the timestamps are sorted and unique
            if (m_timestamp[i] >= m_timestamp[i+1]) {
                printf("Not sorted: %llu(%u) and %llu(%u)\n", m_timestamp[i],m_eventCode[i], m_timestamp[i+1], m_eventCode[i+1]);
                sorted = false;
                break;
            }
        }

        m_eventCodeCt.push_back(m_eventCode[i]);
        m_timestampCt.push_back(m_timestamp[i]);

        while(m_timestampCt[i] > 0xffffffff) {
            printf("%zu: vector size: %zu\n", i, vectorSize);
            if(vectorSize >= MAX_SEQUENCE_SIZE) {
                tooBig = true;
                break;
            }

            m_timestampCt.push_back(m_timestamp.begin()+i+1, m_timestamp[i] - 0xffffffff);
            m_timestampCt[i] = 0xffffffff;
            m_eventCodeCt.insert(m_eventCode.begin()+i, 0);

            vectorSize++;
            i++;
        }

        if(tooBig) {
            break;
        }

    }

    for(size_t i=0; i<m_timestamp.size(); i++) {
        printf("%zu: %u -> %llu\n", i, m_eventCode[i], m_timestamp[i]);
    }

    if(m_timestamp.size() != m_eventCode.size()) {
        printf("length of timestamp and eventCode don't match\n");
        return false;

    }

    if(!sorted) {
        printf("timestamps not sorted\n");
        return false;
    }

    // user did not provide EOS event. Let's do it for him...
    if(!eosEvent) {
        m_eventCode.push_back(EVENT_END_OF_SEQUENCE);
        if(m_timestamp.size() > 0) {
            m_timestamp.push_back(m_timestamp.back() + TIMESTAMP_END_OF_SEQUENCE);
        }
        else {
            m_timestamp.push_back(0);
        }
    }

    printf("Size: %zu and %zu\n", m_eventCode.size(), m_timestamp.size());
    printf("i: Event\ttimestamp\n");
    for(size_t i=0; i<m_timestamp.size(); i++) {
        printf("%zu: %u\t%lu\n", i, m_eventCode[i], m_timestamp[i]);
    }
    // handle continuation events

    vectorSize = m_timestamp.size();
    for(size_t i = 0; i < vectorSize; i++) {
        while(m_timestamp[i] > 0xffffffff) {
            printf("%zu: vector size: %zu\n", i, vectorSize);
            if(vectorSize >= MAX_SEQUENCE_SIZE) {
                tooBig = true;
                break;
            }

            m_timestamp.insert(m_timestamp.begin()+i+1, m_timestamp[i] - 0xffffffff);
            m_timestamp[i] = 0xffffffff;
            m_eventCode.insert(m_eventCode.begin()+i, 0);

            vectorSize++;
            i++;
        }

        if(tooBig) {
            break;
        }
    }

    if(tooBig) {
        printf("Sequence too long! (after inserting events)\n");
        return false;
    }

    printf("Size: %zu and %zu\n", m_eventCode.size(), m_timestamp.size());
    printf("i: Event\ttimestamp\n");
    for(size_t i=0; i<m_timestamp.size(); i++) {
        printf("%zu: %u\t%lu\n", i, m_eventCode[i], m_timestamp[i]);
    }

    m_sequenceValid = true;
    return true;*/
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
