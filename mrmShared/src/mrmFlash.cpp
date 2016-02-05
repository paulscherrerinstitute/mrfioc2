// for htons() et al.
#ifdef _WIN32
 #include <Winsock2.h>
#endif

#include <epicsGuard.h>
#include <mrfCommonIO.h>

#include "stdexcept"
#include <stdlib.h>
#include "stdio.h"

#include "mrmShared.h"

#include <epicsExport.h>
#include <epicsThread.h>    // for sleep
#include "mrmFlash.h"

// Misc
#define RETRY_COUNT             10000         // Amount of retries until we fail when waiting for SPI receiver / transmitter to be ready

// Flash chip commands
#define CMD_READ_FAST              0x0B
#define CMD_READ_IDENTIFICATION    0x9F
#define CMD_WRITE_ENABLE           0x06
#define CMD_WRITE_DISABLE          0x04
#define CMD_READ_STATUS            0x05
#define CMD_SECTOR_ERASE           0xD8
#define CMD_BULK_ERASE             0xC7
#define CMD_PAGE_PROGRAM           0x02

// Flash chip status command returns:
#define STATUS_WIP                 0x01  // write in progress
#define STATUS_WRITE_ENABLE_LATCH  0x02  // write enable latch bit
#define STATUS_BLOCK_PROTECT       0x0C  // block protect bits
#define STATUS_WRITE_PROTECT       0x80  // status register write protect

// Flash chip size definitions
#define MB2BYTE                     1024 * 1024 / 8 // convert from mega bits to bytes
#define SIZE_PAGE                   256             // Maximum number of bytes we can write at once.
#define SIZE_SECTOR                 0x00010000      // most M25P chip have this default sector size
#define SIZE_SECTOR_128             0x00040000      // 128Mb M25P chip has this default sector size
#define SIZE_SECTOR_DEFAULT         SIZE_SECTOR     // default sector size, if it can not be detected [bytes]
#define SIZE_MEMORY_DEFAULT         32 * MB2BYTE    // default flash memory size, if it can not be detected [bytes]

// Read identification command return values (based on information available at Micron web site):
#define MICRON_MANUFACTURER_ID 0x20

#define M25P_MEMORY_TYPE     0x20
#define N25Q_MEMORY_TYPE     0xBA

#define MEMORY_SIZE_128 0x18
#define MEMORY_SIZE_64  0x17
#define MEMORY_SIZE_32  0x16
#define MEMORY_SIZE_16  0x15
#define MEMORY_SIZE_8   0x14
#define MEMORY_SIZE_4   0x13
#define MEMORY_SIZE_2   0x12


extern "C" {
    int mrfioc2_flashDebug = 0;
    epicsExportAddress(int, mrfioc2_flashDebug);
}

mrmFlash::mrmFlash(volatile epicsUInt8 *parentBaseAddress)
    :m_base(parentBaseAddress)
    ,m_size_sector(0)
    ,m_size_memory(0)
{
}

void mrmFlash::init(bool autodetect) {
    epicsUInt8 manufacturerID = 0, memoryType = 0, capacity = 0;

    if(autodetect) {
        readIdentification(&manufacturerID, &memoryType, &capacity);

        infoPrintf(1, "Flash chip identification read:\n\t" \
                      "Manufacturer ID: 0x%x\n\t" \
                      "Memory type: 0x%x\n\t"  \
                      "Capacity: 0x%x\n" \
                   , manufacturerID, memoryType, capacity);

        if(manufacturerID != MICRON_MANUFACTURER_ID) {
            throw std::runtime_error("Micron manufacturer ID not detected. Aborted.");
        }

        if(memoryType == M25P_MEMORY_TYPE){
            switch(capacity) {
                case MEMORY_SIZE_128:
                    m_size_sector = SIZE_SECTOR_128;
                    m_size_memory = 128 * MB2BYTE;
                    break;

                case MEMORY_SIZE_64:
                    m_size_sector = SIZE_SECTOR;    // could not find the info in the datasheets. Assuming this size...
                    m_size_memory = 64 * MB2BYTE;
                    break;

                case MEMORY_SIZE_32:
                    m_size_sector = SIZE_SECTOR;
                    m_size_memory = 32 * MB2BYTE;
                    break;

                case MEMORY_SIZE_16:
                    m_size_sector = SIZE_SECTOR;
                    m_size_memory = 16 * MB2BYTE;
                    break;

                case MEMORY_SIZE_8:
                    m_size_sector = SIZE_SECTOR;
                    m_size_memory = 8 * MB2BYTE;    // Last address in the documentation is 0xFEFFF, which would suggest that the size is 0xFF000 = 1044480 bytes. But the chip is 8 Mb = 1048576 bytes, which is also stated in the documentation. Assuming size = 8 Mb = 1048576 bytes
                    break;

                case MEMORY_SIZE_4:
                    m_size_sector = SIZE_SECTOR;
                    m_size_memory = 4 * MB2BYTE;
                    break;

                case MEMORY_SIZE_2:
                    m_size_sector = SIZE_SECTOR;
                    m_size_memory = 2 * MB2BYTE;
                    break;

                default:
                    throw std::runtime_error("Memory size for this flash chip is not supported. Aborted.");
            }
        } // end M25P_MEMORY_TYPE
        else if(memoryType == N25Q_MEMORY_TYPE){
            switch(capacity) {
                case MEMORY_SIZE_128:
                    m_size_sector = SIZE_SECTOR;
                    m_size_memory = 128 * MB2BYTE;
                    break;

                default:
                    throw std::runtime_error("Memory size for this flash chip is not supported. Aborted.");
            }
        }// end N25Q_MEMORY_TYPE
        else{
            throw std::runtime_error("Micron M25P or N25Q flash chip type not detected. Aborted.");
        }

    } // end of autodetect
    else {
        m_size_sector = SIZE_SECTOR_DEFAULT;
        m_size_memory = SIZE_MEMORY_DEFAULT;
    }
}


