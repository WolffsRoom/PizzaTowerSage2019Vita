#ifndef PIZZA_VITA_THREAD_POLICY_H
#define PIZZA_VITA_THREAD_POLICY_H
#include <stdbool.h>
#include <psp2/kernel/threadmgr.h>
SceUID VitaThread_createBackground(const char*,SceKernelThreadEntry,int,int,unsigned,bool*);
bool VitaThread_applyBackground(void);
#endif
