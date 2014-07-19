#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <nds/ndstypes.h>

#include "aes.h"

#include "ctrclient.h"

/*#define REG_AESCNT *((vu32*)0x10009000)
#define REG_AESWRFIFO *((vu32*)0x10009008)
#define REG_AESRDFIFO *((vu32*)0x1000900C)
#define REG_AESKEYSEL *((vu8*)0x10009010)
#define REG_AESKEYCNT *((vu8*)0x10009011)
#define REG_AESCTR ((vu32*)0x10009020)
#define REG_AESKEYFIFO *((vu32*)0x10009100)
#define REG_AESKEYXFIFO *((vu32*)0x10009104)
#define REG_AESKEYYFIFO *((vu32*)0x10009108)*/

#define AES_CHUNKSIZE 0x10000//0x10000//0x10000

u32 dumpmem(u32 *addr, u32 size);
u32 loadfile(u32 *addr, u32 size, u16 *path, u32 pathsize);
u32 openfile(u32 *archiveobj, u32 lowpathtype, void* path, u32 pathsize, u32 openflags, u32 **fileobj);
u32 fileread(u32 *fileobj, u32 *buf, u32 size, u32 filepos);
u32 filewrite(u32 *fileobj, u32 *buf, u32 size, u32 filepos);
u32 getfilesize(u32 *fileobj);
u32 archive_readsectors(u32 *archiveobj, u32 *buf, u32 sectorcount, u32 mediaoffset);
u32 pxifs_openarchive(u32 **archiveobj, u32 archiveid, u32 *lowpath);//lowpath is a ptr to the following structure: +0 = lowpathtype, +4 = dataptr, +8 = datasize

void launchcode_kernelmode(void*);
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

extern int arm9_fopen(u32 *handle, u8 *path, u32 openflags);
extern int arm9_fclose(u32 *handle);
extern int arm9_fwrite(u32 *handle, u32 *bytes_written, u32 *buf, u32 size, u32 flushflag);
extern int arm9_fread(u32 *handle, u32 *bytes_read, u32 *buf, u32 size);
extern int arm9_GetFSize(u32 *handleptr, u64 *size);

u32 *patch_mmutables(u64 procname, u32 patch_permissions, u32 num);
u32 svcSignalEvent();
void svcFlushProcessDataCache(u32*, u32);
u64 svcGetSystemTick();
u32 svcCreateThread(u32 *threadhandle, void* entrypoint, u32 arg, u32 *stacktop, s32 threadpriority, s32 processorid);
void svcExitThread();
void svcSleepThread(s64 nanoseconds);

void aesengine_waitdone();

void AES_SelectKey(unsigned int keyslot);
void AES_SetKey(unsigned int keyslot, unsigned int order, unsigned int* keydata);
void AES_SetKeyY(unsigned int keyslot, unsigned int order, unsigned int* keydata);
void AES_SetKeyX(unsigned int keyslot, unsigned int order, unsigned int* keydata);
void AES_SetCounter(unsigned int order, unsigned int* counter);
void AES_CtrCrypt(unsigned int* input, unsigned int* output, unsigned int* counter, unsigned int blockcount);
void AES_CbcDecrypt(unsigned int* input, unsigned int* output, unsigned int* iv, unsigned int blockcount);
void AES_CbcEncrypt(unsigned int* input, unsigned int* output, unsigned int* iv, unsigned int blockcount);
void AES_Wait();

u32 *get_kprocessptr(u64 procname, u32 num, u32 get_mmutableptr);
u8 *mmutable_convert_vaddr2physaddr(u32 *mmutable, u32 vaddr);

extern u32 *pxifs_state;
extern u32 *sdarchive_obj;
extern u32 *nandarchive_obj;
extern u16 input_filepath[];
extern u16 dump_filepath[];

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

/*u32 inbuf[0x20>>2];
u32 metadata[0x24>>2];
u32 calcmac[4];*/
u32 aesiv[4];

extern u32 aes_keystate0, aes_keystate1, aes_keystate2, aes_keystate3;
extern u64 aes_time0, aes_time1;

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

typedef struct {
	u8 ID_size;
	u8 palettetype;//0=none, 1=palette used
	u8 colortype;//2=rgb, etc

	u16 palette_firstent;
	u16 palette_totalcolors;
	u8 palette_colorbitsize;

	u16 xorigin;
	u16 yorigin;
	u16 width;
	u16 height;
	u8 bpp;
	u8 descriptor;//bit4 set=horizontal flip, bit5 set=vertical flip
} PACKED tga_header;

#ifdef ENABLEAES
void aes_mutexenter()
{
	void (*funcptr)() = (void*)0x805e4c1;//FW1F

	if(RUNNINGFWVER==0x2E)funcptr = (void*)0x805f7e5;
	if(RUNNINGFWVER==0x30)funcptr = (void*)0x805f7e9;
	if(RUNNINGFWVER==0x37)funcptr = (void*)0x805f9e9;

	funcptr();
}

void aes_mutexleave()
{
	void (*funcptr)() = (void*)0x805e4d9;//FW1F

	if(RUNNINGFWVER==0x2E)funcptr = (void*)0x805f7fd;
	if(RUNNINGFWVER==0x30)funcptr = (void*)0x805f801;
	if(RUNNINGFWVER==0x37)funcptr = (void*)0x805fa01;

	funcptr();
}

void aes_set_ctr(u32 *ctr)
{
	/*u32 i;

	REG_AESCNT |= (5<<23);

	for(i=0; i<4; i++)REG_AESCTR[i] = ctr[3-i];*/

	memcpy(aesiv, ctr, 16);
	AES_SetCounter(AES_ORDER_DEFAULT, (unsigned int*)aesiv);
}

void aes_set_iv(u32 *iv)
{
	aes_set_ctr(iv);
}

void aes_select_key(u32 keyslot)
{
	//REG_AESKEYSEL = keyslot;
	//REG_AESCNT |= 1<<26;

	AES_SelectKey(keyslot);
}

/*void aes_set_keydata(u32 keyslot, u32 *key, u32 keytype)
{
	u32 *ptr = NULL;
	u32 i;

	if(keyslot<4)return;

	if(keyslot<4)
	{
		REG_AESCNT &= ~(5<<23);
	}
	else
	{
		REG_AESCNT |= (5<<23);
	}

	REG_AESKEYCNT = (REG_AESKEYCNT & ~0x3f) | keyslot | 0x80;

	if(keytype>2)return;
	if(keytype==0)ptr = REG_AESKEYFIFO;
	if(keytype==1)ptr = REG_AESKEYXFIFO;
	if(keytype==2)ptr = REG_AESKEYYFIFO;

	for(i=0; i<4; i++)*ptr = key[i];
}*/

void aes_set_ykey(u32 keyslot, u32 *key)
{
	/*u32 i;
	if(keyslot<4)return;

	REG_AESCNT |= (5<<23);

	REG_AESKEYCNT = (REG_AESKEYCNT & ~0x3f) | keyslot | 0x80;
	for(i=0; i<4; i++)REG_AESKEYYFIFO = key[i];*/

	//aes_set_keydata(keyslot, key, 2);

	AES_SetKeyY(keyslot, AES_ORDER_DEFAULT, (unsigned int*)key);
}

void aes_set_xkey(u32 keyslot, u32 *key)
{
	//aes_set_keydata(keyslot, key, 1);

	AES_SetKeyX(keyslot, AES_ORDER_DEFAULT, (unsigned int*)key);
}

void aes_set_key(u32 keyslot, u32 *key)
{
	//aes_set_keydata(keyslot, key, 0);

	AES_SetKey(keyslot, AES_ORDER_DEFAULT, (unsigned int*)key);
}

/*void aesengine_initoperation(u32 mode, u32 size)
{
	void (*funcptr)(u32, u32, u32, u32, u32, u32, u32, u32) = (void*)0x80ff289;
	funcptr(mode, 5, 5, 0, size>>4, 0, 0, 1);
}*/

/*void aesengine_cryptdata_wrap(u32 *output, u32 *input, u32 size)
{
	u32 pos, i;
	u32 chunkwords;
	//void (*funcptr)(u32*, u32*, u32) = (void*)0x80ff319;
	//funcptr(output, input, size);

	pos=0;

	while(size>0)
	{
		chunkwords = size>>2;
		if(chunkwords>0x10)chunkwords = 0x10;

		while((REG_AESCNT & 0x1f) != 0);

		for(i=0; i<chunkwords; i++)REG_AESWRFIFO = input[pos+i];

		while(((REG_AESCNT & 0x3e0) >> 5) != chunkwords);

		for(i=0; i<chunkwords; i++)output[pos+i] = REG_AESRDFIFO;

		pos+= chunkwords;
		size-= chunkwords<<2;
	}

	//aesengine_waitdone();
}*/

