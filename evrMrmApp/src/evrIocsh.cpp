/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <stdlib.h>
#include <cstdio>
#include <cstring>

#include <stdexcept>
#include <sstream>
#include <map>

#include <epicsString.h>
#include <drvSup.h>
#include <iocsh.h>
#include <initHooks.h>
#include <epicsExit.h>

#include <devLibPCI.h>
#include <devcsr.h>
#include <epicsInterrupt.h>
#include <epicsThread.h>
#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#include "mrfcsr.h"
#include "mrfpci.h"
#include "mrmDeviceInfo.h"

// for htons() et al.
#ifdef _WIN32
 #include <Winsock2.h>
#endif

#include "evrRegMap.h"
#include "plx9030.h"
#include "plx9056.h"

#include "evgMrm.h"
#include <epicsExport.h>
#include "evrIocsh.h"


#ifdef _WIN32
 #define strtok_r(strToken,strDelimit,lasts ) (*(lasts) = strtok((strToken),(strDelimit)))
#endif


/* Bit mask used to communicate which VME interrupt levels
 * are used.  Bits are set by mrmEvrSetupVME().  Levels are
 * enabled later during iocInit.
 */
static epicsUInt8 vme_level_mask = 0;

static const epicsPCIID mrmevrs[] = {
    DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9030,    PCI_VENDOR_ID_PLX,
                               PCI_DEVICE_ID_MRF_PMCEVR_230, PCI_VENDOR_ID_MRF)
    ,DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9030,    PCI_VENDOR_ID_PLX,
                                PCI_DEVICE_ID_MRF_PXIEVR_230, PCI_VENDOR_ID_MRF)
    ,DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9056,    PCI_VENDOR_ID_PLX,
                                PCI_DEVICE_ID_MRF_EVRTG_300, PCI_VENDOR_ID_MRF)
    ,DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_EC_30,    PCI_VENDOR_ID_LATTICE,
                                PCI_DEVICE_ID_MRF_EVRTG_300E, PCI_VENDOR_ID_MRF)
    ,DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_XILINX,    PCI_VENDOR_ID_XILINX,
                                PCI_DEVICE_ID_MRF_EVR_300DC, PCI_VENDOR_ID_MRF)
    ,DEVPCI_END
};

static const struct VMECSRID vmeevrs[] = {
    // VME EVR RF 230
    {MRF_VME_IEEE_OUI, MRF_VME_EVR_RF_BID|MRF_SERIES_230, VMECSRANY},
    {MRF_VME_IEEE_OUI, MRF_VME_EVR_300, VMECSRANY}
    ,VMECSR_END
};

static
const
struct printreg
{
    const char *label;
    epicsUInt32 offset;
    int rsize;
} printreg[] = {
#define REGINFO(label, name, size) {label, U##size##_##name, size}
REGINFO("Version", FWVersion, 32),
REGINFO("Control", Control, 32),
REGINFO("Status",  Status, 32),
REGINFO("IRQ Flag",IRQFlag, 32),
REGINFO("IRQ Ena", IRQEnable, 32),
REGINFO("IRQPlsmap",IRQPulseMap, 32),
REGINFO("DataRxCtrlEvr",DataRxCtrlEvr, 32),
REGINFO("DataTxCtrlEvr",DataTxCtrlEvr, 32),
REGINFO("CountPS", CounterPS, 32),
REGINFO("USecDiv", USecDiv, 32),
REGINFO("ClkCtrl", ClkCtrl, 32),
REGINFO("LogSts",  LogStatus, 32),
REGINFO("TSSec",TSSec, 32),
REGINFO("TSEvt",TSEvt, 32),
REGINFO("TSSecLath",TSSecLatch, 32),
REGINFO("TSEvtLath",TSEvtLatch, 32),
REGINFO("FracDiv", FracDiv, 32),
REGINFO("Scaler0",Scaler(0),32),
REGINFO("Pul0Ctrl",PulserCtrl(0),32),
REGINFO("Pul0Scal",PulserScal(0),32),
REGINFO("Pul0Dely",PulserDely(0),32),
REGINFO("Pul0Wdth",PulserWdth(0),32),
REGINFO("FP01MAP",OutputMapFP(0),32),
REGINFO("FPU01MAP",OutputMapFPUniv(0),32),
REGINFO("RB01MAP",OutputMapRB(0),32),
REGINFO("FPIN0",InputMapFP(0),32),
REGINFO("CML4Low",OutputCMLLow(0),32),
REGINFO("CML4Rise",OutputCMLRise(0),32),
REGINFO("CML4High",OutputCMLHigh(0),32),
REGINFO("CML4Fall",OutputCMLFall(0),32),
REGINFO("CML4Ena",OutputCMLEna(0),32),
REGINFO("CML4Cnt",OutputCMLCount(0),32),
REGINFO("CML4Len",OutputCMLPatLength(0),32),
REGINFO("CML4Pat0",OutputCMLPat(0,0),32),
REGINFO("CML4Pat1",OutputCMLPat(0,1),32),
REGINFO("TXBuf0-3",DataTx(0),32),
REGINFO("RXBuf0-3",DataRx(0),32),
REGINFO("RXBuf_seg0-3",DataRx_seg(0),32)
#undef REGINFO
};

static
void
printregisters(volatile epicsUInt8 *evr)
{
    size_t reg;

    epicsPrintf("EVR register dump\n");
    for(reg=0; reg<NELEMENTS(printreg); reg++){

        switch(printreg[reg].rsize){
        case 8:
            epicsPrintf("%9s: %02x\n",
                   printreg[reg].label,
                   ioread8(evr+printreg[reg].offset));
            break;
        case 16:
            epicsPrintf("%9s: %04x\n",
                   printreg[reg].label,
                   nat_ioread16(evr+printreg[reg].offset));
            break;
        case 32:
            epicsPrintf("%9s: %08x\n",
                   printreg[reg].label,
                   nat_ioread32(evr+printreg[reg].offset));
            break;
        }
    }
}

