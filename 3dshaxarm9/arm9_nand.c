#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <3ds.h>

#include "arm9fs.h"

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

u32 nand_rwsector(u32 sector, u32 *buf, u32 sectorcount, u32 rw)
{
	u32 val;
	u32 *state;

	val = ((u32)pxifs_state) - 8;
	state = (u32*)(*((u32*)val));

	return archive_rwsectors(state, buf, sectorcount, sector, rw);
}

#ifdef ENABLE_DUMP_NANDIMAGE
void dump_nandimage()
{
	u32 ret = 0;
	u32 pos = 0, sectorbase = 0, sectori = 0;
	u32 *fileobj;
	u32 size;
	u16 filepath[256];

	char *path0 = "/3dshax_nandimage.bin";

	size = (*((u32*)0x01ffcdc4)) * 0x200;

	memset(filepath, 0, 256*2);
	for(pos=0; pos<strlen(path0); pos++)filepath[pos] = (u16)path0[pos];
	if((ret = openfile(sdarchive_obj, 4, filepath, (strlen(path0)+1)*2, 6, &fileobj))!=0)return;

	setfilesize(fileobj, size);

	for(pos=0; pos<size; pos+=0x100000)
	{
		sectorbase = pos/0x200;

		for(sectori=0; sectori<(0x100000/0x200); sectori+=8)
		{
			ret = nand_rwsector(sectorbase + sectori, ((u32*)(0x21000000 + sectori*0x200)), 8, 0);
			if(ret!=0)
			{
				closefile(fileobj);
				return;
			}
		}

		if(filewrite(fileobj, (u32*)0x21000000, 0x100000, pos)!=0)break;
	}

	closefile(fileobj);
}
#endif

