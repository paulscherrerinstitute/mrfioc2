/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef EVRPRESCALER_H_INC
#define EVRPRESCALER_H_INC

#include "mrf/object.h"
#include <epicsTypes.h>

#include "support/util.h"

class EvrPrescaler : public mrf::ObjectInst<EvrPrescaler>, public IOStatus
{
public:
    EvrPrescaler(const std::string& n, volatile epicsUInt8 * b, size_t i);
    ~EvrPrescaler(){};

    /* no locking needed */
    void lock() const{};
    void unlock() const{};

    epicsUInt32 prescaler() const;
    void setPrescaler(epicsUInt32);

    epicsUInt32 pulserMapping() const;
    void setPulserMapping(epicsUInt32 pulsers);

private:
    volatile epicsUInt8* const base;
    size_t id;
};

#endif // EVRPRESCALER_H_INC
