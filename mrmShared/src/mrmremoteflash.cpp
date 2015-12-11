#include <stdio.h>
#include <string>

#include "iocsh.h"
#include <epicsExport.h>
#include "mrmremoteflash.h"

const char *object_name = ":Flash";

void mrmRemoteFlash::flash_thread(void* args){
    tThreadArgs *threadArgs = static_cast<tThreadArgs*>(args);
    mrmRemoteFlash* parent = threadArgs->parent;

    parent->flash(threadArgs->filename.c_str());
    delete threadArgs;
}

void mrmRemoteFlash::read_thread(void* args){
    tThreadArgs *threadArgs = static_cast<tThreadArgs*>(args);
    mrmRemoteFlash* parent = threadArgs->parent;

    parent->read(threadArgs->filename.c_str());
    delete threadArgs;
}


mrmRemoteFlash::mrmRemoteFlash(const std::string &name, volatile epicsUInt8 *pReg, formFactor formFactor, mrmFlash &flash):
    mrf::ObjectInst<mrmRemoteFlash>(name+object_name),
    m_pReg(pReg),
    m_flash_in_progress(false),
    m_file_valid(false),
    m_flash_success(false),
    m_read_success(false),
    m_flash(flash)
{
    switch(formFactor){
        case formFactor_CPCI:
        case formFactor_CPCIFULL:
        case formFactor_PCIe:
            m_offset = m_flash.getSectorSize();
            m_supported = true;
        case formFactor_VME64:
            m_offset = 0;
            m_supported = true;
            break;
        default:
            m_offset = 0;
            m_supported = false;
    }
}


/**
 * @brief mrmRemoteFlash::setFlashFilename
 * @param filename
 *
 * Sets if file exists and TODO: add some other sanity check (perhaps naming convention?)
 */
void mrmRemoteFlash::setFlashFilename(std::string filename)
{
    printf("MRF FLASH: Checking file %s\n",filename.c_str());

    m_file_valid = false;

    //Check if file exists
    FILE* f = fopen(filename.c_str(),"rw");
    if(f==NULL){
        //Warning: on linux, this will not catch an attempt to open directory.
        m_filename="INVALID";
        throw std::runtime_error("Invalid flash file specified!");
    }

    fclose(f);
    m_file_valid = true;
    m_filename = filename;
}

std::string mrmRemoteFlash::getFlashFilename() const
{
    return m_filename;
}

/**
 * @brief mrmRemoteFlash::setFlashFilenameWF
 * @param wf
 * @param l
 *
 * waveform wrapper for set filename. This is need since EPICS string is limited to 40char
 */
void mrmRemoteFlash::setFlashFilenameWF(const char *wf, epicsUInt32 l)
{
    setFlashFilename(std::string(wf,l));
}

epicsUInt32 mrmRemoteFlash::getFlashFilenameWF(char *wf, epicsUInt32 l) const
{
    return (epicsUInt32)m_filename.copy(wf,l);
}

/**
 * @brief mrmRemoteFlash::startFlash
 * @param start
 *
 * Preform the actual flashing of the EVR/EVM. Function calls original Jukkas code
 * in a new thread (to avoid blocking CA thread for looooong time). Only one thread is allowed
 * at any given time. Note that there is no feedback wheter the flashing was succssful or not,
 * this and any other status outputs would require rewrite/modifications to original code.
 *
 * In majority of the cases you will want to use the IOCSH function instead...
 *
 */

void mrmRemoteFlash::startFlash(bool start)
{
    startFlash(getFlashFilename());
}

void mrmRemoteFlash::startFlash(std::string filename)
{
    tThreadArgs * args;

    //Only flash if valid file is specified
    if(!m_file_valid){
        throw std::runtime_error("Will not start flashing op. since file is not valid!");
    }

    //Check if flash is currently being accessed
    if(!flashInProgress()){
        //Check if thread is still alive
        if(!epicsThreadIsSuspended(m_flash_thread_id)){
            throw std::runtime_error("Flash thread still running!");
        }
    }

    m_flash_in_progress = true;
    args = new tThreadArgs;
    args->filename = filename;
    args->parent = this;
    m_flash_thread_id = epicsThreadCreate("MRF SPI FLASH", epicsThreadPriorityLow, epicsThreadGetStackSize(epicsThreadStackMedium), &mrmRemoteFlash::flash_thread, args);

}

