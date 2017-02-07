.arm

.arch armv5te
.fpu softvfp

.global _start

.global changempu_memregions
.global arm11_stub
.global pxidev_cmdhandler_cmd0
.global mountcontent_nandsd_writehookstub
.global arm9_pxipmcmd1_getexhdr_writepatch
.global arm9general_debughook_writepatch
.global proc9_waitfirmevent_hook
.global proc9_waitfirmevent_hook_patchaddr
.global proc9_autolocate_hookpatchaddr
.global proc9_autolocate_certsigcheck_patchaddr
.global proc9_autolocate_mainsigcheck_patchaddr
.global generate_branch
.global parse_branch
.global parse_branch_thumb
.global parse_armblx

.global new3ds_hookloader_entrypoint

.global call_arbitaryfuncptr

.global pxifs_state
.global sdarchive_obj
.global nandarchive_obj
.global input_filepath
.global dump_filepath
.global FIRMLAUNCH_RUNNINGTYPE
.global RUNNINGFWVER
.global FIRMLAUNCH_FWVER
.global NANDREDIR_SECTORNUM
.global FIRMLAUNCH_CLEARPARAMS
.global arm9_rsaengine_txtwrite_hooksz

.global proc9_textstartaddr

.global FIRM_contentid_versions
.global FIRM_sigword0_array
.global FIRM_contentid_totalversions
.global NEW3DS_FIRM_versions
.global NEW3DS_sigword0_array
.global NEW3DS_totalversions

.type changempu_memregions STT_FUNC
.type mountcontent_nandsd_writehookstub STT_FUNC
.type arm9_pxipmcmd1_getexhdr_writepatch STT_FUNC
.type arm9general_debughook_writepatch STT_FUNC
.type proc9_waitfirmevent_hook STT_FUNC
.type proc9_autolocate_hookpatchaddr STT_FUNC
.type proc9_autolocate_certsigcheck_patchaddr STT_FUNC
.type proc9_autolocate_mainsigcheck_patchaddr STT_FUNC
.type generate_branch STT_FUNC
.type parse_branch STT_FUNC
.type parse_armblx STT_FUNC
.type parse_branch_thumb STT_FUNC

.type new3ds_hookloader_entrypoint STT_FUNC

.type call_arbitaryfuncptr STT_FUNC

.section .init

_start:
.word __text_start + 4
b _start_codebegin

.word 0x4d415250 @ "PRAM", magic number for paramaters.

FIRMLAUNCH_RUNNINGTYPE: @ 0 = native(no FIRM-launch), 1 = FIRM-launch was done via the below code, 2 = _start was called via other means with a Process9 hook. 3 = _start was called via an ARM9 FIRM entrypoint hook, at entry LR *must* be set to the address of the original entrypoint from the FIRM header.
.word 0

RUNNINGFWVER: @ FWVER of the currently running system. Format: byte0 = KERNEL_MINORVERSION(from arm11kernel configmem), byte1-2 = FIRM tidlow u16, byte3 = flags. Bit31 in this u32 indicates a different FWVER format, used by FIRMLAUNCH_RUNNINGTYPE val3. Bit30 = hardware type: 0 = Old3DS, 1 = New3DS.
.word 34

FIRMLAUNCH_FWVER:
.word 0x0 @ The default value here doesn't really matter, since the firmlaunch code automatically determines what the FIRMLAUNCH_FWVER value is via the loaded FIRM. This is the FWVER of the FIRM being launched, same format as RUNNINGFWVER.

NANDREDIR_SECTORNUM:
#ifndef NANDREDIR_SECTORNUM_
#ifdef ENABLENANDREDIR
#error "NANDREDIR is enabled, but the SECTORNUM was not specified via Makefile parameter NANDREDIR_SECTORNUM."
#endif

.word 0
#else
.word NANDREDIR_SECTORNUM_
#endif

_start_codebegin:
ldr r0, FIRMLAUNCH_RUNNINGTYPE
cmp r0, #3
beq startcode_type3

push {r4, lr}
ldr r0, =initialize_proc9_textstartaddr
bl launchcode_kernelmode

/*cmp r0, #0
bne _start_code_fsinitdone*/
mov r0, #0
bl fs_initialize
cmp r0, #0
bne _start_codebegin_fail

//_start_code_fsinitdone:

/*ldr r0, =pxifs_state
ldr r1, =sdarchive_obj
str r5, [r0]
str r6, [r1]*/

/*ldr r4, =0x080cb320
mov r5, #0

closethreads:
ldr r0, [r4, r5, lsl #2]
svc 0x23
mov r0, #0
str r0, [r4, r5, lsl #2]
add r5, r5, #1
cmp r5, #3
blt closethreads*/

ldr r1, =__bss_start @ Clear .bss
ldr r2, =__bss_end
mov r3, #0

bss_clr:
cmp r1, r2
beq _start_done
str r3, [r1]
add r1, r1, #4
b bss_clr

_start_done:
ldr r0, =main
blx r0
//svc 0x7b
//bl main

//svc 0x9

pop {r4, pc}

_start_endlp:
b _start_endlp
.pool

_start_codebegin_fail:
.word 0xffffffff

