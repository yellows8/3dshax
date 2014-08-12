#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <nds/ndstypes.h>

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
u32 *get_arm11debuginfo_physaddr();
void write_arm11debug_patch();
void writepatch_arm11kernel_svcaccess();
u32 init_arm9patchcode3();
void arm9_launchfirm();
u32 getsp();
u32 getcpsr();
void patch_nandredir(u32 readfunc_patchadr, u32 writefunc_patchadr);
void arm9_pxipmcmd1_getexhdr_writepatch(u32 addr);
void arm9general_debughook_writepatch(u32 addr);
u32 generate_branch(u32 branchaddr, u32 targetaddr, u32 flag);//branchaddr = addr of branch instruction, targetaddr = addr to branch to, flag = 0 for regular branch, non-zero for bl. (ARM-mode)
u32 parse_branch(u32 branchaddr, u32 branchval);
u32 parse_branch_thumb(u32 branchaddr, u32 branchval);
void writearm11_firmlaunch_usrpatch();

void call_arbitaryfuncptr(void* funcptr, u32 *regdata);

extern u32 arm9_stub[];
extern u32 arm9_stub2[];
extern u32 arm9dbs_stub[];
extern u32 arm11_stub[];
extern u8 rsamodulo_slot0[];
extern u32 FIRMLAUNCH_RUNNINGTYPE;
extern u32 RUNNINGFWVER;
extern u32 arm9_rsaengine_txtwrite_hooksz;

void pxidev_cmdhandler_cmd0();
void mountcontent_nandsd_writehookstub();

u32 *gamecard_archiveobj = NULL;

u32 input_filepos;
u32 input_filesize = 0;

u32 *framebuf_addr = NULL;

//u32 pxibuf[0x200>>2];

//u32 debuginfo[0x200>>2];

//u32 debuglogbuf[0x200>>2];
//u32 debuglogbuf_cursz = 0;

u32 *fileobj_debuginfo = NULL;
u32 debuginfo_pos = 0;

//u16 meta_filepath[] = {0x2F, 0x33, 0x64, 0x73, 0x68, 0x61, 0x78, 0x5F, 0x6D, 0x65, 0x74, 0x61, 0x2E, 0x62, 0x69, 0x6E, 0x00}; //wchar "/3dshax_meta.bin"

//u16 ctr_filepath[] = {0x2F, 0x33, 0x64, 0x73, 0x68, 0x61, 0x78, 0x5F, 0x66, 0x69, 0x6C, 0x65, 0x70, 0x61, 0x74, 0x68, 0x2E, 0x62, 0x69, 0x6E, 0x00}; //wchar "/3dshax_filepath.bin"

u16 arm11code_filepath[] = {0x2F, 0x33, 0x64, 0x73, 0x68, 0x61, 0x78, 0x5F, 0x61, 0x72, 0x6D, 0x31, 0x31, 0x2E, 0x62, 0x69, 0x6E, 0x00}; //wchar "/3dshax_arm11.bin"

/*u16 subgfxbin_filepath[] = {0x2F, 0x73, 0x75, 0x62, 0x67, 0x66, 0x78, 0x2E, 0x62, 0x69, 0x6E, 0x00};//wchar "/subgfx.bin"

u16 maingfx0bin_filepath[] = {0x2F, 0x6D, 0x61, 0x69, 0x6E, 0x67, 0x66, 0x78, 0x30, 0x2E, 0x62, 0x69, 0x6E, 0x00};//wchar "/maingfx0.bin" 0x62, 0x69, 0x6E

u16 maingfx1bin_filepath[] = {0x2F, 0x6D, 0x61, 0x69, 0x6E, 0x67, 0x66, 0x78, 0x31, 0x2E, 0x62, 0x69, 0x6E, 0x00};//wchar "/maingfx1.bin"
*/

/*u16 subgfxtga_filepath[] = {0x2F, 0x73, 0x75, 0x62, 0x67, 0x66, 0x78, 0x2E, 0x74, 0x67, 0x61, 0x00};//wchar "/subgfx.tga"

u16 maingfx0tga_filepath[] = {0x2F, 0x6D, 0x61, 0x69, 0x6E, 0x67, 0x66, 0x78, 0x30, 0x2E, 0x74, 0x67, 0x61, 0x00};//wchar "/maingfx0.tga" 0x62, 0x69, 0x6E

u16 maingfx1tga_filepath[] = {0x2F, 0x6D, 0x61, 0x69, 0x6E, 0x67, 0x66, 0x78, 0x31, 0x2E, 0x74, 0x67, 0x61, 0x00};//wchar "/maingfx1.tga"

u16 texturetga_filepath[] = {0x2F, 0x74, 0x65, 0x78, 0x74, 0x75, 0x72, 0x65, 0x2E, 0x74, 0x67, 0x61, 0x00};

u16 texturektx_filepath[] = {0x2F, 0x74, 0x65, 0x78, 0x74, 0x75, 0x72, 0x65, 0x2E, 0x6B, 0x74, 0x78, 0x00};

u16 texturepkm_filepath[] = {0x2F, 0x74, 0x65, 0x78, 0x74, 0x75, 0x72, 0x65, 0x2E, 0x70, 0x6B, 0x6D, 0x00};*/