static
bool reportCard(mrf::Object* obj, void* raw)
{
    // this function is called by Object::visitObjects
    // it must return 'true' in order for the Object::visitObjects to continue searching for objects.
    // if false is returned, Object::visitObjects stops

    int *level=(int*)raw;
    EVRMRM *evr=dynamic_cast<EVRMRM*>(obj);
    if(!evr){
        return true;
    }

    mrmDeviceInfo *deviceInfo = evr->getDeviceInfo();

    epicsPrintf("EVR: %s\n",obj->name().c_str());
    epicsPrintf("\tFPGA Version: %08x (firmware: %x)\n", deviceInfo->getFirmwareRegister(), deviceInfo->getFirmwareVersion());
    epicsPrintf("\tDescription: %s\n", deviceInfo->getDeviceDescription().c_str());
    epicsPrintf("\tClock: %.6f MHz\n",evr->clock()*1e-6);

    mrmDeviceInfo::busConfigurationT bus = deviceInfo->getBusConfiguration();
    if(bus.busType == mrmDeviceInfo::busType_vme){
        struct VMECSRID vmeDev;
        volatile unsigned char* csrAddr = devCSRTestSlot(vmeevrs, bus.vme.slot, &vmeDev);
        if(csrAddr){
            epicsUInt32 ader = CSRRead32(csrAddr + CSR_FN_ADER(2));
            size_t user_offset=CSRRead24(csrAddr+CR_BEG_UCSR);
            // Currently that value read from the UCSR pointer is
            // actually little endian.
            user_offset= (( user_offset & 0x00ff0000 ) >> 16 ) |
                         (( user_offset & 0x0000ff00 )       ) |
                         (( user_offset & 0x000000ff ) << 16 );

            epicsPrintf("\tVME configured slot: %d\n", bus.vme.slot);
            epicsPrintf("\tVME configured A32 address 0x%08x\n", bus.vme.address);
            epicsPrintf("\tVME ADER: base address=0x%x\taddress modifier=0x%x\n", ader>>8, (ader&0xFF)>>2);
            epicsPrintf("\tVME IRQ Level %x (configured to %x)\n", CSRRead8(csrAddr + user_offset + UCSR_IRQ_LEVEL), bus.vme.irqLevel);
            epicsPrintf("\tVME IRQ Vector %x (configured to %x)\n", CSRRead8(csrAddr + user_offset + UCSR_IRQ_VECTOR), bus.vme.irqVector);
            if(*level>1) epicsPrintf("\tVME card vendor: 0x%08x\n", vmeDev.vendor);
            if(*level>1) epicsPrintf("\tVME card board: 0x%08x\n", vmeDev.board);
            if(*level>1) epicsPrintf("\tVME card revision: 0x%08x\n", vmeDev.revision);
            if(*level>1) epicsPrintf("\tVME CSR address: %p\n", csrAddr);
        }else{
            epicsPrintf("\tCard not detected in configured slot %d\n", bus.vme.slot);
        }
    }
    else if(bus.busType == mrmDeviceInfo::busType_pci){
        const epicsPCIDevice *pciDev;
        if(!devPCIFindBDF(mrmevrs, bus.pci.bus, bus.pci.device, bus.pci.function, &pciDev, 0)){
            epicsPrintf("\tPCI configured bus: 0x%08x\n", bus.pci.bus);
            epicsPrintf("\tPCI configured device: 0x%08x\n", bus.pci.device);
            epicsPrintf("\tPCI configured function: 0x%08x\n", bus.pci.function);
            epicsPrintf("\tPCI IRQ: %u\n", pciDev->irq);
            if(*level>1) epicsPrintf("\tPCI class: 0x%08x, revision: 0x%x, sub device: 0x%x, sub vendor: 0x%x\n", pciDev->id.pci_class, pciDev->id.revision, pciDev->id.sub_device, pciDev->id.sub_vendor);

        }else{
            epicsPrintf("\tPCI Device not found\n");
        }
    }else{
        epicsPrintf("\tUnknown bus type\n");
    }

    if(*level>=2){
        printregisters(evr->base);
    }
    if(*level>=1 && evr->sfp.get()){
        evr->sfp->updateNow();
        evr->sfp->report();
    }

    return true;
}

static
long report(int level)
{
    epicsPrintf("=== Begin MRF EVR support ===\n");
    mrf::Object::visitObjects(&reportCard, (void*)&level);
    epicsPrintf("=== End MRF EVR support ===\n");
    return 0;
}

static
mrmDeviceInfo::resultT checkVersion(mrmDeviceInfo *deviceInfo)
{
    epicsPrintf("Firmware version register: 0x%08x\n", deviceInfo->getFirmwareRegister());
    epicsPrintf("Firmware version: 0x%08x\n", deviceInfo->getFirmwareVersion());

    mrmDeviceInfo::resultT result = deviceInfo->isDeviceSupported(mrmDeviceInfo::deviceType_receiver);
    if(result != mrmDeviceInfo::result_OK) {
        switch(result) {
        case mrmDeviceInfo::result_deviceTypeError:
            errlogPrintf("Found device type 0x%x which does not correspond to EVR (type 0x%x).\n", deviceInfo->getDeviceType(), mrmDeviceInfo::deviceType_receiver);
            break;

        case mrmDeviceInfo::result_firmwareRegisterError:
            errlogPrintf("Content of the firmware register could not be parsed.\n");
            break;

        case mrmDeviceInfo::result_firmwareVersionError:
            errlogPrintf("This firmware version is not supported by the driver!\n\t"
                         "You may still flash the device with supported firmware.\n\t"
                         "Minimal supported firmware version is 0x%x\n", deviceInfo->getMinSupportedFwVersion());
            break;

        default:
            errlogPrintf("Unknown error while checking firmware version\n");
        }
    }
    else {
        epicsPrintf("Minimal supported firmware version for %s is 0x%x\n", deviceInfo->getDeviceDescription().c_str(), deviceInfo->getMinSupportedFwVersion());
    }

    return result;
}

