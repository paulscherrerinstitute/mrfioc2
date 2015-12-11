// for htons() et al.
#ifdef _WIN32
 #include <Winsock2.h>
#endif

#include <epicsGuard.h>
#include <mrfCommonIO.h>

#include "stdexcept"
#include <stdlib.h>
#include "stdio.h"
//#include <map>

#include "mrmShared.h"

#include <epicsExport.h>
#include "mrmFlash.h"

// Misc
#define RETRY_COUNT             10000         // Amount of retries until we fail when chacking command completition
#define RETRY_COUNT_ERASE       0x10000000    // Amount of retries until we fail when chacking the erase command completition

// Flash chip commands
#define M25P_READ_FAST              0x0B
#define M25P_READ_IDENTIFICATION    0x9F
#define M25P_WRITE_ENABLE           0x06
#define M25P_WRITE_DISABLE          0x04
#define M25P_READ_STATUS            0x05
#define M25P_SECTOR_ERASE           0xD8
#define M25P_BULK_ERASE             0xC7
#define M25P_PAGE_PROGRAM           0x02

// Flash chip status command returns:
#define M25P_STATUS_WIP                 0x01  // write in progress
#define M25P_STATUS_WRITE_ENABLE_LATCH  0x02  // write enable latch bit
#define M25P_STATUS_BLOCK_PROTECT       0x0C  // block protect bits
#define M25P_STATUS_WRITE_PROTECT       0x80  // status register write protect

// Flash chip default size definition
#define SIZE_PAGE           256             // Maximum number of bytes we can write at once.
#define SIZE_SECTOR         0x00010000      // default sector size, if not provided by user [bytes]
#define SIZE_MEMORY         0x00260000      // default flash memory size, if not provided by user [bytes]



extern "C" {
    int mrfioc2_flashDebug;
    epicsExportAddress(int, mrfioc2_flashDebug);
}
#define dbgPrintf(level,M, ...) if(mrfioc2_flashDebug >= level) fprintf(stderr, "mrfioc2_flashDebug: (%s:%d) " M, __FILE__, __LINE__, ##__VA_ARGS__)
#define infoPrintf(level,M, ...) if(mrfioc2_flashDebug >= level) fprintf(stderr, "mrfioc2_flashInfo: " M,##__VA_ARGS__)

//static std::map<std::string, mrmFlash*> flash_instances;

mrmFlash::mrmFlash(//const char * parentName,
                   volatile epicsUInt8 *parentBaseAddress)
    :m_base(parentBaseAddress)
{
    m_size_page   = SIZE_PAGE;
    m_size_sector = SIZE_SECTOR;
    m_size_memory = SIZE_MEMORY;

    //flash_instances[parentName] = this;
}

mrmFlash::mrmFlash(//const char *parentName,
                   volatile epicsUInt8 *parentBaseAddress,
                   size_t pageSize, size_t sectorSize, size_t memorySize)
    :m_base(parentBaseAddress)
    ,m_size_page(pageSize)
    ,m_size_sector(sectorSize)
    ,m_size_memory(memorySize)
{
    //flash_instances[parentName] = this;
}


// TODO rename offset to address?
void mrmFlash::flash(const char *bitfile, size_t offset) {
    epicsUInt8 *buf=NULL;
    FILE *fd=NULL;
    size_t size, readSize;

    epicsGuard<epicsMutex> g(m_flash_lock);
    try{

        m_busy = true;

        if(offset < 0 || offset >= m_size_memory) {
            throw std::invalid_argument("Invalid offset!");
        }

        buf = (epicsUInt8 *)malloc(m_size_page);
        if (!buf) {
          throw std::runtime_error("Could not reserve page memory!");
        }

        infoPrintf(1,"Opening file...");
        fd = fopen(bitfile, "r");
        if (fd == NULL) {
            free(buf);
            throw std::runtime_error("Could not open file for reading!");
        }

        // TODO make this optional?
        // check if file is to big to be written to flash
        fseek(fd, 0L, SEEK_END);
        size = ftell(fd);
        if( offset + size > m_size_memory) {
            free(buf);
            fclose(fd);
            throw std::invalid_argument("File too big to be written to this offset!");
        }
        fseek(fd, 0L, SEEK_SET);
        infoPrintf(1,"opened.\n");


        if(offset < m_size_sector) {    // if provided offset is inside first sector, erase entire memory
            bulkErase();
        }
        else {  // start erasing sector at 'offset' address, and continue until the end of memory
            for (size_t addr = offset; addr < m_size_memory; addr += m_size_sector) {
                sectorErase(addr);
            }
        }

        // start the programming of the flash memory at the 'offset' address
        readSize = m_size_page - (offset % m_size_page);    // offset might not be page-alligned. First write is until the end of page. All the rest are page-alligned.
        do {    // write data page by page
            size = fread(buf, 1, readSize, fd);  // one page can be written at once
            readSize = m_size_page; // consecutive reads are page-alligned
            pageProgram(buf, offset, size);
            if((offset & 0x0000ffff) == 0) {  // Do not print in each iteration so the output is not flooded
                infoPrintf(1,"\tWriting to address: %08x\n", offset);
            }
            offset += size;
        } while (size > 0);
        infoPrintf(1, "Wrote %d bytes to flash.\n", offset);

        free(buf);
        fclose(fd);
        m_busy = false;
    }
    catch(std::exception& ex) {
        if(buf != NULL) free(buf);
        if(fd != NULL) fclose(fd);
        m_busy = false;
        throw;
    }
}

