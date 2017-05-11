#include "evgInput.h"

#include <stdexcept>

#include <errlog.h>

#include <mrfCommonIO.h> 
#include <mrfCommon.h>
 
#include "evgRegMap.h"
std::map<std::string, epicsUInt32> InpStrToEnum;

evgInput::evgInput(const std::string& name, const epicsUInt32 num,
                   const InputType type, volatile epicsUInt8* const pBase):
mrf::ObjectInst<evgInput>(name),
m_num(num),
m_type(type),
m_pStateReg(NULL),
m_pMapReg(NULL) {

    switch(type) {
    case FrontInp:
        m_pStateReg = pBase + U32_FPInput;
        m_pMapReg = pBase + U32_FrontInMap(num);
        break;
    case UnivInp:
        m_pStateReg = pBase + U32_UnivInput;
        m_pMapReg = pBase + U32_UnivInMap(num);
        break;
    case RearInp:
        m_pStateReg = pBase + U32_TBInput;
        m_pMapReg = pBase + U32_RearInMap(num);
        break;
    default:
        throw std::runtime_error("Invalid input type");
        break;
    }

}

evgInput::~evgInput() {
}

epicsUInt32
evgInput::getNum() const {
    return m_num;
}

InputType
evgInput::getType() const {
    return m_type;
}

void
evgInput::setExtIrq(bool ena) {
    if(ena)
        nat_iowrite32(m_pMapReg, nat_ioread32(m_pMapReg) |
                                 (epicsUInt32)EVG_EXT_INP_IRQ_ENA);
    else
        nat_iowrite32(m_pMapReg, nat_ioread32(m_pMapReg) &
                                 (epicsUInt32)~(EVG_EXT_INP_IRQ_ENA));
}

bool
evgInput::getExtIrq() const {
    return  (nat_ioread32(m_pMapReg) & (epicsUInt32)EVG_EXT_INP_IRQ_ENA) != 0;
}

void
evgInput::setSeqMask(epicsUInt16 mask) {
    epicsUInt32 temp = nat_ioread32(m_pMapReg);

    mask &= 0xF;    // mask is a 4 bit value
    temp &= ~EVG_INP_SEQ_MASK;
    temp |= ((epicsUInt32)mask << EVG_INP_SEQ_MASK_shift);

    nat_iowrite32(m_pMapReg, temp);
}

epicsUInt16
evgInput::getSeqMask() const {
    return (epicsUInt16)(nat_ioread32(m_pMapReg) >> EVG_INP_SEQ_MASK_shift);
}

void
evgInput::setSeqEnable(epicsUInt16 enable) {
    epicsUInt32 en = enable;

    en <<= EVG_INP_SEQ_ENABLE_shift;
    en &= EVG_INP_SEQ_ENABLE;   // last bit should be ignored (because it belongs to external IRQ bit). Also a sanity check...

    epicsUInt32 temp = nat_ioread32(m_pMapReg);

    temp &= ~EVG_INP_SEQ_ENABLE;
    temp |= en;

    nat_iowrite32(m_pMapReg, temp);
}

epicsUInt16
evgInput::getSeqEnable() const {
    epicsUInt32 val;

    val = (nat_ioread32(m_pMapReg) & EVG_INP_SEQ_ENABLE);
    val >>= EVG_INP_SEQ_ENABLE_shift;

    return (epicsUInt16)val;
}

void
evgInput::setDbusMap(epicsUInt16 dbus, bool ena) {
    if(dbus > 7)
        throw std::runtime_error("EVG DBUS num out of range. Max: 7");

    epicsUInt32    mask = 0x10000 << dbus;

    //Read-Modify-Write
    epicsUInt32 map = nat_ioread32(m_pMapReg);

    if(ena)
        map = map | mask;
    else
        map = map & ~mask;

    nat_iowrite32(m_pMapReg, map);
}

bool
evgInput::getDbusMap(epicsUInt16 dbus) const {
    if(dbus > 7)
        throw std::runtime_error("EVG DBUS num out of range. Max: 7");

    epicsUInt32 mask = 0x10000 << dbus;
    epicsUInt32 map = nat_ioread32(m_pMapReg);
    return (map & mask) != 0;
}

void
evgInput::setSeqTrigMap(epicsUInt32 seqTrigMap) {
    if(seqTrigMap > 3)
        throw std::runtime_error("Seq Trig Map out of range. Max: 3");

    //Read-Modify-Write
    epicsUInt32 map = nat_ioread32(m_pMapReg);

    map = map & 0xffff00ff;
    map = map | (seqTrigMap << 8);

    nat_iowrite32(m_pMapReg, map);
}

epicsUInt32
evgInput::getSeqTrigMap() const {
    epicsUInt32 map = nat_ioread32(m_pMapReg);
    map = map & 0x0000ff00;
    map = map >> 8;
    return map;
}

void
evgInput::setTrigEvtMap(epicsUInt16 trigEvt, bool ena) {
    if(trigEvt > 7)
        throw std::runtime_error("Trig Event num out of range. Max: 7");

    epicsUInt32    mask = 1 << trigEvt;

    //Read-Modify-Write
    epicsUInt32 map = nat_ioread32(m_pMapReg);

    if(ena)
        map = map | mask;
    else
        map = map & ~mask;

    nat_iowrite32(m_pMapReg, map);
}

bool
evgInput::getTrigEvtMap(epicsUInt16 trigEvt) const {
    if(trigEvt > 7)
        throw std::runtime_error("EVG Trig Event num out of range. Max: 7");

    epicsUInt32 mask = 0x1 << trigEvt;
    epicsUInt32 map = nat_ioread32(m_pMapReg);
    return (map & mask) != 0;
}

bool
evgInput::getSignalState() const {
    return (nat_ioread32(m_pStateReg) & (1 << m_num));
}

