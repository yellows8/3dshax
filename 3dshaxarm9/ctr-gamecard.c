#ifdef ENABLE_GAMECARD

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <nds/ndstypes.h>

#include "arm9fs.h"

extern u32 proc9_textstartaddr;

u32 *gamecard_archiveobj = NULL;
u32 (*funcptr_ctrcard_cmdc6)(u32*, u32*) = NULL;
u32 *gamecard_stateptr = NULL;

u32 *locate_cmdhandler_code(u32 *ptr, u32 size, u32 *pool_cmpdata, u32 pool_cmpdata_wordcount, u32 locate_ldrcc);
u32 parse_branch(u32 branchaddr, u32 branchval);

/*u32 gamecardslot_getpowerstate()
{
	u32 (*funcptr)() = (void*)0x806238b;
	return funcptr();
}

u32 gamecardslot_poweron()
{
	u32 (*funcptr)() = (void*)0x8062f79;
	return funcptr();
}*/

/*void patch_mountcontent()
{
	u32 (*funcptr)(u32*, u64*, u8, u32, u64, u32) = (void*)0x8061d11; //archive_mountcontent
	u64 archivehandle=0;
	u64 programID = 0x0000000000000000LL;//0x00040000000b6e00LL;
	u8 mediatype = 0;//1;
	u32 tmpdata[3];

	while(*((vu16*)0x10146000) & 2);//Wait for button B to be pressed.

	*((u16*)0x805b6b4) = 0x2000;

	//tmpdata[0] = gamecardslot_getpowerstate();
	//tmpdata[1] = gamecardslot_poweron();

	tmpdata[2] = funcptr(&pxifs_state[0x2f1c>>2], &archivehandle, mediatype, 0, programID, 0);
	dumpmem(tmpdata, 12);
}*/

/*void wait_systicks(u32 ticks)
{
	u64 start, cur;

	start = svcGetSystemTick();

	while(1)
	{
		cur = svcGetSystemTick();
		if((u32)(cur - start) >= ticks)break;
	}
}

u32 ctrcard_cmdbf_begindataread(u64 mediaoffset, u16 total_datablocks)
{
	u32 (*funcptr)(u32*, u32, u64, u16) = (void*)0x807428d;
	u32 ctx[5];
	ctx[0] = 0;
	ctx[1] = 0;
	ctx[2] = 0;
	ctx[3] = 0;
	ctx[4] = 0;

	return funcptr(ctx, 0, mediaoffset, total_datablocks);
}*/

void locatefunc_ctrcard_cmdc6()
{
	u32 *ptr;
	u32 pos;
	u32 pool_cmpdata[6] = {0xd9001830, 0x000100c6, 0x00010041, 0x00020284, 0x00020041, 0x00030284};

	ptr = locate_cmdhandler_code((u32*)proc9_textstartaddr, 0x080ff000-proc9_textstartaddr, pool_cmpdata, 6, 1);
	if(ptr==NULL)return;

	ptr = (u32*)ptr[0x6];//ptr = addr of code in pxips9 cmdhandler func for GetRomId.
	pos = 0;

	while(1)
	{
		if((ptr[pos] >> 24) == 0xeb)break;
		pos++;
	}

	ptr = (u32*)parse_branch((u32)&ptr[pos], 0);//ptr = address of the function handling this command.
	pos = 0;

	while(1)
	{
		if((ptr[pos] >> 24) == 0xfa)break;
		pos++;
	}

	funcptr_ctrcard_cmdc6 = (void*)(parse_branch((u32)&ptr[pos], 0) | 1);

	while(1)//Locate the first word of the .pool.
	{
		if(ptr[pos] == 0xe8bd8010)break;
		pos++;
	}
	pos++;

	gamecard_stateptr = (u32*)ptr[pos];
}

u32 ctrcard_cmdc6(u32 *outbuf)
{
	if(funcptr_ctrcard_cmdc6==NULL || gamecard_stateptr==NULL)
	{
		locatefunc_ctrcard_cmdc6();
		if(funcptr_ctrcard_cmdc6==NULL || gamecard_stateptr==NULL)return ~0;
	}

	return funcptr_ctrcard_cmdc6(gamecard_stateptr, outbuf);
}

