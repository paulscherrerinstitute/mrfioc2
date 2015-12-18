#ifndef MRMFLASH_H
#define MRMFLASH_H

#include <string.h>

#include <epicsMutex.h>
#include <epicsTypes.h>
#include <epicsThread.h>

/**
 * @brief mrfioc2_flashDebug defines debug level (verbosity of debug/info printout)
 */
extern "C" {
    extern int mrfioc2_flashDebug;
    #define infoPrintf(level,M, ...) if(mrfioc2_flashDebug >= level) fprintf(stderr, "mrfioc2_flashInfo: " M,##__VA_ARGS__)
}
//#define dbgPrintf(level,M, ...) if(mrfioc2_flashDebug >= level) fprintf(stderr, "mrfioc2_flashDebug: (%s:%d) " M, __FILE__, __LINE__, ##__VA_ARGS__)


/**
 * @brief The mrmFlash class handles the hardware acces to the flash chip using SPI interface. It exposes methods for reading/writing the flash memory to/from a file.
 */
class epicsShareClass mrmFlash
{
public:
    /**
     * @brief mrmFlash is the main constructor for this class. It uses default values for flash chip page, sector and memory size.
     * @param parentBaseAddress is the base memory-map address of the device (EVG or EVR) that created this class
     */
    mrmFlash(volatile epicsUInt8 *parentBaseAddress);

    /**
     * @brief mrmFlash is an alternative constructor for this class. It is used to set custom values for flash chip page, sector and memory size.
     * @param parentBaseAddress is the base memory-map address of the device (EVG or EVR) that created this class
     * @param pageSize is the size of one page in the flash chip in [bytes]
     * @param sectorSize is the size of one sector in the flash chip in [bytes]
     * @param memorySize is the size of entire memory in the flash chip in [bytes]
     */
    mrmFlash(volatile epicsUInt8 *parentBaseAddress,
             size_t pageSize, size_t sectorSize, size_t memorySize);

    /**
     * @brief flash writes the entire bit file to an offset on the flash chip
     * Throws exceptions (std::invalid_argument or std::runtime_error).
     * @param bitfile is the bit file to write
     * @param offset is the offset from the beginning of the flash chip memory to write to
     */
    void flash(const char *bitfile, size_t offset);

    /**
     * @brief read will read the flash chip memory from a specified offset to the end of the flash chip memory into a file
     * Throws exceptions (std::invalid_argument or std::runtime_error).
     * @param bitfile is the file to write the flash chip content to
     * @param offset is the offset from the beginning of the flash chip memory to start reading from
     */
    void read(const char *bitfile, size_t offset);

    /**
     * @brief getPageSize returns the configured size of one page in the flash chip
     * @return configured size of one page in the flash chip in [bytes]
     */
    size_t getPageSize();

    /**
     * @brief getSectorSize returns the configured size of one sector in the flash chip
     * @return configured size of one sector in the flash chip in [bytes]
     */
    size_t getSectorSize();

    /**
     * @brief getMemorySize returns the configured memory size in the flash chip
     * @return configured memory size in the flash chip in [bytes]
     */
    size_t getMemorySize();

    /**
     * @brief flashBusy indicates if the flash memory is currently being accessed (reading or writing)
     * @return true if flash memory is being accessed, false otherwise
     */
    bool flashBusy();


private:
    volatile epicsUInt8 * const m_base; // Base address of the EVR/EVG card
    size_t m_size_page;                 // size of one page on the flash chip [bytes]
    size_t m_size_sector;               // size of one sector on the flash chip [bytes]
    size_t m_size_memory;               // flash memmory size [bytes]

    bool m_busy;                        // true if read or write operation is in progress. False otherwise.
    epicsMutex m_lock;                  // This lock is held while flashing or reading is in progress

    /**
     * @brief pageProgram issues a page program on the flash chip. It is used to write data one page at a time.
     * @param data is the data to be written to the flash chip
     * @param addr is the address of the flash chip memory where the writing should start
     * @param size is the size of the data we want to write. Note, that data cannot be written over the page boundary.
     */
    void pageProgram(epicsUInt8 *data, size_t addr, size_t size);

    /**
     * @brief bulkErase erases entire content of the flash memory
     */
    void bulkErase();

    /**
     * @brief sectorErase erases one page in the flash memory
     * @param addr is the addres within the page we want to erase
     */
    void sectorErase(size_t addr);

    /**
     * @brief fastRead issues a fast read program on the flash chip. It is used to read the content of the flash chip's memory into a file, starting at a specified address and reading a specified amount of bytes.
     * @param bitfile is the file to write the flash chip's memory content to
     * @param addr is the addres in the flash chip memory, where the reading starts
     * @param size is the number of [bytes] to read from the 'addr' of the flash chip's memory.
     */
    void fastRead(const char *bitfile, size_t addr, size_t size);

    /**
     * @brief slaveSelect a slave select used in the SPI protocol
     * @param select if true, raises the slave select signal. If false, slave is deselected.
     */
    void slaveSelect(bool select);

    /**
     * @brief write will write one byte to the SPI data register
     * @param data to be written to the SPI data register
     */
    void write(epicsUInt8 data);

    /**
     * @brief waitTransmitterEmpty will wait until the SPI transmit buffer is empty
     */
    void waitTransmitterEmpty();

    /**
     * @brief waitTransmitterReady will wait until the SPI transmitter is ready
     */
    void waitTransmitterReady();

    /**
     * @brief waitReceiverReady will wait until the SPI receiver is ready
     */
    void waitReceiverReady();

    /**
     * @brief readStatus preforms the read status program, that returns the status bits of the flash chip.
     * @return the status registers of the flash chip
     */
    epicsUInt8 readStatus();

    /**
     * @brief read will read one byte from the SPI data register
     * @return data from the SPI data register
     */
    epicsUInt8 read();
};

#endif // MRMFLASH_H
