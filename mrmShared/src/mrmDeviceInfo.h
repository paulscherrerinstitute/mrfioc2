#ifndef MRMDEVICEINFO_H
#define MRMDEVICEINFO_H

#include <string>
#include <epicsTypes.h>
#include <shareLib.h>

class epicsShareClass mrmDeviceInfo
{
public:
    mrmDeviceInfo(volatile epicsUInt8 *parentBaseAddress);

    //VME
    typedef struct configuration_vme{
        epicsInt32 slot;        // slot where the card is inserted
        epicsUInt32 address;    // VME address in A24 space
        epicsInt32 irqLevel;    // interupt level
        epicsInt32 irqVector;   // interrupt vector
        std::string position;   // position description for EVR
    } busConfigurationVmeT;


    // PCI
    typedef struct configuration_pci{
        int bus;        // Bus number
        int device;     // Device number
        int function;   // Function number
    } busConfigurationPciT;

    enum busType{
        busType_vme = 0,
        busType_pci = 1
    };

    typedef struct busConfiguration{
        struct configuration_vme vme;
        struct configuration_pci pci;
        enum busType busType;
    } busConfigurationT;

    // form factor corresponds to FPGA Firmware Version Register bit 27-24
    typedef enum formFactor {
      formFactor_unknown = -1,
      formFactor_CPCI=0, // 3U
      formFactor_PMC=1,
      formFactor_VME64=2,
      formFactor_CRIO=3,
      formFactor_CPCIFULL=4, // 6U
      formFactor_PXIe=6,
      formFactor_PCIe=7,
      formFactor_mTCAv4=8,
      formFactor_embedded=9,    // this form factor is internal only. The rest are read from the firmware register
      formFactor_last = formFactor_embedded
    } formFactorT;

    typedef enum firmwareId {
        firmwareId_unknown = -1,
        firmwareId_mrm = 0,                 // Modular Register Map firmware (no delay compensation)
        firmwareId_delayCompensation = 2,   // Delay Compensation firmware
    } firmwareIdT;

    typedef enum deviceType {
        deviceType_unknown = -1,
        deviceType_receiver = 1,
        deviceType_generator = 2
    } deviceTypeT;

    typedef enum result {
        result_OK = 0,                  // no error
        result_deviceTypeError,         // device type is not what user thought it was
        result_firmwareRegisterError,   // firmware register content is not parsable
        result_firmwareVersionError     // mrfioc2 does not support this firmware version
    } resultT;

    void setEmbeddedFormFactor();
    formFactorT getFormFactor() const {return m_formFactor; }
    std::string getDeviceDescription();
    std::string getDeviceTypeStr();
    deviceTypeT getDeviceType() {return m_deviceType; }
    epicsUInt8  getRevisionId() {return m_revisionId; }
    firmwareIdT getFirmwareId() { return m_firmwareId; }
    epicsUInt32 getFirmwareVersion() const {return m_firmwareVersion; }
    epicsUInt32 getMinSupportedFwVersion();
    epicsUInt32 getFirmwareRegister() {return m_firmwareRegister; }
    resultT isDeviceSupported(deviceTypeT deviceType);

    void setBusConfigurationVme(configuration_vme configuration);
    void setBusConfigurationPci(configuration_pci configuration);
    busConfigurationT getBusConfiguration() const {return m_busConfiguration; }

private:
    void init();
    epicsUInt8 getMinSupportedFwRevision();
    epicsUInt8 getMinSupportedFwSubrelease();
    epicsUInt32 constructFwVersion(epicsUInt8 subreleaseId, epicsUInt8 firmwareId, epicsUInt8 revisionId);

    volatile epicsUInt8 * const m_base; // Base address of the EVR/EVG card
    epicsUInt32                 m_firmwareRegister;
    deviceTypeT                 m_deviceType;
    formFactorT                 m_formFactor;
    epicsUInt8                  m_subreleaseId; // For production releases the subrelease ID counts up from 00.
                                                // For pre-releases this ID is used “backwards” counting down from ff i.e. when
                                                // approacing release 22000207, we have prereleases 22FF0206, 22FE0206,
                                                // 22FD0206 etc. in this order.
    firmwareIdT                 m_firmwareId;
    epicsUInt8                  m_revisionId;   // this is increased with each version
    epicsUInt32                 m_firmwareVersion; // consists of |firmware ID|revision ID|subrelease ID] so that versions can be easily compared with each-other

    busConfigurationT           m_busConfiguration;
};

#endif // MRMDEVICEINFO_H