#ifdef __linux__
static char ifaceversion[] = "/sys/module/mrf/parameters/interfaceversion";
/* Check the interface version of the kernel module to ensure compatibility */
static
int checkUIOVersion(int expect)
{
    FILE *fd;
    int version = -1;

    fd = fopen(ifaceversion, "r");
    if(!fd) {
        errlogPrintf("Can't open %s in order to read kernel module interface version. Is kernel module loaded?\n", ifaceversion);
        return 1;
    }
    if(fscanf(fd, "%d", &version) != 1) {
        errlogPrintf("Failed to read %s in order to get the kernel module interface version. Is kernel module loaded?\n", ifaceversion);
        fclose(fd);
        return 1;
    }
    fclose(fd);

    if(version<expect) {
        errlogPrintf("Error: Expect MRF kernel module version %d, found %d.\n", version, expect);
        return 1;
    }
    if(version > expect){
        epicsPrintf("Info: Expect MRF kernel module version %d, found %d.\n", version, expect);
    }
    return 0;
}
#else
static int checkUIOVersion(int expect) {return 0;}
#endif

extern "C"
epicsStatus
mrmEvrSetupPCI(const char* id,      // Card Identifier
               int o,               // Domain number
               int b,               // Bus number
               int d,               // Device number
               int f,               // Function number
               bool ignoreVersion)  // Ignore errors due to kernel module and firmware version checks
{
    mrmDeviceInfo::busConfigurationPciT bus;
    bus.bus = b;
    bus.device = d;
    bus.function = f;

    volatile epicsUInt8 *plx = 0, *evr = 0; // base addressed for plx/evr bar

    try {
        if(mrf::Object::getObject(id)){
            errlogPrintf("Object ID %s already in use\n",id);
            return -1;
        }

        if(checkUIOVersion(1) > 0) {    // check if kernel version is successfully read and is as expected or higher, and if it can be read at all.
            if(ignoreVersion){
                epicsPrintf("Ignoring kernel module error.\n");
            }
            else{
                return -1;
            }
        }


        // get pci device from devLib2
        const epicsPCIDevice *cur = 0;
        int err;
        if( (err=devPCIFindDBDF(mrmevrs,o,b,d,f,&cur,0)) ){
            errlogPrintf("PCI Device not found on %x:%x:%x.%x with error: %d\n", o, b, d, f, err);
            return -1;
        }

        epicsPrintf("Device %s  %x:%x.%x\n", id, cur->bus, cur->device, cur->function);
        epicsPrintf("Using IRQ %u\n",cur->irq);


        /*
         * The EC 30 device has only 1 bar (0) which we need to map to *evr.
         */
        if(cur->id.device == PCI_DEVICE_ID_EC_30 || cur->id.device == PCI_DEVICE_ID_XILINX) {
            if(devPCIToLocalAddr(cur,0,(volatile void**)(void *)&evr,DEVLIB_MAP_UIO1TO1))
            {
                errlogPrintf("PCI error: Failed to map BARs 0 for EC 30\n");
                return -1;
            }
            if(!evr){
                errlogPrintf("PCI error: BAR mapped to zero? (%08lx)\n", (unsigned long)evr);
                return -1;
            }
        } else {
            if( devPCIToLocalAddr(cur,0,(volatile void**)(void *)&plx,DEVLIB_MAP_UIO1TO1) ||
                devPCIToLocalAddr(cur,2,(volatile void**)(void *)&evr,DEVLIB_MAP_UIO1TO1))
            {
                errlogPrintf("PCI error: Failed to map BARs 0 and 2\n");
                return -1;
            }
            if(!plx || !evr){
                errlogPrintf("PCI error: BARs mapped to zero? (%08lx,%08lx)\n",
                       (unsigned long)plx,(unsigned long)evr);
                return -1;
            }
        }

        // handle various PCI to local bus bridges
        switch(cur->id.device) {
        case PCI_DEVICE_ID_PLX_9030:
            /* Use the PLX device on the EVR to swap access on
             * little endian systems so we don't have no worry about
             * byte order :)
             */
    #if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
            BITSET(LE,32, plx, LAS0BRD, LAS0BRD_ENDIAN);
    #elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
            BITCLR(LE,32, plx, LAS0BRD, LAS0BRD_ENDIAN);
    #endif

            // Disable interrupts on device

            NAT_WRITE32(evr, IRQEnable, 0);

    #if !defined(__linux__) || !defined(_WIN32)
            /* Enable active high interrupt1 through the PLX to the PCI bus.
             */
            LE_WRITE16(plx, INTCSR, INTCSR_INT1_Enable|
                       INTCSR_INT1_Polarity|
                       INTCSR_PCI_Enable);
    #else
            /* ask the kernel module to enable interrupts through the PLX bridge */
            if(ignoreVersion){
                epicsPrintf("Not enabling interrupts.\n");
            }
            else {
                if(devPCIEnableInterrupt(cur)) {
                    errlogPrintf("PLX 9030: Failed to enable interrupt\n");
                    return -1;
                }
            }
    #endif
            break;

        case PCI_DEVICE_ID_PLX_9056:
    #if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
            BITSET(LE,8, plx, BIGEND9056, BIGEND9056_BIG);
    #elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
            BITCLR(LE,8, plx, BIGEND9056, BIGEND9056_BIG);
    #endif

            // Disable interrupts on device

            NAT_WRITE32(evr, IRQEnable, 0);

    #if !defined(__linux__) || !defined(_WIN32)
            BITSET(LE,32,plx, INTCSR9056, INTCSR9056_PCI_Enable|INTCSR9056_LCL_Enable);
    #else
            /* ask the kernel module to enable interrupts */
            if(ignoreVersion){
                epicsPrintf("Not enabling interrupts.\n");
            }
            else {
                if(devPCIEnableInterrupt(cur)) {
                    errlogPrintf("PLX 9056: Failed to enable interrupt\n");
                    return -1;
                }
            }
    #endif
            break;

        case PCI_DEVICE_ID_EC_30:
        case PCI_DEVICE_ID_XILINX:
    #if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
            //BITCLR(LE,32, evr, AC30CTRL, AC30CTRL_LEMDE);
    #elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
            //BITSET(LE,32, evr, AC30CTRL, AC30CTRL_LEMDE);
            *(epicsUInt8 *)(evr+0x04) = 0x82; // unknown magic number
            epicsPrintf("Setting magic LE number!\n");
    #endif

            // Disable interrupts on device
            NAT_WRITE32(evr, IRQEnable, 0);

    #if !defined(__linux__) || !defined(_WIN32)
            BITSET32(evr, IRQEnable, IRQ_PCIee);
    #else
            /* ask the kernel module to enable interrupts */
            if(ignoreVersion){
                epicsPrintf("Not enabling interrupts.\n");
            }
            else {
                if(cur->id.device == PCI_DEVICE_ID_EC_30){
                    epicsPrintf("EC 30: Enabling interrupts\n");
                }
                else if(cur->id.device == PCI_DEVICE_ID_XILINX){
                    epicsPrintf("Xilinx: Enabling interrupts\n");
                }
                if(devPCIEnableInterrupt(cur)) {
                    errlogPrintf("EC 30: Failed to enable interrupt\n");
                    return -1;
                }
            }
    #endif
            break;
        default:
            errlogPrintf("Unknown PCI bridge %04x\n", cur->id.device);
            return -1;
        }


        // Set up device info and check if device is supported (firmware version check)
        mrmDeviceInfo *deviceInfo = new mrmDeviceInfo(evr);
        deviceInfo->setBusConfigurationPci(bus);

        mrmDeviceInfo::resultT versionCheckResult = checkVersion(deviceInfo);
        if(versionCheckResult == mrmDeviceInfo::result_firmwareVersionError) {
            errlogPrintf("Firmware version error detected. Continuing loading of the driver.\n");
        }
        else if(versionCheckResult != mrmDeviceInfo::result_OK) {
            if(ignoreVersion) {
                epicsPrintf("Ignoring version error.\n");
            }
            else {
                return -1;
            }
        }

        // Acknowledge missed interrupts
        //TODO: This avoids a spurious FIFO Full
        NAT_WRITE32(evr, IRQFlag, NAT_READ32(evr, IRQFlag));

        // Install ISR

        EVRMRM *receiver=new EVRMRM(id,*deviceInfo,evr, NULL);

        void *arg=receiver;
        receiver->pciDevice = cur;

        if(ignoreVersion){
            epicsPrintf("Not connecting interrupts.\n");
        }
        else {
            if(devPCIConnectInterrupt(cur, &EVRMRM::isr_pci, arg, 0)){
                errlogPrintf("Failed to install ISR\n");
                delete receiver;
            }else{
                // Interrupts will be enabled during iocInit()
            }
        }
    } catch(std::exception& e) {
        errlogPrintf("Error: %s\n",e.what());
        errlogFlush();
        return -1;
    }
    errlogFlush();
    return 0;
}