void mrmRemoteFlash::flash(const char *bitfile) {
    try{
        printf("\nMRF FLASH: Starting flash of: %s to card on offset %zu\n",getFlashFilename().c_str(), m_offset);
        if(m_supported) {
            m_flash.flash(bitfile, m_offset);

            m_flash_in_progress = false;
            printf("\nMRF FLASH: DONE!\n");
            m_flash_success = true;
        }
        else {
            printf("\nMRF FLASH: This card does not support flashing. DONE!\n");
        }
    }
    catch(std::exception& ex) {
        //TODO make debug printouts
        printf("An error occured while flashing: %s\n", ex.what());
        m_flash_success = false;
    }
}

//TODO use the same file for reading and writing???
void mrmRemoteFlash::startRead(bool start)
{
    startRead(getFlashFilename());
}

void mrmRemoteFlash::startRead(std::string filename)
{
    tThreadArgs * args;

    //Only read if valid file is specified
    if(!m_file_valid){
        throw std::runtime_error("Will not start reading op. since file is not valid!");
    }

    //Check if flash is currently being accessed
    if(!flashInProgress()){
        //Check if thread is still alive
        if(!epicsThreadIsSuspended(m_flash_thread_id)){
            throw std::runtime_error("Flash thread still running!");
        }
    }

    m_flash_in_progress = true;
    args = new tThreadArgs;
    args->filename = filename;
    args->parent = this;
    m_flash_thread_id = epicsThreadCreate("MRF SPI READ", epicsThreadPriorityLow, epicsThreadGetStackSize(epicsThreadStackMedium), &mrmRemoteFlash::read_thread, args);

}

void mrmRemoteFlash::read(const char *bitfile)
{
    try{
        printf("\nMRF FLASH: Reading flash of %s card on offset %zu\n",getFlashFilename().c_str(), m_offset);
        if(m_supported) {
            m_flash.read(bitfile, m_offset);

            m_flash_in_progress = false;
            printf("\nMRF FLASH: READ DONE!\n");
            m_read_success = true;
        }
        else {
            printf("\nMRF FLASH: This card does not support flash access. DONE!\n");
        }
    }
    catch(std::exception& ex) {
        //TODO make debug printouts
        printf("An error occured while flashing: %s\n", ex.what());
        m_read_success = false;
    }
}

bool mrmRemoteFlash::flashInProgress() const
{
    return m_flash.flashBusy() || m_flash_in_progress;
}


bool mrmRemoteFlash::flashSuccess() const
{
    return m_flash_success;
}

bool mrmRemoteFlash::readSuccess() const
{
    return m_read_success;
}


OBJECT_BEGIN(mrmRemoteFlash) {

    OBJECT_PROP1("InProgress",&mrmRemoteFlash::flashInProgress);
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
    char* bitfile = args[1].sval;


    printf("Starting SPI read procedure for %s [%s]\n", device.c_str(), bitfile);
    device.append(object_name);

    mrmRemoteFlash* remoteFlash = dynamic_cast<mrmRemoteFlash*>(mrf::Object::getObject(device.c_str()));
    if(!remoteFlash){
        printf("Device <%s> with flash support does not exist!\n", args[0].sval);
        return;
    }

    remoteFlash->startRead(bitfile);
}


/******************/

/********** Write flash memory  *******/
static const iocshArg mrmRemoteFlashArg0_write = { "Device", iocshArgString };
static const iocshArg mrmRemoteFlashArg1_write = { "File", iocshArgString };

static const iocshArg * const mrmRemoteFlashArgs_write[2] = { &mrmRemoteFlashArg0_write, &mrmRemoteFlashArg1_write};
static const iocshFuncDef mrmRemoteFlashDef_write = { "mrmRemoteFlashWrite", 2, mrmRemoteFlashArgs_write};


static void mrmRemoteFlashFunc_write(const iocshArgBuf *args) {
    std::string device = args[0].sval;
    char* bitfile = args[1].sval;


    printf("Starting SPI flash procedure for %s [%s]\n", device.c_str(), bitfile);
    device.append(object_name);

    mrmRemoteFlash* remoteFlash = dynamic_cast<mrmRemoteFlash*>(mrf::Object::getObject(device.c_str()));
    if(!remoteFlash){
        printf("Device <%s> with flash support does not exist!\n", args[0].sval);
        return;
    }

    remoteFlash->startFlash(bitfile);
}


/******************/

extern "C" {
    static void mrmRemoteFlashRegistrar() {
        iocshRegister(&mrmRemoteFlashDef_read, mrmRemoteFlashFunc_read);
        iocshRegister(&mrmRemoteFlashDef_write, mrmRemoteFlashFunc_write);
    }

    epicsExportRegistrar(mrmRemoteFlashRegistrar);
}
