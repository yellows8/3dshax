#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <nds/ndstypes.h>

#include "arm9_svc.h"

#ifdef ENABLEAES
//Most of this code is old/unused, or one-off. Note that this currently won't build under this seperate .c file.

/*u32 inbuf[0x20>>2];
u32 metadata[0x24>>2];
u32 calcmac[4];*/

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