void mrmFlash::read(const char *bitfile, size_t offset) {
    epicsGuard<epicsMutex> g(m_flash_lock);

    try{
        if(m_busy) {
            throw std::runtime_error("SPI busy!");
        }

        if(offset < 0 || offset >= m_size_memory) {
            throw std::invalid_argument("Invalid offset!");
        }

        m_busy = true;
        fastRead(bitfile, offset, m_size_memory-offset);
        m_busy = false;
    }
    catch(std::exception& ex) {
        m_busy = false;
        throw;
    }
}

void mrmFlash::read(epicsUInt8 * data, size_t offset, size_t size) {
    epicsGuard<epicsMutex> g(m_flash_lock);

    m_busy = true;
    try {
        fastRead(data, offset, size);
        m_busy = false;
    }
    catch(std::exception& ex) {
        m_busy = false;
        throw;
    }
}

/*void mrmFlash::readAsyn(epicsUInt8 *data, size_t offset, size_t size, readCallback callback, std::string reason, void* args) {
    epicsGuard<epicsMutex> g(m_flash_lock);

    if(m_busy) {
        throw std::runtime_error("SPI busy!");
    }
    else if(!epicsThreadIsSuspended(m_flash_thread_id)){    //Check if thread is still alive
        throw std::runtime_error("SPI thread still running!");
    }

    tReadThreadArgs *threadArgs = new tReadThreadArgs;
    threadArgs->data = data;
    threadArgs->offset = offset;
    threadArgs->size = size;
    threadArgs->callback = callback;
    threadArgs->reason = reason;
    threadArgs->args = args;
    threadArgs->parent = this;

    m_busy = true;
    m_flash_thread_id = epicsThreadCreate("MRF SPI READ",epicsThreadPriorityLow,epicsThreadGetStackSize(epicsThreadStackMedium),&mrmFlash::readThread, threadArgs);
}

void mrmFlash::readThread(void *args) {
    tReadThreadArgs *threadArgs = (tReadThreadArgs*)args;
    mrmFlash *parent = threadArgs->parent;

    try{
        parent->fastRead(threadArgs->data, threadArgs->offset, threadArgs->size);
        parent->m_busy = false;
        (*threadArgs->callback)(threadArgs->data, threadArgs->reason, threadArgs->args);
    }
    catch(std::exception& ex) {
        parent->m_busy = false;
        threadArgs->reason = ex.what();
        (*threadArgs->callback)(threadArgs->data, threadArgs->reason, threadArgs->args);
    }
    delete threadArgs;
}*/

size_t mrmFlash::getPageSize()
{
    return m_size_page;
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
    return m_busy;
}

/*mrmFlash *mrmFlash::getFlashInstanceFromDevice(const char *device){
    // locking not needed because all data buffer instances are created before someone can use them

    if(flash_instances.count(device)){
        return flash_instances[device];
    }

    return NULL;
}*/

void mrmFlash::pageProgram(epicsUInt8 *data, size_t addr, size_t size) {
    size_t i;

    /* Check that size and address are valid */
    if(size < 1 || size > m_size_page) {
        throw std::invalid_argument("Invalid size!");
    }
    if((addr % m_size_page) + size > m_size_page) {
        throw std::invalid_argument("Writing that many bytes on this address would cross the page boundary!");
    }

    try{
        infoPrintf(2,"\tDummy write with SS not active...\n");
        slaveSelect(false);
        write(M25P_READ_IDENTIFICATION);

        infoPrintf(2,"\tWrite enable...\n");
        slaveSelect(true);
        write(M25P_WRITE_ENABLE);
        slaveSelect(false);


        infoPrintf(2,"\tSend page program command...\n");
        slaveSelect(true);
        write(M25P_PAGE_PROGRAM);

        infoPrintf(2,"\tSend address %x\n", addr);
        write((addr >> 16) & 0x00ff);
        write((addr >> 8)  & 0x00ff);
        write( addr        & 0x00ff);

        for (i = 0; i < m_size_page; i++) {
          write(data[i]);
        }

        slaveSelect(false);

        infoPrintf(2,"\tWaiting for page program completition...\n");
        i = 0;
        while(!(readStatus() & M25P_STATUS_WIP) && i < RETRY_COUNT) {
            if ((i % 100000) == 0) {
                infoPrintf(1,"\tWaiting for %zu iterations\n", i);
            }
            i++;
        }

        if(i >= RETRY_COUNT) {
            throw std::runtime_error("Timeout reached while waiting for page program to complete!");
        }

        slaveSelect(false);
    }
    catch(std::exception& ex) {
        slaveSelect(false);
        throw;
    }
}

