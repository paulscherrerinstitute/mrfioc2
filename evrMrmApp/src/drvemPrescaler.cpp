/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */


#include "mrf/databuf.h"
//#include <epicsMMIO.h>
#include <mrfCommonIO.h>
#include "evrRegMap.h"

#include <stdexcept>
#include <evr/prescaler.h>

#include <epicsExport.h>
#include "drvemPrescaler.h"


epicsUInt32
MRMPreScaler::prescaler() const
{
    return nat_ioread32(base+U32_Scaler(id));  //FIXME must be READ32 instead of nat_ioread32
}

void
MRMPreScaler::setPrescaler(epicsUInt32 v)
{
    if(v>0xffff)
        throw std::out_of_range("prescaler setting is out of range");

    nat_iowrite32(base+U32_Scaler(id), v); // FIXME must be WRITE32 instead of nat_iowrite32
}


epicsUInt32
MRMPreScaler::pulserMapping() const{
    return READ32(base, PrescalerTrigger(id));
}

void
MRMPreScaler::setPulserMapping(epicsUInt32 pulsers){
    //TODO check out of range
    return WRITE32(base, PrescalerTrigger(id), pulsers);
}
