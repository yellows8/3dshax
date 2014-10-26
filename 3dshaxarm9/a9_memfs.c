#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <nds/ndstypes.h>

#include "arm9_svc.h"
#include "arm9fs.h"

extern u32 RUNNINGFWVER;

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
	closefile(fileobj);
}*/

void dump_fcramaxiwram()
{
	u32 size;
	u32 pos=0;
	u16 filepath[64];
	u32 *fileobj = NULL;
	char *path = (char*)"/3dshax_axiwram.bin";

	memset(filepath, 0, 64*2);
	for(pos=0; pos<strlen(path); pos++)filepath[pos] = (u16)path[pos];

	size = 0x08000000;

	if(RUNNINGFWVER & 0x40000000)size<<=1;

	dumpmem((u32*)0x20000000, size);

	if(openfile(sdarchive_obj, 4, filepath, (strlen(path)+1)*2, 7, &fileobj)!=0)return;
	if(filewrite(fileobj, (u32*)0x1FF80000, 0x80000, 0)!=0)return;
	closefile(fileobj);
}

void loadfile_charpath(char *path, u32 *addr, u32 maxsize)
{
	u32 ret=0;
	u32 pos=0;
	u32 filesize = 0;
	u16 filepath[256];
	u32 *fileobj = NULL;

	memset(filepath, 0, 256*2);
	for(pos=0; pos<strlen(path); pos++)filepath[pos] = (u16)path[pos];

	if((ret = openfile(sdarchive_obj, 4, filepath, (strlen(path)+1)*2, 1, &fileobj))!=0)
	{
		return;
	}

	filesize = getfilesize(fileobj);
	if(filesize > maxsize)filesize = maxsize;

	if((ret = fileread(fileobj, addr, filesize, 0))!=0)
	{
		return;
	}

	closefile(fileobj);

	svcFlushProcessDataCache(addr, filesize);
}

