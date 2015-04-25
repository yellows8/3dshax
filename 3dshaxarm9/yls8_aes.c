#ifdef ENABLEAES

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <nds/ndstypes.h>

#include "arm9_svc.h"

#define REG_AESCNT *((vu32*)0x10009000)
#define REG_AESBLKCNT *((vu32*)0x10009004)
#define REG_AESWRFIFO *((vu32*)0x10009008)
#define REG_AESRDFIFO *((vu32*)0x1000900C)
#define REG_AESKEYSEL *((vu8*)0x10009010)
#define REG_AESKEYCNT *((vu8*)0x10009011)
#define REG_AESCTR ((vu32*)0x10009020)
#define REG_AESMAC ((vu32*)0x10009030)
#define REG_AESKEY0 ((vu32*)0x10009040)
#define REG_AESKEY1 ((vu32*)0x10009070)
#define REG_AESKEY2 ((vu32*)0x100090A0)
#define REG_AESKEY3 ((vu32*)0x100090D0)
#define REG_AESKEYFIFO *((vu32*)0x10009100)
#define REG_AESKEYXFIFO *((vu32*)0x10009104)
#define REG_AESKEYYFIFO *((vu32*)0x10009108)
#define REG_AESKEYFIFO_PTR ((vu32*)0x10009100)
#define REG_AESKEYXFIFO_PTR ((vu32*)0x10009104)
#define REG_AESKEYYFIFO_PTR ((vu32*)0x10009108)

#define AES_CHUNKSIZE 0xffff0

extern u32 proc9_textstartaddr;

u32 aesiv[4];

void (*funcptr_aesmutex_enter)() = NULL;
void (*funcptr_aesmutex_leave)() = NULL;

void aesengine_waitdone();
void aesengine_flushfifo();

void ctr_add_counter( u8 *ctr, u32 carry );

u32 parse_branch(u32 branchaddr, u32 branchval);

void aes_mutex_ptrsinitialize()
{
	u32 *ptr = (u32*)proc9_textstartaddr;
	u32 pos;

	for(pos=0; pos<(0x080ff000-proc9_textstartaddr)>>2; pos++)
	{
		if(ptr[pos]==0xe8bd8008 && ptr[pos+1]==0x10011000)//"pop {r3, pc}" + reg addr
		{
			pos--;
			ptr = (u32*)(parse_branch((u32)&ptr[pos], 0) | 1);
			funcptr_aesmutex_leave = (void*)ptr;
			funcptr_aesmutex_enter = (void*)(((u32)ptr)-0x18);
			return;
		}
	}
}

void aes_mutexenter()
{
	if(funcptr_aesmutex_enter==NULL)
	{
		aes_mutex_ptrsinitialize();
		if(funcptr_aesmutex_enter==NULL)return;
	}

	funcptr_aesmutex_enter();
}

void aes_mutexleave()
{
	if(funcptr_aesmutex_leave==NULL)
	{
		aes_mutex_ptrsinitialize();
		if(funcptr_aesmutex_leave==NULL)return;
	}

	funcptr_aesmutex_leave();
}

void aes_set_ctr(u32 *ctr)
{
	u32 i;

	if(ctr)memcpy(aesiv, ctr, 16);
	if(ctr==NULL)ctr = aesiv;

	if((REG_AESCNT >> 23) & 4)
	{
		for(i=0; i<4; i++)REG_AESCTR[i] = ctr[3-i];
	}
	else
	{
		for(i=0; i<4; i++)REG_AESCTR[i] = ctr[i];
	}
}

void aes_set_iv(u32 *iv)
{
	aes_set_ctr(iv);
}

void aes_select_key(u32 keyslot)
{
	REG_AESKEYSEL = keyslot;
	REG_AESCNT |= 1<<26;

	if(keyslot<4)
	{
		REG_AESCNT &= ~(0xf<<22);
		//REG_AESCNT |= (0xa<<22);
	}
	else
	{
		REG_AESCNT |= (0xf<<22);
	}
}