static
void
printRamEvt(EVRMRM *evr,int evt,int ram)
{
    if(evt<0 || evt>255)
        return;
    if(ram<0 || ram>1)
        return;

    epicsUInt32 map[4];

    map[0]=NAT_READ32(evr->base, MappingRam(ram,evt,Internal));
    map[1]=NAT_READ32(evr->base, MappingRam(ram,evt,Trigger));
    map[2]=NAT_READ32(evr->base, MappingRam(ram,evt,Set));
    map[3]=NAT_READ32(evr->base, MappingRam(ram,evt,Reset));

    epicsPrintf("Event 0x%02x %3d ",evt,evt);
    epicsPrintf("%08x %08x %08x %08x\n",map[0],map[1],map[2],map[3]);
}

static
bool
enableIRQ(mrf::Object* obj, void*)
{
    EVRMRM *mrm=dynamic_cast<EVRMRM*>(obj);
    if(!mrm)
        return true;

    mrm->enableIRQ();

    return true;
}

static
bool
disableIRQ(mrf::Object* obj, void*)
{
    EVRMRM *mrm=dynamic_cast<EVRMRM*>(obj);
    if(!mrm)
        return true;

    mrm->disableIRQ();

    return true;
}

static
void
evrShutdown(void*)
{
    mrf::Object::visitObjects(&disableIRQ,0);
}

static bool
startSFPUpdate(mrf::Object* obj, void*)
{
    EVRMRM *mrm=dynamic_cast<EVRMRM*>(obj);
    if(!mrm)
        return true;

    SFP * sfp = mrm->sfp.get();
    if(sfp)sfp->startUpdate();
    //mrm->sfp.get()->startUpdate();
    //TODO what if this fails....

    return true;
}

static
void inithooks(initHookState state)
{
    epicsUInt8 lvl;
    switch(state)
    {
    case initHookAfterInterruptAccept:
        // Register hook to disable interrupts on IOC shutdown
        epicsAtExit(&evrShutdown, NULL);
        // First enable interrupts for each EVR
        mrf::Object::visitObjects(&enableIRQ,0);
        // Then enable all used levels
        for(lvl=1; lvl<=7; ++lvl)
        {
            if (vme_level_mask&(1<<(lvl-1))) {
                if(devEnableInterruptLevelVME(lvl))
                {
                    errlogPrintf("Failed to enable interrupt level %d\n",lvl);
                    return;
                }
            }

        }

        break;

    /*
     * callback for updating SFP info gets called here for the first time.
     */
    case initHookAfterCallbackInit:
        mrf::Object::visitObjects(&startSFPUpdate, 0);
        break;

  default:
        break;
    }
}


