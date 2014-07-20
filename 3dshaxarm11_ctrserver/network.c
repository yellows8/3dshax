#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <ctr/types.h>
#include <ctr/svc.h>
#include <ctr/srv.h>
#include <ctr/GSP.h>
#include <ctr/GX.h>
#include <ctr/SOC.h>
#include <ctr/APT.h>
#include <ctr/CSND.h>
#include <ctr/FS.h>
#include <ctr/IR.h>
#include <ctr/CFGNOR.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#include "srv.h"
#include "am.h"

#include "ctrclient.h"

#include "necir.h"

static u8 authdata[0x100] = {
  0x6f, 0xa7, 0xbd, 0xb2, 0xeb, 0xcb, 0x1a, 0x1d, 0xf3, 0x53, 0xd5, 0x75,
  0x92, 0x6e, 0x8b, 0x80, 0xd0, 0xb6, 0x30, 0xb8, 0xe0, 0x3b, 0x0c, 0xfc,
  0x27, 0xa1, 0xe9, 0xc4, 0x7b, 0x26, 0x26, 0xdd, 0xc9, 0x3b, 0x75, 0x5e,
  0x4d, 0x1b, 0xa4, 0x21, 0xcb, 0xe1, 0x25, 0x71, 0xa1, 0x0e, 0x65, 0x6b,
  0x28, 0x20, 0xab, 0x48, 0x87, 0xbe, 0x55, 0xcf, 0x4a, 0xd0, 0xe5, 0x7c,
  0xa0, 0xf4, 0xc9, 0xa4, 0x40, 0x50, 0xf3, 0xd9, 0x52, 0xb9, 0x2a, 0xec,
  0xaf, 0xf8, 0x03, 0x14, 0xf8, 0xd0, 0xc8, 0x4a, 0xa6, 0xd8, 0xa1, 0x06,
  0xb1, 0xa1, 0x7f, 0xfe, 0x35, 0xb6, 0xec, 0x94, 0x6d, 0x33, 0xf4, 0xa3,
  0xa7, 0x9b, 0x2c, 0x33, 0x05, 0xa7, 0x77, 0x9d, 0xaf, 0xac, 0x61, 0x7a,
  0xbe, 0xc8, 0x32, 0x81, 0x71, 0x01, 0x07, 0x36, 0x23, 0x84, 0xb9, 0x41,
  0x08, 0xf5, 0x6d, 0x5b, 0x6f, 0x71, 0xb3, 0x26, 0x42, 0xfe, 0xc6, 0x61,
  0x7c, 0xe0, 0x17, 0xe0, 0x1e, 0x96, 0x85, 0x7d, 0x22, 0xca, 0xcf, 0x73,
  0x39, 0x64, 0x46, 0x6f, 0x09, 0x64, 0x3e, 0x67, 0x13, 0xe0, 0x30, 0x16,
  0xbd, 0xff, 0x2f, 0x68, 0x35, 0xe6, 0xf8, 0x93, 0xf4, 0x77, 0xe1, 0x46,
  0xe7, 0xe0, 0x33, 0xfa, 0xc7, 0x65, 0xc1, 0x0b, 0xf3, 0x80, 0x09, 0x6d,
  0xc7, 0x5f, 0xea, 0xfd, 0x15, 0x73, 0x04, 0x60, 0xc5, 0x85, 0xf2, 0x32,
  0x93, 0xb7, 0xba, 0xf1, 0x21, 0xe0, 0xb9, 0xa0, 0xd3, 0x2d, 0x60, 0x50,
  0x2f, 0xc2, 0x73, 0x68, 0x26, 0x1a, 0x3b, 0x87, 0x8d, 0x6b, 0xc3, 0x6f,
  0xb0, 0x33, 0xb1, 0x41, 0x84, 0xe8, 0xd0, 0xeb, 0xe0, 0xec, 0x8c, 0x37,
  0xf2, 0xd3, 0xd2, 0xc4, 0x41, 0x43, 0xea, 0x39, 0x05, 0xd9, 0x25, 0x44,
  0xce, 0x7c, 0x8f, 0x31, 0x7d, 0x53, 0x2f, 0xc7, 0x95, 0x41, 0x5d, 0x15,
  0x7e, 0x57, 0xb7, 0xb1
};

typedef struct
{
	volatile int sockfd;
} ctrserver;

static Handle pxidev_handle=0;
static Handle fsuser_servhandle = 0;

static int listen_sock;

static u32 *net_payloadptr, net_payload_maxsize;
static ctrserver net_server;
static u32 net_kernelmode_paramblock[4];

static u32 arm9access_available = 0;

static Handle am_handle=0;
static u32 amserv_available = 0;
static Handle lockhandle;