void aes_set_keydata(u32 keyslot, u32 *key, u32 keytype)
{
	u32 *ptr = NULL;
	u32 i;

	if(keyslot<4)
	{
		REG_AESCNT &= ~(0xf<<22);
		//REG_AESCNT |= (0xa<<22);
	}
	else
	{
		REG_AESCNT |= (0xf<<22);
	}

	REG_AESKEYCNT = (REG_AESKEYCNT & ~0x3f) | keyslot | 0x80;

	if(keytype>2)return;
	if(keytype==0)ptr = (u32*)REG_AESKEYFIFO_PTR;
	if(keytype==1)ptr = (u32*)REG_AESKEYXFIFO_PTR;
	if(keytype==2)ptr = (u32*)REG_AESKEYYFIFO_PTR;

	if(keyslot>=4)
	{
		for(i=0; i<4; i++)*ptr = key[i];
	}
	else
	{
		ptr = (u32*)REG_AESKEY0;

		ptr = &ptr[0xc*keyslot + keytype*4];
		for(i=0; i<4; i++)ptr[i] = key[i];
	}
}

void aes_set_ykey(u32 keyslot, u32 *key)
{
	aes_set_keydata(keyslot, key, 2);
}

void aes_set_xkey(u32 keyslot, u32 *key)
{
	aes_set_keydata(keyslot, key, 1);
}

void aes_set_key(u32 keyslot, u32 *key)
{
	aes_set_keydata(keyslot, key, 0);
}

void aesengine_initoperation(u32 mode, u16 aesblks, u16 macassocblks)
{
	u32 val = (REG_AESCNT >> 22) & 0xf;//Save the endian/word-order values.
	
	REG_AESCNT = 0;
	REG_AESBLKCNT = macassocblks | (aesblks<<16);

	aesengine_flushfifo();
	REG_AESCNT |= (val << 22) | ((mode & 0x7) << 27) | (1<<31);
}

void aesengine_cryptdata_wrap(u32 *output, u32 *input, u32 size)
{
	u32 pos, i;
	u32 chunkwords;

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

	aesengine_waitdone();
}

void aesengine_cryptdata(u32 mode, u32 *output, u32 *input, u32 aes_size, u32 macassoc_size)
{
	u32 pos;
	u32 chunksize;

	for(pos = 0; pos<aes_size; pos+= chunksize)
	{
		chunksize = aes_size - pos;
		if(chunksize>AES_CHUNKSIZE)chunksize = AES_CHUNKSIZE;

		aes_set_iv(NULL);

		aesengine_initoperation(mode, chunksize>>4, 0);

		if(mode==4)//AES-CBC decrypt
		{
			memcpy(aesiv, &input[(pos + chunksize - 0x10)>>2], 0x10);
		}
		else if(mode!=5)//AES-CTR
		{
			ctr_add_counter((u8*)aesiv, chunksize>>4);
		}

		aesengine_cryptdata_wrap(&output[pos>>2], &input[pos>>2], chunksize);

		if(mode==5)//AES-CBC encrypt
		{
			memcpy(aesiv, &output[(pos + chunksize - 0x10)>>2], 0x10);
		}
	}

	aesengine_flushfifo();
}

void aesengine_waitdone()
{
	while(REG_AESCNT>>31);
}

void aesengine_flushfifo()
{
	REG_AESCNT |= 0xc00;
}

void aes_ctr_crypt(u32 *buf, u32 size)
{
	aesengine_cryptdata(2, buf, buf, size, 0);
}

void aes_cbc_decrypt(u32 *buf, u32 size)
{
	aesengine_cryptdata(4, buf, buf, size, 0);
}

void aes_cbc_encrypt(u32 *buf, u32 size)
{
	aesengine_cryptdata(5, buf, buf, size, 0);
}

void ctr_add_counter( u8 *ctr, u32 carry )//Based on the ctrtool function.
{
	u32 counter[4];
	u32 sum;
	int i;

	for(i=0; i<4; i++)
		counter[i] = (ctr[i*4+0]<<24) | (ctr[i*4+1]<<16) | (ctr[i*4+2]<<8) | (ctr[i*4+3]<<0);

	for(i=3; i>=0; i--)
	{
		sum = counter[i] + carry;

		if (sum < counter[i])
			carry = 1;
		else
			carry = 0;

		counter[i] = sum;
	}

	for(i=0; i<4; i++)
	{
		ctr[i*4+0] = counter[i]>>24;
		ctr[i*4+1] = counter[i]>>16;
		ctr[i*4+2] = counter[i]>>8;
		ctr[i*4+3] = counter[i]>>0;
	}
}

#endif