static const iocshArg mrmEvrSetupPCIArg0 = { "Device",iocshArgString};
static const iocshArg mrmEvrSetupPCIArg1 = { "Domain number",iocshArgInt};
static const iocshArg mrmEvrSetupPCIArg2 = { "Bus number",iocshArgInt};
static const iocshArg mrmEvrSetupPCIArg3 = { "Device number",iocshArgInt};
static const iocshArg mrmEvrSetupPCIArg4 = { "Function number",iocshArgInt};
static const iocshArg mrmEvrSetupPCIArg5 = { "'Ignore version error'", iocshArgArgv};

static const iocshArg * const mrmEvrSetupPCIArgs[6] ={&mrmEvrSetupPCIArg0,
                                                      &mrmEvrSetupPCIArg1,
                                                      &mrmEvrSetupPCIArg2,
                                                      &mrmEvrSetupPCIArg3,
                                                      &mrmEvrSetupPCIArg4,
                                                      &mrmEvrSetupPCIArg5};
static const iocshFuncDef mrmEvrSetupPCIFuncDef = {"mrmEvrSetupPCI", 6, mrmEvrSetupPCIArgs};
static void mrmEvrSetupPCICallFunc(const iocshArgBuf *args)
{
    // check the 'ignore' parameter
    if(args[5].aval.ac > 1 && (strcmp("true", args[5].aval.av[1]) == 0 ||
                               strtol(args[5].aval.av[1], NULL, 10) != 0)){
        mrmEvrSetupPCI(args[0].sval,args[1].ival,args[2].ival,args[3].ival,args[4].ival, true);
    }
    else {
        mrmEvrSetupPCI(args[0].sval,args[1].ival,args[2].ival,args[3].ival,args[4].ival, false);
    }
}

extern "C"
epicsStatus
mrmEvrSetupVME(const char* id,      // Card Identifier
               int slot,            // VME slot
               int base,            // Desired VME address in A24 space
               int level,           // Desired interrupt level
               int vector,          // Desired interrupt vector number
               bool ignoreVersion)  // Ignore errors due to firmware checks
{

    mrmDeviceInfo::busConfigurationVmeT bus;
    bus.slot = slot;
    bus.address = base;
    bus.irqLevel = level;
    bus.irqVector = vector;


    volatile unsigned char* evr;    // base address for the card

    try {
        if(mrf::Object::getObject(id)){
            errlogPrintf("ID %s already in use\n",id);
            return -1;
        }

        struct VMECSRID info;
        volatile unsigned char* csr;    // csr is VME-CSR space CPU address for the board
        csr=devCSRTestSlot(vmeevrs,slot,&info);
        if(!csr){
            errlogPrintf("No EVR in slot %d\n",slot);
            return -1;
        }

        epicsPrintf("Setting up EVR in VME Slot %d\n",slot);

        epicsPrintf("Found vendor: %08x board: %08x rev.: %08x\n",
               info.vendor, info.board, info.revision);

        // Set base address

      /* Use function 0 for 16-bit addressing (length 0x00800 bytes)
       * and function 1 for 24-bit addressing (length 0x10000 bytes)
       * and function 2 for 32-bit addressing (length 0x40000 bytes)
       *
       * All expose the same registers, but not all registers are
       * visible through functions 0 or 1.
       */

        CSRSetBase(csr, 2, base, VME_AM_EXT_SUP_DATA);

        {
            epicsUInt32 temp=CSRRead32((csr) + CSR_FN_ADER(2));

            if(temp != CSRADER((epicsUInt32)base,VME_AM_EXT_SUP_DATA)) {
                errlogPrintf("Failed to set CSR Base address in ADER2.  Check VME bus and card firmware version.\n");
                return -1;
            }
        }

        char *Description = allocSNPrintf(40, "EVR-%d '%s' slot %d",
                                          info.board & MRF_BID_SERIES_MASK,
                                          id, slot);

        if(devRegisterAddress(Description, atVMEA32, base, EVR_REGMAP_SIZE, (volatile void**)(void *)&evr))
        {
            errlogPrintf("Failed to map address %08x\n",base);
            return -1;
        }

        epicsUInt32 junk;
        if(devReadProbe(sizeof(junk), (volatile void*)(evr+U32_FWVersion), (void*)&junk)) {
            errlogPrintf("Failed to read from MRM registers (but could read CSR registers)\n");
            return -1;
        }


        // Set up device info and check if device is supported (firmware version check)
        mrmDeviceInfo *deviceInfo = new mrmDeviceInfo(evr);
        deviceInfo->setBusConfigurationVme(bus);

        mrmDeviceInfo::resultT versionCheckResult = checkVersion(deviceInfo);
        if(versionCheckResult == mrmDeviceInfo::result_firmwareVersionError) {
            errlogPrintf("Firmware version error detected. Continuing loading of the driver.\n");
        }
        else if(versionCheckResult != mrmDeviceInfo::result_OK) {
            if(ignoreVersion) {
                epicsPrintf("Ignoring version error.\n");
            }
            else {
                return -1;
            }
        }


        // Read offset from start of CSR to start of user (card specific) CSR.
        size_t user_offset=CSRRead24(csr+CR_BEG_UCSR);
        // Currently that value read from the UCSR pointer is
        // actually little endian.
        user_offset= (( user_offset & 0x00ff0000 ) >> 16 ) |
                     (( user_offset & 0x0000ff00 )       ) |
                     (( user_offset & 0x000000ff ) << 16 );
        volatile unsigned char* user_csr=csr+user_offset;

        NAT_WRITE32(evr, IRQEnable, 0); // Disable interrupts

        EVRMRM *receiver=new EVRMRM(id, *deviceInfo, evr, NULL);

        if(!ignoreVersion && level>0 && vector>=0) {
            CSRWrite8(user_csr+UCSR_IRQ_LEVEL,  level&0x7);
            CSRWrite8(user_csr+UCSR_IRQ_VECTOR, vector&0xff);

            epicsPrintf("Using IRQ %d:%2d\n",
                   CSRRead8(user_csr+UCSR_IRQ_LEVEL),
                   CSRRead8(user_csr+UCSR_IRQ_VECTOR)
                   );

            // Acknowledge missed interrupts
            //TODO: This avoids a spurious FIFO Full
            NAT_WRITE32(evr, IRQFlag, NAT_READ32(evr, IRQFlag));

            level&=0x7;
            // VME IRQ level will be enabled later during iocInit()
            vme_level_mask|=1<<(level-1);

            if(devConnectInterruptVME(vector&0xff, &EVRMRM::isr_vme, receiver))
            {
                errlogPrintf("Failed to connection VME IRQ %d\n",vector&0xff);
                delete receiver;
                return -1;
            }

            // Interrupts will be enabled during iocInit()
        }
        else {
            epicsPrintf("Not connecting interrupts.\n");
        }

    } catch(std::exception& e) {
        errlogPrintf("Error: %s\n",e.what());
        errlogFlush();
        return -1;
    }
    errlogFlush();
    return 0;
}