void aesengine_waitdone()
{
	//while(REG_AESCNT>>31);
	AES_Wait();
}

/*void aesengine_flushfifo_something()
{
	REG_AESCNT |= 0xc00;
}*/

/*void aes_crypt(u32 mode, u32 *buf, u32 size)
{
	//aesengine_flushfifo_something();
	aesengine_initoperation(mode, size);
	aesengine_cryptdata_wrap(buf, buf, size);

	//aesengine_flushfifo_something();
	aesengine_waitdone();
}*/

//static u32 *aes_buf, aes_bufsize, aes_type; /*aes_usemac, aes_keytype;*/

/*void kernelmode_aescrypt()
{
	if(aes_type==0)AES_CtrCrypt((unsigned int*)aes_buf, (unsigned int*)aes_buf, (unsigned int*)aesiv, aes_bufsize>>4);
	if(aes_type==1)AES_CbcDecrypt((unsigned int*)aes_buf, (unsigned int*)aes_buf, (unsigned int*)aesiv, aes_bufsize>>4);
	if(aes_type==2)AES_CbcEncrypt((unsigned int*)aes_buf, (unsigned int*)aes_buf, (unsigned int*)aesiv, aes_bufsize>>4);
}*/

void aes_ctr_crypt(u32 *buf, u32 size)
{
	//aes_crypt(2, buf, size);
	AES_CtrCrypt((unsigned int*)buf, (unsigned int*)buf, (unsigned int*)aesiv, size>>4);
	/*aes_type = 0;
	aes_buf = buf;
	aes_bufsize = size;
	//kernelmode_aescrypt();
	launchcode_kernelmode(kernelmode_aescrypt);*/
}

void aes_cbc_decrypt(u32 *buf, u32 size)
{
	//aes_crypt(4, buf, size);
	AES_CbcDecrypt((unsigned int*)buf, (unsigned int*)buf, (unsigned int*)aesiv, size>>4);
	/*aes_type = 1;
	aes_buf = buf;
	aes_bufsize = size;
	launchcode_kernelmode(kernelmode_aescrypt);*/
}

void aes_cbc_encrypt(u32 *buf, u32 size)
{
	//aes_crypt(5, buf, size);
	AES_CbcEncrypt((unsigned int*)buf, (unsigned int*)buf, (unsigned int*)aesiv, size>>4);
	/*aes_type = 2;
	aes_buf = buf;
	aes_bufsize = size;
	launchcode_kernelmode(kernelmode_aescrypt);*/
}

/*void aes_blockrotate(u8 *blk)
{
	int i, carry = 0;

	carry = blk[0] & 0x80;
	for(i=0; i<15; i++)
	{
		blk[i] = (blk[i]<<1) | (blk[i+1]>>7);
	}

	blk[15] <<= 1;
	if(carry)blk[15] ^= 0x87;
}*/

/*void aes_calcmac(unsigned char *outmac, unsigned char *buf, unsigned int size)
{
	u32 bufpos = 0, blki = 0;
	u8 cbcmac[16];
	u32 block[4];
	u32 iv[4];

	memset(cbcmac, 0, 16);
	memset(block, 0, 16);
	memset(iv, 0, 16);

	aes_set_iv(iv);
	aes_cbc_encrypt(block, 16);

	while(bufpos < size)
	{
		for(blki=0; blki<16; blki++)
		{
			cbcmac[blki] ^= buf[bufpos];

			bufpos++;
			if(blki<15 && bufpos==size)break;
		}

		if(bufpos < size)
		{
			aes_set_iv(iv);
			aes_cbc_encrypt((u32*)cbcmac, 16);
		}
	}

	aes_blockrotate((u8*)block);

	if(blki<16)
	{
		cbcmac[blki] ^= 0x80;
		aes_blockrotate((u8*)block);
	}

	for(blki=0; blki<16; blki++)((u8*)block)[blki] ^= cbcmac[blki];

	aes_set_iv(iv);
	aes_cbc_encrypt(block, 16);

	memcpy(outmac, block, 16);
}*/

/*void aes_metadatainit_setup()
{
	u32 pos;
	u32 key[4];
	
	memset(key, 0, 16);
	//memset(key, 0xffffffff, 16);
	key[0] = 1<<14;

	aesengine_waitdone();
	REG_AESCNT = 0;
	//aesengine_flushfifo_something();
	if(((metadata[0]>>16) & 0xff) == 0)aes_set_ykey(metadata[0] & 0xff, &metadata[1]);
	if(((metadata[0]>>16) & 0xff) == 2)
	{
		aes_set_xkey(metadata[0] & 0xff, &metadata[1]);
		aes_set_ykey(metadata[0] & 0xff, &metadata[1]);
	}
	if(((metadata[0]>>16) & 0xff) == 3)
	{
		aes_set_xkey(metadata[0] & 0xff, key);
		aes_set_ykey(metadata[0] & 0xff, &metadata[1]);
	}
	
	if(((metadata[0]>>16) & 0xff) == 4)aes_set_key(metadata[0] & 0xff, &metadata[1]);

	aes_select_key(metadata[0] & 0xff);
	
	if(!aes_usemac)aes_set_ctr(&metadata[5]);

	//aes_ctr_crypt(key, 0x10);
}

void aes_metadatainit(int mac)
{
	loadfile(metadata, 0x24, meta_filepath, 0x22);

	aes_usemac = mac;
	//aes_metadatainit_setup();
	launchcode_kernelmode(aes_metadatainit_setup);
}*/

/*void gensdmac()
{
	memset(framebuf_addr, 0xffffffff, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0xffffffff, 0x46500);

	aes_metadatainit(1);

	aes_calcmac((u8*)calcmac, (u8*)inbuf, 0x20);

	memset(framebuf_addr, 0x80808080, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0x80808080, 0x46500);
}*/

/*void aes_sdcryptdata(u32 *buf, u32 size)
{
	u32 type;

	type = (metadata[0] >> 8) & 0xff;

	if(type==0)
	{
		aes_ctr_crypt(buf, size);
	}
	else if(type==1)
	{
		aes_cbc_decrypt(buf, size);
	}
	else if(type==2)
	{
		aes_cbc_encrypt(buf, size);
	}
}*/

/*void genmac()
{
	memset(calcmac, 0, 0x10);

	loadfile(inbuf, 0x20, input_filepath, 0x24);

	//launchcode_kernelmode(gensdmac);
	gensdmac();

	dumpmem(calcmac, 0x10);
}

void sdcryptdata()
{
	u32 chunksize = AES_CHUNKSIZE;
	u32 *fileobj = NULL;
	u32 *out_fileobj = NULL;
	//u32 instackbuf[0x200>>2];
	//u32 outstackbuf[0x200>>2];
	u32 *inbuf = (u32*)0x20700000;
	u32 *outbuf = (u32*)0x20700000;

	if(openfile(sdarchive_obj, 4, input_filepath, 0x24, 1, &fileobj)!=0)return;

	input_filepos = 0;
	input_filesize = getfilesize(fileobj);
	//input_filesize = 0x2F5D0000;

	if(openfile(sdarchive_obj, 4, dump_filepath, 0x22, 7, &out_fileobj)!=0)return;

	aes_metadatainit(0);

	memset(framebuf_addr, 0xffffffff, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0xffffffff, 0x46500);

	//chunksize = 0x10;
	//chunksize = 0x1000;
	//chunksize = 0x200;
	chunksize = 0x100000;

	while(input_filepos < input_filesize)
	{
		if(input_filesize - input_filepos<chunksize)chunksize = input_filesize - input_filepos;

		memset(inbuf, 0, chunksize);
		memset(outbuf, 0, chunksize);
		//svcFlushProcessDataCache(inbuf, chunksize);
		//svcFlushProcessDataCache(outbuf, chunksize);

		if(fileread(fileobj, inbuf, chunksize, input_filepos)!=0)return;

		//memcpy(outbuf, inbuf, chunksize);
		aes_sdcryptdata(outbuf, chunksize);
		//svcFlushProcessDataCache(outbuf, chunksize);
		if(filewrite(out_fileobj, outbuf, chunksize, input_filepos)!=0)return;

		input_filepos+= chunksize;
	}

	aes_time0 = svcGetSystemTick();
	svcSleepThread(1000000000LL);
	aes_time1 = svcGetSystemTick() - aes_time0;

	if(filewrite(out_fileobj, &aes_keystate0, 4, input_filepos)!=0)return;
	if(filewrite(out_fileobj, &aes_keystate1, 4, input_filepos+4)!=0)return;
	if(filewrite(out_fileobj, &aes_keystate2, 4, input_filepos+8)!=0)return;
	if(filewrite(out_fileobj, &aes_keystate3, 4, input_filepos+12)!=0)return;
	if(filewrite(out_fileobj, &aes_time0, 8, input_filepos+16)!=0)return;
	if(filewrite(out_fileobj, &aes_time1, 8, input_filepos+24)!=0)return;

	memset(framebuf_addr, 0x80808080, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0x80808080, 0x46500);
}*/

