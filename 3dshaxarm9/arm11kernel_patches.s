.arch armv5te
.fpu softvfp
.text
.arm

.global get_arm11debuginfo_physaddr
.global write_arm11debug_patch
.global writepatch_arm11kernel_svcaccess
.type get_arm11debuginfo_physaddr STT_FUNC
.type write_arm11debug_patch STT_FUNC
.type writepatch_arm11kernel_svcaccess STT_FUNC

writepatch_arm11kernel_getsvchandler_physaddr:
push {r4, r5, lr}
ldr r5, =0x1FF80000

ldr r0, =RUNNINGFWVER
ldr r0, [r0]
cmp r0, #0x25
ldrlt r4, =0xfff60000
ldrge r4, =0xfff50000
cmp r0, #0x37
ldrge r4, =0xfff40000

ldr r0, =0xffff0000
add r0, r0, #8
ldr r1, =0x1FFF4000
ldr r1, [r1, #8]
bl parse_branch
sub r0, r0, r4
add r0, r0, r5 @ r0 = phys-addr of the code the 0xffff0000 svc-vector jumps to.

ldr r1, [r0, #8]
sub r1, r1, r4
add r1, r1, r5 @ r0 = phys-addr of the svc-handler.
mov r0, r1
pop {r4, r5, pc}
.pool

writepatch_arm11kernel_svcaccess:
push {r4, r5, lr}
mov r1, #0
ldr r0, =RUNNINGFWVER
ldr r0, [r0]
/*cmp r0, #0x1F
ldreq r1, =0x1FF827CC
cmp r0, #0x2E
ldreq r1, =0x1FF822A8
cmp r1, #0
beq writepatch_arm11kernel_svcaccess_end*/

cmp r0, #0x25
ldrlt r4, =0xfff60000
ldrge r4, =0xfff50000
cmp r0, #0x37
ldrge r4, =0xfff40000
ldr r5, =0x1FF80000

bl writepatch_arm11kernel_getsvchandler_physaddr
mov r1, r0
ldr r3, =0x0affffea

writepatch_arm11kernel_svcaccess_lp:
ldr r2, [r1]
cmp r2, #0
beq writepatch_arm11kernel_svcaccess_end @ Return if a zero word is encountered, like when the target word was already patched, or when the start of the svc-jumptable was reached.
cmp r2, r3
addne r1, r1, #4
bne writepatch_arm11kernel_svcaccess_lp

mov r2, #0
str r2, [r1] @ Patch out the ARM11 kernel branch in the SVC handler which is executed when the process doesn't have access to a SVC.

writepatch_arm11kernel_svcaccess_end:
pop {r4, r5, pc}
.pool

#ifdef ENABLE_ARM11KERNEL_DEBUG

#ifdef ENABLE_ARM11KERNEL_PROCSTARTHOOK
#ifndef DISABLE_NETDEBUG
#define ENABLE_NETDEBUG
#endif
#endif

writepatch_arm11kernel_getsvctableadr: @ inr0: val0=return physaddr of jumptable, non-zero=return addr for the specified SVC. for SVCs: out r0 = vaddr, out r1 = physaddr.
push {r4, r5, r6, lr}
mov r6, r0

ldr r5, =0x1FF80000

ldr r0, =RUNNINGFWVER
ldr r0, [r0]
cmp r0, #0x25
ldrlt r4, =0xfff60000
ldrge r4, =0xfff50000
cmp r0, #0x37
ldrge r4, =0xfff40000

bl writepatch_arm11kernel_getsvchandler_physaddr

mov r1, r0
ldr r3, =0xeaffffef

writepatch_arm11kernel_getsvctableadr_lp:
ldr r2, [r1]
cmp r2, r3
addne r1, r1, #4
bne writepatch_arm11kernel_getsvctableadr_lp

add r0, r1, #4 @ r0 = phys addr of arm11kernel svc jump-table
cmp r6, #0
beq writepatch_arm11kernel_getsvctableadr_end

mov r1, r6
lsl r1, r1, #2
ldr r0, [r0, r1] @ vaddr of the specified handler, from the jump-table
sub r1, r0, r4
add r1, r1, r5 @ phys addr of the above handler

writepatch_arm11kernel_getsvctableadr_end:
pop {r4, r5, r6, pc}
.pool

#ifdef ENABLE_ARM11KERNEL_PROCSTARTHOOK
writepatch_arm11kernel_svc73:
push {r4, r5, r6, lr}
mov r6, r0

ldr r5, =0x1FF80000

ldr r0, =RUNNINGFWVER
ldr r0, [r0]
cmp r0, #0x25
ldrlt r4, =0xfff60000
ldrge r4, =0xfff50000
cmp r0, #0x37
ldrge r4, =0xfff40000

mov r0, #0x73
bl writepatch_arm11kernel_getsvctableadr

add r0, r0, #16
ldr r1, [r1, #16]
bl parse_branch

add r0, r0, #8
sub r1, r0, r4
add r5, r5, r1
mov r1, r6
mov r2, #1
bl generate_branch
str r0, [r5]

pop {r4, r5, r6, pc}
.pool
#endif

writepatch_arm11kernel_getkillprocessaddr:
push {r4, r5, r6, lr}
mov r6, r0

ldr r5, =0x1FF80000

ldr r0, =RUNNINGFWVER
ldr r0, [r0]
cmp r0, #0x25
ldrlt r4, =0xfff60000
ldrge r4, =0xfff50000
cmp r0, #0x37
ldrge r4, =0xfff40000

mov r0, #0x3
bl writepatch_arm11kernel_getsvctableadr

add r0, r0, #0xc
ldr r1, [r1, #0xc]
bl parse_branch

adr r1, arm11kernel_patch_killprocessaddr
str r0, [r1]

pop {r4, r5, r6, pc}
.pool

get_arm11debuginfo_physaddr:
push {r4, lr}
ldr r4, =RUNNINGFWVER
ldr r4, [r4]

ldr r0, =0x1FFDF000

ldr r3, =0x1000
cmp r4, #0x25
subge r0, r0, r3
cmp r4, #0x37
ldrge r0, =0x1FFDD000
pop {r4, pc}
.pool

write_arm11debug_patch:
push {r4, r5, r6, r7, r8, lr}
ldr r8, =RUNNINGFWVER
ldr r8, [r8]

bl get_arm11debuginfo_physaddr

ldr r1, =arm11kernel_patch_fwver
str r8, [r1]

mov r1, #0
mov r2, #0xC00
bl memset

bl writepatch_arm11kernel_getkillprocessaddr

ldr r0, =arm11kernel_patch
ldr r1, =arm11kernel_patchend
sub r1, r1, r0
//ldr r2, =0x1FFF406c
ldr r2, =0x1FF81800 //0x1FFF4b10

/*ldr r3, =0xffffffff
str r3, [r2]
add r2, r2, #0x14*/

write_arm11debug_patch_lp:
ldr r3, [r0], #4
str r3, [r2], #4
sub r1, r1, #4
cmp r1, #0
bgt write_arm11debug_patch_lp

//ldr r4, =0xffff0b10
//ldr r4, =0xffff0080
ldr r5, =0x1FF80000

/*mov r1, #0x1F
mov r2, #0x2E
cmp r8, r1
ldreq r7, =0xfff60000*/
//ldreq r6, =0x324
//cmp r8, r2
//ldreq r7, =0xfff50000
//ldreq r6, =0x614
cmp r8, #0x25
ldrlt r7, =0xfff60000
ldrge r7, =0xfff50000
cmp r8, #0x37
ldrge r7, =0xfff40000

ldr r0, =0xffff0000
add r0, r0, #4
ldr r1, =0x1FFF4000
ldr r1, [r1, #4]
bl parse_branch
sub r6, r0, r7

ldr r4, =0x1800
add r4, r4, r7

add r6, r6, #4  @ Patch the undef instruction vector code.
add r0, r7, r6
mov r1, r4
mov r2, #0
bl generate_branch
str r0, [r5, r6]

adr r1, arm11_undefhandler
adr r3, arm11_prefetchhandler
sub r3, r3, r1
add r4, r4, r3

/*mov r1, #0x1F
mov r2, #0x2E
cmp r8, r1
ldreq r6, =0x33c
cmp r8, r2
ldreq r6, =0x62c*/

ldr r0, =0xffff0000 @ Patch the prefetch abort vector code.
add r0, r0, #0xc
ldr r1, =0x1FFF4000
ldr r1, [r1, #0xc]
bl parse_branch
sub r6, r0, r7

add r6, r6, #4
add r0, r7, r6
mov r1, r4
mov r2, #0
bl generate_branch
str r0, [r5, r6]

adr r1, arm11_prefetchhandler
adr r3, arm11_daborthandler
sub r3, r3, r1
add r4, r4, r3

/*mov r1, #0x1F
mov r2, #0x2E
cmp r8, r1
ldreq r6, =0x348
cmp r8, r2
ldreq r6, =0x638*/

ldr r0, =0xffff0000 @ Patch the data abort vector code.
add r0, r0, #0x10
ldr r1, =0x1FFF4000
ldr r1, [r1, #0x10]
bl parse_branch
sub r6, r0, r7

add r6, r6, #4
add r0, r7, r6
mov r1, r4
mov r2, #0
bl generate_branch
str r0, [r5, r6]

#ifdef ARM11KERNEL_ENABLECMDLOG
adr r1, arm11kernel_patch
adr r3, arm11kernel_processcmd_patch
sub r3, r3, r1
//ldr r4, =0xffff0b10
ldr r4, =0x1800
add r4, r4, r7
add r4, r4, r3

mov r6, #0
cmp r8, #0x1F
ldreq r6, =0x1f3a4 @ Patch the process_cmdbuf_sendreply() code.
cmp r8, #0x2E
ldreq r6, =0x20254//=0x20d6c
cmp r8, #0x30
ldreq r6, =0x20250
cmp r8, #0x37
ldreq r6, =0x20500
cmp r6, #0
beq write_arm11debug_patch_cmdhookend

add r0, r7, r6
mov r1, r4
mov r2, #1
bl generate_branch
cmp r8, #0x2E
ldreq r6, =0x5f254//=0x5fd6c
cmp r8, #0x30
ldreq r6, =0x5f250
cmp r8, #0x37
ldreq r6, =0x5e500
str r0, [r5, r6]

/*ldr r6, =0x1fe84
add r0, r7, r6
mov r1, r4
mov r2, #1
bl generate_branch @ Patch the process_cmdbuf_sendcmd() code.
str r0, [r5, r6]*/
#endif

write_arm11debug_patch_cmdhookend:
#ifdef ENABLE_ARM11KERNEL_PROCSTARTHOOK
adr r1, arm11kernel_patch
ldr r3, =arm11kernel_svc73_hook
sub r3, r3, r1
ldr r4, =0x1800
add r4, r4, r7
add r4, r4, r3

mov r0, r4
bl writepatch_arm11kernel_svc73

/*mov r1, #0x1F
mov r2, #0x2E
cmp r8, r1
ldreq r6, =0x8344
cmp r8, r2
ldreq r6, =0x7fc0

add r0, r7, r6
mov r1, r4
mov r2, #1
bl generate_branch @ Patch the svc73() code.
str r0, [r5, r6]*/

/*ldr r1, =arm11kernel_patch
ldr r3, =arm11kernel_svc75_hook
sub r3, r3, r1
ldr r4, =0x1800
add r4, r4, r7
add r4, r4, r3

mov r1, #0x1F
mov r2, #0x2E
cmp r8, r1
ldreq r6, =0x8670
cmp r8, r2
ldreq r6, =0x82ec

add r0, r7, r6
mov r1, r4
mov r2, #1
bl generate_branch @ Patch the svc75() code.
str r0, [r5, r6]*/
#endif

pop {r4, r5, r6, r7, r8, pc}
.pool

.arch armv6
.fpu vfp

arm11kernel_patch:

arm11_undefhandler: //Undef instruction handler
b arm11_undefhandler_start

arm11kernel_patch_fwver:
.word 0

b arm11kernel_getdebugstateptr

arm11kernel_patch_killprocessaddr:
.word 0

arm11_undefhandler_start:
sub lr, lr, #4
srsdb sp!, #19
cps #19

push {lr}

ldr lr, [sp, #4]
//sub lr, lr, #4
push {lr}
mov lr, #0
push {lr}

ldr lr, [sp, #16] @ Load SPSR
tst lr, #0x20
bne arm11_exceptionhandler @ Branch when not ARM-mode.

ldr lr, [sp, #12]
ldr lr, [lr]
lsl lr, lr, #4
sub lr, lr, #0xc0000000
cmp lr, #0x30000000
bcs arm11_exceptionhandler @ Branch for non-vfp instructions.

.word 0xeef8ea10 //vmrs lr, fpexc
tst lr, #0x40000000
bne arm11_exceptionhandler

stmdb sp, {r0, r1, r2, r3, fp, ip, sp, lr}^
sub sp, sp, #32
//push {r0, r1, r2, r3, fp, ip}
ldr r1, arm11kernel_patch_fwver
mov r2, #0x25
ldr r0, =0xffff05c8
cmp r1, r2
subge r0, r0, #8
blx r0
//pop {r0, r1, r2, r3, fp, ip}
ldm sp, {r0, r1, r2, r3, fp, ip, sp, lr}^
add sp, sp, #32

add sp, sp, #12
rfeia sp!

arm11_prefetchhandler: //Prefetch abort handler
srsdb sp!, #19
cps #19

push {lr}

ldr lr, [sp, #4]
sub lr, lr, #4
push {lr}
mov lr, #1
push {lr}
b arm11_exceptionhandler

arm11_daborthandler: //Data abort handler
srsdb sp!, #19
cps #19

push {lr}

ldr lr, [sp, #4]
sub lr, lr, #8
push {lr}
mov lr, #2
push {lr}
b arm11_exceptionhandler

arm11_exceptionhandler:
stmdb sp, {sp, lr}^
sub sp, sp, #8
push {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
cpsie af

mov r0, sp
bl arm11kernel_exceptionregdump

cpsid aif
pop {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
ldm sp, {sp, lr}^
add sp, sp, #8
add sp, sp, #12
rfeia sp!

.pool

arm11kernel_waitdebuginfo_magic: @ r0=debuginfo ptr
ldr r1, =0x58584148
arm11kernel_waitdebuginfo_magic_lp:
mov r2, #0
mcr p15, 0, r2, c7, c14, 0

ldr r2, [r0]
cmp r2, r1
beq arm11kernel_waitdebuginfo_magic_lp
bx lr

arm11kernel_getdebugstateptr:
ldr r1, arm11kernel_patch_fwver
ldr r0, =0xFFF3F000
ldr r3, =0x1000
cmp r1, #0x25
subge r0, r0, r3
cmp r1, #0x37
ldrge r0, =0xFFF3D000
bx lr

.pool

KProcessmem_getphysicaladdr:
push {r4, r5}
ldr r3, arm11kernel_patch_fwver
cmp r3, #0x1F
ldreq r2, =0xfff7a8c4
cmp r3, #0x25
ldreq r2, =0xfff6b3d8
cmp r3, #0x26
ldreq r2, =0xfff6b3d4
cmp r3, #0x29
cmpne r3, #0x2A
ldreq r2, =0xfff6b7d0
cmp r3, #0x2E
ldreq r2, =0xfff6b810
cmp r3, #0x30
ldreq r2, =0xfff6b80c
cmp r3, #0x37
ldreq r2, =0xfff5b924
pop {r4, r5}
bx r2

.pool

arm11kernel_exceptionregdump:
push {r4, r5, r6, lr}
mov r6, r0

//cpsid i @ disable IRQs

bl arm11kernel_getdebugstateptr
add r4, r0, #0x800
add r0, r0, #0x200
bl arm11kernel_waitdebuginfo_magic

add r0, r4, #4
mov r2, #0

ldr r1, =0x47424445 @ Debuginfo type = "EDBG".
str r1, [r0], #4
mov r1, #0x200
str r1, [r0], #4 @ Total size of the debuginfo.

arm11kernel_exceptionregdump_L1:
ldr r1, [r6, r2]
str r1, [r0], #4
add r2, r2, #4
cmp r2, #0x54
blt arm11kernel_exceptionregdump_L1

mov r2, #0
mov r3, r2
mov r5, r2
ldr r1, =0xffff9004 @ Load the process-name from the current KProcess' KCodeSet.
ldr r1, [r1] @ r1 = current KProcess*
cmp r1, #0
beq arm11kernel_exceptionregdump_writeprocname

ldr r3, arm11kernel_patch_fwver
cmp r3, #0x37
addge r1, r1, #8
mov r3, #0

ldr r5, [r1, #0x54]
ldr r1, [r1, #0xa8]
cmp r1, #0
beq arm11kernel_exceptionregdump_writeprocname

ldr r2, [r1, #0x50]
ldr r3, [r1, #0x54]

arm11kernel_exceptionregdump_writeprocname:
str r2, [r0], #4
str r3, [r0], #4
str r5, [r0], #4 @ process MMU table ptr.

mrc p15, 0, r1, c5, c0, 0 @ Read DFSR "Data Fault Status Register".
mrc p15, 0, r2, c5, c0, 1 @ Read IFSR "Instruction Fault Status Register".
mrc p15, 0, r3, c6, c0, 0 @ Read FAR "Fault Address Register".
str r1, [r0], #4
str r2, [r0], #4
str r3, [r0], #4

ldr r1, [r4, #0x4c]
cmp r1, #1 @ Check whether this exception is a Prefetch Abort.
cmpeq r2, #2
movne r5, #0
moveq r5, #1 @ r5=1 when this exception was triggered via a breakpoint.

ldr r1, [r4, #0x4c]
cmp r1, #1
addeq r0, r0, #0x20
beq arm11kernel_exceptionregdump_datadump_init

arm11kernel_exceptionregdump_datadump_pc:
mov r3, #0
add r1, r4, #0x50//sp = #0x44, lr = #0x48, pc = #0x50
ldr r1, [r1]
sub r1, r1, #0x10

arm11kernel_exceptionregdump_datadump_pclp:
ldr r2, [r1], #4
str r2, [r0], #4
add r3, r3, #4
cmp r3, #0x20
blt arm11kernel_exceptionregdump_datadump_pclp

arm11kernel_exceptionregdump_datadump_init:
mov r3, #0
add r1, r4, #0x44//sp = #0x44, lr = #0x48, pc = #0x50
ldr r1, [r1]

//cmp r5, #1
/*streq r1, [r6, #12] @ saved r3 = sp, for CSND debug.
ldreq r1, [r6, #24] @ r1 = saved r6
ldr r2, =0xfff
biceq r1, r1, r2*/
/*ldr r2, =0x108618
ldr r3, [r4, #0x48] @ r3 = saved lr
cmpeq r3, r2
ldrne r2, =0xfff8b00a
strne r2, [r6] @ saved r0 = 0xfff8b00a*/

//b arm11kernel_exceptionregdump_datadumpstart

/*mov r0, #0
str r0, [r6, #12] @ saved r3 = 0
mov r0, r4
ldr r1, [r6]
bl arm11kernel_gxcmd3debug
b arm11kernel_exceptionregdump_L2_end*/

arm11kernel_exceptionregdump_datadumpstart:
cmp r1, #0
beq arm11kernel_exceptionregdump_L2_end
bic r1, r1, #3
//sub r1, r1, #0x10

arm11kernel_exceptionregdump_L2:
ldr r2, [r1], #4
str r2, [r0], #4
add r3, r3, #4
cmp r3, #0x190
blt arm11kernel_exceptionregdump_L2

arm11kernel_exceptionregdump_L2_end:
ldr r1, =0x58584148
str r1, [r4]

bl arm11kernel_getdebugstateptr
add r5, r0, #0x200
add r6, r5, #0x500
mov r2, #0
arm11kernel_exceptionregdump_blkcpy:
ldr r0, [r4, r2]
str r0, [r5, r2]
str r0, [r6, r2]
add r2, r2, #4
cmp r2, #0x200
blt arm11kernel_exceptionregdump_blkcpy

mov r0, #0
mcr p15, 0, r0, c7, c10, 5
mcr p15, 0, r0, c7, c14, 0

mov r0, r5
bl arm11kernel_waitdebuginfo_magic

#ifdef ENABLE_NETDEBUG
cpsie i @ enable IRQs
mov r0, r6
bl arm11kernel_waitdebuginfo_magic

bl arm11kernel_getdebugstateptr
mov r3, #0
arm11kernel_exceptionregdump_waitsignal:
ldr r1, [r0]
cmp r1, r3
beq arm11kernel_exceptionregdump_waitsignal
str r3, [r0]
cpsid i @ disable IRQs

ldr r2, =0x4d524554 @ "TERM"
cmp r1, r2
bne arm11kernel_exceptionregdump_exit

#endif

arm11kernel_exceptionregdump_end:
ldr r4, arm11kernel_patch_killprocessaddr
ldr r0, =0xffff9004
ldr r0, [r0]
//cmp r5, #0
blx r4 @ Terminate the current process.

arm11kernel_exceptionregdump_exit:
pop {r4, r5, r6, pc}
.pool

/*arm11kernel_gxcmd3debug:
ldr r2, =0x33435847 @ "GXC3"
add r0, r0, #4
str r2, [r0], #4
mov r2, #0x24
str r2, [r0], #4

add r3, r1, #0x14
arm11kernel_gxcmd3debug_copydata:
ldr r2, [r1], #4
str r2, [r0], #4
cmp r1, r3
blt arm11kernel_gxcmd3debug_copydata

lsr r2, r2, #14
and r2, r2, #7
cmp r2, #0
moveq r3, #4
cmpne r2, #1
moveq r3, #3
movne r3, #2

sub r1, r1, #0x14
ldrh r2, [r1, #0xc]
mul r2, r2, r3
ldrh r3, [r1, #0xe]
mul r3, r3, r2
str r3, [r0], #4

bx lr
.pool*/

#ifdef ARM11KERNEL_ENABLECMDLOG
arm11kernel_processcmd_patch:
push {r0, r1, r2, r3, r4, r5, r6, r7, r8, lr}

ldr r5, arm11kernel_patch_fwver
cmp r5, #0x1F
lsreq r1, r1, #6
streq r1, [sp, #4]
//lsrne r2, r2, #6
movne r2, r4
strne r2, [sp, #8]

cpsid i @ disable IRQs

bl arm11kernel_getdebugstateptr
add r4, r0, #0x200

mov r0, r4
bl arm11kernel_waitdebuginfo_magic

mov r0, r4
mov r1, #0
mov r2, #0x420
arm11kernel_processcmd_patch_cleardebuginfo:
str r1, [r0], #4
subs r2, r2, #4
bgt arm11kernel_processcmd_patch_cleardebuginfo

/*ldr r0, =0xfff7f3a8
ldr lr, [sp, #32]
cmp r0, lr
moveq r5, #1 @ process_cmdbuf_sendreply()
movne r5, #0 @ process_cmdbuf_sendcmd()*/
mov r5, #1

/*cmp r5, #0  @ r6/r7 = src/dst KThread's KProcess
//ldreq r0, [sp, #0x98]
ldrne r6, [sp, #0x94]
//ldreq r1, [sp, #0x9c]
ldrne r7, [sp, #0x98]*/

ldr ip, arm11kernel_patch_fwver
cmp ip, #0x1F
ldreq r6, [sp, #0xbc] @ r6/r7 = src/dst KThread
ldreq r7, [sp, #0xb8]
cmp ip, #0x2E
/*ldreq r6, [sp, #0xc0]
ldreq r7, [sp, #0xbc]*/
ldrge r6, [sp, #0x20]
ldrge r7, [sp, #0x10]

ldr r0, [r6, #0x80] @ r6/r7 = src/dst KThread's KProcess
ldr r1, [r7, #0x80]

ldr r3, arm11kernel_patch_fwver
cmp r3, #0x37
addge r0, r0, #8
addge r1, r1, #8

ldr r0, [r0, #0xa8] @ r0/r1 = src/dst KProcess' KCodeSet
ldr r1, [r1, #0xa8]

ldr r2, =0x444d4344 @ Debuginfo type = "DCMD".
str r2, [r4, #4]
mov r3, #0x420
str r3, [r4, #8]

str r5, [r4, #12]

ldr r2, [r0, #0x50] @ Write the src process-name to out+8.
ldr r3, [r0, #0x54]
str r2, [r4, #16]
str r3, [r4, #20]
mov r0, r2

ldr r2, [r1, #0x50] @ Write the dst process-name to out+16.
ldr r3, [r1, #0x54]
str r2, [r4, #24]
str r3, [r4, #28]

#ifdef CMDLOGGING_PADCHECK
cmp ip, #0x37
ldrlt r3, =0xFFFD4000
ldrge r3, =0xFFFC2000
ldr r1, =CMDLOGGING_PADCHECK
ldrh r3, [r3]
tst r3, r1
bne arm11kernel_processcmd_patchend @ Only do logging / check procname when the above button(s) specified via CMDLOGGING_PADCHECK is pressed.
#endif

#ifndef CMDLOGGING_PROCNAME0
#error "The CMDLOGGING_PROCNAME0 define must be set in order to use cmd-logging."
#endif

ldr r1, =CMDLOGGING_PROCNAME0

#ifdef CMDLOGGING_PROCNAME1
ldr r3, =CMDLOGGING_PROCNAME1
#endif

cmp r2, r1 @ Ignore commands where the src/dst is not the above process(es).
cmpne r0, r1
beq arm11kernel_processcmd_procnamecheck_end
//beq arm11kernel_processcmd_patchend
#ifdef CMDLOGGING_PROCNAME1
cmp r2, r3
cmpne r0, r3
beq arm11kernel_processcmd_procnamecheck_end
#endif
b arm11kernel_processcmd_patchend

arm11kernel_processcmd_procnamecheck_end:
add r0, r4, #0x20
/*cmp r5, #0
moveq r1, fp
ldrne r1, [sp, #0x78] @ r1 = src cmdbuf*/
ldr r1, [r6, #0x94]
add r1, r1, #0x80
mov r2, #0x100
arm11kernel_processcmd_patch_cpylp:
ldr r3, [r1], #4
str r3, [r0], #4
subs r2, r2, #4
bgt arm11kernel_processcmd_patch_cpylp

add r0, r4, #0x120
/*cmp r5, #0
ldreq r1, [sp, #0x30]
movne r1, fp @ r1 = dst cmdbuf*/
ldr r1, [r7, #0x98]
add r1, r1, #0x80
mov r2, #0x100
arm11kernel_processcmd_patch_cpylp2:
ldr r3, [r1], #4
str r3, [r0], #4
subs r2, r2, #4
bgt arm11kernel_processcmd_patch_cpylp2

cmp r5, #0
/*ldreq r0, [sp, #0x98]
ldrne r0, [sp, #0x94] @ r0 = src KThread's KProcess*/
ldr r0, [r6, #0x80]
add r1, r4, #0x20 @ r1 = src cmdbuf stored in the debuginfo
add r2, r4, #0x220 @ r2 = output array of converted physical addrs, for the src cmdbuf.
bl arm11kernel_convertcmd_vaddr2phys

cmp r5, #0
/*ldreq r0, [sp, #0x9c]
ldrne r0, [sp, #0x98] @ r0 = dst KThread's KProcess*/
ldr r0, [r7, #0x80]
add r1, r4, #0x120 @ r1 = dst cmdbuf stored in the debuginfo
add r2, r4, #0x320 @ r2 = output array of converted physical addrs, for the dst cmdbuf.
blne arm11kernel_convertcmd_vaddr2phys

ldr r1, =0x58584148
str r1, [r4]
mov r0, #0
mcr p15, 0, r0, c7, c10, 5
mcr p15, 0, r0, c7, c14, 0

mov r0, r4
bl arm11kernel_waitdebuginfo_magic

arm11kernel_processcmd_patchend:
cpsie i @ enable IRQs
pop {r0, r1, r2, r3, r4, r5, r6, r7, r8, lr}
bx lr
.pool

arm11kernel_convertcmd_vaddr2phys: @ r0 = dst KThread's KProcess, r1 = cmdbuf stored in the debuginfo, r2 = output array of converted physical addrs, for the dst cmdbuf.
push {r4, r5, r6, r7, r8, r9, lr}
sub sp, sp, #8
mov r4, r0
mov r5, r1
mov r6, r2

ldr r7, [r5]
and r8, r7, #0x3f
lsr r7, r7, #6
and r7, r7, #0x3f
add r7, r7, #1
str r7, [sp]
add r8, r8, r7
cmp r7, r8
beq arm11kernel_convertcmd_vaddr2phys_end

arm11kernel_convertcmd_vaddr2phys_lp:
ldr r9, [r5, r7, lsl #2]
lsr r0, r9, #26
and r9, r9, #0xe
cmp r9, #0
beq arm11kernel_convertcmd_vaddr2phys_lpend @ This value is for the handle/processid header, so don't process those here.

mov r0, #4
cmp r9, #2
moveq r0, #14
cmp r9, #4
moveq r0, #8

add r9, r7, #1
ldr r1, [r5, r9, lsl #2]
ldr r2, [r5, r7, lsl #2]
lsr r2, r2, r0
//cmp r2, #0x200
//movgt r2, #0x200
str r2, [sp, #4] @ Buffer size loaded from the header word.

lsreq r3, r1, #28
cmpeq r3, #0x2
moveq r0, r1
beq arm11kernel_convertcmd_vaddr2phys_getphysend

add r0, r4, #0x54

ldr r3, arm11kernel_patch_fwver
cmp r3, #0x37
addge r0, r0, #8

bl KProcessmem_getphysicaladdr

arm11kernel_convertcmd_vaddr2phys_getphysend:
ldr r1, [sp]
ldr r2, [sp, #4]
sub r3, r7, r1
add r9, r3, #1
str r2, [r6, r3, lsl #2]
str r0, [r6, r9, lsl #2]

mov r0, #0

arm11kernel_convertcmd_vaddr2phys_lpend:
add r0, r0, #2
add r7, r7, r0
cmp r7, r8
blt arm11kernel_convertcmd_vaddr2phys_lp

arm11kernel_convertcmd_vaddr2phys_end:
add sp, sp, #8
pop {r4, r5, r6, r7, r8, r9, lr}
bx lr
.pool
#endif

#ifdef ENABLE_ARM11KERNEL_PROCSTARTHOOK
arm11kernel_svc73_hook:
mov r5, r0
push {r0, r1, r2, r3, r4, r5, lr}

cpsid i @ disable IRQs

bl arm11kernel_getdebugstateptr
add r4, r0, #0x200

mov r0, r4
bl arm11kernel_waitdebuginfo_magic

ldr r2, =0x3131444c @ Debuginfo type = "LD11".
str r2, [r4, #4]
mov r0, #0x1c
str r0, [r4, #8]

ldr r1, [sp, #4] @ r1=inr1

/*ldr r2, =0x64697073 @ "spider"
ldr r3, =0x7265*/
/*ldr r2, =0x706c64 @ "dlp"
mov r3, #0*/
/*ldr r2, =0x41727443 @ "CtrApp"
ldr r3, =0x7070*/
/*ldr r0, [r1, #0]
cmp r0, r2
ldreq r0, [r1, #4]
cmpeq r0, r3
bne arm11kernel_svc73_hook_end @ Only process this SVC when the process-name is the above.*/

ldr r0, [r1, #0]
str r0, [r4, #20]
ldr r0, [r1, #4]
str r0, [r4, #24]

ldr r0, [r1, #0x28]
ldr r2, [r1, #0x2c]
ldr r3, [r1, #0x30]
add r0, r0, r2
add r0, r0, r3
str r0, [r4, #12] @ Total used pages for .text + .rodata + .data + .bss.

ldr r0, =0xffff9004
ldr r0, [r0]
add r0, r0, #0x54

ldr r3, arm11kernel_patch_fwver
cmp r3, #0x37
addge r0, r0, #8

ldr r1, [sp, #8] @ r1=inr2
bl KProcessmem_getphysicaladdr
str r0, [r4, #16] @ text_mapadr converted to a physical address.

ldr r1, =0x58584148
str r1, [r4]
mov r0, #0
mcr p15, 0, r0, c7, c10, 5
mcr p15, 0, r0, c7, c14, 0

mov r0, r4
bl arm11kernel_waitdebuginfo_magic

arm11kernel_svc73_hook_end:
cpsie i @ enable IRQs
pop {r0, r1, r2, r3, r4, r5, pc}
.pool

/*arm11kernel_svc75_hook:
mov r0, r7
push {r0, r1, r2, r3, r4, r5, lr}

cpsid i @ disable IRQs

bl arm11kernel_getdebugstateptr
add r4, r0, #0x200

mov r0, r4
bl arm11kernel_waitdebuginfo_magic

ldr r2, =0x35375653 @ Debuginfo type = "SV75".
str r2, [r4, #4]
mov r0, #0x14
str r0, [r4, #8]

ldr r1, [sp, #16] @ r1=r4 from hook entry*/

/*ldr r2, =0x64697073 @ "spider"
ldr r3, =0x7265*/
/*ldr r2, =0x706c64 @ "dlp"
mov r3, #0*/
/*ldr r2, =0x41727443 @ "CtrApp"
ldr r3, =0x7070*/
/*ldr r0, [r1, #0x50]
cmp r0, r2
ldreq r0, [r1, #0x54]
cmpeq r0, r3
bne arm11kernel_svc75_hook_end @ Only process this SVC when the process-name is the above.*/

/*ldr r2, [r1, #0x50]
ldr r3, [r1, #0x54]
str r2, [r4, #12]
str r3, [r4, #16]

ldr r1, =0x58584148
str r1, [r4]
mov r0, #0
mcr p15, 0, r0, c7, c10, 5
mcr p15, 0, r0, c7, c14, 0

mov r0, r4
bl arm11kernel_waitdebuginfo_magic

arm11kernel_svc75_hook_end:
cpsie i @ enable IRQs
pop {r0, r1, r2, r3, r4, r5, pc}
.pool*/
#endif

arm11kernel_patchend:
.word 0
#endif

