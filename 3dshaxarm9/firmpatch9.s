.arm

.arch armv5te
.fpu softvfp

.global firmpatch_begin
.global firmpatch_end

firmpatch_begin:
.word 0x8028258//0x30//0x1481C//0x8028000//0x80869d4 @ Load address where the rest of this code is loaded into the patched FIRM Process9.

.thumb

firmpatch_start:
mov r1, #0
mvn r1, r1

firmpatch_clrdata_begin:
ldr r0, =0x20000000
ldr r2, =0x00800000

firmpatch_clrdata:
str r1, [r0]
add r0, r0, #4
sub r2, r2, #4
cmp r2, #0
bgt firmpatch_clrdata
sub r1, r1, #0x10
b firmpatch_clrdata_begin

firmpatch_finish:
b firmpatch_finish
.pool

firmpatch_end:
.word 0

