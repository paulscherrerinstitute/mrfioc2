#include "iocsh.h"
#include <epicsExport.h>
#include "mrmDataBufferObj.h"


const char *mrmDataBufferObj::OBJECT_NAME = ":DataBuffer"; // appended to device name for use in mrfioc2 object model


mrmDataBufferObj::mrmDataBufferObj(const std::string &parentName, mrmDataBuffer &dataBuffer):
    mrf::ObjectInst<mrmDataBufferObj>(parentName+OBJECT_NAME),
    m_data_buffer(dataBuffer)
{
    for(epicsUInt32 i=0; i<256; i++) {
        fakeData[i] = i;
    }
}

// Overflow
epicsUInt32 mrmDataBufferObj::getOverflowCount(epicsUInt32 *wf, epicsUInt32 l) const
{
    epicsUInt32 *count;
    epicsUInt32 nElems = m_data_buffer.getOverflowCount(&count);

    if (l < nElems) {
        nElems = l;
    }

    for (size_t i=0; i< nElems; i++) {
        wf[i] = count[i];
    }
    return nElems;
}

epicsUInt32 mrmDataBufferObj::getOverflowCountSum() const
{
    epicsUInt32 *count;
    epicsUInt32 nElems = m_data_buffer.getOverflowCount(&count);

    return getCountSum(count, nElems);

}

// Checksum
epicsUInt32 mrmDataBufferObj::getChecksumCount(epicsUInt32 *wf, epicsUInt32 l) const
{
    epicsUInt32 *count;
    epicsUInt32 nElems = m_data_buffer.getChecksumCount(&count);

    if (l < nElems) {
        nElems = l;
    }

    for (size_t i=0; i< nElems; i++) {
        wf[i] = count[i];
    }
    return nElems;
}

epicsUInt32 mrmDataBufferObj::getChecksumCountSum() const
{
    epicsUInt32 *count;
    epicsUInt32 nElems = m_data_buffer.getChecksumCount(&count);

    return getCountSum(count, nElems);

}

// Other
bool mrmDataBufferObj::supportsTx() const
{
    return m_data_buffer.supportsTx();
}

bool mrmDataBufferObj::supportsRx() const
{
    return m_data_buffer.supportsRx();
}

void mrmDataBufferObj::enableRx(bool en)
{
    m_data_buffer.enableRx(en);
}

bool mrmDataBufferObj::enabledRx() const
{
    return m_data_buffer.enabledRx();
}

void mrmDataBufferObj::report() const
{
    // could also print IRQ seg reg (registered interest)....

    epicsUInt32 *overflow, *checksum, sumOverflow=0, sumChecksum=0;
    epicsUInt32 nElems = m_data_buffer.getOverflowCount(&overflow);
    m_data_buffer.getChecksumCount(&checksum);
    epicsUInt32 i;

    printf("\tSegment\tOverflow count\tChecksum count\n");
    for (i=0; i<nElems; i++) {
        printf("\t%7d\t%7d\t\t%7d\n", i, overflow[i], checksum[i]);
        sumOverflow += overflow[i];
        sumChecksum += checksum[i];
    }
    printf("\t-------------------------------------\n");
    printf("\tSum:\t%7d\t\t%7d\n", sumOverflow, sumChecksum);

    printf("\n\tData buffer transmission: ");
    if (supportsTx()) {
       printf("supported\n");
    }
    else {
        printf("not supported\n");
    }

    printf("\tData buffer reception: ");
    if (supportsRx()) {
       printf("supported");
    }
    else {
        printf("not supported");
    }

    if(enabledRx()) {
        printf(" and enabled\n");
    }
    else {
        printf(" and not enabled\n");
    }
}

void mrmDataBufferObj::send(bool dummy)
{
    m_data_buffer.send(2, 32, fakeData);
}


// Private function(s)
epicsUInt32 mrmDataBufferObj::getCountSum(epicsUInt32 *count, epicsUInt32 nElems) const
{
    epicsUInt32 sum = 0;

    for (size_t i=0; i<nElems; i++) {
        sum += count[i];
    }

    return sum;
}


/**
 * Construct mrfioc2 objects for linking to EPICS records
 **/

OBJECT_BEGIN(mrmDataBufferObj) {

    OBJECT_PROP1("OverflowCount",&mrmDataBufferObj::getOverflowCount);
    OBJECT_PROP1("OverflowCount",&mrmDataBufferObj::getOverflowCountSum);
    OBJECT_PROP1("ChecksumCount",&mrmDataBufferObj::getChecksumCount);
    OBJECT_PROP1("ChecksumCountSum",&mrmDataBufferObj::getChecksumCountSum);
    //OBJECT_PROP1("RegisteredInterest", &mrmDataBufferObj::getRegisteredInterest);
    OBJECT_PROP1("SupportsTx", &mrmDataBufferObj::supportsRx);
    OBJECT_PROP1("SupportsRx", &mrmDataBufferObj::supportsTx);
    OBJECT_PROP2("EnableRx", &mrmDataBufferObj::enabledRx, &mrmDataBufferObj::enableRx);

    OBJECT_PROP2("Trigger", &mrmDataBufferObj::supportsRx, &mrmDataBufferObj::send);

} OBJECT_END(mrmDataBufferObj)



/**
 * IOCSH functions
 **/

/********** Report  *******/
static const iocshArg   mrmDataBufferObjArg0_report = { "Device", iocshArgString };

static const iocshArg * const mrmDataBufferObjArgs_report[1] = { &mrmDataBufferObjArg0_report};
static const iocshFuncDef mrmDataBufferObjDef_report = { "mrmDataBufferReport", 1, mrmDataBufferObjArgs_report};


static void mrmDataBufferObjFunc_report(const iocshArgBuf *args) {
    if(args[0].sval == NULL){
        printf("Usage: mrmDataBufferReport Device\n\t"   \
               "Device = name of the timing card (eg.: EVR0, EVG0, ...)\n");
        return;
    }

    std::string device = args[0].sval;


    printf("Report for %s data buffer\n", device.c_str());
    device.append(mrmDataBufferObj::OBJECT_NAME);

    mrmDataBufferObj* dataBuffer = dynamic_cast<mrmDataBufferObj*>(mrf::Object::getObject(device.c_str()));
    if(!dataBuffer){
        printf("Device <%s> with data buffer does not exist!\n", args[0].sval);
        return;
    }

    dataBuffer->report();
}


/******************/


extern "C" {
    static void mrmDataBufferObjRegistrar() {
        iocshRegister(&mrmDataBufferObjDef_report, mrmDataBufferObjFunc_report);
    }

    epicsExportRegistrar(mrmDataBufferObjRegistrar);
}