extern Handle srvHandle;
extern Handle aptLockHandle;
extern Handle aptuHandle;
extern Handle gspGpuHandle;
extern Handle CFGNOR_handle;
extern Handle hidHandle;
extern Handle CSND_handle;
extern Handle SOCU_handle;

extern u32* gxCmdBuf;

int ctrserver_recvdata(ctrserver *server, u8 *buf, int size);
int ctrserver_senddata(ctrserver *server, u8 *buf, int size);

u32 launchcode_kernelmode(void*, u32 param);
void call_arbitaryfuncptr(void* funcptr, u32 *regdata);

Result am_init()
{
	Result ret;

	if(am_handle!=0)return 0;

	if((ret = srv_getServiceHandle(NULL, &am_handle, "am:net"))!=0)return ret;

	ret = AM_Initialize(am_handle, &lockhandle);
	if(ret!=0)return ret;

	ret = svc_waitSynchronization1(lockhandle, 0xffffffffffffffff);
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
		if(bufaddr < 0x1c000000)
		{
			bufaddr+= 0x0c000000;
		}
		else
		{
			bufaddr -= 0x10000000;
		}

		cmdbuf[0] = 0x00000100;
		cmdbuf[1] = 0x43565253;//"SRVC"
		cmdbuf[2] = cmdid;
		cmdbuf[3] = *bufsize;//(bufsize<<8) | 4;
		cmdbuf[4] = bufaddr;
	}

	if((ret = svc_sendSyncRequest(pxidev_handle))!=0)return ret;

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

	if((ret = srv_getServiceHandle(NULL, &pxidev_handle, "pxi:dev"))!=0)return ret;

	if((ret = pxidev_cmd0(0, NULL, 0)) != 0)return ret;

	arm9access_available = 1;

	return 0;
}

int ctrserver_handlecmd_common(u32 cmdid, u32 *buf, u32 *bufsize)
{
	u32 *addr;
	u32 size;
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

			if((cmdid & 0xff) == 0x01 || (cmdid & 0xff) == 0x05)size = 1;
			if((cmdid & 0xff) == 0x02 || (cmdid & 0xff) == 0x06)size = 2;
			if((cmdid & 0xff) == 0x03 || (cmdid & 0xff) == 0x07)size = 4;

			buf[0] = 0;
			if(rw==0)memcpy(buf, addr, size);
			if(rw==1)memcpy(addr, &buf[1], size);

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

	return -2;
}

int net_kernelmode_handlecmd(u32 param)
{
	u32 cmdid;
	u32 *buf;
	u32 *bufsize;
	int ret=0;
	u32 *ptr;
	u32* (*funcptr)() = (void*)0xfff51808;

	if(*((u8*)0x1FF80002) < 35)funcptr = (void*)0xfff61808;
	if(*((u8*)0x1FF80002) >= 44)funcptr = (void*)0xfff41808;

	cmdid = net_kernelmode_paramblock[0];
	buf = (u32*)net_kernelmode_paramblock[1];
	bufsize = (u32*)net_kernelmode_paramblock[2];

	cmdid &= 0xff;

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

	if(cmdid==0x91)
	{
		if(*bufsize == 4)
		{
			ptr = funcptr();

			if(buf[0]==0)*ptr = 1;
			if(buf[0])*ptr = 0x4d524554;//"TERM"

			*bufsize = 0;
		}
	}

	return ret;
}

