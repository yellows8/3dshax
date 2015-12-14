#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

#include <3ds.h>

#include "network.h"

extern u64 PROCESSNAME;//Name of the process this code is running under, if set by the code-loader.

Result initsrvhandle_allservices();

extern char* fake_heap_start;
extern char* fake_heap_end;
static u32 linearheapsize;

extern u32 __ctru_heap;
extern u32 __ctru_heap_size;
extern u32 __ctru_linear_heap;
extern u32 __ctru_linear_heap_size;

void __system_allocateHeaps() {
	u32 tmp=0;
	Result ret;
	u32 heapsize;

	if(PROCESSNAME != 0x454d414e434f5250LL)//This is only executed when the PROCESSNAME is not set to the default "PROCNAME" string.
	{
		if(PROCESSNAME == 0x706c64)//"dlp"
		{
			linearheapsize = 0x2000;
			if(*((u8*)0x1FF80030) >= 6)linearheapsize = 0x500000;//New3DS
		}
		else
		{
			linearheapsize = 0x500000;
		}
	}
	else
	{
		linearheapsize = 0x500000;
	}

	heapsize = 0x00088000;
	__ctru_heap_size = heapsize;
	__ctru_linear_heap_size = linearheapsize;

	// Allocate the application heap
	__ctru_heap = 0x08000000;
	ret = svcControlMemory(&tmp, __ctru_heap, 0x0, heapsize, MEMOP_ALLOC, 0x3);

	if(ret!=0)
	{
		((u32*)0x99000000)[0] = ret;
	}

	// Allocate the linear heap
	ret = svcControlMemory(&__ctru_linear_heap, 0x0, 0x0, linearheapsize, MEMOP_ALLOC_LINEAR, 0x3);

	if(ret!=0)
	{
		((u32*)0x99000000)[1] = ret;
	}

	// Set up newlib heap
	fake_heap_start = (char*)__ctru_heap;
	fake_heap_end = fake_heap_start + heapsize;
}

void __appInit() {
	Result ret;
	// Initialize services

	ret = initsrvhandle_allservices();
	if(ret!=0)
	{
		((u32*)0x94000000)[0x800>>2] = ret;
	}

	fsInit();
}

int main(int argc, char **argv)
{
	Result ret=0;
	u32 *ptr;

	if(PROCESSNAME == 0x706c64)svcSleepThread(10000000000LL);//Delay 10 seconds when running under the dlp module.

	if(PROCESSNAME != 0x454d414e434f5250LL)//This is only executed when the PROCESSNAME is not set to the default "PROCNAME" string.
	{
		if(PROCESSNAME == 0x706c64)//"dlp"
		{
			gspInit();
		}
		else
		{
			//aptInit();

			gfxInitDefault();
		}
	}
	else
	{
		aptInit();

		gfxInitDefault();
	}

	acWaitInternetConnection();

	ptr = memalign(0x1000, 0x48000);
	if(ptr==NULL)((u32*)0x84000000)[2] = 0x50505050;

	ret = socInit(ptr, 0x48000);
	if(ret!=0)
	{
		((u32*)0x84000000)[1] = ret;
	}

	network_stuff((u32*)__ctru_linear_heap, linearheapsize);

	while(1);

	return 0;
}

