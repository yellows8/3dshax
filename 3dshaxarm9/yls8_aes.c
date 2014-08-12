#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <nds/ndstypes.h>

#include "aes.h"

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

extern u32 RUNNINGFWVER;

/*u32 inbuf[0x20>>2];
u32 metadata[0x24>>2];
u32 calcmac[4];*/
u32 aesiv[4];

extern u32 aes_keystate0, aes_keystate1, aes_keystate2, aes_keystate3;
extern u64 aes_time0, aes_time1;

void launchcode_kernelmode(void*);

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
#endif

