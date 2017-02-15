#include "iocsh.h"
#include "epicsExport.h"

#include "time.h"
#include "string.h"

#include "mrmDataBuffer.h"
#include "mrmDataBufferUser.h"
#include "mrmDataBufferType.h"



mrmDataBuffer* getDataBufferFromDevice(const char *device, const char* type) {
    mrmDataBuffer *dataBuffer = NULL;
    mrmDataBufferType::type_t dataBufferType;
    bool typeFound = false;

    for(size_t t = mrmDataBufferType::type_first; t <= mrmDataBufferType::type_last; t++) {
        if(strcmp(type, mrmDataBufferType::type_string[t]) == 0) {
            dataBufferType = (mrmDataBufferType::type_t)t;
            typeFound = true;
            break;
        }
    }
    if(!typeFound) {
        fprintf(stderr, "Wrong data buffer type selected: %s\n", type);
        return NULL;
    }

    dataBuffer = mrmDataBuffer::getDataBufferFromDevice(device, dataBufferType);
    if(dataBuffer == NULL) {
        fprintf(stderr, "Data buffer type %s for %s not found.\n", mrmDataBufferType::type_string[dataBufferType], device);
    }

    return dataBuffer;
}

/********** Put to data buffer  *******/
// Fills internal buffer with 'value' starting on 'offset', writing 'length' 'values'.
// When negative 'offset' is used, entire internal buffer is zeroed.
// Use mrmDataBufferSend to actually send the data over the link.

static const iocshArg mrmDataBufferPutArg0 = { "Device", iocshArgString };
static const iocshArg mrmDataBufferPutArg1 = { "Type [230, 300]"  , iocshArgString };
static const iocshArg mrmDataBufferPutArg2 = { "Offset", iocshArgInt };
static const iocshArg mrmDataBufferPutArg3 = { "Length", iocshArgInt };
static const iocshArg mrmDataBufferPutArg4 = { "Value" , iocshArgInt };

static const iocshArg * const mrmDataBufferPutArgs[5] = { &mrmDataBufferPutArg0, &mrmDataBufferPutArg1, &mrmDataBufferPutArg2, &mrmDataBufferPutArg3, &mrmDataBufferPutArg4};
static const iocshFuncDef mrmDataBufferDef_put = { "mrmDataBufferPut", 5, mrmDataBufferPutArgs };

epicsUInt8 data[0x0007ff] = { 0 };
static void mrmDataBufferFunc_put(const iocshArgBuf *args) {
   epicsInt32 offset = args[2].ival;
   epicsInt32 length = args[3].ival;
   epicsInt32 value = args[4].ival;
   epicsInt32 i;

   mrmDataBuffer *dataBuffer = getDataBufferFromDevice(args[0].sval, args[1].sval);
   if(dataBuffer == NULL) return;

   if(offset < 0) { // reset entire buffer
       for(i=0; i<0x0007ff; i++) {
           data[i] = 0;
       }
   } else {
       for(i=offset; i<offset + length; i++) {
           data[i] = value;
       }
   }
}

/******************/

/********** Send data buffer  *******/
// Sends out the buffer from 'offset' to 'offset'+'length'. Data should be previously filled with mrmDataBufferPut.
// When negative 'offset' is used, zeroes are send.

static const iocshArg mrmDataBufferSendArg0 = { "Device", iocshArgString };
static const iocshArg mrmDataBufferSendArg1 = { "Type [230, 300]"  , iocshArgString };
static const iocshArg mrmDataBufferSendArg2 = { "Offset", iocshArgInt };
static const iocshArg mrmDataBufferSendArg3 = { "Length", iocshArgInt };

static const iocshArg * const mrmDataBufferSendArgs[4] = { &mrmDataBufferSendArg0, &mrmDataBufferSendArg1, &mrmDataBufferSendArg2, &mrmDataBufferSendArg3};
static const iocshFuncDef mrmDataBufferDef_send = { "mrmDataBufferSend", 4, mrmDataBufferSendArgs };

static void mrmDataBufferFunc_send(const iocshArgBuf *args) {
   epicsInt32 offset = args[2].ival;
   epicsInt32 length = args[3].ival;

   mrmDataBuffer *dataBuffer = getDataBufferFromDevice(args[0].sval, args[1].sval);
   if(dataBuffer == NULL) return;

   if(offset<0){ // send all zeroes
       epicsUInt8 zeroData[0x0007ff] = { 0 };
       dataBuffer->send(0, 0x0007fc, zeroData);
   } else {
       length += offset - ((offset / 16) * 16);
       printf("Sending using segment %d and length %d\n", offset / 16, length);
       dataBuffer->send((epicsUInt8)offset / 16, (epicsUInt16)length, &data[(offset / 16) * 16]);
   }
}

/******************/

/********** Read data buffer  *******/
// Reads the content of the data buffer memory from 'offset' to 'offset'+'length'.

