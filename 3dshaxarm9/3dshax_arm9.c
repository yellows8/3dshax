#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <3ds.h>

#include "arm9_svc.h"
#include "arm9fs.h"
#include "arm9_FS.h"
#include "a9_memfs.h"
#include "arm9_nand.h"
#include "ctr-gamecard.h"
#include "arm9_a11kernel.h"
#include "yls8_aes.h"

#include "ctrclient.h"

void changempu_memregions();
u32 init_arm9patchcode3(u32 *ptr0, u32 *ptr1, u32 *ptr2);
void arm9_launchfirm();
u32 getsp();
u32 getcpsr();

u32 generate_branch(u32 branchaddr, u32 targetaddr, u32 flag);//branchaddr = addr of branch instruction, targetaddr = addr to branch to, flag = 0 for regular branch, non-zero for bl. (ARM-mode)
u32 parse_branch(u32 branchaddr, u32 branchval);
u32 parse_armblx(u32 branchaddr, u32 branchval);
u32 parse_branch_thumb(u32 branchaddr, u32 branchval);

void call_arbitaryfuncptr(void* funcptr, u32 *regdata);

u32 *get_arm11debuginfo_physaddr();
void write_arm11debug_patch();
void writepatch_arm11kernel_svcaccess();

void arm9_pxipmcmd1_getexhdr_writepatch(u32 addr);
void arm9general_debughook_writepatch(u32 addr);

void writearm11_firmlaunch_usrpatch();

u32 *locate_cmdhandler_code(u32 *ptr, u32 size, u32 *pool_cmpdata, u32 pool_cmpdata_wordcount, u32 locate_ldrcc);

extern u32 arm9_stub[];
extern u32 arm9_stub2[];
extern u32 arm9dbs_stub[];
extern u32 arm11_stub[];
extern u8 rsamodulo_slot0[];
extern u32 FIRMLAUNCH_RUNNINGTYPE;
extern u32 RUNNINGFWVER;
extern u32 FIRMLAUNCH_CLEARPARAMS;
extern u32 arm9_rsaengine_txtwrite_hooksz;

extern u32 proc9_textstartaddr;

void firmlaunch_swprintfhook(u16*);
extern u32 proc9_swprintf_addr;

void pxidev_cmdhandler_cmd0();
void mountcontent_nandsd_writehookstub();

u32 input_filepos;
u32 input_filesize = 0;

u32 *framebuf_addr = NULL;

u32 *fileobj_debuginfo = NULL;
u32 debuginfo_pos = 0;

u16 arm11code_filepath[] = {0x2F, 0x33, 0x64, 0x73, 0x68, 0x61, 0x78, 0x5F, 0x61, 0x72, 0x6D, 0x31, 0x31, 0x2E, 0x62, 0x69, 0x6E, 0x00};//UTF-16 "/3dshax_arm11.bin"

#define THREAD_STACKSIZE 0xa00
#ifndef LOADA9_NEW3DSMEM
u64 *thread_stack = (u64*)(0x08000c00/*0x02000000-THREAD_STACKSIZE*/);//Normally only random data is located around here(0x08000c00 size 0x1100).
#else
u64 thread_stack[THREAD_STACKSIZE>>3];
#endif

u64 ARM11CODELOAD_PROCNAME = 0x706c64;//0x706c64 = "dlp", 0x726564697073LL = "spider"

extern u32 *arm11kernel_textvaddr;

#ifdef ENABLE_ARM11CODELOAD_SERVACCESSCONTROL_OVERWRITE
static char arm11codeload_servaccesscontrol[][8] = { //Service-access-control copied into the exheader for the pxipm get-exheader hook.
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
"pm:app",
"frd:u",
"mic:u",
"qtm:u"
};
#endif

#define ARM11PROCOVERRIDELIST_TOTALENTRIES 0x10

typedef struct {
	u64 procname;
	char codebin_path[0x20];
	char exheader_path[0x20];
} s_arm11_processoverride_entry;

typedef struct {
	s_arm11_processoverride_entry processes[ARM11PROCOVERRIDELIST_TOTALENTRIES];
} s_arm11_processoverride_list;

s_arm11_processoverride_list *arm11_processoverride_list = (s_arm11_processoverride_list*)0x08001600;

void load_arm11code(u32 *loadptr, u32 maxloadsize, u64 procname, u16 *filepath)
{
	//int i;
	u32 *fileobj = NULL;
	u32 *app_physaddr = NULL;
	u32 *ptr = NULL;
	u32 pos;

	if(filepath==NULL)filepath = arm11code_filepath;

	//if(loadptr==NULL)app_physaddr = patch_mmutables(procname, 1, 0);

	pos = 0;
	while(filepath[pos]!=0)pos++;
	pos++;

	if(openfile(sdarchive_obj, 4, filepath, pos*2, 1, &fileobj)!=0)return;
	input_filesize = getfilesize(fileobj);
	ptr = loadptr;
	if(ptr==NULL)
	{
		ptr = app_physaddr;
	}
	if((maxloadsize>0 && maxloadsize<input_filesize) || ptr==NULL)return;
	fileread(fileobj, ptr, input_filesize, 0);

	if(loadptr)
	{
		for(pos=1; pos<0x40; pos++)
		{
			if(loadptr[pos+0]==0x58584148)
			{
				if(loadptr[pos+1]==0x434f5250 && loadptr[pos+2]==0x454d414e)//Set the process-name when these fields are set correctly.
				{
					loadptr[pos+1] = (u32)(procname);
					loadptr[pos+2] = (u32)(procname>>32);
					if(loadptr[pos+3] == 0x5458544b)loadptr[pos+3] = (u32)arm11kernel_textvaddr;
					break;
				}
			}
		}
	}

	if(loadptr==NULL)
	{
		app_physaddr[(0x20da98>>2) + 0] = arm11_stub[0];//0xffffffff This stub is executed after the APT thread handles the signal triggered by NS, in APTU_shutdownhandle where it returns.
		app_physaddr[(0x20da98>>2) + 1] = arm11_stub[1];

		app_physaddr[0x37db08>>2] = 0;//Patch the last branch in the function for the app thread which runs on both arm11 cores, so that it terminates.

		ptr = (u32*)(((u32)app_physaddr) - 0x200000);

		for(pos=0; pos<(0x200000>>2); pos++)
		{
			if(ptr[pos] == 0x58584148)
			{
				ptr[pos] = 0x00100000;
			}
		}
	}
}

/*void rsaengine_setpubk(u32 keyslot, u32 bitsize, u8* modulo, u32 pubexponent)
{
	void (*funcptr)(u32, u32, u8*, u32) = (void*)0x802d439;//FW1F

	if(RUNNINGFWVER>=0x2E)funcptr = (void*)0x0802d6c5;

	funcptr(keyslot, bitsize, modulo, pubexponent);
}*/

u32 *proc9_locate_main_endaddr()//Returns a ptr to the last instruction in Process9 main().
{
	u32 *ptr;
	u32 pos;

	ptr = (u32*)parse_branch(proc9_textstartaddr + 0x14, 0);//ptr = address of Process9 main()

	pos = 0;
	while(1)
	{
		if((ptr[pos] & ~0xfff) == 0xe8bd4000)break;//"pop {... lr}"
		pos++;
	}
	pos++;

	return &ptr[pos];
}

