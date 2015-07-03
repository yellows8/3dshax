#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

#include <3ds.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#include "am.h"

#include "ctrclient.h"

#include "armdebug.h"

#include "auth_bin.h"

typedef struct
{
	volatile int sockfd;
} ctrserver;

static Handle pxidev_handle=0;

static int listen_sock;

static u32 *net_payloadptr, net_payload_maxsize;
static ctrserver net_server;
static u32 net_kernelmode_paramblock[4];

static Handle threadevents[3];
static Handle threadhandle;
static u32 thread_initialized = 0, *thread_stackbottom = NULL, thread_threadpriority = 0, thread_processorid = 0, selected_cmdhandler_thread = 0;
static u64 thread_cmdreplyevent_timeout = U64_MAX;
static u32 net_thread_paramblock[4];

static u32 arm9access_available = 0;

static u32 amserv_available = 0;
static Handle amlockhandle;

extern Handle srvHandle;
extern Handle aptLockHandle;
extern Handle aptuHandle;
extern Handle gspGpuHandle;
extern Handle CFGNOR_handle;
extern Handle hidHandle;
extern Handle CSND_handle;
extern Handle SOCU_handle;

extern u32* gxCmdBuf;

extern u32 *arm11kernel_textvaddr;

void network_cmdhandler_thread(void *arg);

int ctrserver_recvdata(ctrserver *server, u8 *buf, int size);
int ctrserver_senddata(ctrserver *server, u8 *buf, int size);

u32 launchcode_kernelmode(void*, u32 param);
void call_arbitaryfuncptr(void* funcptr, u32 *regdata);

s32 svcStartInterProcessDma(u32* dmahandle, u32 dstProcess, u32* dst, u32 srcProcess, u32* src, u32 size, u32 *config);
s32 svcGetDmaState(u32 *state, u32 dmahandle);

void kernelmode_cachestuff();

