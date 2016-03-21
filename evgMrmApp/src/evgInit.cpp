#include <stdlib.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>

#include <epicsExit.h>
#include <epicsThread.h>

#include <iocsh.h>
#include <drvSup.h>
#include <initHooks.h>
#include <errlog.h>

#include "mrf/object.h"

#include <devcsr.h>
/* DZ: Does Win32 have a problem with devCSRTestSlot()? */
#ifdef _WIN32
#define devCSRTestSlot(vmeEvgIDs,slot,info) (NULL)

#include <time.h>
#endif

#include <mrfcsr.h>
#include <mrfpci.h>
#include <mrfCommonIO.h>
#include <devLibPCI.h>
#include "plx9030.h"

#include <epicsExport.h>
#include "evgRegMap.h"

#include "mrmShared.h"
#include "evgInit.h"

/* Bit mask used to communicate which VME interrupt levels
 * are used.  Bits are set by mrmEvgSetupVME().  Levels are
 * enabled later during iocInit.
 */
static epicsUInt8 vme_level_mask = 0;

static const
struct VMECSRID vmeEvgIDs[] = {
    {MRF_VME_IEEE_OUI,MRF_VME_EVG_BID|MRF_SERIES_230, VMECSRANY},
    {MRF_VME_IEEE_OUI, MRF_VME_EVM_300, VMECSRANY},
    VMECSR_END
};

static bool
enableIRQ(mrf::Object* obj, void*) {
    evgMrm *evg=dynamic_cast<evgMrm*>(obj);
    if(!evg)
        return true;

    /**
     * Enable PCIe interrputs (1<<30)
     *
     * Change by: tslejko
     * Reason: Support for cPCI EVG
     */
    WRITE32(evg->getRegAddr(), IrqEnable,
             EVG_IRQ_PCIIE          | //PCIe interrupt enable,
             EVG_IRQ_ENABLE         |
             EVG_IRQ_EXT_INP        |
             EVG_IRQ_STOP_RAM(0)    |
             EVG_IRQ_STOP_RAM(1)    |
             EVG_IRQ_START_RAM(0)   |
             EVG_IRQ_START_RAM(1)
    );
//     WRITE32(pReg, IrqEnable,
//         EVG_IRQ_ENABLE        |
//         EVG_IRQ_STOP_RAM1     |
//         EVG_IRQ_STOP_RAM0     |
//         EVG_IRQ_START_RAM1    |
//         EVG_IRQ_START_RAM0    |
//         EVG_IRQ_EXT_INP       |
//         EVG_IRQ_DBUFF         |
//         EVG_IRQ_FIFO          |
//         EVG_IRQ_RXVIO
//    );

    return true;
}

static bool
disableIRQ(mrf::Object* obj, void*)
{
    evgMrm *evg=dynamic_cast<evgMrm*>(obj);
    if(!evg)
        return true;

    BITCLR32(evg->getRegAddr(), IrqEnable, EVG_IRQ_ENABLE);
    return true;
}

static void
evgShutdown(void*)
{
    mrf::Object::visitObjects(&disableIRQ,0);
}

static bool
startSFPUpdate(mrf::Object* obj, void*)
{
    evgMrm *evg=dynamic_cast<evgMrm*>(obj);
    if(!evg)
        return true;

    std::vector<SFP*>* sfp = evg->getSFP();
    for (size_t i=0; i<sfp->size(); i++) {
        sfp->at(i)->startUpdate();
    }

    return true;
}

