#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

FILE *fdebuginfo;
int enable_hexdump = 0;
unsigned int debuginfo_pos;

char regnames[16][4] = {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "sl", "fp", "ip", "sp", "lr", "pc"};

void hexdump(void *ptr, int buflen)//This is based on code from ctrtool.
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

void parse_debuginfo_exception(unsigned int *debuginfo, uint32_t new, uint32_t formatversion)
{
	FILE *ftmp;
	int i;
	unsigned int regs[16];
	unsigned int cpsr;
	unsigned int exceptiontype;
	unsigned char *ptr8;
	unsigned int RUNNINGFWVER;
	unsigned int tmp;
	uint32_t zeros_x20[0x20];
	char processname[16];
	char str[256];

	memset(processname, 0, 16);
	strncpy(processname, (char*)&debuginfo[(0x50+4)>>2], 8);
	printf("Process name: %s\n", processname);

	memset(regs, 0, 16 * 4);
	for(i=0; i<13; i++)regs[i] = debuginfo[i];
	regs[13] = debuginfo[14];
	regs[14] = debuginfo[15];
	regs[15] = debuginfo[17];
	cpsr = debuginfo[19+1];
	exceptiontype = debuginfo[16];

	if((cpsr & 0x1f) != 0x10)regs[14] = debuginfo[18];

	if(exceptiontype==0)
	{
		printf("Undefined instruction\n");
	}
	else if(exceptiontype==1)
	{
		printf("Prefetch abort\n");
	}
	else if(exceptiontype==2)
	{
		printf("Data abort\n");
	}

	for(i=0; i<16; i+=4)
	{
		printf("%s = 0x%08x ", regnames[i], regs[i]);
		printf("%s = 0x%08x ", regnames[i+1], regs[i+1]);
		printf("%s = 0x%08x ", regnames[i+2], regs[i+2]);
		printf("%s = 0x%08x ", regnames[i+3], regs[i+3]);
		printf("\n");
	}

	printf("cpsr: 0x%08x\n", cpsr);

	if(strncmp(processname, "Process9", 8))//Only print these for ARM11 processes.
	{
		printf("Process MMU table ptr: 0x%08x\n", debuginfo[(0x58+4)>>2]);
		printf("DFSR: 0x%08x\n", debuginfo[(0x5c+4)>>2]);
		printf("IFSR: 0x%08x\n", debuginfo[(0x60+4)>>2]);
		printf("FAR: 0x%08x\n", debuginfo[(0x64+4)>>2]);

		if(new && formatversion>=1)
		{
			printf("WFAR: 0x%08x\n", debuginfo[(0x90+0x164 + 0x0)>>2]);
			printf("c13 contextID register: 0x%08x\n", debuginfo[(0x90+0x164 + 0x4)>>2]);
			printf("c13 threadID register: 0x%08x\n", debuginfo[(0x90+0x164 + 0x8)>>2]);
			printf("CPUID register: 0x%08x\n", debuginfo[(0x90+0x164 + 0xc)>>2]);
			printf("KThread kernel VA: 0x%08x\n", debuginfo[(0x90+0x164 + 0x10)>>2]);
			printf("KProcess kernel VA: 0x%08x\n", debuginfo[(0x90+0x164 + 0x14)>>2]);

			if(formatversion>=2)
			{
				tmp = debuginfo[(0x90+0x164 + 0x18)>>2];
				printf("DSCR: 0x%08x. ", tmp);

				tmp = (tmp & 0x3c) >> 2;
				printf("Method-of-entry: 0x%x, ", tmp);
				if(tmp==0)
				{
					printf("default setting.\n");
				}
				else
				{
					switch(tmp)
					{
						case 1:
							printf("breakpoint");
						break;

						case 2:
							printf("watchpoint");
						break;

						case 3:
							printf("bkpt instruction");
						break;

						case 4:
							printf("EDBGRQ signal");
						break;

						case 5:
							printf("vector catch");
						break;

						case 6:
							printf("data-side abort");
						break;

						case 7:
							printf("instruction-side abort");
						break;

						default:
							printf("invalid");
						break;
					}

					printf(".\n");
				}
			}
		}
	}

	if(enable_hexdump)
	{
		memset(zeros_x20, 0, 0x20);

		if(memcmp(&debuginfo[(0x68+4)>>2], zeros_x20, 0x20)!=0)
		{
			ftmp = fopen("3dshaxparsedebug_tmpcode.bin", "wb");

			if(ftmp)
			{
				fwrite(&debuginfo[(0x68+4)>>2], 1, 0x20, ftmp);
				fclose(ftmp);

				snprintf(str, 255, "arm-none-eabi-objdump -D -b binary -m arm --adjust-vma=0x%0x", regs[15]-0x10);

				if(cpsr & 0x20)strncat(str, " -M force-thumb", 255);

				strncat(str, " 3dshaxparsedebug_tmpcode.bin", 255);

				printf("\nDisassembly from pc-0x10(0x%08x):\n", regs[15]-0x10);
				system(str);
				printf("\n");

				unlink("3dshaxparsedebug_tmpcode.bin");
			}
			else
			{
				printf("Failed to open file for writing: 3dshaxparsedebug_tmpcode.bin\n");
			}
		}
		else
		{
			printf("Skippig disasm since the dumped code is all-zero(probably due to inaccessible memory / prefetch abort).\n");
		}

		RUNNINGFWVER = debuginfo[(0x8c)>>2];
		printf("RUNNINGFWVER = 0x%08x. Hardware type = %s. FIRM tidlow u16 = 0x%x. Actual FWVER = %u/0x%x.\n", RUNNINGFWVER, RUNNINGFWVER & 0x40000000 ? "New3DS":"Old3DS", (RUNNINGFWVER>>8) & 0xffff, RUNNINGFWVER & 0xff, RUNNINGFWVER & 0xff);

		tmp = debuginfo[(0x210)>>2];
		if(tmp == 0x38)
		{
			printf("Stack dump:\n");
		}
		else
		{
			printf("Memory dump from ");
			if(tmp < 13*4)
			{
				printf("%s", regnames[tmp>>2]);
			}
			else
			{
				if(tmp == 14*4)
				{
					printf("%s", regnames[13]);
				}
				else if(tmp == 15*4)
				{
					printf("%s", regnames[14]);
				}
				else if(tmp == 17*4)
				{
					printf("%s", regnames[15]);
				}
				else if(tmp == 18*4)
				{
					printf("%s", regnames[14]);
				}
				else
				{
					printf("<invalid debuginfo register offset>");
				}
			}
			printf(":\n");
		}
		hexdump(&debuginfo[(0x90)>>2], 0x164);
		printf("\n");
	}
}

