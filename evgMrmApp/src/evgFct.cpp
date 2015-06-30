#include "evgFct.h"

#include <mrfCommonIO.h>
#include <mrfCommon.h>

#include "evgRegMap.h"


evgFct::evgFct(const std::string& evgName, volatile epicsUInt8* const fctReg, std::vector<SFP *> &sfp):
mrf::ObjectInst<evgFct>(evgName+":FCT"),
m_fctReg(fctReg),
m_sfp(sfp)
{
    for(int i = 1; i <= evgNumSFPModules; i++) {
        std::ostringstream name;
        name<<evgName<<":SFP"<<i;
        m_sfp.push_back(new SFP(name.str(), fctReg + U32_SFP(i-1)));
    }
}

evgFct::~evgFct() {
}


epicsUInt32
evgFct::getUpstreamDC() const{
    return READ32(m_fctReg, fct_upstreamDC);
}

epicsUInt32
evgFct::getReceiveDC() const{
    return READ32(m_fctReg, fct_receiveDC);
}

epicsUInt32
evgFct::getInternalDC() const{
    return READ32(m_fctReg, fct_internalDC);
}

epicsUInt16
evgFct::getPortStatus() const{
    epicsUInt32 status;

    status = READ32(m_fctReg, fct_status_base);
    status &= EVG_FCT_STATUS_STATUS_mask;
    status = status >> EVG_FCT_STATUS_STATUS_shift;

    return  (epicsUInt16)status;
}

epicsUInt16
evgFct::getPortViolation() const{
    epicsUInt32 violation;

    violation = READ32(m_fctReg, fct_status_base);
    violation &= EVG_FCT_STATUS_VIOLATION_mask;
    violation = violation >> EVG_FCT_STATUS_VIOLATION_shift;

    return  (epicsUInt16)violation;
}

void
evgFct::clearPortViolation(epicsUInt16 port){
    epicsUInt32 ctrlReg;

    if(port > EVG_FCT_maxPorts || port < 1){    // port 0 == upstream. Fanouts are ports 1+
        throw std::out_of_range("Selected fanout port does not exist.");
    }

    ctrlReg = READ32(m_fctReg, fct_control_base);
    ctrlReg |= EVG_FCT_CONTROL_VIOLATION_start << (port-1);  // clear violation by writing '1' to the clear violation port register
    WRITE32(m_fctReg, fct_control_base, ctrlReg);
}

epicsUInt32
evgFct::getPortDelayValue(epicsUInt16 port)  const{
    if(port > EVG_FCT_maxPorts || port < 1){    // port 0 == upstream. Fanouts are ports 1+
        throw std::out_of_range("Selected fanout port does not exist.");
    }

    return READ32(m_fctReg, fct_portDC(port-1));
}

/*
bool evgFct::getPortStatus()
{
    epicsUInt32 status;

    if(m_id > EVG_FCT_maxPorts || m_id < 1){    // port 0 == upstream. Fanouts are ports 1+
        std::out_of_range("Selected fanout port does not exist.");
    }
    status = READ32(m_fctReg, U32_fct_status_base);
    status &= EVG_FCT_STATUS_STATUS_start << (m_id-1);

    return  status != 0;
}

bool evgFct::getPortViolation()
{
    epicsUInt32 violation;

    if(m_id > EVG_FCT_maxPorts || m_id < 1){    // port 0 == upstream. Fanouts are ports 1+
        std::out_of_range("Selected fanout port does not exist.");
    }
    violation = READ32(m_fctReg, U32_fct_status_base);
    violation &= EVG_FCT_STATUS_VIOLATION_start << (m_id-1);

    return  violation != 0;
}

void evgFct::clearPortViolation()
{
    epicsUInt32 ctrlReg;

    if(m_id > EVG_FCT_maxPorts || m_id < 1){    // port 0 == upstream. Fanouts are ports 1+
        std::out_of_range("Selected fanout port does not exist.");
    }

    ctrlReg = READ32(m_fctReg, U32_fct_control_base);
    ctrlReg |= EVG_FCT_CONTROL_VIOLATION_start << (m_id-1);  // clear violation by writing '1' to the clear violation port register

    WRITE32(m_fctReg, U32_fct_control_base, ctrlReg);
}

epicsUInt32 evgFct::getPortDelayValue()
{
    if(m_id > EVG_FCT_maxPorts || m_id < 1){    // port 0 == upstream. Fanouts are ports 1+
        std::out_of_range("Selected fanout port does not exist.");
    }

    return READ32(m_fctReg, U32_fct_portDC(m_id-1));
}*/
