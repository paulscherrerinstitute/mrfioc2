#ifndef SFP_H
#define SFP_H

#include <string>
#include <vector>

#include <epicsMutex.h>
#include <epicsTypes.h>
#include "callback.h"

#include "mrf/object.h"

class epicsShareClass SFP : public mrf::ObjectInst<SFP> {
    volatile unsigned char* base;
    typedef std::vector<epicsUInt8> buffer_t;
    buffer_t buffer;
    buffer_t temp_buffer;
    mutable epicsMutex guard;
    epicsInt16 read16(unsigned int) const;
public:
    SFP(const std::string& n, volatile unsigned char* reg);
    virtual ~SFP();

    virtual void lock() const{guard.lock();};
    virtual void unlock() const{guard.unlock();};

    void updateNow(bool=true);

    double linkSpeed() const;
    double temperature() const;
    double powerTX() const;
    double powerRX() const;

    std::string vendorName() const;
    std::string vendorPart() const;
    std::string vendorRev() const;
    std::string serial() const;
    std::string manuDate() const;

    void report() const;
    void reportMore() const;

    epicsUInt16 getStatus() const;
    double getVCCPower() const;
    epicsUInt16 getBitRateUpper() const;
    epicsUInt16 getBitRateLower() const;
    epicsUInt32 getLinkLength_9um() const;
    epicsUInt16 getLinkLength_50um() const;
    epicsUInt16 getLinkLength_62um() const;
    epicsUInt16 getLinkLength_copper() const;

    /**
     * @brief updateTask updates a buffer which other functions read from.
     */
    static void updateTask(CALLBACK *pCallback);
    CALLBACK updateCallback;

    /**
     * @brief startUpdate is called from the init hook (evgInit.cpp / evrIocsh.cpp) after the callback stack has been initialized.
     */
    void startUpdate();

private:
    bool readout_valid;
};

#endif // SFP_H