void patch_proc9_launchfirm()
{
	u32 *ptr;
	u32 *arm9_patchaddr;
	u32 pos, pos2;
	u32 val0, val1;

	ptr = (u32*)parse_branch((u32)proc9_locate_main_endaddr(), 0);//ptr = address of launch_firm function called @ the end of main().

	pos = 0;

	#ifdef ENABLE_FIRMLAUNCH_LOADNAND//This is executed when loading FIRM from NAND is enabled and when doing the second firmlaunch, with nand-redir disabled.
	#ifndef ENABLENANDREDIR
	if(FIRMLAUNCH_CLEARPARAMS == 1)
	{
		pos2 = 0;
		while(1)
		{
			if((ptr[pos] >> 24) == 0x1a)//"bne"
			{
				pos2++;
				if(pos2==2)break;
			}
			pos++;

			if(((u32)&ptr[pos]) >= 0x080ff000)while(1);
		}

		pos+=3;
		ptr[pos] = 0xe3a00002;//Patch the proc9 code so that it uses the nativefirm tidlow instead of safemodefirm.*/
		/*pos++;//hook the swprintf func.

		while(1)
		{
			if((ptr[pos] >> 25) == 0x7d)break;//blx immediate
			pos++;

			if(((u32)&ptr[pos]) >= 0x080ff000)while(1);
		}

		proc9_swprintf_addr = parse_armblx((u32)&ptr[pos], 0);
		ptr[pos] = generate_branch(&ptr[pos], proc9_textstartaddr, 1);

		((u32*)proc9_textstartaddr)[0] = 0xe51ff004;//"ldr pc, [pc, #-4]"
		((u32*)proc9_textstartaddr)[1] = firmlaunch_swprintfhook;*/
	}
	#endif
	#endif

	while(1)
	{
		if(ptr[pos]==0xe12fff32)break;//"blx r2"
		pos++;

		if(((u32)&ptr[pos]) >= 0x080ff000)while(1);
	}
	arm9_patchaddr = (u32*)&ptr[pos];

	arm9_patchaddr[0] = arm9_stub[0];
	arm9_patchaddr[1] = arm9_stub[1];
	arm9_patchaddr[2] = arm9_stub[2];

	while(1)
	{
		if(ptr[pos]==0xe12fff3c)break;//"blx ip"
		pos++;

		if(((u32)&ptr[pos]) >= 0x080ff000)while(1);
	}

	ptr[pos] = 0xe3a00000;//patch the fileread blx for the 0x100-byte FIRM header to "mov r0, #0".
	ptr[pos + (0x14 >> 2)] = 0xe3a00000;//patch the bne branch after the fileread call.
	ptr[pos + (0x30 >> 2)] = 0xe3a00000;//patch the FIRM signature read.
	ptr[pos + (0x3C >> 2)] = 0xe3a00c01;//"mov r0, #0x100".

	while(1)
	{
		if(ptr[pos]==0xe3a03b02)break;//"mov r3, #0x800"
		pos++;

		if(((u32)&ptr[pos]) >= 0x080ff000)while(1);
	}

	while(1)
	{
		if(ptr[pos]==0xe1a04000)break;//"mov r4, r0"
		pos++;

		if(((u32)&ptr[pos]) >= 0x080ff000)while(1);
	}

	pos--;
	ptr[pos] = 0xe3a00000;//patch the RSA signature verification func call.

	while(1)
	{
		if((ptr[pos] & ~0xFFFFF) == 0xe2700000)break;//"rsbs"
		pos++;

		if(((u32)&ptr[pos]) >= 0x080ff000)while(1);
	}

	while(1)
	{
		if(ptr[pos] == 0xe2100102)break;//"ands r0, r0, #0x80000000"
		pos++;

		if(((u32)&ptr[pos]) >= 0x080ff000)while(1);
	}

	pos--;

	ptr[pos] = 0xe3a00000;//patch the func-call which reads the encrypted ncch firm data.

	while(1)
	{
		if((ptr[pos] & 0xffc00000) == 0xfac00000)break;//"blx"
		pos++;

		if(((u32)&ptr[pos]) >= 0x080ff000)while(1);
	}

	pos++;
	ptr[pos] = 0xe1a00000;//patch out the func-call which is immediately after the fs_closefile call. (FS shutdown stuff)

	while(1)//This will not work with pre-FW0B FIRM(<2.1.0), since those FIRM do not have PXI-word 0xaee97647 in the .pool.
	{
		if(ptr[pos]==0xaee97647)break;
		pos++;

		if(((u32)&ptr[pos]) >= 0x080ff000)while(1);
	}
	pos--;

	val1 = ptr[pos];

	pos--;
	while(ptr[pos]==0)pos--;

	val0 = ptr[pos];

	pos2 = pos;
	while(1)
	{
		if(ptr[pos2]==0x00044837)break;
		pos2--;

		if(pos2==0)while(1);
	}
	pos2++;

	svcFlushProcessDataCache(ptr, 0x630);

	init_arm9patchcode3((u32*)val0, (u32*)val1, (u32*)ptr[pos2]);
}

/*void get_kernelmode_sp()
{
	*((u32*)(0x08028000+0)) = getsp();
	*((u32*)(0x08028000+4)) = getcpsr();
}*/

u32 *get_framebuffers_addr()
{
	u32 *ptr = (u32*)0x20000000;
	u32 pos = 0;

	while(pos < (0x1000000>>2))
	{
		if(ptr[pos+0]==0x5544 && ptr[pos+1]==0x46500)
		{
			return &ptr[pos+4];
		}

		pos++;
	}

	return NULL;
}

