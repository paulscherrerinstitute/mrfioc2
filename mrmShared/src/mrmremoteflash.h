#ifndef MRMREMOTEFLASH_H
#define MRMREMOTEFLASH_H

#include <epicsTypes.h>
#include <epicsThread.h>
#include <epicsMutex.h>
#include "mrf/object.h"



class mrmRemoteFlash : public mrf::ObjectInst<mrmRemoteFlash>
{
public:
    mrmRemoteFlash(const std::string& name, volatile epicsUInt8* pReg);

    /* locking done internally */
    virtual void lock() const{}
    virtual void unlock() const{}

    void setFlashFilename(std::string filename);
    std::string getFlashFilename() const;

    void setFlashFilenameWF(const char* wf,epicsUInt32 l);
    epicsUInt32 getFlashFilenameWF(char* wf,epicsUInt32 l) const;

    void startFlash(bool start);
    bool flashInProgress() const;
    bool flashSuccess() const;

    //Used by flashing thread
    bool m_flash_in_progress;
    volatile epicsUInt8* const m_pReg; //card reg map

private:
    std::string m_filename;
    bool m_file_valid;

    epicsThreadId m_flash_thread_id;

};

#endif // MRMREMOTEFLASH_H