static const iocshArg mrmEvrSetupVMEArg0 = { "Device",iocshArgString};
static const iocshArg mrmEvrSetupVMEArg1 = { "Bus number",iocshArgInt};
static const iocshArg mrmEvrSetupVMEArg2 = { "A32 base address",iocshArgInt};
static const iocshArg mrmEvrSetupVMEArg3 = { "IRQ Level 1-7 (0 - disable)",iocshArgInt};
static const iocshArg mrmEvrSetupVMEArg4 = { "IRQ vector 0-255",iocshArgInt};
static const iocshArg mrmEvrSetupVMEArg5 = { "'Ignore version error'", iocshArgArgv};

static const iocshArg * const mrmEvrSetupVMEArgs[6] ={&mrmEvrSetupVMEArg0,
                                                      &mrmEvrSetupVMEArg1,
                                                      &mrmEvrSetupVMEArg2,
                                                      &mrmEvrSetupVMEArg3,
                                                      &mrmEvrSetupVMEArg4,
                                                      &mrmEvrSetupVMEArg5};
static const iocshFuncDef mrmEvrSetupVMEFuncDef =  {"mrmEvrSetupVME", 6, mrmEvrSetupVMEArgs};
static void mrmEvrSetupVMECallFunc(const iocshArgBuf *args)
{
    // check the 'ignore' parameter
    if(args[5].aval.ac > 1 && (strcmp("true", args[5].aval.av[1]) == 0 ||
                               strtol(args[5].aval.av[1], NULL, 10) != 0)){
        mrmEvrSetupVME(args[0].sval,args[1].ival,args[2].ival,args[3].ival,args[4].ival, true);
    }
    else {
        mrmEvrSetupVME(args[0].sval,args[1].ival,args[2].ival,args[3].ival,args[4].ival, false);
    }
}

extern "C"
void
mrmEvrDumpMap(const char* id,int evt,int ram)
{
    try {
        mrf::Object *obj=mrf::Object::getObject(id);
        if(!obj)
            throw std::runtime_error("Object not found");
        EVRMRM *card=dynamic_cast<EVRMRM*>(obj);
        if(!card)
            throw std::runtime_error("Not a MRM EVR");

        epicsPrintf("Print ram #%d\n",ram);
        if(evt>=0){
            // Print a single event
            printRamEvt(card,evt,ram);
            return;
        }
        for(evt=0;evt<=255;evt++){
            printRamEvt(card,evt,ram);
        }
    } catch(std::exception& e) {
        errlogPrintf("Error: %s\n",e.what());
    }
}

static const iocshArg mrmEvrDumpMapArg0 = { "Device",iocshArgString};
static const iocshArg mrmEvrDumpMapArg1 = { "Event code",iocshArgInt};
static const iocshArg mrmEvrDumpMapArg2 = { "Mapping select 0 or 1",iocshArgInt};
static const iocshArg * const mrmEvrDumpMapArgs[3] =
{&mrmEvrDumpMapArg0,&mrmEvrDumpMapArg1,&mrmEvrDumpMapArg2};
static const iocshFuncDef mrmEvrDumpMapFuncDef =
    {"mrmEvrDumpMap",3,mrmEvrDumpMapArgs};
static void mrmEvrDumpMapCallFunc(const iocshArgBuf *args)
{
    mrmEvrDumpMap(args[0].sval,args[1].ival,args[2].ival);
}

/** @brief Setup Event forwarding to downstream link
 *
 * Control which events will be forwarded to the downstream event link
 * when they are received on the upstream link.
 * Useful when daisy chaining EVRs.
 *
 * When invoked with the second argument as NULL or "" the current
 * forward mapping is printed.
 *
 * The second argument to this function is a comma seperated list of
 * event numbers and/or the special token 'all'.  If a token is prefixed
 * with '-' then the mapping is cleared, otherwise it is set.
 *
 * After a cold boot, no events are forwarded until/unless mrmrEvrForward
 * is called.
 *
 @code
   > mrmEvrForward("EVR1") # Prints current mappings
   Events forwarded: ...
   > mrmEvrForward("EVR1", "-all") # Clear all forward mappings
   # Clear all forward mappings except timestamp transmission
   > mrmEvrForward("EVR1", "-all, 0x70, 0x71, 0x7A, 0x7D")
   # Forward all except 42
   > mrmEvrForward("EVR1", "all, -42")
 @endcode
 *
 @param id EVR identifier
 @param events A string with a comma seperated list of event specifiers
 */