void parse_debuginfo_command(unsigned int *debuginfo)
{
	unsigned int i, pos, src_wordsize, dst_wordsize, tmp_wordsize, bufpos;
	unsigned int cmd_wordpos, cmdreply_wordpos, base_wordpos;
	unsigned int type;
	unsigned int srcbufdata_size = 0;
	unsigned int dstbufdata_size = 0;
	unsigned int addpos = 0;
	unsigned char *srcbufdata = NULL;
	unsigned char *dstbufdata = NULL;
	char processname_src[16];
	char processname_dst[16];

	type = debuginfo[0];

	memset(processname_src, 0, 16);
	memset(processname_dst, 0, 16);
	strncpy(processname_src, (char*)&debuginfo[0x4>>2], 8);
	strncpy(processname_dst, (char*)&debuginfo[0xc>>2], 8);

	if(type==0)printf("%s->%s ", processname_src, processname_dst);
	if(type==1)printf("%s->%s ", processname_dst, processname_src);

	if(type==1)
	{
		cmd_wordpos = 5+0x40;
		cmdreply_wordpos = 5;
	}
	else
	{
		cmd_wordpos = 5;
		cmdreply_wordpos = 5+0x40;
	}

	printf("command: ");

	dst_wordsize = (debuginfo[cmd_wordpos] & 0x3f) + ((debuginfo[cmd_wordpos] & 0xfc0) >> 6) + 1;

	for(i=0; i<dst_wordsize; i++)
	{
		printf("[%u]=0x%x", i, debuginfo[cmd_wordpos+i]);
		if(i!=dst_wordsize-1)printf(", ");
	}
	printf("  ");

	if(type==1)printf("command reply: ");

	src_wordsize = (debuginfo[cmdreply_wordpos] & 0x3f) + ((debuginfo[cmdreply_wordpos] & 0xfc0) >> 6) + 1;

	if(type==1)
	{
		for(i=0; i<src_wordsize; i++)
		{
			printf("[%u]=0x%x", i, debuginfo[cmdreply_wordpos+i]);
			if(i!=src_wordsize-1)printf(", ");
		}
	}

	for(i=(0x214>>2); i<(0x314>>2); i+=2)
	{
		srcbufdata_size+= debuginfo[i];
	}

	if(type==1)
	{
		for(i=(0x314>>2); i<(0x414>>2); i+=2)
		{
			dstbufdata_size+= debuginfo[i];
		}
	}

	printf("\n");
	//printf("srcbufdata_size=0x%lx dstbufdata_size=0x%lx\n", srcbufdata_size, dstbufdata_size);

	if(srcbufdata_size==0 && dstbufdata_size==0)return;

	if(srcbufdata_size)srcbufdata = (unsigned char*)malloc(srcbufdata_size);
	if(dstbufdata_size)dstbufdata = (unsigned char*)malloc(dstbufdata_size);
	if((srcbufdata==NULL && srcbufdata_size) || (dstbufdata==NULL && dstbufdata_size))
	{
		if(srcbufdata)free(srcbufdata);
		if(dstbufdata)free(dstbufdata);
		printf("Failed to allocate memory for srcbufdata/dstbufdata.");
		return;
	}

	if(srcbufdata)memset(srcbufdata, 0, srcbufdata_size);
	if(dstbufdata)memset(dstbufdata, 0, dstbufdata_size);

	if(srcbufdata)fread(srcbufdata, 1, srcbufdata_size, fdebuginfo);
	if(dstbufdata)fread(dstbufdata, 1, dstbufdata_size, fdebuginfo);

	if(dstbufdata && type==1)
	{
		pos=0x314>>2;
		i = ((debuginfo[cmd_wordpos] & 0xfc0) >> 6) + 1;
		bufpos = 0;
		addpos = 0;

		for(; i<dst_wordsize; i+=2,pos+=2)
		{
			addpos = 0;

			if((debuginfo[cmd_wordpos+i] & 0xe) == 0)
			{
				addpos = debuginfo[cmd_wordpos+i] >> 26;
				continue;
			}
			printf("cmd [%u] = vaddr 0x%x / physaddr 0x%x filepos 0x%x size 0x%x", i, debuginfo[cmd_wordpos+i+1], debuginfo[pos+1], debuginfo_pos+srcbufdata_size+bufpos, debuginfo[pos]);
			if(enable_hexdump)printf(":");
			printf("\n");

			if(enable_hexdump)hexdump(&dstbufdata[bufpos], debuginfo[pos]);

			bufpos+= debuginfo[pos];
		}

		for(; bufpos < dstbufdata_size && pos < (0x414>>2); pos+=2)
		{
			if(debuginfo[pos]==0 || debuginfo[pos+1]==0)continue;

			printf("cmd additional_buf[%u] = physaddr 0x%x filepos 0x%x size 0x%x", pos, debuginfo[pos+1], debuginfo_pos+srcbufdata_size+bufpos, debuginfo[pos]);
			if(enable_hexdump)printf(":");
			printf("\n");

			if(enable_hexdump)hexdump(&dstbufdata[bufpos], debuginfo[pos]);

			bufpos+= debuginfo[pos];
		}
	}

	if(srcbufdata)
	{
		if(type==1)
		{
			base_wordpos = cmdreply_wordpos;
			tmp_wordsize = src_wordsize;
		}
		else if(type==0)
		{
			base_wordpos = cmd_wordpos;
			tmp_wordsize = dst_wordsize;
		}

		pos=0x214>>2;
		i = ((debuginfo[base_wordpos] & 0xfc0) >> 6) + 1;
		bufpos = 0;
		addpos = 0;

		for(; i<tmp_wordsize; i+=2,pos+=2)
		{
			addpos = 0;

			if((debuginfo[base_wordpos+i] & 0xe) == 0)
			{
				addpos = debuginfo[base_wordpos+i] >> 26;
				continue;
			}
			printf("%s [%u] = vaddr 0x%x / physaddr 0x%x filepos 0x%x size 0x%x", type==1?"cmdreply":"cmd", i, debuginfo[base_wordpos+i+1], debuginfo[pos+1], debuginfo_pos+bufpos, debuginfo[pos]);
			if(enable_hexdump)printf(":");
			printf("\n");

			if(enable_hexdump)hexdump(&srcbufdata[bufpos], debuginfo[pos]);

			bufpos+= debuginfo[pos];
		}

		for(; bufpos < srcbufdata_size && pos < (0x314>>2); pos+=2)
		{
			if(debuginfo[pos]==0 || debuginfo[pos+1]==0)continue;

			printf("%s additional_buf[%u] = physaddr 0x%x filepos 0x%x size 0x%x", type==1?"cmdreply":"cmd", pos, debuginfo[pos+1], debuginfo_pos+bufpos, debuginfo[pos]);
			if(enable_hexdump)printf(":");
			printf("\n");

			if(enable_hexdump)hexdump(&srcbufdata[bufpos], debuginfo[pos]);

			bufpos+= debuginfo[pos];
		}
	}

	debuginfo_pos+= srcbufdata_size + dstbufdata_size;

	if(srcbufdata)free(srcbufdata);
	if(dstbufdata)free(dstbufdata);
}

