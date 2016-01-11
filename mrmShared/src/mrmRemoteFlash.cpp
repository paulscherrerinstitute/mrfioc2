#include <stdio.h>
#include <string>

#include <errlog.h>
#include "iocsh.h"
#include <epicsExport.h>
#include "mrmRemoteFlash.h"

const char *object_name = ":Flash"; // appended to device name for use in mrfioc2 object model


mrmRemoteFlash::mrmRemoteFlash(const std::string &parentName, volatile epicsUInt8 *parentBaseAddress, formFactor formFactor, mrmFlash &flash):
    mrf::ObjectInst<mrmRemoteFlash>(parentName+object_name),
    m_base(parentBaseAddress),
    m_flash_success(false),
    m_read_success(false),
    m_flash(flash)
{
    try{
        m_flash.init(true);

        switch(formFactor){
            case formFactor_CPCI:
            case formFactor_CPCIFULL:
            case formFactor_PCIe:
                m_offset = m_flash.getSectorSize();
                m_supported = true;
                break;

            case formFactor_VME64:
                m_offset = 0;
                m_supported = true;
                break;

            default:
                errlogPrintf("Flash access: this form factor is not supported.\n");
                m_offset = 0;
                m_supported = false;
        }
    }
    catch(std::exception& ex) {
        errlogPrintf("Flash access: %s\n", ex.what());
        m_offset = 0;
        m_supported = false;
    }
}

void mrmRemoteFlash::setFlashFilename(std::string filename)
{
    m_filename = filename;
}

std::string mrmRemoteFlash::getFlashFilename() const
{
    return m_filename;
}

void mrmRemoteFlash::setFlashFilenameWF(const char *wf, epicsUInt32 l)
{
    setFlashFilename(std::string(wf,l));
}

epicsUInt32 mrmRemoteFlash::getFlashFilenameWF(char *wf, epicsUInt32 l) const
{
    return (epicsUInt32)m_filename.copy(wf,l);
}


/**
 * Flashing
 **/

void mrmRemoteFlash::startFlash(bool start)
{
    startFlash(getFlashFilename());
}

void mrmRemoteFlash::startFlash(std::string filename)
{
    tThreadArgs * args = NULL;
    epicsThreadId threadId;

    // Is flash access even supported?
    if(!m_supported) {
        throw std::runtime_error("This device does not support flash access");
    }

    // Check if flash is currently being accessed
    if(flashInProgress()) {
        throw std::runtime_error("Flash chip is already in use. Aborting.");
    }

    args = new tThreadArgs;
    args->filename = filename;
    args->parent = this;
    threadId = epicsThreadCreate("MRF SPI FLASH", epicsThreadPriorityLow, epicsThreadGetStackSize(epicsThreadStackMedium), &mrmRemoteFlash::flash_thread, args);

    if(!threadId) {
        if(args != NULL) delete args;
        throw std::runtime_error("Unable to create thread for flash access.\n");
    }
}

void mrmRemoteFlash::flash_thread(void* args){
    tThreadArgs *threadArgs = static_cast<tThreadArgs*>(args);
    mrmRemoteFlash* parent = threadArgs->parent;

    parent->flash(threadArgs->filename.c_str());
    delete threadArgs;
}

void mrmRemoteFlash::flash(const char *bitfile)
{
    try{
        infoPrintf(0, "Starting flash of %s to device. Using offset %zu [bytes]\n", bitfile, m_offset);
        m_flash.flash(bitfile, m_offset);
        infoPrintf(0, "Flashing procedure done.\n");
        m_flash_success = true;
    }
    catch(std::exception& ex) {
        errlogPrintf("An error occured while flashing: %s\n", ex.what());
        m_flash_success = false;
    }
}


/**
 * Reading flash memory
 **/

void mrmRemoteFlash::startRead(bool start)
{
    startRead(getFlashFilename());
}

void mrmRemoteFlash::startRead(std::string filename)
{
    tThreadArgs * args = NULL;
    epicsThreadId threadId;

    // Is flash access even supported?
    if(!m_supported) {
        throw std::runtime_error("This device does not support flash access");
    }

    // Check if flash is currently being accessed
    if(flashInProgress()) {
        throw std::runtime_error("Flash chip is already in use. Aborting.");
    }

    args = new tThreadArgs;
    args->filename = filename;
    args->parent = this;
    threadId = epicsThreadCreate("MRF SPI READ", epicsThreadPriorityLow, epicsThreadGetStackSize(epicsThreadStackMedium), &mrmRemoteFlash::read_thread, args);

    if(!threadId) {
        if(args != NULL) delete args;
        throw std::runtime_error("Unable to create thread for flash access.\n");
    }
}

void mrmRemoteFlash::read_thread(void* args){
    tThreadArgs *threadArgs = static_cast<tThreadArgs*>(args);
    mrmRemoteFlash* parent = threadArgs->parent;

    parent->read(threadArgs->filename.c_str());
    delete threadArgs;
}

void mrmRemoteFlash::read(const char *bitfile)
{
    try{
        infoPrintf(0, "Reading flash to %s. Starting on flash memory offset %zu [bytes]\n", bitfile, m_offset);
        m_flash.read(bitfile, m_offset);
        infoPrintf(0, "Reading procedure done.\n");
        m_read_success = true;
    }
    catch(std::exception& ex) {
        errlogPrintf("An error occured while accessing flash memory: %s\n", ex.what());
        m_read_success = false;
    }
}


/**
 * Flash access information
 **/

bool mrmRemoteFlash::flashInProgress() const
{
    return m_flash.flashBusy();
}

bool mrmRemoteFlash::flashSuccess() const
{
    return m_flash_success;
}