extern "C"
void
mrmEvrForward(const char* id, const char* events_iocsh)
{
    char *events=events_iocsh ? epicsStrDup(events_iocsh) : 0;
try {
    mrf::Object *obj=mrf::Object::getObject(id);
    if(!obj)
        throw std::runtime_error("Object not found");
    EVRMRM *card=dynamic_cast<EVRMRM*>(obj);
    if(!card)
        throw std::runtime_error("Not a MRM EVR");

    if(!events || strlen(events)==0) {
        // Just print current mappings
        epicsPrintf("Events forwarded: ");
        for(unsigned int i=1; i<256; i++) {
            if(card->specialMapped(i, ActionEvtFwd)) {
                epicsPrintf("%d ", i);
            }
        }
        epicsPrintf("\n");
        free(events);
        return;
    }

    // update mappings

    const char sep[]=", ";
    char *save=0;

    for(char *tok=strtok_r(events, sep, &save);
        tok!=NULL;
        tok = strtok_r(0, sep, &save)
        )
    {
        if(strcmp(tok, "-all")==0) {
            for(unsigned int i=1; i<256; i++)
                card->specialSetMap(i, ActionEvtFwd, false);

        } else if(strcmp(tok, "all")==0) {
            for(unsigned int i=1; i<256; i++)
                card->specialSetMap(i, ActionEvtFwd, true);

        } else {
            char *end=0;
            long e=strtol(tok, &end, 0);
            if(*end || e==LONG_MAX || e==LONG_MIN) {
                errlogPrintf("Unable to parse event spec '%s'\n", tok);
            } else if(e>255 || e<-255 || e==0) {
                errlogPrintf("Invalid event %ld\n", e);
            } else if(e>0) {
                card->specialSetMap(e, ActionEvtFwd, true);
            } else if(e<0) {
                card->specialSetMap(-e, ActionEvtFwd, false);
            }

        }
    }


    free(events);
} catch(std::exception& e) {
    errlogPrintf("Error: %s\n",e.what());
    free(events);
}
}

static const iocshArg mrmEvrForwardArg0 = { "Device",iocshArgString};
static const iocshArg mrmEvrForwardArg1 = { "Event spec string",iocshArgString};
static const iocshArg * const mrmEvrForwardArgs[2] =
{&mrmEvrForwardArg0,&mrmEvrForwardArg1};
static const iocshFuncDef mrmEvrForwardFuncDef =
    {"mrmEvrForward",2,mrmEvrForwardArgs};
static void mrmEvrForwardCallFunc(const iocshArgBuf *args)
{
    mrmEvrForward(args[0].sval,args[1].sval);
}

extern "C"
void
mrmEvrLoopback(const char* id, int rxLoopback, int txLoopback)
{
try {
    mrf::Object *obj=mrf::Object::getObject(id);
    if(!obj)
        throw std::runtime_error("Object not found");
    EVRMRM *card=dynamic_cast<EVRMRM*>(obj);
    if(!card){
        throw std::runtime_error("Not a MRM EVR");
    }

    epicsUInt32 control = NAT_READ32(card->base,Control);
    control &= ~(Control_txloop|Control_rxloop);
    if (rxLoopback) control |= Control_rxloop;
    if (txLoopback) control |= Control_txloop;
    NAT_WRITE32(card->base,Control, control);

} catch(std::exception& e) {
    errlogPrintf("Error: %s\n",e.what());
}
}

static const iocshArg mrmEvrLoopbackArg0 = { "Device",iocshArgString};
static const iocshArg mrmEvrLoopbackArg1 = { "RX-loopback",iocshArgInt};
static const iocshArg mrmEvrLoopbackArg2 = { "TX-loopback",iocshArgInt};
static const iocshArg * const mrmEvrLoopbackArgs[3] =
    {&mrmEvrLoopbackArg0,&mrmEvrLoopbackArg1,&mrmEvrLoopbackArg2};
static const iocshFuncDef mrmEvrLoopbackFuncDef =
    {"mrmEvrLoopback",3,mrmEvrLoopbackArgs};

static void mrmEvrLoopbackCallFunc(const iocshArgBuf *args)
{
    mrmEvrLoopback(args[0].sval,args[1].ival,args[2].ival);
}
static
bool mrmEvrAddressRangeCheck(size_t offset)
{
    /* Since we read words with size 4 bytes, this is maximum address */
    static const size_t maxAddr = EVR_REGMAP_SIZE - 4;

    if (offset > maxAddr){
        errlogPrintf("Offset outside EVR address range, size of EVR register map is 0x%x bytes.\n"
                     "Last address that can be addressed is 0x%x, "
                     "since we read words with size of 4 bytes.\n", EVR_REGMAP_SIZE, maxAddr);
        return false;
    }

    return true;
}

extern "C"
void mrmEvrRead(const char* id, size_t offset)
{
    mrf::Object *obj = mrf::Object::getObject(id);
    if (!obj)
        return;
    EVRMRM *card = dynamic_cast<EVRMRM*>(obj);
    if (!card){
        return;
    }
    if (!mrmEvrAddressRangeCheck(offset)){
        return;
    }

#ifdef _WIN32
    printf("0x%0Ix: 0x%08x\n", offset, nat_ioread32(card->base + offset));
#else
    printf("0x%0zx: 0x%08x\n", offset, nat_ioread32(card->base + offset));
#endif

}

