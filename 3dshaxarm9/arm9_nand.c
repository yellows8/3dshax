#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <nds/ndstypes.h>

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

#ifdef ENABLE_DUMP_NANDIMAGE
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
#endif

