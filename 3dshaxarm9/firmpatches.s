.arch armv5te
.fpu softvfp
.text
.arm

.global patch_firm
.global patchfirm_arm9section
.global patchfirm_arm11section_kernel
.global firm_arm11kernel_getminorversion_firmimage
.global firm_gethwtype
.type patch_firm STT_FUNC
.type patchfirm_arm9section STT_FUNC
.type patchfirm_arm11section_kernel STT_FUNC
.type firm_arm11kernel_getminorversion_firmimage STT_FUNC
.type firm_gethwtype STT_FUNC

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
mov r2, r4
mov r3, #1
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

patchfirm_arm9section: @ r0 = address of FIRM section in memory, in the FIRM binary. r1 = FIRM section size. r2 = FIRM header address(only used on New3DS when inr3 flag=1), r3 = flag.
push {r4, r5, r6, r7, lr}
sub sp, sp, #4

mov r4, r1

cmp r3, #0
beq patchfirm_arm9section_begin

ldr r3, =RUNNINGFWVER
ldr r3, [r3]
lsr r3, r3, #30
and r3, r3, #1
cmp r3, #0
beq patchfirm_arm9section_begin @ Branch when not running on new3ds.

add r1, r1, r0
ldr r3, [r2, #12]
ldr r2, =0x08006000
sub r3, r3, r2
add r3, r3, r0
ldr r4, =new3ds_hookloader_entrypoint
blx r4

b patchfirm_arm9section_finish

patchfirm_arm9section_begin:
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

#ifdef ENABLE_FIRMPARTINSTALL_STRS_PATCH
mov r0, r4
mov r1, r5
mov r2, r7
bl proc9_autolocate_patch_firminstallstrs
b patchfirm_arm9section_finish
#endif

/*ldr r3, =0x08078a90
sub r3, r3, r4
add r3, r3, r0
ldr r1, =arm9_nandredir_stub4
ldr r2, [r1, #0]
str r2, [r3, #0]
ldr r2, [r1, #4]
str r2, [r3, #4]*/

mov r0, r4
mov r1, r5
bl proc9_autolocate_hookpatchaddr

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

mov r0, r4
mov r1, r5
mov r2, r7
bl proc9_autolocate_certsigcheck_patchaddr
cmp r0, #0
beq patchfirm_arm9section_L1

ldr r1, =0x2000 @ Patch the RSA verification function used for certs(tmd/tik/cert-chain/...), so that it returns 0 instead of -2011 for invalid signature.
sub r0, r0, r4
add r0, r0, r5
strh r1, [r0] @ "mov r0, #0"

patchfirm_arm9section_L1:
mov r0, r4 @ Patch rsa_verifysignature, so that the very beginning of it is just "return 0". This is used for all RSA sig verification except for the above cert stuff.
mov r1, r5
mov r2, r7
bl proc9_autolocate_mainsigcheck_patchaddr

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

#ifdef ENABLENANDREDIR
#ifdef ENABLE_LOADA9_x01FFB800
adr r0, filepath_x01ffb800
ldr r1, =0x01ffb800
ldr r2, =0x4800
bl loadfile_charpath
#endif
#endif

mov r0, r5
mov r1, r7
mov r2, r4
bl patch_nandredir_autolocate
#if NANDREDIR_SECTORNUM_PADCHECK0 || NANDREDIR_SECTORNUM_PADCHECK1
cmp r0, #0
bne patchfirm_arm9section_L3

#ifdef NANDREDIR_SECTORNUM_PADCHECK0
#ifndef NANDREDIR_SECTORNUM_PADCHECK0VAL
#error NANDREDIR_SECTORNUM_PADCHECK0VAL must be defined when NANDREDIR_SECTORNUM_PADCHECK0 is used.
#endif
#endif

#ifdef NANDREDIR_SECTORNUM_PADCHECK1
#ifndef NANDREDIR_SECTORNUM_PADCHECK1VAL
#error NANDREDIR_SECTORNUM_PADCHECK1VAL must be defined when NANDREDIR_SECTORNUM_PADCHECK1 is used.
#endif
#endif

ldr r0, =0x10146000
ldrh r0, [r0]

#ifdef NANDREDIR_SECTORNUM_PADCHECK0
ldr r1, =NANDREDIR_SECTORNUM_PADCHECK0
ldr r2, =NANDREDIR_SECTORNUM_PADCHECK0VAL
mov r3, r0
ands r3, r1
bne patchfirm_arm9section_nandredir_padcheck1

ldr r3, =NANDREDIR_SECTORNUM
str r2, [r3]
b patchfirm_arm9section_L3
#endif

patchfirm_arm9section_nandredir_padcheck1:
#ifdef NANDREDIR_SECTORNUM_PADCHECK1
ldr r1, =NANDREDIR_SECTORNUM_PADCHECK1
ldr r2, =NANDREDIR_SECTORNUM_PADCHECK1VAL
mov r3, r0
ands r3, r1
bne patchfirm_arm9section_L3

ldr r3, =NANDREDIR_SECTORNUM
str r2, [r3]
#endif

#endif
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
sub r3, r3, r4 @ r3 = first word @ <target addr>, subtracted by the .text addr
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

#endif
#endif*/

#ifndef ENABLE_BOOTSAFEFIRM_STARTUP
ldr r3, =FIRMLAUNCH_RUNNINGTYPE
ldr r3, [r3]
cmp r3, #3
bne patchfirm_arm11section_kernel_end
#else
ldr r3, =RUNNINGFWVER @ Skip the below code if SAFE_MODE_FIRM is already running.
ldr r3, [r3]
lsr r3, r3, #8
ldr r4, =0xffff
and r3, r3, r4
cmp r3, #0x3
beq patchfirm_arm11section_kernel_end

ldr r3, =FIRMLAUNCH_FWVER @ Skip the below code if SAFE_MODE_FIRM is being launched.
ldr r3, [r3]
lsr r3, r3, #8
ldr r4, =0xffff
and r3, r3, r4
cmp r3, #0x3
beq patchfirm_arm11section_kernel_end
#endif

bl firm_arm11kernel_locate_configmeminit
cmp r0, #0
beq patchfirm_arm11section_kernel_end

sub r0, r0, #0x18 @ Patch the "mov <reg>, #0" instruction before the above code, to "mov <reg>, #1", so that the UPDATEFLAG always is set. Hence, this then triggers firmlaunch by NS later.
ldr r1, [r0]
orr r1, r1, #1
str r1, [r0]

patchfirm_arm11section_kernel_end:
pop {r4, r5, r6, pc}
.pool

firm_arm11kernel_locate_configmeminit: @ r0 = address of FIRM section in memory, in the FIRM binary. r1 = FIRM section size.
push {r4, lr}

ldr r4, =0x000ff000

firm_arm11kernel_locate_configmeminit_lp: @ Locate the padcheck code in the kernel configmem init func.
ldr r3, [r0]
bic r3, r3, r4
ldr r2, =0xe2400b03 @ sub <reg>, <reg>, #0xc00
cmp r3, r2
bne firm_arm11kernel_locate_configmeminit_lpnext
ldr r3, [r0, #4]
bic r3, r3, r4
ldr r2, =0xe25000be @ subs <reg>, <reg>, #0xbe
cmp r3, r2
bne firm_arm11kernel_locate_configmeminit_lpnext

b firm_arm11kernel_locate_configmeminit_end

firm_arm11kernel_locate_configmeminit_lpnext:
add r0, r0, #4
subs r1, r1, #4
bgt firm_arm11kernel_locate_configmeminit_lp

mov r0, #0

firm_arm11kernel_locate_configmeminit_end:
pop {r4, pc}
.pool

firm_arm11kernel_getminorversion: @ r0 = address of FIRM section in memory, in the FIRM binary. r1 = FIRM section size.
push {r4, r5, lr}

mvn r4, #0

bl firm_arm11kernel_locate_configmeminit
cmp r0, #0
beq firm_arm11kernel_getminorversion_end

ldr r5, =0xf000
ldr r3, =0xe5c00002

firm_arm11kernel_getminorversion_lp0: @ Locate the "strb <reg>, [r0, #2]" instruction.
ldr r2, [r0]
bic r1, r2, r5
cmp r1, r3
addne r0, r0, #4
bne firm_arm11kernel_getminorversion_lp0

sub r0, r0, #4
and r2, r2, r5

ldr r5, =0xe3a00000
orr r5, r5, r2

firm_arm11kernel_getminorversion_lp1: @ Locate the "mov <reg>, #<val>" instruction, where <reg> matches the one located above, and <val> is the minorversion.
ldr r1, [r0]
lsr r3, r1, #12
lsl r3, r3, #12
cmp r3, r5
subne r0, r0, #4
bne firm_arm11kernel_getminorversion_lp1

and r4, r1, #0xff

firm_arm11kernel_getminorversion_end:
mov r0, r4
pop {r4, r5, pc}
.pool

firm_arm11kernel_getminorversion_firmimage: @ r0 = addr of entire FIRM
push {r4, lr}
mov r1, #0
mov r3, #0x40
add r3, r3, r0 @ start of section headers

ldr r4, =0x1FF80000

firm_arm11kernel_getminorversion_firmimage_lp: @ Locate the section with address=0x1FF80000.
ldr r2, [r3, #4]
cmp r2, r4
beq firm_arm11kernel_getminorversion_firmimage_lp_end
add r3, r3, #0x30
add r1, r1, #1
cmp r1, #4
blt firm_arm11kernel_getminorversion_firmimage_lp

mvneq r0, #0
popeq {r4, pc}

firm_arm11kernel_getminorversion_firmimage_lp_end:
ldr r2, [r3, #8] @ section size
cmp r2, #0
mvneq r0, #0
popeq {r4, pc}

ldr r2, [r3, #0xc] @ core
cmp r2, #0
mvneq r0, #0
popeq {r4, pc} @ type must be arm11.

ldr r1, [r3, #0]
ldr r2, [r3, #8] 
add r0, r0, r1 @ section addr in firm image
mov r1, r2 @ size
pop {r4, lr}
b firm_arm11kernel_getminorversion
.pool

#ifndef DISABLE_MATCHINGFIRM_HWCHECK
firm_gethwtype: @ r0 = addr of entire FIRM. Returns 0 for Old3DS FIRM, 1 for New3DS. Returns ~0 for error.
mov r1, #0
add r2, r0, #0x40

firm_gethwtype_hdrlp:
ldr r3, [r2, #0xc]
cmp r3, #0
bne firm_gethwtype_hdrlpnext @ Look for the arm9 section.

ldr r3, [r2, #0]
add r0, r0, r3 @ r0 = addr of arm9 section start

ldr r0, [r0, #0x40] @ Compare the new3ds control block data, on old3ds actual code is located here instead.
ldr r1, =0x12a82efe
cmp r0, r1
moveq r0, #1
movne r0, #0
b firm_gethwtype_end

firm_gethwtype_hdrlpnext:
add r2, r2, #0x30
add r1, r1, #1
cmp r1, #4
blt firm_gethwtype_hdrlp

mvn r0, #0

firm_gethwtype_end:
bx lr
.pool
#endif

/*@ Locate the two "firmX:" strings used for FIRM-partition installation.
proc9_autolocate_patch_firminstallstrs: @ r0 = Process9 .text addr, r1 = addr of Process9 .code, r2 = Process9 .text size.
push {r4, r5, r6, lr}
mov r4, r1
mov r5, r2
add r4, r4, r5
sub r5, r5, #0x1c
sub r4, r4, #0x1c

proc9_autolocate_patch_firminstallstrs_lp:
mov r0, r4
ldr r1, =proc9_autolocate_patch_firminstallstrs_patterndata
mov r2, #0x1c
bl memcmp
cmp r0, #0
bne proc9_autolocate_patch_firminstallstrs_lpnext

@ Overwrite "firm0:" with "firm1:".
mov r0, #0x31
strb r0, [r4, #8]

b proc9_autolocate_patch_firminstallstrs_end

proc9_autolocate_patch_firminstallstrs_lpnext:
sub r4, r4, #1
sub r5, r5, #1
cmp r5, #0
bcs proc9_autolocate_patch_firminstallstrs_lp

proc9_autolocate_patch_firminstallstrs_end:
pop {r4, r5, r6, pc}

proc9_autolocate_patch_firminstallstrs_patterndata:
.byte 66 00 69 00 72 00 6D 00 30 00 3A 00 00 00 66 00 69 00 72 00 6D 00 31 00 3A 00 00 00*/

//patchfirm_arm11section_additionalmodulesize:
//.word 0

filepath_x01ffb800:
.string "/x01ffb800.bin"
.align 2