static void
inithooks(initHookState state) {
    epicsUInt8 lvl;
    switch(state) {
        case initHookAfterInterruptAccept:
            epicsAtExit(&evgShutdown, NULL);
            mrf::Object::visitObjects(&enableIRQ, 0);
            for(lvl=1; lvl<=7; ++lvl) {
                if (vme_level_mask&(1<<(lvl-1))) {
                    if(devEnableInterruptLevelVME(lvl)) {
                        printf("Failed to enable interrupt level %d\n",lvl);
                        return;
                    }
                }
            }

        break;

    /*
     * Enable interrupts after IOC has been started (this is need for cPCI version)
     *
     * Change by: tslejko
     * Reason: cPCI EVG support
     */
    case initHookAtIocRun:
        epicsAtExit(&evgShutdown, NULL);
        mrf::Object::visitObjects(&enableIRQ, 0);
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

epicsUInt32 checkVersion(volatile epicsUInt8 *base, unsigned int required) {
#ifndef __linux__
    epicsUInt32 junk;
    if(devReadProbe(sizeof(junk), (volatile void*)(base+U32_FWVersion), (void*)&junk)) {
        throw std::runtime_error("Failed to read from MRM registers (but could read CSR registers)\n");
    }
#endif
    epicsUInt32 type, ver;
    epicsUInt32 v = READ32(base, FWVersion);

    epicsPrintf("FPGA version: %08x\n", v);

    type = v >> FWVersion_type_shift;

    if(type != 0x2){
        errlogPrintf("Found type %x which does not correspond to EVG type 0x2.\n", type);
        return 0;
    }

    ver = v & FWVersion_ver_mask;

    if(ver < required) {
        errlogPrintf("Firmware version >= %x is required got %x\n", required,ver);
        return 0;
    }

    return ver;
}

extern "C"
epicsStatus
mrmEvgSetupVME (
    const char* id,         // Card Identifier
    epicsInt32  slot,       // VME slot
    epicsUInt32 vmeAddress, // Desired VME address in A24 space
    epicsInt32  irqLevel,   // Desired interrupt level
    epicsInt32  irqVector,  // Desired interrupt vector number
    bool ignoreVersion)     // Ignore errors due to firmware checks
{
    volatile epicsUInt8* regCpuAddr = 0;
    volatile epicsUInt8* regCpuAddr2 = 0; //function 2 of regmap (fanout/concentrator specifics)
    struct VMECSRID info;
    deviceInfoT deviceInfo;

    info.board = 0; info.revision = 0; info.vendor = 0;

    deviceInfo.bus.busType = busType_vme;
    deviceInfo.bus.vme.slot = slot;
    deviceInfo.bus.vme.address = vmeAddress;
    deviceInfo.bus.vme.irqLevel = irqLevel;
    deviceInfo.bus.vme.irqVector = irqVector;
    deviceInfo.series = series_unknown;


    int status; // a variable to hold function return statuses

    try {
        if(mrf::Object::getObject(id)){
            errlogPrintf("ID %s already in use\n",id);
            return -1;
        }

        volatile unsigned char* csrCpuAddr; // csrCpuAddr is VME-CSR space CPU address for the board
        csrCpuAddr = //devCSRProbeSlot(slot);
                     devCSRTestSlot(vmeEvgIDs,slot,&info); //FIXME: add support for EVM id

        if(!csrCpuAddr) {
            errlogPrintf("No EVG in slot %d\n",slot);
            return -1;
        }

        epicsPrintf("##### Setting up MRF EVG in VME Slot %d #####\n",slot);
        epicsPrintf("Found Vendor: %08x\nBoard: %08x\nRevision: %08x\n",
                info.vendor, info.board, info.revision);

        epicsUInt32 xxx = CSRRead32(csrCpuAddr + CSR_FN_ADER(1));
        if(xxx)
            epicsPrintf("Warning: EVG not in power on state! (%08x)\n", xxx);

        /*Setting the base address of Register Map on VME Board (EVG)*/
        CSRSetBase(csrCpuAddr, 1, vmeAddress, VME_AM_STD_SUP_DATA);

        {
            epicsUInt32 temp=CSRRead32((csrCpuAddr) + CSR_FN_ADER(1));

            if(temp != CSRADER((epicsUInt32)vmeAddress,VME_AM_STD_SUP_DATA)) {
                errlogPrintf("Failed to set CSR Base address in ADER1.  Check VME bus and card firmware version.\n");
                return -1;
            }
        }

        /* Create a static string for the card description (needed by vxWorks) */
        char *Description = allocSNPrintf(40, "EVG-%d '%s' slot %d",
                                          info.board & MRF_BID_SERIES_MASK,
                                          id, slot);

        /*Register VME address and get corresponding CPU address */
        status = devRegisterAddress (
            Description,                           // Event Generator card description
            atVMEA24,                              // A24 Address space
            vmeAddress,                            // Physical address of register space
            EVG_REGMAP_SIZE,                       // Size of card's register space
            (volatile void **)(void *)&regCpuAddr  // Local address of card's register map
        );


        if(status) {
            errlogPrintf("Failed to map VME address %08x\n", vmeAddress);
            return -1;
        }


        epicsUInt32 version = checkVersion(regCpuAddr, 0x3);
        epicsPrintf("Firmware version: %08x\n", version);

        if(version == 0){
            if(ignoreVersion) {
                epicsPrintf("Ignoring version error.\n");
            }
            else {
                return -1;
            }
        }


        /* Set the base address of Register Map for function 2, if we have the right firmware version  */
        if(version >= EVG_FCT_MIN_FIRMWARE) {
            deviceInfo.series = series_300;
            CSRSetBase(csrCpuAddr, 2, vmeAddress+EVG_REGMAP_SIZE, VME_AM_STD_SUP_DATA);
            {
                epicsUInt32 temp=CSRRead32((csrCpuAddr) + CSR_FN_ADER(2));

                if(temp != CSRADER((epicsUInt32)vmeAddress+EVG_REGMAP_SIZE,VME_AM_STD_SUP_DATA)) {
                    epicsPrintf("Failed to set CSR Base address in ADER2 for FCT register mapping.  Check VME bus and card firmware version.\n");
                    return -1;
                }
            }

            /* Create a static string for the card description (needed by vxWorks) */
            char *Description = allocSNPrintf(40, "EVG-%d FOUT'%s' slot %d",
                                              info.board & MRF_BID_SERIES_MASK,
                                              id, slot);

             status = devRegisterAddress (
                Description,                           // Event Generator card description
                atVMEA24,                              // A24 Address space
                vmeAddress+EVG_REGMAP_SIZE,            // Physical address of register space
                EVG_REGMAP_SIZE*2,                     // Size of card's register space
                (volatile void **)(void *)&regCpuAddr2 // Local address of card's register map
            );

            if(status) {
                errlogPrintf("Failed to map VME address %08x for FCT mapping\n", vmeAddress);
                return -1;
            }
        }
        else {
            deviceInfo.series = series_230;
        }

        evgMrm* evg = new evgMrm(id, deviceInfo, regCpuAddr, regCpuAddr2, NULL);

        if(irqLevel > 0 && irqVector >= 0) {
            /*Configure the Interrupt level and vector on the EVG board*/
            CSRWrite8(csrCpuAddr + UCSR_DEFAULT_OFFSET + UCSR_IRQ_LEVEL, irqLevel&0x7);
            CSRWrite8(csrCpuAddr + UCSR_DEFAULT_OFFSET + UCSR_IRQ_VECTOR, irqVector&0xff);

            epicsPrintf("IRQ Level: %d\nIRQ Vector: %d\n",
                CSRRead8(csrCpuAddr + UCSR_DEFAULT_OFFSET + UCSR_IRQ_LEVEL),
                CSRRead8(csrCpuAddr + UCSR_DEFAULT_OFFSET + UCSR_IRQ_VECTOR)
            );


            epicsPrintf("csrCpuAddr : %p\nregCpuAddr : %p\nreCpuAddr2 : %p\n",csrCpuAddr, regCpuAddr, regCpuAddr2);

            /*Disable the interrupts and enable them at the end of iocInit via initHooks*/
            WRITE32(regCpuAddr, IrqFlag, READ32(regCpuAddr, IrqFlag));
            WRITE32(regCpuAddr, IrqEnable, 0);

            // VME IRQ level will be enabled later during iocInit()
            vme_level_mask |= 1 << ((irqLevel&0x7)-1);

            /*Connect Interrupt handler to vector*/
            if(devConnectInterruptVME(irqVector & 0xff, &evgMrm::isr_vme, evg)){
                errlogPrintf("ERROR:Failed to connect VME IRQ vector %d\n"
                                                         ,irqVector&0xff);
                delete evg;
                return -1;
            }
        }
    } catch(std::exception& e) {
        errlogPrintf("Error: %s\n",e.what());
        errlogFlush();
        return -1;
    }
    errlogFlush();
    return 0;

} //mrmEvgSetupVME

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

/**
 * This function and definitions add support for cPCI EVG.
 * Function works similiar to that of EVR device support and reilies on devLib2
 * + mrf_uio kernel module to map EVG address space.
 *
 * Change by: tslejko
 * Reason: cPCI EVG support
 */

static const epicsPCIID
mrmevgs[] = {
        DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9030, PCI_VENDOR_ID_PLX,PCI_DEVICE_ID_MRF_PXIEVG230, PCI_VENDOR_ID_MRF),
        DEVPCI_END };

extern "C"
epicsStatus
mrmEvgSetupPCI (
        const char* id,         // Card Identifier
        int b,       			// Bus number
        int d, 					// Device number
        int f,   				// Function number
        bool ignoreVersion)     // Ignore errors due to kernel module and firmware version checks
{
    deviceInfoT deviceInfo;

    deviceInfo.bus.busType = busType_pci;
    deviceInfo.bus.pci.bus = b;
    deviceInfo.bus.pci.device = d;
    deviceInfo.bus.pci.function = f;
    deviceInfo.series = series_unknown;

    try {
        if (mrf::Object::getObject(id)) {
            errlogPrintf("ID %s already in use\n", id);
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

        /* Find PCI device from devLib2 */
        const epicsPCIDevice *cur = 0;
        if (devPCIFindBDF(mrmevgs, b, d, f, &cur, 0)) {
            errlogPrintf("PCI Device not found on %x:%x.%x\n", b, d, f);
            return -1;
        }

        epicsPrintf("Device %s  %x:%x.%x\n", id, cur->bus, cur->device, cur->function);
        epicsPrintf("Using IRQ %u\n", cur->irq);


        /* MMap BAR0(plx) and BAR2(EVG)*/
        volatile epicsUInt8 *BAR_plx, *BAR_evg; // base addressed for plx/evg bars

        if (devPCIToLocalAddr(cur, 0, (volatile void**) (void *) &BAR_plx, 0)
                || devPCIToLocalAddr(cur, 2, (volatile void**) (void *) &BAR_evg, 0)) {
            errlogPrintf("Failed to map BARs 0 and 2\n");
            return -1;
        }

        if (!BAR_plx || !BAR_evg) {
            errlogPrintf("BARs mapped to zero? (%08lx,%08lx)\n",
                    (unsigned long) BAR_plx, (unsigned long) BAR_evg);
            return -1;
        }

        //Set LE mode on PLX bridge
        //TODO: this limits cPCI EVG device support to LE architectures
        //			At this point in time we do not have any BE PCI systems at hand so this is left as
        //			unsported until we HW to test it on...

        epicsUInt32 plxCtrl = LE_READ32(BAR_plx,LAS0BRD);
        plxCtrl = plxCtrl & ~LAS0BRD_ENDIAN;
        LE_WRITE32(BAR_plx,LAS0BRD,plxCtrl);


        epicsUInt32 version = checkVersion(BAR_evg, 0x3);
        epicsPrintf("Firmware version: %08x\n", version);

        if(version == 0) {
            if(ignoreVersion) {
                epicsPrintf("Ignoring version error.\n");
            }
            else {
                return -1;
            }
        }

        evgMrm* evg = new evgMrm(id, deviceInfo, BAR_evg, 0, cur);

        evg->getSeqRamMgr()->getSeqRam(0)->disable();
        evg->getSeqRamMgr()->getSeqRam(1)->disable();


        /*Disable the interrupts and enable them at the end of iocInit via initHooks*/
        WRITE32(BAR_evg, IrqFlag, READ32(BAR_evg, IrqFlag));
        WRITE32(BAR_evg, IrqEnable, 0);

        /*
         * Enable active high interrupt1 through the PLX to the PCI bus.
         */
//		LE_WRITE16(BAR_plx, INTCSR,	INTCSR_INT1_Enable| INTCSR_INT1_Polarity| INTCSR_PCI_Enable);
        if(ignoreVersion){
            epicsPrintf("Not enabling interrupts.\n");
        }
        else {
            if(devPCIEnableInterrupt(cur)) {
                errlogPrintf("Failed to enable interrupt\n");
                return -1;
            }
        }

#ifdef __linux__
        evg->isrLinuxPvt = (void*) cur;
#endif

        /*Connect Interrupt handler to isr thread*/
        if(ignoreVersion){
            epicsPrintf("Not connecting interrupts.\n");
        }
        else {
            if (devPCIConnectInterrupt(cur, &evgMrm::isr_pci, (void*) evg, 0)) {//devConnectInterruptVME(irqVector & 0xff, &evgMrm::isr, evg)){
                errlogPrintf("ERROR:Failed to connect PCI interrupt\n");
                delete evg;
                return -1;
            } else {
                epicsPrintf("PCI interrupt connected!\n");
            }
        }
    } catch (std::exception& e) {
        errlogPrintf("Error: %s\n", e.what());
        errlogFlush();
        return -1;
    }

    return 0;
} //mrmEvgSetupPCI

#ifndef _WIN32
/*
 * This function spawns additional thread that emulate PPS input. Function is used for
 * testing of timestamping functionality... DO NOT USE IN PRODUCTION!!!!!
 *
 * Change by: tslejko
 * Reason: testing utilities
 */
void mrmEvgSoftTime(void* pvt) {
    evgMrm* evg = static_cast<evgMrm*>(pvt);

    if (!evg) {
        errlogPrintf("mrmEvgSoftTimestamp: Could not find EVG!\n");
    }

    while (1) {
        epicsUInt32 data = evg->sendTimestamp();
        if (!data){
            errlogPrintf("mrmEvgSoftTimestamp: Could not retrive timestamp...\n");
            epicsThreadSleep(1);
            continue;
        }

        //Send out event reset
        evg->getSoftEvt()->setEvtCode(MRF_EVENT_TS_COUNTER_RST);

        //Clock out data...
        for (int i = 0; i < 32; data <<= 1, i++) {
            if (data & 0x80000000)
                evg->getSoftEvt()->setEvtCode(MRF_EVENT_TS_SHIFT_1);
            else
                evg->getSoftEvt()->setEvtCode(MRF_EVENT_TS_SHIFT_0);
        }

        struct timespec sleep_until_t;

        clock_gettime(CLOCK_REALTIME,&sleep_until_t); //Get current time
        /* Sleep until next full second */
        sleep_until_t.tv_nsec=0;
        sleep_until_t.tv_sec++;

        clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&sleep_until_t,0);


//		sleep(1);
    }
}
#else
void mrmEvgSoftTime(void* pvt) {}
#endif

/*
 *    EPICS Registrar Function for this Module
 */
static const iocshArg mrmEvgSoftTimeArg0 = { "Device", iocshArgString};
static const iocshArg * const mrmEvgSoftTimeArgs[1] = { &mrmEvgSoftTimeArg0};
static const iocshFuncDef mrmEvgSoftTimeFuncDef = { "mrmEvgSoftTime", 1, mrmEvgSoftTimeArgs };

static void mrmEvgSoftTimeFunc(const iocshArgBuf *args) {
    epicsPrintf("Starting EVG Software based time provider...\n");

    if(!args[0].sval) return;

    evgMrm* evg = dynamic_cast<evgMrm*>(mrf::Object::getObject(args[0].sval));
    if(!evg){
        errlogPrintf("EVG <%s> does not exist!\n",args[0].sval);
    }

    epicsThreadCreate("EVG_TimestampTestThread",90, epicsThreadStackSmall,mrmEvgSoftTime,static_cast<void*>(evg));
}


static const iocshArg mrmEvgSetupVMEArg0 = { "Device", iocshArgString };
static const iocshArg mrmEvgSetupVMEArg1 = { "Slot number", iocshArgInt };
static const iocshArg mrmEvgSetupVMEArg2 = { "A24 base address", iocshArgInt };
static const iocshArg mrmEvgSetupVMEArg3 = { "IRQ Level 1-7 (0 - disable)", iocshArgInt };
static const iocshArg mrmEvgSetupVMEArg4 = { "IRQ Vector 0-255", iocshArgInt };
static const iocshArg mrmEvgSetupVMEArg5 = { "'Ignore version error'", iocshArgArgv};

static const iocshArg * const mrmEvgSetupVMEArgs[6] = { &mrmEvgSetupVMEArg0,
                                                        &mrmEvgSetupVMEArg1,
                                                        &mrmEvgSetupVMEArg2,
                                                        &mrmEvgSetupVMEArg3,
                                                        &mrmEvgSetupVMEArg4,
                                                        &mrmEvgSetupVMEArg5 };

static const iocshFuncDef mrmEvgSetupVMEFuncDef = { "mrmEvgSetupVME", 6,
        mrmEvgSetupVMEArgs };

static void
mrmEvgSetupVMECallFunc(const iocshArgBuf *args) {
    // check the 'ignore' parameter
    if(args[5].aval.ac > 1 && (strcmp("true", args[5].aval.av[1]) == 0 ||
                               strtol(args[5].aval.av[1], NULL, 10) != 0)){
        mrmEvgSetupVME(args[0].sval,
                       args[1].ival,
                       args[2].ival,
                       args[3].ival,
                       args[4].ival,
                       true);
    }
    else {
        mrmEvgSetupVME(args[0].sval,
                       args[1].ival,
                       args[2].ival,
                       args[3].ival,
                       args[4].ival,
                       false);
    }
}

static const iocshArg mrmEvgSetupPCIArg0 = { "Device", iocshArgString };
static const iocshArg mrmEvgSetupPCIArg1 = { "Bus number", iocshArgInt };
static const iocshArg mrmEvgSetupPCIArg2 = { "Device number", iocshArgInt };
static const iocshArg mrmEvgSetupPCIArg3 = { "Function number", iocshArgInt };
static const iocshArg mrmEvgSetupPCIArg4 = { "'Ignore version error'", iocshArgArgv};

static const iocshArg * const mrmEvgSetupPCIArgs[5] = { &mrmEvgSetupPCIArg0,
        &mrmEvgSetupPCIArg1, &mrmEvgSetupPCIArg2, &mrmEvgSetupPCIArg3, &mrmEvgSetupPCIArg4 };

static const iocshFuncDef mrmEvgSetupPCIFuncDef = { "mrmEvgSetupPCI", 5,
        mrmEvgSetupPCIArgs };

static void mrmEvgSetupPCICallFunc(const iocshArgBuf *args) {
    // check the 'ignore' parameter
    if(args[4].aval.ac > 1 && (strcmp("true", args[4].aval.av[1]) == 0 ||
                               strtol(args[4].aval.av[1], NULL, 10) != 0)){
        mrmEvgSetupPCI(args[0].sval, args[1].ival, args[2].ival, args[3].ival, true);
    }
    else{
        mrmEvgSetupPCI(args[0].sval, args[1].ival, args[2].ival, args[3].ival, false);
    }
}


extern "C"{
static void evgMrmRegistrar() {
    initHookRegister(&inithooks);
    iocshRegister(&mrmEvgSetupVMEFuncDef, mrmEvgSetupVMECallFunc);
    iocshRegister(&mrmEvgSetupPCIFuncDef, mrmEvgSetupPCICallFunc);
    iocshRegister(&mrmEvgSoftTimeFuncDef, mrmEvgSoftTimeFunc);
}

epicsExportRegistrar(evgMrmRegistrar);
}

/*
 * EPICS Driver Support for this module
 */
static const
struct printreg {
    char label[18];
    epicsUInt32 offset;
    int rsize;
} printreg[] = {
#define REGINFO(label, name, size) {label, U##size##_##name, size}
REGINFO("Status",           Status,           32),
REGINFO("Control",          Control,          32),
REGINFO("IrqFlag",          IrqFlag,          32),
REGINFO("IrqEnable",        IrqEnable,        32),
REGINFO("AcTrigControl",    AcTrigControl,    32),
REGINFO("AcTrigEvtMap",     AcTrigEvtMap,      8),
REGINFO("SwEventControl",   SwEventControl,    8),
REGINFO("SwEventCode",      SwEventCode,       8),
REGINFO("DataTxCtrlEvg",    DataTxCtrlEvg,    32),
REGINFO("DBusSrc",          DBusSrc,          32),
REGINFO("FPGAVersion",      FWVersion,        32),
REGINFO("uSecDiv",          uSecDiv,          16),
REGINFO("ClockSource",      ClockSource,       8),
REGINFO("RfDiv",            RfDiv,             8),
REGINFO("ClockStatus",      ClockStatus,      16),
REGINFO("SeqControl(0)",    SeqControl(0),    32),
REGINFO("SeqControl(1)",    SeqControl(1),    32),
REGINFO("FracSynthWord",    FracSynthWord,    32),
REGINFO("TrigEventCtrl(0)", TrigEventCtrl(0), 32),
REGINFO("TrigEventCtrl(1)", TrigEventCtrl(1), 32),
REGINFO("TrigEventCtrl(2)", TrigEventCtrl(2), 32),
REGINFO("TrigEventCtrl(3)", TrigEventCtrl(3), 32),
REGINFO("TrigEventCtrl(4)", TrigEventCtrl(4), 32),
REGINFO("TrigEventCtrl(5)", TrigEventCtrl(5), 32),
REGINFO("TrigEventCtrl(6)", TrigEventCtrl(6), 32),
REGINFO("TrigEventCtrl(7)", TrigEventCtrl(7), 32),
REGINFO("MuxControl(0)",    MuxControl(0),    32),
REGINFO("MuxPrescaler(0)",  MuxPrescaler(0),  32),
REGINFO("MuxControl(1)",    MuxControl(1),    32),
REGINFO("MuxPrescaler(1)",  MuxPrescaler(1),  32),
REGINFO("MuxControl(2)",    MuxControl(2),    32),
REGINFO("MuxPrescaler(2)",  MuxPrescaler(2),  32),
REGINFO("MuxControl(3)",    MuxControl(3),    32),
REGINFO("MuxPrescaler(3)",  MuxPrescaler(3),  32),
REGINFO("MuxControl(4)",    MuxControl(4),    32),
REGINFO("MuxPrescaler(4)",  MuxPrescaler(4),  32),
REGINFO("MuxControl(5)",    MuxControl(5),    32),
REGINFO("MuxPrescaler(5)",  MuxPrescaler(5),  32),
REGINFO("MuxControl(6)",    MuxControl(6),    32),
REGINFO("MuxPrescaler(6)",  MuxPrescaler(6),  32),
REGINFO("MuxControl(7)",    MuxControl(7),    32),
REGINFO("MuxPrescaler(7)",  MuxPrescaler(7),  32),
REGINFO("FrontOutMap(0)",   FrontOutMap(0),   16),
REGINFO("FrontInMap(0)",    FrontInMap(0),    32),
REGINFO("FrontInMap(1)",    FrontInMap(1),    32),
REGINFO("UnivInMap(0)",     UnivInMap(0),     32),
REGINFO("UnivInMap(1)",     UnivInMap(1),     32),
REGINFO("RearInMap(12)",    RearInMap(12),    32),
REGINFO("RearInMap(13)",    RearInMap(13),    32),
REGINFO("RearInMap(14)",    RearInMap(14),    32),
REGINFO("RearInMap(15)",    RearInMap(15),    32),
REGINFO("DataBuffer(0)",    DataBuffer(0),     8),
REGINFO("DataBuffer(1)",    DataBuffer(1),     8),
REGINFO("DataBuffer(2)",    DataBuffer(2),     8),
REGINFO("DataBuffer(3)",    DataBuffer(3),     8),
REGINFO("DataBuffer(4)",    DataBuffer(4),     8),
REGINFO("DataBuffer(5)",    DataBuffer(5),     8),
REGINFO("SeqRamTS(0,0)",    SeqRamTS(0,0),    32),
REGINFO("SeqRamTS(0,1)",    SeqRamTS(0,1),    32),
REGINFO("SeqRamTS(0,2)",    SeqRamTS(0,2),    32),
REGINFO("SeqRamTS(0,3)",    SeqRamTS(0,3),    32),
REGINFO("SeqRamTS(0,4)",    SeqRamTS(0,4),    32),
REGINFO("SeqRamEvent(0,0)", SeqRamEvent(0,0),  8),
REGINFO("SeqRamEvent(0,1)", SeqRamEvent(0,1),  8),
REGINFO("SeqRamEvent(0,2)", SeqRamEvent(0,2),  8),
REGINFO("SeqRamEvent(0,3)", SeqRamEvent(0,3),  8),
REGINFO("SeqRamEvent(0,4)", SeqRamEvent(0,4),  8),
REGINFO("SeqRamTS(1,0)",    SeqRamTS(1,0),    32),
REGINFO("SeqRamTS(1,1)",    SeqRamTS(1,1),    32),
REGINFO("SeqRamTS(1,2)",    SeqRamTS(1,2),    32),
REGINFO("SeqRamTS(1,3)",    SeqRamTS(1,3),    32),
REGINFO("SeqRamTS(1,4)",    SeqRamTS(1,4),    32),
REGINFO("SeqRamEvent(1,0)", SeqRamEvent(1,0),  8),
REGINFO("SeqRamEvent(1,1)", SeqRamEvent(1,1),  8),
REGINFO("SeqRamEvent(1,2)", SeqRamEvent(1,2),  8),
REGINFO("SeqRamEvent(1,3)", SeqRamEvent(1,3),  8),
REGINFO("SeqRamEvent(1,4)", SeqRamEvent(1,4),  8),
#undef REGINFO
};


static void
printregisters(volatile epicsUInt8 *evg) {
    size_t reg;
    epicsPrintf("\n--- Register Dump @%p ---\n", evg);

    for(reg=0; reg<NELEMENTS(printreg); reg++){
        switch(printreg[reg].rsize){
            case 8:
                epicsPrintf("%16s: %02x\n", printreg[reg].label,
                                       ioread8(evg+printreg[reg].offset));
                break;
            case 16:
                epicsPrintf("%16s: %04x\n", printreg[reg].label,
                                       nat_ioread16(evg+printreg[reg].offset));
                break;
            case 32:
                epicsPrintf("%16s: %08x\n", printreg[reg].label,
                                       nat_ioread32(evg+printreg[reg].offset));
                break;
        }
    }
}

static bool
reportCard(mrf::Object* obj, void* arg) {
    // this function is called by Object::visitObjects
    // it must return 'true' in order for the Object::visitObjects to continue searching for objects.
    // if false is returned, Object::visitObjects stops

    int *level=(int*)arg;
    evgMrm *evg=dynamic_cast<evgMrm*>(obj);
    if(!evg){
        return true;
    }

    epicsPrintf("EVG: %s     \n", evg->getId().c_str());
    epicsPrintf("\tFPGA Version: %08x (firmware: %x)\n", evg->getFwVersion(), evg->getFwVersionID());
    epicsPrintf("\tForm factor: %s\n", evg->getFormFactorStr().c_str());

    bus_configuration *bus = evg->getBusConfiguration();
    if(bus->busType == busType_vme){
        struct VMECSRID vmeDev;
        vmeDev.board = 0; vmeDev.revision = 0; vmeDev.vendor = 0;
        volatile unsigned char* csrAddr = devCSRTestSlot(vmeEvgIDs, bus->vme.slot, &vmeDev);
        if(csrAddr){
            epicsUInt32 ader = CSRRead32(csrAddr + CSR_FN_ADER(1));
            epicsPrintf("\tVME configured slot: %d\n", bus->vme.slot);
            epicsPrintf("\tVME configured A24 address 0x%08x\n", bus->vme.address);
            epicsPrintf("\tVME ADER: base address=0x%x\taddress modifier=0x%x\n", ader>>8, (ader&0xFF)>>2);
            epicsPrintf("\tVME IRQ Level %x (configured to %x)\n", CSRRead8(csrAddr + UCSR_DEFAULT_OFFSET + UCSR_IRQ_LEVEL), bus->vme.irqLevel);
            epicsPrintf("\tVME IRQ Vector %x (configured to %x)\n", CSRRead8(csrAddr + UCSR_DEFAULT_OFFSET + UCSR_IRQ_VECTOR), bus->vme.irqVector);
            if(*level>1) epicsPrintf("\tVME card vendor: 0x%08x\n", vmeDev.vendor);
            if(*level>1) epicsPrintf("\tVME card board: 0x%08x\n", vmeDev.board);
            if(*level>1) epicsPrintf("\tVME card revision: 0x%08x\n", vmeDev.revision);
            if(*level>1) epicsPrintf("\tVME CSR address: %p\n", csrAddr);
        }else{
            epicsPrintf("\tCard not detected in configured slot %d\n", bus->vme.slot);
        }
    }
    else if(bus->busType == busType_pci){
        const epicsPCIDevice *pciDev;
        if(!devPCIFindBDF(mrmevgs, bus->pci.bus, bus->pci.device, bus->pci.function, &pciDev, 0)){
            epicsPrintf("\tPCI configured bus: 0x%08x\n", bus->pci.bus);
            epicsPrintf("\tPCI configured device: 0x%08x\n", bus->pci.device);
            epicsPrintf("\tPCI configured function: 0x%08x\n", bus->pci.function);
            epicsPrintf("\tPCI IRQ: %u\n", pciDev->irq);
            if(*level>1) epicsPrintf("\tPCI class: 0x%08x, revision: 0x%x, sub device: 0x%x, sub vendor: 0x%x\n", pciDev->id.pci_class, pciDev->id.revision, pciDev->id.sub_device, pciDev->id.sub_vendor);

        }else{
            epicsPrintf("\tPCI Device not found\n");
        }
    }else{
        epicsPrintf("\tUnknown bus type\n");
    }

    evg->show(*level);

    if(*level >= 2)
        printregisters(evg->getRegAddr());

    epicsPrintf("\n");
    return true;
}

static long
report(int level) {
    epicsPrintf("===  Begin MRF EVG support   ===\n");
    mrf::Object::visitObjects(&reportCard, (void*)&level);
    epicsPrintf("===   End MRF EVG support    ===\n");
    return 0;
}
extern "C"{
static
drvet drvEvgMrm = {
    2,
    (DRVSUPFUN)report,
    NULL
};
epicsExportAddress (drvet, drvEvgMrm);
}
