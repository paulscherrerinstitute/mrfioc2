/*************************************************************************\
* Copyright (c) 2013 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef EVROUTPUT_H_INC
#define EVROUTPUT_H_INC

#include "mrf/object.h"
#include <epicsTypes.h>

#include "evr/util.h"
#include "mrmShared.h"

class EVRMRM;
class mrmDeviceInfo;

enum OutputType {
  OutputInt=0, //!< Internal
  OutputFP=1,  //!< Front Panel
  OutputFPUniv=2, //!< FP Universal
  OutputRB=3 //!< Rear Breakout
};


/**
 * Controls only the single output mapping register
 * shared by all (except CML) outputs on MRM EVRs.
 *
 * This class is reused by other subunits which
 * have identical mapping registers.
 */
class EvrOutput : public mrf::ObjectInst<EvrOutput>, public IOStatus
{
public:
  EvrOutput(const std::string& n, EVRMRM* owner, OutputType t, size_t idx);
  ~EvrOutput();

  void lock() const;
  void unlock() const;

  /**\defgroup src Control which source(s) effect this output.
   *
   * Meaning of source id number is device specific.
   *
   * When enabled()==true then the user mapping provided to setSource()
   * is used.  When enabled()==false then a device specific special mapping
   * is used (eg. Force Low).
   *
   * Note: this is one place where Device Support will have some depth.
   */
  /*@{*/
  epicsUInt32 source() const;
  void setSource(epicsUInt32);

  epicsUInt32 source2() const;
  void setSource2(epicsUInt32);

  bool enabled() const;
  void enable(bool);
  /*@}*/

  const char*sourceName(epicsUInt32) const;

private:
  EVRMRM * const owner;
  const OutputType type;
  const size_t N;
  bool isEnabled;
  epicsUInt32 shadowSource;
  epicsUInt32 shadowSource2;
  mrmDeviceInfo *deviceInfo;


  virtual epicsUInt32 sourceInternal() const;
  virtual epicsUInt32 sourceInternal2() const;
  virtual void setSourceInternal(epicsUInt32 v, epicsUInt32 v1);
  virtual bool isSourceValid(epicsUInt32 source) const;
};


#endif // EVROUTPUT_H_INC
