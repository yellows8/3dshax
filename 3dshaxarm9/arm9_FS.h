#ifndef ARM9_FS_H
#define ARM9_FS_H

int arm9_fopen(u32 *handle, u8 *path, u32 openflags);
int arm9_fclose(u32 *handle);
int arm9_fwrite(u32 *handle, u32 *bytes_written, u32 *buf, u32 size, u32 flushflag);
int arm9_fread(u32 *handle, u32 *bytes_read, u32 *buf, u32 size);
int arm9_GetFSize(u32 *handleptr, u64 *size);

#endif