void handle_debuginfo_ld11(vu32 *debuginfo_ptr)
{
	u64 procname;
	u32 *codebin_physaddr, codebin_vaddr, total_codebin_size;
	u32 *ptr, *ptr2;
	u32 *mmutable;
	u32 pos, pos2;

	#ifdef ENABLE_ARM11PROCLIST_OVERRIDE
	s_arm11_processoverride_entry *ent = NULL;
	u16 tmp_path[0x20];
	#endif

	#ifdef ENABLE_REGIONFREE
	u32 cmpblock[4] = {0xe3140001, 0x13844040, 0xe0100004, 0x13a00001};
	#endif

	#ifdef DISABLE_GAMECARDUPDATE
	u32 cmpblock_nss[7] = {0xd9001830, 0x00010040, 0x00020080, 0xb2d05e00, 0xc8a05801, 0xc8a12402, 0x00030040};
	#endif

	procname = ((u64)debuginfo_ptr[5]) | (((u64)debuginfo_ptr[6])<<32);

	codebin_vaddr = debuginfo_ptr[4];
	total_codebin_size = debuginfo_ptr[3]<<12;

	mmutable = (u32*)get_kprocessptr(0x726564616f6cLL, 0, 1);//"loader"
	if(mmutable==NULL)return;

	codebin_physaddr =  (u32*)mmutable_convert_vaddr2physaddr(mmutable, codebin_vaddr);
	if(codebin_physaddr==NULL)return;

	if(procname==ARM11CODELOAD_PROCNAME)
	{
		load_arm11code(codebin_physaddr, total_codebin_size, procname, arm11code_filepath);
		return;
	}

	#ifdef ENABLE_ARM11PROCLIST_OVERRIDE
	for(pos=0; pos<ARM11PROCOVERRIDELIST_TOTALENTRIES; pos++)
	{
		ent = &arm11_processoverride_list->processes[pos];
		if(ent->procname == 0)continue;
		if(ent->procname == procname)
		{
			if(ent->codebin_path[0x1f]!=0 || ent->codebin_path[0]==0)return;

			memset(tmp_path, 0, 0x20*2);

			for(pos2=0; pos2<0x1f; pos2++)tmp_path[pos2] = (u16)ent->codebin_path[pos2];

			load_arm11code(codebin_physaddr, total_codebin_size, procname, tmp_path);
			return;
		}
	}
	#endif

	#if ENABLE_THEMECACHENAME || ENABLE_REGIONFREE
	if(procname==0x756e656d)//"menu", Home Menu.
	{
		bool done=false;
		for(pos=0; pos<total_codebin_size; pos+=0x1000)
		{
			ptr = (u32*)mmutable_convert_vaddr2physaddr(mmutable, 0x10000000 + pos);
			if(ptr==NULL)continue;
			u8* ptr8 = (u8*)ptr;

			#ifdef ENABLE_THEMECACHENAME
			//search for theme:/ and append a '_' if found
			//this way 3dshax home menu will use its own set of theme cache extdata files and we avoid version mismatch issues or infinite hax loops !
			char str[] = "theme:/";
			int cur=0;
			for(pos2=0; pos2<0x1000; pos2+=2)
			{
				if(cur==sizeof(str)-1){ptr8[pos2]='_'; cur=0;}
				else if(ptr8[pos2] == str[cur])cur++;
				else cur=0;
			}
			#else
			if(done)return;
			#endif

			#ifdef ENABLE_REGIONFREE//SMDH icon region check patch. This does not affect the region-lock via gamecard sysupdates.
			if(done)continue;
			for(pos2=0; pos2<(0x1000-0x10); pos2+=4)
			{
				if(memcmp(&ptr[pos2>>2], cmpblock, 0x10)!=0)continue;

				pos2+= 0xc;
				ptr[pos2>>2] = 0xe3a00001;

				done=true;
			}
			#endif
		}

		return;
	}
	#endif

	#ifdef DISABLE_GAMECARDUPDATE//Patch the ns:s cmd7 code so that result-code 0xc821180b is always returned, indicating that no gamecard sysupdate installation is needed. This is required for launching gamecards from other regions.
	if(procname==0x736e)//"ns"
	{
		ptr = (u32*)mmutable_convert_vaddr2physaddr(mmutable, 0x10000000);
		if(ptr==NULL)return;

		ptr2 = locate_cmdhandler_code(ptr, 0x1e000, cmpblock_nss, 7, 1);
		if(ptr2==NULL)return;

		ptr = (u32*)mmutable_convert_vaddr2physaddr(mmutable, 0x10000000 + (ptr2[0x7] - 0x00100000));//ptr = code for handling ns:s cmd7
		if(ptr==NULL)return;

		ptr2 = (u32*)parse_branch(ptr2[0x7] + 0x28, ptr[0x28>>2]);
		ptr = (u32*)mmutable_convert_vaddr2physaddr(mmutable, 0x10000000 + (((u32)ptr2) - 0x00100000));//ptr = function for handling ns:s cmd7
		if(ptr==NULL)return;

		pos2 = 0;
		for(pos=0; pos<0x40; pos++)//Locate the "bne" instruction.
		{
			if((ptr[pos] & ~0xff) == 0x1a000000)
			{
				pos2 = pos;
				break;
			}
		}

		if(pos2)
		{
			pos2++;
			ptr2 = (u32*)parse_branch(((u32)ptr2) + (pos2<<2), ptr[pos2]);
			ptr = (u32*)mmutable_convert_vaddr2physaddr(mmutable, 0x10000000 + (((u32)ptr2) - 0x00100000));//ptr = physaddr of the function code called immediately after the above bne instruction.
			if(ptr==NULL)return;

			ptr[0] = 0xe59f0000;//ldr r0, [pc, #0]
			ptr[1] = 0xe12fff1e;//"bx lr"
			ptr[2] = 0xc821180b;
		}

		return;
	}
	#endif

	if(procname==0x45454154)//"TAEE", NES VC for TLoZ.
	{
		ptr = (u32*)mmutable_convert_vaddr2physaddr(mmutable, 0x1000e1bc);

		*ptr = (*ptr & ~0xff) | 0x09;//Change the archiveid used for the savedata archive from the savedata archiveid, to sdmc.

		ptr = (u32*)mmutable_convert_vaddr2physaddr(mmutable, 0x1000a920);//Patch the romfs mount code so that it opens the sdmc archive instead of using the actual romfs.
		ptr[0] = 0xe3a01009;//"mov r1, #9"
		ptr[3] = generate_branch(0x10a92c, 0x115a88, 1);

		/*ptr = (u32*)mmutable_convert_vaddr2physaddr(mmutable, 0x10103ff6);
		ptr[0] = 0x61746164;//Change the archive mount-point string used by the code which generates the config.ini and .patch paths, from "rom:" to "data:".
		ptr[1] = 0x3a;

		ptr = (u32*)mmutable_convert_vaddr2physaddr(mmutable, 0x10104028);
		ptr[0] = 0x61746164;//Change the archive mount-point string used by the functions which generates "rom:/rom/" paths, from "rom:" to "data:".
		ptr[1] = 0x3a;*/
		return;
	}

	#ifdef ENABLE_NIMURLS_PATCHES
	if(procname==0x6D696E)// "nim"
	{
		char *ptr;
		char *targeturl0 = "https://nus.";
		char *targeturl1 = "https://ecs.";
		char *replaceurl0 = NIMPATCHURL_UPDATE;
		char *replaceurl1 = NIMPATCHURL_ECOMMERCE;
		u32 pos, pos2;

		for(pos=0; pos<total_codebin_size; pos+=0x1000)
		{
			ptr = (char*)mmutable_convert_vaddr2physaddr(mmutable, 0x10000000 + pos);
			if(ptr==NULL)continue;

			for(pos2=0; pos2<0x1000; pos2++)
			{
				if(!memcmp(&ptr[pos2], targeturl0, 11))strncpy(&ptr[pos2], replaceurl0, 62);

				if(!memcmp(&ptr[pos2], targeturl1, 11))strncpy(&ptr[pos2], replaceurl1, 62);
			}
		}

		//physaddr[0x7d00>>2] = 0xe3a00000;//"mov r0, #0" Patch the NIM command code which returns whether a sysupdate is available.
	}
	#endif
}

#ifdef ENABLE_ARM11KERNEL_DEBUG
#ifdef ARM11KERNEL_ENABLECMDLOG
void arm11debuginfo_convertcmd_vaddr2phys(u64 procname, u32 *cmdbuf, u32 *outdata)
{
	u32 wordindex_translateparams = 0;
	u32 total_cmdreq_words = 0;
	u32 pos, type, shiftval, skipwords;
	u32 addr, physaddr, size;
	u32 *mmutable;

	mmutable = (u32*)get_kprocessptr(procname, 0, 1);
	if(mmutable==NULL)return;

	wordindex_translateparams = ((cmdbuf[0] >> 6) & 0x3f) + 1;
	total_cmdreq_words = cmdbuf[0] & 0x3f;
	if(total_cmdreq_words == 0)return;
	total_cmdreq_words+= wordindex_translateparams;

	skipwords = 0;

	for(pos = wordindex_translateparams; pos<total_cmdreq_words; pos += 2+skipwords)
	{
		type = cmdbuf[pos] & 0xe;
		skipwords = cmdbuf[pos] >> 26;
		if(type==0)continue;//This value is for the handle/processid header, so don't process those here.
		skipwords = 0;
		
		shiftval = 4;
		if(type==2)shiftval = 14;
		if(type==4 || type==6)shiftval = 8;

		size = cmdbuf[pos] >> shiftval;
		addr = cmdbuf[pos+1];
		physaddr = addr;

		if(addr>>28 != 0x2)//Don't convert the addr to physaddr when it's already a physaddr.
		{
			physaddr = (u32)mmutable_convert_vaddr2physaddr(mmutable, addr);
		}

		outdata[(pos - wordindex_translateparams)] = size;
		outdata[(pos - wordindex_translateparams) + 1] = physaddr;
	}
}
#endif

