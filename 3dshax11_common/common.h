#include "srv.h"

u32 launchcode_kernelmode(void*, u32 param);
u32 *get_threadlocalstorage();
u32 *get_threadcmdbuf();
int svcControlMemory(u32 **outaddr, u32 addr0, u32 addr1, u32 size, u32 memorytype, u32 permissions);
int svcSendSyncRequest(u32);
int svcGetProcessId(u32 *procid, u32 KProcess_handle);
int svc7c(u32 Type, u32 Param0, u32 Param1, u32 Param2);
int svcSignalEvent(u32 handle);
int svcCloseHandle(u32 handle);
int svcConnectToPort(u32 *handle, char *portname);
u64 svcGetSystemTick();
int svcCreateMemoryBlock(u32 *handle, u32 addr, u32 size, u32 mypermission, u32 otherpermission);
int svcMapMemoryBlock(u32 handle, u32 *addr, u32 mypermissions, u32 otherpermission);
int svcControlProcessMemory(u32 prochandle, u32 Addr0, u32 Addr1, u32 Size, u32 Type, u32 Permissions);
int svcWaitSynchronizationN(s32* out, u32* handles, s32 handlecount, bool waitAll, s64 nanoseconds);
int svcReleaseMutex(u32 handle);
int svcCreateThread(u32 *threadhandle, void* entrypoint, void* arg, u32 *stacktop, s32 threadpriority, s32 processorid);
void svcExitThread();
void svcSleepThread(s64 nanoseconds);
void setup_fpscr();

void call_arbitaryfuncptr(void* funcptr, u32 *regdata);

void jumptomain();
void jumptomain_loopend();

void irq_disable();
void irq_enable();

void instructionff();
void DCInvalidateAll();
void DCFlushAll();