/*void sdcryptdata2()
{
	u32 index = 0;
	u32 keyslot = 0x04;
	u32 chunksize = 0x10;
	u32 *fileobj = NULL;
	u32 *out_fileobj = NULL;
	//u32 instackbuf[0x200>>2];
	//u32 outstackbuf[0x200>>2];
	u32 *inbuf = (u32*)0x20700000;
	u32 *outbuf = (u32*)0x20700000;
	u32 key[4];
	u32 ctr[4];
	u32 cryptblock[4];

	//if(openfile(sdarchive_obj, 4, input_filepath, 0x24, 1, &fileobj)!=0)return;

	input_filepos = 0;
	//input_filesize = getfilesize(fileobj);
	input_filesize = 0x20;

	if(openfile(sdarchive_obj, 4, dump_filepath, 0x22, 7, &out_fileobj)!=0)return;

	//memset(framebuf_addr, 0xffffffff, 0x46500);
	//memset(&framebuf_addr[(0x46500+0x10)>>2], 0xffffffff, 0x46500);

	//chunksize = 0x10;
	//chunksize = 0x1000;
	//chunksize = 0x200;
	//chunksize = 0x100000;

	memset(key, 0, 16);
	memset(cryptblock, 0, 16);
	memset(ctr, 0, 16);

	while(keyslot < 0x40)
	{
		//if(input_filesize - input_filepos<chunksize)chunksize = input_filesize - input_filepos;

		memset(inbuf, 0, chunksize);
		memset(outbuf, 0, chunksize);
		//svcFlushProcessDataCache(inbuf, chunksize);
		//svcFlushProcessDataCache(outbuf, chunksize);

		//if(fileread(fileobj, inbuf, chunksize, input_filepos)!=0)return;

		//memcpy(outbuf, inbuf, chunksize);

		if(index & 1)
		{
			//aes_set_key(keyslot, key);
			aes_set_ykey(keyslot, key);
		}
		aes_select_key(keyslot);
		aes_set_ctr(ctr);
		aes_ctr_crypt(outbuf, chunksize);
		//svcFlushProcessDataCache(outbuf, chunksize);
		if(filewrite(out_fileobj, outbuf, chunksize, input_filepos)!=0)return;

		if((index & 1) == 0)memcpy(cryptblock, outbuf, 16);

		input_filepos+= chunksize;
		index++;
		if((index & 1) == 0)keyslot++;
	}

	//memset(framebuf_addr, 0x80808080, 0x46500);
	//memset(&framebuf_addr[(0x46500+0x10)>>2], 0x80808080, 0x46500);
}*/

/*void sdcryptdata3()
{
	u32 pos, biti, keyindex;
	u32 index = 0;
	u32 keyslot = 0x04;
	u32 chunksize = 0x10;
	u32 *fileobj = NULL;
	u32 *out_fileobj = NULL;
	//u32 instackbuf[0x200>>2];
	//u32 outstackbuf[0x200>>2];
	u32 *inbuf = (u32*)0x20700000;
	u32 *outbuf = (u32*)0x20700000;
	u32 xkey[4];
	u32 ykey[4];
	u32 cryptblock[4];

	//if(openfile(sdarchive_obj, 4, input_filepath, 0x24, 1, &fileobj)!=0)return;

	input_filepos = 0;
	//input_filesize = getfilesize(fileobj);
	input_filesize = 0x20;

	if(openfile(sdarchive_obj, 4, dump_filepath, 0x22, 7, &out_fileobj)!=0)return;

	aes_metadatainit(0);

	memset(framebuf_addr, 0xffffffff, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0xffffffff, 0x46500);

	//chunksize = 0x10;
	//chunksize = 0x1000;
	//chunksize = 0x200;
	//chunksize = 0x100000;

	memset(xkey, 0, 16);
	memset(ykey, 0, 16);
	memset(cryptblock, 0, 16);

	keyslot = 0x31;

	//while(keyslot < 0x40)
	for(biti=0; biti<128; biti++)
	{
		memset(inbuf, 0, chunksize);
		memset(outbuf, 0, chunksize);

		for(index=0; index<2; index++)
		{
			memset(xkey, 0, 16);
			memset(ykey, 0, 16);

			if((index & 1) == 1)
			{
				((u8*)ykey)[biti >> 3] = 1 << (biti & 7);

				aes_set_xkey(keyslot, xkey);
				aes_set_ykey(keyslot, ykey);
				aes_select_key(keyslot);
				aes_set_ctr(&metadata[5]);
				memset(outbuf, 0, chunksize);
				aes_sdcryptdata(outbuf, chunksize);
			}
			else
			{
				((u8*)xkey)[biti >> 3] = 1 << (biti & 7);

				aes_set_xkey(keyslot, xkey);
				aes_set_ykey(keyslot, ykey);
				aes_select_key(keyslot);
				aes_set_ctr(&metadata[5]);
				aes_sdcryptdata(outbuf, chunksize);
				memcpy(cryptblock, outbuf, 16);
			}

			if(filewrite(out_fileobj, outbuf, chunksize, input_filepos)!=0)return;

			input_filepos+= chunksize;
		}
	}

	memset(framebuf_addr, 0x80808080, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0x80808080, 0x46500);
}*/

/*void sdcryptdata4()
{
	u32 index = 0;
	u32 keyslot = 0x04;
	u32 chunksize = 0x10;
	u32 *fileobj = NULL;
	u32 *out_fileobj = NULL;
	u32 pos;
	u32 *ptr;
	//u32 instackbuf[0x200>>2];
	//u32 outstackbuf[0x200>>2];
	u32 *inbuf = (u32*)0x20700000;
	u32 *outbuf = (u32*)0x20700000;
	u32 key[4];
	u32 ctr[4];
	u32 cryptblock[4];

	//if(openfile(sdarchive_obj, 4, input_filepath, 0x24, 1, &fileobj)!=0)return;

	input_filepos = 0;
	//input_filesize = getfilesize(fileobj);
	input_filesize = 0x20;

	if(openfile(sdarchive_obj, 4, dump_filepath, 0x22, 7, &out_fileobj)!=0)return;

	memset(framebuf_addr, 0xffffffff, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0xffffffff, 0x46500);

	//chunksize = 0x10;
	//chunksize = 0x1000;
	//chunksize = 0x200;
	//chunksize = 0x100000;

	memset(key, 0, 16);
	memset(cryptblock, 0, 16);
	memset(ctr, 0, 16);

	while(keyslot < 0x40)
	{
		//if(input_filesize - input_filepos<chunksize)chunksize = input_filesize - input_filepos;

		memset(inbuf, 0, chunksize);
		memset(outbuf, 0, chunksize);
		//svcFlushProcessDataCache(inbuf, chunksize);
		//svcFlushProcessDataCache(outbuf, chunksize);

		//if(fileread(fileobj, inbuf, chunksize, input_filepos)!=0)return;

		//memcpy(outbuf, inbuf, chunksize);

		aes_select_key(keyslot);
		aes_set_ctr(ctr);
		aes_ctr_crypt(outbuf, chunksize);
		//svcFlushProcessDataCache(outbuf, chunksize);

		//if(index & 1)
		//{
			//aes_set_key(keyslot, key);
		//	aes_set_ykey(keyslot, key);
		//}

		if((index & 1) == 1)
		{
			memcpy(cryptblock, outbuf, 16);

			memset(outbuf, 0, 16);
			ptr = (u32*)0x01ff8000;
			for(pos=0; pos<((0x8000-0x10)>>2); pos++)
			{
				aes_set_ykey(keyslot, &ptr[pos]);
				aes_select_key(keyslot);
				aes_set_ctr(ctr);
				aes_ctr_crypt(outbuf, chunksize);

				if(memcmp(outbuf, cryptblock, 16)==0)
				{
					memcpy(outbuf, &ptr[pos], 16);
					break;
				}
				else
				{
					memset(outbuf, 0, 16);
				}
			}
		}

		if(filewrite(out_fileobj, outbuf, chunksize, input_filepos)!=0)return;

		input_filepos+= chunksize;
		index++;
		if((index & 1) == 0)keyslot++;
	}

	memset(framebuf_addr, 0x80808080, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0x80808080, 0x46500);
}*/

