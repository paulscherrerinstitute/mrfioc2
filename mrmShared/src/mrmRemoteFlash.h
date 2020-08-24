#ifndef MRMREMOTEFLASH_H
#define MRMREMOTEFLASH_H

#include <epicsTypes.h>
#include <epicsThread.h>
#include <epicsMutex.h>

#include "mrmFlash.h"
#include "mrmShared.h"
#include "mrf/object.h"

class mrmDeviceInfo;

class epicsShareClass mrmRemoteFlash : public mrf::ObjectInst<mrmRemoteFlash>
{
public:
    mrmRemoteFlash(const std::string& parentName, volatile epicsUInt8* parentBaseAddress, mrmDeviceInfo &deviceInfo, mrmFlash &flash);
    static const char *OBJECT_NAME;

    /* locking done internally */
    virtual void lock() const{}
    virtual void unlock() const{}

    /**
     * @brief setOffset allows to set the offset from the start of the flash chip memory where reading/writing begins. Setting wrong offset is very dangerous!
     * The offset is autodetected in the constructor based on device form factor. The autodetection can fail in rare cases where device firmware is not supported by this driver. This can mean that it does not have the correct form factor description written at the correct address. This is the only case where using this function should be considered.
     * @param offset is the offset from the beginning of the flash chip memory to write to / read from
     */
    void setOffset(size_t offset);

    /**
     * @brief setFlashFilename sets the file name for use in reading or flashing the chip.
     * @param filename the file name to set.
     */
    void setFlashFilename(std::string filename);

    /**
     * @brief getFlashFilename returns the currently set file name.
     * @return the currently set file name or an empty string if no file name was set yet.
     */
    std::string getFlashFilename() const;

    /**
     * @brief setFlashFilenameWF is a waveform wrapper for setting the file name. This is needed since EPICS string is limited to 40 chars.
     */
    void setFlashFilenameWF(const char* wf,epicsUInt32 l);

    /**
     * @brief getFlashFilenameWF is a waveform wrapper for getting the file name. This is needed since EPICS string is limited to 40 chars.
     */
    epicsUInt32 getFlashFilenameWF(char* wf,epicsUInt32 l) const;

    /**
     * @brief startFlash calls the overloaded 'startFlash()' function with the currently set file name.
     * @param start dummy parameter to satisfy mrfioc2 object model.
     */
    void startFlash(bool start);

    /**
     * @brief startRead calls the overloaded 'startRead()' function with the currently set file name.
     * @param start dummy parameter to satisfy mrfioc2 object model.
     */
    void startRead(bool start);

    /**
     * @brief startFlash starts the flashing procedure in a new thread. It throws an exception if flash access is not supported, flash chip is currently being used or the thread could not be created.
     * @param filename is the name of the file to read from.
     */
    void startFlash(std::string filename);

    /**
     * @brief startRead starts the reading procedure in a new thread. It throws an exception if flash access is not supported, flash chip is currently being used or the thread could not be created.
     * @param filename is the name of the file to write to.
     */
    void startRead(std::string filename);

    /**
     * @brief flashInProgress indicates if the flash chip is currently being accessed.
     * @return true if the flash is currently being accessed, false otherwise.
     */
    bool flashInProgress() const;

    /**
     * @brief flashSuccess indicates if the last flashing operation completed successfully.
     * @return true if the last flashing operation completed successfully, false otherwise.
     */
    bool flashSuccess() const;

    /**
     * @brief readSuccess indicates if the last read operation completed successfully.
     * @return true if the last read operation completed successfully, false otherwise.
     */
    bool readSuccess() const;

    /**
     * @brief isOffsetValid indicates if the offset from the start of the flash chip memory where reading/writing begins is correctly set.
     * @return true if flash access is supported on this device, false otherwise.
     */
    bool isOffsetValid() const;

    /**
     * @brief report prints basic flash chip information to iocsh
     */
    void report() const;


    // Used by flashing / reading thread
    /**
     * @brief  tThreadArgs is a struct with arguments passed to the thread.
     */
    typedef struct{
        std::string filename;
        mrmRemoteFlash *parent;
    } tThreadArgs;

    /**
     * @brief flash_thread is the thread in which flashing of the chip is done.
     * @param args user arguments (tThreadArgs)
     */
    static void flash_thread(void* args);

    /**
     * @brief read_thread is the thread in which reading of the chip is done.
     * @param args user arguments (tThreadArgs)
     */
    static void read_thread(void* args);

private:
    std::string m_filename;             // the name of the file to be written to the flash chip, or the destination file to be read from the flash chip when accessing the flash chip from EPICS records.
    volatile epicsUInt8* const m_base;  // base address of the EVR/EVG card
    bool m_flash_success;               // true when the last flashing operation completed successfully, false otherwise.
    bool m_read_success;                // true when the last reading operation completed successfully, false otherwise.
    size_t m_offset;                    // offset from the start of the flash chip memory where reading/writing begins.
    bool m_offsetValid;                 // true when the device offset was successfuly detected based on form factor, false otherwise.

    mrmFlash &m_flash;                  // reference to the mrmFlash class that is responsible for hardware access to the flash chip

    /**
     * @brief flash is the worker called by the 'flash_thread' that does the flashing.
     * @param bitfile is the file name to be written to the flash chip
     */
    void flash(const char *bitfile);

    /**
     * @brief read is the worker called by the 'read_thread' that does the reading of the flash chip.
     * @param bitfile is the destination file name where the content of the flash chip is written to.
     */
    void read(const char *bitfile);

};

#endif // MRMREMOTEFLASH_H
