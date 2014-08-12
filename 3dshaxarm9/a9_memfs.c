#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <nds/ndstypes.h>

void svcFlushProcessDataCache(u32*, u32);

u32 dumpmem(u32 *addr, u32 size);
u32 loadfile(u32 *addr, u32 size, u16 *path, u32 pathsize);
u32 openfile(u32 *archiveobj, u32 lowpathtype, void* path, u32 pathsize, u32 openflags, u32 **fileobj);
u32 fileread(u32 *fileobj, u32 *buf, u32 size, u32 filepos);
u32 filewrite(u32 *fileobj, u32 *buf, u32 size, u32 filepos);
u32 getfilesize(u32 *fileobj);
u32 archive_readsectors(u32 *archiveobj, u32 *buf, u32 sectorcount, u32 mediaoffset);
u32 pxifs_openarchive(u32 **archiveobj, u32 archiveid, u32 *lowpath);//lowpath is a ptr to the following structure: +0 = lowpathtype, +4 = dataptr, +8 = datasize

extern u32 *pxifs_state;
extern u32 *sdarchive_obj;
extern u32 *nandarchive_obj;

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

