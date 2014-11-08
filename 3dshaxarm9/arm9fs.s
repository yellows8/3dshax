.arch armv5te
.fpu softvfp
.text
.thumb

.global fs_initialize
.global dumpmem
.global loadfile
.global openfile
.global closefile
.global fileread
.global filewrite
.global getfilesize
.global setfilesize
.global initialize_nandarchiveobj
.global archive_readsectors
.global pxifs_openarchive

.type fs_initialize STT_FUNC
.type dumpmem STT_FUNC
.type loadfile STT_FUNC
.type openfile STT_FUNC
.type closefile STT_FUNC
.type fileread STT_FUNC
.type filewrite STT_FUNC
.type getfilesize STT_FUNC
.type setfilesize STT_FUNC
.type initialize_nandarchiveobj STT_FUNC
.type archive_readsectors STT_FUNC
.type pxifs_openarchive STT_FUNC

fs_initialize:
push {r4, r5, r6, r7, lr}
sub sp, sp, #12

mov r7, r0

bl initializeptr_pxifs_state
mov r3, r0
cmp r0, #0
bne fs_initialize_end

bl initializeptr_fsvtables
mov r3, r0
cmp r0, #0
bne fs_initialize_end

bl initializeptr_pxifsopenarchive
mov r3, r0
cmp r0, #0
bne fs_initialize_end

bl initializeptr_getarchiveclass_something
mov r3, r0
cmp r0, #0
bne fs_initialize_end

mov r3, #0
cmp r7, #0
bne fs_initialize_end

