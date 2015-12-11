#ifndef MRMFLASH_H
#define MRMFLASH_H

#include <string.h>

#include <epicsMutex.h>
#include <epicsTypes.h>
#include <epicsThread.h>

class epicsShareClass mrmFlash
{
public:
    mrmFlash(//const char *parentName,
             volatile epicsUInt8 *parentBaseAddress);
    mrmFlash(//const char *parentName,
             volatile epicsUInt8 *parentBaseAddress,
             size_t pageSize, size_t sectorSize, size_t memorySize);


    /*typedef void (*flashCallback)( std::string reason, void* args);
    typedef void (*readCallback)(epicsUInt8 *data, std::string reason, void* args);
    static void readThread(void *args);

    typedef struct{
        epicsUInt8 *data;
        size_t offset;
        size_t size;
        std::string reason;
        readCallback callback;
        mrmFlash *parent;
        void* args;
    } tReadThreadArgs;
    static mrmFlash* getFlashInstanceFromDevice(const char *device);*/



    void flash(const char *bitfile, size_t offset);
    void read(const char *bitfile, size_t offset);
    void read(epicsUInt8 * data, size_t offset, size_t size);
    //void readAsyn(epicsUInt8 *data, size_t offset, size_t size, readCallback callback, std::string reason, void *args);
    size_t getPageSize();
    size_t getSectorSize();
    size_t getMemorySize();
    bool flashBusy();



private:
    volatile epicsUInt8 * const m_base; // Base address of the EVR/EVG card
    size_t m_size_page;                 // size of one page on the flash chip [bytes]
    size_t m_size_sector;               // size of one sector on the flash chip [bytes]
    size_t m_size_memory;               // flash memmory size [bytes]

    bool m_busy;                        // true if read or write operation is in progress. False otherwise.
    epicsMutex m_flash_lock;             // This lock is held while flashing is in progress
    epicsThreadId m_flash_thread_id;


    //void flashVME(const char *bitFile);
    void pageProgram(epicsUInt8 *data, size_t addr, size_t size);
    void bulkErase();
    void sectorErase(size_t addr);
    void fastRead(const char *bitfile, size_t addr, size_t size);
    void fastRead(epicsUInt8 *data, size_t addr, size_t size);
    void slaveSelect(bool select);
    void write(epicsUInt8 data);
    void waitTransmitterEmpty();
    void waitTransmitterReady();
    void waitReceiverReady();
    epicsUInt8 readStatus();
    epicsUInt8 read();
};

#endif // MRMFLASH_H
