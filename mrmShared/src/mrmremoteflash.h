/*
 * Windows
 * When mrmShared is compiled the class should be set as exported (included in dll).
 * But when for example evgMrm is compiled, class should be set as imported. This means that evgMrm uses the previously exported class from the mrmShared dll.
 */
#ifdef MRMREMOTEFLASH_H_LEVEL2
 #ifdef epicsExportSharedSymbols
  #define MRMREMOTEFLASH_H_epicsExportSharedSymbols
  #undef epicsExportSharedSymbols
  #include "shareLib.h"
 #endif
#endif

#ifndef MRMREMOTEFLASH_H
#define MRMREMOTEFLASH_H

#include "mrmFlash.h"
#include "mrmShared.h"

#include <epicsTypes.h>
#include <epicsThread.h>
#include <epicsMutex.h>
#include "mrf/object.h"

class epicsShareClass mrmRemoteFlash : public mrf::ObjectInst<mrmRemoteFlash>
{
public:
    mrmRemoteFlash(const std::string& name, volatile epicsUInt8* pReg, formFactor formFactor, mrmFlash &flash);

    /* locking done internally */
    virtual void lock() const{}
    virtual void unlock() const{}

    void setFlashFilename(std::string filename);
    std::string getFlashFilename() const;

    void setFlashFilenameWF(const char* wf,epicsUInt32 l);
    epicsUInt32 getFlashFilenameWF(char* wf,epicsUInt32 l) const;

    void startFlash(bool start);
    void startRead(bool start);
    void startFlash(std::string filename);
    void startRead(std::string filename);
    bool flashInProgress() const;
    bool flashSuccess() const;
    bool readSuccess() const;



    //Used by flashing thread
    typedef struct{
        std::string filename;
        mrmRemoteFlash *parent;
    } tThreadArgs;
    static void flash_thread(void* args);
    static void read_thread(void* args);

private:
    std::string m_filename;
    volatile epicsUInt8* const m_pReg; //card reg map
    bool m_flash_in_progress;
    bool m_file_valid;
    bool m_flash_success;
    bool m_read_success;
    bool m_supported;
    size_t m_offset;

    mrmFlash &m_flash;

    epicsThreadId m_flash_thread_id;


    void flash(const char *bitfile);
    void read(const char *bitfile);
    size_t getFlashOffset();
};

#endif // MRMREMOTEFLASH_H

#ifdef MRMREMOTEFLASH_H_epicsExportSharedSymbols
 #undef MRMREMOTEFLASH_H_LEVEL2
 #define epicsExportSharedSymbols
 #include "shareLib.h"
#endif
