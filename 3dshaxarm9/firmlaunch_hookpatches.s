.arch armv5te
.fpu softvfp
.text
.arm

.global firmlaunch_hook0_stub
.global init_firmlaunch_hook1
//.global firmlaunch_swprintfhook
.type firmlaunch_hook0_stub STT_FUNC
.type init_firmlaunch_hook1 STT_FUNC
//.type firmlaunch_swprintfhook STT_FUNC

firmlaunch_hook0_stub:
ldr r3, =firmlaunch_hook0
blx r3
.pool

#ifndef DISABLE_FIRMLAUNCH_LOADSD
firmlaunch_loadfirmsd: @ r0 = FIRM tidlow u16, r1 = u64* filesize out
push {r0, r1, r2, r3, r4, lr}
sub sp, sp, #12

mov r4, #0

ldr r0, =sdarchive_obj
ldr r0, [r0]
mov r3, #1
str r3, [sp, #0]
add r1, sp, #8
str r1, [sp, #4]
mov r1, #4

//lower half word so it'll work on n3ds too
ldrh r2, [sp, #12]
mov r3, #0x100
add r3, r3, #0x2 // TWL_FIRM tidlow
cmp r2, r3
ldreq r2, =twlfirmbin_filepath
beq firmlaunch_loadfirmsd_openfile

cmp r2, #0x2
mvnne r4, #1
bne firmlaunch_loadfirmsd_end

ldr r2, =firmbin_filepath

firmlaunch_loadfirmsd_openfile:
mov r3, #0x14
bl openfile
mov r4, r0
cmp r4, #0
bne firmlaunch_loadfirmsd_end
mvn r4, #0

ldr r0, [sp, #8]
cmp r0, #0
beq firmlaunch_loadfirmsd_end
bl getfilesize
ldr r1, [sp, #16]
str r0, [r1]
mov r2, #0
str r2, [r1, #4]

mov r2, r0
ldr r0, [sp, #8]
ldr r1, =0x21000000
mov r3, #0
bl fileread
mov r4, r0

ldr r0, [sp, #8]
bl closefile

firmlaunch_loadfirmsd_end:
mov r0, r4
add sp, sp, #12
add sp, sp, #16
pop {r4, pc}
.pool
#endif

#ifdef ENABLE_FIRMLAUNCH_LOADNAND
firmlaunch_loadfirmnand: @ r0 = _this, r1 = u64* filesize out, r2 = getfilesize funcptr
push {r0, r1, r2, r3, r4, lr}
sub sp, sp, #12

blx r2 @ Call the getfilesize vtable funcptr, then execute infinite loop if it fails.
cmp r0, #0
bne firmlaunch_loadfirmnand_end

ldr r0, [sp, #12] @ _this
mov r1, sp @ u32* total actual read data
ldr r2, =0x21000000 @ outbuf
ldr r3, [sp, #16]
ldr r3, [r3] @ size
ldr r4, [r0]
ldr r4, [r4, #0x28]
blx r4
cmp r0, #0
bne firmlaunch_loadfirmnand_end

ldr r2, [sp, #0] @ read size
ldr r3, [sp, #16]
ldr r3, [r3] @ filesize
cmp r2, r3
mvnne r0, #0

firmlaunch_loadfirmnand_end:
add sp, sp, #12
add sp, sp, #16
pop {r4, pc}
.pool

/*firmlaunch_swprintfhook://This code was intended to have proc9 open "firm0:" instead of the exefs .firm, but that somehow breaks firmlaunch even when firmlaunch_loadfirmnand() wasn't even executed.
mov r3, #0xe
ldr r2, =firm0device

firmlaunch_swprintfhook_cpylp:
ldrh r1, [r2], #2
strh r1, [r0], #2
subs r3, r3, #2
bgt firmlaunch_swprintfhook_cpylp
bx lr
.pool*/
#endif

firmlaunch_hook0:
push {r0, r1, r2, r3, r4, r5, lr}
sub sp, sp, #12

ldrh r2, [sp, #12+4*7]
mov r3, #0x100
add r3, r3, #0x2 // TWL_FIRM tidlow
cmp r2, r3
moveq r5, #1
movne r5, #0
add r3, r3, #0x100 @ AGB_FIRM
cmp r2, r3
moveq r5, #1

ldr r3, =FIRMLAUNCH_CLEARPARAMS
ldr r3, [r3]
cmp r3, #1
cmpeq r2, #0x3
moveq r2, #0x2
strh r2, [sp, #12+4*7]

firmlaunch_hook0_firmload:
#ifndef DISABLE_FIRMLAUNCH_LOADSD
mov r0, r2
ldr r1, [sp, #16]
bl firmlaunch_loadfirmsd
cmp r0, #0
beq firmlaunch_hook0_copyheader
#endif

#ifdef ENABLE_FIRMLAUNCH_LOADNAND
#ifdef ENABLENANDREDIR//Refuse to load from NAND for the second firmlaunch when nandredir is enabled, since that would mean loading FIRM from physnand with a potentially newer sd-nandimage which could fail to boot with older FIRM.
ldr r3, =FIRMLAUNCH_CLEARPARAMS
ldr r3, [r3]
cmp r3, #1
mvneq r0, #5
beq firmlaunch_hook0_fail
#endif

ldr r0, [sp, #12]
ldr r1, [sp, #16]
ldr r2, [sp, #20]
bl firmlaunch_loadfirmnand
#endif

cmp r0, #0
bne firmlaunch_hook0_fail

#ifdef DISABLE_FIRMLAUNCH_LOADSD
#ifndef ENABLE_FIRMLAUNCH_LOADNAND
#error "No FIRMLAUNCH load-type(SD/NAND) is enabled at all."
#endif
#endif

firmlaunch_hook0_copyheader:
ldr r0, =firmheader_address
ldr r0, [r0]
ldr r1, =0x21000000
mov r2, #0x100
bl memcpy

cmp r5, #1
beq firmlaunch_hook0_skip_paramsclear @ skip patches and clear param if twl_firm/agb_firm.

#ifndef DISABLE_MATCHINGFIRM_HWCHECK
ldr r0, =0x21000000
bl firm_gethwtype
mvn r1, #0
cmp r0, r1
beq firmlaunch_hook0_patchabort @ Check for error from firm_gethwtype().

ldr r1, =RUNNINGFWVER
ldr r1, [r1]
lsr r1, r1, #30
and r1, r1, #1

cmp r0, r1
bne firmlaunch_hook0_patchabort @ Return error when the hw type returned by firm_gethwtype() doesn't match the current one from RUNNINGFWVER.
#endif

ldr r0, =0x21000000
ldrh r1, [sp, #12+4*7] @ FIRM tidlow
bl init_firmlaunch_fwver
cmp r0, #0
beq firmlaunch_hook0_beginpatch

firmlaunch_hook0_patchabort:
b firmlaunch_hook0_patchabort @ Halt firmlaunch when fwver init failed, or for fail with the above firm_gethwtype() code.

firmlaunch_hook0_beginpatch:
ldr r0, =0x21000000 @ Only execute this when init_firmlaunch_fwver() successfully determined the FWVER.
bl patch_firm

firmlaunch_hook0_patchfinish:
/*ldr r0, =0x20000440
ldr r1, =0x4B464445
ldr r2, =0x00048004
mov r3, #0
str r1, [r0]
str r2, [r0, #4]
str r3, [r0, #8]
mov r3, #1
str r3, [r0, #0x20]*/

ldr r0, =FIRMLAUNCH_CLEARPARAMS
ldr r0, [r0]
cmp r0, #1
bne firmlaunch_hook0_skip_paramsclear

ldr r0, =0x20000000 @ Clear FIRM-launch params.
mov r1, #0
mov r2, #0x1000
bl memset

firmlaunch_hook0_skip_paramsclear:
@ Enable the following block to have NS launch the gamecard title, which bypasses the region-lock. Note that the system will fail to boot with this enabled if no gamecard is inserted.
/*ldr r0, =0x20000440
mov r1, #0
mov r2, #0
mov r3, #2
str r1, [r0, #0]
str r2, [r0, #4]
str r3, [r0, #8]
add r0, r0, #0x20
ldr r1, [r0]
orr r1, r1, #1
str r1, [r0] @ Set FIRMlaunch params so that NS launches the gamecard title.

ldr r1, =0x20000438
ldr r2, =0xffff
str r2, [r1]

ldr r1, =0x2000043c
mov r0, #0
str r0, [r1]

ldr r0, =0x20000400
mov r1, #0x140
blx CalcCRC32
ldr r1, =0x2000043c
str r0, [r1]*/

mov r2, #0
ldr r0, =0x01ffcd00
ldr r1, [r0]
cmp r1, #0
strne r2, [r0, #0]
strne r2, [r0, #4]
strne r2, [r0, #8]
strne r2, [r0, #12]

#ifdef ENABLE_LOADSD_AESKEYS
ldr r0, =0x20F00000
bl loadsd_aeskeys
#endif

add sp, sp, #12
pop {r0, r1, r2, r3, r4, r5, lr}
add lr, lr, #4
bx lr
firmlaunch_hook0_fail:
b firmlaunch_hook0_fail
.pool

init_firmlaunch_hook1:
push {r4, r5, r6, lr}

mov r4, r0
mov r6, r1

mov r0, #2
strb r0, [r2]
ldr r0, [r2, #8]
mov r1, r0
lsr r1, r1, #24
cmp r1, #0x01
bne init_firmlaunch_hook1_writefirmhdradr

ldr r3, =RUNNINGFWVER @ When FIRM-header is located in ITCM(>=v9.5 running FIRM), change it to endofarm9mem-0x200. FIRM-launching with these hooks is broken when the FIRM-header is located in ITCM.
ldr r3, [r3]
lsr r3, r3, #30
and r3, r3, #1
cmp r3, #0
ldreq r0, =0x08100000
ldrne r0, =0x08180000
sub r0, r0, #0x200

str r0, [r2, #8]

init_firmlaunch_hook1_writefirmhdradr:
ldr r2, =firmheader_address
str r0, [r2]

mov r1, r6
mov r5, #0
ldr r2, =0xe5900048
ldr r3, =0xe3500000

init_firmlaunch_hook1_locatelp0:
ldr r0, [r1, r5]
cmp r0, r2
bne init_firmlaunch_hook1_locatelp0next
add r5, r5, #4
ldr r0, [r1, r5]
cmp r0, r3
beq init_firmlaunch_hook1_locatelp0end

init_firmlaunch_hook1_locatelp0next:
add r5, r5, #4
b init_firmlaunch_hook1_locatelp0

init_firmlaunch_hook1_locatelp0end:
add r5, r5, #4
add r1, r1, r5
mov r0, r1

ldr r3, =0xe59d0000

init_firmlaunch_hook1_locatelp1:
ldr r2, [r1], #4
cmp r2, r3
bne init_firmlaunch_hook1_locatelp1
sub r1, r1, #4

ldr r2, =firmlaunch_hook1_finishjumpadr
str r1, [r2]

adr r1, firmlaunch_hook1
adr r2, firmlaunch_hook1_end
sub r2, r2, r1

init_firmlaunch_hook1_cpy:
ldr r3, [r1], #4
str r3, [r0], #4
subs r2, r2, #4
bgt init_firmlaunch_hook1_cpy

mov r0, r4
mov r1, #0
mov r2, #0x20
bl memset

ldr r0, =0xfff
bic r4, r4, r0
mov r0, r4
ldr r1, =0xc00
mov r2, r1
mov r1, r0
ldr r0, =0xffff8001
bl svcFlushProcessDataCache

mov r0, #0
pop {r4, r5, r6, pc}
.pool

firmlaunch_hook1:
/*ldr r2, [sp, #0]
cmp r2, #0
bne firmlaunch_hook1_handlesection

ldr r0, =0x10146000
firmlaunch_hook1_buttonwait: @ Wait for button X to be pressed.
ldrh r1, [r0]
tst r1, #0x400
bne firmlaunch_hook1_buttonwait

firmlaunch_hook1_handlesection:*/
ldr r1, =0x21000000
ldr r2, [r7, #0]
add r1, r1, r2
ldr r0, [r7, #4]
ldr r2, [r7, #8]
cmp r2, #0
cmpne r0, #0
beq firmlaunch_hook1_copyend

firmlaunch_hook1_copylp:
ldr r3, [r1], #4
str r3, [r0], #4
subs r2, r2, #4
bgt firmlaunch_hook1_copylp

firmlaunch_hook1_copyend:
ldr r0, =firmlaunch_hook1_finishjumpadr
ldr r0, [r0]
bx r0
.pool

firmlaunch_hook1_end:
.word 0

init_firmlaunch_fwver:
push {r1, r4, r5, r6, r7, r8, lr}

ldr r3, =RUNNINGFWVER
ldr r3, [r3]
lsr r3, r3, #30
and r3, r3, #1
mov r8, r3
lsl r8, r8, #30

bl firm_arm11kernel_getminorversion_firmimage
mvn r1, #0
cmp r0, r1
beq init_firmlaunch_fwver_end

orr r3, r0, r8
ldrh r0, [sp]
lsl r0, r0, #8 @ FIRM tidlow
orr r3, r3, r0
ldr r2, =FIRMLAUNCH_FWVER
str r3, [r2]

mov r0, #0

init_firmlaunch_fwver_end:
add sp, sp, #4
pop {r4, r5, r6, r7, r8, pc}
.pool

firmbin_filepath:
#ifndef ALTSD_FIRMPATH
.hword 0x2F, 0x66, 0x69, 0x72, 0x6D, 0x2E, 0x62, 0x69, 0x6E, 0x00 //UTF-16 "/firm.bin"
#else
.string16 "/3dshax_firm.bin"
#endif
.align 2

twlfirmbin_filepath:
.hword 0x2F, 0x74, 0x77, 0x6C, 0x5F, 0x66, 0x69, 0x72, 0x6D, 0x2E, 0x62, 0x69, 0x6E, 0x00 //UTF-16 "/twl_firm.bin"
.align 2

firmlaunch_hook1_finishjumpadr:
.word 0

firmheader_address:
.word 0

#ifdef ENABLE_FIRMLAUNCH_LOADNAND
.global proc9_swprintf_addr
proc9_swprintf_addr:
.word 0

firm0device:
.string16 "firm0:"
#endif

