
#include "evrGpio.h"
#include "evrMrm.h"

EvrGPIO::EvrGPIO(EVRMRM &o): owner_(o)
{
}

epicsUInt32 EvrGPIO::getDirection()
{
    epicsUInt32 val = READ32(owner_.base, GPIODir);
    return val;
}

void EvrGPIO::setDirection(epicsUInt32 val)
{
    WRITE32(owner_.base, GPIODir, val);
}

epicsUInt32 EvrGPIO::read()
{
    epicsUInt32 val = READ32(owner_.base, GPIOIn);
    return val;
}

epicsUInt32 EvrGPIO::getOutput()
{
    epicsUInt32 val = READ32(owner_.base, GPIOOut);
    return val;
}

void EvrGPIO::setOutput(epicsUInt32 val)
{
    WRITE32(owner_.base, GPIOOut, val);
}
