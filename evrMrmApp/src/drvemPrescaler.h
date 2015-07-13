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

#include <evr/prescaler.h>

class epicsShareClass MRMPreScaler : public PreScaler
{
    volatile unsigned char* base;
    epicsUInt32 id;

public:
    MRMPreScaler(const std::string& n, EVR& o,volatile unsigned char* b, epicsUInt32 i):
            PreScaler(n,o),base(b), id(i) {};
    virtual ~MRMPreScaler(){};

    /* no locking needed */
    virtual void lock() const{};
    virtual void unlock() const{};

    virtual epicsUInt32 prescaler() const;
    virtual void setPrescaler(epicsUInt32);

    virtual epicsUInt32 pulserMapping() const;
    virtual void setPulserMapping(epicsUInt32 pulsers);
};

#endif // MRMEVRPRESCALER_H_INC
