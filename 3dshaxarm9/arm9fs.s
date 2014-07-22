.arch armv5te
.fpu softvfp
.text
.arm

.global fs_initialize
.global dumpmem
.global loadfile
.global openfile
.global fileread
.global filewrite
.global getfilesize
.global initialize_nandarchiveobj
.global archive_readsectors
.global pxifs_openarchive

.type fs_initialize STT_FUNC
.type dumpmem STT_FUNC
.type loadfile STT_FUNC
.type openfile STT_FUNC
.type fileread STT_FUNC
.type filewrite STT_FUNC
.type getfilesize STT_FUNC
.type initialize_nandarchiveobj STT_FUNC
.type archive_readsectors STT_FUNC
.type pxifs_openarchive STT_FUNC

getarchiveclass_something: @ inr0/inr1 = u64 archivehandle
push {r0, r1, r4, lr}
sub sp, sp, #24

mov r3, #0
str r3, [sp, #8]
str r3, [sp, #12]
str r3, [sp, #16]
str r3, [sp, #0]
str r3, [sp, #4]

ldr r2, =RUNNINGFWVER
ldr r2, [r2]
cmp r2, #0x1F
ldreq r4, =0x8063f91
cmp r2, #0x2E
ldreq r4, =0x8062715
cmp r2, #0x30
ldreq r4, =0x08062719
cmp r2, #0x37
ldreq r4, =0x08062915
ldr r0, =0x29
add r1, r0, #1
cmp r2, r0
cmpne r2, r1
ldreq r4, =0x08062861

add r0, sp, #8
ldr r1, =pxifs_state
ldr r1, [r1]
ldr r2, [sp, #24]
ldr r3, [sp, #28]
blx r4

ldr r0, [sp, #20]

add sp, sp, #32
pop {r4, pc}
.pool

fs_initialize:
push {r4, r5, r6, r7, lr}
sub sp, sp, #8

mov r7, r0

mov r0, #0
str r0, [sp, #0]
str r0, [sp, #4]

ldr r2, =RUNNINGFWVER
ldr r2, [r2]

cmp r2, #0x1F
ldreq r5, =0x080d93f8/*0x0809797c*/ @ pxifs state
ldreq r4, =0x8061451 @ mntsd archive funcptr
cmp r2, #0x2E
ldreq r5, =0x080d8b60/*0x080945c4*/
ldreq r4, =0x805eb79
cmp r2, #0x30
ldreq r5, =0x080d8b60
ldreq r4, =0x0805eb7d
cmp r2, #0x37
ldreq r5, =0x080d8c20
ldreq r4, =0x0805ed7d

ldr r0, =0x29
add r1, r0, #1
cmp r2, r0
cmpne r2, r1
ldreq r5, =0x080d8be0
ldreq r4, =0x0805ecbd

/*mvn r3, #0
ldr r5, [r5]
cmp r5, #0
beq fs_initialize_end*/

add r5, r5, #8 @ r5 = state
ldr r0, =pxifs_state
str r5, [r0]

mov r3, #0
cmp r7, #0
bne fs_initialize_end

ldr r1, =0x2EA0
add r0, r5, r1
add r1, sp, #0
blx r4

mvn r3, #1
cmp r0, #0
bne fs_initialize_end

ldr r0, [sp, #0]
ldr r1, [sp, #4]
bl getarchiveclass_something

mvn r3, #2
cmp r0, #0
beq fs_initialize_end

ldr r1, =sdarchive_obj
str r0, [r1]

mov r3, #0
fs_initialize_end:
mov r0, r3
add sp, sp, #8
pop {r4, r5, r6, r7, pc}
.pool

initialize_nandarchiveobj:
push {r4, r5, lr}
sub sp, sp, #12

ldr r2, =RUNNINGFWVER
ldr r2, [r2]
cmp r2, #0x1F
ldreq r4, =0x080590b5
moveq r3, #0
cmp r2, #0x2E
ldreq r4, =0x08055d29
moveq r3, #1
cmp r2, #0x30
ldreq r4, =0x08055d2d
moveq r3, #1
cmp r2, #0x37
ldreq r4, =0x08055f05
moveq r3, #1
ldr r0, =0x29
add r1, r0, #1
cmp r2, r0
cmpne r2, r1
ldreq r4, =0x08055de1
moveq r3, #1

ldr r0, =pxifs_state
ldr r0, [r0]
ldr r1, =0x2de8
add r0, r0, r1
mov r1, sp
ldr r2, =0x567890ab
mov r5, #0
str r5, [sp, #0]
str r5, [sp, #4]

blx r4 @ archive_mountnand

cmp r0, #0
bne initialize_nandarchiveobj_end

ldr r0, [sp, #0]
ldr r1, [sp, #4]
bl getarchiveclass_something
ldr r1, =nandarchive_obj
str r0, [r1]
mov r0, #0

initialize_nandarchiveobj_end:
add sp, sp, #12
pop {r4, r5, pc}
.pool

fs_getvtableptr_rw: @ r0=rw. 0 = fileread(data write to FCRAM), 1 = filewrite(data read from FCRAM).
cmp r0, #0
bne fs_getvtableptr_rw_write

ldr r1, =RUNNINGFWVER
ldr r1, [r1]
cmp r1, #0x1F
ldreq r0, =0x080944c8
beq fs_getvtableptr_rw_end
cmp r1, #0x2E
ldreq r0, =0x0809106c
beq fs_getvtableptr_rw_end
cmp r1, #0x30
ldreq r0, =0x08091070
beq fs_getvtableptr_rw_end
cmp r1, #0x37
ldreq r0, =0x08091368
beq fs_getvtableptr_rw_end
ldr r2, =0x29
add r3, r2, #1
cmp r1, r2
cmpne r1, r3
ldreq r0, =0x08090efc
b fs_getvtableptr_rw_end

fs_getvtableptr_rw_write:
mov r0, #0
ldr r1, =RUNNINGFWVER
ldr r1, [r1]
cmp r1, #0x1F
ldreq r0, =0x08094490
beq fs_getvtableptr_rw_end
cmp r1, #0x2E
ldreq r0, =0x08091034
beq fs_getvtableptr_rw_end
cmp r1, #0x30
ldreq r0, =0x08091038
beq fs_getvtableptr_rw_end
cmp r1, #0x37
ldreq r0, =0x08091330
beq fs_getvtableptr_rw_end
ldr r2, =0x29
add r3, r2, #1
cmp r1, r2
cmpne r1, r3
ldreq r0, =0x08090ec4

fs_getvtableptr_rw_end:
bx lr
.pool

dumpmem: @ r0=addr, r1=size
push {r0, r1, r4, lr}
sub sp, sp, #12
ldr r0, =sdarchive_obj
ldr r0, [r0]
mov r3, #7
str r3, [sp, #0]
add r1, sp, #8
str r1, [sp, #4]
mov r1, #4
ldr r2, =dump_filepath
mov r3, #0x22
bl openfile

mov r4, #0
dumpmem_writelp:
ldr r0, [sp, #8]
ldr r1, [sp, #12]
ldr r2, [sp, #16]
mov r3, #0
bl filewrite
add r4, r4, #1
cmp r4, #16
cmplt r0, #0
blt dumpmem_writelp

add sp, sp, #12
add sp, sp, #8
pop {r4, pc}

loadfile: @ r0=addr, r1=size, r2=wchar lowpath*, r3=lowpath size
push {r4, r5, lr}
sub sp, sp, #12
mov r4, r0
mov r5, r1
ldr r0, =sdarchive_obj
ldr r0, [r0]
mov r1, #1
str r1, [sp, #0]
mov r1, #0
str r1, [sp, #8]
add r1, sp, #8
str r1, [sp, #4]
mov r1, #4
bl openfile

cmp r0, #0
bne loadfile_end

ldr r0, [sp, #8]
mov r1, r4
mov r2, r5
mov r3, #0
bl fileread

loadfile_end:
add sp, sp, #12
pop {r4, r5, pc}

openfile:
push {r3, r4, r5, lr} @ r0=archiveclass*, r1=lowpath type, r2=wchar* filepath, r3=lowpath buffer size, sp0=openflags, sp4=fileclass**.
sub sp, sp, #20
mov r5, r0
add r0, sp, #8
str r1, [r0]
str r2, [r0, #4]
str r3, [r0, #8]

mov r1, #0
ldr r3, [sp, #36]
str r3, [sp, #0]
str r1, [sp, #4]
ldr r1, [sp, #40]
mov r2, #0
add r3, sp, #8
mov r0, r5
ldr r4, [r0]
ldr r4, [r4, #8]
blx r4
add sp, sp, #20
pop {r3, r4, r5, pc}

fileread: @ r0=fileclass*, r1=buffer, r2=size, r3=filepos
push {r0, r1, r2, r3, r4, r5, lr}
sub sp, sp, #24

mov r0, #0
bl fs_getvtableptr_rw

ldr r2, [sp, #32]
str r2, [sp, #8]

add r3, sp, #16
str r0, [r3]

ldr r0, [sp, #24]
ldr r1, [sp, #28]
str r1, [r3, #4]
str r3, [sp, #0]
mov r1, #0
str r1, [sp, #4]
add r1, sp, #12
ldr r2, [sp, #36]
mov r3, #0

ldr r4, [r0]
ldr r4, [r4, #0x38]
blx r4 //readfile

fileread_end:
add sp, sp, #28
pop {r1, r2, r3, r4, r5, pc}
.pool

filewrite: @ r0=fileclass*, r1=buffer, r2=size, r3=filepos
push {r0, r1, r2, r3, r4, r5, lr}
sub sp, sp, #24

mov r0, #1
bl fs_getvtableptr_rw

ldr r2, [sp, #32]
add r3, sp, #16
str r2, [sp, #8]

str r0, [r3]
ldr r0, [sp, #24]
ldr r1, [sp, #28]
str r1, [r3, #4]
str r3, [sp, #0]
mov r1, #0
str r1, [sp, #4]
ldr r1, =0x10001
str r1, [sp, #12]
add r1, sp, #12
ldr r2, [sp, #36]
mov r3, #0

ldr r4, [r0]
ldr r4, [r4, #0x3c]
blx r4 //writefile

add sp, sp, #32
pop {r2, r3, r4, r5, pc}
.pool

getfilesize:
push {lr}
sub sp, sp, #8
add r1, sp, #0
ldr r2, [r0]
ldr r2, [r2, #16]
blx r2 //getfilesize
ldr r0, [sp, #0]
add sp, sp, #8
pop {pc}

archive_readsectors: @ r0=archiveclass*, r1=buffer, r2=sectorcount, r3=mediaoffset/sector#
push {r0, r1, r2, r3, r4, r5, lr}
sub sp, sp, #24

mov r0, #0
bl fs_getvtableptr_rw

ldr r2, [sp, #32]
str r2, [sp, #0]
add r3, sp, #16

str r0, [r3]
ldr r0, [sp, #24]
ldr r1, [sp, #28]
str r1, [r3, #4]
mov r1, r3
mov r2, #0
ldr r3, [sp, #36]

ldr r4, [r0]
ldr r4, [r4, #0x8]
blx r4

add sp, sp, #28
pop {r1, r2, r3, r4, r5, pc}
.pool

pxifs_openarchive: @ inr0=ptr where the archiveobj* will be written, inr1=archiveid, and inr2=lowpath*
push {r0, r1, r2, r3, r4, r5, lr}
sub sp, sp, #24

ldr r2, =RUNNINGFWVER
ldr r2, [r2]
cmp r2, #0x1F
ldreq r4, =0x0805d8fd @ pxifs_openarchive
cmp r2, #0x2E
ldreq r4, =0x0805b835
cmp r2, #0x30
ldreq r4, =0x0805b839
cmp r2, #0x37
ldreq r4, =0x0805ba1d
ldr r0, =0x29
add r1, r0, #1
cmp r2, r0
cmpne r2, r1
ldreq r4, =0x0805ba31

mov r0, #0
str r0, [sp, #0]
str r0, [sp, #4]
ldr r0, =pxifs_state
ldr r0, [r0]
add r1, sp, #8
ldr r2, [sp, #28]
ldr r3, [sp, #32]

blx r4
mov r5, r0
cmp r5, #0
bne pxifs_openarchive_end

ldr r0, [sp, #8]
ldr r1, [sp, #12]
bl getarchiveclass_something
ldr r1, [sp, #24]
str r0, [r1]

pxifs_openarchive_end:
mov r0, r5
add sp, sp, #24
add sp, sp, #16
pop {r4, r5, pc}
.pool

