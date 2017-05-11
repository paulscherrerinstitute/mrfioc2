#ifndef EVG_INPUT_H
#define EVG_INPUT_H

#include <iostream>
#include <string>
#include <map>

#include <epicsTypes.h>
#include "mrf/object.h"

#include "stdio.h"

enum InputType {
    NoneInp = 0,
    FrontInp,
    UnivInp,
    RearInp
};

class evgInput : public mrf::ObjectInst<evgInput> {
public:
    evgInput(const std::string& name, const epicsUInt32 num,
        const InputType type, volatile epicsUInt8* const pBase);
    ~evgInput();

    /* no locking. There is only database access to theese functions. */
    virtual void lock() const{}
    virtual void unlock() const{}

    epicsUInt32 getNum() const;
    InputType getType() const;

    void setExtIrq(bool);
    bool getExtIrq() const;

    void setSeqMask(epicsUInt16 mask);
    epicsUInt16 getSeqMask() const;

    void setSeqEnable(epicsUInt16 enable);
    epicsUInt16 getSeqEnable() const;

    void setDbusMap(epicsUInt16, bool);
    bool getDbusMap(epicsUInt16) const;

    void setSeqTrigMap(epicsUInt32);
    epicsUInt32 getSeqTrigMap() const;

    void setTrigEvtMap(epicsUInt16, bool);
    bool getTrigEvtMap(epicsUInt16) const;

    bool getSignalState() const;

private:
    const epicsUInt32    m_num;
    const InputType      m_type;
    volatile epicsUInt8* m_pStateReg;
    volatile epicsUInt8* m_pMapReg;
};
#endif //EVG_INPUT_H
