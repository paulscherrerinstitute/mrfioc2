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


EvrPrescaler::EvrPrescaler(const std::string& n, volatile epicsUInt8 * b, size_t i)
    :mrf::ObjectInst<EvrPrescaler>(n)
    ,base(b)
    ,id(i)
{

}

epicsUInt32
EvrPrescaler::prescaler() const
{
    return READ32(base, Scaler(id));
}

void
EvrPrescaler::setPrescaler(epicsUInt32 v)
{
    WRITE32(base, Scaler(id), v);
}


epicsUInt32
EvrPrescaler::pulserMapping() const{
    return READ32(base, PrescalerTrigger(id));
}

void
EvrPrescaler::setPulserMapping(epicsUInt32 pulsers){
    //TODO check out of range
    return WRITE32(base, PrescalerTrigger(id), pulsers);
}
