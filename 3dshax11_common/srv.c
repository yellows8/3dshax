#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/svc.h>
#include <ctr/srv.h>

#include "srv.h"

static Handle srv_handle;
static u32 srv_type = 0;

static char default_new_servaccesscontrol[][8] = {
"APT:U",
"y2r:u",
"gsp::Gpu",
"ndm:u",
"fs:USER",
"hid:USER",
"dsp::DSP",
"cfg:u",
"fs:REG",
"ps:ps",
"ir:u",
"ns:s",
"nwm::UDS",
"nim:s",
"ac:u",
"am:net",
"cfg:nor",
"soc:U",
"pxi:dev",
"ptm:sysm",
"csnd:SND",
"pm:app"
};

/*Result srv_RegisterClient()
{
	Result ret=0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x00010002;
	cmdbuf[1] = 32;

	if((ret = svc_sendSyncRequest(srv_handle))!=0)return ret;

	ret = (Result)cmdbuf[1];
	return ret;
}*/

/*Result srv_RegisterService(u32 *handle, char *service, u32 servicelen, u32 unk)
{
	Result ret=0;
	u32 *cmdbuf = getThreadCommandBuffer();

	if(strlen(service)>8)return -1;

	cmdbuf[0] = 0x00030100;
	
	cmdbuf[1] = 0;
	cmdbuf[2] = 0;
	strncpy((char*)&cmdbuf[1], service, 8);
	cmdbuf[3] = strlen(service);
	cmdbuf[4] = unk;

	if((ret = svc_sendSyncRequest(srv_handle))!=0)return ret;

	ret = (Result)cmdbuf[1];

	if(ret==0)*handle = cmdbuf[3];

	return ret;
}

Result srv_UnregisterService(u32 *handle, char *service, u32 servicelen, u32 unk)
{
	Result ret=0;
	u32 *cmdbuf = getThreadCommandBuffer();

	if(strlen(service)>8)return -1;

	cmdbuf[0] = 0x000400c0;
	
	cmdbuf[1] = 0;
	cmdbuf[2] = 0;
	strncpy((char*)&cmdbuf[1], service, 8);
	cmdbuf[3] = strlen(service);

	if((ret = svc_sendSyncRequest(srv_handle))!=0)return ret;

	ret = (Result)cmdbuf[1];
	return ret;
}

Result srv_GetServiceHandle(u32 *handle, char *service, u32 servicelen, u32 unk)
{
	Result ret=0;
	u32 *cmdbuf = getThreadCommandBuffer();

	if(strlen(service)>8)return -1;

	cmdbuf[0] = 0x00050100;
	
	cmdbuf[1] = 0;
	cmdbuf[2] = 0;
	strncpy((char*)&cmdbuf[1], service, 8);
	cmdbuf[3] = strlen(service);
	cmdbuf[4] = unk;

	if((ret = svc_sendSyncRequest(srv_handle))!=0)return ret;

	ret = (Result)cmdbuf[1];

	if(ret==0)*handle = cmdbuf[3];

	return ret;
}*/

Result srvpm_registerprocess(u32 procid, char *servicelist, u32 servicelist_size)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	u32 ret=0;

	cmdbuf[0] = 0x04030082;
	cmdbuf[1] = procid;
	cmdbuf[2] = servicelist_size>>2;
	cmdbuf[3] = (servicelist_size<<14) | 2;
	cmdbuf[4] = (u32)servicelist;

	if((ret = svc_sendSyncRequest(srv_handle)) != 0)return ret;
	
	ret = (Result)cmdbuf[1];
	return ret;
}

Result srvpm_unregisterprocess(u32 procid)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	Result ret=0;

	cmdbuf[0] = 0x04040040;
	cmdbuf[1] = procid;

	if((ret = svc_sendSyncRequest(srv_handle)) != 0)return ret;
	
	ret = (Result)cmdbuf[1];
	return ret;
}

Result srvpm_replace_servaccesscontrol(char *servicelist, u32 servicelist_size)
{
	u32 procid = 0;
	Result ret=0;

	svc_getProcessId(&procid, 0xffff8001);

	if((ret = srvpm_unregisterprocess(procid)) != 0)return ret;

	ret = srvpm_registerprocess(procid, servicelist, servicelist_size);

	return ret;
}

Result srvpm_replace_servaccesscontrol_default()
{
	return srvpm_replace_servaccesscontrol((char*)default_new_servaccesscontrol, sizeof(default_new_servaccesscontrol));
}

Result srv_initialize(u32 use_srvpm)
{
	int ret=0;
	char *portname = "srv:";

	if(use_srvpm)portname = "srv:pm";
	srv_type = use_srvpm;

	ret = svc_connectToPort(&srv_handle, portname);
	if(ret!=0)return ret;

	ret = srv_RegisterClient(&srv_handle);

	return ret;
}

void srv_shutdown()
{
	svc_closeHandle(srv_handle);
}

