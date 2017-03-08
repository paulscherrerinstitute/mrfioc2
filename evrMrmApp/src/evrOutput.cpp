/*************************************************************************\
* Copyright (c) 2013 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */


//#include <epicsMMIO.h>
#include "mrmDeviceInfo.h"
#include "evrRegMap.h"
#include "evrMrm.h"

#include <mrfCommon.h>
#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#include <stdexcept>
#include "evrOutput.h"


EvrOutput::EvrOutput(const std::string& n, EVRMRM* o, OutputType t, size_t idx)
    :mrf::ObjectInst<EvrOutput>(n)
    ,owner(o)
    ,type(t)
    ,N(idx)
    ,isEnabled(true)
    ,deviceInfo(o->getDeviceInfo())
{
    shadowSource = sourceInternal();
    shadowSource2 = 63; //force low, for backwards compatiblity
}

EvrOutput::~EvrOutput()
{
}

void EvrOutput::lock() const{owner->lock();};
void EvrOutput::unlock() const{owner->unlock();};

epicsUInt32
EvrOutput::source() const
{
    return shadowSource;
}

void
EvrOutput::setSource(epicsUInt32 v)
{
    if(!isSourceValid(v)) {
        throw std::out_of_range("Mapping code is out of range");
    }

    shadowSource = v;

    if(isEnabled)
        setSourceInternal(shadowSource,shadowSource2);
}

epicsUInt32 EvrOutput::source2() const
{
    return shadowSource2;
}

void EvrOutput::setSource2(epicsUInt32 v)
{
    if(!isSourceValid(v)) {
        throw std::out_of_range("Mapping code is out of range");
    }

    if(deviceInfo->getFirmwareId() != mrmDeviceInfo::firmwareId_delayCompensation) {
        //throw std::out_of_range("Hardware does not support second output source selection");
        return;
    }

    shadowSource2 = v;

    if(isEnabled)
        setSourceInternal(shadowSource,shadowSource2);
}

bool
EvrOutput::enabled() const
{
    return isEnabled;
}

void
EvrOutput::enable(bool e)
{
    if(e==isEnabled)
        return;

    isEnabled = e;

    if(isEnabled)
        setSourceInternal(shadowSource,shadowSource2);
    else
        setSourceInternal(63,63); // Force Off
}

epicsUInt32
EvrOutput::sourceInternal() const
{
    epicsUInt32 val=64; // an invalid value
    switch(type) {
    case OutputInt:
        return  READ32(owner->base, IRQPulseMap) & 0xffff;
    case OutputFP:
        val = READ32(owner->base, OutputMapFP(N)); break;
    case OutputFPUniv:
        val = READ32(owner->base, OutputMapFPUniv(N)); break;
    case OutputRB:
        val = READ32(owner->base, OutputMapRB(N)); break;
    }
    val &= Output_mask(N);
    val >>= Output_shift(N);
    return val & 0xff;
}

epicsUInt32
EvrOutput::sourceInternal2() const
{
    epicsUInt32 val=64; // an invalid value
    switch(type) {
    case OutputInt:
        return  READ32(owner->base, IRQPulseMap) & 0xffff;
    case OutputFP:
        val = READ32(owner->base, OutputMapFP(N)); break;
    case OutputFPUniv:
        val = READ32(owner->base, OutputMapFPUniv(N)); break;
    case OutputRB:
        val = READ32(owner->base, OutputMapRB(N)); break;
    }
    val &= Output_mask(N);
    val >>= Output_shift(N);
    return val>>8;
}

void
EvrOutput::setSourceInternal(epicsUInt32 v, epicsUInt32 v1)
{

    v |= (v1<<8);
    epicsUInt32 val=63;
    switch(type) {
    case OutputInt:
        WRITE32(owner->base, IRQPulseMap, v); return;
    case OutputFP:
        val = READ32(owner->base, OutputMapFP(N)); break;
    case OutputFPUniv:
        val = READ32(owner->base, OutputMapFPUniv(N)); break;
    case OutputRB:
        val = READ32(owner->base, OutputMapRB(N)); break;
    }

    val &= ~Output_mask(N);
    val |= v << Output_shift(N);

    switch(type) {
    case OutputInt:
        break; // will not get here
    case OutputFP:
        WRITE32(owner->base, OutputMapFP(N), val); break;
    case OutputFPUniv:
        WRITE32(owner->base, OutputMapFPUniv(N), val); break;
    case OutputRB:
        WRITE32(owner->base, OutputMapRB(N), val); break;
    }
}

bool EvrOutput::isSourceValid(epicsUInt32 source) const
{

    if( ( (source<=63 && source>=62) ||
          (source<=47 && source>=32) ||
          (source<=31) )        ||
        (deviceInfo->getFormFactor() == mrmDeviceInfo::formFactor_PCIe && deviceInfo->getFirmwareId() == mrmDeviceInfo::firmwareId_delayCompensation && source == 61)
    ) {
        return true;
    }
    else {
        return false;
    }
}

const char*
EvrOutput::sourceName(epicsUInt32 id) const
{
    switch(id){
    case 63: return "Force Low";
    case 62: return "Force High";
    case 61: return "Tri-state output";   // only on PCIe-EVR
    case 60: return "Event clock output with 180 phase shift";   // only on PXIe-EVR-300
    case 59: return "Event clock output";   // only on PXIe-EVR-300
    // 48 -> 58 Reserved
    case 47: return "Prescaler (Divider) 7";
    case 46: return "Prescaler (Divider) 6";
    case 45: return "Prescaler (Divider) 5";
    case 44: return "Prescaler (Divider) 4";
    case 43: return "Prescaler (Divider) 3";
    case 42: return "Prescaler (Divider) 2";
    case 41: return "Prescaler (Divider) 1";
    case 40: return "Prescaler (Divider) 0";
    case 39: return "Distributed Bus Bit 7";
    case 38: return "Distributed Bus Bit 6";
    case 37: return "Distributed Bus Bit 5";
    case 36: return "Distributed Bus Bit 4";
    case 35: return "Distributed Bus Bit 3";
    case 34: return "Distributed Bus Bit 2";
    case 33: return "Distributed Bus Bit 1";
    case 32: return "Distributed Bus Bit 0";
    // 24 -> 31 Reserved
    case 23: return "Pulse generator 23";
    case 22: return "Pulse generator 22";
    case 21: return "Pulse generator 21";
    case 20: return "Pulse generator 20";
    case 19: return "Pulse generator 19";
    case 18: return "Pulse generator 18";
    case 17: return "Pulse generator 17";
    case 16: return "Pulse generator 16";
    case 15: return "Pulse generator 15";
    case 14: return "Pulse generator 14";
    case 13: return "Pulse generator 13";
    case 12: return "Pulse generator 12";
    case 11: return "Pulse generator 11";
    case 10: return "Pulse generator 10";
    case 9: return "Pulse generator 9";
    case 8: return "Pulse generator 8";
    case 7: return "Pulse generator 7";
    case 6: return "Pulse generator 6";
    case 5: return "Pulse generator 5";
    case 4: return "Pulse generator 4";
    case 3: return "Pulse generator 3";
    case 2: return "Pulse generator 2";
    case 1: return "Pulse generator 1";
    case 0: return "Pulse generator 0";
    default: return "Invalid output source";
    }
}
