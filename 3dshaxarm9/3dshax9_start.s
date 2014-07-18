.arm

.arch armv5te
.fpu softvfp

.global _start
.global memset
.global launchcode_kernelmode
.global ioDelay

.global changempu_memregions
.global getsp
.global getcpsr
.global arm9dbs_stub
.global arm11_stub
.global write_arm11debug_patch
.global arm9_launchfirm
.global thread_entry
.global pxidev_cmdhandler_cmd0
.global mountcontent_nandsd_writehookstub
.global arm9_pxipmcmd1_getexhdr_writepatch
.global arm9general_debughook_writepatch
.global proc9_waitfirmevent_hook
.global proc9_waitfirmevent_hook_patchaddr
.global generate_branch
.global parse_branch
.global parse_branch_thumb

.global call_arbitaryfuncptr

.global svcSignalEvent
.global svcFlushProcessDataCache
.global svcGetSystemTick
.global svcCreateThread
.global svcExitThread
.global svcSleepThread
.global svcCloseHandle

.global pxifs_state
.global sdarchive_obj
.global nandarchive_obj
.global input_filepath
.global dump_filepath
.global FIRMLAUNCH_RUNNINGTYPE
.global RUNNINGFWVER
.global FIRMLAUNCH_FWVER
.global NANDREDIR_SECTORNUM
.global arm9_rsaengine_txtwrite_hooksz

.global FIRM_contentid_versions
.global FIRM_sigword0_array
.global FIRM_contentid_totalversions

.type launchcode_kernelmode STT_FUNC
.type ioDelay STT_FUNC
.type changempu_memregions STT_FUNC
.type write_arm11debug_patch STT_FUNC
.type arm9_launchfirm STT_FUNC
.type thread_entry STT_FUNC
.type mountcontent_nandsd_writehookstub STT_FUNC
.type arm9_pxipmcmd1_getexhdr_writepatch STT_FUNC
.type arm9general_debughook_writepatch STT_FUNC
.type proc9_waitfirmevent_hook STT_FUNC
.type generate_branch STT_FUNC
.type parse_branch STT_FUNC
.type parse_branch_thumb STT_FUNC

.type call_arbitaryfuncptr STT_FUNC

.type getsp STT_FUNC
.type getcpsr STT_FUNC
.type memset STT_FUNC
.type svcSignalEvent STT_FUNC
.type svcFlushProcessDataCache STT_FUNC
.type svcGetSystemTick STT_FUNC
.type svcCreateThread STT_FUNC
.type svcExitThread STT_FUNC
.type svcSleepThread STT_FUNC
.type svcCloseHandle STT_FUNC

.section .init

_start:
.word __text_start + 4
b _start_codebegin

.word 0x4d415250 @ "PRAM", magic number for paramaters.

FIRMLAUNCH_RUNNINGTYPE: @ 0 = native(no FIRM-launch), 1 = FIRM-launch was done via the below code, 2 = _start was called via other means with a Process9 hook.
.word 0

RUNNINGFWVER: @ FWVER of the currently running system.
.word 0x1F

FIRMLAUNCH_FWVER:
.word 0x2E

NANDREDIR_SECTORNUM:
.word /*0x1DD000*/ /*0x38CE800*/ 0x3598000 //4GB card = 0x1DD000, main image on 32GB card = 0x38CE800, other image on 32GB card = 0x3598000.

_start_codebegin:
ldr r0, FIRMLAUNCH_RUNNINGTYPE
cmp r0, #3
beq startcode_type3

push {r4, lr}
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

@bl initialize_nandarchiveobj

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

startcode_type3: @ This is jumped to when _start was called via a hook for the FIRM ARM9-kernel entrypoint. Returning from here results in jumping to the actual kernel entrypoint.
@b startcode_type3
push {r4, lr}

/*ldr r0, =0xffff0000
ldr r1, =0x18600000
ldr r2, =0x10000
sub r1, r1, r2
sub r1, r1, r2
startcode_type3_cpy:
ldr r3, [r0], #4
str r3, [r1], #4
subs r2, r2, #4
bgt startcode_type3_cpy*/

