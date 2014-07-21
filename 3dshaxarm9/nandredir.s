#ifdef ENABLENANDREDIR

.arch armv5te
.fpu softvfp
.text
.arm

.global patch_nandredir
.global patch_nandredir_autolocate
.type patch_nandredir STT_FUNC
.type patch_nandredir_autolocate STT_FUNC

.thumb
.align 2
arm9_nandredir_stub:
ldr r7, =arm9_nandredir_code
blx r7
.pool

arm9_nandredir_stub2:
ldr r7, =arm9_nandredir_readcodefinal
blx r7
.pool

arm9_nandredir_stub3:
ldr r7, =arm9_nandredir_writecodefinal
blx r7
.pool

/*arm9_nandredir_stub4:
ldr r0, =arm9_nandredir_debugcode
bx r0
.pool*/

.arm
/*arm9_nandredir_dumpdebug:
push {r4, lr}
ldr r0, arm9_nandredir_code_savaddr
mov r3, #2
ldr r4, =cardWriteEeprom
blx r4
pop {r4, pc}
.pool*/

arm9_nandredir_code:
push {r0, r1, r2, r3, r4, r5, r6, r7, ip, lr}
sub sp, sp, #0x40
mov r5, r0
ldr r2, =0x101
ldr ip, [r5, #4]
ldr ip, [ip, #32]
cmp ip, r2 @ 0x101=NAND device, 0x100=SD device
mov r6, #0
bne arm9_nandredir_code_end

/*ldr r2, arm9_nandredir_code_savaddr
cmp r2, #0x10000
bge arm9_nandredir_code_debugend*/

/*mov r0, sp
add r1, sp, #0x40
mov r2, #0x20
arm9_nandredir_code_debugcpy:
ldr r3, [r1], #4
str r3, [r0], #4
subs r2, r2, #4
bgt arm9_nandredir_code_debugcpy

ldr r3, [r5, #8]
str r3, [sp, #4]
ldr r1, [sp, #0x80]
str r1, [sp, #0x1c]*/

//add r0, sp, #0x40
//mov r1, #0x20
//bl dumpmem
//bl arm9_nandredir_dumpdebug

/*ldr r0, arm9_nandredir_code_savaddr
ldr r2, =0x10000
add r0, r0, r2
str r0, arm9_nandredir_code_savaddr

arm9_nandredir_code_debugend:*/

ldr r0, =0x14f0
ldr r1, [r5, #4] @ Change the object ptr to the SD one.
add r1, r1, r0
ldr r2, =NANDREDIR_SECTORNUM @ Sector# of the NAND image "partition" on the SD card.
ldr r2, [r2]
ldr r3, [r5, #8]
add r3, r3, r2
str r3, [r5, #8]
str r1, [r5, #4]
mov r6, #1

arm9_nandredir_code_end:
str r6, arm9_nandredir_code_hookflag
add sp, sp, #0x40
pop {r0, r1, r2, r3, r4, r5, r6, r7, ip, lr}
mov r4, r0
mov r5, r1
mov r7, r2
mov r6, r3
add lr, lr, #4
bx lr
.pool

arm9_nandredir_rwcodefinal:
push {r0, r1, r2, r3, r4, r5, r6, ip, lr}
ldr r1, arm9_nandredir_code_hookflag
mov r2, #0
cmp r1, r2
beq arm9_nandredir_rwcodefinal_end
str r2, arm9_nandredir_code_hookflag

ldr r2, =NANDREDIR_SECTORNUM
ldr r2, [r2]
ldr r1, [r4, #8]
sub r1, r1, r2
str r1, [r4, #8]
ldr r2, =0x14f0
ldr r1, [r4, #4]
sub r1, r1, r2
str r1, [r4, #4]

/*ldr r1, arm9_nandredir_code_savaddr
cmp r1, #0x10000
bge arm9_nandredir_rwcodefinal_end

//sub r1, sp, #0x200
//mov r2, #0x200
mov r1, sp
mov r2, #0x20
//ldr r1, =0x01ff8000
//ldr r2, =0x8000
//bl arm9_nandredir_dumpdebug

ldr r1, arm9_nandredir_code_savaddr
//ldr r2, =0x8000
mov r2, #0x20
add r1, r1, r2
str r1, arm9_nandredir_code_savaddr*/

arm9_nandredir_rwcodefinal_end:
pop {r0, r1, r2, r3, r4, r5, r6, ip, pc}
.pool

arm9_nandredir_readcodefinal:
add r2, #12
mov r0, ip
ldr r5, =arm9_nandredir_readcodefinal_jumpaddr
ldr r5, [r5]
blx r5
bl arm9_nandredir_rwcodefinal
pop {r1, r2, r3, r4, r5, r6, r7, pc}
.pool

arm9_nandredir_writecodefinal:
add r2, #12
mov r0, ip
ldr r5, =arm9_nandredir_writecodefinal_jumpaddr
ldr r5, [r5]
blx r5
bl arm9_nandredir_rwcodefinal
pop {r1, r2, r3, r4, r5, r6, r7, pc}
.pool

/*arm9_nandredir_debugcode:
cmp r1, #0
moveq r0, #0
bxeq lr

ldr r0, =0x20001000
str r1, [r0], #4
//sub r1, sp, #0x200
//mov r1, sp
ldr r1, =0x10006000
mov r2, #0x200

arm9_nandredir_debugcode_cpylp:
ldr r3, [r1], #4
str r3, [r0], #4
subs r2, r2, #4
bgt arm9_nandredir_debugcode_cpylp

push {r0, r1, r2, r3, r4, r5, r6, lr}

ldr r1, arm9_nandredir_code_savaddr
cmp r1, #0x10000
bge arm9_nandredir_debugcode_end

ldr r1, =0x20001000
add r1, r1, #4
ldr r2, =0x200
bl arm9_nandredir_dumpdebug

ldr r1, arm9_nandredir_code_savaddr
ldr r2, =0x200
add r1, r1, r2
str r1, arm9_nandredir_code_savaddr

arm9_nandredir_debugcode_end:
pop {r0, r1, r2, r3, r4, r5, r6, lr}

ldr r0, =0x20001000
ldr r1, [r0]

ldr r0, =0x40200000
tst r1, r0
ldr pc, =0x8078a99
.pool*/

patch_nandredir:
push {r0, r1, r2, r3, r4, r5, lr}
adr r2, arm9_nandredir_stub
add r0, r0, #0x2//0x2a
add r1, r1, #0x2//0x2a
ldrh r3, [r2, #0]
strh r3, [r0, #0]
strh r3, [r1, #0]
ldrh r3, [r2, #2]
strh r3, [r0, #2]
strh r3, [r1, #2]
ldrh r3, [r2, #4]
strh r3, [r0, #4]
strh r3, [r1, #4]
ldrh r3, [r2, #6]
strh r3, [r0, #6]
strh r3, [r1, #6]

add r0, r0, #0x34
add r1, r1, #0x34
mov r4, r0
mov r5, r1

ldr r0, [sp, #8]
add r0, r0, #0x36
add r0, r0, #4
add r1, r4, #4
ldr r1, [r1]
bl parse_branch_thumb
ldr r1, =arm9_nandredir_readcodefinal_jumpaddr
str r0, [r1]

ldr r0, [sp, #12]
add r0, r0, #0x36
add r0, r0, #4
add r1, r5, #4
ldr r1, [r1]
bl parse_branch_thumb
ldr r1, =arm9_nandredir_writecodefinal_jumpaddr
str r0, [r1]

mov r0, r4
mov r1, r5
adr r2, arm9_nandredir_stub2
adr r3, arm9_nandredir_stub3
ldrh r4, [r2, #0]
strh r4, [r0, #0]
ldrh r4, [r3, #0]
strh r4, [r1, #0]
ldrh r4, [r2, #2]
strh r4, [r0, #2]
ldrh r4, [r3, #2]
strh r4, [r1, #2]
ldrh r4, [r2, #4]
strh r4, [r0, #4]
ldrh r4, [r3, #4]
strh r4, [r1, #4]
ldrh r4, [r2, #6]
strh r4, [r0, #6]
ldrh r4, [r3, #6]
strh r4, [r1, #6]

mov r2, #0x44
ldr r1, [sp, #0]
ldr r0, =0xffff8001
svc 0x54 @ svcFlushProcessDataCache

mov r2, #0x44
ldr r1, [sp, #4]
ldr r0, =0xffff8001
svc 0x54 @ svcFlushProcessDataCache
pop {r0, r1, r2, r3, r4, r5, pc}
.pool

patch_nandredir_autolocate:
push {r4, r5, r6, r7, lr}
sub sp, sp, #8

mov r5, #0
ldr r4, =arm9_nandredir_patch_cmpdata
mov r6, r2
mov r7, r0

patch_nandredir_autolocate_lp:
ldrh r2, [r0, #0]
ldrh r3, [r4, #0]
cmp r2, r3
bne patch_nandredir_autolocate_lpend
ldrh r2, [r0, #2]
ldrh r3, [r4, #2]
cmp r2, r3
bne patch_nandredir_autolocate_lpend
ldrh r2, [r0, #4]
ldrh r3, [r4, #4]
cmp r2, r3
bne patch_nandredir_autolocate_lpend
ldrh r2, [r0, #6]
ldrh r3, [r4, #6]
cmp r2, r3
bne patch_nandredir_autolocate_lpend
ldrh r2, [r0, #8]
ldrh r3, [r4, #8]
cmp r2, r3
bne patch_nandredir_autolocate_lpend
ldrh r2, [r0, #0xa]
ldrh r3, [r4, #0xa]
cmp r2, r3
bne patch_nandredir_autolocate_lpend
ldrh r2, [r0, #0xc]
ldrh r3, [r4, #0xc]
cmp r2, r3
bne patch_nandredir_autolocate_lpend

str r0, [sp, r5]
add r5, r5, #4
cmp r5, #8
beq patch_nandredir_autolocate_lpfinish

patch_nandredir_autolocate_lpend:
add r0, r0, #2
subs r1, r1, #2
bgt patch_nandredir_autolocate_lp

mov r0, #1
b patch_nandredir_autolocate_end

patch_nandredir_autolocate_lpfinish:
ldr r0, [sp, #4]
ldr r1, [sp, #0]
sub r2, r0, r7
sub r3, r1, r7
add r2, r2, r6
add r3, r3, r6
bl patch_nandredir

mov r0, #0

patch_nandredir_autolocate_end:
add sp, sp, #8
pop {r4, r5, r6, r7, pc}
.pool

arm9_nandredir_code_savaddr:
.word 0
arm9_nandredir_code_hookflag:
.word 0
arm9_nandredir_code_initflag:
.word 0

arm9_nandredir_readcodefinal_jumpaddr:
.word 0
arm9_nandredir_writecodefinal_jumpaddr:
.word 0

arm9_nandredir_patch_cmpdata: @ This is the first 0xe-bytes of the functions which are patched for nand-redir.
.hword 0xb5fe, 0x0004, 0x000d, 0x0017, 0x001e, 0x05c8, 0xd001

#endif

