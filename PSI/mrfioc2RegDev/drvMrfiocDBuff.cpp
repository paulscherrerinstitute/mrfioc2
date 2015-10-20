/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** drvMrfiocDBuff.cpp
**
** RegDev device support for Distributed Buffer on MRF EVR and EVG cards using
** mrfioc2 driver.
**
** -- DOCS ----------------------------------------------------------------
** Driver is registered via iocsh command:
**         mrfiocDBuffConfigure <regDevName> <mrfName> <protocol ID>
**
**             -regDevName: name of device as seen from regDev. E.g. this
**                         name must be the same as parameter 1 in record OUT/IN
**
**             -mrfName: name of mrf device
**
**             -protocol ID: protocol id (32 bit int). If set to 0, than receiver
**                         will accept all buffers. This is useful for
**                         debugging. If protocol != 0 then only received buffers
**                         with same protocol id are accepted. If you need to work
**                         with multiple protocols you can register multiple instances
**                         of regDev using the same mrfName but different regDevNames and
**                         protocols.
**
**
**         example:    mrfiocDBuffConfigure EVGDBUFF EVG1 42
**
**
** EPICS use:
**
**         - records behave the same as for any other regDev device with one exception:
**
**         - offset 0x00-0x04 can not be written to, since it is occupied by protocolID
**         - writing to offset 0x00 will flush the buffer
**         - input records are automatically processed (if scan == IO) when a valid buffer
**             is received
**
**
**
** -- SUPPORTED DEVICES -----------------------------------------------------
**
** VME EVG-230 (tx only, EVG does not support databuffer rx)
** VME EVR-230 (tx and rx)
** PCI EVR-230 (rx only, firmware version 3 does not support databuffer tx)
** PCIe EVR-300 (tx and rx)
**
**
** -- IMPLEMENTATION ---------------------------------------------------------
**
** In order to sync endianess across different devices and buses a following
** convention is followed.
**         - Data in distributed buffer is always BigEndian (4321). This also includes
**         data-types that are longer than 4bytes (e.g. doubles are 7654321)
**
**         - Data in scratch buffers (device->txBuffer, device->rxBuffer) is in
**         the same format as in hw buffer (always BigEndian).
**
**
** Device access routines mrfiocDBuff_flush, mrmEvrDataRxCB, implement
** correct conversions and data reconstructions. E.g. data received over PCI/
** PCIe will be in littleEndian, but the littleEndian conversion will be 4 bytes
** wide (this means that if data in HW is 76543210 the result will be 45670123)
**
**
** -- MISSING ---------------------------------------------------------------
**
** Explicit setting of DataBuffer MODE (whether DataBuffer is shared with DBUS)
**
**
** Author: Tom Slejko
** -------------------------------------------------------------------------*/

/*
 * mrfIoc2 headers
 */

#include <stdlib.h>
#include <string.h>
/*
 *  EPICS headers
 */
#include <iocsh.h>
#include <drvSup.h>
#include <epicsExport.h>
#include <epicsEndian.h>
#include <regDev.h>
#include <dataBuffer/mrmDataBufferUser.h>
#include <errlog.h>

/*                                        */
/*        DEFINES                         */
/*                                        */

int drvMrfiocDBuffDebug = 0;
epicsExportAddress(int, drvMrfiocDBuffDebug);

#if defined __GNUC__ && __GNUC__ < 3
#define dbgPrintf(args...)  if(drvMrfiocDBuffDebug) printf(args);
#else
#define dbgPrintf(...)  if(drvMrfiocDBuffDebug) printf(__VA_ARGS__);
#endif

#if defined __GNUC__
#define _unused(x) x __attribute__((unused))
#else
#define _unused(x)
#endif

#define PROTO_START 4   // offset of the protocol ID
#define PROTO_LEN 4     // length of the protocol ID

/*
 * mrfDev reg driver private
 */
struct regDevice{
    char*                   name;            //regDevName of device
    mrmDataBufferUser*      dataBuffer;
    IOSCANPVT               ioscanpvt;
    epicsUInt8*             rxBuffer;        //pointer to 2k rx buffer
    epicsUInt8*             txBuffer;        //pointer to 2k tx buffer
    epicsUInt32             proto;           //protocol ID (4 bytes)
    epicsUInt16             usedTxBufferLen; //amount of data written in buffer (last touched byte)
    epicsStatus             receive_status;
};


/****************************************/
/*        DEVICE ACCESS FUNCIONS             */
/****************************************/

/*
 * Function will flush software scratch buffer.
 * Buffer will be copied and (if needed) its contents
 * will be converted into appropriate endiannes
 */