void mrmFlash::bulkErase() {
    size_t i = 0;

    try {
        infoPrintf(1,"Erasing flash (bulk erase)...\n");

        infoPrintf(2,"\tDummy write with SS not active...\n");
        slaveSelect(false);
        write(M25P_READ_IDENTIFICATION);

        infoPrintf(2,"\tWrite enable...\n");
        slaveSelect(true);
        write(M25P_WRITE_ENABLE);
        slaveSelect(false);

        infoPrintf(2,"\tSending bulk erase command...\n");
        slaveSelect(true);
        write(M25P_BULK_ERASE);
        slaveSelect(false);


        infoPrintf(2,"\tWaiting for bulk erase completition...\n");
        while(!(readStatus() & M25P_STATUS_WIP) && i < RETRY_COUNT_ERASE) {
            if ((i % 100000) == 0) {
                infoPrintf(1,"\tWaiting for %zu iterations\n", i);
            }
            i++;
        }

        if(i >= RETRY_COUNT_ERASE) {
            throw std::runtime_error("Timeout reached while waiting for bulk erase to complete!");
        }

        slaveSelect(false);

        infoPrintf(1,"Flash erased (bulk erase).\n");
    }
    catch(std::exception& ex) {
        slaveSelect(false);
        throw;
    }
}

void mrmFlash::sectorErase(size_t addr) {
    size_t i = 0;

    if(addr < 0 || addr >= m_size_memory) {
        throw std::invalid_argument("Invalid address!");
    }

    try{
        infoPrintf(1,"Erasing flash (sector erase)...\n");

        infoPrintf(2,"\tDummy write with SS not active...\n");
        slaveSelect(false);
        write(M25P_READ_IDENTIFICATION);

        infoPrintf(2,"\tWrite enable...\n");
        slaveSelect(true);
        write(M25P_WRITE_ENABLE);
        slaveSelect(false);

        infoPrintf(2,"\tSending sector erase command...\n");
        slaveSelect(true);
        write(M25P_SECTOR_ERASE);
        slaveSelect(false);

        infoPrintf(2,"\tSend address: %x\n", addr);
        write((addr >> 16) & 0x00ff);
        write((addr >> 8)  & 0x00ff);
        write( addr        & 0x00ff);
        slaveSelect(false);

        infoPrintf(2,"\tWaiting for sector erase completition...\n");
        while(!(readStatus() & M25P_STATUS_WIP) && i < RETRY_COUNT_ERASE) {
            if ((i % 100000) == 0) {
                infoPrintf(1,"\tWaiting for %zu iterations\n", i);
            }
            i++;
        }

        if(i >= RETRY_COUNT_ERASE) {
            throw std::runtime_error("Timeout reached while waiting for bulk erase to complete!");
        }

        slaveSelect(false);
    }
    catch(std::exception& ex) {
        slaveSelect(false);
        throw;
    }
}

void mrmFlash::fastRead(const char *bitfile, size_t addr, size_t size) {
    size_t i;
    FILE *fd;
    epicsUInt8 buf;

    if( addr < 0 || addr > m_size_memory) {
        throw std::invalid_argument("Address for reading out of bounds!");
    }

    if(addr + size > m_size_memory) {
        size = m_size_memory - addr;
    }

    fd = fopen(bitfile, "w");
    if (fd == NULL) {
        throw std::runtime_error("Could not open file for writing!");
    }

    try {
        slaveSelect(false);
        write(0);

        slaveSelect(true);
        write(M25P_READ_FAST);

        /* Three address bytes */
        write((addr >> 16) & 0x00ff);
        write((addr >> 8)  & 0x00ff);
        write( addr        & 0x00ff);

        /* One dummy write + the first write that actually reads and starts
         the transfer of the first real byte */
        write(0);
        write(0);

        for (i = 0; i < size; i++) {
            buf = read();
            if(fwrite(&buf, 1, 1, fd) != 1){
                throw std::runtime_error("Could not write to file!");
            }
            write(0);
        }

        slaveSelect(false);
        fclose(fd);
    }
    catch(std::exception& ex) {
        slaveSelect(false);
        fclose(fd);
        throw;
    }
}