ldr r0, =RUNNINGFWVER
ldr r0, [r0]
lsr r1, r0, #31
cmp r1, #0
beq startcode_type3_patchstart
bl parse_configmem_firmversion
cmp r0, #0
popne {r4, pc}

ldr r1, =FIRMLAUNCH_FWVER
ldr r0, =RUNNINGFWVER
ldr r4, [r1]
ldr r0, [r0]
str r0, [r1]

startcode_type3_patchstart:
ldr r0, =0x08006800 @ Address
ldr r1, =0x000F8800 @ Size
bl patchfirm_arm9section

ldr r0, =FIRMLAUNCH_FWVER
str r4, [r0]

pop {r4, pc}
.pool

parse_configmem_firmversion:
and r3, r0, #0xff @ FIRM_VERSIONMINOR
lsr r1, r0, #8 @ FIRM_VERSIONMAJOR
lsr r2, r0, #16 @ FIRM_SYSCOREVER
and r1, r1, #0xff
and r2, r2, #0xff
mov r0, #1

cmp r1, #2 @ Return error when FIRM_VERSIONMAJOR!=2.
bxne lr
cmp r2, #2 @ Return error when FIRM_SYSCOREVER!=2.
bxne lr
cmp r3, #27 @ Return error when FIRM_VERSIONMINOR<27, should never happen on retail.
bxlt lr

ldr r1, =FIRM_contentid_totalversions
ldr r1, [r1]
sub r3, r3, #27

cmp r3, #17
subge r3, r3, #3

cmp r3, r1
bxge lr

ldr r0, =FIRM_contentid_versions
ldrb r0, [r0, r3]

ldr r1, =RUNNINGFWVER
str r0, [r1]

mov r0, #0
bx lr
.pool

launchcode_kernelmode_stub:
mov r0, sp
//ldr sp, =0x080ffe00
push {r0, r5, lr}
mrs r5, CPSR
orr r2, r5, #0x80
msr CPSR_c, r2
blx r4
msr CPSR_c, r5
pop {r0, r5, lr}
mov sp, r0
bx lr

launchcode_kernelmode:
push {r4}
mov r4, r0
adr r0, launchcode_kernelmode_stub
svc 0x0000007b
pop {r4}
bx lr

ioDelay:
  subs r0, #1
  bgt ioDelay
  bx lr
.pool

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

getsp:
mov r0, sp
bx lr

getcpsr:
mrs r0, CPSR
bx lr

call_arbitaryfuncptr:
push {r4, r5, r6, r7, r8, lr}
mov r7, r0
mov r8, r1
ldm r8, {r0, r1, r2, r3, r4, r5, r6}
blx r7
stm r8, {r0, r1, r2, r3, r4, r5, r6}
pop {r4, r5, r6, r7, r8, pc}

svcSignalEvent:
svc 0x18
bx lr

svcFlushProcessDataCache:
mov r2, r1
mov r1, r0
ldr r0, =0xffff8001
svc 0x54
bx lr
.pool

svcGetSystemTick:
svc 0x28
bx lr

