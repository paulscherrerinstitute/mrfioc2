/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <mrfCommonIO.h>
#include "evrRegMap.h"

#include <stdexcept>
#include "evrPrescaler.h"

#define BIT_MASK_16 0x0000FFFF
#define BIT_MASK_16_shift 16

EvrPrescaler::EvrPrescaler(const std::string& n, volatile epicsUInt8 * b, size_t i)
    :mrf::ObjectInst<EvrPrescaler>(n)
    ,base(b)
    ,id(i)
{

}

epicsUInt32 EvrPrescaler::prescaler() const
{
    return READ32(base, Scaler(id));
}

void
EvrPrescaler::setPrescaler(epicsUInt32 v)
{
    WRITE32(base, Scaler(id), v);
}


epicsUInt16
EvrPrescaler::pulserMappingL() const{
    return READ32(base, PrescalerTrigger(id)) & BIT_MASK_16;
}

void
EvrPrescaler::setPulserMappingL(epicsUInt16 pulsers){
    //TODO check out of range
    epicsUInt32 reg = READ32(base, PrescalerTrigger(id));
    reg &= ~BIT_MASK_16;
    reg |= pulsers;

    WRITE32(base, PrescalerTrigger(id), reg);
}

epicsUInt16
EvrPrescaler::pulserMappingH() const{
    return READ32(base, PrescalerTrigger(id)) >> BIT_MASK_16_shift;
}

void
EvrPrescaler::setPulserMappingH(epicsUInt16 pulsers){
    //TODO check out of range
    epicsUInt32 reg = READ32(base, PrescalerTrigger(id));
    reg &= BIT_MASK_16;
    reg |= ((epicsUInt32)pulsers) << BIT_MASK_16_shift;

    WRITE32(base, PrescalerTrigger(id), reg);
}