void dump_arm11debuginfo()
{
	u32 pos=0;
	vu32 *debuginfo_ptr = NULL;//0x21b00000;//0x18300000
	vu32 *debuginfo_arm9flag = NULL;
	//u32 i;
	u16 filepath[64];
	char *path = (char*)"/3dshax_debug.bin";
	//u32 *ptr;
	u32 val=0;
	vu32 *ptrpxi = (vu32*)0x10008000;
	u64 procname=0;

	debuginfo_ptr = (vu32*)((u32)get_arm11debuginfo_physaddr() + 0x200);
	debuginfo_arm9flag = (vu32*)((u32)get_arm11debuginfo_physaddr() + 0x20);

	debuginfo_arm9flag[4]++;

	if(fileobj_debuginfo==NULL)
	{
		memset(filepath, 0, 64*2);
		for(pos=0; pos<strlen(path); pos++)filepath[pos] = (u16)path[pos];

		if(openfile(sdarchive_obj, 4, filepath, (strlen(path)+1)*2, 6, &fileobj_debuginfo)!=0)return;
	}

	if(*debuginfo_arm9flag != 0x394d5241)
	{
		val = *ptrpxi;
		if((val & 0xf) == 0xf)
		{
			val &= ~0xf00;
			val |= 0xf << 8;
			*ptrpxi = val;
		}
	}

	if(*debuginfo_ptr != 0x58584148)return;

	if(*debuginfo_arm9flag != 0x394d5241)
	{
		while(1)
		{
			val = *ptrpxi;
			if(((val & 0xf) == 0xe) || *debuginfo_arm9flag == 0x394d5241)break;
		}
	}

	if(debuginfo_ptr[1]==0x3131444c)//"LD11"
	{
		handle_debuginfo_ld11(debuginfo_ptr);
	}
	else if(debuginfo_ptr[1]==0x35375653)//"SV75"
	{
		//procname = ((u64)debuginfo_ptr[3]) | (((u64)debuginfo_ptr[4])<<32);
		//if(procname==ARM11CODELOAD_PROCNAME)patch_mmutables(procname, 1, 1);//Only enable this when loading arm11-code which doesn't change mem-permissions itself, etc.
	}
	else if(debuginfo_ptr[1]!=0x444d4344)
	{
		//setfilesize(fileobj_debuginfo, debuginfo_pos + debuginfo_ptr[2]);
		filewrite(fileobj_debuginfo, (u32*)debuginfo_ptr, debuginfo_ptr[2], debuginfo_pos);
		debuginfo_pos+= debuginfo_ptr[2];
	}

	#ifdef ARM11KERNEL_ENABLECMDLOG
	if(debuginfo_ptr[1]==0x444d4344)//"DCMD"
	{
		procname = *((u64*)&debuginfo_ptr[0x10>>2]);
		arm11debuginfo_convertcmd_vaddr2phys(procname, (u32*)&debuginfo_ptr[0x20>>2], (u32*)&debuginfo_ptr[0x220>>2]);

		procname = *((u64*)&debuginfo_ptr[0x18>>2]);
		arm11debuginfo_convertcmd_vaddr2phys(procname, (u32*)&debuginfo_ptr[0x120>>2], (u32*)&debuginfo_ptr[0x320>>2]);

		//setfilesize(fileobj_debuginfo, debuginfo_pos + debuginfo_ptr[2]);
		filewrite(fileobj_debuginfo, (u32*)debuginfo_ptr, debuginfo_ptr[2], debuginfo_pos);
		debuginfo_pos+= debuginfo_ptr[2];

		for(pos=(0x220>>2); pos<(0x420>>2); pos+=2)
		{
			if(debuginfo_ptr[pos]!=0 && debuginfo_ptr[pos+1]!=0)
			{
				//setfilesize(fileobj_debuginfo, debuginfo_ptr[pos] + debuginfo_pos);
				filewrite(fileobj_debuginfo, (u32*)debuginfo_ptr[pos+1], debuginfo_ptr[pos], debuginfo_pos);
				debuginfo_pos+= debuginfo_ptr[pos];
			}
		}
	}
	#endif

	/*if(debuginfo_ptr[1]==0x33435847)//"GXC3"
	{
		ptr = (u32*)debuginfo_ptr[3+1];
		if((u32)ptr>=0x14000000 && (u32)ptr<0x1c000000)
		{
			ptr = (u32*)(((u32)ptr) + 0x0c000000);
		}
		else if((u32)ptr>=0x1f000000 && (u32)ptr<0x1f600000)
		{
			ptr = (u32*)(((u32)ptr) - 0x07000000);
		}
		else
		{
			ptr = NULL;
		}

		if(ptr)
		{
			//setfilesize(fileobj_debuginfo, debuginfo_ptr[3+5] + debuginfo_pos);
			filewrite(fileobj_debuginfo, ptr, debuginfo_ptr[3+5], debuginfo_pos);
		}
		debuginfo_pos+= debuginfo_ptr[3+5];
	}*/

	memset((u32*)debuginfo_ptr, 0, debuginfo_ptr[2]);

	if(*debuginfo_arm9flag != 0x394d5241)
	{
		val = *ptrpxi;
		val &= ~0xf00;
		val |= 0xe << 8;
		*ptrpxi = val;
	}
	else
	{
		*debuginfo_arm9flag = 0;
	}
}
#endif

#ifdef ENABLEAES
int ctrserver_process_aescontrol(aescontrol *control)
{
	u32 keyslot = (u32)control->keyslot[0];

	aes_mutexenter();

	if(control->flags[0] & AES_FLAGS_SET_IV)aes_set_iv((u32*)control->iv);
	if(control->flags[0] & AES_FLAGS_SET_KEY)
	{
		aes_set_key(keyslot, (u32*)control->key);
	}
	else if(control->flags[0] & AES_FLAGS_SET_YKEY)
	{
		aes_set_ykey(keyslot, (u32*)control->key);
	}
	if(control->flags[0] & AES_FLAGS_SELECT_KEY)aes_select_key(keyslot);

	aes_mutexleave();

	return 0;
}
#endif