mov r0, #1
str r0, [sp, #0]
mov r0, sp
str r0, [sp, #4]
str r0, [sp, #8]

ldr r0, =sdarchive_obj
mov r1, #9
mov r2, sp
bl pxifs_openarchive
mov r3, r0

fs_initialize_end:
mov r0, r3
add sp, sp, #12
pop {r4, r5, r6, r7, pc}
.pool

getarchiveclass_something: @ inr0/inr1 = u64 archivehandle
push {r0, r1, r4, lr}
sub sp, sp, #24

mov r3, #0
str r3, [sp, #8]
str r3, [sp, #12]
str r3, [sp, #16]
str r3, [sp, #0]
str r3, [sp, #4]

ldr r4, =getarchiveclass_something_adr
ldr r4, [r4]

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

initialize_nandarchiveobj:
push {r4, r5, lr}
sub sp, sp, #12

mov r0, #1
str r0, [sp, #0]
mov r0, sp
str r0, [sp, #4]
str r0, [sp, #8]

mov r0, #0
ldr r3, =nandarchive_obj
ldr r4, [r3]
cmp r4, #0
bne initialize_nandarchiveobj_end
mov r0, r3
ldr r1, =0x567890ab
mov r2, sp
bl pxifs_openarchive

initialize_nandarchiveobj_end:
add sp, sp, #12
pop {r4, r5, pc}
.pool

fs_getvtableptr_rw: @ r0=rw. 0 = fileread(data write to FCRAM), 1 = filewrite(data read from FCRAM).
cmp r0, #0
bne fs_getvtableptr_rw_write

ldr r0, =fs_vtableptr_fileread
ldr r0, [r0]
b fs_getvtableptr_rw_end

fs_getvtableptr_rw_write:
ldr r0, =fs_vtableptr_filewrite
ldr r0, [r0]

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
mov r4, r0
cmp r0, #0
bne dumpmem_end

ldr r0, [sp, #8]
ldr r1, [sp, #16]
bl setfilesize

ldr r0, [sp, #8]
ldr r1, [sp, #12]
ldr r2, [sp, #16]
mov r3, #0
bl filewrite
mov r4, r0

ldr r0, [sp, #8]
bl closefile

dumpmem_end:
mov r0, r4
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

ldr r0, [sp, #8]
bl closefile

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

closefile:
bx lr
/*ldr r1, [r0] @ This doesn't seem to actually close the file?
ldr r1, [r1, #4]
bx r1*/

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
blx r2
ldr r0, [sp, #0]
add sp, sp, #8
pop {pc}

setfilesize:
push {r4, lr}
mov r3, #0
mov r2, r1
ldr r4, [r0]
ldr r4, [r4, #20]
blx r4
pop {r4, pc}

archive_readsectors: @ r0=archiveclass*, r1=buffer, r2=sectorcount, r3=mediaoffset/sector#
push {r0, r1, r2, r3, r4, lr}
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
pop {r1, r2, r3, r4, pc}
.pool

pxifs_openarchive: @ inr0=ptr where the archiveobj* will be written, inr1=archiveid, and inr2=lowpath*
push {r0, r1, r2, r3, r4, r5, lr}
sub sp, sp, #24

ldr r4, =pxifsopenarchive_adr
ldr r4, [r4]

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

initializeptr_pxifsopenarchive:
push {r4, lr}

mov r4, #0
mvn r4, r4

mov r0, #1 @ Locate the pxifs cmdhandler jump-table.
str r0, [sp]
ldr r0, =0x08028000
ldr r1, =(0x000ff000-0x28000)
adr r2, pxifs_cmdhandler_poolcmpdata
mov r3, #7
bl locate_cmdhandler_code
cmp r0, #0
beq initializeptr_pxifsopenarchive_end

mov r1, #0x12
lsl r1, r1, #2
ldr r0, [r0, r1] @ r0 = jump-addr in the pxifs cmdhandler for cmd 0x12, OpenArchive.

ldr r1, =0x080ff000

initializeptr_pxifsopenarchive_lp: @ Locate the pxifs_openarchive blx instruction.
ldr r2, [r0]
lsr r2, r2, #24
cmp r2, #0xfa
bne initializeptr_pxifsopenarchive_lpnext
b initializeptr_pxifsopenarchive_lpend

initializeptr_pxifsopenarchive_lpnext:
add r0, r0, #4
cmp r0, r1
blt initializeptr_pxifsopenarchive_lp

mov r4, #1
mvn r4, r4
b initializeptr_pxifsopenarchive_end

initializeptr_pxifsopenarchive_lpend:
mov r1, #0
blx parse_branch

ldr r1, =pxifsopenarchive_adr
mov r2, #1
orr r0, r0, r2
str r0, [r1]

mov r4, #0

initializeptr_pxifsopenarchive_end:
mov r0, r4
pop {r4, pc}
.pool

initializeptr_getarchiveclass_something:
push {r4, r5, lr}

mov r4, #0
mvn r4, r4

ldr r0, =0x08028000
ldr r1, =0x080ff000
ldr r2, =0x567890ae
ldr r5, =0x0074002f

initializeptr_getarchiveclass_something_lp0:
ldr r3, [r0]
cmp r3, r2
bne initializeptr_getarchiveclass_something_lp0next
ldr r3, [r0, #4]
cmp r3, r5
bne initializeptr_getarchiveclass_something_lp0next
b initializeptr_getarchiveclass_something_lp1_begin

initializeptr_getarchiveclass_something_lp0next:
add r0, r0, #4
cmp r0, r1
blt initializeptr_getarchiveclass_something_lp0
b initializeptr_getarchiveclass_something_end

initializeptr_getarchiveclass_something_lp1_begin:
ldr r2, =0x0280

initializeptr_getarchiveclass_something_lp1:
ldrh r3, [r0]
cmp r3, r2
beq initializeptr_getarchiveclass_something_lp1end

initializeptr_getarchiveclass_something_lp1next:
sub r0, r0, #2
b initializeptr_getarchiveclass_something_lp1

initializeptr_getarchiveclass_something_lp1end:
sub r0, r0, #6
mov r1, #0
bl parse_branch_thumb

mov r2, #1
orr r0, r0, r2
ldr r1, =getarchiveclass_something_adr
str r0, [r1]

mov r4, #0

initializeptr_getarchiveclass_something_end:
mov r0, r4
pop {r4, r5, pc}
.pool

initializeptr_pxifs_state:
push {r4, r5, lr}
mov r4, #0
mvn r4, r4

bl proc9_locate_main_endaddr
cmp r0, #0
beq initializeptr_pxifs_state_end

sub r0, r0, #0x18
mov r1, #0
bl parse_branch

ldr r1, =0xe2800008
ldr r4, =0x000ff000

initializeptr_pxifs_state_lp0: @ Locate "add <reg>, <reg>, #8"
ldr r3, [r0]
mov r2, r3
bic r2, r2, r4
cmp r2, r1
beq initializeptr_pxifs_state_lp0end

initializeptr_pxifs_state_lp0next:
add r0, r0, #4
b initializeptr_pxifs_state_lp0

initializeptr_pxifs_state_lp0end:
mov r2, #16
lsr r3, r3, r2
mov r2, #0xf
and r3, r3, r2

ldr r1, =0xe59f0000
lsl r3, r3, #12
orr r1, r1, r3
ldr r4, =0xfff

initializeptr_pxifs_state_lp1: @ Locate the ldr instruction for pxifs state.
ldr r3, [r0]
mov r2, r3
bic r2, r2, r4
cmp r2, r1
beq initializeptr_pxifs_state_lp1end

initializeptr_pxifs_state_lp1next:
sub r0, r0, #4
b initializeptr_pxifs_state_lp1

initializeptr_pxifs_state_lp1end:
mov r2, #0xff
and r3, r3, r2
add r0, r0, #8
add r0, r0, r3
ldr r0, [r0]

add r0, r0, #8
ldr r1, =pxifs_state
str r0, [r1]

mov r4, #0

initializeptr_pxifs_state_end:
mov r0, r4
pop {r4, r5, pc}
.pool

initializeptr_fsvtables:
push {r4, r5, r6, r7, lr}
mov r4, #0
mvn r4, r4

ldr r0, =0x08028000
ldr r1, =0x080ff000
ldr r2, =0x4453434e

initializeptr_fsvtables_lp0: @ Locate "NCSD" word in the target function's .pool.
ldr r3, [r0]
cmp r3, r2
bne initializeptr_fsvtables_lp0next
b initializeptr_fsvtables_lp0end

initializeptr_fsvtables_lp0next:
add r0, r0, #4
cmp r0, r1
blt initializeptr_fsvtables_lp0
b initializeptr_fsvtables_end

initializeptr_fsvtables_lp0end:
sub r0, r0, #4
ldr r5, [r0]
ldr r2, =fs_vtableptr_fileread
str r5, [r2]
add r0, r0, #4

ldr r6, =0xb005 @ "add sp, #20"
ldr r7, =0xbdf0 @ "pop {r4, r5, r6, r7, pc}"

initializeptr_fsvtables_lp1: @ Locate the fileread vtable ptr in a function's .pool, where the above instructions are immediately before it.
ldr r3, [r0]
cmp r3, r5
bne initializeptr_fsvtables_lp1next
sub r2, r0, #4
ldrh r3, [r2, #0]
cmp r3, r6
bne initializeptr_fsvtables_lp1next
ldrh r3, [r2, #2]
cmp r3, r7
bne initializeptr_fsvtables_lp1next
b initializeptr_fsvtables_lp1end

initializeptr_fsvtables_lp1next:
add r0, r0, #4
cmp r0, r1
blt initializeptr_fsvtables_lp1
b initializeptr_fsvtables_end

initializeptr_fsvtables_lp1end:
add r0, r0, #4
ldr r0, [r0]
ldr r1, =fs_vtableptr_filewrite
str r0, [r1]

mov r4, #0

initializeptr_fsvtables_end:
mov r0, r4
pop {r4, r5, r6, r7, pc}
.pool

pxifs_cmdhandler_poolcmpdata:
.word 0xd9001830, 0x000101c2, 0xe0e046be, 0x000100c1, 0x00020142, 0x00020041, 0x00030244

pxifsopenarchive_adr:
.word 0

getarchiveclass_something_adr:
.word 0

fs_vtableptr_fileread:
.word 0

fs_vtableptr_filewrite:
.word 0

