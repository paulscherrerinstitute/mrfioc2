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

#include <epicsTypes.h>

#include "evr/prescaler.h"
#include "evr/evr.h"

class EvrPrescaler : public PreScaler
{
public:
    EvrPrescaler(const std::string& n, volatile epicsUInt8 * b, size_t i);
    ~EvrPrescaler(){};

    /* no locking needed */
    void lock() const{}
    void unlock() const{}

    epicsUInt32 prescaler() const;
    void setPrescaler(epicsUInt32);

    // Mapping a prescaler to pulser
    // Pulsers 0-15
    epicsUInt16 pulserMappingL() const;
    void setPulserMappingL(epicsUInt16 pulsers);

    // Pulsers 16-32
    epicsUInt16 pulserMappingH() const;
    void setPulserMappingH(epicsUInt16 pulsers);

private:
    volatile epicsUInt8* const base;
    size_t id;
};

#endif // EVRPRESCALER_H_INC
