#include "evgOutput.h"

#include <iostream>
#include <stdexcept>

#include <mrfCommonIO.h> 
#include <errlog.h> 
#include <mrfCommon.h> 

#include "evgRegMap.h"

#define U16_zero 0

evgOutput::evgOutput(const std::string& name, const epicsUInt32 num,
                     const evgOutputType type, volatile epicsUInt8* const pOutReg):
mrf::ObjectInst<evgOutput>(name),
m_num(num),
m_type(type),
m_pOutReg(pOutReg) {

}

evgOutput::~evgOutput() {
}

void
evgOutput::setSource(epicsUInt16 map) {
    WRITE16(m_pOutReg, zero, map);
}

epicsUInt16
evgOutput::getSource() const {
    return READ16(m_pOutReg, zero);
}