extern "C"
void mrmEvrWrite(const char* id, size_t offset, epicsUInt32 value)
{
    mrf::Object *obj = mrf::Object::getObject(id);
    if (!obj)
        return;
    EVRMRM *card = dynamic_cast<EVRMRM*>(obj);
    if (!card){
        return;
    }
    if (!mrmEvrAddressRangeCheck(offset)){
        return;
    }

    mrmEvrRead(id, offset);
    nat_iowrite32(card->base + offset, value);
    mrmEvrRead(id, offset);
}

/********** Read device memory  *******/

static const iocshArg mrmEvrReadArg0 = { "Device", iocshArgString };
static const iocshArg mrmEvrReadArg1 = { "offset", iocshArgInt };

static const iocshArg * const mrmEvrReadArgs[2] = { &mrmEvrReadArg0, &mrmEvrReadArg1 };
static const iocshFuncDef mrmEvrReadFuncDef = { "mrmEvrRead", 2, mrmEvrReadArgs };


static void mrmEvrReadFunc(const iocshArgBuf *args) {
    epicsUInt32 offset = args[1].ival;
    //epicsUInt32 length = args[2].ival;
    mrmEvrRead(args[0].sval, offset);
}

/******************/

/********** Write device memory  *******/

static const iocshArg mrmEvrWriteArg0 = { "Device", iocshArgString };
static const iocshArg mrmEvrWriteArg1 = { "offset", iocshArgInt };
static const iocshArg mrmEvrWriteArg2 = { "value", iocshArgInt };

static const iocshArg * const mrmEvrWriteArgs[3] = { &mrmEvrWriteArg0, &mrmEvrWriteArg1, &mrmEvrWriteArg2 };
static const iocshFuncDef mrmEvrWriteFuncDef = { "mrmEvrWrite", 3, mrmEvrWriteArgs };


static void mrmEvrWriteFunc(const iocshArgBuf *args) {
    epicsUInt32 offset = args[1].ival;
    epicsUInt32 value = args[2].ival;

    mrmEvrWrite(args[0].sval, offset, value);
}

/******************/

/********** Create embedded EVR on EVG  *******/

static const iocshArg mrmEvrSetupEmbeddedArg0 = { "Device name", iocshArgString };
static const iocshArg mrmEvrSetupEmbeddedArg1 = { "Parent device name", iocshArgString };
static const iocshArg mrmEvrSetupEmbeddedArg2 = { "Type [upstream, downstream]", iocshArgString };

static const iocshArg * const mrmEvrSetupEmbeddedArgs[3] = { &mrmEvrSetupEmbeddedArg0, &mrmEvrSetupEmbeddedArg1, &mrmEvrSetupEmbeddedArg2 };
static const iocshFuncDef mrmEvrSetupEmbeddedFuncDef = { "mrmEvrSetupEmbedded", 3, mrmEvrSetupEmbeddedArgs };


static void mrmEvrSetupEmbeddedCallFunc(const iocshArgBuf *args) {
    char *evrName = args[0].sval;
    char *evgName = args[1].sval;
    evgMrm *evg;
    mrf::Object *obj;
    mrmDeviceInfo *deviceInfo;
    size_t memoryOffset;

    if(strcmp("upstream", args[2].sval) == 0 || strtol(args[2].sval, NULL, 10) != 0) {
        memoryOffset = 0x30000;
    }
    else if(strcmp("downstream", args[2].sval) == 0 || strtol(args[2].sval, NULL, 10) != 1) {
        memoryOffset = 0x20000;
    }
    else {
        errlogPrintf("Device type %s not supported. Choices are:\n - upstream or 0\n- downstream or 1\n", args[2].sval);
        return;
    }

    obj = mrf::Object::getObject(evgName);
    if(!obj) {
        errlogPrintf("Event generator %s does not exist\n", evgName);
        return;
    }
    evg = dynamic_cast<evgMrm*>(obj);
    if(!evg) {
        errlogPrintf("Device named %s is not an event generator\n", evgName);
        return;
    }

    obj = mrf::Object::getObject(evrName);
    if(obj) {
        errlogPrintf("Event receiver %s already exist\n", evrName);
        return;
    }

    deviceInfo = evg->getDeviceInfo();
    if(deviceInfo->getFirmwareId() != mrmDeviceInfo::firmwareId_delayCompensation) {
        errlogPrintf("%s: %s does not support embedded EVRs, because it does not have delay compensation firmware\n", evgName, deviceInfo->getDeviceDescription().c_str());
        return;
    }

    deviceInfo = new mrmDeviceInfo(evg->getRegAddr() + memoryOffset);
    deviceInfo->setEmbeddedFormFactor();
    checkVersion(deviceInfo);

    new EVRMRM(evrName, *deviceInfo, evg->getRegAddr() + memoryOffset, evg->getRegAddr());
}

/******************/

static
void mrmsetupreg()
{
    initHookRegister(&inithooks);
    iocshRegister(&mrmEvrSetupPCIFuncDef, mrmEvrSetupPCICallFunc);
    iocshRegister(&mrmEvrSetupVMEFuncDef, mrmEvrSetupVMECallFunc);
    iocshRegister(&mrmEvrSetupEmbeddedFuncDef, mrmEvrSetupEmbeddedCallFunc);
    iocshRegister(&mrmEvrDumpMapFuncDef, mrmEvrDumpMapCallFunc);
    iocshRegister(&mrmEvrForwardFuncDef, mrmEvrForwardCallFunc);
    iocshRegister(&mrmEvrLoopbackFuncDef, mrmEvrLoopbackCallFunc);
    iocshRegister(&mrmEvrWriteFuncDef, mrmEvrWriteFunc);
    iocshRegister(&mrmEvrReadFuncDef, mrmEvrReadFunc);
}


static
drvet drvEvrMrm = {
    2,
    (DRVSUPFUN)report,
    NULL
};
extern "C"{
epicsExportRegistrar(mrmsetupreg);
epicsExportAddress (drvet, drvEvrMrm);
}