int ctrserver_processcmd(u32 cmdid, u32 *pxibuf, u32 *bufsize)
{
	int ret=0;
	u32 rw, openflags;
	u32 pos, val;
	u64 val64;
	u64 *val64ptr;
	u32 *addr;
	u16 *ptr16;
	u8 *ptr8;
	u32 size=0;
	u32 tmpsize=0, tmpsize2=0;
	u32 bufpos = 0;
	u32 *mmutable;
	u32 *buf = (u32*)pxibuf[0];

	#ifdef ENABLE_OLDFS
	u32 chunksize;
	u64 tmpsize64=0;
	u8 *buf8 = (u8*)buf;
	u32 fsfile_ctx[8];
	u16 filepath[0x100];
	#endif

	#ifdef ENABLE_DMA
	u32 dmaconfig[24>>2];
	#endif

	#ifdef ENABLEAES
	if(cmdid==CMD_AESCONTROL)
	{
		if(sizeof(aescontrol) != 40 || *bufsize != 40)return -1;
		*bufsize = 0;
		return ctrserver_process_aescontrol((aescontrol*)buf);
	}

	if(cmdid==CMD_AESCTR || cmdid==CMD_AESCBCDEC || cmdid==CMD_AESCBCENC)
	{
		aes_mutexenter();

		if(cmdid==CMD_AESCTR)
		{
			aes_ctr_crypt(buf, *bufsize);
		}
		else if(cmdid==CMD_AESCBCDEC)
		{
			aes_cbc_decrypt(buf, *bufsize);
		}
		else if(cmdid==CMD_AESCBCENC)
		{
			aes_cbc_encrypt(buf, *bufsize);
		}

		aes_mutexleave();

		return 0;
	}
	#endif

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

	if(cmdid==0x31)
	{
		val64ptr = (u64*)buf;
		val64 = svcGetSystemTick();
		svcSleepThread(1000000000LL);
		*val64ptr = svcGetSystemTick() - val64;
		*bufsize = 8;
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

	#ifdef ENABLE_DMA
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
	#endif

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

	#ifdef ENABLE_OLDFS
	if(cmdid==0xc1)
	{
		openflags = buf[0];
		rw = 1;
		if(openflags & 2)rw = 0;//rw=1 for reading, 0 for writing

		memset(fsfile_ctx, 0, sizeof(fsfile_ctx));
		memset(filepath, 0, sizeof(filepath));

		for(pos=0; pos<0xff; pos++)
		{
			filepath[pos] = buf8[4 + (rw*4) + pos];
			if(filepath[pos]==0)break;
		}

		pos+= 4 + (rw*4) + 1;
		size = buf[1];
		if(rw==0)size = *bufsize - pos;

		if((ret = arm9_fopen(fsfile_ctx, (u8*)filepath, openflags))!=0)
		{
			*bufsize = 16;
			buf[0] = 0;
			buf[1] = (u32)ret;
			buf[2] = 0;
			buf[3] = 0;
			return 0;
		}

		if(rw==1 && size==0)
		{
			tmpsize64 = 0;
			ret = arm9_GetFSize(fsfile_ctx, &tmpsize64);
			size = (u32)tmpsize64;

			if(ret!=0)
			{
				arm9_fclose(fsfile_ctx);
				*bufsize = 16;
				buf[0] = 1;
				buf[1] = (u32)ret;
				buf[2] = 0;
				buf[3] = 0;
				return 0;
			}
		}

		*bufsize = 16;
		tmpsize = 0;
		bufpos = 0;
		chunksize = 0x200;
		tmpsize2 = 0;

		while(bufpos < size)
		{
			if(size - bufpos < chunksize)chunksize = size - bufpos;

			tmpsize = 0;
			if(rw==1)ret = arm9_fread(fsfile_ctx, &tmpsize, (u32*)&buf8[(4*4) + bufpos], chunksize);
			if(rw==0)ret = arm9_fwrite(fsfile_ctx, &tmpsize, (u32*)&buf8[pos + bufpos], chunksize, 1);

			bufpos+= tmpsize;
			tmpsize2+= tmpsize;
			if(ret!=0 || tmpsize==0)break;
		}

		buf[0] = 2;
		buf[1] = (u32)ret;
		if(tmpsize64==0)tmpsize64 = (u64)size;
		memcpy(&buf[2], &tmpsize64, 8);

		if(rw==1 && ret==0)*bufsize += tmpsize2;
		if(rw==0)
		{
			*bufsize+= 4;
			buf[4] = tmpsize2;
		}

		arm9_fclose(fsfile_ctx);

		return 0;
	}
	#endif

	if(cmdid==0xc2)
	{
		if(*bufsize != 8)
		{
			*bufsize = 0;
			return 0;
		}

		*bufsize = (buf[1] * 0x200) + 4;
		buf[0] = nand_readsector(buf[0], &buf[1], buf[1]);//buf[0]=sector#, buf[1]=sectorcount
		if(buf[0]!=0)*bufsize = 4;

		return 0;
	}

	#ifdef ENABLE_GAMECARD
	if(cmdid==0xc3)
	{
		if(*bufsize != 8)
		{
			*bufsize = 0;
			return 0;
		}

		pos = buf[0];
		size = buf[1];
		bufpos = 0;

		*bufsize = (size * 0x200) + 4;
		buf[0] = gamecard_readsectors(&buf[1], buf[0], buf[1]);//buf[0]=sector#, buf[1]=sectorcount
		if(buf[0]!=0)*bufsize = 4;

		svcSleepThread(800000000LL);

		return 0;
	}
	#endif

	#ifdef ENABLE_GAMECARD
	if(cmdid==0xd0)
	{
		*bufsize = 0x44;
		memset(buf, 0, 0x44);
		buf[0] = ctrcard_cmdc6(&buf[1]);
		return 0;
	}
	#endif

	#ifdef ENABLEAES
	if(cmdid==0xe2)
	{
		if(*bufsize != 0x14)return -2;

		*bufsize = 0;

		aes_mutexenter();

		aes_set_xkey(buf[0], &buf[1]);

		aes_mutexleave();

		return 0;
	}
	#endif

	if(cmdid==0xf0)
	{
		if(*bufsize!=16)
		{
			*bufsize = 0;
			return 0;
		}

		if(buf[3]==0)
		{
			mmutable = get_kprocessptr(((u64)buf[0]) | ((u64)buf[1]<<32), 0, 1);
			if(mmutable)
			{
				if((((RUNNINGFWVER & 0xff) < 44) && buf[2]>=0x20000000) || (((RUNNINGFWVER & 0xff) >= 44) && buf[2]>=0x40000000))//Verify the the specified address is actually within userland memory for the mmutable.
				{
					buf[0] = 0;
				}
				else
				{
					buf[0] = (u32)mmutable_convert_vaddr2physaddr(mmutable, buf[2]);
				}
				if(buf[0]==0)buf[0] = ~1;
			}
			else
			{
				buf[0] = ~0;
			}
		}
		else
		{
			buf[0] = (u32)get_kprocessptr(((u64)buf[0]) | ((u64)buf[1]<<32), 0, buf[3]-1);
			if(buf[0]==0)buf[0] = ~0;
		}
		*bufsize = 4;

		return 0;
	}

	if(cmdid==0xf1)
	{
		if(*bufsize != 4)
		{
			*bufsize = 0;
			return 0;
		}

		ptr8 = get_kprocess_contextid((u32*)buf[0]);
		buf[0] = 0;
		if(ptr8)buf[0] = *ptr8;

		*bufsize = 4;
		
		return 0;
	}

	return -2;
}

void pxidev_cmdhandler_cmd0handler(u32 *cmdbuf)
{
	int ret=0;
	u32 payloadsize=0;
	u32 type;

	type = cmdbuf[1];

	if(type==0x43565253 && cmdbuf[0]==0x000000c2)//"SRVC"
	{
		payloadsize = cmdbuf[3];
		ret = ctrserver_processcmd(cmdbuf[2], (u32*)cmdbuf[5], &payloadsize);
	}

	cmdbuf[0] = 0x00000040;
	cmdbuf[1] = (u32)ret;

	if(type==0x43565253)//"SRVC"
	{
		cmdbuf[0] = 0x00000081;
		cmdbuf[2] = payloadsize;
		cmdbuf[3] = 4;
	}
}

#ifdef ENABLE_GETEXHDRHOOK
void pxipmcmd1_getexhdr(u32 *exhdr)
{
	u32 *servlist;
	u8 *exhdr8 = (u8*)exhdr;
	u32 pos;

	#ifdef ENABLE_ARM11PROCLIST_OVERRIDE
	u32 pos2;
	s_arm11_processoverride_entry *ent = NULL;
	u16 tmp_path[0x20];
	#endif

	if(exhdr[0] == ((u32)(ARM11CODELOAD_PROCNAME)) && exhdr[1] == ((u32)(ARM11CODELOAD_PROCNAME>>32)))//Only modify the exheader for this block when the exhdr name matches ARM11CODELOAD_PROCNAME.
	{
		servlist = &exhdr[0x250>>2];

		#ifdef ENABLE_ARM11CODELOAD_SERVACCESSCONTROL_OVERWRITE
		memset(servlist, 0, 0x100);
		memcpy(servlist, arm11codeload_servaccesscontrol, sizeof(arm11codeload_servaccesscontrol));
		#endif

		#ifndef DISABLE_FSACCESSINFO_OVERWRITE
		for(pos=0x248; pos<0x248+0x7; pos++)exhdr8[pos] = 0xFF;//Set FS accessinfo to all 0xFF.
		#endif

		if(exhdr[0] == 0x706c64)//When loading code under dlp-module, increase the .data section size by 0x20000-bytes so that there's enough space for ctrserver(and some unused space as well).
		{
			exhdr[(0x30+4)>>2] += 0x20;
			exhdr[(0x30+8)>>2] += (0x20<<12);
		}
	}

	if(exhdr[0]==0x45454154)//"TAEE" for NES VC for TLoZ
	{
		for(pos=0x248; pos<0x248+0x7; pos++)exhdr8[pos] = 0xFF;//Set FS accessinfo to all 0xFF.
	}

	#ifdef ADDEXHDR_SYSMODULE_DEPENDENCY
	#ifdef ADDEXHDR_SYSMODULE_DEPENDENCY_PADCHECK
	if(!((*((vu16*)0x10146000) & ADDEXHDR_SYSMODULE_DEPENDENCY_PADCHECK)))
	{
	#endif
		if(exhdr[0]==ADDEXHDR_SYSMODULE_DEPENDENCY)//Add the dlp sysmodule to the specified process header.
		{
			for(pos=(0x40>>2); pos<(0x1c0>>2); pos+=2)
			{
				if(!(exhdr[pos]==0 && exhdr[pos+1]==0))continue;

				exhdr[pos] = 0x00002c02;//nim module, so that most modules required for using networking gets loaded.
				exhdr[pos+1] = 0x00040130;
				exhdr[pos+2] = 0x00002b02;//ndm module
				exhdr[pos+3] = 0x00040130;
				exhdr[pos+4] = 0x00002802;//dlp module
				exhdr[pos+5] = 0x00040130;
				break;
			}
		}
	#ifdef ADDEXHDR_SYSMODULE_DEPENDENCY_PADCHECK
	}
	#endif
	#endif

	#ifdef ENABLE_ARM11PROCLIST_OVERRIDE
	for(pos=0; pos<ARM11PROCOVERRIDELIST_TOTALENTRIES; pos++)
	{
		ent = &arm11_processoverride_list->processes[pos];
		if(ent->procname == 0)continue;
		if(memcmp(&ent->procname, exhdr, 8)==0)
		{
			if(ent->exheader_path[0x1f]!=0 || ent->exheader_path[0]==0)return;

			memset(tmp_path, 0, 0x20*2);

			for(pos2=0; pos2<0x1f; pos2++)
			{
				if(ent->exheader_path[pos2] == 0)break;
				tmp_path[pos2] = (u16)ent->exheader_path[pos2];
			}

			loadfile(exhdr, 0x400, tmp_path, (pos2+1) * 2);
			return;
		}
	}
	#endif
}
#endif

u32 *locate_cmdhandler_code(u32 *ptr, u32 size, u32 *pool_cmpdata, u32 pool_cmpdata_wordcount, u32 locate_ldrcc)
{
	u32 pos, i, found;

	pos = 0;
	while(size)
	{
		found = 1;
		for(i=0; i<pool_cmpdata_wordcount; i++)
		{
			if(ptr[pos+i] != pool_cmpdata[i])
			{
				found = 0;
				break;
			}
		}

		if(found)break;

		pos++;
		size-= 4;
	}

	if(found==0)return NULL;

	if(locate_ldrcc==0)return &ptr[pos];//Return a ptr to the address of the beginning of the located .pool data.

	found = 0;

	while(pos)//Locate the "ldrcc pc, [pc, XX, lsl #2]" instruction in this function where the matching .pool data was found.
	{
		if((ptr[pos] & ~0xf) == 0x379ff100)
		{
			found = 1;
			break;
		}

		pos--;
	}

	if(found==0)return NULL;

	pos+=2;

	return &ptr[pos];//Return a ptr to the cmd jump-table.
}

void patch_pxidev_cmdhandler_cmd0(u32 *startptr, u32 size)
{
	u32 *ptr = NULL;
	u32 pool_cmpdata[9] = {0xd9001830, 0x000101c2, 0x00010041, 0x000201c2, 0x00020041, 0x00030102, 0x00030041, 0x00040102, 0x00040041};

	ptr = locate_cmdhandler_code(startptr, size, pool_cmpdata, 9, 1);
	if(ptr==NULL)return;

	*ptr = (u32)pxidev_cmdhandler_cmd0;
	svcFlushProcessDataCache(ptr, 0x4);
}

#ifdef ENABLE_GETEXHDRHOOK
void arm9_pxipmcmd1_getexhdr_writepatch_autolocate(u32 *startptr, u32 size)
{
	u32 *ptr = NULL;
	u32 i, found;
	u32 pool_cmpdata[6] = {0xd900182f, 0xd9001830, 0x00010082, 0x00010041, 0x000200c0, 0x00030040};

	ptr = locate_cmdhandler_code(startptr, size, pool_cmpdata, 6, 0);
	if(ptr==NULL)return;

	found = 0;

	for(i=0; i<(0x400/4); i++)
	{
		if(*ptr == 0xe3a01b01)//"mov r1, #0x400"
		{
			found = 1;
			break;
		}

		ptr = (u32*)(((u32)ptr) - 4);
	}

	if(found==0)return;

	ptr = &ptr[4];
	ptr = (u32*)parse_branch((u32)ptr, 0);

	arm9_pxipmcmd1_getexhdr_writepatch((u32)&ptr[0x24>>2]);
}
#endif

/*void patch_nim(u32 *physaddr)
{
	char *ptr = (char*)physaddr;
	char *targeturl0 = "https://nus.";
	char *targeturl1 = "https://ecs.";
	char *replaceurl0 = "http://yls8.mtheall.com/3ds-soap/NetUpdateSOAP.php";//"http://192.168.1.33/NetUpdateSOAP";
	char *replaceurl1 = "http://yls8.mtheall.com/3ds-soap/ECommerceSOAP.php";//"http://192.168.1.33/ECommerceSOAP";
	u32 pos, i, found0=0, found1=0;

	for(pos=0; pos<0x60000; pos++)
	{
		for(i=0; i<12; i++)
		{
			if(ptr[pos+i] != targeturl0[i])break;

			if(i==11)
			{
				found0 = 1;
				break;
			}
		}

		for(i=0; i<12; i++)
		{
			if(ptr[pos+i] != targeturl1[i])break;

			if(i==11)
			{
				found1 = 1;
				break;
			}
		}

		if(found0)
		{
			for(i=0; i<strlen(replaceurl0)+1; i++)ptr[pos+i] = replaceurl0[i];
			found0 = 0;
		}

		if(found1)
		{
			for(i=0; i<strlen(replaceurl1)+1; i++)ptr[pos+i] = replaceurl1[i];
			found1 = 0;
		}
	}

	//physaddr[0x7d00>>2] = 0xe3a00000;//"mov r0, #0" Patch the NIM command code which returns whether a sysupdate is available.
}*/

#ifdef ENABLE_LOADSD_AESKEYS
void loadsd_aeskeys(u32 *buffer)//This is called from firmlaunch_hookpatches.s.
{
	u64 *ptr64;
	u64 normalkey_bitmask, keyx_bitmask, keyy_bitmask;
	u32 keyslot, pos;

	if(buffer==NULL)return;

	memset(buffer, 0, 0xc18);

	if(loadfile_charpath("/3dshax_aeskeydata.bin", buffer, 0xc18)!=0)return;

	ptr64 = (u64*)buffer;
	normalkey_bitmask = ptr64[0];
	keyx_bitmask = ptr64[1];
	keyy_bitmask = ptr64[2];

	aes_mutexenter();

	pos = 0x18;
	for(keyslot=0; keyslot<0x40; keyslot++)
	{
		if((normalkey_bitmask >> keyslot) & 1)
		{
			aes_set_key(keyslot, &buffer[pos>>2]);
		}

		pos+=0x10;

		if((keyx_bitmask >> keyslot) & 1)
		{
			aes_set_xkey(keyslot, &buffer[pos>>2]);
		}

		pos+=0x10;

		if((keyy_bitmask >> keyslot) & 1)
		{
			aes_set_ykey(keyslot, &buffer[pos>>2]);
		}

		pos+=0x10;
	}

	aes_mutexleave();
}
#endif
void twlstuff();
void thread_entry()
{
	u32 pos=0;
	//u32 *axiwram = (u32*)0x1FF80000;
	//u32 *menutextphys;// = (u32*)0x2696e000;
	/*u32 *nwm_physaddr;// = (u32*)0x276cf000;
	u32 *cecd_physaddr;
	u32 *nim_physaddr;
	u32 *ns_physaddr;
	u32 *csnd_physaddr;
	u32 *gsp_physaddr;*/
	//u32 *ptr;
	u32 debuginitialized = 0;
	//u32 totalmenu_textoverwrite = 0;
	u32 *mmutable, *ptr;

	if(FIRMLAUNCH_RUNNINGTYPE==0)svcSleepThread(2000000000LL);

	if(((u8)RUNNINGFWVER)==39)//v7.0
	{
		*((u16*)0x0805ed34) |= 1;
		svcFlushProcessDataCache((u32*)0x0805ed34, 0x4);//Patch the code which reads the arm9 access-control mount flags, so that all of these archives are accessible.

		//arm9general_debughook_writepatch(0x0802ea90);//Hook the gamecard v6.0 savegame keyY init code for debug.
		//arm9general_debughook_writepatch(0x0807b49c);

		//axiwram[0x1b6f8>>2] = ~0;

		/*ptr = mmutable_convert_vaddr2physaddr(get_kprocessptr(0x707367, 0, 1), 0x10a474);//Patch the GSP module gxcmd4 code so that it uses the input addresses as the physical addresses, when the addresses are outside of the VRAM/LINEAR-mem range.
		ptr[0x8>>2] = 0xe1a03001;//"mov r3, r1"
		ptr[0x2C>>2] = 0xe1a01002;//"mov r1, r2"*/
	}
	else if(((u8)RUNNINGFWVER)==40)//v7.2
	{
		*((u16*)0x0805ed38) |= 1;
		svcFlushProcessDataCache((u32*)0x0805ed38, 0x4);//Patch the code which reads the arm9 access-control mount flags, so that all of these archives are accessible.
	}
	else if(((u8)RUNNINGFWVER)==44)//v8.0
	{
		*((u16*)0x0805ef38) |= 1;
		svcFlushProcessDataCache((u32*)0x0805ef38, 0x4);//Patch the code which reads the arm9 access-control mount flags, so that all of these archives are accessible.
	}

	patch_pxidev_cmdhandler_cmd0((u32*)proc9_textstartaddr, 0x080ff000-proc9_textstartaddr);
	#ifdef ENABLE_GETEXHDRHOOK
	arm9_pxipmcmd1_getexhdr_writepatch_autolocate((u32*)proc9_textstartaddr, 0x080ff000-proc9_textstartaddr);
	#endif

	writepatch_arm11kernel_kernelpanicbkpt((u32*)0x1FF80000, 0x80000);

	//ns_physaddr = patch_mmutables(0x736e, 0, 0);

	while(1)
	{
		//memset(framebuf_addr, pos, 0x46500*10);
		if((*((vu16*)0x10146000) & 0x800) == 0 && !debuginitialized)//button Y
		{
			//menutextphys = patch_mmutables(0x756e656d, 0, 0);

			//read_gamecard();

			#ifdef ENABLE_DUMP_NANDIMAGE
			dump_nandimage();
			#endif
			
			/*mmutable = (u32*)get_kprocessptr(0x707041727443LL, 0, 1);
			if(mmutable)
			{
				ptr = (u32*)mmutable_convert_vaddr2physaddr(mmutable, 0x2e772c);
				if(ptr)*ptr = 0xE1200070;
			}*/
			//memset((u32*)0x18000000, pos | (pos<<8) | (pos<<16) | (pos<<24), 0x00600000);
			////memset(&framebuf_addr[(0x46500)>>2], 0x88888888, 0x46500);

			//sdcryptdata2();

			//mountcontent_nandsd_writehookstub();

			#ifdef ENABLE_ARM11KERNEL_DEBUG
			if(FIRMLAUNCH_RUNNINGTYPE==0)write_arm11debug_patch();
			#endif
			debuginitialized = 1;
			//launchcode_kernelmode(twlstuff);
			//if(((u8)RUNNINGFWVER)==39)axiwram[0x14ab0>>2] = 0xE1200070;//Patch the start of the svc7c handler(at the push instruction) with: "bkpt #0".

			//dumpmem((u32*)0x01ffb700, arm9_rsaengine_txtwrite_hooksz);

			//patch_mmutables(0x7465736dLL, 1, 0);//patch the mset MMU tables so that .text is writable.

			//axiwram[0x1C3D0>>2] = (axiwram[0x1C3D0>>2] & ~(0xff<<24)) | 0xea000000;//Change the conditional branch to an unconditional branch, for the arm11 kernel exheader "kernel release version" field. With this patch the kernel will not return an error because of this version field, in this function.

			//menutextphys = patch_mmutables(0x756e656d, 0, 0);//"menu"
			//nwm_physaddr = patch_mmutables(0x6d776e, 0, 0);//"nwm"
			//cecd_physaddr = patch_mmutables(0x64636563, 0, 0);//"cecd"

			/*menutextphys[0xe23e4>>2] = 0xe3a03000;//"mov r3, #0" These three patches modify menu code which calls read_logo(), so that the input params result in the gamecard logo being read.
			menutextphys[0xe23e8>>2] = 0xe3a02000;//"mov r2, #0"
			menutextphys[0xe23ec>>2] = 0xe3a01002;//"mov r1, #2"

			menutextphys[0x1de6c>>2] = 0xe3a03000;//"mov r3, #0" These three patches do the same thing as above, except this is for a different read_logo() call.
			menutextphys[0x1de70>>2] = 0xe3a02000;//"mov r2, #0"
			menutextphys[0x1de74>>2] = 0xe3a01002;//"mov r1, #2"*/

			//*((u32*)0x1FF84088) = ~0;

			//nwm_physaddr[0xe844>>2] = ~0;//Patch the nwm psps EncryptDecryptAes() call.

			//cecd_physaddr[0x18e8>>2] = ~0;
			//*((u16*)&cecd_physaddr[0x1854>>2]) = 0x2000;

			/*if(((u8)RUNNINGFWVER)==34)
			{
				nim_physaddr = patch_mmutables(0x6d696e, 0, 0);//"nim"
				patch_nim(nim_physaddr);
			}*/

			//csnd_physaddr = patch_mmutables(0x646e7363, 0, 0);//"csnd"

			//((u16*)csnd_physaddr)[0x104e>>1] = 0xBE00;//In the CSND function handling type0 shared-mem commands, patch the instruction immediately after the memcpy call which copies the command data to stack. "bkpt #0"

			//gsp_physaddr = patch_mmutables(0x707367, 0, 0);//"gsp"

			//gsp_physaddr[0xaaf0>>2] = 0xE1200070;//Patch the "mov r3, #0" at the start of the GSP module function for handling gxcmd3, with: "bkpt #0".

			//*((u32*)0x808a944) = 0;//Patch the pxifs cmdhandler so that it always executes the code-path for invalid cmdid.
			//svcFlushProcessDataCache((u32*)0x808a944, 0x4);
			/**((u32*)0x808d81c) = 0xe3a05000;//"mov r5, #0"
			svcFlushProcessDataCache((u32*)0x808d81c, 0x4);//Patch the pxipm cmdhandler so that it always executes the code-path for invalid cmdid.
			*((u32*)0x8087a48) = 0;//Patch the pxi gamecard cmdhandler so that it always executes the code-path for invalid cmdid.
			svcFlushProcessDataCache((u32*)0x8087a48, 0x4);
			*((u32*)0x808ddf4) = 0;//Patch the pxiam9 cmdhandler so that it always executes the code-path for invalid cmdid.
			svcFlushProcessDataCache((u32*)0x808ddf4, 0x4);
			// *((u32*)0x808d98c) = 0;//Patch the pxips9 cmdhandler so that it always executes the code-path for invalid cmdid.
			//svcFlushProcessDataCache((u32*)0x808d98c, 0x4);
			*((u32*)0x808d58c) = 0;//Patch the pxidev cmdhandler so that it always executes the code-path for invalid cmdid.
			svcFlushProcessDataCache((u32*)0x808d58c, 0x4);

			*((u32*)0x808db1c) = 0xe3a00000;//"mov r0, #0"
			svcFlushProcessDataCache((u32*)0x808db1c, 0x4);//Patch the bl used for calling the pxips9 RSA-verify func in the pxips9 cmdhandler, with the above instruction.
			*/
		}

		pos+=0x20;

		//if((*((vu16*)0x10146000) & 2) == 0)break;

		/*if((*((vu16*)0x10146000) & 2) == 0 && totalmenu_textoverwrite < 2 && FIRMLAUNCH_RUNNINGTYPE==0)//button B
		{
			//menutextphys = patch_mmutables(0x756e656d, 0, 0);

			memset(menutextphys, ~0, 0x106000);
			while((*((vu16*)0x10146000) & 2) == 0);
			totalmenu_textoverwrite++;
		}*/

		#ifdef ENABLE_ARM11KERNEL_DEBUG
		dump_arm11debuginfo();
		#endif

		if((*((vu16*)0x10146000) & 0x40c) == 0)break;//button X, Select, and Start

		//svcSleepThread(1000000000LL);
	}

	dump_fcramaxiwram();

	svcExitThread();

	while(1);
}

void main_startupcommon()
{
	u32 threadhandle = 0;

	#ifdef ENABLE_ARM11PROCLIST_OVERRIDE
	u32 pos=0;
	u16 tmp_path[0x20];
	char *filepath = "/3dshax_proclistoverride.bin";
	#endif

	#ifndef DISABLE_A9THREAD
	memset(thread_stack, 0, THREAD_STACKSIZE);
	svcCreateThread(&threadhandle, thread_entry, 0, (u32*)&thread_stack[THREAD_STACKSIZE>>3], 0x3f, ~1);
	#endif

	#ifdef ENABLE_ARM11PROCLIST_OVERRIDE
	memset(arm11_processoverride_list, 0, sizeof(s_arm11_processoverride_list));

	memset(tmp_path, 0, 0x20*2);
	for(pos=0; pos<strlen(filepath); pos++)
	{
		if(pos>=0x1f)break;
		tmp_path[pos] = (u16)filepath[pos];
	}

	loadfile((u32*)arm11_processoverride_list, sizeof(s_arm11_processoverride_list), tmp_path, (pos+1) * 2);
	#endif
}

int main(void)
{
	u32 pos=0;
	u32 *ptr;
	u8 *ptr8;

	launchcode_kernelmode(changempu_memregions);

	if(FIRMLAUNCH_RUNNINGTYPE==0)
	{
		if((*((vu16*)0x10146000) & 1) == 0)framebuf_addr = /*0x18000000+0x1e6000;*/get_framebuffers_addr();

		if((*((vu16*)0x10146000) & 1) == 0)
		{
			memset(framebuf_addr, 0xffffffff, 0x46500);
			memset(&framebuf_addr[(0x46500)>>2], 0xffffffff, 0x46500);
		}
	}

	if((*((vu16*)0x10146000) & 1) == 0 && FIRMLAUNCH_RUNNINGTYPE==0)memset(framebuf_addr, pos | (pos<<8) | (pos<<16) | (pos<<24), 0x46500*10);

	if(FIRMLAUNCH_RUNNINGTYPE==0)
	{
		if((*((vu16*)0x10146000) & 0x100) == 0)//button R
		{
			FIRMLAUNCH_CLEARPARAMS = 1;
			patch_proc9_launchfirm();
			//load_arm11code(NULL, 0, 0x707041727443LL, arm11code_filepath);

			writearm11_firmlaunch_usrpatch();
			svcSleepThread(10000000000LL);
		}

		if((*((vu16*)0x10146000) & 0x200) == 0)//button L
		{
			load_arm11code(NULL, 0, 0x707041727443LL, arm11code_filepath);
		}

		if((*((vu16*)0x10146000) & 1) == 0)
		{
			memset(framebuf_addr, 0x40404040, 0x46500);
			memset(&framebuf_addr[(0x46500)>>2], 0x40404040, 0x46500);
		}

		//while(*((vu16*)0x10146000) & 2);

		main_startupcommon();
	}
	else
	{
		if(FIRMLAUNCH_RUNNINGTYPE==2)
		{
			FIRMLAUNCH_CLEARPARAMS = 1;
			patch_proc9_launchfirm();
		}
		else if((*((vu16*)0x10146000) & 0x100))//button R not pressed
		{
			#ifdef ENABLE_ARM11KERNEL_DEBUG
			write_arm11debug_patch();
			#endif

			#ifdef ENABLE_CONFIGMEM_DEVUNIT
			ptr8 = NULL;
			while(ptr8 == NULL)ptr8 = (u32*)mmutable_convert_vaddr2physaddr(get_kprocessptr(0x697870, 0, 1), 0x1FF80014);
			*ptr8 = 0;
			#endif

			#ifdef ENABLE_FIRMLAUNCH_HOOK
			FIRMLAUNCH_CLEARPARAMS = 0;
			patch_proc9_launchfirm();
			#endif

			main_startupcommon();
		}
	}

	if((*((vu16*)0x10146000) & 0x40) == 0)ARM11CODELOAD_PROCNAME = 0x726564697073LL;//0x726564697073LL = "spider". Change the the code-load procname to this, when the Up button is pressed.
	if((*((vu16*)0x10146000) & 0x80) == 0)ARM11CODELOAD_PROCNAME = 0x766c6fLL;//Same as above except with Miiverse applet + Down button.

	//while(1);

	return 0;
}

