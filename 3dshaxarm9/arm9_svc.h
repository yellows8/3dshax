#ifndef ARM9_SVC_H
#define ARM9_SVC_H

void launchcode_kernelmode(void*);

u32 svcSignalEvent();
void svcFlushProcessDataCache(u32*, u32);
u64 svcGetSystemTick();
u32 svcCreateThread(u32 *threadhandle, void* entrypoint, u32 arg, u32 *stacktop, s32 threadpriority, s32 processorid);
void svcExitThread();
void svcSleepThread(s64 nanoseconds);

#endif
