/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef EVRPULSER_H_INC
#define EVRPULSER_H_INC

#include <epicsMutex.h>
#include "mrf/object.h"
#include <epicsTypes.h>
#include <string>

#include "support/util.h"


class EVRMRM;

struct MapType {
  enum type {
    None=0,
    Trigger,
    Reset,
    Set
  };
};

/**@brief A programmable delay unit.
 *
 * A Pulser has two modes of operation: Triggered,
 * and gated.  In triggered mode an event starts a count
 * down (delay) to the start of the pulse.  A second
 * counter (width) then runs until the end of the pulse.
 * Gated mode has two event codes.  One is sets the output
 * high and the second resets the output low.
 */
class EvrPulser : public mrf::ObjectInst<EvrPulser>, public IOStatus
{
    const size_t id;
    EVRMRM& owner;

public:
    EvrPulser(const std::string& n, EVRMRM& o, size_t i);
    ~EvrPulser(){};

    void lock() const;
    void unlock() const;

    /**\defgroup ena Enable/disable pulser output.
     */
    /*@{*/
    bool enabled() const;
    void enable(bool);
    /*@}*/

    /**\defgroup dly Set triggered mode delay length.
     *
     * Units of event clock period.
     */
    /*@{*/
    void setDelayRaw(epicsUInt32);
    void setDelay(double);
    epicsUInt32 delayRaw() const;
    double delay() const;
    /*@}*/

    /**\defgroup wth Set triggered mode width
     *
     * Units of event clock period.
     */
    /*@{*/
    void setWidthRaw(epicsUInt32);
    void setWidth(double);
    epicsUInt32 widthRaw() const;
    double width() const;
    /*@}*/

    /**\defgroup scaler Set triggered mode prescaler
     */
    /*@{*/
    epicsUInt32 prescaler() const;
    void setPrescaler(epicsUInt32);
    /*@}*/

    /**\defgroup pol Set output polarity
     *
     * Selects normal or inverted.
     */
    /*@{*/
    bool polarityInvert() const;
    void setPolarityInvert(bool);
    /*@}*/

    /**\defgroup map Control which source(s) effect this pulser.
     *
     * Meaning of source id number is device specific.
     *
     * Note: this is one place where Device Support will have some depth.
     */
    /*@{*/
     //! What action is source 'src' mapped to?
    MapType::type mappedSource(epicsUInt32 src) const;
    //! Set mapping of source 'src'.
    void sourceSetMap(epicsUInt32 src,MapType::type action);
    /*@}*/

    /**\defgroup gate Pulse generator gates.
     *
     * Settings for mask and enable gates
     */
    /*@{*/
    epicsUInt16 gateMask() const;
    void setGateMask(epicsUInt16 mask);

    epicsUInt16 gateEnable() const;
    void setGateEnable(epicsUInt16 mask);
    /*@}*/

    /**\defgroup swSetReset Software set / reset for the pulser
     *
     * When 'set' is true the pulser will be set, when false it will be reset.
     */
    /*@{*/
    bool dummyReturn() const{return false;}
    void swSetReset(bool set);
    /*@}*/


    /**\defgroup out Get current pulser output
     *
     * Output high or low.
     */
    /*@{*/
    bool getOutput() const;
    /*@}*/
private:
    // bit map of which event #'s are mapped
    // used as a safty check to avoid overloaded mappings
    unsigned char mapped[256/8];

    void _map(epicsUInt8 evt)   {        mapped[evt/8] |=    1<<(evt%8);  }
    void _unmap(epicsUInt8 evt) {        mapped[evt/8] &= ~( 1<<(evt%8) );}
    bool _ismap(epicsUInt8 evt) const { return (mapped[evt/8]  &    1<<(evt%8)) != 0;  }
};

#endif // EVRPULSER_H_INC