/*void sdcryptdata5()
{
	u32 index = 0;
	u32 keyslot = 0x08;
	u32 chunksize = 0x10;
	u32 *fileobj = NULL;
	u32 *out_fileobj = NULL;
	u32 pos;
	u32 *ptr;
	//u32 instackbuf[0x200>>2];
	//u32 outstackbuf[0x200>>2];
	u32 *inbuf = (u32*)0x20700000;
	u32 *outbuf = (u32*)0x20700000;
	u32 key[4];
	u32 ctr[4];
	u32 cryptblock[4];

	//if(openfile(sdarchive_obj, 4, input_filepath, 0x24, 1, &fileobj)!=0)return;

	input_filepos = 0;
	//input_filesize = getfilesize(fileobj);
	input_filesize = 0x20;

	if(openfile(sdarchive_obj, 4, dump_filepath, 0x22, 7, &out_fileobj)!=0)return;

	//memset(framebuf_addr, 0xffffffff, 0x46500);
	//memset(&framebuf_addr[(0x46500+0x10)>>2], 0xffffffff, 0x46500);

	//chunksize = 0x10;
	//chunksize = 0x1000;
	//chunksize = 0x200;
	//chunksize = 0x100000;

	memset(key, 0, 16);
	memset(cryptblock, 0, 16);
	memset(ctr, 0, 16);

	aes_mutexenter();

	while(keyslot < 0x40)
	{
		//if(input_filesize - input_filepos<chunksize)chunksize = input_filesize - input_filepos;

		memset(inbuf, 0, chunksize);
		memset(outbuf, 0, chunksize);
		//svcFlushProcessDataCache(inbuf, chunksize);
		//svcFlushProcessDataCache(outbuf, chunksize);

		//if(fileread(fileobj, inbuf, chunksize, input_filepos)!=0)return;

		//memcpy(outbuf, inbuf, chunksize);

		if((index & 1) == 0)
		{
			//aes_set_key(keyslot, key);
			aes_set_ykey(keyslot, key);
		}

		aes_select_key(keyslot);
		aes_set_ctr(ctr);
		aes_ctr_crypt(outbuf, chunksize);
		//svcFlushProcessDataCache(outbuf, chunksize);

		if((index & 1) == 1)
		{
			memcpy(cryptblock, outbuf, 16);

			memset(outbuf, 0, 16);
			ptr = (u32*)0x01ff8000;
			for(pos=0; pos<((0x8000-0x10)>>2); pos++)
			{
				aes_set_xkey(keyslot, &ptr[pos]);
				aes_set_ykey(keyslot, key);
				aes_select_key(keyslot);
				aes_set_ctr(ctr);
				aes_ctr_crypt(outbuf, chunksize);

				if(memcmp(outbuf, cryptblock, 16)==0)
				{
					memcpy(outbuf, &ptr[pos], 16);
					break;
				}
				else
				{
					memset(outbuf, 0, 16);
				}
			}
		}

		if(filewrite(out_fileobj, outbuf, chunksize, input_filepos)!=0)return;

		input_filepos+= chunksize;
		index++;
		if((index & 1) == 0)keyslot++;
	}

	aes_mutexleave();

	//memset(framebuf_addr, 0x80808080, 0x46500);
	//memset(&framebuf_addr[(0x46500+0x10)>>2], 0x80808080, 0x46500);
}*/

/*
aes_keytype=0:
00 cf 9a 81 02 00 00 00 80 d3 9a 81 02 00 00 00 0x2819acf00 0x2819ad380 diff=0x480/1152
00 14 5b fc 02 00 00 00 80 18 5b fc 02 00 00 00 0x2fc5b1400 0x2fc5b1880 diff=0x480
80 a2 eb 21 03 00 00 00 00 a7 eb 21 03 00 00 00 0x321eba280 0x321eba700 diff=0x480
80 f6 ba 2b 03 00 00 00 00 fb ba 2b 03 00 00 00 0x32bbaf680 0x32bbafb00 diff=0x480
80 c6 4d 32 03 00 00 00 00 cb 4d 32 03 00 00 00 0x3324dc680 0x3324dcb00 diff=0x480

aes_keytype=1:
80 cf e9 87 05 00 00 00 80 d4 e9 87 05 00 00 00 0x587e9cf80 0x587e9d480 diff=0x500/1280
00 7f f3 8e 05 00 00 00 00 84 f3 8e 05 00 00 00 0x58ef37f00 0x58ef38400 diff=0x500
00 1c 88 95 05 00 00 00 00 21 88 95 05 00 00 00 0x595881c00 0x595882100 diff=0x500
00 e5 39 9b 05 00 00 00 00 ea 39 9b 05 00 00 00 0x59b39e500 0x59b39ea00 diff=0x500
00 f0 9e a2 05 00 00 00 00 f5 9e a2 05 00 00 00 0x5a29ef000 0x5a29ef500 diff=0x500

aes_keytype=2:
80 b0 88 76 06 00 00 00 80 b5 88 76 06 00 00 00 0x67688b080 0x67688b580 diff=0x500
80 b9 e4 7d 06 00 00 00 80 be e4 7d 06 00 00 00 0x67de4b980 0x67de4be80 diff=0x500
80 52 9b 83 06 00 00 00 80 57 9b 83 06 00 00 00 0x6839b5280 0x6839b5780 diff=0x500
80 d4 2a 8a 06 00 00 00 80 d9 2a 8a 06 00 00 00 0x68a2ad480 0x68a2ad980 diff=0x500
00 79 e5 8f 06 00 00 00 00 7e e5 8f 06 00 00 00 0x68fe57900 0x68fe57e00 diff=0x500

00 cf 9a 81 02 00 00 00 80 d3 9a 81 02 00 00 00 00 14 5b fc 02 00 00 00 80 18 5b fc 02 00 00 00 80 a2 eb 21 03 00 00 00 00 a7 eb 21 03 00 00 00 80 f6 ba 2b 03 00 00 00 00 fb ba 2b 03 00 00 00 80 c6 4d 32 03 00 00 00 00 cb 4d 32 03 00 00 00 80 cf e9 87 05 00 00 00 80 d4 e9 87 05 00 00 00 00 7f f3 8e 05 00 00 00 00 84 f3 8e 05 00 00 00 00 1c 88 95 05 00 00 00 00 21 88 95 05 00 00 00 00 e5 39 9b 05 00 00 00 00 ea 39 9b 05 00 00 00 00 f0 9e a2 05 00 00 00 00 f5 9e a2 05 00 00 00 80 b0 88 76 06 00 00 00 80 b5 88 76 06 00 00 00 80 b9 e4 7d 06 00 00 00 80 be e4 7d 06 00 00 00 80 52 9b 83 06 00 00 00 80 57 9b 83 06 00 00 00 80 d4 2a 8a 06 00 00 00 80 d9 2a 8a 06 00 00 00 00 79 e5 8f 06 00 00 00 00 7e e5 8f 06 00 00 00
*/

/*void aestiming_test_kernelmode()//This is executed in kernel-mode with IRQs disabled.
{
	u64 (*a9kern_svcGetSystemTick)() = (void*)0x08008b68;//FW2E addr
	u32 cryptbuf[4];
	u32 keydata[4];
	u32 ctr[4];

	memset(cryptbuf, 0, 16);
	memset(keydata, 0, 16);
	memset(ctr, 0, 16);

	memcpy(keydata, aesiv, 16);

	if((aes_keytype & 0xf0) == 0)aes_time0 = a9kern_svcGetSystemTick();

	if((aes_keytype & 0xf)==1)
	{
		aes_set_key(0x3f, keydata);
	}
	else if((aes_keytype & 0xf)==2)
	{
		aes_set_ykey(0x3f, keydata);
	}

	if((aes_keytype & 0xf0) != 0)aes_time0 = a9kern_svcGetSystemTick();

	aes_select_key(0x3f);	
	aes_set_ctr(ctr);

	aes_ctr_crypt(cryptbuf, 0x10);

	aes_time1 = a9kern_svcGetSystemTick();
}

void aestiming_test(u32 *buf)
{
	aes_keytype = buf[0];

	memcpy(aesiv, &buf[1], 16);

	launchcode_kernelmode(aestiming_test_kernelmode);

	memcpy(&buf[0], &aes_time0, 8);
	memcpy(&buf[2], &aes_time1, 8);
}*/

