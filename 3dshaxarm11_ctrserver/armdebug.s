.text
.cpu mpcore
.arm

.global getdebug_didr
.type getdebug_didr, %function
getdebug_didr:
mrc p14, 0, r0, c0, c0, 0
bx lr

.global getdebug_dscr
.type getdebug_dscr, %function
getdebug_dscr:
mrc p14, 0, r0, c0, c1, 0
bx lr

.global setdebug_dscr
.type setdebug_dscr, %function
setdebug_dscr:
mcr p14, 0, r0, c0, c1, 0
bx lr

.global getdebug_vcr
.type getdebug_vcr, %function
getdebug_vcr:
mrc p14, 0, r0, c0, c7, 0
bx lr

.global setdebug_vcr
.type setdebug_vcr, %function
setdebug_vcr:
mcr p14, 0, r0, c0, c7, 0
b kernelmode_cachestuff

.global getdebug_bvr
.type getdebug_bvr, %function
getdebug_bvr:
cmp r0, #0
beq getdebug_bvr_l0
cmp r0, #1
beq getdebug_bvr_l1
cmp r0, #2
beq getdebug_bvr_l2
cmp r0, #3
beq getdebug_bvr_l3
cmp r0, #4
beq getdebug_bvr_l4
cmp r0, #5
beq getdebug_bvr_l5
mov r0, #0 @ Invalid
bx lr

getdebug_bvr_l0:
mrc p14, 0, r0, c0, c0, 4
bx lr

getdebug_bvr_l1:
mrc p14, 0, r0, c0, c1, 4
bx lr

getdebug_bvr_l2:
mrc p14, 0, r0, c0, c2, 4
bx lr

getdebug_bvr_l3:
mrc p14, 0, r0, c0, c3, 4
bx lr

getdebug_bvr_l4:
mrc p14, 0, r0, c0, c4, 4
bx lr

getdebug_bvr_l5:
mrc p14, 0, r0, c0, c5, 4
bx lr

.global setdebug_bvr
.type setdebug_bvr, %function
setdebug_bvr:
cmp r0, #0
beq setdebug_bvr_l0
cmp r0, #1
beq setdebug_bvr_l1
cmp r0, #2
beq setdebug_bvr_l2
cmp r0, #3
beq setdebug_bvr_l3
cmp r0, #4
beq setdebug_bvr_l4
cmp r0, #5
beq setdebug_bvr_l5
mov r0, #1 @ Invalid
bx lr

setdebug_bvr_l0:
mcr p14, 0, r1, c0, c0, 4
mov r0, #0
b kernelmode_cachestuff

setdebug_bvr_l1:
mcr p14, 0, r1, c0, c1, 4
mov r0, #0
b kernelmode_cachestuff

setdebug_bvr_l2:
mcr p14, 0, r1, c0, c2, 4
mov r0, #0
b kernelmode_cachestuff

setdebug_bvr_l3:
mcr p14, 0, r1, c0, c3, 4
mov r0, #0
b kernelmode_cachestuff

setdebug_bvr_l4:
mcr p14, 0, r1, c0, c4, 4
mov r0, #0
b kernelmode_cachestuff

setdebug_bvr_l5:
mcr p14, 0, r1, c0, c5, 4
mov r0, #0
b kernelmode_cachestuff

.global getdebug_bcr
.type getdebug_bcr, %function
getdebug_bcr:
cmp r0, #0
beq getdebug_bcr_l0
cmp r0, #1
beq getdebug_bcr_l1
cmp r0, #2
beq getdebug_bcr_l2
cmp r0, #3
beq getdebug_bcr_l3
cmp r0, #4
beq getdebug_bcr_l4
cmp r0, #5
beq getdebug_bcr_l5
mov r0, #0 @ Invalid
bx lr

getdebug_bcr_l0:
mrc p14, 0, r0, c0, c0, 5
bx lr

getdebug_bcr_l1:
mrc p14, 0, r0, c0, c1, 5
bx lr

getdebug_bcr_l2:
mrc p14, 0, r0, c0, c2, 5
bx lr

getdebug_bcr_l3:
mrc p14, 0, r0, c0, c3, 5
bx lr

getdebug_bcr_l4:
mrc p14, 0, r0, c0, c4, 5
bx lr

getdebug_bcr_l5:
mrc p14, 0, r0, c0, c5, 5
bx lr

.global setdebug_bcr
.type setdebug_bcr, %function
setdebug_bcr:
cmp r0, #0
beq setdebug_bcr_l0
cmp r0, #1
beq setdebug_bcr_l1
cmp r0, #2
beq setdebug_bcr_l2
cmp r0, #3
beq setdebug_bcr_l3
cmp r0, #4
beq setdebug_bcr_l4
cmp r0, #5
beq setdebug_bcr_l5
mov r0, #1 @ Invalid
bx lr

setdebug_bcr_l0:
mcr p14, 0, r1, c0, c0, 5
mov r0, #0
b kernelmode_cachestuff

setdebug_bcr_l1:
mcr p14, 0, r1, c0, c1, 5
mov r0, #0
b kernelmode_cachestuff

setdebug_bcr_l2:
mcr p14, 0, r1, c0, c2, 5
mov r0, #0
b kernelmode_cachestuff

setdebug_bcr_l3:
mcr p14, 0, r1, c0, c3, 5
mov r0, #0
b kernelmode_cachestuff

setdebug_bcr_l4:
mcr p14, 0, r1, c0, c4, 5
mov r0, #0
bx lr

setdebug_bcr_l5:
mcr p14, 0, r1, c0, c5, 5
mov r0, #0
b kernelmode_cachestuff

.global getdebug_wvr
.type getdebug_wvr, %function
getdebug_wvr:
cmp r0, #0
beq getdebug_wvr_l0
cmp r0, #1
beq getdebug_wvr_l1
mov r0, #0 @ Invalid
bx lr

getdebug_wvr_l0:
mrc p14, 0, r0, c0, c0, 6
bx lr

getdebug_wvr_l1:
mrc p14, 0, r0, c0, c1, 6
bx lr

.global setdebug_wvr
.type setdebug_wvr, %function
setdebug_wvr:
cmp r0, #0
beq setdebug_wvr_l0
cmp r0, #1
beq setdebug_wvr_l1
mov r0, #1 @ Invalid
bx lr

setdebug_wvr_l0:
mcr p14, 0, r1, c0, c0, 6
mov r0, #0
b kernelmode_cachestuff

setdebug_wvr_l1:
mcr p14, 0, r1, c0, c1, 6
mov r0, #0
b kernelmode_cachestuff

.global getdebug_wcr
.type getdebug_wcr, %function
getdebug_wcr:
cmp r0, #0
beq getdebug_wcr_l0
cmp r0, #1
beq getdebug_wcr_l1
mov r0, #0 @ Invalid
bx lr

getdebug_wcr_l0:
mrc p14, 0, r0, c0, c0, 7
bx lr

getdebug_wcr_l1:
mrc p14, 0, r0, c0, c1, 7
bx lr

.global setdebug_wcr
.type setdebug_wcr, %function
setdebug_wcr:
cmp r0, #0
beq setdebug_wcr_l0
cmp r0, #1
beq setdebug_wcr_l1
mov r0, #1 @ Invalid
bx lr

setdebug_wcr_l0:
mcr p14, 0, r1, c0, c0, 7
mov r0, #0
b kernelmode_cachestuff

setdebug_wcr_l1:
mcr p14, 0, r1, c0, c1, 7
mov r0, #0
b kernelmode_cachestuff

