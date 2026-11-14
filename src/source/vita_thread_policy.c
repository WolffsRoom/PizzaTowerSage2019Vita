/* Adapted from DeltaruneVita's optional CapUnlocker affinity policy. */
#include "vita_thread_policy.h"
#include <stddef.h>
SceUID VitaThread_createBackground(const char* name,SceKernelThreadEntry entry,
                                   int priority,int stack,unsigned attr,bool* unlocked) {
    if(unlocked) *unlocked=false;
    SceUID id=sceKernelCreateThread(name,entry,priority,stack,attr,0x00080000,NULL);
    if(id>=0) { if(unlocked)*unlocked=true; return id; }
    return sceKernelCreateThread(name,entry,priority,stack,attr,0,NULL);
}

/* Apply policy after pthread has initialized the worker's TLS. */
bool VitaThread_applyBackground(void) {
    SceUID id=sceKernelGetThreadId();
    sceKernelChangeThreadPriority(id,0x10000100);
    if(sceKernelChangeThreadCpuAffinityMask(id,0x00080000)>=0)return true;
    sceKernelChangeThreadCpuAffinityMask(id,0);
    return false;
}