FILE *fvid = NULL;

void parse_debuginfo_gxcmd3(unsigned int *debuginfo)
{
	unsigned char *buf;

	printf("gxcmd3\n");

	buf = (unsigned char*)malloc(debuginfo[5]);
	memset(buf, 0, debuginfo[5]);

	fread(buf, 1, debuginfo[5], fdebuginfo);
	debuginfo_pos+=debuginfo[5];

	if(fvid==NULL)fvid = fopen("videodump.bin", "wb");
	if(debuginfo[0] == 0x1f000000)fwrite(buf, 1, debuginfo[5], fvid);

	free(buf);
}

int main(int argc, char **argv)
{
	unsigned int *debuginfo_ptr = NULL;
	unsigned int debuginfo[0x200>>2];
	unsigned int debuginfo_size;
	uint32_t formatversion, parser_formatversion = 0x3;
	struct stat filestat;

	if(argc<2)return 0;

	if(stat(argv[1], &filestat)==-1)return 0;

	if(argc>2)
	{
		if(strncmp(argv[2], "--hexdump", 9)==0)enable_hexdump = 1;
	}

	debuginfo_size = (unsigned long)filestat.st_size;
	if(debuginfo_size<0x200)
	{
		printf("Debuginfo size is invalid: 0x%x\n", debuginfo_size);
		return 2;
	}

	memset(debuginfo, 0, 0x200);

	fdebuginfo = fopen(argv[1], "rb");
	if(fdebuginfo==NULL)return 2;
	
	debuginfo_pos = 0;
	while(debuginfo_pos < debuginfo_size)
	{
		fread(debuginfo, 1, 12, fdebuginfo);

		if(debuginfo[0] != 0x58584148)
		{
			printf("Invalid magic number.\n");
			fclose(fdebuginfo);
			return 1;
		}

		debuginfo_ptr = (unsigned int*)malloc(debuginfo[2]);
		if(debuginfo_ptr==NULL)
		{
			printf("Failed to allocate debuginfo_buf with size 0x%x.\n", debuginfo[2]);
			fclose(fdebuginfo);
			return 1;
		}
		memcpy(debuginfo_ptr, debuginfo, 12);
		fread(&debuginfo_ptr[12>>2], 1, debuginfo[2]-12, fdebuginfo);

		printf("pos 0x%x: ", debuginfo_pos);
		debuginfo_pos+= debuginfo[2];

		fseek(fdebuginfo, debuginfo_pos, SEEK_SET);

		if(debuginfo[1]==0x47424445)
		{
			printf("Exception(old format)\n");
			parse_debuginfo_exception(&debuginfo_ptr[3], 0, 0);
		}
		else if(debuginfo[1]==0x4e474445)
		{
			formatversion = debuginfo_ptr[3];

			printf("Exception(new format, version=0x%x)\n", formatversion);
			if(formatversion > parser_formatversion)
			{
				printf("The exception dump format version(0x%x) is newer than the version supported by this parser(0x%x). Skipping parsing.\n", formatversion, parser_formatversion);
			}
			parse_debuginfo_exception(&debuginfo_ptr[4], 1, formatversion);
		}
		else if(debuginfo[1]==0x444d4344)
		{
			parse_debuginfo_command(&debuginfo_ptr[3]);
		}
		else if(debuginfo[1]==0x33435847)
		{
			parse_debuginfo_gxcmd3(&debuginfo_ptr[3]);
		}
		else
		{
			printf("Skipping unknown debuginfo with type 0x%08x\n", debuginfo[1]);

			debuginfo_pos+= debuginfo[2];

			/*printf("Old debuginfo(exception) pos 0x%lx:\n", debuginfo_pos);
			debuginfo_pos+= 0x200;

			parse_debuginfo_exception(&debuginfo[1]);
			printf("\n");*/
		}

		fseek(fdebuginfo, debuginfo_pos, SEEK_SET);

		free(debuginfo_ptr);
	}

	fclose(fdebuginfo);
	if(fvid)fclose(fvid);

	return 0;
}

