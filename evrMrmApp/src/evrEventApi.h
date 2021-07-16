#ifndef EVREVENTAPI_H_INC
#define EVREVENTAPI_H_INC

#include <shareLib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*eventCallback_t)(void* userarg, epicsUInt32 event);

/* These functions return 0 on success and -1 on error */

epicsShareFunc int evrEventNotifyAdd(const char* evrName, int event, eventCallback_t callback, void* userarg);
epicsShareFunc int evrEventNotifyDel(const char* evrName, int event, eventCallback_t callback, void* userarg);

#ifdef __cplusplus
}
#endif

#endif /* EVREVENTAPI_H_INC */
