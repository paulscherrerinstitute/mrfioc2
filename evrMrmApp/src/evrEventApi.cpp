#include <epicsTypes.h>
#include <errlog.h>

#define epicsExportSharedSymbols
#include "evrEventApi.h"
#include "evrMrm.h"

int evrEventNotifyAdd(const char* evrName, int event, eventCallback_t callback, void* userarg)
{
    try {
        mrf::Object *O = mrf::Object::getObject(evrName);
        if (!O) throw std::runtime_error("Not found");
        EVRMRM* evr = dynamic_cast<EVRMRM*>(O);
        if (!evr) throw std::runtime_error("Not a MRM EVR");
        evr->eventNotifyAdd(event, callback, userarg);
    } catch (std::exception& e) {
        errlogPrintf("Error in evrEventNotifyAdd %s: %s\n", evrName, e.what());
        return -1;
    }
    return 0;
}

int evrEventNotifyDel(const char* evrName, int event, eventCallback_t callback, void* userarg)
{
    try {
        mrf::Object *O = mrf::Object::getObject(evrName);
        if (!O) throw std::runtime_error("Not found");
        EVRMRM* evr = dynamic_cast<EVRMRM*>(O);
        if (!evr) throw std::runtime_error("Not a MRM EVR");
        evr->eventNotifyDel(event, callback, userarg);
    } catch (std::exception& e) {
        errlogPrintf("Error in evrEventNotifyDel %s: %s\n", evrName, e.what());
        return -1;
    }
    return 0;
}
