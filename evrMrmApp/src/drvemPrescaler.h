/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef MRMEVRPRESCALER_H_INC
#define MRMEVRPRESCALER_H_INC

#include "mrf/object.h"
#include <epicsTypes.h>

#include "support/util.h"

class epicsShareClass MRMPreScaler : public mrf::ObjectInst<MRMPreScaler>, public IOStatus
{
public:
    MRMPreScaler(const std::string& n, volatile epicsUInt8 * b, size_t i);
    ~MRMPreScaler(){};

    /* no locking needed */
    void lock() const{};
    void unlock() const{};

    epicsUInt32 prescaler() const;
    void setPrescaler(epicsUInt32);

    epicsUInt32 pulserMapping() const;
    void setPulserMapping(epicsUInt32 pulsers);

private:
    volatile epicsUInt8* const base;
    epicsUInt32 id;
};

#endif // MRMEVRPRESCALER_H_INC