void mrmFlash::fastRead(epicsUInt8 *data, size_t addr, size_t size) {
    size_t i;

    if( addr < 0 || addr > m_size_memory) {
        throw std::invalid_argument("Address for reading out of bounds!");
    }

    if(addr + size > m_size_memory) {
        size = m_size_memory - addr;
    }

    try {
        slaveSelect(false);
        write(0);

        slaveSelect(true);
        write(M25P_READ_FAST);

        /* Three address bytes */
        write((addr >> 16) & 0x00ff);
        write((addr >> 8)  & 0x00ff);
        write( addr        & 0x00ff);

        /* One dummy write + the first write that actually reads and starts
         the transfer of the first real byte */
        write(0);
        write(0);

        for (i = 0; i < size; i++) {
            data[i] = read();
            write(0);
        }
        /*while (size--) {
            *(data++) = read();
            write(0);
        }*/

        slaveSelect(false);
    }
    catch(std::exception& ex) {
        slaveSelect(false);
        throw;
    }
}

void mrmFlash::slaveSelect(bool select) {
    epicsUInt32 spiControlReg;

    waitTransmitterEmpty();

    infoPrintf(4,"Slave select: %s\n", select ? "select" : "deselect");
    if(select) {
        spiControlReg = BE_READ32(m_base, SpiCtrl);
        spiControlReg |= SpiCtrl_oe;
        BE_WRITE32(m_base, SpiCtrl, spiControlReg);

        // set slave select
        spiControlReg = BE_READ32(m_base, SpiCtrl);
        spiControlReg |= SpiCtrl_oe | SpiCtrl_slaveSelect;
        BE_WRITE32(m_base, SpiCtrl, spiControlReg);

    }
    else {
        spiControlReg = BE_READ32(m_base, SpiCtrl);
        spiControlReg |= SpiCtrl_oe;
        BE_WRITE32(m_base, SpiCtrl, spiControlReg);

        // reset slave select
        spiControlReg = BE_READ32(m_base, SpiCtrl);
        spiControlReg &= ~(SpiCtrl_oe | SpiCtrl_slaveSelect);
        BE_WRITE32(m_base, SpiCtrl, spiControlReg);
    }
}

void mrmFlash::write(epicsUInt8 data) {

    waitTransmitterReady();

    infoPrintf(1,"Writing %02x...", data);
    BE_WRITE32(m_base, SpiData, data);
    infoPrintf(1,"done.\n");
}

void mrmFlash::waitTransmitterEmpty() {
    size_t i=0;

    infoPrintf(2,"Waiting until transmitter empty...");

    while(!(BE_READ32(m_base, SpiCtrl) & SpiCtrl_tmt) && i < RETRY_COUNT) {
        i++;
    }
    infoPrintf(2,"done waiting.\n");

    if (i >= RETRY_COUNT) {
        throw std::runtime_error("Timeout reached while waiting for transmitter to be ready!");
    }
}

void mrmFlash::waitTransmitterReady() {
    size_t i=0;

    infoPrintf(2,"Waiting until transmitter ready...");
    while(!(BE_READ32(m_base, SpiCtrl) & SpiCtrl_trdy) && i < RETRY_COUNT) {
      i++;
    }
    infoPrintf(2,"done waiting.\n");

    if (i >= RETRY_COUNT) {
        throw std::runtime_error("Timeout reached while waiting for transmitter to be ready!");
    }
}

void mrmFlash::waitReceiverReady() {
    size_t i=0;

    infoPrintf(2,"Waiting until receiver ready...");
    while(!(BE_READ32(m_base, SpiCtrl) & SpiCtrl_rrdy) && i < RETRY_COUNT) {
        i++;
    }

    if (i >= RETRY_COUNT) {
        throw std::runtime_error("Timeout reached while waiting for receiver to be ready!");
    }
}


epicsUInt8 mrmFlash::readStatus() {
    epicsUInt8 data;

    try {
        infoPrintf(2,"Reading status...\n");

        infoPrintf(3,"\tDummy write with SS not active...\n");
        slaveSelect(false);
        write(0);

        infoPrintf(3,"\tSending read status command...\n");
        slaveSelect(true);
        write(M25P_READ_STATUS);
        write(0);

        data = read();
        slaveSelect(false);

        infoPrintf(2,"Reading status done.\n");

        return data;
    }
    catch(std::exception& ex) {
        slaveSelect(false);
        throw;
    }
}

epicsUInt8 mrmFlash::read() {
    epicsUInt32 data;

    waitReceiverReady();

    infoPrintf(1,"Reading...");
    data = BE_READ32(m_base, SpiData);
    infoPrintf(1,"Read %02x...", data);

    return (epicsUInt8)data;
}
