#ifndef ARM9_NAND_H
#define ARM9_NAND_H

void dump_nandfile(char *path);
u32 nand_rwsector(u32 sector, u32 *outbuf, u32 sectorcount, u32 rw);
void dump_nandimage();

#endif
