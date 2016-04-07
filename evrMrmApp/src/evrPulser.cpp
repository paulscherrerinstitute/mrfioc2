/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include "evrRegMap.h"
#include "evrMrm.h"

#include <stdexcept>
#include <cstring>

#include <errlog.h>
#include <dbDefs.h>
#include <epicsMath.h>

#include "mrfCommonIO.h"
#include "mrfBitOps.h"



#include "evrPulser.h"

EvrPulser::EvrPulser(const std::string& n, EVRMRM& o, size_t i)
  :mrf::ObjectInst<EvrPulser>(n)
  ,id(i)
  ,owner(o)
{
    if(id>31)
        throw std::out_of_range("pulser id is out of range");

    std::memset(&this->mapped, 0, NELEMENTS(this->mapped));
}

void EvrPulser::lock() const{owner.lock();};
void EvrPulser::unlock() const{owner.unlock();};

bool
EvrPulser::enabled() const
{
    return READ32(owner.base, PulserCtrl(id)) & PulserCtrl_ena;
}

void
EvrPulser::enable(bool s)
{
    if(s)
        BITSET(NAT,32,owner.base, PulserCtrl(id),
             PulserCtrl_ena|PulserCtrl_mtrg|PulserCtrl_mset|PulserCtrl_mrst);
    else
        BITCLR(NAT,32,owner.base, PulserCtrl(id),
             PulserCtrl_ena|PulserCtrl_mtrg|PulserCtrl_mset|PulserCtrl_mrst);
}

void
EvrPulser::setDelayRaw(epicsUInt32 v)
{
    epicsUInt64 val = (epicsUInt64)v;   // convert double to something that can hold value this big
    val &= 0xFFFFFFFF;  // make sure it's a 32-bit unsigned value

    WRITE32(owner.base, PulserDely(id), v);
}

void
EvrPulser::setDelay(double v)
{
    double scal=double(prescaler());
    if (scal<=0) scal=1;
    double clk=owner.clock(); // in MHz.  MTicks/second

    epicsUInt32 ticks=roundToUInt((v*clk)/scal);

    setDelayRaw(ticks);
}

epicsUInt32
EvrPulser::delayRaw() const
{
    return READ32(owner.base,PulserDely(id));
}

double
EvrPulser::delay() const
{
    double scal=double(prescaler());
    double ticks=double(delayRaw());
    double clk=owner.clock(); // in MHz.  MTicks/second
    if (scal<=0) scal=1;

    return (ticks*scal)/clk;
}

void
EvrPulser::setWidthRaw(epicsUInt32 v)
{
    epicsUInt64 val = (epicsUInt64)v;   // convert double to something that can hold value this big
    val &= 0xFFFFFFFF;  // make sure it's a 32-bit unsigned value

    WRITE32(owner.base, PulserWdth(id), v);
}

void
EvrPulser::setWidth(double v)
{
    double scal=double(prescaler());
    double clk=owner.clock(); // in MHz.  MTicks/second
    if (scal<=0) scal=1;

    epicsUInt32 ticks=roundToUInt((v*clk)/scal);

    setWidthRaw(ticks);
}

epicsUInt32 EvrPulser::widthRaw() const
{
    return READ32(owner.base,PulserWdth(id));
}

double
EvrPulser::width() const
{
    double scal=double(prescaler());
    double ticks=double(widthRaw());
    double clk=owner.clock(); // in MHz.  MTicks/second
    if (scal<=0) scal=1;

    return (ticks*scal)/clk;
}

epicsUInt32
EvrPulser::prescaler() const
{
    return READ32(owner.base,PulserScal(id));
}

void
EvrPulser::setPrescaler(epicsUInt32 v)
{
    WRITE32(owner.base, PulserScal(id), v);
}

bool
EvrPulser::polarityInvert() const
{
    return (READ32(owner.base, PulserCtrl(id)) & PulserCtrl_pol) != 0;
}

void
EvrPulser::setPolarityInvert(bool s)
{
    if(s)
        BITSET(NAT,32,owner.base, PulserCtrl(id), PulserCtrl_pol);
    else
        BITCLR(NAT,32,owner.base, PulserCtrl(id), PulserCtrl_pol);
}

