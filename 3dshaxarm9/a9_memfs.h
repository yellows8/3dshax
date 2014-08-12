#ifndef A9MEMFS_H
#define A9MEMFS_H

void dump_fcramvram();
void dump_fcramaxiwram();
void loadrun_file(char *path, u32 *addr, int execute);

#endif
