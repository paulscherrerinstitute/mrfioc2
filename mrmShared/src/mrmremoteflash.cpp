#include <stdio.h>
#include <string>

#include <epicsExport.h>
#include "mrmremoteflash.h"

extern "C" {
    #include "spi_flash.h"
}

void flash_thread(void* args){

    mrmRemoteFlash* parent = static_cast<mrmRemoteFlash*>(args);

    printf("\nMRF FLASH: Starting flash of: %s to card on offset 0x%x\n",parent->getFlashFilename().c_str(),parent->m_pReg);

//    sleep(10);
    spi_program_flash((void*)parent->m_pReg,parent->getFlashFilename().c_str());

    parent->m_flash_in_progress = false;
    printf("\nMRF FLASH: DONE!\n");
}


mrmRemoteFlash::mrmRemoteFlash(const std::string &name, volatile epicsUInt8 *pReg):
    mrf::ObjectInst<mrmRemoteFlash>(name),
    m_pReg(pReg),
    m_file_valid(false),
    m_flash_in_progress(false)
{

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
    return m_filename.copy(wf,l);
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
    //Only flash if valid file is specified
    if(!m_file_valid){
        throw std::runtime_error("Will not start flashing op. since file is not valid!");
    }

    //Check if flash is in progress
    if(m_flash_in_progress){
        //Check if thread is still alive
        if(!epicsThreadIsSuspended(m_flash_thread_id)){
            throw std::runtime_error("Flash thread still running!");
        }
    }

    m_flash_in_progress = true;
    m_flash_thread_id = epicsThreadCreate("MRF SPI FLASH",epicsThreadPriorityLow,epicsThreadGetStackSize(epicsThreadStackMedium),&flash_thread,this);

}

bool mrmRemoteFlash::flashInProgress() const
{
    return m_flash_in_progress;
}


bool mrmRemoteFlash::flashSuccess() const
{
    //TODO: When/if refractoring Jukkas code add support for flash result here
    return false;
}

#undef epicsExportSharedSymbols
#include "shareLib.h"
OBJECT_BEGIN(mrmRemoteFlash) {

    OBJECT_PROP1("InProgress",&mrmRemoteFlash::flashInProgress);
    OBJECT_PROP2("Flash", &mrmRemoteFlash::flashSuccess, &mrmRemoteFlash::startFlash);
    OBJECT_PROP2("Filename", &mrmRemoteFlash::getFlashFilenameWF, &mrmRemoteFlash::setFlashFilenameWF);


} OBJECT_END(mrmRemoteFlash)
