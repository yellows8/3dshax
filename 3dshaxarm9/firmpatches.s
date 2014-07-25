.arch armv5te
.fpu softvfp
.text
.arm

.global patch_firm
.global patchfirm_arm9section
.type patch_firm STT_FUNC
.type patchfirm_arm9section STT_FUNC

patch_firm: @ r0 = addr of entire FIRM
push {r4, r5, r6, lr}
mov r4, r0

mov r5, #0
patchfirm_sectionlp:
mov r3, #0x30
mul r1, r5, r3
add r1, r1, #0x40
add r1, r1, r4

ldr r3, [r1, #0x8]
cmp r3, #0
beq patchfirm_sectionlp_end @ branch when sectionsize == 0

ldr r3, [r1, #0x0]
add r0, r4, r3
mov r2, r4

ldr r3, [r1, #0xc]
cmp r3, #0
bne patchfirm_sectionlp_checkarm11 @ branch when the section type is not arm9.

ldr r1, [r1, #0x8]
bl patchfirm_arm9section
b patchfirm_sectionlp_end

patchfirm_sectionlp_checkarm11:
ldr r1, [r1, #0x8]
cmp r5, #0
bleq patchfirm_arm11section_modules
cmp r5, #1
bleq patchfirm_arm11section_kernel

patchfirm_sectionlp_end:
add r5, r5, #1
cmp r5, #4
blt patchfirm_sectionlp
pop {r4, r5, r6, pc}

patchfirm_arm9section: @ r0 = address of FIRM section in memory, in the FIRM binary. r1 = FIRM section size. r2 = FIRM header address.
push {r4, r5, r6, r7, lr}
sub sp, sp, #4

mov r4, r1

/*#ifdef ENABLENANDREDIR
#ifdef ENABLE_LOADA9_x01FFB800
ldr r1, =FIRMLAUNCH_RUNNINGTYPE
ldr r1, [r1]
cmp r1, #3
beq patchfirm_arm9section_locateprocess9_start

ldr r5, =FIRMLAUNCH_FWVER
ldr r5, [r5]
cmp r5, #0x2E
cmpne r5, #0x30
cmpne r5, #0x37
bne patchfirm_arm9section_locateprocess9_start
ldr r1, =0x0801b43c
ldr r2, =0x08006800
sub r2, r1, r2
add r2, r2, r0
mov r3, #0
str r3, [r2, #0] @ Patch the arm9 kernel crt0 code so that it always does the TWL console-unique keyslot keydata-init.
ldr r3, =0xe3a01000 @ "mov r1, #0"
cmp r5, #0x37
ldrge r3, =0xe3a02000 @ "mov r2, #0"
str r3, [r2, #16]
#endif
#endif*/

patchfirm_arm9section_locateprocess9_start:
ldr r1, =0x636f7250 @ "Process9"
ldr r2, =0x39737365

patchfirm_arm9section_locateprocess9: @ Locate the "Process9" exheader.
ldr r3, [r0, #0]
cmp r3, r1
ldreq r3, [r0, #4]
cmpeq r3, r2
beq patchfirm_arm9section_locateprocess9_finish
sub r4, r4, #8
add r0, r0, #8
cmp r4, #0
bgt patchfirm_arm9section_locateprocess9
b patchfirm_arm9section_finish

patchfirm_arm9section_locateprocess9_finish:
ldr r4, [r0, #0x10] @ r4 = Process9 .text addr
ldr r7, [r0, #0x18] @ r7 = Process9 .text size
add r0, r0, #0xa00 @ r0/r5 = addr of Process9 .code
mov r5, r0

/*ldr r1, =firmpatch_begin
ldr r2, =firmpatch_end
sub r2, r2, r1
sub r2, r2, #4
ldr r3, [r1], #4*/

/*ldr r3, =0x08078a90
sub r3, r3, r4
add r3, r3, r0
ldr r1, =arm9_nandredir_stub4
ldr r2, [r1, #0]
str r2, [r3, #0]
ldr r2, [r1, #4]
str r2, [r3, #4]*/

ldr r1, =FIRMLAUNCH_FWVER
ldr r1, [r1]
mov r0, #0
cmp r1, #0x1F
ldreq r0, =0x08087250
cmp r1, #0x2E
ldreq r0, =0x08085cc8
cmp r1, #0x30
ldreq r0, =0x08085ccc
cmp r1, #0x37
ldreq r0, =0x08085fac
ldr r2, =0x29
add r3, r2, #1
cmp r1, r2
cmpne r1, r3
ldreq r0, =0x08085c80
cmp r0, #0
beq patchfirm_arm9section_L0

ldr r1, =proc9_waitfirmevent_hook
ldr r2, =0xe51ff004
ldr r3, =proc9_waitfirmevent_hook_patchaddr
str r0, [r3]
sub r0, r0, r4
add r0, r0, r5
str r2, [r0] @ "ldr pc, [pc, #-4]"
str r1, [r0, #4]

patchfirm_arm9section_L0:
/*#if FIRMLAUNCH_FWVER == 0x1F
ldr r0, =0x08029004
#elif FIRMLAUNCH_FWVER == 0x2E
ldr r0, =0x080292b8
#endif
sub r0, r0, r4
add r0, r0, r5
ldr r1, =proc9_fsdeviceinit_hookstub
ldr r2, [r1, #0]
str r2, [r0, #0]
ldr r2, [r1, #4]
str r2, [r0, #4]*/

ldr r1, =FIRMLAUNCH_FWVER
ldr r1, [r1]
mov r0, #0
cmp r1, #0x1F
ldreq r0, =0x0804d5c0 @ Patch the RSA verification function used for certs(tmd/tik/cert-chain/...), so that it returns 0 instead of -2011 for invalid signature.
cmp r1, #0x2E
ldreq r0, =0x080630bc
cmp r1, #0x30
ldreq r0, =0x080630c0
cmp r1, #0x37
ldreq r0, =0x080632bc
cmp r0, #0
beq patchfirm_arm9section_L1

ldr r1, =0x2000
ldr r2, =0x4770
sub r0, r0, r4
add r0, r0, r5
strh r1, [r0] @ "mov r0, #0"

patchfirm_arm9section_L1:
ldr r1, =FIRMLAUNCH_FWVER
ldr r1, [r1]
mov r0, #0
cmp r1, #0x1F
ldreq r0, =0x0805fac0 @ Patch rsa_verifysignature, so that the very beginning of it is just "return 0". This is used for all RSA sig verification except for the above cert stuff.
cmp r1, #0x2E
ldreq r0, =0x0805d2c0
cmp r1, #0x30
ldreq r0, =0x0805d2c4
cmp r1, #0x37
ldreq r0, =0x0805d4c0
cmp r0, #0
beq patchfirm_arm9section_L2

ldr r1, =0x2000
ldr r2, =0x4770
sub r0, r0, r4
add r0, r0, r5
strh r1, [r0] @ "mov r0, #0"
strh r2, [r0, #2] @ "bx lr"

patchfirm_arm9section_L2:
/*#if FIRMLAUNCH_FWVER == 0x1F
ldr r0, =0x08081e10
ldr r1, =arm9_rsaengine_txtwrite_stub
sub r0, r0, r4
add r0, r0, r5

ldrh r2, [r1, #0]
strh r2, [r0, #0]
ldrh r2, [r1, #2]
strh r2, [r0, #2]
ldrh r2, [r1, #4]
strh r2, [r0, #4]
ldrh r2, [r1, #6]
strh r2, [r0, #6]
#endif*/

/*ldr r0, =0x08085ee0
sub r0, r0, r4
add r0, r0, r5
bl arm9general_debughook_writepatch*/

#ifdef ENABLENANDREDIR
ldr r0, =FIRMLAUNCH_RUNNINGTYPE
ldr r0, [r0]
cmp r0, #3
beq patchfirm_arm9section_L3

/*ldr r0, =FIRMLAUNCH_FWVER
ldr r0, [r0]
mov r1, #0x1F
mov r2, #0
mov r3, #0
cmp r0, r1
ldreq r2, =0x8078c6e//FW1F
ldreq r3, =0x8078c2e
mov r1, #0x2E
cmp r0, r1
ldreq r2, =0x8078446//FW2E
ldreq r3, =0x8078406
cmp r2, #0
beq patchfirm_arm9section_L3

sub r0, r2, r4
add r0, r0, r5
sub r1, r3, r4
add r1, r1, r5
bl patch_nandredir*/

mov r0, r5
mov r1, r7
mov r2, r4
bl patch_nandredir_autolocate
#endif

patchfirm_arm9section_L3:
/*ldr r3, =0x802da5e
sub r3, r3, r4
add r3, r3, r0
ldr r2, =0x2101 @ "mov r1, #1"
//strh r2, [r3] @ Patch the instruction executed before a mmcbus_selectdevice() call which sets r1, so that it always selects the SD device.

ldr r3, =0x805f1ba
sub r3, r3, r4
add r3, r3, r0
strh r2, [r3] @ Same as above.*/

/*ldr r3, =0x8032a74 @ twlfirm v6704 patches:
sub r3, r3, r4 @ r3 = first word @ firmpatch_begin / etc, subtracted by the .text addr
add r3, r3, r0 @ r3+= .code addr

mov r2, #0
strh r2, [r3] @ NOP a subtract instruction so that the non-dsiretail RSA modulo is always used.

ldr r3, =0x8032bbe
sub r3, r3, r4
add r3, r3, r0
ldr r2, =0x2000 @ "mov r0, #0"
strh r2, [r3] @ Patch the arm9bin hash compare func-call with the above instruction.

ldr r3, =0x8032c6a
sub r3, r3, r4
add r3, r3, r0
strh r2, [r3] @ Patch the arm7bin hash compare func-call with the above instruction.*/

/*ldr r3, =0x805e5c4
sub r3, r3, r4
add r3, r3, r0
mov r2, #0
str r2, [r3] @ Patch the twlfirm thumb bl, which calls code which waits for bit0 in u8 0x04004000 to clear.

ldr r3, =0x805e5d6
sub r3, r3, r4
add r3, r3, r0
mov r2, #0
strh r2, [r3]
strh r2, [r3, #2] @ Patch out the func-call for the panic function executed when the word @ 0xffff0000 is invalid.

ldr r3, =0x805e318
sub r3, r3, r4
add r3, r3, r0
mov r2, #0
str r2, [r3] @ Patch out the func-call for the function which writes value 1 to u8 registers 0x04004000 and 0x04004001.*/

/*ldr r1, =arm9_twlfirm_stub
ldr r3, =0x8032a30
sub r3, r3, r4
add r3, r3, r0
ldr r2, [r1, #0]
str r2, [r3, #0]
ldr r2, [r1, #4]
str r2, [r3, #4]*/

/*ldr r3, =0x803c456
sub r3, r3, r4
add r3, r3, r0
ldr r2, =0x4809 @ "ldr r0, [pc, #36]"
strh r2, [r3] @ Change the instruction before the twl-bootldr function call from "add r0, pc, #36" to the above instruction. (bootldr image utf16 filepath)

ldr r2, =0x80582D4
ldr r3, =0x803c47c
sub r3, r3, r4
add r3, r3, r0
str r2, [r3] @ Patch the .pool data used by the above patched instruction.

ldr r1, =twlbootldr_filepath
ldr r3, =0x80582D4
sub r3, r3, r4
add r3, r3, r0
mov r2, #0x2a

patchfirm_cpypatch:
ldrh r4, [r1], #2
strh r4, [r3], #2
subs r2, r2, #2
bgt patchfirm_cpypatch*/

patchfirm_arm9section_finish:
add sp, sp, #4
pop {r4, r5, r6, r7, pc}
.pool

patchfirm_arm11section_modules: @ r0 = address of FIRM section in memory, in the FIRM binary. r1 = FIRM section size. r2 = FIRM header address.
push {r4, r5, r6, r7, r8, lr}
sub sp, sp, #20
/*#ifdef FIRMLAUNCH_ENABLEFIRM_MODULELOAD
#if FIRMLAUNCH_FWVER == 0x1F//This code itself is not nativefirm-version dependent, however this module will not get loaded without the below kernel patches.

add r5, r0, r1
mov r6, r2

ldr r0, sdarchive_obj
mov r3, #1
str r3, [sp, #0]
add r1, sp, #8
str r1, [sp, #4]
mov r1, #4
ldr r2, =yls8proc_cxi_filepath
mov r3, #0x1c
bl openfile

ldr r0, [sp, #8]
cmp r0, #0
beq patchfirm_arm11section_modules_end
bl getfilesize
adr r1, patchfirm_arm11section_additionalmodulesize
str r0, [r1]
mov r4, r0

mov r7, #3 @ Move each FIRM section(excluding section0 containing the arm11 modules) forward(offset change + data-copy) by the size of the SD .cxi module.
patchfirm_arm11section_modules_firmhdrlp:
mov r3, #0x30
mul r1, r7, r3
add r1, r1, #0x40
add r8, r1, r6

ldr r2, [r8, #8] @ section size
cmp r2, #0
beq patchfirm_arm11section_modules_firmhdrlp_end

ldr r0, [r8, #0] @ section offset
mov r1, r0
add r0, r0, r4
str r0, [r8, #0]
add r1, r1, r6
ldr r0, =0x25000000
bl memcpy

ldr r1, =0x25000000
ldr r0, [r8, #0]
add r0, r0, r6
ldr r2, [r8, #8]
bl memcpy

patchfirm_arm11section_modules_firmhdrlp_end:
sub r7, r7, #1
cmp r7, #0
bgt patchfirm_arm11section_modules_firmhdrlp

ldr r0, [r6, #0x48] @ section0 size
add r0, r0, r4
str r0, [r6, #0x48]

mov r2, r4
ldr r0, [sp, #8]
mov r1, r5
mov r3, #0
bl fileread

patchfirm_arm11section_modules_end:
#endif
#endif*/
add sp, sp, #20
pop {r4, r5, r6, r7, r8, pc}
.pool

patchfirm_arm11section_kernel: @ r0 = address of FIRM section in memory, in the FIRM binary. r1 = FIRM section size. r2 = FIRM header address.
push {r4, r5, r6, lr}
/*#ifdef FIRMLAUNCH_ENABLEFIRM_MODULELOAD
#if FIRMLAUNCH_FWVER == 0x1F

ldr r3, patchfirm_arm11section_additionalmodulesize
cmp r3, #0
beq patchfirm_arm11section_kernel_end

ldr r1, =0x8a08 @ Increase the FIRM module count returned by svcGetSystemInfo.
ldr r2, [r0, r1]
add r2, r2, #1
str r2, [r0, r1]

ldr r1, =0xdb8 @ Increase the module-count value in the cmp instruction in the start_firmprocesses() loop.
ldr r2, [r0, r1]
add r2, r2, #1
str r2, [r0, r1]

ldr r1, =0xde4 @ Increase the ncch module-data end-address field in the start_firmprocesses() .pool.
ldr r2, [r0, r1]
add r2, r2, r3
str r2, [r0, r1]

ldr r1, =0xdfc @ Increase the totalncchmodulesize field in the start_firmprocesses() .pool, used for clearing the ncch data once finished.
ldr r2, [r0, r1]
add r2, r2, r3
str r2, [r0, r1]

patchfirm_arm11section_kernel_end:
#endif
#endif*/
pop {r4, r5, r6, pc}
.pool

patchfirm_arm11section_additionalmodulesize:
.word 0

/*arm9_launchfirm:
ldr r4, =0x8056a05//waitrecv_pxiword
ldr r5, =0x805ac41//sendword_pxi

arm9launchfirm_waitpxibegin:
blx r4
ldr r1, =0x00044836
cmp r0, r1
bne arm9launchfirm_waitpxibegin

ldr r0, =0x00964536
blx r5

arm9launchfirm_wait_titleidpxi:
blx r4
ldr r1, =0x00044837
cmp r0, r1
bne arm9launchfirm_wait_titleidpxi

blx r4//read the titleID from PXI, however this is unused by this code since the FIRM is loaded from a hard-coded SD card path.
blx r4

bl arm9_debugcode

arm9launchfirm_waitpxibootbegin:
blx r4
ldr r1, =0x00044846
cmp r0, r1
bne arm9launchfirm_waitpxibootbegin

mov r0, #0
mov r1, r0
mov r2, r0
mov r3, r0
svc 0x7c

ldr r0, =0x080ff4fc
svc 0x7b

arm9launchfirm_end:
b arm9launchfirm_end
.pool*/

