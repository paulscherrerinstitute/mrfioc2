/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef EVREvrInput_H_INC
#define EVREvrInput_H_INC

#include <cstdlib>
#include "mrf/object.h"
#include <epicsTypes.h>

#include "support/util.h"


enum TrigMode {
  TrigNone=0,
  TrigLevel=1,
  TrigEdge=2
};


/**
 * Controls only the single output mapping register
 * shared by all (except CML) outputs on MRM EVRs.
 *
 * This class is reused by other subunits which
 * have identical mapping registers.
 */
class EvrInput : public mrf::ObjectInst<EvrInput>, public IOStatus
{
public:
    EvrInput(const std::string& n, volatile epicsUInt8 *, size_t);
    virtual ~EvrInput(){};

    /* no locking needed */
    void lock() const{};
    void unlock() const{};

    //! Set mask of dbus bits are driven by this input
    void dbusSet(epicsUInt16);
    epicsUInt16 dbus() const;

    //! Set active high/low when using level trigger mode
    void levelHighSet(bool);
    bool levelHigh() const;

    //! Set active rise/fall when using edge trigger mode
    void edgeRiseSet(bool);
    bool edgeRise() const;

    //! Set external (local) event trigger mode
    void extModeSet(TrigMode);
    TrigMode extMode() const;

    //! Set the event code sent by an externel (local) event
    void extEvtSet(epicsUInt32);
    epicsUInt32 extEvt() const;

    //! Set the backwards event trigger mode
    void backModeSet(TrigMode);
    TrigMode backMode() const;

    //! Set the event code sent by an a backwards event
    void backEvtSet(epicsUInt32);
    epicsUInt32 backEvt() const;

    /**\ingroup device support helpers
     */
    /*@{*/
    void extModeSetraw(epicsUInt16 r){extModeSet((TrigMode)r);};
    epicsUInt16 extModeraw() const{return (TrigMode)extMode();};

    void backModeSetraw(epicsUInt16 r){backModeSet((TrigMode)r);};
    epicsUInt16 backModeraw() const{return (TrigMode)backMode();};
    /*@}*/

private:
    volatile epicsUInt8 * const base;
    const size_t idx;
};


#endif // EVREvrInput_H_INC