void mrmFlash::flash(const char *bitfile, size_t offset) {
    epicsUInt8 buf[SIZE_PAGE];
    FILE *fd = NULL;
    size_t size, readSize;


    if(m_size_memory <= 0 || m_size_sector <= 0) {
        throw std::runtime_error("Flash chip sector and memory sizes not defined. Did you run init() function?");
    }

    if(offset < 0 || offset >= m_size_memory) {
        throw std::invalid_argument("Invalid offset for flashing.");
    }

    // Open the file
    fd = fopen(bitfile, "rb");
    if (fd == NULL) {
        throw std::runtime_error("Could not open file for reading.");
    }

    // check if file is to big to be written to flash
    fseek(fd, 0L, SEEK_END);
    size = ftell(fd);
    if( offset + size > m_size_memory) {
        fclose(fd);
        throw std::invalid_argument("File too big to be written to this offset.");
    }
    fseek(fd, 0L, SEEK_SET);

    infoPrintf(1,"Starting flash procedure\n");
    try{
        epicsGuard<epicsMutex> g(m_lock);

        infoPrintf(1, "Erasing flash...\n");
        if(offset <= 0) {    // if offset is 0, erase entire memory
            bulkErase();
        }
        else {  // start erasing sector at 'offset' address, and continue until the end of memory
            for (size_t addr = offset; addr < m_size_memory; addr += m_size_sector) {
                sectorErase(addr);
            }
        }

        // start the programming of the flash memory at the 'offset' address
        infoPrintf(1, "Writing to flash....\n");
        readSize = SIZE_PAGE - (offset % SIZE_PAGE);    // offset might not be page-alligned. First write is until the end of page. All the rest are page-alligned.
        size = fread(buf, 1, readSize, fd); // max one page can be written at once
        readSize = SIZE_PAGE;             // consecutive reads are page-alligned
        while (size > 0) {    // write data page by page
            pageProgram(buf, offset, size);     // use page program to write data to the flash memory
            if((offset & 0x0000ffff) == 0) {    // Do not print in each iteration so the output is not flooded
                infoPrintf(1,"\tWriting to address: 0x%08zx\n", offset);
            }
            offset += size;
            size = fread(buf, 1, readSize, fd); // max one page can be written at once
        }
    }
    catch(std::exception&) {
        fclose(fd);
        throw;
    }

    infoPrintf(1, "Flash written.\n");
    fclose(fd);

}

void mrmFlash::read(const char *bitfile, size_t offset) {
    size_t i;
    FILE *fd;
    epicsUInt8 buf;
    const size_t size = m_size_memory-offset;

    if(m_size_memory <= 0 || m_size_sector <= 0) {
        throw std::runtime_error("Flash chip sector and memory sizes invalid. Did you run init() function?");
    }

    if( offset < 0 || offset >= m_size_memory) {
        throw std::invalid_argument("Address for reading out of bounds!");
    }

//    if(offset + size > m_size_memory) {
//        size = m_size_memory - offset;
//    }

    fd = fopen(bitfile, "wb");
    if (fd == NULL) {
        throw std::runtime_error("Could not open file for writing the flash content to!");
    }

    try {
        infoPrintf(1,"Starting to read flash memory...\n");
        epicsGuard<epicsMutex> g(m_lock);

        // Dummy write with SS not active
        slaveSelect(false);
        write(0);

        // Send read fast command
        slaveSelect(true);
        write(CMD_READ_FAST);

        // Three address bytes
        write((epicsUInt8)(offset >> 16));
        write((epicsUInt8)(offset >> 8 ));
        write((epicsUInt8)(offset      ));

        // One dummy write + the first write that actually starts the transfer of the first real byte
        write(0);
        write(0);

        for (i = 0; i < size; i++) {
            buf = read();
            if(fwrite(&buf, 1, 1, fd) != 1){
                throw std::runtime_error("Could not write to file!");
            }
            if((i & 0x0000ffff) == 0) {    // Do not print in each iteration so the output is not flooded
                infoPrintf(1,"\tReading address: 0x%08zx\n", offset+i);
            }
            write(0);
        }

        slaveSelect(false);
        fclose(fd);
        infoPrintf(1,"Reading of the flash memory done.\n");
    }
    catch(std::exception&) {
        slaveSelect(false);
        fclose(fd);
        throw;
    }
}