/*void sdcryptdata7()
{
	u32 index = 0;
	u32 keyslot = 0x31;
	u32 chunksize = 0x10;
	u32 *fileobj = NULL;
	u32 *out_fileobj = NULL;
	u32 pos;
	u32 *ptr;
	//u32 instackbuf[0x200>>2];
	//u32 outstackbuf[0x200>>2];
	u32 *inbuf = (u32*)0x20700000;
	u32 *outbuf = (u32*)0x20700000;
	u32 key[4];
	u32 ctr[4];
	u32 cryptblock[4];
	u8 keydata[16] = {0x74, 0x62, 0x55, 0x3F, 0x9E, 0x5A, 0x79, 0x04, 0xB8, 0x64, 0x7C, 0xCA, 0x73, 0x6D, 0xA1, 0xF5};
	u8 *key8 = (u8*)key;

	//if(openfile(sdarchive_obj, 4, input_filepath, 0x24, 1, &fileobj)!=0)return;

	input_filepos = 0;
	//input_filesize = getfilesize(fileobj);
	input_filesize = 0x20;

	if(openfile(sdarchive_obj, 4, dump_filepath, 0x22, 7, &out_fileobj)!=0)return;*/

	/*memset(framebuf_addr, 0xffffffff, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0xffffffff, 0x46500);*/

	//chunksize = 0x10;
	//chunksize = 0x1000;
	//chunksize = 0x200;
	//chunksize = 0x100000;

	/*memset(key, 0, 16);
	memset(cryptblock, 0, 16);
	memset(ctr, 0, 16);

	for(index=0; index<0x81; index++)
	{
		//if(input_filesize - input_filepos<chunksize)chunksize = input_filesize - input_filepos;

		memset(inbuf, 0, chunksize);
		memset(outbuf, 0, chunksize);
		//svcFlushProcessDataCache(inbuf, chunksize);
		//svcFlushProcessDataCache(outbuf, chunksize);

		//if(fileread(fileobj, inbuf, chunksize, input_filepos)!=0)return;

		//memcpy(outbuf, inbuf, chunksize);

		if(index)
		{
			memcpy(key, keydata, 16);

			pos = index-1;
			key8[pos >> 3] ^= 1 << (pos & 0x7);

			//aes_set_key(keyslot, key);
			aes_set_ykey(keyslot, key);
		}

		aes_select_key(keyslot);
		aes_set_ctr(ctr);
		aes_ctr_crypt(outbuf, chunksize);

		if(filewrite(out_fileobj, outbuf, chunksize, input_filepos)!=0)return;

		input_filepos+= chunksize;
	}*/

	/*memset(framebuf_addr, 0x80808080, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0x80808080, 0x46500);*/
//}
#endif

/*void dump_movablesed()
{
	void (*readmovablesed)(u32*, u32*) = (void*)0x8060c65;
	u32 movablesed[0x120>>2];

	memset(movablesed, 0, 0x120);
	readmovablesed(&pxifs_state[0x34a0>>2], movablesed);
	dumpmem(movablesed, 0x120);
}

u32 pxiam9_cmdgettitlecount(u32 *count, u8 mediatype)
{
	u32 *state;

	state = (u32*)*((u32*)0x8097980);
	state = &state[0x2888>>2];

	u32 (*funcptr)(u32*, u32*, u8) = (void*)0x8041c2d;
	return funcptr(state, count, mediatype);
}

u32 pxiam9_cmd3d(u32 *buf0, u32 buf0sz, u32 *buf1, u32 buf1sz, u32 *buf2, u32 buf2sz, u32 *buf3, u32 buf3sz, u32 flag)
{
	u32 *state;

	state = (u32*)*((u32*)0x8097980);
	state = &state[0x20>>2];

	u32 (*funcptr)(u32*, u32*, u32, u32*, u32, u32*, u32, u32*, u32, u32) = (void*)0x804562d;
	return funcptr(state, buf0, buf0sz, buf1, buf1sz, buf2, buf2sz, buf3, buf3sz, flag);
}

u32 pxiam9_cmd48(u8 *out, u8 mediatype)
{
	u32 *state;

	state = (u32*)*((u32*)0x8097980);

	u32 (*funcptr)(u32*, u8*, u8) = (void*)0x80481ed;
	return funcptr(state, out, mediatype);
}

u32 pxiam9_cmd4d(u64 titleid, u16 *path, u32 pathsize, u32 *outbuf, u32 outbufsize, u8 unk8)
{
	u32 *state;

	state = (u32*)*((u32*)0x8097980);
	state = &state[0x2a0c>>2];

	u32 (*funcptr)(u32*, u32, u64, u16*, u32, u32*, u32, u8) = (void*)0x8044599;
	return funcptr(state, 0, titleid, path, pathsize, outbuf, outbufsize, unk8);
}*/

/*u32 pxiam9_cmd2a(u8 mediatype, u32 *tidbuf, u32 totaltitles, u8 unk8)
{
	u32 *state;

	state = (u32*)*((u32*)0x8097980);

	u32 (*funcptr)(u32*, u8, u32*, u32, u8) = (void*)0x80477ad;
	return funcptr(state, mediatype, tidbuf, totaltitles, unk8);
}*/

/*void memctx_init(u32 *memctx, u32 *fcramadr, u32 size)
{
	u32 (*funcptr)(u32*, u32*, u32) = (void*)0x805a025;
	funcptr(memctx, fcramadr, size);
}

u32 dsiware_writefooter(u32 *hashbuf, u32 *memctx)
{
	u32 (*funcptr)(u32, u32, u32*, u32*, u8) = (void*)0x8043f7d;
	return funcptr(0, 0, hashbuf, memctx, 1);
}

void dsiware_test()
{
	u32 ret = 0;
	u32 pos, len;
	u32 *hashbuf = (u32*)(0x20703000-0x1000);
	u32 memctx[24>>2];

	char *path = (char*)"/footer_hashes.bin";//"sdmc:/dsiware.bin";
	u16 *filepath = (u16*)(0x20703000 - 0x100);

	memset(filepath, 0, 64*2);
	len = strlen(path);
	for(pos=0; pos<len; pos++)filepath[pos] = (u16)path[pos];

	if(loadfile(hashbuf, 0x1a0, filepath, (len+1) * 2)!=0)return;

	memset(((u32*)0x20703000), 0, 0x20004);

	*((u16*)0x804402e) = 0;
	// *((u16*)0x8044094) = 0;
	// *((u16*)0x804408e) = 0;
	svcFlushProcessDataCache((u32*)0x804402e, 2);
	//svcFlushProcessDataCache((u32*)0x8044094, 2);
	//svcFlushProcessDataCache((u32*)0x804408e, 2);

	memctx_init(memctx, (u32*)0x20703004, 0x20000);
	ret = dsiware_writefooter(hashbuf, memctx);

	//ret = pxiam9_cmd4d(0x000480044B513945, filepath, (len+1) * 2, ((u32*)0x20703004), 0x20000, 7);

	memcpy((u32*)0x20703004, (u32*)(getsp()-0x1000), 0x1000);

	*((u32*)0x20703000) = ret;
	dumpmem(((u32*)0x20703000), 0x20004);
}*/

/*void am9_stuff()
{
	u32 ret=0;
	u8 out;
	u64 *tidbuf = (u64*)0x20000000;//0x20703000;
	u32 *fcramptr = (u32*)tidbuf;*/

	//memset(((u32*)0x20703000), 0, 8);
	//tidbuf[0] = 0x0004001000021900LL;

	//memset(fcramptr, 0xffffffff, 0x2800 + 0x1c+8);

	/*loadfile(fcramptr, 0x2800 + 0x3c+8, input_filepath, 0x24);

	memset(framebuf_addr, 0x33333333, 0x46500);
	memset(&framebuf_addr[(0x46500)>>2], 0x33333333, 0x46500);*/

	//((u32*)0x809039c)[0] = arm9_stub[0];
	//((u32*)0x809039c)[1] = arm9_stub[1];
	//((u32*)0x808f4b4)[2] = arm9_stub[2];
	//svcFlushProcessDataCache((u32*)0x809039c, 8);

	//*tidbuf = 0x000400300000CE02LL;

	//fcramptr[2] = pxiam9_cmd2a(0, fcramptr, 1, 0);

	//while(*((vu16*)0x10146000) & 2);

	/*fcramptr[0] = pxiam9_cmd3d(&fcramptr[0], 0xa00, &fcramptr[(0xa00)>>2], 0xa00, &fcramptr[(0xa00*2)>>2], 0xa00, &fcramptr[(0xa00*3)>>2], 0xa00+0x3c+8, 0);

	fcramptr[1] = pxiam9_cmd48(&out, 1);

	memcpy(&fcramptr[2], (u32*)(0x08028000+8), 0xD8000-8);
	dumpmem(fcramptr, 0xD8000+8);*/