static int ctrserver_handlecmd_installcia(u32 *buf, u32 *bufsize)
{
	Handle ciahandle=0;
	u8 mediatype, unku8;
	u64 titleid;
	u32 ciasize, chunksize, maxchunksize;
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
	unku8 = (buf[0]>>8) & 0xff;
	titleid = ((u64)buf[1]) | (((u64)buf[2])<<32);
	ciasize = buf[3];
	tmpbuf = (u32*)0x08048000;

	maxchunksize = buf[4];

	*bufsize = 0xc;
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0;

	ret = AM_StartInstallCIADB0(am_handle, &ciahandle, mediatype);
	if(ret!=0)
	{
		buf[1] = ret;
		return 0;
	}
	buf[0]++;

	pos = 0;
	while(pos < ciasize)
	{
		chunksize = ciasize - pos;
		if(chunksize > maxchunksize)chunksize = maxchunksize;

		buf[2] = pos;

		if((ret2 = ctrserver_recvdata(&net_server, (u8*)tmpbuf, chunksize))!=chunksize)
		{
			//((u32*)0x14000000)[0x888>>2] = ret;

			if(!fail)
			{
				AM_AbortCIAInstall(am_handle, ciahandle);

				fail = 1;
				buf[1] = 0xf0f0f0f0;
			}
			break;
		}

		ret2 = 0;
		if(!fail)ret2 = FSFILE_Write(ciahandle, &size, (u64)pos, tmpbuf, chunksize, 0x10001);
		if(ret2!=0)
		{
			if(!fail)
			{
				AM_AbortCIAInstall(am_handle, ciahandle);

				buf[1] = ret2;
				fail = 1;
			}
		}

		if((ret2 = ctrserver_senddata(&net_server, (u8*)&ackword, 4))!=4)
		{
			//((u32*)0x14000000)[0x888>>2] = ret;

			if(!fail)
			{
				AM_AbortCIAInstall(am_handle, ciahandle);

				buf[1] = 0xf4f4f4f4;
				fail = 1;
			}
			break;
		}

		pos+= chunksize;
	}

	if(fail)return 0;

	buf[0]++;

	ret = AM_CloseFileCIA(am_handle, ciahandle);
	if(ret!=0)
	{
		buf[1] = ret;
		return 0;
	}
	buf[0]++;

	ret = AM_FinalizeTitlesInstall(am_handle, mediatype, 1, &titleid, unku8);
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
	u8 *buf8 = (u8*)buf;
	u32 size;
	int ret=0;
	u32 filehandle=0;
	Handle handle;
	u32 val=0;
	u32 pos;
	u64 filesize;
	u32 handletype=0;
	u32 *ptr;
	u32 *cmdbuf = getThreadCommandBuffer();
	GSP_FramebufferInfo framebufinfo;
	FS_archive archive;
	FS_path fileLowPath;
	char namebuf[8];

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

	if(cmdid==0x4f)
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
	}

	if(cmdid==0x50)
	{
		if(*bufsize <= 8)return 0;

		handletype = buf[1];

		if(buf[1])
		{
			buf[0] = 0;

			if(buf[1]==1)
			{
				buf[0] = srvHandle;
			}
			else if(buf[1]==2)
			{
				if(amserv_available==0)am_init();
				if(amserv_available)buf[0] = am_handle;
			}
			else if(buf[1]==3)
			{
				buf[0] = IRU_GetServHandle();
			}
			else if(buf[1]==4 && aptLockHandle)
			{
				aptOpenSession();
				buf[0] = aptuHandle;
			}
			else if(buf[1]==5)
			{
				buf[0] = fsuser_servhandle;
			}
			else if(buf[1]==6)
			{
				buf[0] = gspGpuHandle;
			}
			else if(buf[1]==7)
			{
				buf[0] = CFGNOR_handle;
			}
			else if(buf[1]==8)
			{
				buf[0] = hidHandle;
			}
			else if(buf[1]==9)
			{
				buf[0] = CSND_handle;
			}
			else if(buf[1]==10)
			{
				buf[0] = SOCU_handle;
			}

			if(buf[0]==0)
			{
				buf[0] = ~0;
				*bufsize = 4;
				return 0;
			}
		}

		memcpy(cmdbuf, &buf[2], *bufsize - 8);

		ret = svc_sendSyncRequest(buf[0]);

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

	if(cmdid==0x53)
	{
		size = *bufsize - 4;

		ret = GSPGPU_FlushDataCache(NULL, (u8*)&buf[1], *bufsize);
		if(ret<0)return ret;

		CSND_playsound(0x8, (buf[0]>>24) & 0xff, buf[0] & 0xff, 44100, &buf[1], NULL, size, (buf[0]>>8) & 0xff, (buf[0]>>16) & 0xff);//(u32*)((u32)buf+size)
		*bufsize = 0;

		return 0;
	}

	if(cmdid==0x54)
	{
		CSND_writesharedmem_cmdtype0((u16)buf[0], (u8*)&buf[1]);
		//CSND_setchannel_playbackstate(0x8, buf[0]);
		//CSND_sharedmemtype0_cmd0(0x8, buf[0]);
		CSND_sharedmemtype0_cmdupdatestate(0);
		*bufsize = 0;

		return 0;
	}

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

		if((ret = srv_getServiceHandle(NULL, &handle, "ps:ps"))!=0)
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

		ret = svc_sendSyncRequest(handle);

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

		svc_closeHandle(handle);

		return 0;
	}

	if(cmdid==0x57)
	{
		if(*bufsize != 8)return 0;
		memcpy(namebuf, buf, 8);
		buf[0] = svc_connectToPort(&buf[1], namebuf);
		return 0;
	}

	if(cmdid==0x58)
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

		ret = FSUSER_OpenFileDirectly(fsuser_servhandle, &filehandle, archive, fileLowPath, buf[5], FS_ATTRIBUTE_NONE);
		pos+= 6 + (((buf[4] + 3) & ~3)>>2);

		filesize = 0;

		if(ret==0)
		{
			ret = FSFILE_GetSize(filehandle, &filesize);
			if(ret!=0)FSFILE_Close(filehandle);
			//filesize = 0x36c0;
			//filesize = 0x80000;

			if((buf[5] & 2) && ((*bufsize)-pos*4) < filesize)filesize = (*bufsize)-pos*4;
		}

		*bufsize = 0;

		if(ret==0)
		{
			if(buf[5] == 1)ret = FSFILE_Read(filehandle, bufsize, 0, buf, filesize);
			if(buf[5] & 2)
			{
				ret = FSFILE_Write(filehandle, &buf[0], 0, &buf[pos], filesize, 0x10001);
				*bufsize = 4;
			}
			FSFILE_Close(filehandle);
		}

		if(ret!=0)
		{
			buf[0] = (u32)ret;
			*bufsize = 4;
		}

		return 0;
	}

	if(cmdid==0xb0)
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

				//if(val==0)svc_sleepThread(560);
				//if(val)svc_sleepThread(1690);
				if(val)svc_sleepThread(val);
			}
			*bufsize = 4 + size;
			return 0;
		}
		else if(buf[0]==9)
		{
			*bufsize = 0;
			transmit_nec_ir_command(buf[1], buf[2]);
			return 0;
		}

		*bufsize = 4;
		buf[0] = ~0;
		return 0;
	}

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
			buf[0] = AM_DeleteApplicationTitle(am_handle, buf[0] & 0xff, ((u64)buf[1]) | (((u64)buf[2])<<32));
		}
		else
		{
			buf[0] = AM_DeleteTitle(am_handle, buf[0] & 0xff, ((u64)buf[1]) | (((u64)buf[2])<<32));
		}

		return 0;
	}

	return 0;
}

