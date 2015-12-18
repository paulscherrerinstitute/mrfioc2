#ifndef MRMFLASH_H
#define MRMFLASH_H

#include <string.h>

#include <epicsMutex.h>
#include <epicsTypes.h>
#include <epicsThread.h>

extern "C" {
    extern int mrfioc2_flashDebug;
    #define infoPrintf(level,M, ...) if(mrfioc2_flashDebug >= level) fprintf(stderr, "mrfioc2_flashInfo: " M,##__VA_ARGS__)
}
//#define dbgPrintf(level,M, ...) if(mrfioc2_flashDebug >= level) fprintf(stderr, "mrfioc2_flashDebug: (%s:%d) " M, __FILE__, __LINE__, ##__VA_ARGS__)



class epicsShareClass mrmFlash
{
public:
    mrmFlash(volatile epicsUInt8 *parentBaseAddress);
    mrmFlash(volatile epicsUInt8 *parentBaseAddress,
             size_t pageSize, size_t sectorSize, size_t memorySize);

    void flash(const char *bitfile, size_t offset);
    void read(const char *bitfile, size_t offset);
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
    epicsMutex m_lock;             // This lock is held while flashing or reading is in progress

    typedef int (*writeToDestination)(epicsUInt8 value, void *dest);

    void pageProgram(epicsUInt8 *data, size_t addr, size_t size);
    void bulkErase();
    void sectorErase(size_t addr);
    void fastRead(const char *bitfile, size_t addr, size_t size);
    void slaveSelect(bool select);
    void write(epicsUInt8 data);
    void waitTransmitterEmpty();
    void waitTransmitterReady();
    void waitReceiverReady();
    epicsUInt8 readStatus();
    epicsUInt8 read();
};

#endif // MRMFLASH_H
