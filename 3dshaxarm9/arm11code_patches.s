.arm

.arch armv5te
.fpu softvfp

.text

.global writearm11_firmlaunch_usrpatch
.type writearm11_firmlaunch_usrpatch STT_FUNC

writearm11_firmlaunch_usrpatch:
push {r4, r5, r6, r7, lr}
ldr r4, =0x004360c0
ldr r5, =0x00100000

ldr r0, =0x41727443 @ "CtrApp"
ldr r1, =0x7070
mov r2, #0
mov r3, #1
bl get_kprocessptr @ Get the MMU table ptr for the above process
cmp r0, #0
beq writearm11_firmlaunch_usrpatch_end

mov r6, r0
mov r1, r4
bl mmutable_convert_vaddr2physaddr
mov r7, r0

mov r0, r4
mov r1, r5
mov r2, #0
bl generate_branch
str r0, [r7]

mov r0, r6
mov r1, r5
bl mmutable_convert_vaddr2physaddr
mov r7, r0

ldr r0, =arm11code_start
ldr r1, =arm11code_end

writearm11_firmlaunch_usrpatch_cpylp:
ldr r2, [r0], #4
str r2, [r7], #4
cmp r0, r1
blt writearm11_firmlaunch_usrpatch_cpylp

writearm11_firmlaunch_usrpatch_end:
pop {r4, r5, r6, r7, pc}
.pool

arm11code_start:
sub sp, sp, #4
mov r0, sp
adr r1, servname
mov r2, #4
mov r3, #0
ldr r4, =0x0030dde8 @ srv_GetServiceHandle
blx r4
cmp r0, #0
bne arm11code_endlp

mrc 15, 0, r0, cr13, cr0, 3
add r0, r0, #0x80

ldr r1, =0x000500C0
//ldr r2, =0x42383841
//ldr r3, =0x00048005
ldr r2, =0x00008002
ldr r3, =0x00040130
str r1, [r0, #0]
str r2, [r0, #4]
str r3, [r0, #8]
mov r1, #2
str r1, [r0, #12]

ldr r0, [sp, #0]
svc 0x32 @ Trigger a FIRM launch.

arm11code_endlp:
b arm11code_endlp
.pool

servname:
.ascii "ns:s"

arm11code_end:
.word 0

