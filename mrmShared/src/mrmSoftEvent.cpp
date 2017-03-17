#include <stdexcept>

#include <mrfCommonIO.h>
#include "mrmShared.h"

#include <epicsExport.h>
#include "mrmSoftEvent.h"

mrmSoftEvent::mrmSoftEvent(const std::string &name, volatile epicsUInt8 * const base):
     mrf::ObjectInst<mrmSoftEvent>(name)
    ,m_base(base)
    ,m_lock()
{

}

void mrmSoftEvent::enable(bool ena)
{
    if(ena)
         BITSET8(m_base, SwEventControl, SW_EVT_ENABLE);
    else
         BITCLR8(m_base, SwEventControl, SW_EVT_ENABLE);
}

bool
mrmSoftEvent::enabled() const
{
    return READ8(m_base, SwEventControl) & SW_EVT_ENABLE;
}

bool  mrmSoftEvent::pend() const
{
    return (READ8(m_base, SwEventControl) & SW_EVT_PEND) != 0;
}

void mrmSoftEvent::setEvtCode(epicsUInt32 evtCode)
{
    if(evtCode > 255)
        throw std::runtime_error("Event Code out of range. Valid range: 0 - 255.");

    if(!enabled())
        throw std::runtime_error("Software Event Disabled");

    int count = 0;
    while(pend() == 1) {
        count++;
        if(count == 50)
            throw std::runtime_error("Software Event Discarded.");
    }
    WRITE8(m_base, SwEventCode, evtCode);
}

epicsUInt32 mrmSoftEvent::getEvtCode() const
{
    return READ8(m_base, SwEventCode);
}


/**
 * Construct mrfioc2 objects for linking to EPICS records
 **/

OBJECT_BEGIN(mrmSoftEvent) {
    OBJECT_PROP2("Enable",  &mrmSoftEvent::enabled,    &mrmSoftEvent::enable);
    OBJECT_PROP2("EvtCode", &mrmSoftEvent::getEvtCode, &mrmSoftEvent::setEvtCode);
} OBJECT_END(mrmSoftEvent)