#define THREAD_STACKSIZE 0x800
u32 *thread_stack = (u32*)(0x02000000-THREAD_STACKSIZE);//[THREAD_STACKSIZE>>2];

u64 ARM11CODELOAD_PROCNAME = 0x706c64;//0x706c64 = "dlp", 0x726564697073LL = "spider"

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
"frd:u"
};

void load_arm11code(u32 *loadptr, u32 maxloadsize, u64 procname)
{
	//int i;
	u32 *fileobj = NULL;
	u32 *app_physaddr = NULL;
	u32 *ptr = NULL;
	u32 pos;

	//if(loadptr==NULL)app_physaddr = patch_mmutables(procname, 1, 0);

	writepatch_arm11kernel_svcaccess();

	/*if(RUNNINGFWVER==0x1F)ptr = (u32*)0x1FF827CC;
	if(RUNNINGFWVER==0x2E)ptr = (u32*)0x1FF822A8;

	*ptr = 0;*/

	if(openfile(sdarchive_obj, 4, arm11code_filepath, 0x24, 1, &fileobj)!=0)return;
	input_filesize = getfilesize(fileobj);
	ptr = loadptr;
	if(ptr==NULL)
	{
		ptr = app_physaddr;
	}
	if(maxloadsize>0 && maxloadsize<input_filesize)return;
	fileread(fileobj, ptr, input_filesize, 0);

	if(loadptr)
	{
		if(loadptr[1]==0x58584148)
		{
			if(loadptr[2]==0x434f5250 && loadptr[3]==0x454d414e)//Set the process-name when these fields are set correctly.
			{
				loadptr[2] = (u32)(procname);
				loadptr[3] = (u32)(procname>>32);
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

void launch_firm()
{
	u32 *ptr;
	u32 *arm9_patchaddr; //= (u32*)0x8086b98;
	u32 pos;

	/*void (*funcptr)() = (void*)0x8086a7c;
	void (*funcptr2)(u32) = (void*)0x8087680;
	void (*funcptr3)() = (void*)0x8028089;

	void (*funcptr_signalfirmlaunch)(u32*) = (void*)0x8088560;*/

	//*((u32*)0x080c4afc) = 0x0809796c;
	//*((u32*)0x10010000) = 1;
	//*((u8*)(0x10000010)) = 0;

	ptr = (u32*)0x08086a7c;//FWVER 0x1F
	if(RUNNINGFWVER==0x2E)ptr = (u32*)0x080854d8;
	if(RUNNINGFWVER==0x30)ptr = (u32*)0x080854dc;
	if(RUNNINGFWVER==0x37)ptr = (u32*)0x080857a4;

	arm9_patchaddr = (u32*)&ptr[0x11c>>2];
	if(RUNNINGFWVER==0x37)arm9_patchaddr = (u32*)&ptr[0x124>>2];

	arm9_patchaddr[0] = arm9_stub[0];
	arm9_patchaddr[1] = arm9_stub[1];
	arm9_patchaddr[2] = arm9_stub[2];

	pos = 0;
	if(RUNNINGFWVER==0x37)pos = 8;

	//*((u32*)0x8086b7c) = 0xe3a00000;//0xe3e00000;//patch the launch_firm fs_openfile() blx to "mov r0, #0".
	ptr[(0x140 + pos)>>2] = 0xe3a00000;//patch the fileread blx for the 0x100-byte FIRM header to "mov r0, #0".
	ptr[(0x154 + pos)>>2] = 0xe3a00000;//patch the bne branch after the fileread call.
	ptr[(0x170 + pos)>>2] = 0xe3a00000;//patch the FIRM signature read.
	ptr[(0x17C + pos)>>2] = 0xe3a00c01;//"mov r0, #0x100".
	//*((u32*)0x8086c0c) = 0xe1a00000;//nop the fs_closefile() call, this patch isn't needed since this function won't really do anything since the file ctx wasn't initialized via fs_openfile.

	pos = 0;
	if(RUNNINGFWVER==0x37)pos = 0x14;	

	ptr[(0x3D8 + pos)>>2] = 0xe3a00000;//patch the RSA signature verification func call.

	pos = 0;
	if(RUNNINGFWVER==0x37)pos = 0x18;

	ptr[(0x594 + pos)>>2] = 0xe3a00000;//patch the func-call which reads the encrypted ncch firm data.
	//*((u32*)0x8087048) = 0xe1a00000;//patch out the fs_closefile() func-call.
	ptr[(0x5D0 + pos)>>2] = 0xe1a00000;//patch out the func-call which is immediately after the fs_closefile call. (FS shutdown stuff)

	svcFlushProcessDataCache(ptr, 0x630);

	/*ptr = (u32*)0x8086a2c;//FWVER 0x1F
	if(RUNNINGFWVER==0x2E)ptr = (u32*)0x8085488;
	*ptr = 0xe1a00000;//nop "mov r0, r0" (FS shutdown function called by main() right before the launch_firm() call at the end of main())
	svcFlushProcessDataCache(ptr, 0x4);*/

	init_arm9patchcode3();

	//rsaengine_setpubk(0, 2048, rsamodulo_slot0, 0x00010001);
	//if(*((u32*)0x01ffcd00) == 0)*((u32*)0x01ffcd00) |= 1;

	svcFlushProcessDataCache((u32*)0x80ff000, 0xc00);

	//memset(framebuf_addr, 0x80808080, 0x46500*2);

	//*((u32*)0x1ff86e74) = generate_branch(0xfff66e74, 0xfff748c0, 0);//Patch the arm11kernel svcSendSyncRequest code so that it branches to the svc7c code, which then triggers FIRM launch.
	//funcptr_signalfirmlaunch((u32*)0x80C4AFC);

	//funcptr2(0x08097969);//called by wait_firmevent().
	//funcptr3();

	//funcptr();//launch_firm()

	//while(1);

	//arm9_launchfirm();
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
	u32 *codebin_physaddr, total_codebin_size;
	u32 *ptr;
	u32 *mmutable;

	procname = ((u64)debuginfo_ptr[5]) | (((u64)debuginfo_ptr[6])<<32);

	codebin_physaddr = (u32*)debuginfo_ptr[4];
	total_codebin_size = debuginfo_ptr[3]<<12;

	if(procname==ARM11CODELOAD_PROCNAME)
	{
		load_arm11code(codebin_physaddr, total_codebin_size, procname);
		return;
	}

	if(procname==0x45454154)//"TAEE", NES VC for TLoZ.
	{
		mmutable = (u32*)get_kprocessptr(0x726564616f6cLL, 0, 1);//"loader"
		if(mmutable==NULL)return;

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
	}
}

#ifdef ENABLE_ARM11KERNEL_DEBUG
void dump_arm11debuginfo()
{
	//u64 procname=0;
	u32 pos=0;
	vu32 *debuginfo_ptr = NULL;//0x21b00000;//0x18300000
	//u32 i;
	u16 filepath[64];
	char *path = (char*)"/3dshax_debug.bin";
	//u32 *ptr, val=0;

	debuginfo_ptr = (vu32*)((u32)get_arm11debuginfo_physaddr() + 0x200);

	if(fileobj_debuginfo==NULL)
	{
		memset(filepath, 0, 64*2);
		for(pos=0; pos<strlen(path); pos++)filepath[pos] = (u16)path[pos];

		if(openfile(sdarchive_obj, 4, filepath, (strlen(path)+1)*2, 6, &fileobj_debuginfo)!=0)return;
	}

	
	if(*debuginfo_ptr != 0x58584148)return;

	if(debuginfo_ptr[1]==0x3131444c)//"LD11"
	{
		handle_debuginfo_ld11(debuginfo_ptr);
	}
	else if(debuginfo_ptr[1]==0x35375653)//"SV75"
	{
		//procname = ((u64)debuginfo_ptr[3]) | (((u64)debuginfo_ptr[4])<<32);
		//if(procname==ARM11CODELOAD_PROCNAME)patch_mmutables(procname, 1, 1);//Only enable this when loading arm11-code which doesn't change mem-permissions itself, etc.
	}
	else
	{
		filewrite(fileobj_debuginfo, (u32*)debuginfo_ptr, debuginfo_ptr[2], debuginfo_pos);
		debuginfo_pos+= debuginfo_ptr[2];
	}

	if(debuginfo_ptr[1]==0x444d4344)//"DCMD"
	{
		for(pos=(0x220>>2); pos<(0x420>>2); pos+=2)
		{
			if(debuginfo_ptr[pos]!=0 && debuginfo_ptr[pos+1]!=0)
			{
				filewrite(fileobj_debuginfo, (u32*)debuginfo_ptr[pos+1], debuginfo_ptr[pos], debuginfo_pos);
				debuginfo_pos+= debuginfo_ptr[pos];
			}
		}
	}

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

		if(ptr)filewrite(fileobj_debuginfo, ptr, debuginfo_ptr[3+5], debuginfo_pos);
		debuginfo_pos+= debuginfo_ptr[3+5];
	}*/

	//if(debuginfo_ptr[1]==0x47424445)dumpmem((u32*)0x20000000, 0x08000000);
	//if(debuginfo_ptr[1]==0x47424445/* && debuginfo_ptr[0x50>>2] == 0x50505050*/)dumpmem((u32*)0x24cff000/*0x24d02000*/, 0x1a97000);//web-browser 0x08000000 heap //v1024: 0x24cff000 v2050: 0x24d02000
	memset((u32*)debuginfo_ptr, 0, debuginfo_ptr[2]);
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

/*u8* AesKeyScrambler(u8 *Key, u8 *KeyX, u8 *KeyY, u32 flags)//From makerom, not correct after the first loop.
{
	int i;

	// Process KeyX/KeyY to get raw normal key
	for(i = 0; i < 16; i++)
	{
		if(flags & 2)Key[i] = KeyX[i] ^ ((KeyY[i] >> 2) | ((KeyY[i < 15 ? i+1 : 0] & 3) << 6));
		if((flags & 2) == 0)Key[i] = KeyX[i] ^ ((KeyY[i] >> 2) | ((KeyY[i == 0 ? 15 : i-1] & 3) << 6));
	}

	if(flags & 1)
	{
	//#ifndef PUBLIC_BUILD
	const u8 SCRAMBLE_SECRET[16] = {0x51, 0xD7, 0x5D, 0xBE, 0xFD, 0x07, 0x57, 0x6A, 0x1C, 0xFC, 0x2A, 0xF0, 0x94, 0x4B, 0xD5, 0x6C};

	// Apply Secret to get final normal key
	for(i = 0; i < 16; i++)
		Key[i] = Key[i] ^ SCRAMBLE_SECRET[i];
	//#endif
	}

	return Key;
}*/

int ctrserver_processcmd(u32 cmdid, u32 *pxibuf, u32 *bufsize)
{
	int ret=0;
	u32 rw, openflags;
	u32 pos;
	u32 *addr;
	u32 size=0;
	u32 tmpsize=0, tmpsize2=0;
	u32 chunksize;
	u32 bufpos = 0;
	u64 tmpsize64=0;
	u32 *mmutable;
	u32 *buf = (u32*)pxibuf[0];
	u8 *buf8 = (u8*)buf;
	u32 fsfile_ctx[8];
	u16 filepath[0x100];
	//u8 tmpkey[0x10];

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

	/*if(cmdid>=0xc0)
	{
		if(cmdid==0xc0)dumpmem((u32*)payload[0], payload[1]);
		return 0;
	}*/

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
		//buf[0] = SendReadCommand(buf[0]*0x200, 0x200, buf[1], &buf[1]);
		/*while(size)
		{
			buf[0] = 0;//gamecard_readsectors(&buf[1 + bufpos], pos, 1);
			if(buf[0]!=0)break;
			Cart_ReadSectorSD(&buf[1 + bufpos], pos);
			pos++;
			size--;
			bufpos+= 0x200>>2;
		}*/
		if(buf[0]!=0)*bufsize = 4;

		svcSleepThread(800000000LL);

		return 0;
	}
	#endif

	if(cmdid==0xd0)
	{
		*bufsize = 0x44;
		memset(buf, 0, 0x44);
		buf[0] = ctrcard_cmdc6(&buf[1]);
		return 0;
	}

	/*if(cmdid==0xe0)
	{
		*bufsize = 16;
		aestiming_test(buf);
		return 0;
	}*/

	/*if(cmdid==0xe1)
	{
		*bufsize = 16;
		memset(tmpkey, 0, 16);
		AesKeyScrambler(tmpkey, &buf[1], &buf[5], buf[0]);
		memcpy(buf, tmpkey, 16);

		return 0;
	}*/

	#ifdef ENABLEAES
	if(cmdid==0xe2)
	{
		if(*bufsize != 0x14)return -2;

		*bufsize = 0;

		aes_set_xkey(buf[0], &buf[1]);

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
				buf[0] = (u32)mmutable_convert_vaddr2physaddr(mmutable, buf[2]);
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

void pxipmcmd1_getexhdr(u32 *exhdr)
{
	u32 *servlist;
	u8 *exhdr8 = (u8*)exhdr;
	u32 pos;

	if(exhdr[0] == ((u32)(ARM11CODELOAD_PROCNAME)) && exhdr[1] == ((u32)(ARM11CODELOAD_PROCNAME>>32)))//Only modify the exheader for this block when the exhdr name matches ARM11CODELOAD_PROCNAME.
	{
		servlist = &exhdr[0x250>>2];

		memset(servlist, 0, 0x100);
		memcpy(servlist, arm11codeload_servaccesscontrol, sizeof(arm11codeload_servaccesscontrol));

		for(pos=0x248; pos<0x248+0x7; pos++)exhdr8[pos] = 0xFF;//Set FS accessinfo to all 0xFF.
	}

	if(exhdr[0]==0x45454154)//"TAEE", NES VC for TLoZ.
	{
		for(pos=0x248; pos<0x248+0x7; pos++)exhdr8[pos] = 0xFF;//Set FS accessinfo to all 0xFF.
	}
}

u32 *locate_pxicmdhandler_code(u32 *ptr, u32 size, u32 *pool_cmpdata, u32 pool_cmpdata_wordcount, u32 locate_ldrcc)
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

	while(pos)//Locate the "ldrcc pc, [pc, r0, lsl #2]" instruction in this function where the matching .pool data was found.
	{
		if(ptr[pos] == 0x379ff100)
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

	ptr = locate_pxicmdhandler_code(startptr, size, pool_cmpdata, 9, 1);
	if(ptr==NULL)return;

	*ptr = (u32)pxidev_cmdhandler_cmd0;
	svcFlushProcessDataCache(ptr, 0x4);
}

void arm9_pxipmcmd1_getexhdr_writepatch_autolocate(u32 *startptr, u32 size)
{
	u32 *ptr = NULL;
	u32 i, found;
	u32 pool_cmpdata[6] = {0xd900182f, 0xd9001830, 0x00010082, 0x00010041, 0x000200c0, 0x00030040};

	ptr = locate_pxicmdhandler_code(startptr, size, pool_cmpdata, 6, 0);
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

	//dumpmem((u32*)0x08000000, 0x100000);
	//dumpmem((u32*)0x20000000, 0x100000);

	//dumpmem((u32*)0x20000000, 0x100000);
	//memset((u32*)0x2696e000, ~0, 0x134000);
	//svcExitThread();

	//while(*((vu16*)0x10146000) & 2);

	if(FIRMLAUNCH_RUNNINGTYPE==0)svcSleepThread(2000000000LL);

	//while((*((vu16*)0x10146000) & 2) == 0);

	//*((u32*)0x808f194) = 0xe3a01001;//"mov r1, #1" Patch the code calling the function for pxiam9_cmd40() so that the mediatype is always value 1 for SD, so that the install for the updated NAND titles aren't finalized when mediatype=0.
	//svcFlushProcessDataCache((u32*)0x808f194, 0x4);

	//*((u32*)0x08086af8) = 0xe3a04002;//Patch the FIRM launch code so that it uses FIRM programID-low value 2 for NATIVE_FIRM, instead of using the one from PXI.
	//svcFlushProcessDataCache((u32*)0x08086af8, 0x4);

	//axiwram[0x9608>>2] = (axiwram[0x9608>>2] & ~(0xff<<24)) | 0xea000000;//Patch the conditional branch to an unconditional branch, for the check in svcCreateMemoryBlock which normally returns an error when memorytype==application.

	if(RUNNINGFWVER==0x1F)
	{
		//*((u32*)0x8087a50) = (u32)pxidev_cmdhandler_cmd0;//Patch the jump-addr used for "pxi:dev" cmd0.
		//svcFlushProcessDataCache((u32*)0x8087a50, 0x4);
		
		//*((u16*)0x0803e88e) = 0x2002;//"mov r0, #2"
		//svcFlushProcessDataCache((u32*)0x0803e88c, 0x4);//Patch the pxipm opentitle code checking exheader_systeminfoflags.flag bit1: instead of loading the flag from exheader, the value for that is always set to value 2 with this.

		//arm9_pxipmcmd1_getexhdr_writepatch(0x0803ea5c);

		//axiwram[0x1a008>>2] = 0xE1200070;//Patch the first word in the ARM11 kernelpanic function, with: "bkpt #0"
		//axiwram[0x67dc>>2] = 0xE1200070;//Same as above except for kernelpanic_assert.
	}
	else if(RUNNINGFWVER==0x2E)
	{
		//*((u32*)0x8086560) = (u32)pxidev_cmdhandler_cmd0;
		//svcFlushProcessDataCache((u32*)0x8086560, 0x4);

		//*((u16*)0x0803e602) = 0x2002;
		//svcFlushProcessDataCache((u32*)0x0803e600, 0x4);//pxipm opentitle sd-flag check patch

		//arm9_pxipmcmd1_getexhdr_writepatch(0x0803e7d0);

		*((u16*)0x0805ed34) |= 1;
		svcFlushProcessDataCache((u32*)0x0805ed34, 0x4);//Patch the code which reads the arm9 access-control mount flags, so that all of these archives are accessible.

		//arm9general_debughook_writepatch(0x0802ea90);//Hook the gamecard v6.0 savegame keyY init code for debug.
		//arm9general_debughook_writepatch(0x0807b49c);

		//axiwram[0x1af2c>>2] = 0xE1200070;//ARM11 kernelpanic function patch. Note that the "kernelpanic_assert" function doesn't exist here.

		//axiwram[0x1b6f8>>2] = ~0;

		/*ptr = mmutable_convert_vaddr2physaddr(get_kprocessptr(0x707367, 0, 1), 0x10a474);//Patch the GSP module gxcmd4 code so that it uses the input addresses as the physical addresses, when the addresses are outside of the VRAM/LINEAR-mem range.
		ptr[0x8>>2] = 0xe1a03001;//"mov r3, r1"
		ptr[0x2C>>2] = 0xe1a01002;//"mov r1, r2"*/
	}
	else if(RUNNINGFWVER==0x30)
	{
		//*((u32*)0x8086564) = (u32)pxidev_cmdhandler_cmd0;
		//svcFlushProcessDataCache((u32*)0x8086564, 0x4);

		//arm9_pxipmcmd1_getexhdr_writepatch(0x0803e7d0);

		*((u16*)0x0805ed38) |= 1;
		svcFlushProcessDataCache((u32*)0x0805ed38, 0x4);//Patch the code which reads the arm9 access-control mount flags, so that all of these archives are accessible.

		//axiwram[0x1af28>>2] = 0xE1200070;//ARM11 kernelpanic function patch.
	}
	else if(RUNNINGFWVER==0x37)
	{
		*((u16*)0x0803e676) = 0x2002;
		svcFlushProcessDataCache((u32*)0x0803e674, 0x4);//pxipm opentitle sd-flag check patch
	}

	patch_pxidev_cmdhandler_cmd0((u32*)0x08028000, 0x080ff000-0x08028000);
	#ifdef ENABLE_GETEXHDRHOOK
	arm9_pxipmcmd1_getexhdr_writepatch_autolocate((u32*)0x08028000, 0x080ff000-0x08028000);
	#endif

	writepatch_arm11kernel_kernelpanicbkpt((u32*)0x1FF80000, 0x80000);

	//dumpmem((u32*)0x20000000, 0x100000 - 0x28000);

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

			//memset((u32*)0x18000000, pos | (pos<<8) | (pos<<16) | (pos<<24), 0x00600000);
			////memset(&framebuf_addr[(0x46500)>>2], 0x88888888, 0x46500);

			//sdcryptdata2();

			//mountcontent_nandsd_writehookstub();

			#ifdef ENABLE_ARM11KERNEL_DEBUG
			if(FIRMLAUNCH_RUNNINGTYPE==0)write_arm11debug_patch();
			#endif
			debuginitialized = 1;

			//if(RUNNINGFWVER==0x2E)axiwram[0x14ab0>>2] = 0xE1200070;//Patch the start of the svc7c handler(at the push instruction) with: "bkpt #0".

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

			/*if(RUNNINGFWVER==0x1F)
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

	//dumpmem(0x20000000, 0x1000);
	//dumpmem(0x1FF80000, 0x80000);
	//dumpmem(0x20000000, 0x08000000);
	//dumpmem(0x1FFF4000, 0x1000);

	//dumpmem((u32*)0x08000000, 0x100000);

	dump_fcramaxiwram();

	svcExitThread();

	while(1);
}

int main(void)
{
	//u32 *axiwram = (u32*)0x1FF80000;
	//u32 *framebuf;
	u32 pos=0;
	u32 threadhandle = 0;
	//u32 ret;
	u32 *ptr;

	//launchcode_kernelmode(changempu_memregions);
	//dumpmem(0x20000000, 0x08000000);
	//dumpmem(0x1FF80000, 0x80000);

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

	//loadrun_arm9code("/boot.bin", (u32*)0x809C000);

	//dump_nandfile("/data/e34f9af95c767280db87a62880d0426d/sysdata/00010034/00000000");
	//dump_nandfile("/dbs/title.db");

	//pos = 0x10000000 - 0xFFFF0000;
	//dumpmem(&pos, 4);

	//*((u32*)(0x23ba5000+0x2351b4)) = 0xffffffff;
	//memset((u32*)(0x26AA8000+0x3100), 0x20202020, 0x5000);

	//dump_nandimage();

	//dumpmem(0x20000000, 0x08000000);
	//dumpmem((u32*)0x08000000, 0x100000);
	//dumpmem((u32*)0x01ff8000, 0x8000);

	//*((u32*)0x20000000) = getsp();
	//dumpmem((u32*)0x20000000, 4);

	/**((u32*)0x20000000) = getsp();
	memcpy(((u32*)0x20000004), (u32*)(0x08028000+4), 0xD8000-4);
	dumpmem(((u32*)0x20000000), 0xD8000);*/

	//dump_nandfile("/rw/sys/native.log");
	//dump_nandfile("/rw/sys/lgy.log");

	//while(1)
	//{
		if((*((vu16*)0x10146000) & 1) == 0 && FIRMLAUNCH_RUNNINGTYPE==0)memset(framebuf_addr, pos | (pos<<8) | (pos<<16) | (pos<<24), 0x46500*10);
		//memset((u32*)0x18000000, pos | (pos<<8) | (pos<<16) | (pos<<24), 0x00600000);
		////memset(&framebuf_addr[(0x46500)>>2], 0x88888888, 0x46500);

		//pos+=0x20;

	//	if((*((vu16*)0x10146000) & 1) == 0)break;
	//}

	//dumpmem((u32*)0x20000000, 8);

	//dsiware_test();
	//am9_stuff();

	//memset(framebuf_addr, 0x80808080, 0x46500*10);
	//patch_mountcontent();

	//axiwram[0xa494>>2] = ~0;

	//debug_dbs();

	//dump_pxirecv();
	//send_pxireply();
	//launchcode_kernelmode(send_pxireply);

	//memcpy((u32*)0x20000000, (u32*)0x08000000, 0x100000);
	//dumpmem((u32*)0x20000000, 0x100000);

	//dump_movablesed();

	//patch_nandredir(0x8078c6e, 0x8078c2e);//FW1F
	//patch_nandredir(0x807846e, 0x807842e);//FW2E

	//dump_nandfile("/rw/sys/native.log");

	//load_gxcmd1_buffers();

	//read_gamecard();

	//genmac();

	//rsa_test();

	//sdcryptdata();
	//sdcryptdata2();

	//while(1);

	//launchcode_kernelmode(get_kernelmode_sp);
	/**((u32*)(0x08028008+0)) = getsp();
	*((u32*)(0x08028008+4)) = getcpsr();
	//svcFlushProcessDataCache((u32*)0x08028000, 0x10);
	memcpy((u32*)0x20000000, (u32*)0x08028000, 0x100000 - 0x28000);
	dumpmem((u32*)0x20000000, 0x100000 - 0x28000);*/

	//dumpmem((u32*)0x08000000, 0x100000);
	//dumpmem((u32*)0x08028000, 0x100000 - 0x28000);

	//changempu_memregions();
	//*((u32*)0x1FF827CC) = 0xffffffff;

	/*if(framebuf_addr)
	{
		framebuf = framebuf_addr;
		memset(framebuf_addr, 0xffffffff, 0x46500);
		memset(&framebuf_addr[(0x46500+0x10)>>2], 0xffffffff, 0x46500);
		load_tga(maingfx0tga_filepath, 0x1c, (u8*)framebuf, 0);//Main screen frambuffers for left 3D image
		memcpy(&framebuf[(0x46500+0x10)>>2], (u8*)framebuf, 0x46500);
		framebuf = &framebuf[((0x46500*2) + (0x10*2))>>2];

		load_tga(subgfxtga_filepath, 0x18, (u8*)framebuf, 0);//Sub screen framebuffers
		memcpy(&framebuf[(0x38400+0x10)>>2], (u8*)framebuf, 0x38400);
		framebuf = &framebuf[((0x38400*2) + (0x10*2))>>2];

		load_tga(maingfx1tga_filepath, 0x1c, (u8*)framebuf, 0);//Main screen frambuffers for right 3D image
		memcpy(&framebuf[(0x46500+0x10)>>2], framebuf, 0x46500);
		framebuf = &framebuf[((0x46500*2) + (0x10*2))>>2];
	}*/

	//loadfile(0x20703000, 0x20000, input_filepath, 0x24);

	//while(*((u32*)0x20000000) != 0x58584148);
	//while(*((vu16*)0x10146000) & 0x400);//button X
	//dumpmem(0x20000000, 0x4+0x200);
	//dumpmem(0x20000000, 0x1000000);
	//dumpmem(0x1FF80000, 0x80000);
	//dumpmem(0x1FF00000, 0x80000);
	//dumpmem(0x18000000, 0x600000);
	//dumpmem(0x01FF8000, 0x8000);

	if(FIRMLAUNCH_RUNNINGTYPE==0)
	{
		if((*((vu16*)0x10146000) & 0x100) == 0)//button R
		{
			loadrun_file("/x01ffb800.bin", (u32*)0x01ffb800, 0);
			launch_firm();
			//load_arm11code(NULL, 0, 0x707041727443LL);

			writearm11_firmlaunch_usrpatch();
			svcSleepThread(10000000000LL);
		}

		if((*((vu16*)0x10146000) & 0x200) == 0)//button L
		{
			load_arm11code(NULL, 0, 0x707041727443LL);
		}

		if((*((vu16*)0x10146000) & 1) == 0)
		{
			memset(framebuf_addr, 0x40404040, 0x46500);
			memset(&framebuf_addr[(0x46500)>>2], 0x40404040, 0x46500);
		}

		//while(*((vu16*)0x10146000) & 2);

		svcCreateThread(&threadhandle, thread_entry, 0, &thread_stack[THREAD_STACKSIZE>>2], 0x3f, ~1);
	}
	else
	{
		if(FIRMLAUNCH_RUNNINGTYPE==2)
		{
			//dumpmem(0x18600000-0x20000, 0x20000);

			#ifdef ENABLENANDREDIR
			#ifdef ENABLE_LOADA9_x01FFB800
			loadrun_file("/x01ffb800.bin", (u32*)0x01ffb800, 0);
			#endif
			#endif

			launch_firm();

			//Change the configmem UPDATEFLAG value to 1, which then causes NS module to do a FIRM launch, once NS gets loaded.
			ptr = NULL;
			while(ptr == NULL)ptr = (u32*)mmutable_convert_vaddr2physaddr(get_kprocessptr(0x697870, 0, 1), 0x1FF80004);
			*ptr = 1;
		}
		else if((*((vu16*)0x10146000) & 0x100))//button R not pressed
		{
			#ifdef ENABLE_ARM11KERNEL_DEBUG
			write_arm11debug_patch();
			#endif
			svcCreateThread(&threadhandle, thread_entry, 0, &thread_stack[THREAD_STACKSIZE>>2], 0x3f, ~1);
		}
	}

	if((*((vu16*)0x10146000) & 0x40) == 0)ARM11CODELOAD_PROCNAME = 0x726564697073LL;//0x726564697073LL = "spider". Change the the code-load procname to this, when the Up button is pressed.

	//dumpmem(0x20000000, 0x08000000);

	//memset((u32*)0x18300000, 0xffffffff, 0x46500);

	/*while(1)
	{
		if((*((vu16*)0x10146000) & 0x10) == 0)
		{*/
			/*for(pos=0; pos<3; pos+=3)
			{
				((u16*)0x18464ef8)[0+pos] = (((u16*)0x18464ef8)[0+pos] + 1) & 0xef;//0x18464ef8
				((u16*)0x18464ef8)[1+pos] = (((u16*)0x18464ef8)[1+pos] + 1) & 0xef;
				((u16*)0x18464ef8)[2+pos] = (((u16*)0x18464ef8)[2+pos] + 1) & 0xef;
			}*/
			//memset((u32*)0x18464ef8, 0x3f800000, 0x8);//0x3f800000
			//memset((u32*)0x182447C0+0x100000, 0x3f800000, 0x300000-0x100000);
			//memset((u32*)0x185447C0, 0x3f800000, 0x18600000-0x185447C0);
		//}
	//}
	//memset((u32*)0x1846a66c, 0xffffffff, 0x400);//0x3f800000
	//memset((u32*)0x1845dd00, 0x0, 0x18464b18-0x1845dd00);
	//memset((u32*)0x1846d400, 0x3f800000, 0x1000);
	//memset((u32*)0x18464b18, 0, 0x1846a66c-0x18464b18);
	//memset((u32*)0x185447C0, 0, 0x600000 - 0x5447C0);
	//memset((u32*)0x18061d58, 0x3f800000, 0x100);
	//memcpy((u32*)0x18061d58, vertices, 24*4);
	//for(pos=0; pos<36; pos++)vert_elements[pos] += 2;
	//memcpy((u16*)0x18469bc0, vert_elements, 36*2);

	/*for (pos = 1; pos < 6; pos++)memcpy(&cube_texcoords[pos*4*2], &cube_texcoords[0], 2*4*sizeof(float));
	memcpy((float*)0x18061d58, cube_vertices, 6*4*3*sizeof(float));
	memcpy((float*)0x18068a90, cube_texcoords, 2*4*6*sizeof(float));
	memcpy((u16*)0x18469bc0, cube_elements, 36*2);*/

	//dump_fcramvram();

	//pos = load_tga(texturetga_filepath, 0x1a, (u8*)0x20703000, 1 | 8);//(0x1846F480)0x208AFF80
	//loadfile((u8*)(0x1846F480-0x44), 0x844, texturektx_filepath, 0x1a);
	//loadfile((u8*)(0x1846F480-0x10), 0x810, texturepkm_filepath, 0x1a);
	//memset((u8*)0x1846F480, 0xffffffff, 128*32*4);
	//dumpmem(&pos, 4);

	//while(*((vu16*)0x10146000) & 0x400);
	//dumpmem((u32*)0x20313890, 0x46500);

	/*loadfile(((u32*)(0x20313890)), 0x46500, maingfx0bin_filepath, 0x1c);
	loadfile(((u32*)(0x20359da0)), 0x46500, maingfx0bin_filepath, 0x1c);
	loadfile(((u32*)(0x20410ad0)), 0x46500, maingfx1bin_filepath, 0x1c);
	loadfile(((u32*)(0x20456fe0)), 0x46500, maingfx1bin_filepath, 0x1c);

	loadfile(((u32*)(0x203A02b0)), 0x38400, subgfxbin_filepath, 0x18);
	loadfile(((u32*)(0x203A02b0+0x38400+0x10)), 0x38400, subgfx_filepath, 0x18);*/

	//launch_firm();

	//pxirecv();

	//while(*((vu16*)0x10146000) & 1);
	//dumpmem(0x20000000, 0x1000);

	//dumpmem(0x20000000, 0x4);
	//dumpmem(0x20000000, 0x1000);

	//debug_dbs();
	//debug_memalloc();

	//pxirecv();
	//dumpmem(0x20703000+0x20000, 0x20000);
	//dumpmem(0x20000000, 0x08000000);
	//dump_arm11debuginfo();
	//memset(framebuf_addr, 0xffffffff, 0x46500);
	//memset(&framebuf_addr[(0x46500+0x10)>>2], 0xffffffff, 0x46500);

	//while(1);

	return 0;
}

