#ifndef CTRGAMECARD_H
#define CTRGAMECARD_H

u32 ctrcard_cmdc6(u32 *outbuf);
u32 gamecard_readsectors(u32 *outbuf, u32 sector, u32 sectorcount);
void read_gamecard();

#endif
