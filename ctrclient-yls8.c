#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "ctrclient.h"

void hexdump(void *ptr, int buflen)//From ctrtool.
{
	unsigned char *buf = (unsigned char*)ptr;
	int i, j;

	for (i=0; i<buflen; i+=16)
	{
		printf("%06x: ", i);
		for (j=0; j<16; j++)
		{ 
			if (i+j < buflen)
			{
				printf("%02x ", buf[i+j]);
			}
			else
			{
				printf("   ");
			}
		}

		printf(" ");

		for (j=0; j<16; j++) 
		{
			if (i+j < buflen)
			{
				printf("%c", (buf[i+j] >= 0x20 && buf[i+j] <= 0x7e) ? buf[i+j] : '.');
			}
		}
		printf("\n");
	}
}

unsigned int getbe32(unsigned char* p)//getbe32 and putle32 are based on the code from ctrtool utils.c.
{
	return (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | (p[3]<<0);
}
void putle32(unsigned char* p, unsigned int n)
{
	p[0] = n;
	p[1] = n>>8;
	p[2] = n>>16;
	p[3] = n>>24;
}

int load_bindata(char *arg, unsigned char **buf, unsigned int *size)
{
	int i;
	unsigned int tmp=0;
	unsigned char *bufptr;
	FILE *f;
	struct stat filestat;

	//if(strlen(arg) != size*2)exit(1);

	bufptr = *buf;

	if(arg[0]!='@')
	{
		if(bufptr==NULL)
		{
			*size = strlen(arg) / 2;
			*buf = (unsigned char*)malloc(*size);
			bufptr = *buf;
			if(bufptr==NULL)
			{
				printf("Failed to allocate memory for input buffer.\n");
				return 1;
			}

			memset(bufptr, 0, *size);
		}

		for(i=0; i<*size; i++)
		{
			if(i>=strlen(arg))break;
			sscanf(&arg[i*2], "%02x", &tmp);
			bufptr[i] = (unsigned char)tmp;
		}
	}
	else
	{
		if(stat(&arg[1], &filestat)==-1)
		{
			printf("Failed to stat %s\n", &arg[1]);
			return 2;
		}

		f = fopen(&arg[1], "rb");
		if(f==NULL)
		{
			printf("Failed to open %s\n", &arg[1]);
			return 2;
		}

		if(bufptr)
		{
			if(*size < filestat.st_size)*size = filestat.st_size;
		}
		else
		{
			*size = filestat.st_size;
			*buf = (unsigned char*)malloc(*size);
			bufptr = *buf;

			if(bufptr==NULL)
			{
				printf("Failed to allocate memory for input buffer.\n");
				return 1;
			}

			memset(bufptr, 0, *size);
		}

		if(fread(bufptr, 1, *size, f) != *size)
		{
			printf("Failed to read file %s\n", &arg[1]);
			fclose(f);
			return 3;
		}

		fclose(f);
	}

	return 0;
}

int cmd8c0_installcia(ctrclient *client, unsigned char mediatype, unsigned char dbselect, unsigned char *ciabuf, unsigned int ciabufsize, unsigned int maxchunksize)
{
	unsigned int *ciabuf32 = (unsigned int*)ciabuf;
	unsigned int offset = 0, chunksize=0;
	unsigned int *titleid;
	unsigned int ackword = 0;
	unsigned int header[5];

	if(ciabufsize < 0x2020)
	{
		printf("Invalid input CIA size.\n");
		return 3;
	}

	if(ctrclient_sendlong(client, 0x8c0)!=1)return 1;
	if(ctrclient_sendlong(client, 0x14)!=1)return 1;

	offset+= (ciabuf32[0x0] + 0x3f) & ~0x3f;
	offset+= (ciabuf32[0x2] + 0x3f) & ~0x3f;
	offset+= 0x1dc;

	titleid = (unsigned int*)&ciabuf[offset];//titleid in the ticket

	memset(header, 0, 0x14);

	if(maxchunksize==0)maxchunksize = 0x3ffc0;

	header[0] = mediatype | (dbselect<<8);
	putle32((unsigned char*)&header[1], getbe32((unsigned char*)&titleid[1]));
	putle32((unsigned char*)&header[2], getbe32((unsigned char*)&titleid[0]));
	header[3] = ciabufsize;
	header[4] = maxchunksize;

	if(ctrclient_sendbuffer(client, header, 0x14)!=1)return 1;

	printf("Sending CIA...\n");

	offset = 0;
	while(offset < ciabufsize)
	{
		chunksize = ciabufsize - offset;
		if(chunksize > maxchunksize)chunksize = maxchunksize;

		printf("Sending data at offset 0x%x size 0x%x... ", offset, chunksize);

		if(ctrclient_sendbuffer(client, &ciabuf[offset], chunksize)!=1)return 1;

		printf("Waiting for ACK...\n");

		ackword = 0;
		if(ctrclient_recvbuffer(client, &ackword, 4)!=1)
		{
			printf("Failed to recieve ACK message.\n");
			return 1;
		}
		if(ackword!=0x4b4341)
		{
			printf("ACK message is invalid, or the server sent the final command response too early.\n");
			return 1;
		}

		offset+= chunksize;
	}

	//if(ctrclient_sendbuffer(client, ciabuf, ciabufsize)!=1)return 1;

	memset(header, 0, 0x14);

	printf("Receiving reply...\n");
	if(ctrclient_recvbuffer(client, header, 0x14)!=1)return 1;

	printf("Command-index: %x\n", header[2+0]);
	printf("Result-code: %x\n", header[2+1]);
	printf("Write pos: %x\n", header[2+2]);

	return 0;
}

int cmd_deletetitle(ctrclient *client, unsigned int mediatype, unsigned int cmdtype, unsigned long long titleid)
{
	unsigned int header[5];

	memset(header, 0, 5*4);
	
	header[0] = 0x8c1;
	header[1] = 0xc;
	header[2] = mediatype;
	if(cmdtype)header[2] |= (1<<8);
	memcpy(&header[3], &titleid, 8);

	if(ctrclient_sendbuffer(client, header, 5*4)!=1)return 1;
	if(ctrclient_recvbuffer(client, header, 3*4)!=1)return 1;

	printf("Result-code: 0x%x\n", header[2]);

	return 0;
}

int cryptdata(ctrclient *client, unsigned int keyslot, int keytype, int cryptmode, unsigned char *key, unsigned char *ctr, unsigned char *buffer, unsigned int bufsize, char *outpath, unsigned char *keyX)
{
	FILE *fout;
	unsigned int cmd[0x1c>>2];

	printf("keytype: %d\n", keytype);
	printf("key:\n");
	hexdump(key, 16);
	printf("ctr:\n");
	hexdump(ctr, 16);
	printf("input:\n");
	hexdump(buffer, 16);

	if(keyX)
	{
		printf("keyX:\n");
		hexdump(keyX, 16);

		memset(cmd, 0, 0x1c);
		cmd[0] = 0xe2;
		cmd[1] = 0x14;
		cmd[2] = keyslot;
		memcpy(&cmd[3], keyX, 16);

		if(ctrclient_sendbuffer(client, cmd, 0x1c)!=1)return 1;
		if(ctrclient_recvbuffer(client, cmd, 8)!=1)return 1;
	}

	if(keytype)
	{
		if(keytype==1)
		{
			if (!ctrclient_aes_set_key(client, keyslot, key))return 1;
		}

		if(keytype==2)
		{
			if (!ctrclient_aes_set_ykey(client, keyslot, key))return 1;
		}
	}
	else
	{
		if (!ctrclient_aes_select_key(client, keyslot))return 1;
	}

	if(cryptmode==2)
	{
		if (!ctrclient_aes_set_ctr(client, ctr))return 1;
	}
	else
	{
		if (!ctrclient_aes_set_iv(client, ctr))return 1;
	}

	if(cryptmode==2)
	{
		if (!ctrclient_aes_ctr_crypt(client, buffer, bufsize))return 1;
	}
	else if(cryptmode==4)
	{
		if (!ctrclient_aes_cbc_decrypt(client, buffer, bufsize))return 1;
	}
	else if(cryptmode==5)
	{
		if (!ctrclient_aes_cbc_encrypt(client, buffer, bufsize))return 1;
	}
	else
	{
		return 1;
		//if (!ctrclient_aes_crypto(client, buffer, bufsize, 0xe1 + cryptmode))return 1;
	}
	
	printf("output:\n");
	hexdump(buffer, 16);

	if(outpath[0])
	{
		printf("Writing output...\n");
		fout = fopen(outpath, "wb");
		if(fout)
		{
			fwrite(buffer, 1, bufsize, fout);
			fclose(fout);
		}
		else
		{
			printf("Failed to open file for output data writing: %s\n", outpath);
			return 2;
		}
	}

	return 0;
}

int cmd_memoryrw(ctrclient *client, unsigned int address, unsigned char *buf, unsigned int size, unsigned int rw, unsigned int type)
{
	int buftransfer = 0;
	unsigned int cmdid;
	unsigned int header[2];

	if(size==1)
	{
		cmdid = 0x01;
	}
	else if(size==2)
	{
		cmdid = 0x02;
	}
	else if(size==4)
	{
		cmdid = 0x03;
	}
	else
	{
		cmdid = 0x04;
		buftransfer = 1;
	}
	
	if(!rw)cmdid+= 0x4;

	//type0 = arm11-usrmode under the ctrserver process, type1 = arm9, type2 = arm11kernel-mode
	if(!type)
	{
		cmdid |= 0x800;
	}
	else if(type==2)
	{
		cmdid |= 0x400;
	}

	if(ctrclient_sendlong(client, cmdid)!=1)return 1;
	if(buftransfer==0)
	{
		if(ctrclient_sendlong(client, 4 + rw*4)!=1)return 1;
	}
	else
	{
		if(!rw)
		{
			if(ctrclient_sendlong(client, 8)!=1)return 1;
		}
		else
		{
			if(ctrclient_sendlong(client, 4+size)!=1)return 1;
		}
	}

	if(ctrclient_sendlong(client, address)!=1)return 1;

	if(buftransfer && !rw)
	{
		if(ctrclient_sendlong(client, size)!=1)return 1;
	}

	if(buftransfer==0)
	{
		if(!rw)
		{
			if(ctrclient_recvbuffer(client, header, 8)!=1)return 1;
			if(ctrclient_recvbuffer(client, buf, 4)!=1)return 1;
		}
		else
		{
			if(ctrclient_sendbuffer(client, buf, 4)!=1)return 1;
			if(ctrclient_recvbuffer(client, header, 8)!=1)return 1;
		}
	}

	if(buftransfer)
	{
		if(!rw)
		{
			if(ctrclient_recvbuffer(client, header, 8)!=1)return 1;
			if(ctrclient_recvbuffer(client, buf, size)!=1)return 1;
		}
		else
		{
			if(ctrclient_sendbuffer(client, buf, size)!=1)return 1;
			if(ctrclient_recvbuffer(client, header, 8)!=1)return 1;
		}
	}

	return 0;
}

int cmd_memset(ctrclient *client, unsigned int address, unsigned int value, unsigned int size, unsigned int type)
{
	unsigned int cmdid;
	unsigned int header[5];

	memset(header, 0, sizeof(header));
	
	cmdid = 0xe;

	//type0 = arm11-usrmode under the ctrserver process, type1 = arm9, type2 = arm11kernel-mode
	if(!type)
	{
		cmdid |= 0x800;
	}
	else if(type==2)
	{
		cmdid |= 0x400;
	}

	header[0] = cmdid;
	header[1] = 0xc;

	header[2] = address;
	header[3] = value;
	header[4] = size;

	if(ctrclient_sendbuffer(client, header, 5*4)!=1)return 1;
	if(ctrclient_recvbuffer(client, header, 2*4)!=1)return 1;

	return 0;
}

int cmd_sendservicecmd(ctrclient *client, unsigned int handle, unsigned int handletype, unsigned int *inbuf, unsigned int insize, unsigned int **outbuf, unsigned int *outsize)
{
	unsigned int header[4];

	memset(header, 0, 4*4);
	
	header[0] = 0x850;
	header[1] = 0x8 + insize;
	header[2] = handle;
	header[3] = handletype;

	if(ctrclient_sendbuffer(client, header, 4*4)!=1)return 1;
	if(ctrclient_sendbuffer(client, inbuf, insize)!=1)return 1;
	if(ctrclient_recvbuffer(client, header, 2*4)!=1)return 1;

	*outsize = header[1];

	if(*outsize == 0)return 1;

	if(ctrclient_recvbuffer(client, &header[2], 4)!=1)return 1;

	if(header[2]!=0)
	{
		if(header[2] == ~0)
		{
			printf("Invalid handle type.\n");
			return 8;
		}
		else
		{
			printf("Error from svcSendSyncRequest: 0x%x.\n", header[2]);
			return 8;
		}
	}

	*outsize -= 4;

	if(*outsize == 0)return 0;

	*outbuf = (unsigned int*)malloc(*outsize);
	if(*outbuf == NULL)
	{
		printf("Failed to allocate memory for the service command response.\n");
		return 2;
	}

	if(ctrclient_recvbuffer(client, *outbuf, *outsize)!=1)return 1;

	return 0;
}

int cmd_getprocinfo_vaddrconv(ctrclient *client, char *procname, unsigned int address, unsigned int type, unsigned int *out)
{
	unsigned int header[6];

	memset(header, 0, 6*4);
	
	header[0] = 0xf0;
	header[1] = 0x10;

	strncpy((char*)&header[2], procname, 8);
	header[4] = address;
	header[5] = type;

	if(ctrclient_sendbuffer(client, header, 6*4)!=1)return 1;
	if(ctrclient_recvbuffer(client, header, 3*4)!=1)return 1;

	*out = header[2];

	return 0;
}

int cmd_getdebuginfoblk(ctrclient *client, unsigned int **buf, unsigned int *size)
{
	unsigned int header[2];

	memset(header, 0, 8);
	
	header[0] = 0x490;
	header[1] = 0x0;

	if(ctrclient_sendbuffer(client, header, 2*4)!=1)return 1;
	if(ctrclient_recvbuffer(client, header, 2*4)!=1)return 1;

	*size = header[1];

	if(*size == 0)return 0;

	*buf = (unsigned int*)malloc(*size);
	if(*buf == NULL)
	{
		printf("Failed to allocate memory for the debuginfo block.\n");
		return 2;
	}

	if(ctrclient_recvbuffer(client, *buf, *size)!=1)return 1;

	return 0;
}

int cmd_sendexceptionhandler_signal(ctrclient *client, int type)
{
	unsigned int header[3];

	memset(header, 0, 3*4);
	
	header[0] = 0x491;
	header[1] = 0x4;

	if(type==0)header[2] = 1;//continue
	if(type)header[2] = 0x4d524554;//"TERM", terminate

	if(ctrclient_sendbuffer(client, header, 3*4)!=1)return 1;
	if(ctrclient_recvbuffer(client, header, 2*4)!=1)return 1;

	return 0;
}

int cmd_pscryptaes(ctrclient *client, unsigned int algotype, unsigned int keytype, unsigned char *iv, unsigned int bufsize, unsigned char *buf, unsigned int *resultcode)
{
	unsigned int header[0x20>>2];

	header[0] = 0x856;
	header[1] = bufsize + 0x18;

	header[2] = algotype;
	header[3] = keytype;
	memcpy(&header[4], iv, 0x10);

	if(ctrclient_sendbuffer(client, header, 0x20)!=1)return 1;
	if(ctrclient_sendbuffer(client, buf, bufsize)!=1)return 1;
	if(ctrclient_recvbuffer(client, header, 0x20)!=1)return 1;
	if(ctrclient_recvbuffer(client, buf, bufsize)!=1)return 1;

	*resultcode = header[2];
	memcpy(iv, &header[3], 0x10);

	return 0;
}

int cmd_readctrcard(ctrclient *client, unsigned int sector, unsigned int sectorcount, unsigned char *buf, unsigned int *resultcode)
{
	unsigned int header[4];

	header[0] = 0xc3;
	header[1] = 0x8;
	header[2] = sector;
	header[3] = sectorcount;

	memset(buf, 0, sectorcount*0x200);

	if(ctrclient_sendbuffer(client, header, 0x10)!=1)return 1;
	if(ctrclient_recvbuffer(client, header, 0x8)!=1)return 1;

	if(header[1]<4)
	{
		printf("Invalid response payload size: 0x%x.\n", header[1]);
		return 1;
	}

	if(ctrclient_recvbuffer(client, &header[2], 4)!=1)return 1;

	if(header[1]>4)
	{
		if(header[1] != sectorcount*0x200 + 4)
		{
			printf("Invalid response payload size: 0x%x.\n", header[1]);
			return 1;
		}

		if(ctrclient_recvbuffer(client, buf, sectorcount*0x200)!=1)return 1;
	}

	*resultcode = header[2];

	return 0;
}

int parsecmd_inputhexdata(unsigned int *databuf, unsigned int size)
{
	unsigned int pos, pos2, sz;
	unsigned int tmpval;
	char *str;
	struct stat filestat;
	unsigned char *databuf8 = (unsigned char*)databuf;
	FILE *f;

	pos = 0;
	while(pos<size)
	{
		str = strtok(NULL, " ");

		if(str==NULL)
		{
			printf("Invalid input hex-data, or not enough input data specified.\n");
			return 4;
		}

		if(str[0]=='@')
		{
			if(stat(&str[1], &filestat)==-1)
			{
				printf("Failed to stat %s\n", &str[1]);
				return 2;
			}

			f = fopen(&str[1], "rb");
			if(f==NULL)
			{
				printf("Failed to open %s\n", &str[1]);
				return 2;
			}

			sz = filestat.st_size;
			if(sz > size - pos)sz = size - pos;

			if(fread(&databuf8[pos], 1, sz, f) != sz)
			{
				printf("Failed to read file.\n");
				fclose(f);
				return 2;
			}

			fclose(f);

			pos+= sz;

		}
		else if(strncmp(str, "0x", 2)!=0)
		{
			for(pos2=0; pos2<strlen(str); pos2+=2)
			{
				if(pos>=size)break;

				tmpval = 0;
				sscanf(&str[pos2], "%02x", &tmpval);
				databuf8[pos] = tmpval;

				pos++;
			}
		}
		else
		{
			sscanf(&str[2], "%x", &tmpval);
			memcpy(&databuf8[pos], &tmpval, 4);
			pos+= 4;
		}
	}

	return 0;
}
int parsecmd_writeoutput(unsigned int *databuf, unsigned int size)
{
	int flag;
	char *str;
	FILE *f;

	if(size<=4)printf("Output: 0x%x\n", databuf[0]);

	flag = 0;
	str = strtok(NULL, " ");
	if(str)
	{
		if(str[0]=='@')
		{
			flag = 1;
		}
	}

	if(!flag)
	{
		printf("Output-data hexdump:\n");
		hexdump(databuf, size);
	}
	else
	{
		printf("Writing data to output file... ");

		f = fopen(&str[1], "wb");
		if(f==NULL)
		{
			printf("Failed to open %s\n", &str[1]);
			free(databuf);
			return 2;
		}

		if(fwrite(databuf, 1, size, f) != size)
		{
			printf("Failed to write file.\n");
			free(databuf);
			return 2;
		}

		fclose(f);

		printf("Done.\n");
	}

	return 0;
}

int parsecmd_hexvalue(char *tokstr, char *delim, unsigned int *out)
{
	char *str;

	str = strtok(tokstr, delim);

	if(str==NULL)
	{
		printf("Invalid paramaters.\n");
		return 4;
	}

	if(strncmp(str, "0x", 2)!=0)
	{
		printf("Invalid hex input.\n");
		return 4;
	}
	sscanf(&str[2], "%x", out);

	return 0;
}

int parsecmd_memoryrw(ctrclient *client, char *customcmd)
{
	unsigned int type=0, in_address=0, mem_address=0, out=0;
	unsigned int arm11usr=0;
	unsigned int pos=0;
	int ret=0;
	unsigned int size, sz, chunksize;
	unsigned int clearval=0;
	unsigned int optype=0;
	unsigned char *databuf = NULL;
	unsigned long long val64=0;
	char *procname = NULL;
	char *str = NULL;
	char procnamebuf[8];

	memset(procnamebuf, 0, 8);

	if(strncmp(customcmd, "readmem", 7)==0)
	{
		optype = 0;
		pos+= 7;
	}
	else if(strncmp(customcmd, "writemem", 8)==0)
	{
		optype = 1;
		pos+= 8;
	}
	else if(strncmp(customcmd, "memset", 6)==0)
	{
		optype = 2;
		pos+= 6;
	}
	else
	{
		printf("Invalid memoryrw command-type.\n");
		return 4;
	}

	size = 0;

	if(optype<2)
	{
		if(strncmp(&customcmd[pos], "8", 1)==0)
		{
			size = 1;
			pos++;
		}
		else if(strncmp(&customcmd[pos], "16", 1)==0)
		{
			size = 2;
			pos+=2;
		}
		else if(strncmp(&customcmd[pos], "32", 1)==0)
		{
			size = 4;
			pos+=2;
		}
	}

	if(customcmd[pos]==':')
	{
		pos++;
		if(strncmp(&customcmd[pos], "11 ", 3)==0)
		{
			type = 0;
			pos+= 2;
		}
		else if(strncmp(&customcmd[pos], "11usr=", 6)==0)
		{
			arm11usr = 1;
			type = 1;
			pos+= 6;
		}
		else if(strncmp(&customcmd[pos], "9 ", 2)==0)
		{
			type = 1;
			pos++;
		}
		else if(strncmp(&customcmd[pos], "11kern", 6)==0)
		{
			type = 2;
			pos+= 6;
		}
		else
		{
			printf("Invalid memory access type.\n");
			return 4;
		}
	}
	else
	{
		printf("Specify a mem-access type.\n");
		return 4;
	}

	if(arm11usr)
	{
		procname = strtok(&customcmd[pos], " ");
		if(procname==NULL)
		{
			printf("Invalid input paramaters(procname).\n");
			return 4;
		}

		if(strncmp(procname, "0x", 2)!=0)
		{
			strncpy(procnamebuf, procname, 8);
		}
		else
		{
			sscanf(procname, "%llx", &val64);
			memcpy(procnamebuf, &val64, 8);
		}

		str = NULL;
	}
	else
	{
		str = &customcmd[pos];
	}

	ret = parsecmd_hexvalue(str, " ", &in_address);
	if(ret!=0)
	{
		printf("Invalid input address.\n");
		return ret;
	}

	if(optype==2)
	{
		ret = parsecmd_hexvalue(NULL, " ", &clearval);
		if(ret!=0)
		{
			printf("Invalid clear value.\n");
			return ret;
		}
	}

	if(size==0)
	{
		ret = parsecmd_hexvalue(NULL, " ", &size);
		if(ret!=0)
		{
			printf("Invalid input size.\n");
			return ret;
		}
	}

	if(optype<2)
	{
		sz = size;
		if(sz<4)sz = 4;
		databuf = (unsigned char*)malloc(sz);
		if(databuf==NULL)
		{
			printf("Failed to allocate databuf.\n");
			return 5;
		}
		memset(databuf, 0, sz);
	}

	if(optype==1)
	{
		ret = parsecmd_inputhexdata((unsigned int*)databuf, size);
		if(ret!=0)
		{
			free(databuf);
			return ret;
		}
	}

	pos = 0;
	while(pos < size)
	{
		chunksize = 0x1000 - (in_address & 0xfff);
		if(chunksize > (size - pos))chunksize = size - pos;

		mem_address = in_address;

		if(arm11usr)
		{
			out = 0;
			ret = cmd_getprocinfo_vaddrconv(client, procnamebuf, in_address, 0, &out);
			if(ret == 0 && (out==~0 || out==~1))
			{
				if(out==~0)printf("Failed to find the specified process.\n");
				if(out==~1)printf("The specified virtual memory is not mapped: 0x%08x.\n", in_address);

				ret = 7;
			}
			else
			{
				mem_address = out;
				printf("Using physical address: 0x%08x (in_address = 0x%08x)\n", mem_address, in_address);
			}
		}

		if(ret==0)
		{
			if(optype<2)
			{
				ret = cmd_memoryrw(client, mem_address, &databuf[pos], chunksize, optype, type);
			}
			else
			{
				ret = cmd_memset(client, mem_address, clearval, chunksize, type);
			}
		}

		if(ret!=0)break;

		pos+= chunksize;
		in_address+= chunksize;
	}

	if(ret==0 && optype==0)
	{
		ret = parsecmd_writeoutput((unsigned int*)databuf, size);
	}

	if(optype<2)free(databuf);

	return ret;
}

int parsecmd_directfilerw(ctrclient *client, char *customcmd)
{
	int ret=0;
	unsigned int header[8];
	unsigned int *archive_lowpath, *file_lowpath, *databuf = NULL;
	unsigned int archive_lowpathsz, file_lowpathsz, databufsz;
	unsigned int payloadsize = 6*4;

	ret = parsecmd_hexvalue(&customcmd[12], " ,", &header[2]);//archiveid
	if(ret!=0)return ret;

	ret = parsecmd_hexvalue(NULL, " ,", &header[3]);//archive_lowpathtype
	if(ret!=0)return ret;

	ret = parsecmd_hexvalue(NULL, " ,", &header[4]);//archive_lowpathsize
	if(ret!=0)return ret;

	ret = parsecmd_hexvalue(NULL, " ,", &header[5]);//file_lowpathtype
	if(ret!=0)return ret;

	ret = parsecmd_hexvalue(NULL, " ,", &header[6]);//file_lowpathsize
	if(ret!=0)return ret;

	ret = parsecmd_hexvalue(NULL, " ,", &header[7]);//openflags
	if(ret!=0)return ret;

	if(header[7] & 0x2)
	{
		ret = parsecmd_hexvalue(NULL, " ,", &databufsz);
		if(ret!=0)return ret;
	}

	archive_lowpathsz = (header[4]+3) & ~3;//archive_lowpathsize aligned to 4-bytes
	file_lowpathsz = (header[6]+3) & ~3;//file_lowpathsize aligned to 4-bytes
	payloadsize+= archive_lowpathsz;
	payloadsize+= file_lowpathsz;

	archive_lowpath = (unsigned int*)malloc(archive_lowpathsz);
	if(archive_lowpath==NULL)
	{
		printf("Failed to allocate memory for archive lowpath.\n");
		return 5;
	}
	memset(archive_lowpath, 0, archive_lowpathsz);

	file_lowpath = (unsigned int*)malloc(file_lowpathsz);
	if(file_lowpath==NULL)
	{
		printf("Failed to allocate memory for file lowpath.\n");
		free(archive_lowpath);
		return 5;
	}
	memset(file_lowpath, 0, file_lowpathsz);

	if(header[7] & 0x2)
	{
		databuf = (unsigned int*)malloc(databufsz);
		if(databuf==NULL)
		{
			printf("Failed to allocate memory for the input databuf.\n");
			free(archive_lowpath);
			free(file_lowpath);
			return 5;
		}
	}

	ret = parsecmd_inputhexdata(archive_lowpath, header[4]);
	if(ret!=0)
	{
		free(archive_lowpath);
		free(file_lowpath);
		if(header[7] & 0x2)free(databuf);
		return ret;
	}

	ret = parsecmd_inputhexdata(file_lowpath, header[6]);

	if(ret!=0)
	{
		free(archive_lowpath);
		free(file_lowpath);
		if(header[7] & 0x2)free(databuf);
		return ret;
	}

	if(header[7] & 0x2)
	{
		ret = parsecmd_inputhexdata(databuf, databufsz);

		if(ret!=0)
		{
			free(archive_lowpath);
			free(file_lowpath);
			free(databuf);
			return ret;
		}

		payloadsize+= databufsz;
	}

	header[0] = 0x880;
	header[1] = payloadsize;

	if(ctrclient_sendbuffer(client, header, 8*4)!=1)
	{
		free(archive_lowpath);
		free(file_lowpath);
		if(header[7] & 0x2)free(databuf);
		return 1;
	}

	if(ctrclient_sendbuffer(client, archive_lowpath, archive_lowpathsz)!=1)
	{
		free(archive_lowpath);
		free(file_lowpath);
		if(header[7] & 0x2)free(databuf);
		return 1;
	}

	if(ctrclient_sendbuffer(client, file_lowpath, file_lowpathsz)!=1)
	{
		free(archive_lowpath);
		free(file_lowpath);
		if(header[7] & 0x2)free(databuf);
		return 1;
	}

	free(archive_lowpath);
	free(file_lowpath);

	if(header[7] & 0x2)
	{
		if(ctrclient_sendbuffer(client, databuf, databufsz)!=1)
		{
			free(databuf);
			return 1;
		}

		free(databuf);
	}

	if(ctrclient_recvbuffer(client, header, 8)!=1)return 1;

	databufsz = header[1];

	if(databufsz)
	{
		databuf = (unsigned int*)malloc(databufsz);
		if(databuf==NULL)
		{
			printf("Failed to allocate memory for the output databuf.\n");
			return 5;
		}

		if(ctrclient_recvbuffer(client, databuf, databufsz)!=1)
		{
			free(databuf);
			return 1;
		}

		ret = parsecmd_writeoutput(databuf, databufsz);
		free(databuf);
	}

	return ret;
}

int parsecmd_getprocinfo(ctrclient *client, char *customcmd)
{
	char *str;
	char *procname;
	int ret=0;
	unsigned int address=0, type=0, out=0;

	if(customcmd[11]!=':')
	{
		printf("Invalid paramaters.\n");
		return 4;
	}

	if(strncmp(&customcmd[12], "addrconv", 8)==0)
	{
		type = 0;
	}
	else if(strncmp(&customcmd[12], "kprocess", 8)==0)
	{
		type = 1;
	}
	else if(strncmp(&customcmd[12], "mmutable", 8)==0)
	{
		type = 2;
	}
	else
	{
		printf("Invalid type.\n");
		return 4;
	}

	str = strtok(&customcmd[12+8], " ");
	if(str==NULL)
	{
		printf("Invalid paramaters.\n");
		return 4;
	}

	procname = str;

	if(type==0)
	{
		ret = parsecmd_hexvalue(NULL, " ", &address);
		if(ret!=0)return ret;
	}

	ret = cmd_getprocinfo_vaddrconv(client, procname, address, type, &out);

	if(ret==0)
	{
		if(out!=~0 && out!=~1)
		{
			printf("Output: 0x%08x\n", out);
		}
		else
		{
			if(out==~0)printf("Failed to find the specified process.\n");
			if(out==~1)printf("The specified virtual memory is not mapped.\n");
		}
	}

	return ret;
}

int parsecmd_getdebuginfoblk(ctrclient *client, char *customcmd)
{
	int ret=0;
	unsigned int *buf=NULL;
	unsigned int size=0;
	FILE *f;

	ret = cmd_getdebuginfoblk(client, &buf, &size);
	if(ret!=0)return ret;

	if(size==0)
	{
		if(customcmd)printf("No debuginfo block available.\n");
		return 2;
	}

	if(customcmd==NULL)
	{
		printf("\nCaught exception/debuginfo-block.\n");
	}

	printf("Writing response data to ctrclientyls8_debuginfotmp.bin...\n");

	f = fopen("ctrclientyls8_debuginfotmp.bin", "wb");
	if(f)
	{
		fwrite(buf, 1, size, f);
		fclose(f);
	}
	else
	{
		printf("Failed to open output tmp file for writing.\n");
		ret = 3;
	}

	free(buf);

	if(ret==0)
	{
		ret = system("3dshax_parsedebug ctrclientyls8_debuginfotmp.bin --hexdump");
	}

	return ret;
}

int parsecmd_readctrcard(ctrclient *client, char *customcmd)
{
	int ret=0;
	unsigned int resultcode = 0;
	unsigned int mediaimagesize = 0, used_imagesize = 0;
	unsigned int mediaunitsize = 0;
	unsigned int cardtype = 0;
	unsigned int saveaddr = 0;
	unsigned int pos=0, chunksize=0, tmp=0, cnt=0;

	unsigned int ncsdhdr[0x400>>2];
	unsigned char *ncsdhdr8 = (unsigned char*)ncsdhdr;
	unsigned int *buf = NULL;
	FILE *f;
	char *str;

	if((ret = cmd_readctrcard(client, 0, 2, (unsigned char*)ncsdhdr, &resultcode))!=0)return ret;
	if(resultcode!=0)
	{
		printf("Result-code: 0x%x\n", resultcode);
		return 0;
	}

	if(ncsdhdr[0x100>>2] != 0x4453434e)
	{
		printf("NCSD magic is invalid.\n");
		return 0;
	}

	mediaunitsize = 1<<(ncsdhdr8[0x188+6]+9);
	mediaimagesize = ncsdhdr[0x104>>2] * mediaunitsize;
	cardtype = ncsdhdr8[0x188+5];

	if(cardtype==2)saveaddr = ncsdhdr[0x200>>2] * mediaunitsize;
	used_imagesize = ncsdhdr[0x300>>2];

	printf("Media-unit size: 0x%x\n", mediaunitsize);
	printf("Total image byte-size: 0x%x(%u MiB)\n", mediaimagesize, mediaimagesize / 0x100000);
	printf("Card type: %x\n", cardtype);
	printf("Total used image byte-size: 0x%x(%u MiB)\n", used_imagesize, used_imagesize / 0x100000);
	if(saveaddr)printf("CARD2 Savedata byte-pos: 0x%x\n", saveaddr);

	printf("First 0x400-bytes of the image:\n");
	hexdump(ncsdhdr, 0x400);
	printf("\n");

	str = strtok(&customcmd[11], " ");
	if(str==NULL)
	{
		printf("Invalid paramaters, specify an output image path.\n");
		return 4;
	}

	f = fopen(str, "wb");
	if(f==NULL)
	{
		printf("Failed to open output file: %s\n", str);
		return 0;
	}

	chunksize = 0x100000;
	buf = (unsigned int*)malloc(chunksize);
	if(buf==NULL)
	{
		printf("Failed to allocate memory.\n");
		fclose(f);
		return 0;
	}
	memset(buf, 0, chunksize);
	//chunksize = 0x1000;

	printf("Writing first 0x4000-bytes of image...\n");

	memcpy(buf, ncsdhdr, 0x400);
	memset(&buf[0x1000>>2], 0xFF, 0x3000);

	tmp = fwrite(buf, 1, 0x4000, f);
	if(tmp!=0x4000)
	{
		printf("File write failed.\n");
		free(buf);
		fclose(f);
		return 0;
	}

	printf("Beginning main card dump...\n");

	cnt = 0;

	for(pos=0x4000; pos<used_imagesize; pos+=chunksize)
	{
		if(used_imagesize - pos < chunksize)chunksize = used_imagesize - pos;

		//pos = mediaimagesize;//saveaddr+0x1000;

		if(cnt==0)
		{
			if(pos>0x4000)printf("\r");
			printf("curpos = 0x%x, end = 0x%x ", pos, used_imagesize);
			fflush(stdout);
		}

		cnt++;
		if(cnt == (0x100000*4) / chunksize)cnt=0;

		memset(buf, 0, chunksize);

		resultcode = 0;
		if((ret = cmd_readctrcard(client, pos/mediaunitsize, chunksize/mediaunitsize, (unsigned char*)buf, &resultcode))!=0)return ret;
		if(resultcode!=0)
		{
			printf("\nResult-code: 0x%x\n", resultcode);
			break;
		}

		tmp = fwrite(buf, 1, chunksize, f);
		if(tmp!=chunksize)
		{
			printf("\nFile write failed.\n");
			break;
		}

		//break;
	}
	printf("\nDone.\n");

	free(buf);
	fclose(f);

	return 0;
}

int parse_customcmd(ctrclient *client, char *customcmd)
{
	int ret=0;
	unsigned int pos=0;
	char *str;
	FILE *f;
	unsigned char *inbuffer=NULL, *outbuf=NULL;
	unsigned int *outbuf32;
	unsigned int inbuffersize=0, outbufsize=0;
	unsigned int paramblock[16];
	struct stat filestat;
	unsigned int val=0;
	unsigned long long val64 = 0;
	unsigned int chunksize;

	memset(paramblock, 0, 16*4);

	if(strncmp(customcmd, "quit", 4)==0)
	{
		ret = 1;
	}
	else if(strncmp(customcmd, "installcia", 10)==0)
	{
		pos = 10;

		if(customcmd[pos]==':')
		{
			pos++;
			sscanf(&customcmd[pos], "%x", &paramblock[0]);
				
			pos++;
			if(customcmd[pos]==',')
			{
				pos++;
				sscanf(&customcmd[pos], "%x", &paramblock[1]);

				pos++;
			}
		}

		if(paramblock[0]!=0 && paramblock[0]!=1)
		{
			printf("Invalid mediatype.\n");
			ret = 4;
		}

		if(paramblock[1]!=0 && paramblock[1]!=1)
		{
			printf("Invalid dbselect.\n");
			ret = 4;
		}

		str = strtok(&customcmd[pos], " ");
		if(str==NULL)
		{
			printf("Specify a .cia file-path.\n");
			ret = 4;
		}

		if(stat(str, &filestat)==-1)
		{
			printf("Failed to stat input CIA file.\n");
			ret = 4;
		}
		else
		{
			inbuffersize = filestat.st_size;
		}

		if(ret==0)
		{
			inbuffer = (unsigned char*)malloc(inbuffersize);
			if(inbuffer==NULL)
			{
				printf("Failed to allocate buffer for the input CIA.\n");
				ret = 5;
			}
			else
			{
				f = fopen(str, "rb");
				if(f)
				{
					if(fread(inbuffer, 1, inbuffersize, f)!=inbuffersize)
					{
						printf("Failed to read the input CIA.\n");
						ret = 4;
					}
					fclose(f);
				}
				else
				{
					printf("Failed to open the input CIA file.\n");
					ret = 4;
				}
			}
		}

		if(ret==0)
		{
			str = strtok(NULL, " ");
			if(str)
			{
				if(strncmp(str, "0x", 2)!=0)
				{
					printf("Invalid hex input, for maxchunksize.\n");
					ret = 4;
				}
				else
				{
					sscanf(&str[2], "%x", &paramblock[2]);
				}
			}
		}

		if(ret==0)
		{
			printf("Sending CIA install command...\n");
			ret = cmd8c0_installcia(client, paramblock[0], paramblock[1], inbuffer, inbuffersize, paramblock[2]);
		}

		free(inbuffer);
	}
	else if(strncmp(customcmd, "deletetitle", 11)==0)
	{
		pos = 11;
		val64 = 0;

		if(customcmd[pos]==':')
		{
			pos++;
			sscanf(&customcmd[pos], "%x", &paramblock[0]);
			pos++;

			if(customcmd[pos]==',')
			{
				pos++;
				sscanf(&customcmd[pos], "%x", &paramblock[1]);

				pos++;
			}
		}
		else
		{
			ret = 4;
			printf("Specify the mediatype.\n");
		}

		if(ret==0)
		{
			str = strtok(&customcmd[pos], " ");
			if(str)
			{
				if(strncmp(str, "0x", 2)!=0)
				{
					printf("Invalid hex input, for titleID.\n");
					ret = 4;
				}
				else
				{
					sscanf(&str[2], "%llx", &val64);
				}
			}
			else
			{
				ret = 4;
				printf("Specify the titleID.\n");
			}
		}

		if(ret==0)ret = cmd_deletetitle(client, paramblock[0], paramblock[1], val64);
	}
	else if(strncmp(customcmd, "sendservicecmd", 14)==0)
	{
		pos = 14;

		if(customcmd[pos]==':')
		{
			pos++;
			sscanf(&customcmd[pos], "%x", &paramblock[1]);
			while(customcmd[pos]!=' ')pos++;
		}

		str = strtok(&customcmd[pos], " ");
		if(str==NULL)
		{
			ret = 4;
			printf("Invalid input paramaters.\n");
		}

		if(ret==0)
		{
			
			if(strncmp(str, "0x", 2)!=0)
			{
				printf("Invalid hex input, for handle.\n");
				ret = 4;
			}
			else
			{
				sscanf(&str[2], "%x", &paramblock[0]);
			}
		}

		if(ret==0)
		{
			ret = parsecmd_hexvalue(NULL, " ", &val);
			if(ret!=0)
			{
				printf("Invalid input for command header.\n");
			}
			else
			{
				inbuffersize = (val & 0x3f) + ((val & 0xfc0) >> 6);
				inbuffersize = (inbuffersize + 1) * 4;

				inbuffer = (unsigned char*)malloc(inbuffersize);
				if(inbuffer==NULL)
				{
					ret = 2;
					printf("Failed to allocate memory for the input cmd-data.\n");
				}
				else
				{
					memset(inbuffer, 0, inbuffersize);
					memcpy(inbuffer, &val, 4);

					if(inbuffersize>4)
					{
						ret = parsecmd_inputhexdata((unsigned int*)&inbuffer[4], inbuffersize-4);
						if(ret!=0)
						{
							printf("Invalid input cmd-data.\n");
						}
					}
				}
			}
		}

		if(ret==0)
		{
			ret = cmd_sendservicecmd(client, paramblock[0], paramblock[1], (unsigned int*)inbuffer, inbuffersize, (unsigned int**)&outbuf, &outbufsize);
			if(ret==0 && outbufsize)
			{
				ret = parsecmd_writeoutput((unsigned int*)outbuf, outbufsize);
				for(pos=0; pos<outbufsize; pos+=4)printf("[%d]=0x%x ", pos>>2, *((unsigned int*)&outbuf[pos]));
				printf("\n");
			}
		}

		free(inbuffer);
		free(outbuf);
	}
	else if(strncmp(customcmd, "getservhandle", 13)==0)
	{
		pos = 13;

		str = strtok(&customcmd[pos], " ");
		if(str==NULL)
		{
			ret = 4;
			printf("Invalid input paramaters.\n");
		}

		if(strlen(str)>8)
		{
			ret = 4;
			printf("Service string is too long.\n");
		}

		if(ret==0)
		{
			paramblock[0] = 0x00050100;
			strncpy((char*)&paramblock[1], str, 8);
			paramblock[3] = strlen(str);
			paramblock[4] = 0;

			ret = cmd_sendservicecmd(client, 0x0, 0x1, paramblock, 0x14, (unsigned int**)&outbuf, &outbufsize);
		}

		if(ret==0 && outbufsize)
		{
			outbuf32 = (unsigned int*)outbuf;

			printf("Result-code: 0x%x\n", outbuf32[1]);
			if(outbuf[1]==0)printf("Handle: 0x%x\n", outbuf32[3]);

			if(outbuf32[1]==0xd8e06406)
			{
				printf("The server process doesn't have access to the specified service(not found in the exheader serviceaccesscontrol).\n");
			}

			free(outbuf);
		}
	}
	else if(strncmp(customcmd, "pscryptaes", 10)==0)
	{
		pos = 10;

		ret = parsecmd_hexvalue(&customcmd[pos], " ", &inbuffersize);
		if(ret!=0 || inbuffersize<0x10)
		{
			if(ret==0)ret = 5;
			printf("Invalid size.\n");
		}

		if(ret==0)
		{
			ret = parsecmd_hexvalue(NULL, " ", &paramblock[0]);
			if(ret!=0)
			{
				printf("Invalid algo-type.\n");
			}
		}

		if(ret==0)
		{
			ret = parsecmd_hexvalue(NULL, " ", &paramblock[1]);
			if(ret!=0)
			{
				printf("Invalid key-type.\n");
			}
		}

		if(ret==0)
		{
			ret = parsecmd_inputhexdata(&paramblock[2], 0x10);
			if(ret!=0)
			{
				printf("Invalid input CTR.\n");
			}
		}

		if(ret==0)
		{
			inbuffer = (unsigned char*)malloc(inbuffersize);
			if(inbuffer==NULL)
			{
				printf("Failed to allocate memory for input buffer.\n");
				ret = 4;
			}
			else
			{
				ret = parsecmd_inputhexdata((unsigned int*)inbuffer, inbuffersize);
				if(ret!=0)
				{
					printf("Invalid input payload data.\n");
				}
			}
		}

		if(ret==0)
		{
			pos = 0;
			chunksize = 0x80000;//0x3FF000;
			while(pos < inbuffersize)
			{
				if(inbuffersize - pos < chunksize)chunksize = inbuffersize - pos;

				printf("Crypting data @ pos 0x%x chunksize 0x%x(totalsize=0x%x)...\n", pos, chunksize, inbuffersize);

				val = 0;
				ret = cmd_pscryptaes(client, paramblock[0], paramblock[1], (unsigned char*)&paramblock[2], chunksize, &inbuffer[pos], &val);

				if(ret!=0)break;

				if(val!=0)
				{
					printf("Result-code: 0x%x\n", val);
					ret = 7;
					break;
				}

				pos+= chunksize;
			}

			if(ret==0)ret = parsecmd_writeoutput((unsigned int*)inbuffer, inbuffersize);
		}
	}
	else if(strncmp(customcmd, "rawservercmd", 12)==0)
	{
		pos = 12;

		ret = parsecmd_hexvalue(&customcmd[pos], " ", &paramblock[0]);
		if(ret!=0)
		{
			printf("Invalid cmdid.\n");
		}

		if(ret==0)
		{
			ret = parsecmd_hexvalue(NULL, " ", &paramblock[1]);
			if(ret!=0)
			{
				printf("Invalid payload size.\n");
			}
		}

		if(ret==0 && paramblock[1])
		{
			inbuffer = (unsigned char*)malloc(paramblock[1]);
			if(inbuffer==NULL)
			{
				printf("Failed to allocate memory for input buffer.\n");
				ret = 4;
			}
			else
			{
				ret = parsecmd_inputhexdata((unsigned int*)inbuffer, paramblock[1]);
				if(ret!=0)
				{
					printf("Invalid input payload data.\n");
				}
			}
		}

		if(ret==0)
		{
			if(ctrclient_sendbuffer(client, paramblock, 8)!=1)ret = 1;
		}

		if(ret==0 && paramblock[1])
		{
			if(ctrclient_sendbuffer(client, inbuffer, paramblock[1])!=1)ret = 1;
		}

		if(paramblock[1])free(inbuffer);

		if(ret==0)
		{
			if(ctrclient_recvbuffer(client, paramblock, 8)!=1)ret = 1;
		}

		if(ret==0)
		{
			printf("Reply cmdID: 0x%x\n", paramblock[0]);
			printf("Reply payload size: 0x%x\n", paramblock[1]);

			if(paramblock[1])
			{
				outbuf = (unsigned char*)malloc(paramblock[1]);
				if(outbuf==NULL)
				{
					printf("Failed to allocate memory for output buffer.\n");
					ret = 1;
				}
				else
				{
					if(ctrclient_recvbuffer(client, outbuf, paramblock[1])!=1)ret = 1;

					if(ret==0)ret = parsecmd_writeoutput((unsigned int*)outbuf, paramblock[1]);

					free(outbuf);
				}
			}
		}
	}
	else if(strncmp(customcmd, "readctrcard", 11)==0)
	{
		ret = parsecmd_readctrcard(client, customcmd);
	}
	else if(strncmp(customcmd, "readmem", 7)==0 || strncmp(customcmd, "writemem", 8)==0 || strncmp(customcmd, "memset", 6)==0)
	{
		ret = parsecmd_memoryrw(client, customcmd);
	}
	else if(strncmp(customcmd, "directfilerw", 12)==0)
	{
		ret = parsecmd_directfilerw(client, customcmd);
	}
	else if(strncmp(customcmd, "getprocinfo", 11)==0)
	{
		ret = parsecmd_getprocinfo(client, customcmd);
	}
	else if(strncmp(customcmd, "getdebuginfoblk", 15)==0)
	{
		ret = parsecmd_getdebuginfoblk(client, customcmd);
	}
	else if(strncmp(customcmd, "continue", 8)==0)
	{
		ret = cmd_sendexceptionhandler_signal(client, 0);
		if(ret==0)printf("Successfully sent exception-handler signal.\n");
	}
	else if(strncmp(customcmd, "kill", 4)==0)
	{
		ret = cmd_sendexceptionhandler_signal(client, 1);
		if(ret==0)printf("Successfully sent exception-handler signal.\n");
	}
	else
	{
		printf("Invalid customcmd type.\n");
	}

	return ret;
}

int main(int argc, char *argv[])
{
	ctrclient client;
	unsigned char *inputbuffer = NULL;
	unsigned char *key = NULL, *keyX = NULL;
	unsigned char *ctr = NULL;
	unsigned char tmpkey[16];
	char serveradr[256];
	char customcmd[256];
	char outpath[256];

	int argi;
	int keytype = 0;
	unsigned int keyslot = 0;
	int cryptmode = 2;
	int brute=0;
	int ret;
	unsigned int tmpsize, inbufsize=0;
	int enable_shell = 0;
	int pos, pos2;

	#ifdef NIX
	char *str;
	unsigned int wakeup_seconds = 5;

	fd_set rfds;
        struct timeval tv;
	#endif

	memset(serveradr, 0, sizeof(serveradr));
	memset(customcmd, 0, sizeof(customcmd));
	memset(outpath, 0, sizeof(outpath));

	for(argi=1; argi<argc; argi++)
	{
		ret = 0;

		if(strncmp(argv[argi], "--key=", 6)==0)
		{
			tmpsize = 0x10;
			load_bindata(&argv[argi][6], (unsigned char**)&key, &tmpsize);
			keytype = 1;
		}

		if(strncmp(argv[argi], "--keyY=", 7)==0)
		{
			tmpsize = 0x10;
			load_bindata(&argv[argi][7], (unsigned char**)&key, &tmpsize);
			keytype = 2;
		}

		if(strncmp(argv[argi], "--keyX=", 7)==0)
		{
			tmpsize = 0x10;
			load_bindata(&argv[argi][7], (unsigned char**)&keyX, &tmpsize);
		}

		if(strncmp(argv[argi], "--keyslot=", 10)==0)sscanf(&argv[argi][10], "%x", &keyslot);

		if(strncmp(argv[argi], "--ctr=", 6)==0)
		{
			tmpsize = 0x10;
			ret = load_bindata(&argv[argi][6], (unsigned char**)&ctr, &tmpsize);
		}
		if(strncmp(argv[argi], "--iv=", 5)==0)
		{
			tmpsize = 0x10;
			ret = load_bindata(&argv[argi][5], (unsigned char**)&ctr, &tmpsize);
		}
		if(strncmp(argv[argi], "--input=", 8)==0)
		{
			ret = load_bindata(&argv[argi][8], &inputbuffer, &inbufsize);
		}
		if(strncmp(argv[argi], "--ctrcrypt", 10)==0)cryptmode = 2;
		if(strncmp(argv[argi], "--cbcdecrypt", 12)==0)cryptmode = 4;
		if(strncmp(argv[argi], "--cbcencrypt", 12)==0)cryptmode = 5;
		if(strncmp(argv[argi], "--cryptmode=", 12)==0)sscanf(&argv[argi][12], "%x", &cryptmode);

		if(strncmp(argv[argi], "--brute", 7)==0)brute = 1;

		if(strncmp(argv[argi], "--serveradr=", 12)==0)strncpy(serveradr, &argv[argi][12], 255);
		if(strncmp(argv[argi], "--outpath=", 10)==0)strncpy(outpath, &argv[argi][10], 255);

		if(strncmp(argv[argi], "--customcmd=", 12)==0)strncpy(customcmd, &argv[argi][12], 255);
		#ifdef NIX
		if(strncmp(argv[argi], "--shell", 7)==0)
		{
			enable_shell = 1;
			if(argv[argi][7] == '=')
			{
				sscanf(&argv[argi][8], "%u", &wakeup_seconds);
			}
		}
		#endif

		if(ret!=0)break;
	}

	if(serveradr[0]==0)
	{
		printf("Specify a serveradr.\n");
		return 1;
	}

	if(ret!=0)
	{
		if(inputbuffer)free(inputbuffer);
		if(key)free(key);
		if(keyX)free(keyX);
		if(ctr)free(ctr);
		return ret;
	}

	if(customcmd[0]==0)
	{
		if(inputbuffer==NULL)
		{
			inbufsize = 0x10;
			inputbuffer = (unsigned char*)malloc(inbufsize);
			if(inputbuffer==NULL)
			{
				printf("Failed to allocate memory for input buffer.\n");
				return 2;
			}
			memset(inputbuffer, 0, inbufsize);
		}

		if(key==NULL)
		{
			key = (unsigned char*)malloc(0x10);
			if(key==NULL)
			{
				printf("Failed to allocate memory for key buffer.\n");
				free(inputbuffer);
				return 2;
			}
			memset(key, 0, 0x10);
		}

		if(ctr==NULL)
		{
			ctr = (unsigned char*)malloc(0x10);
			if(ctr==NULL)
			{
				printf("Failed to allocate memory for CTR buffer.\n");
				free(inputbuffer);
				free(key);
				return 2;
			}
			memset(ctr, 0, 0x10);
		}
	}

	ctrclient_init();

	if (0 == ctrclient_connect(&client, serveradr, "8333"))
	{
		free(inputbuffer);
		return 1;
	}

	if(customcmd[0] || enable_shell)
	{
		ret = 0;

		if(!enable_shell)
		{
			ret = parse_customcmd(&client, customcmd);
		}
		#ifdef NIX
		else
		{
			printf("> ");
			fflush(stdout);
			while(1)
			{
				FD_ZERO(&rfds);
           			FD_SET(0, &rfds);

				tv.tv_sec = (time_t)wakeup_seconds;
          			tv.tv_usec = 0;

				ret = select(1, &rfds, NULL, NULL, &tv);
				if(ret==-1)
				{
					perror("select");
					break;
				}

				if(ret)
				{
					memset(customcmd, 0, sizeof(customcmd));

					if(fgets(customcmd, sizeof(customcmd)-1, stdin)==NULL)break;
					str = strchr(customcmd, 0x0a);
					if(str)*str = 0;

					ret = 0;
					if(customcmd[0])
					{
						ret = parse_customcmd(&client, customcmd);
						printf("\n");
						if(ret==1)break;
					}

					printf("> ");
					fflush(stdout);
				}
				else if(wakeup_seconds)//No input available within "wakeup_seconds".
				{
					ret = parsecmd_getdebuginfoblk(&client, NULL);
					if(ret==1)break;

					if(ret==0)
					{
						printf("\n> ");
						fflush(stdout);
					}
				}
			}
		}
		#endif

		ctrclient_disconnect(&client);
		free(inputbuffer);
		return ret;
	}
	
	if(keyX==NULL || brute==0)
	{
		ret = cryptdata(&client, keyslot, keytype, cryptmode, key, ctr, inputbuffer, inbufsize, outpath, keyX);
	}
	else
	{
		memset(tmpkey, 0, 16);

		for(pos=0; pos<16; pos++)
		{
			memcpy(tmpkey, keyX, 16);

			for(pos2=0; pos2<0x100; pos2++)
			{
				tmpkey[pos] = (unsigned char)pos2;
				ret = cryptdata(&client, keyslot, keytype, cryptmode, key, ctr, inputbuffer, inbufsize, outpath, tmpkey);
			}
		}
	}

	ctrclient_disconnect(&client);

	free(inputbuffer);

	return ret;
}