startcode_type3:
@b startcode_type3
push {r4, lr}

#ifdef MEMDUMPBOOT_SRCADDR
ldr r0, =MEMDUMPBOOT_DSTADDR
ldr r1, [r0]
ldr r2, =0x504d5544
cmp r1, r2
beq startcode_type3_begin
str r2, [r0], #4

ldr r1, =MEMDUMPBOOT_SRCADDR
ldr r2, =MEMDUMPBOOT_SIZE

startcode_type3_dumpcpylp:
ldr r3, [r1], #4
str r3, [r0], #4
subs r2, r2, #4
bgt startcode_type3_dumpcpylp
#endif

startcode_type3_begin:
mov r4, #0

ldr r0, =RUNNINGFWVER
ldr r0, [r0]
lsr r1, r0, #31
cmp r1, #0
moveq r4, #1
beq startcode_type3_patchstart
mov r1, lr
bl parse_configmem_firmversion
cmp r0, #0
popne {r4, pc}

startcode_type3_patchstart:
mov r0, r4
bl startcode_type3_patchfirm

pop {r4, pc}
.pool

startcode_type3_patchfirm:
push {r4, r5, lr}
ldr r1, =FIRMLAUNCH_RUNNINGTYPE
ldr r1, [r1]
cmp r1, #3
movne r0, #1

mov r5, r0
cmp r5, #0
bne startcode_type3_patchfirm_patchstart

ldr r1, =FIRMLAUNCH_FWVER
ldr r0, =RUNNINGFWVER
ldr r4, [r1]
ldr r0, [r0]
str r0, [r1]

startcode_type3_patchfirm_patchstart:
ldr r0, =0x08006800 @ Address
ldr r1, =0x000F8800 @ Size
mov r3, #0
bl patchfirm_arm9section

ldr r0, =0x1FF80000
ldr r1, =0x80000
bl patchfirm_arm11section_kernel

cmp r5, #0
bne startcode_type3_patchfirm_end

ldr r0, =FIRMLAUNCH_FWVER
str r4, [r0]

startcode_type3_patchfirm_end:

#ifdef LOADA9_NEW3DSMEM
bl patchfirm_setup_tmpaddr_bincopy
#endif

pop {r4, r5, pc}
.pool

parse_configmem_firmversion:
push {r4, r5, lr}
mov r5, r1
and r3, r0, #0xff @ KERNEL_VERSIONMINOR
lsr r1, r0, #8 @ KERNEL_VERSIONMAJOR
lsr r2, r0, #16 @ KERNEL_SYSCOREVER
and r1, r1, #0xff
and r2, r2, #0xff
mov r4, r0
mov r0, #1

cmp r1, #2 @ Return error when KERNEL_VERSIONMAJOR!=2.
popne {r4, r5, pc}

lsr r4, r4, #30
and r4, r4, #1
cmp r4, #1
moveq r0, r1 @ KERNEL_VERSIONMAJOR
moveq r1, r2 @ KERNEL_SYSCOREVER
moveq r2, r3 @ KERNEL_VERSIONMINOR
moveq r3, r5
popeq {r4, r5, lr}
beq parse_configmem_firmversion_new3ds

ldr r1, =RUNNINGFWVER
lsl r2, r2, #8
orr r3, r3, r2
str r3, [r1]

mov r0, #0
pop {r4, r5, pc}
.pool

parse_configmem_firmversion_new3ds: @ r0=KERNEL_VERSIONMAJOR, r1=KERNEL_SYSCOREVER, r2=KERNEL_VERSIONMINOR, r3=original FIRM arm9 entrypoint
push {r4, r5, lr}
ldr r4, =RUNNINGFWVER
mov r5, #1
lsl r5, r5, #30
orr r5, r5, r2
lsl r1, r1, #8
orr r5, r5, r1
str r5, [r4] @ RUNNINGFWVER = 0x40000000 | KERNEL_VERSIONMINOR

ldr r0, =0x08006000
ldr r1, =0x080ff000
bl new3ds_hookloader_entrypoint

mov r0, #0
pop {r4, r5, pc}
.pool

new3ds_hookloader_entrypoint: @ Hook the new3ds entrypoint word in the arm9bin loader. r0=startaddr, r1=endaddr, r2=unused, r3=arm9 entrypoint from FIRM hdr
mov r5, r0

#ifdef NEW3DS_ARM9BINLDR_PATCHADDR0
ldr r2, =0x08006000
ldr r0, =NEW3DS_ARM9BINLDR_PATCHADDR0
sub r0, r0, r2
add r0, r0, r5
ldr r4, =NEW3DS_ARM9BINLDR_PATCHADDR0_VAL
str r4, [r0]
#endif

#ifdef NEW3DS_ARM9BINLDR_PATCHADDR1
ldr r2, =0x08006000
ldr r0, =NEW3DS_ARM9BINLDR_PATCHADDR1
sub r0, r0, r2
add r0, r0, r5
ldr r4, =NEW3DS_ARM9BINLDR_PATCHADDR1_VAL
str r4, [r0]
#endif