size_t mrmFlash::getPageSize()
{
    return SIZE_PAGE;
}

size_t mrmFlash::getSectorSize()
{
    return m_size_sector;
}

size_t mrmFlash::getMemorySize()
{
    return m_size_memory;
}

bool mrmFlash::flashBusy()
{
    if(m_lock.tryLock()) {
        m_lock.unlock();
        return false;
    }
    else {
        return true;
    }
}

void mrmFlash::report()
{
    printf("\tMemory size: %" FORMAT_SIZET_U " bytes = %" FORMAT_SIZET_U " kB = %" FORMAT_SIZET_U " MB\n", m_size_memory, m_size_memory / 1024, m_size_memory / 1024 / 1024);
    printf("\tSector size: %" FORMAT_SIZET_U " bytes = %" FORMAT_SIZET_U " kB\n", m_size_sector, m_size_sector / 1024);
    printf("\tPage size: %d bytes\n", SIZE_PAGE);
}

//// Private functions start here ////

void mrmFlash::pageProgram(epicsUInt8 *data, size_t addr, size_t size) {
    size_t i;

    /* Check that size and address are valid */
    if(size < 1 || size > SIZE_PAGE) {
        throw std::invalid_argument("Invalid size. Cannot write more than page size at a time.");
    }
    if((addr % SIZE_PAGE) + size > SIZE_PAGE) {
        throw std::invalid_argument("Writing that many bytes on this address would cross the page boundary.");
    }

    try{
        infoPrintf(3,"Starting page program on address 0x%" FORMAT_SIZET_X " with size %" FORMAT_SIZET_U "\n", addr, size);

        // Dummy write with SS not active
        slaveSelect(false);
        write(CMD_READ_IDENTIFICATION);

        // Write enable
        slaveSelect(true);
        write(CMD_WRITE_ENABLE);
        slaveSelect(false);

        // Send page program command
        slaveSelect(true);
        write(CMD_PAGE_PROGRAM);

        // Send address where the programming will start
        write((epicsUInt8)(addr >> 16));
        write((epicsUInt8)(addr >> 8 ));
        write((epicsUInt8)(addr      ));

        for (i = 0; i < size; i++) {
          write(data[i]);
        }

        slaveSelect(false);

        waitForCompletition(10, 1, 4); // Command duration info: Typical: 0.8 ms, Max 5ms
        infoPrintf(3,"Page programmed.\n\n");
    }
    catch(std::exception&) {
        slaveSelect(false);
        throw;
    }
}

void mrmFlash::bulkErase() {
    try {
        infoPrintf(2,"Starting bulk erase...\n");

        // Dummy write with SS not active
        slaveSelect(false);
        write(CMD_READ_IDENTIFICATION);

        // Write enable
        slaveSelect(true);
        write(CMD_WRITE_ENABLE);
        slaveSelect(false);

        // Send bulk erase command
        slaveSelect(true);
        write(CMD_BULK_ERASE);
        slaveSelect(false);

        waitForCompletition(60, 5000, 2); // Command duration info: Typical: 10 s, Max 250 s
        infoPrintf(2,"Flash erased (bulk erase).\n\n");
    }
    catch(std::exception&) {
        slaveSelect(false);
        throw;
    }
}

void mrmFlash::sectorErase(size_t addr) {
    if(addr < 0 || addr >= m_size_memory) {
        throw std::invalid_argument("Invalid address for sector erase!");
    }

    try{
        infoPrintf(2,"Starting sector erase on address: 0x%" FORMAT_SIZET_X "\n", addr);

        // Dummy write with SS not active
        slaveSelect(false);
        write(CMD_READ_IDENTIFICATION);

        // Write enable
        slaveSelect(true);
        write(CMD_WRITE_ENABLE);
        slaveSelect(false);

        // Send sector erase command
        slaveSelect(true);
        write(CMD_SECTOR_ERASE);

        // Send address in a sector we are erasing
        write((epicsUInt8)(addr >> 16));
        write((epicsUInt8)(addr >> 8 ));
        write((epicsUInt8)(addr      ));
        slaveSelect(false);

        waitForCompletition(3500, 1, 3);   // Command duration info: Typical: 1.6 ms, Max 3 s
        infoPrintf(2,"Sector erase complete\n\n");
    }
    catch(std::exception&) {
        slaveSelect(false);
        throw;
    }
}