int ctrserver_recvdata(ctrserver *server, u8 *buf, int size)
{
	int ret, pos=0;
	int tmpsize=size;

	while(tmpsize)
	{
		if((ret = recv(server->sockfd, &buf[pos], tmpsize, 0))<=0)
		{
			if(ret<0)ret = SOC_GetErrno();
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
			ret = SOC_GetErrno();
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
	int authlen = sizeof(authdata);
	u8 recvauth[MAX_CHALLENGESIZE];

	memset(recvauth, 0, MAX_CHALLENGESIZE);
	if(authlen>MAX_CHALLENGESIZE)authlen = MAX_CHALLENGESIZE;

	ret = ctrserver_recvdata(server, recvauth, authlen);
	if(ret<=0)return ret;

	for(pos=0; pos<authlen; pos++)
	{
		if(recvauth[pos] != authdata[pos])validauth = 0;
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

	ret = ctrserver_handlecmd(cmdid, payloadptr, &payloadsize);
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
		((u32*)0x84000000)[3] = (u32)listen_sock;
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
		((u32*)0x84000000)[5] = (u32)SOC_GetErrno();
		SOC_Shutdown();
		return;
	}

	ret = listen(listen_sock, 1);
	if(ret==-1)ret = SOC_GetErrno();
	if(ret<0)
	{
		((u32*)0x84000000)[8] = (u32)ret;
		SOC_Shutdown();
		return;
	}

	init_arm9access();
}

Result fs_init()
{
	Result ret=0;

	if((ret = srv_getServiceHandle(NULL, &fsuser_servhandle, "fs:USER"))!=0)return ret;

	return FSUSER_Initialize(fsuser_servhandle);
}

void network_stuff(u32 *payloadptr, u32 payload_maxsize)
{
	Result ret=0;
	struct sockaddr addr;
	int addrlen;

	net_payloadptr = payloadptr;
	net_payload_maxsize = payload_maxsize;

	net_server.sockfd = 0;

	ret = fs_init();
	if(ret!=0)
	{
		((u32*)0x84000000)[0x700] = ret;
	}

	network_initialize();

	/*process_clientconnection_test_udp(listen_sock);
	while(1);*/

	while(1)
	{
		memset(&addr, 0, sizeof(struct sockaddr));
		addrlen = sizeof(struct sockaddr);

		net_server.sockfd = accept(listen_sock, &addr, &addrlen);
		if(net_server.sockfd==-1)net_server.sockfd = SOC_GetErrno();
		if(net_server.sockfd<0)
		{
			if(net_server.sockfd == -EWOULDBLOCK)continue;
			((u32*)0x84000000)[9] = (u32)net_server.sockfd;
			break;
		}

		*((u32*)0x08050000) = addrlen;
		memcpy((u32*)0x08050004, &addr, addrlen);

		ctrserver_processclient_connection(&net_server, net_payloadptr);
		
		//process_clientconnection_test(net_server.sockfd);

		closesocket(net_server.sockfd);
		net_server.sockfd = 0;
	}

	while(1);
}