MapType::type
EvrPulser::mappedSource(epicsUInt32 evt) const
{
    if(evt>255)
        throw std::out_of_range("Event code is out of range");

    if(evt==0)
        return MapType::None;

    epicsUInt32 map[3];

    map[0]=READ32(owner.base, MappingRam(0,evt,Trigger));
    map[1]=READ32(owner.base, MappingRam(0,evt,Set));
    map[2]=READ32(owner.base, MappingRam(0,evt,Reset));

    epicsUInt32 pmask=1<<id, insanity=0;

    MapType::type ret=MapType::None;

    if(map[0]&pmask){
        ret=MapType::Trigger;
        insanity++;
    }
    if(map[1]&pmask){
        ret=MapType::Set;
        insanity++;
    }
    if(map[2]&pmask){
        ret=MapType::Reset;
        insanity++;
    }
    if(insanity>1){
        errlogPrintf("EVR %s pulser #%zd code %02x maps too many actions %08x %08x %08x\n",
            owner.id.c_str(),id,evt,map[0],map[1],map[2]);
    }

    if( (ret==MapType::None) ^ _ismap(evt) ){
        errlogPrintf("EVR %s pulser #%zd code %02x mapping (%08x %08x %08x) is out of sync with view (%d)\n",
            owner.id.c_str(),id,evt,map[0],map[1],map[2],_ismap(evt));
    }

    return ret;
}

void
EvrPulser::sourceSetMap(epicsUInt32 evt,MapType::type action)
{
    if(evt>255)
        throw std::out_of_range("Event code is out of range");

    if(evt==0)
        return;

    epicsUInt32 pmask=1<<id;

    if( (action!=MapType::None) && _ismap(evt) )
        throw std::runtime_error("Ignore request for duplicate mapping");

    if(action!=MapType::None)
        _map(evt);
    else
        _unmap(evt);

    if(action==MapType::Trigger)
        BITSET(NAT,32, owner.base, MappingRam(0,evt,Trigger), pmask);
    else
        BITCLR(NAT,32, owner.base, MappingRam(0,evt,Trigger), pmask);

    if(action==MapType::Set)
        BITSET(NAT,32, owner.base, MappingRam(0,evt,Set), pmask);
    else
        BITCLR(NAT,32, owner.base, MappingRam(0,evt,Set), pmask);

    if(action==MapType::Reset)
        BITSET(NAT,32, owner.base, MappingRam(0,evt,Reset), pmask);
    else
        BITCLR(NAT,32, owner.base, MappingRam(0,evt,Reset), pmask);
}

epicsUInt16
EvrPulser::gateMask() const{
    epicsUInt32 mask;

    mask = READ32(owner.base, PulserCtrl(id)) & PulserCtrl_gateMask;
    mask = mask >> PulserCtrl_gateMask_shift;
    mask = mask >> 4;   // pulser gate 0 is mapped to the forth bit, gate1 -> bit 5, ....

    return (epicsUInt16)mask;
}

void
EvrPulser::setGateMask(epicsUInt16 mask){
    epicsUInt32 pulserCtrl;

    // TODO check if out of range
    mask = mask << 4;   // pulser gate 0 is mapped to the forth bit, gate1 -> bit 5, ....

    pulserCtrl = READ32(owner.base, PulserCtrl(id));
    pulserCtrl = pulserCtrl & ~PulserCtrl_gateMask;
    pulserCtrl |= ((epicsUInt32)mask << PulserCtrl_gateMask_shift);
    WRITE32(owner.base, PulserCtrl(id), pulserCtrl);
}

epicsUInt16
EvrPulser::gateEnable() const{
    epicsUInt32 gate;

    gate = READ32(owner.base, PulserCtrl(id)) & PulserCtrl_gateEnable;
    gate = gate >> PulserCtrl_gateEnable_shift;
    gate = gate >> 4;   // pulser gate 0 is mapped to the forth bit, gate1 -> bit 5, ....

    return (epicsUInt16)gate;
}

void
EvrPulser::setGateEnable(epicsUInt16 gate){
    epicsUInt32 pulserCtrl;

    // TODO check if out of range

    gate = gate << 4;   // pulser gate 0 is mapped to the forth bit, gate1 -> bit 5, ....

    pulserCtrl = READ32(owner.base, PulserCtrl(id));
    pulserCtrl = pulserCtrl & ~PulserCtrl_gateEnable;
    pulserCtrl |= ((epicsUInt32)gate << PulserCtrl_gateEnable_shift);
    WRITE32(owner.base, PulserCtrl(id), pulserCtrl);
}


void EvrPulser::swSetReset(bool set)
{
    epicsUInt32 pulserCtrl;

    pulserCtrl = READ32(owner.base, PulserCtrl(id));

    if(set) {
        pulserCtrl |= PulserCtrl_sset;
    }
    else {
        pulserCtrl |= PulserCtrl_srst;
    }

    WRITE32(owner.base, PulserCtrl(id), pulserCtrl);
}


bool
EvrPulser::getOutput() const {
    epicsUInt32 pulserCtrl;

    pulserCtrl = READ32(owner.base, PulserCtrl(id));

    return (bool)(pulserCtrl & PulserCtrl_rbv);
}
