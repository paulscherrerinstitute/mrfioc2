// minimal revisions for individual firmware variations which are supported by mrfioc2
#define MIN_FW_EVG                      0x3
#define MIN_FW_EVR_PCI                  0x3
#define MIN_FW_EVR_VME                  0x4
#define MIN_FW_DC_REVISION              0x7
#define MIN_FW_DC_EVG_SUBRELEASE        0x1
#define MIN_FW_DC_EVR_SUBRELEASE        0x6
#define MIN_FW_DC_EMBEDDED_SUBRELEASE   0x0

#include <epicsMMIO.h>
#include "mrmShared.h"

#define epicsExportSharedSymbols
#include <shareLib.h>
#include "mrmDeviceInfo.h"


mrmDeviceInfo::mrmDeviceInfo(volatile epicsUInt8 *parentBaseAddress):
    m_base(parentBaseAddress)
{
    m_deviceType = deviceType_unknown;
    m_firmwareId = firmwareId_unknown;
    m_formFactor = formFactor_unknown;
    init();
}

void mrmDeviceInfo::setEmbeddedFormFactor()
{
    m_formFactor = formFactor_embedded;
}

std::string mrmDeviceInfo::getDeviceDescription()
{
    std::string text = getDeviceTypeStr();


    switch(m_formFactor){
    case formFactor_CPCI:
        text.append("CompactPCI 3U");
        break;

    case formFactor_CPCIFULL:
        text.append("CompactPCI 6U");
        break;

    case formFactor_CRIO:
        text.append("CompactRIO");
        break;

    case formFactor_PCIe:
        text.append("PCIe");
        break;

    case formFactor_PXIe:
        text.append("PXIe");
        break;

    case formFactor_PMC:
        text.append("PMC");
        break;

    case formFactor_VME64:
        text.append("VME 64");
        break;

    case formFactor_mTCAv4:
        text.append("MicroTCA v4");
        break;

    case formFactor_embedded:
        text.append("Embedded");
        break;

    default:
        text.append("Unknown form factor");
    }


    switch(m_firmwareId) {
    case firmwareId_mrm:
        text.append(" with modular register map");
        break;

    case firmwareId_delayCompensation:
        text.append(" with delay compensation support");
        break;

    default:
        text.append(" and cannot determine firmware ID");
    }

    return text;
}

std::string mrmDeviceInfo::getDeviceTypeStr()
{
    std::string text;

    switch(m_deviceType) {
    case deviceType_generator:
        text = "Event generator ";
        break;

    case deviceType_receiver:
        text = "Event receiver ";
        break;

    default:
        text = "Unknown device type (not EVR, not EVG): ";
    }

    return text;
}

mrmDeviceInfo::resultT mrmDeviceInfo::isDeviceSupported(deviceTypeT deviceType)
{
    // check if firmware register content makes sense
    bool baseFirmwareCheck =  m_firmwareId != firmwareId_unknown &&
                              m_deviceType != deviceType_unknown &&
                              m_formFactor != formFactor_unknown;
    if(!baseFirmwareCheck) return result_firmwareRegisterError;


    // see if device type read from firmware register is as expected by the user
    bool deviceTypeOK = ((deviceType == deviceType_generator && m_deviceType == deviceType_generator) ||
                         (deviceType == deviceType_receiver  && m_deviceType == deviceType_receiver ) ||
                          deviceType == deviceType_unknown);
    if(!deviceTypeOK) return result_deviceTypeError;


    // check if mrfioc2 supports this firmware version
    bool versionOK = m_revisionId >= getMinSupportedFwRevision() && m_subreleaseId >= getMinSupportedFwSubrelease();
    if(!versionOK) return result_firmwareVersionError;

    return result_OK;
}

void mrmDeviceInfo::setBusConfigurationVme(mrmDeviceInfo::configuration_vme configuration)
{
    m_busConfiguration.busType = busType_vme;
    m_busConfiguration.vme = configuration;
}

void mrmDeviceInfo::setBusConfigurationPci(mrmDeviceInfo::configuration_pci configuration)
{
    m_busConfiguration.busType = busType_pci;
    m_busConfiguration.pci = configuration;
}

epicsUInt32 mrmDeviceInfo::getMinSupportedFwVersion()
{
    return constructFwVersion(getMinSupportedFwSubrelease(), m_firmwareId, getMinSupportedFwRevision());
}


void mrmDeviceInfo::init()
{
    m_firmwareRegister = nat_ioread32(m_base + U32_FWVersion);

    // parse firmware register
    m_deviceType = (deviceTypeT)((m_firmwareRegister & FWVersion_type_mask) >> FWVersion_type_shift);
    m_subreleaseId = (m_firmwareRegister & FWVersion_subreleaseId_mask) >> FWVersion_subreleaseId_shift;
    epicsUInt32 firmwareId = ((m_firmwareRegister & FWVersion_firmwareId_mask) >> FWVersion_firmwareId_shift);
    m_firmwareId = (firmwareIdT)firmwareId;
    m_revisionId = (m_firmwareRegister & FWVersion_revisionId_mask) >> FWVersion_revisionId_shift;

    // construct firmware version
    m_firmwareVersion = constructFwVersion(m_subreleaseId, firmwareId, m_revisionId);

    // get form factor
    epicsUInt32 form = (m_firmwareRegister & FWVersion_form_mask) >> FWVersion_form_shift;
    if(form <= formFactor_last){
        m_formFactor = (formFactor)form;
    }
    else{
        m_formFactor = formFactor_unknown;
    }
}

epicsUInt8 mrmDeviceInfo::getMinSupportedFwRevision()
{
    epicsUInt8 minRevision;

    if(m_firmwareId == firmwareId_delayCompensation) {
        minRevision = MIN_FW_DC_REVISION;
    }
    else if (m_deviceType == deviceType_generator) {
        minRevision = MIN_FW_EVG;
    }
    else if (m_formFactor == formFactor_VME64) {
        minRevision = MIN_FW_EVR_VME;
    }
    else {
        minRevision = MIN_FW_EVR_PCI;
    }

    return minRevision;
}

epicsUInt8 mrmDeviceInfo::getMinSupportedFwSubrelease()
{
    epicsUInt8 minSubrelease = 0;

    if(m_firmwareId == firmwareId_delayCompensation) {
        if(m_formFactor == formFactor_embedded) {
            minSubrelease = MIN_FW_DC_EMBEDDED_SUBRELEASE;
        }
        else if(m_deviceType == deviceType_generator) {
            minSubrelease = MIN_FW_DC_EVG_SUBRELEASE;
        }
        else if(m_deviceType == deviceType_receiver) {
            minSubrelease = MIN_FW_DC_EVR_SUBRELEASE;
        }
    }

    return minSubrelease;
}

epicsUInt32 mrmDeviceInfo::constructFwVersion(epicsUInt8 subreleaseId, epicsUInt8 firmwareId, epicsUInt8 revisionId)
{
    epicsUInt32 version = firmwareId;
    version <<= 8;
    version += revisionId;
    version <<= 8;
    version += subreleaseId;
    return version;
}
