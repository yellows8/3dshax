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

/*Result srv_cmd6(Handle *handleptr, char *str, u32 len, Handle handle)
{
	if(!handleptr)handleptr=&srvHandle;
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x000600c2; //request header code
	strncpy((char*)&cmdbuf[1], str, 8);
	cmdbuf[3] = len;
	cmdbuf[4] = 0;
	cmdbuf[5] = handle;

	Result ret=0;
	if((ret=svc_sendSyncRequest(*handleptr)))return ret;

	return cmdbuf[1];
}*/

int main(int argc, char **argv)
{
	Result ret=0;
	void (*funcptr)(u32);
	u32 *gspheap = NULL;
	u32 gspheap_size=0;
	u32 heap_size = 0;
	u32 tmp=0;
	u32 i;
	//u64 name = 0x4141414141414141ULL;

	if(PROCESSNAME == 0x706c64)svc_sleepThread(10000000000LL);//Delay 10 seconds when running under the dlp module.

	ret = /*svc_connectToPort(&srvHandle, "srv:");*/initSrv();
	if(ret!=0)
	{
		((u32*)0x94000000)[0x800>>2] = ret;
	}

	ret = gspInit();
	if(ret!=0)*((u32*)0x58000500) = ret;

	if(PROCESSNAME != 0x454d414e434f5250LL)//This is only executed when the PROCESSNAME is not set to the default "PROCNAME" string.
	{
		if(PROCESSNAME == 0x706c64)//"dlp"
		{
			gspheap_size = 0x1000;
		}
		else
		{
			gspheap_size = 0xc00000;
			aptInit(APPID_WEB);
			//aptSetupEventHandler();

			GSPGPU_AcquireRight(NULL, 0);
		}
	}
	else
	{
		gspheap_size = 0xc00000;
		aptInit(APPID_APPLICATION);
		//aptSetupEventHandler();

		GSPGPU_AcquireRight(NULL, 0);
	}

	ret = svc_controlMemory((u32*)&gspheap, 0, 0, gspheap_size, 0x10003, 3);
	if(ret!=0)*((u32*)0x58000000) = ret;

	heap_size = 0x00088000;

	ret = svc_controlMemory(&tmp, 0x08000000, 0, heap_size, 0x3, 3);
	if(ret!=0)*((u32*)0x58000004) = ret;

	fake_heap_start = (u32*)0x08048000;
	fake_heap_end = (u32*)(0x08000000 + heap_size);

	/*for(i=0; i<0x1000; i++)
	{
		((u32*)0x08060000)[i] = srv_cmd6(&srvHandle, (char*)&name, 8, srvHandle);
		name++;
	}*/

	ACU_WaitInternetConnection();

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

