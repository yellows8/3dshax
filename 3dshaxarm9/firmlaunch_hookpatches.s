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

#ifdef ENABLE_FIRMBOOT
arm9_stub2:
ldr pc, =arm9_debugcode2
//bx r0
.pool
#endif

arm9_debugcode:
push {r0, r1, r2, r3, r4, lr}
sub sp, sp, #12

/*ldr r0, =0x20000000
ldr r1, =0x1000
bl dumpmem*/

//ldr r0, =0x08028000
//ldr r1, =0xD8000
//bl dumpmem
/*ldr r0, =0x20703000
ldr r1, [r0]
add r0, r0, #4
add r0, r0, r1
ldr r2, [r4, #12]
str r2, [r0]

ldr r2, [r4, #8]
ldr r2, [r2]
ldr r2, [r2, #0x6c]
str r2, [r0, #4]

ldr r0, =0x20703000
ldr r1, [r0]
add r1, r1, #8
str r1, [r0]

ldr r0, =0x20703000
ldr r1, [r0]
add r0, r0, #4
add r0, r0, r1
mov r1, r5
mov r2, r6
bl memcpy
ldr r0, =0x20703000
ldr r1, [r0]
add r1, r1, r6
str r1, [r0]*/

/*ldr r0, =0x20000000
mov r1, #0x400000
bl dumpmem*/

/*mov r0, #0 @ RSA keyslot
mov r1, #2048 @ RSA bitsize
ldr r2, =rsamodulo_slot0 @ modulo
mov r3, #3 @ exponent
blx rsaengine_setpubk*/
//mvn r2, #0
mov r2, #0
ldr r0, =0x01ffcd00
ldr r1, [r0]
cmp r1, #0
strne r2, [r0, #0]
strne r2, [r0, #4]
strne r2, [r0, #8]
strne r2, [r0, #12]

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

ldr r0, =0x21000000
bl init_firmlaunch_fwver
cmp r0, #0
bne arm9_debugcode_skip_patch

ldr r0, =0x21000000 @ Only execute this when init_firmlaunch_fwver() successfully determined the FWVER.
bl patch_firm

arm9_debugcode_skip_patch:
ldr r0, =0x24000000
ldr r1, =0x21000000
mov r2, #0x100
bl memcpy

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
arm9_debugcode2:
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
.pool

init_arm9patchcode3:
push {r4, r5, lr}

mov r5, #0
ldr r0, =RUNNINGFWVER
ldr r1, =0x2E
ldr r0, [r0]
cmp r0, r1
moveq r5, #4

ldr r0, =0x80ff6f8
add r0, r0, r5
adr r1, arm9_patchcode3
adr r2, arm9_patchcode3_end
sub r2, r2, r1
mov r4, r2

init_arm9patchcode3_cpy:
ldr r3, [r1], #4
str r3, [r0], #4
subs r2, r2, #4
bgt init_arm9patchcode3_cpy

ldr r0, =0x080ffb10
add r0, r0, r5
mov r1, #0
mov r2, #0x20
bl memset

ldr r0, =0x080ff9c4
add r0, r0, r5
mov r1, #2
strb r1, [r0]

mov r0, r4
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
mov r2, #0
ldr r0, =RUNNINGFWVER
ldr r1, =0x2E
ldr r0, [r0]
cmp r0, r1
moveq r2, #4

ldr r0, =0x80ff8f8
add r0, r0, r2
bx r0
.pool

arm9_patchcode3_end:
.word 0

init_firmlaunch_fwver: @ Use the u32 from the first word of the FIRM RSA signature to determine the FIRMLAUNCH_FWVER, via the array @ FIRM_sigword0_array.
push {r4, lr}
mov r4, #0
add r0, r0, #0x100
ldr r0, [r0]
ldr r3, =FIRM_contentid_totalversions
ldr r3, [r3]
ldr r1, =FIRM_sigword0_array

init_firmlaunch_fwver_lp:
ldr r2, [r1, r4, lsl #2]
cmp r0, r2
beq init_firmlaunch_fwver_lpend
add r4, r4, #1
cmp r4, r3
blt init_firmlaunch_fwver_lp

mov r0, #1
b init_firmlaunch_fwver_end

init_firmlaunch_fwver_lpend:
ldr r3, =FIRM_contentid_versions
ldrb r3, [r3, r4]
ldr r2, =FIRMLAUNCH_FWVER
str r3, [r2]

mov r0, #0

init_firmlaunch_fwver_end:
pop {r4, pc}
.pool

#endif

firmbin_filepath:
.hword 0x2F, 0x66, 0x69, 0x72, 0x6D, 0x2E, 0x62, 0x69, 0x6E, 0x00 //UTF-16 "/firm.bin"

.align 2

