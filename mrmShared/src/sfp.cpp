
#include <stdio.h>

// for htons() et al.
#ifdef _WIN32
 #include <Winsock2.h>
#endif

#include <epicsExport.h>
#include "mrf/object.h"
#include "sfp.h"

#include "mrfCommonIO.h"

#include "sfpinfo.h"

epicsInt16 SFP::read16(unsigned int offset) const
{
    epicsUInt16 val = buffer[offset];
    val<<=8;
    val |= buffer[offset+1];
    return val;
}

SFP::SFP(const std::string &n, volatile unsigned char *reg)
    :mrf::ObjectInst<SFP>(n)
    ,base(reg)
    ,buffer(SFPMEM_SIZE)
    ,valid(false)
{
    updateNow();

    /* Check for SFP with LC connector */
    if(valid){
        fprintf(stderr, "Found %s EEPROM\n", n.c_str());
    }else{
        fprintf(stderr, "Could not read %s EEPROM. Read: %02x %02x %02x %02x\n",
                n.c_str(), buffer[0],buffer[1],buffer[2],buffer[3]);
    }
}

SFP::~SFP() {}

void SFP::updateNow(bool)
{
    /* read I/O 4 bytes at a time to preserve endianness
     * for both PCI and VME
     */
    epicsUInt32* p32=(epicsUInt32*)&buffer[0];

    for(unsigned int i=0; i<SFPMEM_SIZE/4; i++)
        p32[i] = be_ioread32(base+ i*4);

    valid = buffer[0]==3 && buffer[2]==7;
}

double SFP::linkSpeed() const
{
    if(!valid){
        fprintf(stderr, "SFP readout not valid in linkSpeed()\n");
        return -1;
    }
    return buffer[SFP_linkrate] * 100.0; // Gives MBits/s
}

double SFP::temperature() const
{
    if(!valid){
        fprintf(stderr, "SFP readout not valid in temperature()\n");
        return -40;
    }
    return read16(SFP_temp) / 256.0; // Gives degrees C
}

double SFP::powerTX() const
{
    if(!valid){
        fprintf(stderr, "SFP readout not valid in powerTX()\n");
        return -1e-6;
    }
    return read16(SFP_tx_pwr) * 0.1e-6; // Gives Watts
}

double SFP::powerRX() const
{
    if(!valid){
        fprintf(stderr, "SFP readout not valid in powerRX()\n");
        return -1e-6;
    }
    return read16(SFP_rx_pwr) * 0.1e-6; // Gives Watts
}

static const char nomod[] = "<No Module>";

std::string SFP::vendorName() const
{
    if(!valid)
        return std::string(nomod);
    buffer_t::const_iterator it=buffer.begin()+SFP_vendor_name;
    return std::string(it, it+16);
}

std::string SFP::vendorPart() const
{
    if(!valid)
        return std::string(nomod);
    buffer_t::const_iterator it=buffer.begin()+SFP_part_num;
    return std::string(it, it+16);
}

std::string SFP::vendorRev() const
{
    if(!valid)
        return std::string(nomod);
    buffer_t::const_iterator it=buffer.begin()+SFP_part_rev;
    return std::string(it, it+4);
}

std::string SFP::serial() const
{
    if(!valid)
        return std::string(nomod);
    buffer_t::const_iterator it=buffer.begin()+SFP_serial;
    return std::string(it, it+16);
}

std::string SFP::manuDate() const
{
    if(!valid)
        return std::string(nomod);
    std::string ret("20XX/XX");
    ret[2]=buffer[SFP_man_date];
    ret[3]=buffer[SFP_man_date+1];
    ret[5]=buffer[SFP_man_date+2];
    ret[6]=buffer[SFP_man_date+3];
    return ret;
}

epicsUInt16 SFP::getStatus() const{
    if(!valid){
        fprintf(stderr, "SFP readout not valid in getStatus()\n");
        return 0xFFFF;
    }
    return (epicsUInt16)buffer[SFP_status];
}

double SFP::getVCCPower() const{
    if(!valid){
        fprintf(stderr, "SFP readout not valid in getVCCPower()\n");
        return -1e-6;
    }
    return (epicsUInt16)read16(SFP_vccPower) * 100e-6; // Gives Volts
}

epicsUInt16 SFP::getBitRateUpper() const{
    if(!valid){
        fprintf(stderr, "SFP readout not valid in getBitRateUpper()\n");
        return -1;
    }
    return (epicsUInt16)buffer[SFP_bitRateMargin_upper];    // in %
}

