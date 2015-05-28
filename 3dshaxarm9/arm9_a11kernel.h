#ifndef ARM9_A11KERNEL_H
#define ARM9_A11KERNEL_H

u32 *get_kprocessptr(u64 procname, u32 num, u32 get_mmutableptr);
u8 *get_kprocess_contextid(u32 *kprocess);
u8 *mmutable_convert_vaddr2physaddr(u32 *mmutable, u32 vaddr);
u32 *patch_mmutables(u64 procname, u32 patch_permissions, u32 num);
void writepatch_arm11kernel_kernelpanicbkpt(u32 *ptr, u32 size);

#endif