void mrmFlash::readIdentification(epicsUInt8 *manufacturerID, epicsUInt8 *memoryType, epicsUInt8 *memoryCapacity) {
    try{
        // Dummy write with SS not active
        slaveSelect(false);
        write(0);

        // Send read identification command
        slaveSelect(true);
        write(CMD_READ_IDENTIFICATION);

        write(0);   // Transfer byte
        *manufacturerID = read();

        write(0);   // Transfer byte
        *memoryType = read();

        write(0);   // Transfer byte
        *memoryCapacity = read();

        slaveSelect(false);
    }
    catch(std::exception&) {
        slaveSelect(false);
        throw;
    }

}

void mrmFlash::slaveSelect(bool select) {
    epicsUInt32 spiControlReg;

    waitTransmitterEmpty();

    if(select) {
        spiControlReg = READ32(m_base, SpiCtrl);
        spiControlReg |= SpiCtrl_oe;
        WRITE32(m_base, SpiCtrl, spiControlReg);

        // set slave select
        spiControlReg = READ32(m_base, SpiCtrl);
        spiControlReg |= SpiCtrl_oe | SpiCtrl_slaveSelect;
        WRITE32(m_base, SpiCtrl, spiControlReg);

    }
    else {
        spiControlReg = READ32(m_base, SpiCtrl);
        spiControlReg |= SpiCtrl_oe;
        WRITE32(m_base, SpiCtrl, spiControlReg);

        // reset slave select
        spiControlReg = READ32(m_base, SpiCtrl);
        spiControlReg &= ~(SpiCtrl_oe | SpiCtrl_slaveSelect);
        WRITE32(m_base, SpiCtrl, spiControlReg);
    }
}

void mrmFlash::write(epicsUInt8 data) {

    waitTransmitterReady();

    WRITE32(m_base, SpiData, data);
}

void mrmFlash::waitForCompletition(size_t retryCount, size_t msSleep, int verbosity)
{
    size_t i = 0;

    infoPrintf(verbosity,"\tWaiting for command completition...\n");
    while((readStatus() & STATUS_WIP) && (i < retryCount)) {
        infoPrintf(verbosity,"\t\tWaiting for %" FORMAT_SIZET_U " ms\n", i * msSleep);
        i++;
        epicsThreadSleep(msSleep / 1000.0);
    }

    if(i >= retryCount) {
        throw std::runtime_error("Timeout reached while waiting for command to complete!");
    }
}

// TODO remove this one and only use waitTransmitterReady?
void mrmFlash::waitTransmitterEmpty() {
    size_t i=0;

    while(!(READ32(m_base, SpiCtrl) & SpiCtrl_tmt) && (i < RETRY_COUNT)) {
        i++;
        // removed epicsThreadSleep because it takes too long on windows
    }

    if (i >= RETRY_COUNT) {
        throw std::runtime_error("Timeout reached while waiting for transmitter to be empty!");
    }
}

void mrmFlash::waitTransmitterReady() {
    size_t i=0;

    while(!(READ32(m_base, SpiCtrl) & SpiCtrl_trdy) && (i < RETRY_COUNT)) {
      i++;
      // removed epicsThreadSleep because it takes too long on windows
    }

    if (i >= RETRY_COUNT) {
        throw std::runtime_error("Timeout reached while waiting for transmitter to be ready!");
    }
}

void mrmFlash::waitReceiverReady() {
    size_t i=0;

    while(!(READ32(m_base, SpiCtrl) & SpiCtrl_rrdy) && (i < RETRY_COUNT)) {
        i++;
        // removed epicsThreadSleep because it takes too long on windows
    }

    if (i >= RETRY_COUNT) {
        throw std::runtime_error("Timeout reached while waiting for receiver to be ready!");
    }
}

epicsUInt8 mrmFlash::readStatus() {
    epicsUInt8 data;

    try {
        // Dummy write with SS not active
        slaveSelect(false);
        write(0);

        // Send read status command
        slaveSelect(true);
        write(CMD_READ_STATUS);
        write(0);

        data = read();
        slaveSelect(false);

        return data;
    }
    catch(std::exception&) {
        slaveSelect(false);
        throw;
    }
}

epicsUInt8 mrmFlash::read() {
    epicsUInt32 data;

    waitReceiverReady();

    data = READ32(m_base, SpiData);

    return (epicsUInt8)data;
}
