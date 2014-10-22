.arch armv5te
.fpu softvfp
.text
.arm

.global arm9_stub
.global arm9_stub2
.global init_arm9patchcode3
.type arm9_stub STT_FUNC
.type arm9_stub2 STT_FUNC
.type init_arm9patchcode3 STT_FUNC

#define ENABLE_FIRMBOOT

arm9_stub:
ldr r3, =arm9_debugcode
blx r3
.pool
//.arm

/*#ifdef ENABLE_FIRMBOOT
arm9_stub2:
ldr pc, =arm9_debugcode2
//bx r0
.pool
#endif*/

arm9_debugcode:
push {r0, r1, r2, r3, r4, lr}
sub sp, sp, #12

/*ldr r0, =0x20000000
ldr r1, =0x1000
bl dumpmem*/

ldr r0, =sdarchive_obj
ldr r0, [r0]
mov r3, #1
str r3, [sp, #0]
add r1, sp, #8
str r1, [sp, #4]
mov r1, #4
ldr r2, =firmbin_filepath
mov r3, #0x14
bl openfile

ldr r0, [sp, #8]
cmp r0, #0
beq arm9_debugcode_fail
bl getfilesize
ldr r1, [sp, #16]
str r0, [r1]
mov r2, #0
str r2, [r1, #4]

mov r4, r0
mov r2, r4
ldr r0, [sp, #8]
ldr r1, =0x21000000
mov r3, #0
bl fileread
cmp r0, #0
bne arm9_debugcode_fail

ldr r0, =0x24000000
ldr r1, =0x21000000
mov r2, #0x100
bl memcpy

ldr r0, =0x21000000
bl init_firmlaunch_fwver
cmp r0, #0
beq arm9_debugcode_beginpatch

arm9_debugcode_patchabort:
b arm9_debugcode_patchabort @ Halt firmlaunch when the FIRM isn't recognized by the current system. This is to prevent booting FIRM for the wrong system.

arm9_debugcode_beginpatch:
ldr r0, =0x21000000 @ Only execute this when init_firmlaunch_fwver() successfully determined the FWVER.
bl patch_firm

arm9_debugcode_patchfinish:
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
bne arm9_debugcode_skip_paramsclear

ldr r0, =0x20000000 @ Clear FIRM-launch params.
mov r1, #0
mov r2, #0x1000
bl memset

arm9_debugcode_skip_paramsclear:
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

/*ldr r0, =0x21000000
//mov r1, r4
ldr r1, =0x00400000
bl dumpmem*/

/*ldr r0, =0x203d86c0
mov r1, #0
ldr r2, =0x38400
bl memset*/
/*ldr r0, =0x18000000//=0x20000000//0x203a02b0
ldr r1, =0x13333337
ldr r2, =0x600000//=0x800000//0x38400
bl memset*/

/*mov r0, #0 @ RSA keyslot
mov r1, #2048 @ RSA bitsize
ldr r2, =rsamodulo_slot0 @ modulo
mov r3, #3 @ exponent
blx rsaengine_setpubk*/
mov r2, #0
ldr r0, =0x01ffcd00
ldr r1, [r0]
cmp r1, #0
strne r2, [r0, #0]
strne r2, [r0, #4]
strne r2, [r0, #8]
strne r2, [r0, #12]

add sp, sp, #12
pop {r0, r1, r2, r3, r4, lr}
add lr, lr, #4
bx lr
arm9_debugcode_fail:
ldr r0, =0x20000000
ldr r1, =0xf0f0f0f0
ldr r2, =0x800000
bl memset
arm9_debugcode_failend:
b arm9_debugcode_failend
.pool

#ifdef ENABLE_FIRMBOOT
/*arm9_debugcode2:
push {r0, r1, r2, r3, lr}
ldr r0, =0x20000000//0x203a02b0
//mov r1, #0
ldr r1, =0xc0c0c0c0
ldr r2, =0x400000//0x38400
bl memset
pop {r0, r1, r2, r3, lr}
//bx lr
mov r2, #0
mov r1, r2
ldr pc, =0x80ff914
arm9_debugcode2_end:
b arm9_debugcode2_end
.pool*/

init_arm9patchcode3:
push {r4, r5, lr}

mov r4, r0

mov r0, #2
strb r0, [r2]

mov r5, #0
ldr r2, =0xe5900048
ldr r3, =0xe3500000

init_arm9patchcode3_locatelp0:
ldr r0, [r1, r5]
cmp r0, r2
bne init_arm9patchcode3_locatelp0next
add r5, r5, #4
ldr r0, [r1, r5]
cmp r0, r3
beq init_arm9patchcode3_locatelp0end

init_arm9patchcode3_locatelp0next:
add r5, r5, #4
b init_arm9patchcode3_locatelp0

init_arm9patchcode3_locatelp0end:
add r5, r5, #4
add r1, r1, r5
mov r0, r1

ldr r3, =0xe59d0000

init_arm9patchcode3_locatelp1:
ldr r2, [r1], #4
cmp r2, r3
bne init_arm9patchcode3_locatelp1
sub r1, r1, #4

ldr r2, =arm9_patchcode3_finishjumpadr
str r1, [r2]

adr r1, arm9_patchcode3
adr r2, arm9_patchcode3_end
sub r2, r2, r1

init_arm9patchcode3_cpy:
ldr r3, [r1], #4
str r3, [r0], #4
subs r2, r2, #4
bgt init_arm9patchcode3_cpy

mov r0, r4
mov r1, #0
mov r2, #0x20
bl memset

ldr r0, =0xfff
bic r4, r4, r0
mov r0, r4
ldr r1, =0xc00
bl svcFlushProcessDataCache

mov r0, #0
pop {r4, r5, pc}
.pool

arm9_patchcode3:
/*ldr r2, [sp, #0]
cmp r2, #0
bne arm9_patchcode3_handlesection

ldr r0, =0x10146000
arm9_patchcode3_buttonwait: @ Wait for button X to be pressed.
ldrh r1, [r0]
tst r1, #0x400
bne arm9_patchcode3_buttonwait

arm9_patchcode3_handlesection:*/
ldr r1, =0x21000000
ldr r2, [r7, #0]
add r1, r1, r2
ldr r0, [r7, #4]
ldr r2, [r7, #8]
cmp r2, #0
cmpne r0, #0
beq arm9_patchcode3_copyend

arm9_patchcode3_copylp:
ldr r3, [r1], #4
str r3, [r0], #4
subs r2, r2, #4
bgt arm9_patchcode3_copylp

arm9_patchcode3_copyend:
ldr r0, =arm9_patchcode3_finishjumpadr
ldr r0, [r0]
bx r0
.pool

arm9_patchcode3_end:
.word 0

init_firmlaunch_fwver: @ Use the u32 from the first word of the FIRM RSA signature to determine the FIRMLAUNCH_FWVER, via the array @ FIRM_sigword0_array.
push {r4, r5, r6, r7, r8, lr}
mov r4, #0
add r0, r0, #0x100
ldr r0, [r0]

ldr r3, =RUNNINGFWVER
ldr r3, [r3]
lsr r3, r3, #30
and r3, r3, #1
mov r8, r3
lsl r8, r8, #30
cmp r3, #0
bne init_firmlaunch_fwver_new3ds

ldr r5, =FIRM_sigword0_array
ldr r7, =FIRM_contentid_versions
ldr r6, =FIRM_contentid_totalversions
b init_firmlaunch_fwver_start

init_firmlaunch_fwver_new3ds:
ldr r5, =NEW3DS_sigword0_array
ldr r7, =NEW3DS_FIRM_versions
ldr r6, =NEW3DS_totalversions

init_firmlaunch_fwver_start:
ldr r6, [r6]

init_firmlaunch_fwver_lp:
ldr r2, [r5, r4, lsl #2]
cmp r0, r2
beq init_firmlaunch_fwver_lpend
add r4, r4, #1
cmp r4, r6
blt init_firmlaunch_fwver_lp

mov r0, #1
b init_firmlaunch_fwver_end

init_firmlaunch_fwver_lpend:
ldrb r3, [r7, r4]
orr r3, r3, r8
ldr r2, =FIRMLAUNCH_FWVER
str r3, [r2]

mov r0, #0

init_firmlaunch_fwver_end:
pop {r4, r5, r6, r7, r8, pc}
.pool

#endif

firmbin_filepath:
.hword 0x2F, 0x66, 0x69, 0x72, 0x6D, 0x2E, 0x62, 0x69, 0x6E, 0x00 //UTF-16 "/firm.bin"
.align 2

arm9_patchcode3_finishjumpadr:
.word 0