#ifdef NEW3DS_ARM9BINLDR_CLRMEM
ldr r2, =0x08006000
ldr r0, =NEW3DS_ARM9BINLDR_CLRMEM
ldr r6, =NEW3DS_ARM9BINLDR_CLRMEM_SIZE
sub r0, r0, r2
add r0, r0, r5
mov r4, #0

new3ds_hookloader_entrypoint_clrmemlp:
str r4, [r0], #4
subs r6, r6, #4
bgt new3ds_hookloader_entrypoint_clrmemlp
#endif

ldr r2, =0x0801B01C

#ifdef LOADA9_FCRAM
ldr r4, =0x08098000
#else
ldr r4, =0x08101000
#endif

new3ds_hookloader_entrypoint_lp:
ldr r0, [r3]
cmp r0, r2
beq new3ds_hookloader_entrypoint_finish
add r3, r3, #4
cmp r3, r1
bge new3ds_hookloader_entrypoint_end
b new3ds_hookloader_entrypoint_lp

new3ds_hookloader_entrypoint_finish:
str r0, new3ds_entrypointhookword
str r4, [r3]

ldr r2, =0xe12fff12 @ "bx r2"

new3ds_hookloader_entrypoint_lp2:
ldr r0, [r3]
cmp r0, r2
beq new3ds_hookloader_entrypoint_finish2
sub r3, r3, #4
cmp r3, r5
ble new3ds_hookloader_entrypoint_end
b new3ds_hookloader_entrypoint_lp2

new3ds_hookloader_entrypoint_finish2:
sub r3, r3, #8
mov r0, #0
str r0, [r3] @ Patch the branch executed when the plaintext binary entrypoint address is out-of-bounds.

adr r0, new3ds_plaintextbin_entrypoint_stubstart
adr r1, new3ds_plaintextbin_entrypoint_stubend

new3ds_hookloader_entrypoint_cpylp: @ No need to do anything with dcache since the arm9bin loader does that anyway.
ldr r3, [r0], #4
str r3, [r4], #4
cmp r0, r1
blt new3ds_hookloader_entrypoint_cpylp

new3ds_hookloader_entrypoint_end:
bx lr
.pool

new3ds_plaintextbin_entrypoint_stubstart:
#ifdef NEW3DS_MEMDUMPA9_ADR
#ifndef NEW3DS_MEMDUMPA9_SIZE
#error "The NEW3DS_MEMDUMPA9_SIZE option is required when using NEW3DS_MEMDUMPA9_ADR."
#endif
ldr r0, =new3ds_entrypointhookword
ldr r0, [r0]
mov lr, r0
b new3ds_dumpmema9
#else
ldr r0, =new3ds_entrypointhookword
ldr r0, [r0]
mov lr, r0
#ifndef LOADA9_FCRAM
mov r0, #0
ldr pc, =startcode_type3_patchfirm
#else
bx r0
#endif
#endif
.pool

#ifdef NEW3DS_MEMDUMPA9_ADR
new3ds_dumpmema9:
mrc 15, 0, r0, cr1, cr0, 0 @ Disable MPU.
bic r0, r0, #1
mcr 15, 0, r0, cr1, cr0, 0

ldr r1, =0x1FF80000 @ Patch the arm11kernel main() wfi loop, with the below arm11stub.
ldr r2, =0xe320f003 @ "wfi"
mov r3, #0

new3ds_dumpmema9_locatewfi:
ldr r0, [r1, r3]
cmp r0, r2
addne r3, r3, #4
bne new3ds_dumpmema9_locatewfi

sub r3, r3, #8
mov r0, r3
add r0, r0, r1

