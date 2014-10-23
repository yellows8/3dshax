#ifndef ARM9FS_H
#define ARM9FS_H

u32 dumpmem(u32 *addr, u32 size);
u32 loadfile(u32 *addr, u32 size, u16 *path, u32 pathsize);
u32 openfile(u32 *archiveobj, u32 lowpathtype, void* path, u32 pathsize, u32 openflags, u32 **fileobj);
u32 closefile(u32 *fileobj);
u32 fileread(u32 *fileobj, u32 *buf, u32 size, u32 filepos);
u32 filewrite(u32 *fileobj, u32 *buf, u32 size, u32 filepos);
u32 getfilesize(u32 *fileobj);
u32 archive_readsectors(u32 *archiveobj, u32 *buf, u32 sectorcount, u32 mediaoffset);
u32 pxifs_openarchive(u32 **archiveobj, u32 archiveid, u32 *lowpath);//lowpath is a ptr to the following structure: +0 = lowpathtype, +4 = dataptr, +8 = datasize

extern u32 *pxifs_state;
extern u32 *sdarchive_obj;
extern u32 *nandarchive_obj;
extern u16 input_filepath[];
extern u16 dump_filepath[];

#endif