bool mrmRemoteFlash::readSuccess() const
{
    return m_read_success;
}

bool mrmRemoteFlash::isSupported() const
{
    return m_supported;
}

void mrmRemoteFlash::report() const
{
    if(!isSupported()) {
        printf("\tThis device does not support flash chip access\n");
    }
    else {
        m_flash.report();
        printf("\tFirst address for reading / writing is at offset: %zu [bytes]\n", m_offset);
        printf("\tFlash is currently ");
        if(!flashInProgress()) {
            printf("not ");
        }
        printf("being accessed.\n");

        printf("\tLast completed flash operation was ");
        if(!flashSuccess()) {
            printf("not ");
        }
        printf("successful.\n");

        printf("\tLast completed read operation was ");
        if(!readSuccess()) {
            printf("not ");
        }
        printf("successful.\n");
    }
}



/**
 * Construct mrfioc2 objects for linking to EPICS records
 **/

OBJECT_BEGIN(mrmRemoteFlash) {

    OBJECT_PROP1("InProgress",&mrmRemoteFlash::flashInProgress);
    OBJECT_PROP1("IsSupported",&mrmRemoteFlash::isSupported);
    OBJECT_PROP2("Flash", &mrmRemoteFlash::flashSuccess, &mrmRemoteFlash::startFlash);
    OBJECT_PROP2("Read", &mrmRemoteFlash::readSuccess, &mrmRemoteFlash::startRead);
    OBJECT_PROP2("Filename", &mrmRemoteFlash::getFlashFilenameWF, &mrmRemoteFlash::setFlashFilenameWF);

} OBJECT_END(mrmRemoteFlash)



/**
 * IOCSH functions
 **/

/********** Read flash memory  *******/
static const iocshArg mrmRemoteFlashArg0_read = { "Device", iocshArgString };
static const iocshArg mrmRemoteFlashArg1_read = { "File", iocshArgString };

static const iocshArg * const mrmRemoteFlashArgs_read[2] = { &mrmRemoteFlashArg0_read, &mrmRemoteFlashArg1_read};
static const iocshFuncDef mrmRemoteFlashDef_read = { "mrmRemoteFlashRead", 2, mrmRemoteFlashArgs_read};


static void mrmRemoteFlashFunc_read(const iocshArgBuf *args) {
    std::string device = args[0].sval;
    std::string bitfile = args[1].sval;


    printf("Starting flash read procedure for %s [%s]\n", device.c_str(), bitfile.c_str());
    device.append(object_name);

    mrmRemoteFlash* remoteFlash = dynamic_cast<mrmRemoteFlash*>(mrf::Object::getObject(device.c_str()));
    if(!remoteFlash){
        printf("Device <%s> with flash support does not exist!\n", args[0].sval);
        return;
    }

    try{
        remoteFlash->startRead(bitfile);
    }
    catch(std::exception& ex) {
        errlogPrintf("An error occured while starting read procedure: %s\n", ex.what());
    }
}


/******************/

/********** Write flash memory  *******/
static const iocshArg mrmRemoteFlashArg0_write = { "Device", iocshArgString };
static const iocshArg mrmRemoteFlashArg1_write = { "File", iocshArgString };

static const iocshArg * const mrmRemoteFlashArgs_write[2] = { &mrmRemoteFlashArg0_write, &mrmRemoteFlashArg1_write};
static const iocshFuncDef mrmRemoteFlashDef_write = { "mrmRemoteFlashWrite", 2, mrmRemoteFlashArgs_write};


static void mrmRemoteFlashFunc_write(const iocshArgBuf *args) {
    std::string device = args[0].sval;
    std::string bitfile = args[1].sval;


    printf("Starting flashing procedure for %s [%s]\n", device.c_str(), bitfile.c_str());
    device.append(object_name);

    mrmRemoteFlash* remoteFlash = dynamic_cast<mrmRemoteFlash*>(mrf::Object::getObject(device.c_str()));
    if(!remoteFlash){
        printf("Device <%s> with flash support does not exist!\n", args[0].sval);
        return;
    }

    try{
        remoteFlash->startFlash(bitfile);
    }
    catch(std::exception& ex) {
        errlogPrintf("An error occured while starting flash procedure: %s\n", ex.what());
    }
}


/******************/

/********** Report status for flash chip  *******/
static const iocshArg mrmRemoteFlashArg0_status = { "Device", iocshArgString };

static const iocshArg * const mrmRemoteFlashArgs_status[1] = { &mrmRemoteFlashArg0_status};
static const iocshFuncDef mrmRemoteFlashDef_status = { "mrmRemoteFlashStatus", 1, mrmRemoteFlashArgs_status};


static void mrmRemoteFlashFunc_status(const iocshArgBuf *args) {
    std::string device = args[0].sval;

    printf("Status report of %s flash chip\n", device.c_str());
    device.append(object_name);

    mrmRemoteFlash* remoteFlash = dynamic_cast<mrmRemoteFlash*>(mrf::Object::getObject(device.c_str()));
    if(!remoteFlash){
        printf("Device <%s> with flash support does not exist!\n", args[0].sval);
        return;
    }

    remoteFlash->report();
}


/******************/


extern "C" {
    static void mrmRemoteFlashRegistrar() {
        iocshRegister(&mrmRemoteFlashDef_read, mrmRemoteFlashFunc_read);
        iocshRegister(&mrmRemoteFlashDef_write, mrmRemoteFlashFunc_write);
        iocshRegister(&mrmRemoteFlashDef_status, mrmRemoteFlashFunc_status);
    }

    epicsExportRegistrar(mrmRemoteFlashRegistrar);
}