u32 gamecard_initarchiveobj()
{
	u32 lowpath[3];
	u32 lowpathdata[4];

	lowpath[0] = 2;//type
	lowpath[1] = (u32)lowpathdata;//dataptr
	lowpath[2] = 0x10;//size

	lowpathdata[0] = 0x0;//programID-low
	lowpathdata[1] = 0x0;//programID-high
	lowpathdata[2] = 0x2;//mediatype
	lowpathdata[3] = 0x0;//reserved

	return pxifs_openarchive(&gamecard_archiveobj, 0x2345678A, lowpath);
}

u32 gamecard_readsectors(u32 *outbuf, u32 sector, u32 sectorcount)
{
	u32 ret=0;
	u32 *ptr;

	if(gamecard_archiveobj==NULL)
	{
		ret = gamecard_initarchiveobj();
		if(ret!=0)return ret;
		if(gamecard_archiveobj==NULL)return ~0;
	}

	ptr = (u32*)(gamecard_archiveobj[0x24>>2]+8);
	ptr = (u32*)*ptr;

	return archive_readsectors(ptr, outbuf, sectorcount, sector);
}

/*u32 read_gamecardsector(u64 mediaoffset, u32 *outbuf, u32 readsize, u16 total_datablocks)
{
	u32 ret, pos, val;

	ret = ctrcard_cmdbf_begindataread(mediaoffset, total_datablocks);
	if(ret!=1)return ret;

	pos = 0;
	while(1)
	{
		if(*((vu32*)0x10004000) & (1<<27))
		{
			val = *((vu32*)0x10004030);
			outbuf[pos] = val;
			pos++;

			if(pos >= (readsize>>2))break;
		}

		if(*((vu32*)0x10004000) & 0x10)return 0;
	}

	//while(*((vu32*)0x10004000) & (1<<31))val = *((vu32*)0x10004030);

	wait_systicks(4000);

	return 1;
}*/

/*void read_gamecard()
{
	u32 *fileobj = NULL;
	u64 mediaoffset = 0x000000 >> 9;
	u32 ret;
	u32 pos=0;
	u32 val = 0;
	u32 *sectorbuf = (u32*)0x20000000;
	u32 dumpsize = 0x100000;

	if(openfile(sdarchive_obj, 4, dump_filepath, 0x22, 7, &fileobj)!=0)return;

	*((u16*)0x807431c) = 0x2302;
	svcFlushProcessDataCache((u32*)0x807431c, 4);//patch the arm9fw code for ctrcard_cmdbf_begindataread() so that it doesn't set the REG_CTRCARDCNT IRQ enable bit.

	//sectorbuf[0] = ctrcard_cmdc6(&sectorbuf[1]);
	//if(filewrite(fileobj, sectorbuf, 0x44, 0)!=0)return;

	//memset(framebuf_addr, 0xffffffff, 0x46500);
	//memset(&framebuf_addr[(0x46500+0x10)>>2], 0xffffffff, 0x46500);

	memset(sectorbuf, 0x0, dumpsize);
	ret = read_gamecardsector(mediaoffset, sectorbuf, dumpsize, dumpsize>>9);
	if(ret!=1)return;
	if(filewrite(fileobj, sectorbuf, dumpsize, (u32)(mediaoffset<<9))!=0)return;

	//ret = read_gamecardsector(mediaoffset + (dumpsize>>9), sectorbuf, dumpsize, dumpsize>>9);
	//if(filewrite(fileobj, sectorbuf, dumpsize, dumpsize)!=0)return;

	//mediaoffset = sectorbuf[0x120>>2];
	*/
	/*mediaoffset = 0x27D84800>>9;//0x20;
	for(pos=0; pos<((0x28E9FA00 - 0x27D84800)>>9); pos+=(dumpsize>>9))
	{
		memset(sectorbuf, 0, dumpsize);
		ret = read_gamecardsector(mediaoffset, sectorbuf, dumpsize, dumpsize>>9);
		if(ret!=1)return;
		if(filewrite(fileobj, sectorbuf, dumpsize, pos << 9)!=0)return;

		svcSleepThread(1000000000LL);

		memset(framebuf_addr, val, 0x46500);
		memset(&framebuf_addr[(0x46500+0x10)>>2], val, 0x46500);

		val +=0x456789ab;

		mediaoffset+= dumpsize>>9;
	}*/

	//memset(framebuf_addr, 0x20202020, 0x46500);
	//memset(&framebuf_addr[(0x46500+0x10)>>2], 0x20202020, 0x46500);

	//*((u16*)0x807431c) = 0x2303;
	//svcFlushProcessDataCache((u32*)0x807431c, 4);
//}

#endif