//}

/*void debug_dbs()
{
	u32 infobuf[2];

	((u32*)0x806ccc0)[0] = arm9dbs_stub[0];
	((u32*)0x806ccc0)[1] = arm9dbs_stub[1];
	*((u32*)0x808f108) = 0xe3a00000;//Patch the pxiam9_cmd3d() call with: "mov r0, #0"
	svcFlushProcessDataCache((u32*)0x806ccc0, 8);
	svcFlushProcessDataCache((u32*)0x808f108, 4);
	*((u32*)0x20703000) = 0;

	//while(*((vu16*)0x10146000) & 2);

	memset(infobuf, 0, 8);
	infobuf[0] = pxiam9_cmd48((u8*)&infobuf[1], 1);//pxiam9_cmdgettitlecount(&infobuf[1], 1);
	dumpmem(infobuf, 8);
	//dumpmem((u32*)0x20703004, *((u32*)0x20703000));

	memset(framebuf_addr, 0xffffffff, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0xffffffff, 0x46500);
}*/

/*void rsactx_init(u32 *rsactx)
{
	void (*funcptr)(u32*) = (void*)0x80607e5;
	funcptr(rsactx);
}

void rsapubkctx_init(u32 *rsapubkctx, u32 *rsactx)
{
	void (*funcptr)(u32*, u32*) = (void*)0x805f899;
	funcptr(rsapubkctx, rsactx);
}

void rsapubkctx_destroy(u32 *rsapubkctx)
{
	void (*funcptr)(u32*) = (void*)0x804e599;
	funcptr(rsapubkctx);
}

u32 rsapubkctx_cryptmsg(u32 *rsapubkctx, u32 *outmsg, u32 *insig, u32 sigsize)
{
	u32 *vtable = (u32*)rsapubkctx[0];

	u32 (*funcptr)(u32*, u32*, u32*, u32) = (void*)vtable[0];
	return funcptr(rsapubkctx, outmsg, insig, sigsize);
}

void rsa_test()
{
	u32 pos;
	u32 rsasize = 0x100;
	u32 *fcramptr = (u32*)0x20000000;
	u32 rsactx[0x208>>2];
	u32 rsapubkctx[0x44>>2];

	rsactx_init(rsactx);

	memset(rsactx, 0xffffffff, 0x100);//modulo
	rsactx[0x100>>2] = 1;//exponent
	rsactx[0x200>>2] = rsasize<<3;//RSA bitsize
	rsactx[0x204>>2] = 0;//0 = public exponent

	rsapubkctx_init(rsapubkctx, rsactx);
	rsapubkctx[0] = 0x080939e4;

	for(pos=0; pos<(rsasize>>2); pos++)fcramptr[pos+1] = pos | (pos<<8) | (pos<<16) | (pos<<24);

	fcramptr[0] = rsapubkctx_cryptmsg(rsapubkctx, &fcramptr[1 + (rsasize>>2)], &fcramptr[1], rsasize);

	rsapubkctx_destroy(rsapubkctx);

	dumpmem(fcramptr, 4 + rsasize*2);
}*/

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

u32 ctrcard_cmdc6(u32 *outbuf)
{
	u32 (*funcptr)(u32*, u32*);
	u32 ctx[1];

	ctx[0] = 0;	

	if(RUNNINGFWVER==0x1F)funcptr = (void*)0x08035635;
	if(RUNNINGFWVER==0x2E || RUNNINGFWVER==0x30)funcptr = (void*)0x08031cf1;
	if(RUNNINGFWVER==0x37)funcptr = (void*)0x08031d35;

	return funcptr(ctx, outbuf);
}

/*u32 gamecard_initarchiveobj()
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
}*/

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

/*u32 load_tga(u16 *lowpath, u32 lowpathsize, u8 *out_gfx, u32 outcolor)
{
	u32 x, y, pos;
	u32 *fileobj = NULL;
	u32 *tgabuf = (u32*)0x20200000;
	tga_header *tgahdr = (tga_header*)tgabuf;
	u8 *tmp_gfx = (u8*)((u32)tgabuf + sizeof(tga_header));
	u8 *in_pixel, *out_pixel;
	u16 *out_pix16;
	u32 outalpha=0, inalpha=0, alphaval=0;
	u32 in_pixsz, out_pixsz;
	u32 out_imagesz;
	u8 in_pix[4];

	if(outcolor & 8)
	{
		outcolor &= ~8;
		outalpha = 1;
		alphaval = 0xff;
	}

	if(openfile(sdarchive_obj, 4, lowpath, lowpathsize, 1, &fileobj)!=0)return 4;
	input_filesize = getfilesize(fileobj);
	fileread(fileobj, tgabuf, input_filesize, 0);

	//if(tgahdr->palettetype!=0 || tgahdr->colortype!=2)return 1;
	//if(outcolor==0 && (tgahdr->height>240 || (tgahdr->width!=320 && tgahdr->width!=400)))return 3;

	in_pixsz = tgahdr->bpp / 8;
	if(in_pixsz==32)inalpha = 1;

	out_pixsz = 2;
	if(outcolor==1)out_pixsz = 3+outalpha;

	out_imagesz = out_pixsz * tgahdr->width*tgahdr->height;

	if(tgahdr->palettetype==0)
	{
		if(outcolor==0)
		{
			for(y=0; y<tgahdr->height; y++)
			{
				for(x=0; x<tgahdr->width; x++)
				{
					in_pixel = &tmp_gfx[((y*tgahdr->width) + x) * in_pixsz];
					out_pixel = &out_gfx[((x * tgahdr->height) + (tgahdr->height-1 - y)) * 3];

					out_pixel[0] = in_pixel[0];//in B -> out B
					out_pixel[1] = in_pixel[1];//in G -> out G
					out_pixel[2] = in_pixel[2];//in R -> out R
				}
			}
		}
		else
		{
			for(x=0; x<tgahdr->width; x++)
			{
				for(y=0; y<tgahdr->height; y++)
				{
					in_pixel = &tmp_gfx[((y*tgahdr->width) + x) * in_pixsz];
					out_pixel = &out_gfx[((y*tgahdr->width) + x) * out_pixsz];
					out_pix16 = (u16*)out_pixel;

					in_pix[0] = in_pixel[2+inalpha];
					in_pix[1] = in_pixel[1+inalpha];
					in_pix[2] = in_pixel[0+inalpha];
					//if(outalpha)in_pix[3] = alphaval;
					//memcpy(in_pix, in_pixel, 3);

					if(in_pixsz==4)alphaval = in_pixel[3];

					if(outcolor==1)
					{
						if(outalpha==0)
						{
							out_pixel[0] = in_pix[2];
							out_pixel[1] = in_pix[1];
							out_pixel[2] = in_pix[0];
						}
						else
						{
							out_pixel[0] = in_pix[0];
							out_pixel[1] = in_pix[1];
							out_pixel[2] = in_pix[2];
						}
						if(outalpha)out_pixel[3] = alphaval;
					}
					if(outcolor==2)*out_pix16 = ((in_pix[2]>>3) & 0x1f) | (((in_pix[1]>>2) & 0x3f)<<5) | (((in_pix[0]>>3) & 0x1f)<<11);//BGR565
					if(outcolor==3)*out_pix16 = ((in_pix[0]>>4) & 0xf) | (((in_pix[1]>>4) & 0xf)<<4) | (((in_pix[2]>>4) & 0xf)<<8) | (((alphaval>>4) & 0xf)<<12);//RGBA4444
					if(outcolor==4)
					{
						if(alphaval)alphaval = 1;
						*out_pix16 = ((in_pix[0]>>3) & 0x1f) | (((in_pix[1]>>3) & 0x1f)<<5) | (((in_pix[2]>>3) & 0x1f)<<10) | (alphaval<<15);//RGBA5551
					}
				}
			}
		}
	}
	else
	{
		in_pixsz = tgahdr->palette_colorbitsize / 8;

		memset(out_gfx, 0, 0x100*in_pixsz);
		memcpy(out_gfx, &tmp_gfx[tgahdr->palette_firstent], tgahdr->palette_totalcolors * in_pixsz);
		memcpy(&out_gfx[0x100*in_pixsz], &tmp_gfx[tgahdr->palette_firstent + tgahdr->palette_totalcolors * in_pixsz], tgahdr->width*tgahdr->height);
	}

	return 0;
}*/

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

