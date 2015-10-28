//#include "mrmDataBuffer_test.h"
#include "iocsh.h"
#include "epicsExport.h"

#include "evgMrm.h"
#include "evrMrm.h"

#include "time.h"

#include "mrmDataBufferUser.h"

/*mrmDataBuffer_test::mrmDataBuffer_test()
{
}*/

mrmDataBuffer* getDataBufferFromDevice(char *device) {
    evgMrm* evg = dynamic_cast<evgMrm*>(mrf::Object::getObject(device));
    if(evg){
        return evg->getDataBuffer();
    } else {
        EVRMRM* evr = dynamic_cast<EVRMRM*>(mrf::Object::getObject(device));
        if(evr){
            return evr->getDataBuffer();
        }
    }

    return NULL;
}

/********** Send data buffer  *******/
static const iocshArg mrmDataBufferSendArg0 = { "Device", iocshArgString };
static const iocshArg mrmDataBufferSendArg1 = { "Segment", iocshArgInt };

static const iocshArg * const mrmDataBufferSendArgs[2] = { &mrmDataBufferSendArg0, &mrmDataBufferSendArg1};
static const iocshFuncDef mrmDataBufferSendDef = { "mrmDataBufferSend", 2, mrmDataBufferSendArgs };


static void mrmDataBufferSendFunc(const iocshArgBuf *args) {
   epicsInt32 segment = args[1].ival;

   mrmDataBuffer *dataBuffer = getDataBufferFromDevice(args[0].sval);
   if(dataBuffer == NULL) {
       printf("Data buffer for %s not found.\n", args[0].sval);
       return;
   }

   if(segment<0){ // reset entire buffer
       epicsUInt8 data[0x0007ff] = { 0 };
       dataBuffer->send(0, 0x0007fc, data);
   } else {
       printf("Sending using segment %d\n", segment);
       epicsUInt8 data[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
       dataBuffer->send((epicsUInt8)segment, 20, data);
   }

   /*printf("Waiting for TX complete...\n");
   dataBuffer->waitForTxComplete();
   printf("TX complete!\n");*/
}

/******************/


/********** Enable / disable data buffer  *******/
static const iocshArg mrmDataBufferEnableArg0 = { "Device", iocshArgString };
static const iocshArg mrmDataBufferEnableArg1 = { "Direction", iocshArgInt }; // 0 = Rx, other = Tx
static const iocshArg mrmDataBufferEnableArg2 = { "Enable", iocshArgInt };  // 0 = Disable, other = Enable

static const iocshArg * const mrmDataBufferEnableArgs[3] = { &mrmDataBufferEnableArg0, &mrmDataBufferEnableArg1, &mrmDataBufferEnableArg2};
static const iocshFuncDef mrmDataBufferEnableDef = { "mrmDataBufferEnable", 3, mrmDataBufferEnableArgs };


static void mrmDataBufferEnableFunc(const iocshArgBuf *args) {
    epicsUInt32 direction = args[1].ival;
    epicsUInt32 enable = args[2].ival;

    mrmDataBuffer *dataBuffer = getDataBufferFromDevice(args[0].sval);
    if(dataBuffer == NULL) {
       printf("Data buffer for %s not found.\n", args[0].sval);
       return;
    }

    if(direction == 0) { // Rx
        dataBuffer->enableRx(enable);
        printf("Enabled Rx data buffer. Status: %d\n", dataBuffer->enabledRx());
    } else { // Tx
        dataBuffer->enableTx(enable);
        printf("Enabled Tx data buffer. Status: %d\n", dataBuffer->enabledTx());
    }
}

/******************/

/********** Enable / disable evr IRQ  *******/
static const iocshArg mrmDataBufferArg0_IRQ = { "Device", iocshArgString };
static const iocshArg mrmDataBufferArg1_IRQ = { "Enable", iocshArgInt }; // 0 = Disable, neg = status, pos = enable

static const iocshArg * const mrmDataBufferArgs_IRQ[2] = { &mrmDataBufferArg0_IRQ, &mrmDataBufferArg1_IRQ};
static const iocshFuncDef mrmDataBufferDef_IRQ = { "mrmDataBufferIRQ", 2, mrmDataBufferArgs_IRQ };


static void mrmDataBufferFunc_IRQ(const iocshArgBuf *args) {
    //epicsUInt32 enable = args[1].ival;

    /*evgMrm* evg = dynamic_cast<evgMrm*>(mrf::Object::getObject(device));
    if(evg){
        return evg->getDataBuffer();
    } else {
        EVRMRM* evr = dynamic_cast<EVRMRM*>(mrf::Object::getObject(device));
        if(evr){
            evr->enableIRQ();
        }
    }*/

    EVRMRM* evr = dynamic_cast<EVRMRM*>(mrf::Object::getObject(args[0].sval));
    if(evr){
        evr->enableIRQ();
        printf("Enabler EVR IRQ\n");
    }
}

/******************/

/********** Send and receive on an offset  *******/
static const iocshArg mrmDataBufferArg0_user = { "Offset", iocshArgInt };

static const iocshArg * const mrmDataBufferArgs_user[1] = { &mrmDataBufferArg0_user};
static const iocshFuncDef mrmDataBufferDef_user = { "mrmDataBufferUser", 1, mrmDataBufferArgs_user };

//bool done;
struct cbpvt{
    mrmDataBufferUser *rx;
    size_t offset;
} cb_pvt;

void callback(size_t updated_offset, size_t length, void* pvt){
    cbpvt *str = (cbpvt *)pvt;
    mrmDataBufferUser *rx = (*str).rx;
    size_t offset = (*str).offset;
    epicsUInt8 buffer[3000];

    printf("Got update on offset %d+%d\n", updated_offset, length);
    rx->get(offset, length, (void *)&buffer);
    for(unsigned int i=0; i<length; i++){
        printf("%d, ", buffer[i]);
    }
    printf("\n");
    //done=true;
}

static void mrmDataBufferFunc_user(const iocshArgBuf *args) {
    mrmDataBufferUser *rx = new mrmDataBufferUser();
    mrmDataBufferUser *tx = new mrmDataBufferUser();
    struct timespec t;
    t.tv_nsec = 0000;
    t.tv_sec = 2;
    size_t offset = args[0].ival;
    int idx;

    if (rx->init("EVR0") || tx->init("EVG0")) {
        printf("Failed to initialize data buffer\n");
        return;
    }

    epicsUInt8 data[20] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
    //done = false;

    cb_pvt.offset = offset;
    cb_pvt.rx = rx;
    idx = rx->registerInterest(offset, 20, callback, &cb_pvt);

    tx->put(offset, 20, (void *)data);
    tx->send(true);
    printf("sleeping...");
    nanosleep(&t,&t);
    printf("DONE!\n");

    rx->removeInterest(idx);


    printf("interest removed\n");

    delete tx;
    printf("Deleted tx\n");
    delete rx;

    printf("Deleted rx tx\n");
}

/******************/

extern "C" {
    static void mrmDataBufferRegistrar() {
        iocshRegister(&mrmDataBufferSendDef, mrmDataBufferSendFunc);
        iocshRegister(&mrmDataBufferEnableDef, mrmDataBufferEnableFunc);
        iocshRegister(&mrmDataBufferDef_IRQ, mrmDataBufferFunc_IRQ);
        iocshRegister(&mrmDataBufferDef_user, mrmDataBufferFunc_user);
    }

    epicsExportRegistrar(mrmDataBufferRegistrar);
}