static void mrfiocDBuff_flush(regDevice* device)
{
    /* Copy protocol ID (big endian) */
    device->dataBuffer->put(PROTO_START, sizeof(device->proto), &device->proto);

    /* Send out the data */
    device->dataBuffer->send(false);
}


/*
 *    This callback is attached to EVR rx buffer logic.
 *    Byte order is big endian as on network.
 *    Callback then compares received protocol ID
 *    with desired protocol. If there is a match then buffer
 *    is copied into device private rxBuffer and scan IO
 *    is requested.
 */
void mrmEvrDataRxCB(size_t updated_offset, void* pvt) {
    regDevice* device = (regDevice *)pvt;

    if (device->proto != 0)
    {
        // Extract protocol ID
        epicsUInt32 receivedProtocolID;

        device->dataBuffer->get(PROTO_START, PROTO_LEN, &receivedProtocolID);
        dbgPrintf("mrmEvrDataRxCB %s: protocol ID = %d\n", device->name, receivedProtocolID);

        if (device->proto != receivedProtocolID) return;
    }
    dbgPrintf("Received new DATA at %d\n", updated_offset);
    device->dataBuffer->get(updated_offset, DataTxCtrl_segment_bytes, &device->rxBuffer[updated_offset]);

    scanIoRequest(device->ioscanpvt);
}


/****************************************/
/*            REG DEV FUNCIONS              */
/****************************************/

void mrfiocDBuff_report(regDevice* device, int _unused(level))
{
    printf("%s dataBuffer max length %u\n", device->name, DataTxCtrl_len_max - DataTxCtrl_segment_bytes);
}

/*
 * Read will make sure that the data is correctly copied into the records.
 * Since data in MRF DBUFF is big endian (since all EVGs are running on BE
 * systems) the data may need to be converted to LE. Data in rxBuffer is
 * always BE.
 */
int mrfiocDBuff_read(
        regDevice* device,
        size_t offset,
        unsigned int datalength,
        size_t nelem,
        void* pdata,
        int _unused(priority),
        regDevTransferComplete _unused(callback),
        char* user)
{
    dbgPrintf("mrfiocDBuff_read %s: from %s:0x%x len: 0x%x receive_status = %d\n",
        user, device->name, (int)offset, (int)(datalength*nelem), device->receive_status);

    if (device->receive_status) return -1;

    /* Data in buffer is in big endian byte order */
    regDevCopy(datalength, nelem, &device->rxBuffer[offset], pdata, NULL, REGDEV_LE_SWAP);

    return 0;
}

int mrfiocDBuff_write(
        regDevice* device,
        size_t offset,
        unsigned int datalength,
        size_t nelem,
        void* pdata,
        void* pmask,
        int _unused(priority),
        regDevTransferComplete _unused(callback),
        char* user)
{
    /*
     * We use offset <= 4 (that is illegal for normal use since it is occupied by protoID and delay compensation data)
     * to flush the output buffer. This eliminates the need for extra record.
     */
    if (offset < DataTxCtrl_segment_bytes-1) {
        mrfiocDBuff_flush(device);
        return 0;
    }


    /* Copy into the scratch buffer */
    /* Data in buffer is in big endian byte order */
    regDevCopy(datalength, nelem, pdata, &device->txBuffer[offset], pmask, REGDEV_LE_SWAP);
    device->dataBuffer->put(offset, datalength, &device->txBuffer[offset]);

    return 0;
}


IOSCANPVT mrfiocDBuff_getInIoscan(regDevice* device, size_t _unused(offset))
{
    return device->ioscanpvt;
}


//RegDev device definition
static const regDevSupport mrfiocDBuffSupport = {
    mrfiocDBuff_report,
    mrfiocDBuff_getInIoscan,
    NULL,
    mrfiocDBuff_read,
    mrfiocDBuff_write
};


/*
 * Initialization, this is the entry point.
 * Function is called from iocsh. Function will try
 * to find desired device (mrfName) and attach mrfiocDBuff
 * support to it.
 *
 * Args:Can not find mrf device: %s
 *         regDevName - desired name of the regDev device
 *         mrfName - name of mrfioc2 device (evg, evr, ...)
 *
 */