/*void send_pxireply()
{
	u32 i;
	u32 pxi_id;
	u32 totalwords = 3;
	u32 pxidata[3];

	//pxidata[0] = 8;
	pxidata[1] = 0x00030040;
	pxidata[2] = 0xc8888888;

	pxi_id = 0;

	//for(pxi_id=0; pxi_id<16; pxi_id++)
	//{
		pxidata[0] = pxi_id;
		pxidata[2]++;
		for(i=0; i<totalwords; i++)
		{
			while(*((vu32*)0x10008004) & 2);

			*((vu32*)0x10008008) = pxidata[i];
		}
	//}
}*/

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
	//u32 pos;

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

	arm9_patchaddr = (u32*)&ptr[0x11c>>2];

	arm9_patchaddr[0] = arm9_stub[0];
	arm9_patchaddr[1] = arm9_stub[1];
	arm9_patchaddr[2] = arm9_stub[2];

	//*((u32*)0x8086b7c) = 0xe3a00000;//0xe3e00000;//patch the launch_firm fs_openfile() blx to "mov r0, #0".
	ptr[0x140>>2] = 0xe3a00000;//patch the fileread blx for the 0x100-byte FIRM header to "mov r0, #0".
	ptr[0x154>>2] = 0xe3a00000;//patch the bne branch after the fileread call.
	ptr[0x170>>2] = 0xe3a00000;//patch the FIRM signature read.
	ptr[0x17C>>2] = 0xe3a00c01;//"mov r0, #0x100".
	//*((u32*)0x8086c0c) = 0xe1a00000;//nop the fs_closefile() call, this patch isn't needed since this function won't really do anything since the file ctx wasn't initialized via fs_openfile.
	ptr[0x3D8>>2] = 0xe3a00000;//patch the RSA signature verification func call.
	ptr[0x594>>2] = 0xe3a00000;//patch the func-call which reads the encrypted ncch firm data.
	//*((u32*)0x8087048) = 0xe1a00000;//patch out the fs_closefile() func-call.
	ptr[0x5D0>>2] = 0xe1a00000;//patch out the func-call which is immediately after the fs_closefile call. (FS shutdown stuff)

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

/*void write_loginfo(u8 *data, u32 datasize)
{
	u32 pos=0;
	u16 filepath[64];
	char *path = (char*)"/3dshax_debug.bin";

	if(fileobj_debuginfo==NULL)
	{
		memset(filepath, 0, 64*2);
		for(pos=0; pos<strlen(path); pos++)filepath[pos] = (u16)path[pos];

		if(openfile(sdarchive_obj, 4, filepath, (strlen(path)+1)*2, 7, &fileobj_debuginfo)!=0)return;
	}

	filewrite(fileobj_debuginfo, &datasize, 4, debuginfo_pos);
	debuginfo_pos+= 4;
	filewrite(fileobj_debuginfo, data, datasize, debuginfo_pos);
	debuginfo_pos+= datasize;
}*/

u32 *get_kprocessptr(u64 procname, u32 num, u32 get_mmutableptr)
{
	u32 pos=0;
	//u32 pos2=0;
	u32 kcodeset_adr=0;
	u32 *wram = (u32*)0x1FF80000;
	u32 *kprocess = NULL;
	u32 slabheap_physaddr = 0x1FFA0000;
	u32 kernelfcram_phys2vaddr_value = 0xd0000000;
	u32 kprocess_adjustoffset = 0;

	if(RUNNINGFWVER>=0x37)
	{
		kernelfcram_phys2vaddr_value = 0xc0000000;
		kprocess_adjustoffset = 8;
	}

	for(pos=0; pos<(0x80000>>2)-1; pos++)//kcodeset_adr = KCodeSet object arm11 kernel vaddr for the specified process name.
	{
		if(wram[pos]==((u32)procname) && wram[pos+1]==((u32)(procname>>32)))
		{
			kcodeset_adr = ((pos<<2) - 0x50) + 0x1FF80000;
			kcodeset_adr = (kcodeset_adr - slabheap_physaddr) + 0xFFF00000;
			break;
		}
	}

	if(kcodeset_adr==0)return NULL;

	for(pos=0; pos<(0x80000>>2); pos++)//kprocess = physical addr for KProcess object containing the above KCodeSet addr.
	{
		if(wram[pos]==kcodeset_adr)
		{
			if(num)
			{
				num--;
			}
			else
			{
			kprocess = &wram[pos - ((0xa8+kprocess_adjustoffset)>>2)];
				break;
			}
		}
	}

	if(kprocess==NULL)return NULL;

	if(get_mmutableptr)return (u32*)(kprocess[(0x54+kprocess_adjustoffset)>>2] - kernelfcram_phys2vaddr_value);
	return kprocess;
}

u8 *mmutable_convert_vaddr2physaddr(u32 *mmutable, u32 vaddr)
{
	u32 *ptr;
	u32 val;

	if(mmutable==NULL)return NULL;

	val = mmutable[vaddr >> 20];
	if((val & 0x3) == 0x0 || (val & 0x3) == 0x3)return NULL;
	if((val & 0x3) == 0x2)return (u8*)((val & ~0xFFFFF) | (vaddr & 0xFFFFF));

	ptr = (u32*)(val & ~0x3FF);
	val = ptr[(vaddr >> 12) & 0xff];
	if((val & 0x3) == 0)return NULL;
	if((val & 0x2) == 0)
	{
		return (u8*)((val & ~0xFFFF) | (vaddr & 0xFFFF));
	}

	return (u8*)((val & ~0xFFF) | (vaddr & 0xFFF));
}

/*u32 *patch_mmutables(u64 procname, u32 patch_permissions, u32 num)
{
	u32 pos=0, pos2=0;
	//u32 *wram = (u32*)0x1FF80000;
	u32 *kprocess;
	u32 *mmutable = NULL;
	u32 *mmutableL2;
	u32 *page_physaddr = 0;

	kprocess = get_kprocessptr(procname, num, 0);
	if(kprocess==NULL)return NULL;

	mmutable = (u32*)(kprocess[0x54>>2] - 0xd0000000);

	for(pos=1; pos<0x10; pos++)
	{
		if(mmutable[pos]==0)break;

		mmutableL2 = (u32*)((mmutable[pos] >> 10) << 10);

		for(pos2=0; pos2<0x100; pos2++)
		{
			if(mmutableL2[pos2]==0)break;

			if(page_physaddr==0)page_physaddr = (u32*)((mmutableL2[pos2] >> 12) << 12);
			if(patch_permissions)mmutableL2[pos2] |= 0x30;//Set permissions to usr-RW/priv-RW.
		}
	}

	if(patch_permissions)
	{
		kprocess[(28+8 + 0)>>2] |= 0x0101;//Set the TLB invalidation flags used when doing a process context switch.(CPUID0+CPUID1)
	}

	return page_physaddr;
}*/

/*void dump_fcramvram()
{
	u32 pos=0;
	u16 filepath[64];
	u32 *fileobj = NULL;
	char *path = (char*)"/3dshax_vram.bin";

	memset(filepath, 0, 64*2);
	for(pos=0; pos<strlen(path); pos++)filepath[pos] = (u16)path[pos];

	dumpmem((u32*)0x20000000, 0x08000000);

	if(openfile(sdarchive_obj, 4, filepath, (strlen(path)+1)*2, 7, &fileobj)!=0)return;
	if(filewrite(fileobj, (u32*)0x18000000, 0x00600000, 0)!=0)return;
}*/