svcCreateThread:
push {r0, r4}
ldr r0, [sp, #8]
ldr r4, [sp, #12]
svc 0x08
ldr r2, [sp, #0]
str r1, [r2]
add sp, sp, #4
pop {r4}
bx lr

svcExitThread:
svc 0x9
bx lr

svcSleepThread:
svc 0xa
bx lr

svcCloseHandle:
svc 0x23
bx lr

pxidev_cmdhandler_cmd0: @ This is code which the pxidev cmdhandler will jump to for handling cmdid 0x0000, when that switch statement addr was patched.
mov r0, r4
bl pxidev_cmdhandler_cmd0handler
ldr r0, =RUNNINGFWVER
ldr r0, [r0]
cmp r0, #0x1F
addeq sp, sp, #0x9c
popeq {r4, r5, r6, r7, r8, r9, sl, fp, pc} @ FW1F
add sp, sp, #0x84
pop {r4, r5, r6, r7, r8, r9, sl, fp, pc} @ FW2E
.pool

/*mountcontent_nandsd_writehookstub:
ldr r0, =0x8033570//0x802e62c
adr r1, mountcontent_nandsd_hookstub
ldr r2, [r1]
str r2, [r0]
ldr r2, [r1, #4]
str r2, [r0, #4]
mov r1, #8
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
/*sub sp, sp, #32
#if FIRMLAUNCH_FWVER == 0x1F

#define PXIFS_STATEPTR 0x0809797c
#define ARM9_ARCHIVE_MNTSD 0x8061451
#define ARM9_GETARCHIVECLASS 0x8063f91

#elif FIRMLAUNCH_FWVER == 0x2E

#define PXIFS_STATEPTR 0x080945c4
#define ARM9_ARCHIVE_MNTSD 0x805eb79
#define ARM9_GETARCHIVECLASS 0x8062715
#endif

mov r0, #0
str r0, [sp, #8]
str r0, [sp, #12]

ldr r5, =PXIFS_STATEPTR
ldr r5, [r5]
add r5, r5, #8 @ r5 = state
ldr r1, =0x2EA0
add r0, r5, r1
add r1, sp, #8
ldr r4, =ARM9_ARCHIVE_MNTSD
blx r4

mov r3, #0
str r3, [sp, #28]
str r3, [sp, #0]
str r3, [sp, #4]
add r0, sp, #16
mov r1, r5
ldr r2, [sp, #8]
ldr r3, [sp, #12]
ldr r4, =ARM9_GETARCHIVECLASS
blx r4

ldr r6, [sp, #28]

cmp r6, #0
beq proc9_waitfirmevent_hook_fail*/

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

//add sp, sp, #32
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

//.thumb

.thumb
arm9dbs_stub:
ldr r5, =arm9dbs_debugcode
blx r5
.pool

/*arm9_rsaengine_txtwrite_stub:
ldr r2, =arm9_rsaengine_txtwrite_hook
bx r2
.pool*/

.arm

arm9general_debugstub:
ldr pc, =arm9general_debughook
//blx ip
.pool

.thumb

arm9_pxipmcmd1_getexhdr_stub:
ldr r1, =arm9_pxipmcmd1_getexhdr_hook
blx r1
.pool

/*proc9_fsdeviceinit_hookstub:
ldr r0, =proc9_fsdeviceinit_hook
blx r0
.pool*/

.arm

arm11_stub:
ldr pc, =0x00100000
.pool

arm9dbs_debugcode:
add lr, lr, #4

ldr	r1, [r0, #0]
add	r2, sp, #0x74
ldr	r4, [r1, #0x6c]
add	r1, sp, #0x94

push {r0, r1, r2, r3, r4, r5, r6, r7, lr}
mov r0, sp
mov r1, #0x20
bl dumpmem
/*ldr r0, =0x20703000
mov r1, #0x20
str r1, [r0], #4
mov r3, sp

arm9dbs_debugcode_cpylp:
ldr r2, [r3], #4
str r2, [r0], #4
subs r1, r1, #4
bgt arm9dbs_debugcode_cpylp*/

pop {r0, r1, r2, r3, r4, r5, r6, r7, lr}

bx lr
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
/*ldr r1, [r0, #0]
ldr r2, =0x706c64 @ "dlp"
cmp r1, r2
bne arm9_pxipmcmd1_getexhdr_hookend @ Only handle exheaders where the exheader-name field matches the above value.

ldr r1, =0x250
add r4, r0, r1
mov r5, #0
ldr r1, =pxipm_getexhdrhook_servlist
mov r2, #0xa8
mov r3, #0x100

arm9_pxipmcmd1_getexhdr_hook_cpylp:
cmp r5, r2
ldrlt r0, [r1, r5]
movge r0, #0
str r0, [r4, r5]
add r5, r5, #4
cmp r5, r3
blt arm9_pxipmcmd1_getexhdr_hook_cpylp*/

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
b svcFlushProcessDataCache
.pool

arm9general_debughook_writepatch:
ldr r1, =arm9general_debugstub
ldr r2, [r1, #0]
str r2, [r0, #0]
ldr r2, [r1, #4]
str r2, [r0, #4]
/*ldr r2, [r1, #8]
str r2, [r0, #8]*/
mov r1, #8
b svcFlushProcessDataCache
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

input_filepath:
.hword 0x2F, 0x33, 0x64, 0x73, 0x68, 0x61, 0x78, 0x5F, 0x69, 0x6E, 0x70, 0x75, 0x74, 0x2E, 0x62, 0x69, 0x6E, 0x00 //UTF-16 "/3dshax_input.bin"

.align 2

twlbootldr_filepath:
.hword 0x73, 0x70, 0x69, 0x63, 0x61, 0x72, 0x64, 0x3A, 0x00 //0x73, 0x64, 0x6D, 0x63, 0x3A, 0x2F, 0x74, 0x77, 0x6C, 0x62, 0x6F, 0x6F, 0x74, 0x6C, 0x64, 0x72, 0x2E, 0x62, 0x69, 0x6E, 0x00 @ utf16 "sdmc:/twlbootldr.bin"

.align 2

yls8proc_cxi_filepath:
.hword 0x2F, 0x79, 0x6C, 0x73, 0x38, 0x70, 0x72, 0x6F, 0x63, 0x2E, 0x63, 0x78, 0x69, 0x00 @ utf16 "/yls8proc.cxi"

.align 2

mountcontenthook_archivevtable:
.space 0x100

mountcontenthook_archivevtable_openfilefuncptr:
.word 0

FIRM_contentid_versions:
.byte 0x00, 0x02, 0x09, 0x0B, 0x0F, 0x18, 0x1D, 0x1F, 0x25, 0x26, 0x29, 0x2A, 0x2E, 0x30, 0x37
.align 2

FIRM_sigword0_array: @ First u32 from the FIRM RSA signature, for each version.
.word 0xc6bf9f87, 0xbd593467, 0xf7836451, 0x83678b5a, 0x78abc618, 0x1422f3a0, 0x3e60995, 0x26c3ec7f, 0x26c3ec7f, 0x322a2354, 0xf84fe1c1, 0x6bb5b75b, 0xaf8689bb, 0x315bcf7d, 0xbf240fcc

FIRM_contentid_totalversions:
.word 15

/*twlbootldrenc_arm7:
//.incbin "../twlbootldrenc_arm7.bin"

twlbootldrenc_arm7_end:
.word 0

twlbootldrenc_arm9:
//.incbin "../twlbootldrenc_arm9.bin"

twlbootldrenc_arm9_end:
.word 0*/

/*pxipm_getexhdrhook_servlist:
.byte 0x41, 0x50, 0x54, 0x3A, 0x55, 0x00, 0x00, 0x00, 0x79, 0x32, 0x72, 0x3A, 0x75, 0x00, 0x00, 0x00, 0x67, 0x73, 0x70, 0x3A, 0x3A, 0x47, 0x70, 0x75, 0x6E, 0x64, 0x6D, 0x3A, 0x75, 0x00, 0x00, 0x00, 0x66, 0x73, 0x3A, 0x55, 0x53, 0x45, 0x52, 0x00, 0x68, 0x69, 0x64, 0x3A, 0x55, 0x53, 0x45, 0x52, 0x64, 0x73, 0x70, 0x3A, 0x3A, 0x44, 0x53, 0x50, 0x63, 0x66, 0x67, 0x3A, 0x75, 0x00, 0x00, 0x00, 0x66, 0x73, 0x3A, 0x52, 0x45, 0x47, 0x00, 0x00, 0x70, 0x73, 0x3A, 0x70, 0x73, 0x00, 0x00, 0x00, 0x69, 0x72, 0x3A, 0x75, 0x00, 0x00, 0x00, 0x00, 0x6E, 0x73, 0x3A, 0x73, 0x00, 0x00, 0x00, 0x00, 0x6E, 0x77, 0x6D, 0x3A, 0x3A, 0x55, 0x44, 0x53, 0x6E, 0x69, 0x6D, 0x3A, 0x73, 0x00, 0x00, 0x00, 0x61, 0x63, 0x3A, 0x75, 0x00, 0x00, 0x00, 0x00, 0x61, 0x6D, 0x3A, 0x6E, 0x65, 0x74, 0x00, 0x00, 0x63, 0x66, 0x67, 0x3A, 0x6E, 0x6F, 0x72, 0x00, 0x73, 0x6F, 0x63, 0x3A, 0x55, 0x00, 0x00, 0x00, 0x70, 0x78, 0x69, 0x3A, 0x64, 0x65, 0x76, 0x00, 0x70, 0x74, 0x6D, 0x3A, 0x73, 0x79, 0x73, 0x6D, 0x63, 0x73, 0x6E, 0x64, 0x3A, 0x53, 0x4E, 0x44*/

/*.global rsamodulo_slot0
rsamodulo_slot0:
.byte 0xA0, 0xED, 0xC3, 0xDA, 0xBB, 0x63, 0x88, 0xCD, 0xE1, 0x84, 0xBF, 0x5F, 0xBA, 0x60, 0x0D, 0xC8, 0x1E, 0xD7, 0x65, 0x35, 0x3C, 0x12, 0xA6, 0x24, 0x35, 0x93, 0xF3, 0x23, 0x56, 0xBD, 0x90, 0x08, 0x7D, 0xBE, 0x78, 0xDA, 0x16, 0x7A, 0x6D, 0xB1, 0x6C, 0x44, 0x32, 0xDE, 0x65, 0xE3, 0xC7, 0x62, 0x27, 0x57, 0x36, 0xAC, 0x96, 0x39, 0xFC, 0x77, 0xF9, 0xCF, 0x25, 0xCD, 0x02, 0xA3, 0x54, 0x1F, 0x29, 0xCB, 0x3E, 0x04, 0xD9, 0x77, 0xA0, 0xD3, 0x30, 0x21, 0xD8, 0x50, 0xFB, 0xFF, 0x2E, 0x55, 0x27, 0xA5, 0x24, 0xFF, 0x6B, 0x17, 0xEF, 0x55, 0x0B, 0x33, 0x3B, 0x3D, 0x34, 0xF1, 0xB8, 0xFA, 0x23, 0x1E, 0x26, 0xA9, 0x89, 0x69, 0x3F, 0x64, 0xA9, 0x9D, 0x20, 0x1E, 0x0A, 0xD7, 0xDA, 0xDE, 0x7C, 0xF5, 0xBD, 0x0E, 0x75, 0xEA, 0x15, 0x9A, 0x34, 0xB2, 0xAB, 0x3B, 0xE8, 0x1E, 0xF0, 0x30, 0x1F, 0xD5, 0x9B, 0xED, 0x27, 0xDC, 0x18, 0xEE, 0x12, 0x43, 0xAE, 0x3E, 0xB6, 0x54, 0x18, 0xA5, 0xBF, 0x32, 0xB9, 0x9B, 0x6E, 0xEA, 0xE2, 0xBE, 0x23, 0xF2, 0x40, 0x70, 0x05, 0x3D, 0x2F, 0xE8, 0xE9, 0x8B, 0x56, 0x30, 0x7C, 0x57, 0x86, 0xA5, 0x69, 0x96, 0x5F, 0x8A, 0xB7, 0xB6, 0xFA, 0xF2, 0xD7, 0x4A, 0x47, 0xD7, 0xC0, 0x23, 0x64, 0x7D, 0xCE, 0x56, 0xC2, 0x0F, 0x51, 0x07, 0x0B, 0x6D, 0xFD, 0xA9, 0xA4, 0x6D, 0xE5, 0xA8, 0x16, 0x49, 0xCD, 0x93, 0x05, 0x57, 0x79, 0x19, 0x0C, 0x4C, 0xA0, 0xA7, 0xDB, 0x34, 0x13, 0xB6, 0x32, 0x6F, 0x39, 0xFE, 0x28, 0x4C, 0xAC, 0x32, 0xF7, 0xE2, 0xAD, 0xE4, 0xF7, 0xE7, 0x3D, 0x3E, 0xAA, 0x06, 0x8A, 0x6D, 0x4E, 0x08, 0x94, 0x69, 0x82, 0x97, 0x63, 0x74, 0xF1, 0x3D, 0x63, 0x3C, 0x4C, 0x2D, 0xD0, 0x01, 0x59, 0x69, 0x37, 0x81, 0x34, 0x41*/


/*.byte 0xC9, 0xA9, 0xBF, 0x9C, 0xEB, 0x07, 0xAF, 0x23, 0x40, 0x6B, 0xDA, 0xB5, 0x69, 0x93, 0xA6, 0xE4
.byte 0xD4, 0x62, 0x76, 0x3B, 0xF8, 0x55, 0x10, 0xF3, 0x28, 0x2D, 0x6D, 0x9F, 0x81, 0xEC, 0x0C, 0x5F
.byte 0xF7, 0x8F, 0x58, 0x57, 0x04, 0xD6, 0xE5, 0x24, 0x6E, 0x27, 0x55, 0x2D, 0x55, 0xAA, 0x9B, 0x0B
.byte 0x20, 0x4C, 0x42, 0x08, 0xC1, 0xE0, 0xE2, 0xAA, 0x03, 0x13, 0x5D, 0x69, 0x60, 0x9F, 0x94, 0xBC
.byte 0x0F, 0xE1, 0xE1, 0x82, 0x53, 0x5C, 0x4F, 0xBA, 0x90, 0x21, 0x42, 0xBB, 0x81, 0x0C, 0x05, 0x06
.byte 0xFE, 0x10, 0x8F, 0x05, 0x40, 0xD9, 0x55, 0xAB, 0xD4, 0xCA, 0x74, 0x6E, 0xF7, 0x0B, 0x94, 0x5F
.byte 0xDB, 0x06, 0x59, 0xB6, 0x31, 0x2B, 0x1E, 0xB2, 0x03, 0xBA, 0xDF, 0x0C, 0xE7, 0x11, 0x5A, 0x0F
.byte 0xB6, 0x4D, 0xAE, 0x3E, 0xCA, 0x93, 0xD7, 0x3D, 0xA5, 0x9F, 0xEE, 0x54, 0xE0, 0x37, 0xE9, 0xB2
.byte 0x35, 0x33, 0x47, 0x29, 0xC6, 0x24, 0x78, 0xB7, 0xDE, 0xE6, 0xC5, 0x63, 0xD4, 0xFD, 0x7C, 0x9B
.byte 0x3D, 0xC2, 0x40, 0xB5, 0x7A, 0xC1, 0x15, 0x3A, 0x98, 0x1C, 0xE1, 0xEB, 0xA8, 0x65, 0x7A, 0xFD
.byte 0xAA, 0x86, 0xD6, 0xBE, 0xE6, 0xFD, 0x6E, 0xAF, 0x5A, 0xA0, 0x96, 0x74, 0x83, 0xE7, 0x17, 0x6A
.byte 0x34, 0x02, 0x99, 0x54, 0xF9, 0x56, 0x8D, 0x31, 0xAF, 0x06, 0x68, 0xD8, 0xFF, 0x12, 0x5E, 0x5E
.byte 0xCE, 0x68, 0x18, 0xB2, 0x93, 0x67, 0x2A, 0x5B, 0x51, 0xE9, 0xE7, 0x71, 0x1B, 0x22, 0x4B, 0x9A
.byte 0x25, 0x78, 0xDB, 0x5E, 0xEE, 0x3C, 0xFC, 0xAD, 0x55, 0x02, 0x67, 0xC7, 0xE9, 0x56, 0x15, 0xB2
.byte 0xA9, 0xE6, 0xBC, 0x75, 0x89, 0x28, 0x0A, 0x61, 0xFD, 0x91, 0xD1, 0xFC, 0x72, 0x0C, 0x9D, 0x44
.byte 0xE9, 0x08, 0x4E, 0xB6, 0xA1, 0x55, 0xEC, 0x30, 0xFE, 0xA7, 0xCF, 0x46, 0x35, 0x9A, 0x75, 0xA3*/