epicsUInt16 SFP::getBitRateLower() const{
    if(!valid){
        fprintf(stderr, "SFP readout not valid in getBitRateLower()\n");
        return -1;
    }
    return (epicsUInt16)buffer[SFP_bitRateMargin_lower];    // in %
}

epicsUInt32 SFP::getLinkLength_9um() const{
    epicsUInt32 length;

    if(!valid){
        fprintf(stderr, "SFP readout not valid in getLinkLength_9um()\n");
        return 0xFFFFFFFF;
    }
    length = (epicsUInt32)buffer[SFP_linkLength_9uminkm] * 1000;    // km
    length += (epicsUInt32)buffer[SFP_linkLength_9umin100m] * 100;  // m

    return length; // in meters
}

epicsUInt16 SFP::getLinkLength_50um() const{
    if(!valid){
        fprintf(stderr, "SFP readout not valid in getLinkLength_50um()\n");
        return 0xFFFF;
    }
    return (epicsUInt16)buffer[SFP_linkLength_50umin10m] * 10;    // in m
}

epicsUInt16 SFP::getLinkLength_62um() const{
    if(!valid){
        fprintf(stderr, "SFP readout not valid in getLinkLength_62um()\n");
        return 0xFFFF;
    }
    return (epicsUInt16)buffer[SFP_linkLength_62umin10m] * 10;    // in m
}

epicsUInt16 SFP::getLinkLength_copper() const{
    if(!valid){
        fprintf(stderr, "SFP readout not valid in getLinkLength_copper()\n");
        return 0xFFFF;
    }
    return (epicsUInt16)buffer[SFP_linkLength_copper];    // in m
}

void SFP::report() const
{
    printf("SFP tranceiver information\n"
            " Temp: %.1f C\n"
            " Link: %.1f MBits/s\n"
            " Tx Power: %.1f uW\n"
            " Rx Power: %.1f uW\n",
            temperature(),
            linkSpeed(),
            powerTX()*1e6,
            powerRX()*1e6);
    printf(" Vendor:%s\n Model: %s\n Rev: %s\n Manufacture date: %s\n Serial: %s\n",
           vendorName().c_str(),
           vendorPart().c_str(),
           vendorRev().c_str(),
           manuDate().c_str(),
           serial().c_str());
}

void SFP::reportMore() const{
    report();
    printf( " Status: 0x%X \n"
            " Supply VCC: %.1f uV/s\n"
            " Upper bit rate margin: %d %%\n"
            " Lower bit rate margin: %d %%\n"
            " Link length for 9/125 um fiber: %d m\n"
            " Link length for 50/125 um fiber: %d m\n"
            " Link length for 62.5/125 um fiber: %d m\n"
            " Link length for copper: %d m\n",
            getStatus(),
            getVCCPower()*1e6,
            getBitRateUpper(),
            getBitRateLower(),
            getLinkLength_9um(),
            getLinkLength_50um(),
            getLinkLength_62um(),
            getLinkLength_copper());
}

OBJECT_BEGIN(SFP) {

    OBJECT_PROP2("Update", &SFP::junk, &SFP::updateNow);

    OBJECT_PROP1("Vendor", &SFP::vendorName);
    OBJECT_PROP1("Part", &SFP::vendorPart);
    OBJECT_PROP1("Rev", &SFP::vendorRev);
    OBJECT_PROP1("Serial", &SFP::serial);
    OBJECT_PROP1("Date", &SFP::manuDate);

    OBJECT_PROP1("Temperature", &SFP::temperature);
    OBJECT_PROP1("Link Speed", &SFP::linkSpeed);
    OBJECT_PROP1("Power TX", &SFP::powerTX);
    OBJECT_PROP1("Power RX", &SFP::powerRX);

    OBJECT_PROP1("Status", &SFP::getStatus);
    OBJECT_PROP1("Power VCC", &SFP::getVCCPower);
    OBJECT_PROP1("BitRate Upper", &SFP::getBitRateUpper);
    OBJECT_PROP1("BitRate Lower", &SFP::getBitRateLower);
    OBJECT_PROP1("LinkLength 9um", &SFP::getLinkLength_9um);
    OBJECT_PROP1("LinkLength 50um", &SFP::getLinkLength_50um);
    OBJECT_PROP1("LinkLength 62um", &SFP::getLinkLength_62um);
    OBJECT_PROP1("LinkLength copper", &SFP::getLinkLength_copper);

} OBJECT_END(SFP)