adr r2, new3ds_plaintextbin_entrypoint_stub_arm11stub
ldr r3, [r2, #0]
str r3, [r0, #0]
ldr r3, [r2, #4]
str r3, [r0, #4]
ldr r3, [r2, #8]
str r3, [r0, #8]

ldr r0, =0x2dc68
add r0, r0, r1
adr r1, new3ds_plaintextbin_entrypoint_stub_arm11code
adr r2, new3ds_plaintextbin_entrypoint_stub_arm11code_end

new3ds_dumpmema9_arm11codecpy:
ldr r3, [r1], #4
str r3, [r0], #4
cmp r1, r2
blt new3ds_dumpmema9_arm11codecpy

ldr r0, =0x18000000
#ifndef NEW3DS_MEMDUMPA9_DISABLEVRAMCLR
ldr r1, =0x600000
mov r2, #0
mov r3, r2

new3ds_dumpmema9_memclr:
str r3, [r0, r2]
add r2, r2, #4
subs r1, r1, #4
bgt new3ds_dumpmema9_memclr
#endif

ldr r1, =NEW3DS_MEMDUMPA9_ADR
ldr r2, =NEW3DS_MEMDUMPA9_SIZE

new3ds_dumpmema9_memcpy:
ldr r3, [r1], #4
str r3, [r0], #4
subs r2, r2, #4
bgt new3ds_dumpmema9_memcpy

mrc 15, 0, r0, cr1, cr0, 0 @ Enable MPU.
orr r0, r0, #1
mcr 15, 0, r0, cr1, cr0, 0
bx lr
.pool

new3ds_plaintextbin_entrypoint_stub_arm11stub:
ldr r0, =0xffff0c68
blx r0
.pool

new3ds_plaintextbin_entrypoint_stub_arm11code: @ Copy the entire VRAM to FCRAM+0x02001000.
mrc 15, 0, r0, cr0, cr0, 5
and r0, r0, #3
cmp r0, #0
bne new3ds_plaintextbin_entrypoint_stub_arm11code_lp

ldr r0, =0xd8000000
ldr r1, =0xe2001000
ldr r2, =0x600000

new3ds_plaintextbin_entrypoint_stub_arm11code_cpylp:
ldr r3, [r0], #4
str r3, [r1], #4
subs r2, r2, #4
bgt new3ds_plaintextbin_entrypoint_stub_arm11code_cpylp

mov r0, #0
mcr p15, 0, r0, c7, c14, 0 @ "Clean and Invalidate Entire Data Cache"
mcr p15, 0, r0, c7, c10, 5 @ "Data Memory Barrier"
mcr p15, 0, r0, c7, c10, 4 @ "Data Synchronization Barrier"

new3ds_plaintextbin_entrypoint_stub_arm11code_lp:
.word 0xe320f003 @ "wfi", use .word here instead of the actual instruction this .s is under armv5.
b new3ds_plaintextbin_entrypoint_stub_arm11code_lp
.pool

new3ds_plaintextbin_entrypoint_stub_arm11code_end:
.word 0
#endif

new3ds_plaintextbin_entrypoint_stubend:
.word 0

changempu_memregions:
mov r0, #0
mcr 15, 0, r0, cr6, cr4, 0
ldr r0, =0x10000037
mcr 15, 0, r0, cr6, cr3, 0 @ Disable region4 which starts at 0x10100000. Then set the size of region3 starting at 0x10000000 to 256MB, so that it covers 0x10000000 - 0x20000000.

mrc 15, 0, r0, cr6, cr6, 0
bic r0, r0, #1
mcr 15, 0, r0, cr6, cr6, 0
mrc 15, 0, r0, cr6, cr7, 0
bic r0, r0, #1
mcr 15, 0, r0, cr6, cr7, 0 @ Disable region6/region7 for the arm9 kernel region, so that user-mode can access the arm9 kernel region.
bx lr
.pool

initialize_proc9_textstartaddr:
push {r4, r5, r6, lr}
sub sp, sp, #0x20

ldr r0, =proc9_textstartaddr
mov r1, #0
str r1, [r0]

mrc 15, 0, r0, cr6, cr0, 0 @ Copy all memory protection base/size registers to stack.
str r0, [sp, #0x0]
mrc 15, 0, r0, cr6, cr1, 0
str r0, [sp, #0x4]
mrc 15, 0, r0, cr6, cr2, 0
str r0, [sp, #0x8]
mrc 15, 0, r0, cr6, cr3, 0
str r0, [sp, #0xC]
mrc 15, 0, r0, cr6, cr4, 0
str r0, [sp, #0x10]
mrc 15, 0, r0, cr6, cr5, 0
str r0, [sp, #0x14]
mrc 15, 0, r0, cr6, cr6, 0
str r0, [sp, #0x18]
mrc 15, 0, r0, cr6, cr7, 0
str r0, [sp, #0x1C]

mrc 15, 0, r6, c5, c0, 2 @ "read data access permission bits"

mov r1, #0
mov r4, #0
mvn r5, #0

initialize_proc9_textstartaddr_lp:
lsl r2, r1, #2
ldr r0, [sp, r2]
tst r0, #1
beq initialize_proc9_textstartaddr_lp_next @ Skip disabled mem-regions.

mov r2, r0

lsr r2, r2, #24
cmp r2, #0x08
bne initialize_proc9_textstartaddr_lp_next @ High-byte of mem-region base address must be 0x08.

mov r2, r1
lsl r2, r2, #2
mov r3, r6
lsr r3, r3, r2
and r3, r3, #0xf
cmp r3, #0x1 @ Check that the current mem-region has data permissions priv=RW/usr=none.
bne initialize_proc9_textstartaddr_lp_next

mov r4, #1
mov r5, r1

initialize_proc9_textstartaddr_lp_next:
add r1, r1, #1
cmp r1, #8
blt initialize_proc9_textstartaddr_lp

ldr r0, =0x01FFF470
mvn r2, #0
str r2, [r0]

cmp r4, #0
beq initialize_proc9_textstartaddr_end

ldr r0, =0x01FFF470
mvn r2, #1
str r2, [r0]

lsl r0, r5, #2
ldr r0, [sp, r0]
mov r2, r0

lsr r2, r2, #12
lsl r2, r2, #12
ldr r3, =0x08000000
cmp r2, r3
beq initialize_proc9_textstartaddr_end @ If the base address of the found mem-region is 0x08000000, then Process9 isn't running.

lsr r0, r0, #1 @ Get byte-size of mem-region.
and r0, r0, #0x1F
sub r0, r0, #0xB
ldr r1, =0x1000
lsl r1, r1, r0

add r2, r2, r1

ldr r0, =proc9_textstartaddr
str r2, [r0]

initialize_proc9_textstartaddr_end:
add sp, sp, #0x20
pop {r4, r5, r6, pc}
.pool

call_arbitaryfuncptr:
push {r4, r5, r6, r7, r8, lr}
mov r7, r0
mov r8, r1
ldm r8, {r0, r1, r2, r3, r4, r5, r6}
blx r7
stm r8, {r0, r1, r2, r3, r4, r5, r6}
pop {r4, r5, r6, r7, r8, pc}

pxidev_cmdhandler_cmd0: @ This is code which the pxidev cmdhandler will jump to for handling cmdid 0x0000, when that switch statement addr was patched.
mov r0, r4
bl pxidev_cmdhandler_cmd0handler
ldr r0, =RUNNINGFWVER
ldrb r0, [r0]
cmp r0, #34 @ v4.1
addeq sp, sp, #0x9c
popeq {r4, r5, r6, r7, r8, r9, sl, fp, pc} @ v4.1, this check needs updated so that other <=v4.1 versions are supported.
add sp, sp, #0x84
pop {r4, r5, r6, r7, r8, r9, sl, fp, pc}
.pool

/*mountcontent_nandsd_writehookstub:
ldr r0, =0x8033570//0x802e62c
adr r1, mountcontent_nandsd_hookstub
ldr r2, [r1]
str r2, [r0]
ldr r2, [r1, #4]
str r2, [r0, #4]
mov r1, #8
mov r2, r1
mov r1, r0
ldr r0, =0xffff8001
b svcFlushProcessDataCache
.pool

.thumb
mountcontent_nandsd_hookstub:
ldr r7, =mountcontent_nandsd_hook
bx r7
.pool
.arm

mountcontent_nandsd_hook:*/
/*ldr r7, [sp, #0x34]
add r5, sp, #0x2c
add r1, sp, #8
add r0, sp, #0x38*/
/*ldr	r3, [sp, #0x5c]
ldr	r7, [r1, #8]
ldr	r2, [sp, #0x50]
add	r1, sp, #32
b mountcontent_openfile_hook

ldr r2, [sp, #0x74]
ldr r3, =0x00040030
cmp r2, r3
bne mountcontent_nandsd_hook_end

push {r0, r1, lr}
ldr r0, =mountcontenthook_archivevtable
ldr r1, [r7]
mov r2, #0x100

mountcontent_nandsd_hook_cpylp:
ldr r3, [r1], #4
str r3, [r0], #4
subs r2, r2, #4
bgt mountcontent_nandsd_hook_cpylp

ldr r1, =mountcontenthook_archivevtable_openfilefuncptr
ldr r0, =mountcontenthook_archivevtable
ldr r2, [r0, #8]
str r2, [r1]
ldr r2, =mountcontent_openfile_hook
str r2, [r0, #8]
str r0, [r7]

pop {r0, r1, lr}

mountcontent_nandsd_hook_end:
ldr pc, =0x802e635
.pool

mountcontent_openfile_hook:
push {r0, r1, r2, r3, lr}
bl mountcontent_openfile_hook_opensd
cmp r0, #0
pop {r0, r1, r2, r3, lr}
moveq r0, #0
ldreq r1, =0x803357b*/
//bxeq r1//lr
/*ldr ip, =mountcontenthook_archivevtable_openfilefuncptr
ldr ip, [ip]
bx ip*/
//ldr pc, =0x8033579
//.pool

/*.thumb
.align 2
arm9_twlfirm_stub:
ldr r2, =arm9_twlfirm_code
blx r2
.pool

.arm
arm9_twlfirm_code:
ldr r0, =twlbootldrenc_arm9
ldr r6, =twlbootldrenc_arm9_end
sub r6, r6, r0
ldr r1, =0x27d00800

arm9_twlfirm_code_9cpy:
ldr r2, [r0], #4
str r2, [r1], #4
subs r6, r6, #4
bgt arm9_twlfirm_code_9cpy

ldr r0, =twlbootldrenc_arm7
ldr r6, =twlbootldrenc_arm7_end
sub r6, r6, r0
ldr r1, =0x27d0a800

arm9_twlfirm_code_7cpy:
ldr r2, [r0], #4
str r2, [r1], #4
subs r6, r6, #4
bgt arm9_twlfirm_code_7cpy

add r0, pc, #4
svc 0x7b
b arm9_twlfirm_code_end

mrc p15, 0, r0, c1, c0
bic r0, r0, #1
mcr p15, 0, r0, c1, c0*/

/*ldr r6, =0x1FFAC000
ldr r3, =0x320
mvn r2, #0
add r3, r3, r6
str r2, [r3] @ Write a thumb undef instruction to TwlBg procmem 0x00100320.*/

/*push {r6, lr}
mov r0, #0
//ldr r1, =0x1FFAB000
//ldr r1, =0x20000000
//ldr r1, =0x27E00000
ldr r1, =0x1FF90000
ldr r2, =0x10000
mov r3, #2
ldr r6, =cardWriteEeprom
blx r6
pop {r6, lr}

mrc p15, 0, r0, c1, c0
orr r0, r0, #1
mcr p15, 0, r0, c1, c0
bx lr

arm9_twlfirm_code_end:

ldrh r0, [r5, #0]
mov r6, #1
lsl r6, r6, #10
cmp r0, #0
ldr pc, =0x8032a39
.pool*/

//#define FIRMLAUNCH_ENABLEFIRM_MODULELOAD

proc9_waitfirmevent_hook:
push {r0, r1, r2, r3, r4, r5, r6, lr}

ldr r0, =FIRMLAUNCH_RUNNINGTYPE
ldr r1, =FIRMLAUNCH_FWVER
ldr r1, [r1]
ldr r2, [r0]
mov r3, #1
cmp r2, #3
moveq r3, #2
str r3, [r0]
ldr r3, =RUNNINGFWVER
strne r1, [r3]

ldr r0, =_start
add r0, r0, #4
blx r0

pop {r0, r1, r2, r3, r4, r5, r6, lr}
ldr r2, =proc9_waitfirmevent_hook_patchaddr
ldr r2, [r2]
add r2, r2, #8

ldr r0, [r2, #44]
ldr r1, [r2, #48]
bx r2
.pool

proc9_waitfirmevent_hook_fail:
.word 0xffffffff

proc9_waitfirmevent_hook_patchaddr:
.word 0

proc9_autolocate_hookpatchaddr: @ inr0 = Process9 .text addr, inr1 = addr of Process9 .code
push {r4, r5, lr}
mov r4, r0
mov r5, r1

add r0, r4, #0x14
ldr r1, [r5, #0x14]
bl parse_branch
sub r0, r0, r4
add r0, r0, r5 @ r0 = .code addr of Process9 main()

ldr r1, =0xe8bd

proc9_autolocate_hookpatchaddr_lp:
ldr r2, [r0]
lsr r3, r2, #16
cmp r3, r1
addne r0, r0, #4
bne proc9_autolocate_hookpatchaddr_lp @ r0 = Addr of the pop instruction in main().

lsr r2, r2, #12
ldr r1, =0xe8bd8
cmp r2, r1
subne r0, r0, #0x10 @ <v10.0 FIRM
subeq r0, r0, #0x28

ldr r1, [r0]
sub r0, r0, r5
add r0, r0, r4
bl parse_branch
sub r0, r0, r4
add r0, r0, r5 @ r0 = .code addr of Process9 init_pxi_threads() func

ldr r1, =0xe8bd8ff0

proc9_autolocate_hookpatchaddr_lp2:
ldr r2, [r0]
cmp r2, r1
addne r0, r0, #4
bne proc9_autolocate_hookpatchaddr_lp2 @ r0 = Addr of the pop instruction in init_pxi_threads().

sub r0, r0, #0x10

sub r0, r0, r5
add r0, r0, r4

pop {r4, r5, pc}
.pool

/*proc9_fsdeviceinit_hook:
push {r0, r1, r2, r3, lr}
mov r0, #1
bl fs_initialize*/
/*mvn r1, #0
cmp r0, r1
beq proc9_fsdeviceinit_hook_fail_lp*/
/*pop {r0, r1, r2, r3, lr}

#if FIRMLAUNCH_FWVER == 0x2E
mov r1, #0x23
#endif
lsl r1, r1, #7
mov r0, #0
add r7, r4, r1
#if FIRMLAUNCH_FWVER == 0x1F
str r0, [r7, #32]
#endif
add lr, lr, #4
bx lr
.pool

proc9_fsdeviceinit_hook_fail_lp:
b proc9_fsdeviceinit_hook_fail_lp*/

/*arm9_rsaengine_txtwrite_stub:
ldr r2, =arm9_rsaengine_txtwrite_hook
bx r2
.pool*/

arm9general_debugstub:
ldr pc, =arm9general_debughook
//blx ip
.pool

.thumb

#ifdef ENABLE_GETEXHDRHOOK
arm9_pxipmcmd1_getexhdr_stub:
ldr r1, =arm9_pxipmcmd1_getexhdr_hook
blx r1
.pool
#endif

/*proc9_fsdeviceinit_hookstub:
ldr r0, =proc9_fsdeviceinit_hook
blx r0
.pool*/

.arm

arm11_stub:
ldr pc, =0x00100000
.pool

/*arm9_rsaengine_txtwrite_hook:
push {r0, r1, r4, r5, r6, r7, lr}
mov r4, r0
add r0, r0, #7
asr r5, r0, #3

push {r0, r1, r2, r3, lr}
ldr r0, =0x01ffb700
adr r3, arm9_rsaengine_txtwrite_hooksz

ldr r1, [r3]
cmp r1, #0x100
bge arm9_rsaengine_txtwrite_hookend

add r0, r0, r1
mov r2, #8
add r1, r1, r2
str r1, [r3]
ldr r1, [sp, #4]
bl memcpy
arm9_rsaengine_txtwrite_hookend:
pop {r0, r1, r2, r3, lr}

ldr pc, =0x08081e19
.pool
arm9_rsaengine_txtwrite_hooksz:
.word 0*/

arm9general_debughook:
//cmp r7, r1
//ldrne pc, =0x807b4a5

//add lr, lr, #12
mov r0, #0x2f
mov r1, sp
push {r0, r1, r2, r3, r4, r5, r6, r7, r8, lr}

/*ldr r0, =0x20001000
str sp, [r0]
add r0, r0, #4
ldr r1, =0x08000000
add r1, r1, #4
ldr r2, =0x00100000
sub r2, r2, #4*/
ldr r0, =0x01ffd000
mov r1, sp
ldr r2, =0x1000
bl memcpy
/*ldr r0, =0x20001000
ldr r1, =0x00100000
bl dumpmem*/

pop {r0, r1, r2, r3, r4, r5, r6, r7, r8, lr}

/*ldr r0, =0x080cdff8
mov r1, sp
ldr lr, =0x0802ea99
ldr pc, =0x0804e2dd*/
/*movs r0, #0
pop {r3, r4, r5, r6, r7, pc}*/
ldr pc, =0x08085ee8//=0x08057301
.pool

#ifdef ENABLE_GETEXHDRHOOK
arm9_pxipmcmd1_getexhdr_hook:
cmp r0, #0
bge arm9_pxipmcmd1_getexhdr_hook_begin
ldr r1, =arm9_pxipmcmd1_getexhdr_hook_jumpaddr
ldr r1, [r1]
mov r0, sp
blx r1
add sp, #20
pop {r4, r5, r6, r7, pc}

arm9_pxipmcmd1_getexhdr_hook_begin:
push {r0, r1, r2, r3, r4, r5, lr}
ldr r0, [sp, #32]
bl pxipmcmd1_getexhdr

arm9_pxipmcmd1_getexhdr_hookend:
pop {r0, r1, r2, r3, r4, r5, lr}

add lr, lr, #0xa
bx lr
.pool

arm9_pxipmcmd1_getexhdr_hook_jumpaddr:
.word 0

arm9_pxipmcmd1_getexhdr_writepatch:
push {r4, lr}
mov r4, r0
add r0, r0, #4
mov r1, #0
bl parse_branch_thumb
ldr r1, =arm9_pxipmcmd1_getexhdr_hook_jumpaddr
str r0, [r1]
mov r0, r4
ldr r1, =arm9_pxipmcmd1_getexhdr_stub
ldr r2, [r1, #0]
str r2, [r0, #0]
ldr r2, [r1, #4]
str r2, [r0, #4]
mov r1, #8
pop {r4, lr}
mov r2, r1
mov r1, r0
ldr r0, =0xffff8001
b svcFlushProcessDataCache
.pool
#endif

arm9general_debughook_writepatch:
ldr r1, =arm9general_debugstub
ldr r2, [r1, #0]
str r2, [r0, #0]
ldr r2, [r1, #4]
str r2, [r0, #4]
/*ldr r2, [r1, #8]
str r2, [r0, #8]*/
mov r1, #8
mov r2, r1
mov r1, r0
ldr r0, =0xffff8001
b svcFlushProcessDataCache
.pool

proc9_autolocate_certsigcheck_patchaddr: @ inr0 = Process9 .text addr, inr1 = addr of Process9 .code, inr2 = .text size
push {r0, r1, r4, r5, r6, lr}
mov r6, r2
ldr r4, =0xfffff822
ldr r5, =0x0fffffff
ldr r2, =0x080ff000

proc9_autolocate_certsigcheck_patchaddr_lp:
ldr r3, [r1]
cmp r3, r4
bne proc9_autolocate_certsigcheck_patchaddr_lpnext
ldr r3, [r1, #4]
cmp r3, r5
bne proc9_autolocate_certsigcheck_patchaddr_lpnext

mov r0, r1
sub r0, r0, #8
ldr r1, [sp, #4]
ldr r2, [sp, #0]
sub r0, r0, r1
add r0, r0, r2
b proc9_autolocate_certsigcheck_patchaddr_end

proc9_autolocate_certsigcheck_patchaddr_lpnext:
add r1, r1, #4
subs r6, r6, #4
bgt proc9_autolocate_certsigcheck_patchaddr_lp
mov r0, #0

proc9_autolocate_certsigcheck_patchaddr_end:
add sp, sp, #8
pop {r4, r5, r6, pc}
.pool

proc9_autolocate_mainsigcheck_patchaddr: @ inr0 = Process9 .text addr, inr1 = addr of Process9 .code, inr2 = .text size
push {r0, r1, r4, r5, r6, lr}
mov r6, r2
ldr r4, =0x4d22b570
ldr r5, =0x6869000c
ldr r2, =0x080ff000

proc9_autolocate_mainsigcheck_patchaddr_lp:
ldr r3, [r1]
cmp r3, r4
bne proc9_autolocate_mainsigcheck_patchaddr_lpnext
ldr r3, [r1, #4]
cmp r3, r5
bne proc9_autolocate_mainsigcheck_patchaddr_lpnext

mov r0, r1
ldr r1, [sp, #4]
ldr r2, [sp, #0]
sub r0, r0, r1
add r0, r0, r2
b proc9_autolocate_mainsigcheck_patchaddr_end

proc9_autolocate_mainsigcheck_patchaddr_lpnext:
add r1, r1, #4
subs r6, r6, #4
bgt proc9_autolocate_mainsigcheck_patchaddr_lp
mov r0, #0

proc9_autolocate_mainsigcheck_patchaddr_end:
add sp, sp, #8
pop {r4, r5, r6, pc}
.pool

parse_branch: @ r0 = addr of branch instruction, r1 = branch instruction u32 value (ARM-mode)
cmp r1, #0
ldreq r1, [r0]
lsl r1, r1, #8
lsr r1, r1, #8
tst r1, #0x800000
moveq r2, #0
ldrne r2, =0xff000000
orr r2, r2, r1
lsl r2, r2, #2
add r0, r0, #8
add r0, r0, r2
bx lr

parse_armblx: @ r0 = addr of branch instruction, r1 = branch instruction u32 value (ARM-mode)
push {r0, r1, lr}
bl parse_branch
pop {r1, r2, lr}
cmp r2, #0
ldreq r2, [r1]
tst r2, #0x01000000
orrne r0, r0, #0x2
orr r0, r0, #1
bx lr

parse_branch_thumb: @ r0 = addr of branch instruction, r1 = branch instruction u32 value
cmp r1, #0
ldreqh r1, [r0]
ldreqh r2, [r0, #2]
lsleq r2, r2, #16
orreq r1, r1, r2
ldr r3, =0x7ff
and r2, r1, r3
lsl r2, r2, #12
lsr r1, r1, #16
and r1, r1, r3
lsl r1, r1, #1
orr r2, r2, r1
orr r2, r2, #1
tst r2, #0x400000
moveq r3, #0
ldrne r3, =0xff800000
orr r2, r2, r3

add r0, r0, #4
add r0, r0, r2
bx lr
.pool

generate_branch: @ r0 = addr of branch instruction, r1 = addr to branch to, r2 = 0 for regular branch, non-zero for bl. (ARM-mode)
add r0, r0, #8
sub r1, r1, r0
asr r1, r1, #2
tst r1, #0x20000000
lsl r1, r1, #9
lsr r1, r1, #9
orrne r1, #0x800000
cmp r2, #0
orreq r1, #0xea000000
orrne r1, #0xeb000000
mov r0, r1
bx lr

pxifs_state:
.word 0

sdarchive_obj:
.word 0

nandarchive_obj:
.word 0

dump_filepath:
.hword 0x2F, 0x33, 0x64, 0x73, 0x68, 0x61, 0x78, 0x5F, 0x64, 0x75, 0x6D, 0x70, 0x2E, 0x62, 0x69, 0x6E, 0x00 //UTF-16 "/3dshax_dump.bin"

.align 2

/*input_filepath:
.hword 0x2F, 0x33, 0x64, 0x73, 0x68, 0x61, 0x78, 0x5F, 0x69, 0x6E, 0x70, 0x75, 0x74, 0x2E, 0x62, 0x69, 0x6E, 0x00 //UTF-16 "/3dshax_input.bin"

.align 2

twlbootldr_filepath:
.hword 0x73, 0x70, 0x69, 0x63, 0x61, 0x72, 0x64, 0x3A, 0x00 //0x73, 0x64, 0x6D, 0x63, 0x3A, 0x2F, 0x74, 0x77, 0x6C, 0x62, 0x6F, 0x6F, 0x74, 0x6C, 0x64, 0x72, 0x2E, 0x62, 0x69, 0x6E, 0x00 @ utf16 "sdmc:/twlbootldr.bin"

.align 2

yls8proc_cxi_filepath:
.hword 0x2F, 0x79, 0x6C, 0x73, 0x38, 0x70, 0x72, 0x6F, 0x63, 0x2E, 0x63, 0x78, 0x69, 0x00 @ utf16 "/yls8proc.cxi"

.align 2*/

/*mountcontenthook_archivevtable:
.space 0x100

mountcontenthook_archivevtable_openfilefuncptr:
.word 0*/

/*FIRM_contentid_versions:
.byte 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 44, 46, 48, 49, 50
.align 2

FIRM_sigword0_array: @ First u32 from the FIRM RSA signature, for each version.
.word 0xc6bf9f87, 0xbd593467, 0xf7836451, 0x83678b5a, 0x78abc618, 0x1422f3a0, 0x3e60995, 0x26c3ec7f, 0x322a2354, 0xf84fe1c1, 0x6bb5b75b, 0x594a4423, 0xaf8689bb, 0x315bcf7d, 0xbf240fcc, 0xceaa6ec2, 0x9f0a6868, 0xf8aad6dc, 0x5300febe

FIRM_contentid_totalversions:
.word 19

NEW3DS_FIRM_versions:
.byte 45, 46, 48, 49, 50
.align 2

NEW3DS_sigword0_array:
.word 0x79d0fb89, 0x87b79d15, 0x20f05310, 0xda1e3879, 0x5f96d06f

NEW3DS_totalversions:
.word 5*/

FIRMLAUNCH_CLEARPARAMS:
.word 1

new3ds_entrypointhookword:
.word 0

proc9_textstartaddr:
.word 0

/*twlbootldrenc_arm7:
//.incbin "../twlbootldrenc_arm7.bin"

twlbootldrenc_arm7_end:
.word 0

twlbootldrenc_arm9:
//.incbin "../twlbootldrenc_arm9.bin"

twlbootldrenc_arm9_end:
.word 0*/

