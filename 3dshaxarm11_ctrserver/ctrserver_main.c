#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <ctr/types.h>
#include <ctr/svc.h>
#include <ctr/srv.h>
#include <ctr/GSP.h>
#include <ctr/APT.h>
#include <ctr/HID.h>
#include <ctr/CSND.h>
#include <ctr/AC.h>
#include <ctr/CFGNOR.h>
#include <ctr/SOC.h>

#include "network.h"

extern u64 PROCESSNAME;//Name of the process this code is running under, if set by the code-loader.

extern u32 *fake_heap_start;
extern u32 *fake_heap_end;

extern Handle srvHandle;

u32* gxCmdBuf = NULL;

Handle gspEvent, gspSharedMemHandle;

void gspGpuInit()
{
	gspInit();

	GSPGPU_AcquireRight(NULL, 0x0);
	GSPGPU_SetLcdForceBlack(NULL, 0x0);

	//setup our gsp shared mem section
	u8 threadID;
	svc_createEvent(&gspEvent, 0x0);
	GSPGPU_RegisterInterruptRelayQueue(NULL, gspEvent, 0x1, &gspSharedMemHandle, &threadID);
	svc_mapMemoryBlock(gspSharedMemHandle, 0x10002000, 0x3, 0x10000000);

	//wait until we can write stuff to it
	svc_waitSynchronization1(gspEvent, 0x55bcb0);

	//GSP shared mem : 0x2779F000
	gxCmdBuf=(u32*)(0x10002000+0x800+threadID*0x200);
}

void gspGpuExit()
{
	GSPGPU_UnregisterInterruptRelayQueue(NULL);

	//unmap GSP shared mem
	svc_unmapMemoryBlock(gspSharedMemHandle, 0x10002000);
	svc_closeHandle(gspSharedMemHandle);
	svc_closeHandle(gspEvent);
	
	gspExit();
}

Result _ACU_WaitInternetConnection()
{
	Handle servhandle = 0;
	Result ret=0;
	u32 outval=0;

	if((ret = srv_getServiceHandle(NULL, &servhandle, "ac:u"))!=0)return ret;

	while(1)
	{
		ret = ACU_GetWifiStatus(servhandle, &outval);
		if(ret==0 && outval!=0)break;
	}

	svc_closeHandle(servhandle);

	return ret;
}


int main(int argc, char **argv)
{
	Result ret=0;
	void (*funcptr)(u32);
	u32 *gspheap = NULL;
	u32 gspheap_size=0;
	u32 heap_size = 0;
	u32 tmp=0;

	if(PROCESSNAME == 0x706c64)svc_sleepThread(10000000000LL);//Delay 10 seconds when running under the dlp module.

	ret = initSrv();
	if(ret!=0)
	{
		((u32*)0x94000000)[0x800>>2] = ret;
	}

	if(PROCESSNAME != 0x454d414e434f5250LL)//This is only executed when the PROCESSNAME is not set to the default "PROCNAME" string.
	{
		if(PROCESSNAME == 0x706c64)//"dlp"
		{
			gspheap_size = 0x2000;
			if(*((u8*)0x1FF80030) >= 6)gspheap_size = 0x500000;//New3DS
			gspInit();
		}
		else
		{
			gspheap_size = 0x500000;
			//aptInit(APPID_WEB);
			//aptSetupEventHandler();

			GSPGPU_AcquireRight(NULL, 0);
			gspGpuInit();
		}
	}
	else
	{
		gspheap_size = 0x500000;
		aptInit(APPID_APPLICATION);
		//aptSetupEventHandler();

		//GSPGPU_AcquireRight(NULL, 0);
		gspGpuInit();
	}

	ret = svc_controlMemory((u32*)&gspheap, 0, 0, gspheap_size, 0x10003, 3);
	if(ret!=0)*((u32*)0x58000000) = ret;

	heap_size = 0x00088000;

	ret = svc_controlMemory(&tmp, 0x08000000, 0, heap_size, 0x3, 3);
	if(ret!=0)*((u32*)0x58000004) = ret;

	fake_heap_start = (u32*)0x08048000;
	fake_heap_end = (u32*)(0x08000000 + heap_size);

	_ACU_WaitInternetConnection();

	CFGNOR_Initialize(1);

	ret = hidInit(NULL);
	if(ret!=0)
	{
		funcptr = (void*)0x44444948;
		funcptr(ret);
	}

	ret = CSND_initialize(NULL);
	if(ret!=0)
	{
		funcptr = (void*)0x444e5343;
		funcptr(ret);
	}

	ret = SOC_Initialize((u32*)0x08000000, 0x48000);
	if(ret!=0)
	{
		((u32*)0x84000000)[1] = ret;
	}

	network_stuff(gspheap, gspheap_size);

	while(1);

	return 0;
}