Result FSUSER_ControlArchive(Handle *handle, FS_archive archive)//This is based on code from smea.
{
	u32* cmdbuf=getThreadCommandBuffer();

	u32 b1 = 0, b2 = 0;

	cmdbuf[0]=0x080d0144;
	cmdbuf[1]=archive.handleLow;
	cmdbuf[2]=archive.handleHigh;
	cmdbuf[3]=0x0;
	cmdbuf[4]=0x1; //buffer1 size
	cmdbuf[5]=0x1; //buffer1 size
	cmdbuf[6]=0x1a;
	cmdbuf[7]=(u32)&b1;
	cmdbuf[8]=0x1c;
	cmdbuf[9]=(u32)&b2;

	Result ret=0;
	if((ret=svcSendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result am_init()//This is based on code by smea.
{
	Result ret;
	Handle *tmphandle=0;

	if(amserv_available)return 0;

	ret = amInit();
	if(ret!=0)return ret;

	tmphandle = amGetSessionHandle();

	ret = AM_Initialize(*tmphandle, &amlockhandle);
	if(ret!=0)return ret;

	ret = svcWaitSynchronization(amlockhandle, 0xffffffffffffffff);
	if(ret==0)amserv_available = 1;

	return ret;
}

Result pxidev_cmd0(u32 cmdid, u32 *buf, u32 *bufsize)
{
	Result ret=0;
	u32 *cmdbuf = getThreadCommandBuffer();
	u32 bufaddr;

	if(cmdid==0)
	{
		cmdbuf[0] = 0x00000040;
		cmdbuf[1] = 0x454e4f4e;//"NONE"
	}
	else
	{
		bufaddr = (u32)buf;
		/*if(bufaddr < 0x1c000000)
		{
			bufaddr+= 0x0c000000;
		}
		else
		{
			bufaddr -= 0x10000000;
		}*/

		cmdbuf[0] = 0x000000c2;
		cmdbuf[1] = 0x43565253;//"SRVC"
		cmdbuf[2] = cmdid;
		cmdbuf[3] = *bufsize;
		cmdbuf[4] = (net_payload_maxsize<<8) | 4;
		cmdbuf[5] = bufaddr;
	}

	if((ret = svcSendSyncRequest(pxidev_handle))!=0)return ret;

	if(cmdid)*bufsize = cmdbuf[2];

	return (Result)cmdbuf[1];
}

Result ctrserver_arm9cmd(u32 cmdid, u32 *buf, u32 *bufsize)
{
	Result ret=0;

	if(arm9access_available==0)return -5;

	if(*bufsize)
	{
		ret = GSPGPU_FlushDataCache(NULL, (u8*)buf, *bufsize);
		if(ret<0)return ret;
	}

	ret = pxidev_cmd0(cmdid, buf, bufsize);
	if(ret<0)return ret;
	
	if(*bufsize)
	{
		ret = GSPGPU_InvalidateDataCache(NULL, (u8*)buf, *bufsize);
		if(ret<0)return ret;
	}

	return 0;
}

static Result init_arm9access()
{
	Result ret=0;

	if((ret = srvGetServiceHandle(&pxidev_handle, "pxi:dev"))!=0)return ret;

	if((ret = pxidev_cmd0(0, NULL, 0)) != 0)return ret;

	arm9access_available = 1;

	return 0;
}

int ctrserver_handlecmd_common(u32 cmdid, u32 *buf, u32 *bufsize)
{
	u32 *addr;
	u16 *ptr16;
	u8 *ptr8;
	u32 size;
	u32 tmpsize=0, tmpsize2=0;
	u32 pos, bufpos = 0;
	//int ret=0;
	int rw=0;

	cmdid &= 0xff;

	if(cmdid>=0x1 && cmdid<0x9)
	{
		rw = 0;//0=read, 1=write
		if((cmdid & 0xff)<0x05)rw = 1;

		if((rw==1 && (cmdid & 0xff)<0x4) || (rw==0 && (cmdid & 0xff)<0x8))
		{
			if(*bufsize != (4 + rw*4))return 0;

			addr = (u32*)buf[0];
			ptr16 = (u16*)addr;
			ptr8 = (u8*)addr;

			if((cmdid & 0xff) == 0x01 || (cmdid & 0xff) == 0x05)size = 1;
			if((cmdid & 0xff) == 0x02 || (cmdid & 0xff) == 0x06)size = 2;
			if((cmdid & 0xff) == 0x03 || (cmdid & 0xff) == 0x07)size = 4;

			buf[0] = 0;

			if(rw==0)*bufsize = 4;

			if(size==1)//Don't use memcpy here since that would use byte-copies when u16/u32 copies were intended.
			{
				if(rw==0)buf[0] = *ptr8;
				if(rw==1)*ptr8 = buf[1];
			}
			else if(size==2)
			{
				if(rw==0)buf[0] = *ptr16;
				if(rw==1)*ptr16 = buf[1];
			}
			else if(size==4)
			{
				if(rw==0)buf[0] = *addr;
				if(rw==1)*addr = buf[1];
			}

			if(rw==1)*bufsize = 0;
		}
		else//cmd 0x04 = arbitary-size read, cmd 0x08 = arbitary-size write
		{
			if(rw==0 && *bufsize != 8)return 0;
			if(rw==1 && *bufsize < 5)return 0;

			addr = (u32*)buf[0];

			if(rw==0)
			{
				size = buf[1];
				*bufsize = size;
			}
			if(rw==1)
			{
				size = *bufsize - 4;
				*bufsize = 0;
			}

			if(rw==0)memcpy(buf, addr, size);
			if(rw==1)memcpy(addr, &buf[1], size);
		}

		return 0;
	}

	if(cmdid==0xe)
	{
		if(*bufsize != 12)return 0;

		addr = (u32*)buf[0];

		memset(addr, buf[1], buf[2]);

		*bufsize = 0;

		return 0;
	}

	if(cmdid==0x30)
	{
		if(*bufsize != 0x20)return 0;

		call_arbitaryfuncptr((void*)buf[0], &buf[1]);

		return 0;
	}

	if(cmdid==0x32 || cmdid==0x33)
	{
		if(*bufsize < 8)return 0;

		rw = 0;//read
		if(cmdid==0x33)rw = 1;//rw val1 = write

		if(!rw)size = buf[2];
		if(rw)size = *bufsize - 8;
		addr = (u32*)buf[0];
		tmpsize = buf[1];

		ptr16 = (u16*)addr;
		ptr8 = (u8*)addr;

		bufpos = 0;
		if(rw)bufpos = 0x8;

		if(!rw)*bufsize = size;
		if(rw)*bufsize = 0;

		while(size)
		{
			for(pos=0; pos<tmpsize; pos+=4)//Don't use memcpy here for sizes >=4, because that uses byte-copies when size is <16(newlib memcpy).
			{
				tmpsize2 = tmpsize - pos;
				if(tmpsize2 >= 4)
				{
					if(!rw)buf[bufpos>>2] = addr[pos>>2];
					if(rw)addr[pos>>2] = buf[bufpos>>2];
				}
				else if(tmpsize2 == 2)
				{
					if(!rw)buf[bufpos>>2] = ptr16[pos>>1];
					if(rw)ptr16[pos>>1] = buf[bufpos>>2];
				}
				else
				{
					if(!rw)memcpy(&buf[bufpos>>2], &ptr8[pos], tmpsize2);
					if(rw)memcpy(&ptr8[pos], &buf[bufpos>>2], tmpsize2);
				}

				bufpos+=4;
				size-=4;
			}
		}

		return 0;
	}

	return -2;
}

int net_kernelmode_handlecmd(u32 param)
{
	u32 cmdid;
	u32 *buf;
	u32 *bufsize;
	int ret=0;
	u32 *ptr;
	u32* (*funcptr)() = NULL;
	vu32 *regaddr;
	u32 val, tmpval, count, count2, pos, size, i;

	cmdid = net_kernelmode_paramblock[0];
	buf = (u32*)net_kernelmode_paramblock[1];
	bufsize = (u32*)net_kernelmode_paramblock[2];

	cmdid &= 0xff;

	if(((u32)arm11kernel_textvaddr) != 0x5458544b)
	{
		funcptr = (void*)(&arm11kernel_textvaddr[0x1808>>2]);
	}
	else
	{
		*bufsize = 0;
		return -9;
	}

	ret = ctrserver_handlecmd_common(cmdid, buf, bufsize);
	if(ret != -2)return ret;

	if(cmdid==0x90)
	{
		ptr = funcptr();
		ptr = &ptr[(0x200 + 0x500) >> 2];

		if(ptr[0]!=0x58584148)
		{
			*bufsize = 0;
		}
		else
		{
			*bufsize = ptr[2];
			memcpy(buf, ptr, *bufsize);
			memset(ptr, 0, *bufsize);
		}
	}

	if(cmdid==0x93)
	{
		if(*bufsize == 8)
		{
			if(buf[0] < (0x200>>2))
			{
				ptr = funcptr();

				ptr[buf[0]] = buf[1];

				*bufsize = 0;
			}
			else
			{
				*bufsize = 4;
				buf[0] = ~1;
			}
		}
		else
		{
			*bufsize = 4;
			buf[0] = ~0;
		}

		return 0;
	}

	if(cmdid==0x98)
	{
		if(*bufsize < 12)return 0;

		regaddr = (vu32*)buf[0];//PXI registers base addr

		tmpval = (regaddr[1] >> 10) & 1;
		regaddr[1] &= ~(1<<10);

		count = buf[1];//Total PXI command requests to send.
		count2 = buf[2];//Total PXI command responses to receive.
		pos = 3;

		for(i=0; i<count; i++)
		{
			while(regaddr[1] & 2);
			regaddr[2] = buf[pos];
			pos++;

			((vu8*)regaddr)[3] |= (0x40);

			val = buf[pos];
			pos++;

			size = ((val>>6) & 0x3f) + (val & 0x3f);

			while(regaddr[1] & 2);
			regaddr[2] = val;

			while(size)
			{
				while(regaddr[1] & 2);
				regaddr[2] = buf[pos];
				pos++;
				size--;
			}
		}

		*bufsize = 4;
		buf[0] = count2;
		pos = 1;

		for(i=0; i<count2; i++)
		{
			while(regaddr[1] & 0x100);
			buf[pos] = regaddr[3];
			pos++;
			*bufsize += 4;

			while(regaddr[1] & 0x100);
			val = regaddr[3];
			buf[pos] = val;
			pos++;
			*bufsize += 4;

			size = ((val>>6) & 0x3f) + (val & 0x3f);

			while(size)
			{
				while(regaddr[1] & 0x100);
				buf[pos] = regaddr[3];
				pos++;
				size--;
				*bufsize += 4;
			}
		}

		regaddr[1] |= tmpval<<10;

		return 0;
	}

	if(cmdid==0xa0 || cmdid==0xa1)
	{
		if(*bufsize < 4)
		{
			*bufsize = 0;
			return 0;
		}

		val = buf[0];
		count = 1;

		tmpval = 0;//read
		if(cmdid==0xa1)tmpval = 1;//write

		if(tmpval && *bufsize != (4 + ((3 + 12 + 4) * 4)))
		{
			buf[0] = ~0;
			*bufsize = 4;
			return 0;
		}

		if(tmpval==0)memset(&buf[count], 0, (3 + 12 + 4) * 4);

		pos = 0;
		if(val & (1<<pos))
		{
			if(!tmpval)buf[count] = getdebug_didr();
		}
		pos++;
		count++;

		if(val & (1<<pos))
		{
			if(!tmpval)buf[count] = getdebug_dscr();
			if(tmpval)setdebug_dscr(buf[count]);
		}
		pos++;
		count++;

		if(val & (1<<pos))
		{
			if(!tmpval)buf[count] = getdebug_vcr();
			if(tmpval)setdebug_vcr(buf[count]);
		}
		pos++;
		count++;

		for(i=0; i<6; i++)
		{
			if(val & (1<<pos))
			{
				if(!tmpval)buf[count] = getdebug_bvr(i);
				if(tmpval)setdebug_bvr(i, buf[count]);
			}
			pos++;
			count++;

			if(val & (1<<pos))
			{
				if(!tmpval)buf[count] = getdebug_bcr(i);
				if(tmpval)setdebug_bcr(i, buf[count]);
			}
			pos++;
			count++;
		}

		for(i=0; i<2; i++)
		{
			if(val & (1<<pos))
			{
				if(!tmpval)buf[count] = getdebug_wvr(i);
				if(tmpval)setdebug_wvr(i, buf[count]);
			}
			pos++;
			count++;

			if(val & (1<<pos))
			{
				if(!tmpval)buf[count] = getdebug_wcr(i);
				if(tmpval)setdebug_wcr(i, buf[count]);
			}
			pos++;
			count++;
		}

		if(!tmpval)*bufsize = count*4;
		
		if(tmpval)*bufsize = 0;

		return 0;
	}

	return ret;
}

static int ctrserver_handlecmd_installcia(u32 *buf, u32 *bufsize)
{
	Handle ciahandle=0;
	u8 mediatype;
	//u8 unku8;
	//u64 titleid;
	u32 ciasize, chunksize, maxchunksize, tmpsize;
	u32 *tmpbuf;
	u32 size=0;
	u32 pos=0;
	int ret=0, ret2=0;
	int fail=0;
	u32 ackword = 0x4b4341;//"ACK"

	if(*bufsize != 0x14)return 0;

	if(amserv_available==0)
	{
		ret = am_init();
		if(ret!=0)return -5;
	}

	mediatype = buf[0] & 0xff;
	//unku8 = (buf[0]>>8) & 0xff;
	//titleid = ((u64)buf[1]) | (((u64)buf[2])<<32);
	ciasize = buf[3];

	maxchunksize = buf[4];
	tmpsize = maxchunksize;
	if(tmpsize>0x10000)tmpsize = 0x10000;

	*bufsize = 0xc;
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0;

	tmpbuf = malloc(tmpsize);
	if(tmpbuf==NULL)
	{
		buf[1] = 0x414d454d;
		return 0;
	}

	ret = AM_StartCiaInstall(mediatype, &ciahandle);
	if(ret!=0)
	{
		buf[1] = ret;
		free(tmpbuf);
		return 0;
	}
	buf[0]++;

	pos = 0;
	while(pos < ciasize)
	{
		chunksize = ciasize - pos;
		if(chunksize > maxchunksize)chunksize = maxchunksize;

		buf[2] = pos;

		while(chunksize)
		{
			tmpsize = chunksize;
			if(tmpsize>0x10000)tmpsize = 0x10000;

			if((ret2 = ctrserver_recvdata(&net_server, (u8*)tmpbuf, tmpsize))!=tmpsize)
			{
				//((u32*)0x14000000)[0x888>>2] = ret;

				if(!fail)
				{
					AM_CancelCIAInstall(&ciahandle);

					fail = 1;
					buf[1] = 0xf0f0f0f0;
				}
				break;
			}

			ret2 = 0;
			if(!fail)ret2 = FSFILE_Write(ciahandle, &size, (u64)pos, tmpbuf, tmpsize, 0x10001);
			if(ret2!=0)
			{
				if(!fail)
				{
					AM_CancelCIAInstall(&ciahandle);

					buf[1] = ret2;
					fail = 1;
				}
			}

			pos+= tmpsize;
			chunksize-= tmpsize;
		}

		if((ret2 = ctrserver_senddata(&net_server, (u8*)&ackword, 4))!=4)
		{
			//((u32*)0x14000000)[0x888>>2] = ret;

			if(!fail)
			{
				AM_CancelCIAInstall(&ciahandle);

				buf[1] = 0xf4f4f4f4;
				fail = 1;
			}
			break;
		}
	}

	free(tmpbuf);

	if(fail)return 0;

	buf[0]++;

	ret = AM_FinishCiaInstall(mediatype, &ciahandle);
	if(ret!=0)
	{
		buf[1] = ret;
		return 0;
	}
	buf[0]++;

	return 0;
}

static int ctrserver_handlecmd(u32 cmdid, u32 *buf, u32 *bufsize)
{
	//u8 *buf8 = (u8*)buf;
	u32 size;
	int ret=0, ret2, ret3;
	u32 filehandle=0;
	Handle handle, *handleptr;
	u32 val=0;
	u32 pos;
	u32 openflags;
	u64 filesize;
	u32 handletype=0;
	u32 *ptr;
	u64 val64;
	u64 *val64ptr;
	u32 *cmdbuf = getThreadCommandBuffer();
	GSP_FramebufferInfo framebufinfo;
	FS_archive archive;
	FS_path fileLowPath;
	char namebuf[8];
	u32 dmaconfig[24>>2];

	if(cmdid & 0x400)
	{
		memset(net_kernelmode_paramblock, 0, sizeof(net_kernelmode_paramblock));
		
		net_kernelmode_paramblock[0] = cmdid;
		net_kernelmode_paramblock[1] = (u32)buf;
		net_kernelmode_paramblock[2] = (u32)bufsize;

		launchcode_kernelmode(net_kernelmode_handlecmd, 0);
		return 0;
	}
	else if((cmdid & 0x800) == 0)
	{
		return ctrserver_arm9cmd(cmdid, buf, bufsize);
	}

	ret = ctrserver_handlecmd_common(cmdid, buf, bufsize);
	if(ret != -2)return ret;

	cmdid &= 0xff;

	if(cmdid==0x31)
	{
		val64ptr = (u64*)buf;
		val64 = svcGetSystemTick();
		svcSleepThread(1000000000LL);
		*val64ptr = svcGetSystemTick() - val64;
		*bufsize = 8;
		return 0;
	}

	if(cmdid==0x40)
	{
		if(*bufsize != (12 + 24))
		{
			buf[0] = ~0;
			buf[1] = 0;
			*bufsize = 8;
			return 0;
		}

		memcpy(dmaconfig, &buf[3], 24);
		buf[0] = svcStartInterProcessDma(&buf[1], 0xffff8001, (u32*)buf[0], 0xffff8001, (u32*)buf[1], buf[2], dmaconfig);
		*bufsize = 8;

		if(buf[0]==0)
		{
			val = 0;

			while(1)
			{
				buf[0] = (u32)svcGetDmaState(&val, buf[1]);
				if(buf[0]!=0)break;
				if(val>=2)break;
			}

			svcCloseHandle(buf[1]);
		}

		return 0;
	}

	if(cmdid==0x41)
	{
		*bufsize = 4;
		buf[0] = svcGetProcessorID();
		return 0;
	}

	if(cmdid==0x4d)
	{
		if(*bufsize != 20)
		{
			*bufsize = 8;
			buf[0] = 0;
			buf[1] = ~0;
			return 0;
		}

		if(buf[0]==0)//Start the seperate thread for cmd-handling.
		{
			if(thread_initialized)
			{
				*bufsize = 8;
				buf[0] = 0;
				buf[1] = ~1;
				return 0;
			}

			for(pos=0; pos<3; pos++)
			{
				ret = 0;
				if(threadevents[pos] == 0)ret = svcCreateEvent(&threadevents[pos], 0);
				if(ret!=0)((u32*)0x86000000)[pos] = ret;
			}

			for(pos=0; pos<3; pos++)svcClearEvent(threadevents[pos]);

			memset(net_thread_paramblock, 0, sizeof(net_thread_paramblock));

			thread_threadpriority = buf[1];
			thread_processorid = buf[2];
			if(thread_threadpriority == ~0)thread_threadpriority = 0x31;

			memcpy(&thread_cmdreplyevent_timeout, &buf[3], 8);

			if(thread_stackbottom)free(thread_stackbottom);
			thread_stackbottom = memalign(0x8, 0x1000);

			if(thread_stackbottom==NULL)
			{
				*bufsize = 8;
				buf[0] = 0;
				buf[1] = ~2;
				return 0;
			}

			buf[0] = 1;
			buf[1] = svcCreateThread(&threadhandle, network_cmdhandler_thread, 0x0, &thread_stackbottom[0x1000>>2], thread_threadpriority, thread_processorid);

			if(buf[1]==0)
			{
				thread_initialized = 1;
				selected_cmdhandler_thread = 1;
			}
			else
			{
				free(thread_stackbottom);
				thread_stackbottom = NULL;
			}

			*bufsize = 8;
		}
		else if(buf[0]==1)//Terminate the seperate thread for cmd-handling.
		{
			if(!thread_initialized)
			{
				*bufsize = 8;
				buf[0] = 0;
				buf[1] = ~1;
				return 0;
			}

			svcSignalEvent(threadevents[0]);//Send the signal for thread termination, then wait for the actual thread termination.
			svcWaitSynchronization(threadhandle, U64_MAX);
			svcCloseHandle(threadhandle);

			free(thread_stackbottom);
			thread_stackbottom = NULL;

			for(pos=0; pos<3; pos++)svcClearEvent(threadevents[pos]);

			thread_initialized = 0;
			selected_cmdhandler_thread = 0;

			*bufsize = 8;
			buf[0] = 0;
			buf[1] = 0;
		}
		else if(buf[0]==2)
		{
			buf[0] = thread_initialized;
			if(!thread_initialized)
			{
				*bufsize = 4;
				return 0;
			}

			for(pos=0; pos<3; pos++)buf[1+pos] = threadevents[pos];

			buf[4] = threadhandle;

			buf[5] = (u32)thread_stackbottom;
			buf[6] = 0x1000;//stacksize
			buf[7] = thread_threadpriority;
			buf[8] = thread_processorid;
			buf[9] = selected_cmdhandler_thread;

			*bufsize = 10*4;
		}
		else if(buf[0]==3)
		{
			if(!thread_initialized)
			{
				*bufsize = 8;
				buf[0] = 0;
				buf[1] = ~1;
				return 0;
			}

			selected_cmdhandler_thread = buf[1];
			*bufsize = 0;
			return 0;
		}

		return 0;
	}

	if(cmdid==0x4e)
	{
		*bufsize = 0;
		ptr = buf;

		*bufsize+=4;
		*ptr++ = (u32)buf;

		*bufsize+=4;
		*ptr++ = net_payload_maxsize;

		*bufsize+=4;
		*ptr++ = net_server.sockfd;

		return 0;
	}

	/*if(cmdid==0x4f)
	{
		size = *bufsize;
		*bufsize = 4;

		ret = srv_initialize(1);
		if(ret!=0)
		{
			buf[0] = (u32)ret;
			return 0;
		}

		if(size==0)ret = srvpm_replace_servaccesscontrol_default();
		if(size)ret = srvpm_replace_servaccesscontrol((char*)buf, size);

		srv_shutdown();

		buf[0] = (u32)ret;

		return 0;
	}*/

	if(cmdid==0x50)
	{
		if(*bufsize <= 8)return 0;

		handletype = buf[1];

		if(handletype)
		{
			buf[0] = 0;

			if(handletype==1)
			{
				handleptr = srvGetSessionHandle();
				buf[0] = *handleptr;
			}
			else if(handletype==2)
			{
				if(amserv_available==0)am_init();
				handleptr = amGetSessionHandle();
				if(amserv_available)buf[0] = *handleptr;
			}
			else if(handletype==3)
			{
				buf[0] = IRU_GetServHandle();
			}
			else if(handletype==4 && aptLockHandle)
			{
				aptOpenSession();
				buf[0] = aptuHandle;
			}
			else if(handletype==5)
			{
				handleptr = fsGetSessionHandle();
				buf[0] = *handleptr;
			}
			else if(handletype==6)
			{
				buf[0] = gspGpuHandle;
			}
			else if(handletype==7)
			{
				buf[0] = CFGNOR_handle;
			}
			else if(handletype==8)
			{
				buf[0] = hidHandle;
			}
			else if(handletype==9)
			{
				//buf[0] = CSND_handle;
			}
			else if(handletype==10)
			{
				buf[0] = SOCU_handle;
			}
			else if(handletype==11 && arm9access_available)
			{
				buf[0] = pxidev_handle;
			}

			if(buf[0]==0)
			{
				buf[0] = ~0;
				*bufsize = 4;
				return 0;
			}
		}

		memcpy(cmdbuf, &buf[2], *bufsize - 8);

		ret = svcSendSyncRequest(buf[0]);

		if(handletype==4)aptCloseSession();

		if(ret!=0)
		{
			buf[0] = ret;
			*bufsize = 4;
			return 0;
		}

		*bufsize = (cmdbuf[0] & 0x3f) + ((cmdbuf[0] & 0xfc0) >> 6);
		*bufsize = (*bufsize + 1) * 4;

		buf[0] = 0;
		memcpy(&buf[1], cmdbuf, *bufsize);
		*bufsize += 4;

		return 0;
	}

	if(cmdid==0x51)
	{
		*bufsize = 0x20000;
		CFGNOR_DumpFlash(buf, 0x20000);

		return 0;
	}

	if(cmdid==0x52)
	{
		size = *bufsize;
		if(size>0x20000)size = 0x20000;
		*bufsize = 0x0;
		CFGNOR_WriteFlash(buf, size);

		return 0;
	}

	/*if(cmdid==0x53)
	{
		size = (*bufsize) - 4;

		ret = GSPGPU_FlushDataCache(NULL, (u8*)&buf[1], size);
		if(ret<0)return ret;

		*bufsize = 0;

		CSND_playsound(0x8, (buf[0]>>24) & 0xff, buf[0] & 0xff, 44100, &buf[1], NULL, size, (buf[0]>>8) & 0xff, (buf[0]>>16) & 0xff);//(u32*)((u32)buf+size)
		Result CSND_playsound(u32 channel, CSND_LOOPING looping, CSND_ENCODING encoding, u32 samplerate, u32 *vaddr0, u32 *vaddr1, u32 totalbytesize, u32 unk0, u32 unk1);
		Result csndPlaySound(0x8, u32 flags, 44100, float vol, float pan, void* data0, void* data1, u32 size);

		return 0;
	}*/

	/*if(cmdid==0x54)
	{
		CSND_writesharedmem_cmdtype0((u16)buf[0], (u8*)&buf[1]);
		//CSND_setchannel_playbackstate(0x8, buf[0]);
		//CSND_sharedmemtype0_cmd0(0x8, buf[0]);
		CSND_sharedmemtype0_cmdupdatestate(0);
		*bufsize = 0;

		return 0;
	}*/

	if(cmdid==0x55)
	{
		memset(&framebufinfo, 0, sizeof(GSP_FramebufferInfo));
		memcpy(&framebufinfo, &buf[1], sizeof(GSP_FramebufferInfo));

		framebufinfo.framebuf0_vaddr = &buf[(1 + sizeof(GSP_FramebufferInfo))>>2];
		framebufinfo.framebuf1_vaddr = &buf[(1 + sizeof(GSP_FramebufferInfo))>>2];

		buf[0] = GSPGPU_SetBufferSwap(NULL, buf[0], &framebufinfo);

		*bufsize = 4;

		return 0;
	}

	if(cmdid==0x56)
	{
		if(*bufsize <= 0x18)
		{
			*bufsize = 0;
			return 0;
		}

		size = *bufsize - 0x18;

		if((ret = srvGetServiceHandle(&handle, "ps:ps"))!=0)
		{
			*bufsize = 4;
			buf[0] = (u32)ret;
			return 0;
		}

		cmdbuf[0] = 0x00040204;//PS:EncryptDecryptAes
		cmdbuf[1] = size;
		cmdbuf[2] = 0;
		memcpy(&cmdbuf[3], &buf[2], 0x10);//CTR/IV
		cmdbuf[7] = buf[0];//algo-type
		cmdbuf[8] = buf[1];//key-type
		cmdbuf[9] = (size<<4) | 10;
		cmdbuf[10] = (u32)&buf[0x18>>2];
		cmdbuf[11] = (size<<4) | 12;
		cmdbuf[12] = (u32)&buf[0x18>>2];

		ret = svcSendSyncRequest(handle);

		if(ret!=0)
		{
			buf[0] = ret;
			*bufsize = 4;
			return 0;
		}

		//*bufsize = size + 0x18;
		buf[0] = cmdbuf[1];
		memcpy(&buf[1], &cmdbuf[2], 0x10);
		buf[0x14>>2] = 0;
		//memcpy(&buf[0x14>>2], &buf[(0x18 + size)>>2], size);

		svcCloseHandle(handle);

		return 0;
	}

	if(cmdid==0x57)
	{
		if(*bufsize != 8)return 0;
		memcpy(namebuf, buf, 8);
		buf[0] = svcConnectToPort(&buf[1], namebuf);
		return 0;
	}

	if(cmdid==0x58)
	{
		if(*bufsize != 0xc || gxCmdBuf==NULL)
		{
			*bufsize = 0;
			return 0;
		}
		*bufsize = 4;
		buf[0] = GX_RequestDma(gxCmdBuf, (u32*)buf[0], (u32*)buf[1], buf[2]);
		return 0;
	}

	if(cmdid==0x5c)
	{
		if(*bufsize != 0x18 || gxCmdBuf==NULL)
		{
			*bufsize = 0;
			return 0;
		}
		*bufsize = 4;
		buf[0] = GX_SetTextureCopy(gxCmdBuf, (u32*)buf[0], buf[2], (u32*)buf[1], buf[3], buf[4], buf[5]);
		return 0;
	}

	if(cmdid==0x5d)
	{
		buf[0] = csndInit();
		*bufsize = 4;
		return 0;
	}

	if(cmdid==0x5e)
	{
		if(*bufsize != 4)
		{
			*bufsize = 0;
			return 0;
		}
		
		if(buf[0] == 0x0)
		{
			buf[0] = hidInit(NULL);
		}
		else if(buf[0] == 0x1)
		{
			buf[0] = CFGNOR_Initialize(1);
		}
		else
		{
			*bufsize = 0;
		}

		return 0;
	}

	if(cmdid==0x60)
	{
		if(*bufsize != 0x18)
		{
			*bufsize = 0;
			return 0;
		}

		*bufsize = 4;
		val = 0;
		if(buf[0]==0xffff8001)
		{
			svcDuplicateHandle(&val, 0xffff8001);
			buf[0] = val;
		}

		buf[0] = svcControlProcessMemory(buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

		if(val)svcCloseHandle(val);

		return 0;
	}

	if(cmdid==0x61)
	{
		if(*bufsize != 0x14)
		{
			*bufsize = 0;
			return 0;
		}

		*bufsize = 8;
		buf[0] = svcControlMemory(&buf[1], buf[0], buf[1], buf[2], buf[3], buf[4]);
		return 0;
	}

	if(cmdid==0x62)
	{
		if(*bufsize != 0x4)
		{
			*bufsize = 0;
			return 0;
		}

		buf[0] = svcCloseHandle(buf[0]);
		return 0;
	}

	if(cmdid==0x63)
	{
		if(*bufsize != 0x10)
		{
			*bufsize = 0;
			return 0;
		}

		*bufsize = 8;
		buf[0] = svcCreateMemoryBlock(&buf[1], buf[0], buf[1], buf[2], buf[3]);
		return 0;
	}

	if(cmdid==0x64)
	{
		if(*bufsize != 0x10)
		{
			*bufsize = 0;
			return 0;
		}

		*bufsize = 4;
		buf[0] = svcMapMemoryBlock(buf[0], buf[1], buf[2], buf[3]);
		return 0;
	}

	if(cmdid==0x65)
	{
		if(*bufsize != 0x8)
		{
			*bufsize = 0;
			return 0;
		}

		*bufsize = 4;
		buf[0] = svcUnmapMemoryBlock(buf[0], buf[1]);
		return 0;
	}

	if(cmdid==0x66)
	{
		if(*bufsize != 0x8)
		{
			*bufsize = 0;
			return 0;
		}

		*bufsize = 0xc;

		buf[0] = svcGetSystemInfo((s64*)&buf[1], buf[0], buf[1]);
		return 0;
	}

	if(cmdid==0x67)
	{
		if(*bufsize != 5*4)
		{
			*bufsize = 0;
			return 0;
		}

		buf[0] = svcCreateThread(&buf[1], (ThreadFunc)buf[0], buf[1], (u32*)buf[2], buf[3], buf[4]);

		*bufsize = 8;
		return 0;
	}

	if(cmdid==0x80)
	{
		//buf[0]=archiveid, [1]=archive_lowpathtype, [2]=archive_lowpathsize, [3]=file_lowpathtype, [4]=file_lowpathsize, [5]=openflags. starting @ buf[6]: archive_lowpathdata. starting @ buf[6 + <archive_lowpathsize aligned to 4-bytes>]: file_lowpathdata. immediately after the file_lowpathdata with 4-byte alignment is the data to write, when openflags bit1 is set.

		pos = (((buf[2] + 3) & ~3)>>2);

		archive.id = buf[0];
		archive.lowPath.type = buf[1];
		archive.lowPath.size = buf[2];
		archive.lowPath.data = (u8*)&buf[6];

		fileLowPath.type = buf[3];
		fileLowPath.size = buf[4];
		fileLowPath.data = (u8*)&buf[6 + pos];

		openflags = buf[5];

		ret = FSUSER_OpenArchive(NULL, &archive);
		ret2 = ret;

		if(ret==0)ret = FSUSER_OpenFile(NULL, &filehandle, archive, fileLowPath, openflags, FS_ATTRIBUTE_NONE);

		//ret = FSUSER_OpenFileDirectly(NULL, &filehandle, archive, fileLowPath, openflags, FS_ATTRIBUTE_NONE);
		pos+= 6 + (((buf[4] + 3) & ~3)>>2);

		filesize = 0;

		if(ret==0)
		{
			ret = FSFILE_GetSize(filehandle, &filesize);
			if(ret!=0)FSFILE_Close(filehandle);

			//if((openflags & 2) && ((*bufsize)-pos*4) < filesize)filesize = (*bufsize)-pos*4;
			if(openflags & 2)filesize = (*bufsize)-pos*4;
		}

		*bufsize = 0;

		if(ret==0)
		{
			if(openflags == 1)ret = FSFILE_Read(filehandle, bufsize, 0, buf, filesize);
			if(openflags & 2)
			{
				ret = FSFILE_Write(filehandle, &buf[0], 0, &buf[pos], filesize, 0x10001);
				*bufsize = 4;
			}
			FSFILE_Close(filehandle);

			if(openflags & 2)
			{
				if(archive.id==4 || archive.id==8 || archive.id==0x1234567C || archive.id==0x567890B1 || archive.id==0x567890B2)FSUSER_ControlArchive(fsGetSessionHandle(), archive);
			}
		}

		if(ret2==0)FSUSER_CloseArchive(NULL, &archive);

		if(ret!=0)
		{
			buf[0] = (u32)ret;
			*bufsize = 4;
		}

		return 0;
	}

	if(cmdid==0x81)
	{
		pos = (((buf[2] + 3) & ~3)>>2);

		archive.id = buf[0];
		archive.lowPath.type = buf[1];
		archive.lowPath.size = buf[2];
		archive.lowPath.data = (u8*)&buf[6];

		fileLowPath.type = buf[3];
		fileLowPath.size = buf[4];
		fileLowPath.data = (u8*)&buf[6 + pos];

		ret = FSUSER_OpenArchive(NULL, &archive);
		ret2 = ret;

		if(ret==0)
		{
			ret = FSUSER_OpenDirectory(NULL, &filehandle, archive, fileLowPath);
		}

		ret3 = 1;
		if(ret==0)
		{
			ret = FSDIR_Read(filehandle, &buf[1], buf[5], (FS_dirent*)&buf[2]);
			ret3 = ret;
			FSDIR_Close(filehandle);
			svcCloseHandle(filehandle);
		}

		if(ret2==0)FSUSER_CloseArchive(NULL, &archive);

		buf[0] = (u32)ret;
		*bufsize = 4;
		if(ret3==0)
		{
			*bufsize = 8 + buf[1]*0x228;
		}

		return 0;
	}

	/*if(cmdid==0xb0)
	{
		if(*bufsize < 4)
		{
			*bufsize = 0;
			return 0;
		}

		if(buf[0]==0)
		{
			buf[0] = IRU_Initialize((u32*)0x08087000, 0x1000);
			*bufsize = 4;
			return 0;
		}
		else if(buf[0]==1)
		{
			buf[0] = IRU_Shutdown();
			*bufsize = 4;
			return 0;
		}
		else if(buf[0]==2)
		{
			buf[0] = IRU_SetBitRate(buf[1]);
			*bufsize = 4;
			return 0;
		}
		else if(buf[0]==3)
		{
			buf[1] = 0;
			buf[0] = IRU_GetBitRate((u8*)&buf[1]);
			*bufsize = 8;
			return 0;
		}
		else if(buf[0]==4)
		{
			buf[0] = IRU_SendData((u8*)&buf[1], *bufsize - 4, 1);
			*bufsize = 4;
			return 0;
		}
		else if(buf[0]==5)
		{
			*bufsize = buf[1] + 8;
			buf[0] = IRU_RecvData((u8*)&buf[2], buf[1], buf[2], &buf[1], 1);
			return 0;
		}
		else if(buf[0]==6)
		{
			buf[0] = IRU_SetIRLEDState(buf[1]);
			*bufsize = 4;
			return 0;
		}
		else if(buf[0]==7)
		{
			buf[1] = 0;
			buf[0] = IRU_GetIRLEDRecvState(&buf[1]);
			*bufsize = 8;
			return 0;
		}
		else if(buf[0]==8)
		{
			size = buf[1];
			val = buf[2];
			memset(&buf[1], 0, size);
			for(pos=0; pos<size*8; pos++)
			{
				buf[0] = IRU_GetIRLEDRecvState(&val);
				if(buf[0]!=0)break;
				buf8[4 + (pos>>3)] |= val << (pos & 7);

				//if(val==0)svcSleepThread(560);
				//if(val)svcSleepThread(1690);
				if(val)svcSleepThread(val);
			}
			*bufsize = 4 + size;
			return 0;
		}*/
		/*else if(buf[0]==9)
		{
			*bufsize = 0;
			transmit_nec_ir_command(buf[1], buf[2]);
			return 0;
		}*/
		/*
		*bufsize = 4;
		buf[0] = ~0;
		return 0;
	}*/

	if(cmdid==0xc0)
	{
		return ctrserver_handlecmd_installcia(buf, bufsize);
	}

	if(cmdid==0xc1)
	{
		if(*bufsize != 12)return 0;
		if(amserv_available==0)
		{
			if(am_init()!=0)return -5;
		}

		*bufsize = 4;

		if(((buf[0] >> 8) & 0xff) == 0)
		{
			buf[0] = AM_DeleteAppTitle(buf[0] & 0xff, ((u64)buf[1]) | (((u64)buf[2])<<32));
		}
		else
		{
			buf[0] = AM_DeleteTitle(buf[0] & 0xff, ((u64)buf[1]) | (((u64)buf[2])<<32));
		}

		return 0;
	}

	return 0;
}

void network_cmdhandler_thread(void *arg)
{
	Result ret;
	s32 syncedID;
	u32 cmdid, *buf, *bufsize;

	while(1)
	{
		ret = svcWaitSynchronizationN(&syncedID, threadevents, 2, 0, U64_MAX);
		if(ret!=0 || (syncedID<0 || syncedID>1))break;

		svcClearEvent(threadevents[syncedID]);

		if(syncedID==0)//terminate
		{
			break;
		}
		else//cmdreq
		{
			cmdid = net_thread_paramblock[0];
			buf = (u32*)net_thread_paramblock[1];
			bufsize = (u32*)net_thread_paramblock[2];

			ctrserver_handlecmd(cmdid, buf, bufsize);

			svcSignalEvent(threadevents[2]);
		}
	}

	svcExitThread();
}

int ctrserver_recvdata(ctrserver *server, u8 *buf, int size)
{
	int ret, pos=0;
	int tmpsize=size;

	while(tmpsize)
	{
		if((ret = recv(server->sockfd, &buf[pos], tmpsize, 0))<=0)
		{
			if(ret<0)ret = errno;
			if(ret == -EWOULDBLOCK)continue;
			return ret;
		}

		pos+= ret;
		tmpsize-= ret;
	}

	return size;
}

int ctrserver_senddata(ctrserver *server, u8 *buf, int size)
{
	int ret, pos=0;
	int tmpsize=size;

	while(tmpsize)
	{
		if((ret = send(server->sockfd, &buf[pos], tmpsize, 0))<0)
		{
			ret = errno;
			if(ret == -EWOULDBLOCK)continue;
			return ret;
		}

		pos+= ret;
		tmpsize-= ret;
	}

	return size;
}

int ctrserver_recvlong(ctrserver *server, u32 *data)
{
	return ctrserver_recvdata(server, (u8*)data, 4);
}

int ctrserver_sendlong(ctrserver *server, u32 data)
{
	return ctrserver_senddata(server, (u8*)&data, 4);
}

int ctrserver_checkauthclient(ctrserver *server)
{
	int ret=0;
	int pos;
	int validauth = 1;
	int authlen = auth_bin_size;
	u8 recvauth[MAX_CHALLENGESIZE];

	memset(recvauth, 0, MAX_CHALLENGESIZE);
	if(authlen>MAX_CHALLENGESIZE)authlen = MAX_CHALLENGESIZE;

	ret = ctrserver_recvdata(server, recvauth, authlen);
	if(ret<=0)return ret;

	for(pos=0; pos<authlen; pos++)
	{
		if(recvauth[pos] != auth_bin[pos])validauth = 0;
	}

	return validauth;
}

int ctrserver_transfermessage(ctrserver *server, int dir, u32 *cmdid, u32 *payloadsize, u32 *payload)
{
	int ret=0;

	if(dir==0)ret = ctrserver_recvlong(server, cmdid);
	if(dir==1)ret = ctrserver_sendlong(server, *cmdid);
	if(ret<=0)return ret;

	if(dir==0)ret = ctrserver_recvlong(server, payloadsize);
	if(dir==1)ret = ctrserver_sendlong(server, *payloadsize);
	if(ret<=0)return ret;

	if(*payloadsize)
	{
		if(net_payload_maxsize < *payloadsize)return -9;

		if(dir==0)ret = ctrserver_recvdata(server, (u8*)payload, (int)*payloadsize);
		if(dir==1)ret = ctrserver_senddata(server, (u8*)payload, (int)*payloadsize);
		if(ret<=0)return ret;
	}

	return 1;
}

int ctrserver_processcmd(ctrserver *server, u32 *payloadptr)
{
	int ret=0;
	u32 cmdid=0, payloadsize=0;

	memset(payloadptr, 0, 0x100);

	ret = ctrserver_transfermessage(server, 0, &cmdid, &payloadsize, payloadptr);
	if(ret<=0)return ret;

	if(selected_cmdhandler_thread==0 || ((cmdid & 0xcff) == 0x84d))//The thread-management command for the cmd-handler thread must be run under the current thread. Other commands will be sent to the cmd-handler thread if available, otherwise those are processed by the current thread.
	{
		ret = ctrserver_handlecmd(cmdid, payloadptr, &payloadsize);
	}
	else
	{
		ret = 1;

		memset(net_thread_paramblock, 0, sizeof(net_thread_paramblock));
		
		net_thread_paramblock[0] = cmdid;
		net_thread_paramblock[1] = (u32)payloadptr;
		net_thread_paramblock[2] = (u32)&payloadsize;

		ret = svcSignalEvent(threadevents[1]);//Signal the cmdreq event.
		if(ret<0)ret = -7;
		ret = svcWaitSynchronization(threadevents[2], thread_cmdreplyevent_timeout);//Wait for cmdreply.
		if(ret<0 || ret==0x09401bfe)ret = -8;
		if(ret==0)svcClearEvent(threadevents[2]);

		if(ret<0)selected_cmdhandler_thread = 0;

		if(ret>=0)ret = 1;
	}

	if(ret<0)return ret;

	ret = ctrserver_transfermessage(server, 1, &cmdid, &payloadsize, payloadptr);
	if(ret<=0)return ret;

	return 1;
}

int ctrserver_processclient_connection(ctrserver *server, u32 *payloadptr)
{
	int ret=0;

	if((ret = ctrserver_checkauthclient(server))!=1)
	{
		((u32*)0x14000000)[0x204>>2] = ret;
		return ret;
	}

	while(1)
	{
		ret = ctrserver_processcmd(server, payloadptr);
		if(ret<=0)break;
	}

	//if(ret!=0)((u32*)0x14000000)[0x200>>2] = ret;

	return ret;
}

/*int process_clientconnection_test(int commsock)
{
	int ret;
	char strbuf[256];

	memset(strbuf, 0, 256);
	strncpy(strbuf, "Hello World from 3DS via sockets!\n", 255);

	while(1)
	{
		if(send(commsock, strbuf, strlen(strbuf), 0)<0)
		{
			((u32*)0x14000000)[6] = (u32)SOC_GetErrno();
			break;
		}

		memset(strbuf, 0, 256);
		strncpy(strbuf, "Echo from 3DS: ", 255);

		if((ret = recv(commsock, &strbuf[0xf], 0xff-0xf, 0))<=0)
		{
			if(ret<0)((u32*)0x14000000)[7] = (u32)SOC_GetErrno();
			break;
		}

		if(strncmp(&strbuf[0xf], "quit", 4)==0)
		{
			break;
		}
	}

	return 0;
}*/

/*int process_clientconnection_test_udp(int commsock)
{
	int ret;
	struct sockaddr addr;
	int addrlen, tmpaddrlen;
	char strbuf[256];

	memset(strbuf, 0, 256);
	
	while(1)
	{
		memset(strbuf, 0, 256);
		strncpy(strbuf, "Echo from 3DS: ", 255);

		addrlen = sizeof(struct sockaddr);
		if((ret = recvfrom(commsock, &strbuf[0xf], 0xff-0xf, 0, &addr, &addrlen))<=0)
		{
			if(ret<0)((u32*)0x14000000)[7] = (u32)SOC_GetErrno();
			break;
		}

		tmpaddrlen = addrlen;

		if(strncmp(&strbuf[0xf], "quit", 4)==0)
		{
			break;
		}

		addrlen = sizeof(struct sockaddr);
		if(sendto(commsock, strbuf, strlen(strbuf), 0, &addr, addrlen)<0)
		{
			((u32*)0x14000000)[6] = (u32)SOC_GetErrno();
			break;
		}

		if(sendto(commsock, &addr, tmpaddrlen, 0, &addr, addrlen)<0)
		{
			((u32*)0x14000000)[6] = (u32)SOC_GetErrno();
			break;
		}
	}

	return 0;
}*/

void network_initialize()
{
	int ret=0;
	struct sockaddr_in addr;

	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	//listen_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(listen_sock<0)
	{
		((u32*)0x84000000)[3] = errno;
		SOC_Shutdown();
		return;
	}

	/**((u32*)0x08050090) = fcntl(listen_sock, F_GETFL);
	*((u32*)0x08050094) = fcntl(listen_sock, F_SETFL, *((u32*)0x08050090) | O_NONBLOCK);
	*((u32*)0x08050098) = fcntl(listen_sock, F_GETFL);*/

	addr.sin_family = AF_INET;
	addr.sin_port = 0x8d20;// 0x3905 = big-endian 1337. 0x8d20 = big-endian 8333.
	addr.sin_addr.s_addr = INADDR_ANY;
	//addr.sin_addr.s_addr= 0x2401a8c0;//0x2301a8c0 = 192.168.1.35. 0x0102a8c0 = 192.168.2.1
	//ret = connect(listen_sock, (struct sockaddr *)&addr, sizeof(addr));
	ret = bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr));
	if(ret<0)
	{
		((u32*)0x84000000)[5] = (u32)errno;
		SOC_Shutdown();
		return;
	}

	ret = listen(listen_sock, 1);
	if(ret==-1)ret = errno;
	if(ret<0)
	{
		((u32*)0x84000000)[8] = (u32)ret;
		SOC_Shutdown();
		return;
	}

	init_arm9access();
}

void network_stuff(u32 *payloadptr, u32 payload_maxsize)
{
	Result ret;
	struct sockaddr addr;
	socklen_t addrlen;

	net_payloadptr = payloadptr;
	net_payload_maxsize = payload_maxsize;

	net_server.sockfd = 0;

	network_initialize();

	/*process_clientconnection_test_udp(listen_sock);
	while(1);*/

	while(1)
	{
		memset(&addr, 0, sizeof(struct sockaddr));
		addrlen = sizeof(struct sockaddr);

		net_server.sockfd = accept(listen_sock, &addr, &addrlen);
		if(net_server.sockfd==-1)net_server.sockfd = errno;
		if(net_server.sockfd<0)
		{
			if(net_server.sockfd == -EWOULDBLOCK)continue;
			//((u32*)0x84000000)[9] = (u32)net_server.sockfd;
			//break;
			continue;
		}

		ctrserver_processclient_connection(&net_server, net_payloadptr);
		
		//process_clientconnection_test(net_server.sockfd);

		closesocket(net_server.sockfd);
		net_server.sockfd = 0;
	}

	while(1);
}