void mrfiocDBuffConfigure(const char* regDevName, const char* mrfName, int protocol)
{
    if (!regDevName || !mrfName) {
        errlogPrintf("usage: mrfiocDBuffConfigure \"regDevName\", \"mrfName\", [protocol]\n");
        return;
    }

    //Check if device already exists:
    if (regDevFind(regDevName)) {
        errlogPrintf("mrfiocDBuffConfigure: FATAL ERROR! device %s already exists!\n", regDevName);
        return;
    }

    regDevice* device;

    device = (regDevice*) calloc(1, sizeof(regDevice) + strlen(regDevName) + 1);
    if (!device) {
        errlogPrintf("mrfiocDBuffConfigure %s: FATAL ERROR! Out of memory!\n", regDevName);
        return;
    }
    device->name = (char*)(device + 1);
    strcpy(device->name, regDevName);

    /*
     * Create new data buffer user.
     */
    device->dataBuffer = new mrmDataBufferUser();    // TODO where to put destructor??

    if (!device->dataBuffer) {
        errlogPrintf("mrfiocDBuffConfigure %s: FAILED! Can not connect to mrf data buffer on device: %s\n", regDevName, mrfName);
        return;
    }

    if (device->dataBuffer->init(mrfName) != 0) {
        errlogPrintf("mrfiocDBuffConfigure %s: FAILED to initialize data buffer on device %s\n", regDevName, mrfName);
        delete device->dataBuffer;
        return;
    }

    epicsUInt32 maxLength = DataTxCtrl_len_max - DataTxCtrl_segment_bytes;

    device->proto = (epicsUInt32) protocol; //protocol ID
    epicsPrintf("mrfiocDBuffConfigure %s: registering to protocol %d\n", regDevName, device->proto);

    if (device->dataBuffer->supportsTx())
    {
        dbgPrintf("mrfiocDBuffConfigure %s: %s supports TX buffer. Allocating.\n",
            regDevName, mrfName);
        // Allocate the buffer memory
        device->txBuffer = (epicsUInt8*) calloc(1, maxLength);
        if (!device->txBuffer) {
            errlogPrintf("mrfiocDBuffConfigure %s: FATAL ERROR! Could not allocate TX buffer!", regDevName);
            return;
        }
    }

    if (device->dataBuffer->supportsRx())
    {
        dbgPrintf("mrfiocDBuffConfigure %s: %s supports RX buffer. Allocating and installing callback\n",
            regDevName, mrfName);
        device->rxBuffer = (epicsUInt8*) calloc(1, maxLength);
        if (!device->rxBuffer) {
            errlogPrintf("mrfiocDBuffConfigure %s: FATAL ERROR! Could not allocate RX buffer!", regDevName);
            return;
        }
        device->dataBuffer->registerInterest(DataTxCtrl_segment_bytes, maxLength, mrmEvrDataRxCB, device);
        scanIoInit(&device->ioscanpvt);
    }

    regDevRegisterDevice(regDevName, &mrfiocDBuffSupport, device, maxLength);
}

/****************************************/
/*        EPICS IOCSH REGISTRATION        */
/****************************************/

/*         mrfiocDBuffConfigure           */
static const iocshArg mrfiocDBuffConfigureDefArg0 = { "regDevName", iocshArgString};
static const iocshArg mrfiocDBuffConfigureDefArg1 = { "mrfioc2 device name", iocshArgString};
static const iocshArg mrfiocDBuffConfigureDefArg2 = { "protocol", iocshArgInt};
static const iocshArg *const mrfiocDBuffConfigureDefArgs[3] = {&mrfiocDBuffConfigureDefArg0, &mrfiocDBuffConfigureDefArg1, &mrfiocDBuffConfigureDefArg2};

static const iocshFuncDef mrfiocDBuffConfigureDef = {"mrfiocDBuffConfigure", 3, mrfiocDBuffConfigureDefArgs};

static void mrfioDBuffConfigureFunc(const iocshArgBuf* args) {
    mrfiocDBuffConfigure(args[0].sval, args[1].sval, args[2].ival);
}


/*         mrfiocDBuffFlush           */
static const iocshArg mrfiocDBuffFlushDefArg0 = { "regDevName", iocshArgString};
static const iocshArg *const mrfiocDBuffFlushDefArgs[1] = {&mrfiocDBuffFlushDefArg0};

static const iocshFuncDef mrfiocDBuffFlushDef = {"mrfiocDBuffFlush", 1, mrfiocDBuffFlushDefArgs};

static void mrfioDBuffFlushFunc(const iocshArgBuf* args) {
    regDevice* device = regDevFind(args[0].sval);
    if(!device){
        errlogPrintf("Can not find device: %s\n", args[0].sval);
        return;
    }

    mrfiocDBuff_flush(device);
}

/*        registrar            */

static int mrfiocDBuffRegistrar(void) {
    iocshRegister(&mrfiocDBuffConfigureDef, mrfioDBuffConfigureFunc);
    iocshRegister(&mrfiocDBuffFlushDef, mrfioDBuffFlushFunc);

    return 1;
}
extern "C" {
 epicsExportRegistrar(mrfiocDBuffRegistrar);
}
static int done = mrfiocDBuffRegistrar();