static const iocshArg mrmDataBufferArg0_read = { "Device", iocshArgString };
static const iocshArg mrmDataBufferArg1_read = { "Type [230, 300]"  , iocshArgString };
static const iocshArg mrmDataBufferArg2_read = { "offset", iocshArgInt };
static const iocshArg mrmDataBufferArg3_read = { "length", iocshArgInt };

static const iocshArg * const mrmDataBufferArgs_read[4] = { &mrmDataBufferArg0_read, &mrmDataBufferArg1_read, &mrmDataBufferArg2_read, &mrmDataBufferArg3_read};
static const iocshFuncDef mrmDataBufferDef_read = { "mrmDataBufferRead", 4, mrmDataBufferArgs_read };


static void mrmDataBufferFunc_read(const iocshArgBuf *args) {

    mrmDataBuffer *dataBuffer = getDataBufferFromDevice(args[0].sval, args[1].sval);
    if(dataBuffer == NULL) return;

    dataBuffer->read(args[1].ival, args[2].ival);
}

/******************/

/********** Enable / disable Rx IRQ  *******/
// Sets first, second, third or forth 32bits of the segment IRQ register

static const iocshArg mrmDataBufferArg0_IRQ = { "Device", iocshArgString };
static const iocshArg mrmDataBufferArg1_IRQ = { "segments", iocshArgInt }; // 0 - 4
static const iocshArg mrmDataBufferArg2_IRQ = { "mask", iocshArgInt }; // mask to apply

static const iocshArg * const mrmDataBufferArgs_IRQ[3] = { &mrmDataBufferArg0_IRQ, &mrmDataBufferArg1_IRQ, &mrmDataBufferArg2_IRQ};
static const iocshFuncDef mrmDataBufferDef_IRQ = { "mrmDataBufferIRQ", 3, mrmDataBufferArgs_IRQ };


static void mrmDataBufferFunc_IRQ(const iocshArgBuf *args) {
    epicsUInt32 i = args[1].ival;
    epicsUInt32 mask = args[2].ival;

    mrmDataBuffer *dataBuffer = getDataBufferFromDevice(args[0].sval, mrmDataBufferType::type_string[mrmDataBufferType::type_300]);
    if(dataBuffer == NULL) return;

    dataBuffer->setSegmentIRQ(i, mask);
}

/******************/

/********** Enable / disable Rx flags  *******/
// Sets first, second, third or forth 32bits of the segment Rx register

static const iocshArg mrmDataBufferArg0_Rx = { "Device", iocshArgString };
static const iocshArg mrmDataBufferArg1_Rx = { "segmens", iocshArgInt }; // 0 - 4
static const iocshArg mrmDataBufferArg2_Rx = { "mask", iocshArgInt }; // mask to apply

static const iocshArg * const mrmDataBufferArgs_Rx[3] = { &mrmDataBufferArg0_Rx, &mrmDataBufferArg1_Rx, &mrmDataBufferArg2_Rx};
static const iocshFuncDef mrmDataBufferDef_Rx = { "mrmDataBufferRx", 3, mrmDataBufferArgs_Rx };


static void mrmDataBufferFunc_Rx(const iocshArgBuf *args) {
    epicsUInt32 i = args[1].ival;
    epicsUInt32 mask = args[2].ival;

    mrmDataBuffer *dataBuffer = getDataBufferFromDevice(args[0].sval, mrmDataBufferType::type_string[mrmDataBufferType::type_300]);
    if(dataBuffer == NULL) return;

    dataBuffer->setRx(i, mask);
}

/******************/


/********** Print IRQ, checksum, overflow and Rx registers  *******/
static const iocshArg mrmDataBufferArg0_print = { "Device", iocshArgString };

static const iocshArg * const mrmDataBufferArgs_print[1] = { &mrmDataBufferArg0_print};
static const iocshFuncDef mrmDataBufferDef_print = { "mrmDataBufferPrint", 1, mrmDataBufferArgs_print };


static void mrmDataBufferFunc_print(const iocshArgBuf *args) {

    mrmDataBuffer *dataBuffer = getDataBufferFromDevice(args[0].sval, mrmDataBufferType::type_string[mrmDataBufferType::type_300]);
    if(dataBuffer == NULL) return;

    dataBuffer->printRegs();
}

/******************/

extern "C" {
    static void mrmDataBufferRegistrar() {
        iocshRegister(&mrmDataBufferDef_send, mrmDataBufferFunc_send);
        iocshRegister(&mrmDataBufferDef_put, mrmDataBufferFunc_put);
        iocshRegister(&mrmDataBufferDef_read, mrmDataBufferFunc_read);
        iocshRegister(&mrmDataBufferDef_IRQ, mrmDataBufferFunc_IRQ);
        iocshRegister(&mrmDataBufferDef_Rx, mrmDataBufferFunc_Rx);
        iocshRegister(&mrmDataBufferDef_print, mrmDataBufferFunc_print);
    }

    epicsExportRegistrar(mrmDataBufferRegistrar);
}
