//Most of this code is by smea.

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <ctr/types.h>
#include <ctr/svc.h>

#include "am.h"

Result AM_Initialize(Handle handle, Handle* out) //more like getLockHandle ?
{
        u32* cmdbuf=getThreadCommandBuffer();
        cmdbuf[0]=0x4120000; //request header code
       
        Result ret=0;
        if((ret=svc_sendSyncRequest(handle)))return ret;
       
        if(out)*out=cmdbuf[3]; //used for a waitsync1...
       
        return cmdbuf[1];
}

Result AM_StartInstallCIADB0(Handle handle, Handle* out, u8 mediatype)
{
        u32* cmdbuf=getThreadCommandBuffer();
        cmdbuf[0]=0x04020040; //request header code

	cmdbuf[1] = (u32)mediatype;
 
        Result ret=0;
        if((ret=svc_sendSyncRequest(handle)))return ret;
       
        if(out)*out=cmdbuf[3]; //CIA "file" handle
 
        return cmdbuf[1];
}

Result AM_StartInstallCIADB1(Handle handle, Handle* out)
{
        u32* cmdbuf=getThreadCommandBuffer();
        cmdbuf[0]=0x4030000; //request header code
 
        Result ret=0;
        if((ret=svc_sendSyncRequest(handle)))return ret;
       
        if(out)*out=cmdbuf[3]; //CIA "file" handle
 
        return cmdbuf[1];
}

Result AM_AbortCIAInstall(Handle handle, Handle ciaFileHandle)
{
        u32* cmdbuf=getThreadCommandBuffer();
        cmdbuf[0]=0x4040002; //request header code
 
        cmdbuf[1]=0x10;
        cmdbuf[2]=ciaFileHandle;
       
        Result ret=0;
        if((ret=svc_sendSyncRequest(handle)))return ret;
 
        return cmdbuf[1];
}
 
Result AM_CloseFileCIA(Handle handle, Handle ciaFileHandle)
{
        u32* cmdbuf=getThreadCommandBuffer();
        cmdbuf[0]=0x4060002; //request header code
 
        cmdbuf[1]=0x10;
        cmdbuf[2]=ciaFileHandle;
       
        Result ret=0;
        if((ret=svc_sendSyncRequest(handle)))return ret;
 
        return cmdbuf[1];
}
 
Result AM_FinalizeTitlesInstall(Handle handle, u8 mediatype, u32 total_titles, u64 *titleidlist, u8 unku8)
{
        u32* cmdbuf=getThreadCommandBuffer();
        cmdbuf[0]=0x40700c2; //request header code
 
        cmdbuf[1]=(u32)mediatype;
        cmdbuf[2]=total_titles;
        cmdbuf[3]=(u32)unku8;
        cmdbuf[4]=((total_titles*8) << 4) | 10;
        cmdbuf[5]=(u32)titleidlist;
       
        Result ret=0;
        if((ret=svc_sendSyncRequest(handle)))return ret;
 
        return cmdbuf[1];
}

//End of code by smea.

Result AM_DeleteApplicationTitle(Handle handle, u8 mediatype, u64 titleid)
{
        u32* cmdbuf=getThreadCommandBuffer();
        cmdbuf[0]=0x000400c0; //request header code

	cmdbuf[1] = (u32)mediatype;
	cmdbuf[2] = (u32)(titleid);
	cmdbuf[3] = (u32)(titleid>>32);

        Result ret=0;
        if((ret=svc_sendSyncRequest(handle)))return ret;
       
        return cmdbuf[1];
}

Result AM_DeleteTitle(Handle handle, u8 mediatype, u64 titleid)
{
        u32* cmdbuf=getThreadCommandBuffer();
        cmdbuf[0]=0x041000c0; //request header code

	cmdbuf[1] = (u32)mediatype;
	cmdbuf[2] = (u32)(titleid);
	cmdbuf[3] = (u32)(titleid>>32);

        Result ret=0;
        if((ret=svc_sendSyncRequest(handle)))return ret;
       
        return cmdbuf[1];
}

