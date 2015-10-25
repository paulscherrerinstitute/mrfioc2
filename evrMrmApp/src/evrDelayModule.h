#ifndef EVRDELAYMODULE_H_INC
#define EVRDELAYMODULE_H_INC

#include "mrf/object.h"

#include <epicsGuard.h>
#include <epicsTypes.h>

#include "evrGpio.h"

class EVRMRM;

class EvrDelayModule : public mrf::ObjectInst<EvrDelayModule>
{
public:
    EvrDelayModule(const std::string&, EVRMRM*, size_t);
    virtual ~EvrDelayModule();

    /**
     * @brief setDelay0 Sets the delay of the output 0 in the module
     * @param val Delay in range of 2.2ns - 12.43ns. If the value is greater it will be set to maximum range value, if it is smaller it will be set to minimum range value.
     */
    void setDelay0(double val);
    /**
     * @brief getDelay0 Returns the last set delay for the output 0 in the module
     * @return The delay in [ns]
     */
    double getDelay0() const;

    /**
     * @brief setDelay1 Sets the delay of the output 1 in the module
     * @param val Delay in range of 2.2ns - 12.43ns. If the value is greater it will be set to maximum range value, if it is smaller it will be set to minimum range value.
     */
    void setDelay1(double val);
    /**
     * @brief getDelay1R eturns the last set delay for the output 1 in the module
     * @return  The delay in [ns]
     */
    double getDelay1() const;

    /**
     * @brief setState Sets the enabled state of the delay module. If disabled, the module will output logic low on both ouputs.
     * @param enabled True for enabled and false for disabled
     */
    void setState(bool enabled);
    /**
     * @brief enabled Checks if the module is enabled or not.
     * @return True if the module is enabled, false othwerwise.
     */
    bool enabled() const;

    //void set(bool a, bool b, epicsUInt16 c, epicsUInt16 d){setDelay(a,b,c,d);}

private:
    const size_t N_;
    EvrGPIO *gpio_;
    epicsUInt16 dly0_, dly1_;

    /**
     * @brief setGpioOutput Sets the EVR GPIO pins for this delay module to output mode
     */
    void setGpioOutput();

    /**
     * @brief enable Sets the output of the module to delayed input
     */
    void enable();

    /**
     * @brief disable Sets the output of the module to logic low
     */
    void disable();

    /**
     * @brief setDelay Sets delay value of the module.
     * @param output0 Enable output 0 of the module
     * @param output1 Enable output 1 of the module
     * @param value0 Delay value for output 0
     * @param value1 Delay value for output 1
     */
    void setDelay(bool output0, bool output1, epicsUInt16 value0, epicsUInt16 value1);

    /**
     * @brief pushData Writes the data to the module bit by bit
     * @param data The data to be written
     */
    void pushData(epicsUInt32 data);

    // There is no locking needed, but methods must be present since there are virtual in mrf::Object class
    virtual void lock() const {}
    virtual void unlock() const {}
};

#endif // EVRDELAYMODULE_H_INC