void dump_fcramaxiwram()
{
	u32 pos=0;
	u16 filepath[64];
	u32 *fileobj = NULL;
	char *path = (char*)"/3dshax_axiwram.bin";

	memset(filepath, 0, 64*2);
	for(pos=0; pos<strlen(path); pos++)filepath[pos] = (u16)path[pos];

	dumpmem((u32*)0x20000000, 0x08000000);

	if(openfile(sdarchive_obj, 4, filepath, (strlen(path)+1)*2, 7, &fileobj)!=0)return;
	if(filewrite(fileobj, (u32*)0x1FF80000, 0x80000, 0)!=0)return;
}

/*void dump_nandfile(char *path)
{
	u32 ret=0;
	u32 pos=0;
	u32 filesize = 0;
	u16 filepath[256];
	u32 *fileobj = NULL;

	if(initialize_nandarchiveobj()!=0)return;

	memset(filepath, 0, 256*2);
	for(pos=0; pos<strlen(path); pos++)filepath[pos] = (u16)path[pos];

	if((ret = openfile(nandarchive_obj, 4, filepath, (strlen(path)+1)*2, 1, &fileobj))!=0)
	{
		dumpmem(&ret, 4);
		return;
	}
	filesize = getfilesize(fileobj);
	if((ret = fileread(fileobj, (u32*)0x20703000, filesize, 0))!=0)
	{
		dumpmem(&ret, 4);
		return;
	}

	dumpmem((u32*)0x20703000, filesize);
}*/

u32 nand_readsector(u32 sector, u32 *outbuf, u32 sectorcount)
{
	u32 val;
	u32 *state;// = (u32*)0x080d77e0;//FWVER 0x1F
	//u32 (*funcptr)(u32*, u32*, u32, u32, u32);
	//u32 outclass[2];

	//if(RUNNINGFWVER>=0x2E)state = (u32*)0x080d6f40;
	val = ((u32)pxifs_state) - 8;
	state = (u32*)(*((u32*)val));

	/*funcptr = (void*)*((u32*)((*state) + 8));

	outclass[0] = 0x080944c8;//FWVER 0x1F
	if(RUNNINGFWVER==0x2E)outclass[0] = 0x0809106c;
	outclass[1] = outbuf;

	return funcptr(state, outclass, 0, sector, sectorcount);*/

	return archive_readsectors(state, outbuf, sectorcount, sector);
}

void dump_nandimage()
{
	u32 ret = 0;
	u32 pos = 0, sectorbase = 0, sectori = 0;
	u32 *fileobj;

	if(openfile(sdarchive_obj, 4, dump_filepath, 0x22, 7, &fileobj)!=0)return;

	for(pos=0; pos<0x3af00000; pos+=0x100000)
	{
		sectorbase = pos/0x200;

		for(sectori=0; sectori<(0x100000/0x200); sectori+=8)
		{
			ret = nand_readsector(sectorbase + sectori, ((u32*)(0x20000000 + sectori*0x200)), 8);
			if(ret!=0)return;
		}

		if(filewrite(fileobj, (u32*)0x20000000, 0x100000, pos)!=0)return;
	}
}

void loadrun_file(char *path, u32 *addr, int execute)
{
	u32 ret=0;
	u32 pos=0;
	u32 filesize = 0;
	u16 filepath[256];
	u32 *fileobj = NULL;
	u32 (*funcptr)(void) = (void*)addr;

	memset(filepath, 0, 256*2);
	for(pos=0; pos<strlen(path); pos++)filepath[pos] = (u16)path[pos];

	if((ret = openfile(sdarchive_obj, 4, filepath, (strlen(path)+1)*2, 1, &fileobj))!=0)
	{
		return;
	}
	filesize = getfilesize(fileobj);
	if((ret = fileread(fileobj, addr, filesize, 0))!=0)
	{
		return;
	}

	svcFlushProcessDataCache(addr, filesize);

	if(execute)funcptr();
}

/*void pxirecv()
{
	u32 pos=0;
	u32 i;
	u32 *fileobj = NULL;

	memset(framebuf_addr, 0x10101010, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0x10101010, 0x46500);

	memset(pxibuf, 0, 0x200);

	if(openfile(sdarchive_obj, 4, dump_filepath, 0x22, 7, &fileobj)!=0)return;

	while(*((vu16*)0x10146000) & 2)
	{
		if(*((vu32*)0x10008004) & 0x100)continue;

		pxibuf[pos] = *((vu32*)0x1000800c);
		pos++;
		if(pos >= 0x200>>2)break;
	}

	memset(pxibuf, 0xc0c0c0c0, 0x200);

	if(filewrite(fileobj, pxibuf, 0x200, 0)!=0)return;

	memset(framebuf_addr, pxibuf[0], 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], pxibuf[0], 0x46500);
}

void dump_pxirecv()
{
	memset(pxibuf, 0, 0x20);
	memset(framebuf_addr, 0x48484848, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0x48484848, 0x46500);
	launchcode_kernelmode(pxirecv);
	memset(framebuf_addr, 0x13333337, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0x13333337, 0x46500);
	dumpmem(pxibuf, 0x20);
}*/

/*void debug_memalloc()
{
	u32 *ptr = NULL;
	u32 *fileobj = NULL;

	u32* (*funcptr)(u32) = (void*)0x8060548;

	ptr = funcptr(0xffffffff);
	
	if(openfile(sdarchive_obj, 4, dump_filepath, 0x22, 7, &fileobj)!=0)return;
	if(filewrite(fileobj, &ptr, 4, 0)!=0)return;
	if(filewrite(fileobj, (u32*)0x08028004, 0xD8000-4, 4)!=0)return;
}*/

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

int ctrserver_processcmd(u32 cmdid, u32 *buf, u32 *bufsize)
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
		//buf[0] = gamecard_readsectors(&buf[1], buf[0], buf[1]);//buf[0]=sector#, buf[1]=sectorcount
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

	if(type==0x43565253)//"SRVC"
	{
		payloadsize = cmdbuf[3];
		ret = ctrserver_processcmd(cmdbuf[2], (u32*)cmdbuf[4], &payloadsize);
	}

	cmdbuf[0] = 0x00000040;
	cmdbuf[1] = (u32)ret;

	if(type==0x43565253)//"SRVC"
	{
		cmdbuf[0] = 0x00000080;
		cmdbuf[2] = payloadsize;
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

void writepatch_arm11kernel_kernelpanicbkpt(u32 *ptr, u32 size)
{
	u32 pos, i;

	pos = 0;
	while(size)
	{
		if(ptr[pos] == 0xffff9004 && ptr[pos+1] == 0x010000ff)//Locate the kernelpanic() function(s) via the .pool data. Note that older kernel versions had two kernelpanic() functions.
		{
			for(i=0; i<(0x400/4); i++)
			{
				if(ptr[pos-i] == 0xe92d4010)//"push {r4, lr}"
				{
					//The actual start of the function is this instruction, immediately before the push instruction: "ldr r0, [pc, <offset>]"
					ptr[pos-i-1] = 0xE1200070;//"bkpt #0"
					break;
				}
			}
		}

		pos++;
		size-= 4;
	}
}

/*u32 mountcontent_openfile_hook_opensd(u32 *archiveclass, u32 **fileobj, u32 unk0, u32 *lowpath, u32 openflags, u32 unk1)
{
	u32 *lowpathdata = (u32*)lowpath[1];
	u32 pos;
	char *path = "/3dshax_title_code.bin";//"/3dshax_title_icon.bin";
	u16 filepath[256];

	//if(lowpath[2]!=0x14 || lowpathdata[3]!=0x6e6f6369)return 1;//Only handle ExeFS:/icon
	if(lowpath[2]!=0xc || lowpathdata[1]!=0x646f632e)return 1;//Only handle ExeFS:/.code

	write_loginfo(lowpathdata, lowpath[2]);

	memset(filepath, 0, 256*2);
	for(pos=0; pos<strlen(path); pos++)filepath[pos] = (u16)path[pos];

	if(openfile(sdarchive_obj, 4, filepath, (strlen(path)+1)*2, 1, fileobj)!=0)return 1;

	return 0;
}*/

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

			//dump_nandimage();

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

	//load_arm11code(NULL, 0);
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
			//load_arm11code(NULL, 0);

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

			//loadrun_file("/x01ffb800.bin", (u32*)0x01ffb800, 0);
			if(RUNNINGFWVER<0x37)
			{
				launch_firm();

				//Change the configmem UPDATEFLAG value to 1, which then causes NS module to do a FIRM launch, once NS gets loaded.
				ptr = NULL;
				while(ptr == NULL)ptr = (u32*)mmutable_convert_vaddr2physaddr(get_kprocessptr(0x697870, 0, 1), 0x1FF80004);
				*ptr = 1;
			}
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

