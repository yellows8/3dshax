//Originally located right after "patchfirm_arm9section_L3:" in firmpatches.s.

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
