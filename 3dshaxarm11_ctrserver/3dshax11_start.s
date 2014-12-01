.section .init

.arm

.global _start
.global _init
.global launchcode_kernelmode
.global call_arbitaryfuncptr
.global svcControlProcessMemory

.type _init STT_FUNC
.type launchcode_kernelmode STT_FUNC
.type call_arbitaryfuncptr STT_FUNC
.type svcControlProcessMemory STT_FUNC

.global PROCESSNAME
.global arm11kernel_textvaddr

_start:
b codestart

.word 0x58584148 @ "HAXX", indicating that the following parameters can be set by the code loader.

PROCESSNAME:
.word 0x434f5250 @ "PROC"
.word 0x454d414e @ "NAME"

arm11kernel_textvaddr:
.word 0x5458544b

codestart:
push {r4, r5, r6, r7, r8, lr}

adr r0, PROCESSNAME
ldr r1, [r0, #0]
ldr r2, [r0, #4]
ldr r0, =0x434f5250
ldr r3, =0x454d414e
cmp r1, r0
cmpeq r2, r3
beq clearbss @ When the PROCESSNAME wasn't changed, assume we're running under an actual CXI.

ldr r1, =0xFFFF8001
svc 0x27
mov r6, r1
cmp r0, #0
bne fail

ldr r3, =_end
ldr r4, =0xfff
add r3, r3, r4
lsr r3, r3, #12
lsl r3, r3, #12
mov r8, r3

ldr r7, =0x00100000

change_permissions_lp:
mov r0, r6
mov r1, r7
mov r2, #0
ldr r3, =0x1000
mov r4, #6
mov r5, #7
svc 0x70
cmp r0, #0
bne fail
ldr r1, =0x1000
add r7, r7, r1
cmp r7, r8
blt change_permissions_lp

clearbss:
ldr r1, =__bss_start @ Clear .bss
ldr r2, =__bss_end
mov r3, #0

bss_clr:
cmp r1, r2
beq _start_done
str r3, [r1]
add r1, r1, #4
b bss_clr

_start_done:
bl setup_fpscr

ldr r0, =__libc_init_array @ global constructors
blx r0

mov r0, #0
mov r1, #0
ldr r2, =main
blx r2

_start_end:
pop {r4, r5, r6, r7, r8, lr}
bx lr
.pool

_init:
bx lr

fail:
.word 0xffffffff

setup_fpscr:
mov r0, #0x3000000
vmsr fpscr, r0
bx lr

kernelmodestub:
cpsid i @ Disable IRQs
push {lr}
mov r0, r3
blx r4
mov r2, r0
pop {lr}
//cpsie i @ Enable IRQs (don't re-enable IRQs since the svc-handler will just disable IRQs after the SVC returns)
bx lr

launchcode_kernelmode:
push {r4, lr}
mov r4, r0
mov r3, r1
adr r0, kernelmodestub
svc 0x7b
mov r0, r2
pop {r4, pc}

call_arbitaryfuncptr:
push {r4, r5, r6, r7, r8, lr}
mov r7, r0
mov r8, r1
ldm r8, {r0, r1, r2, r3, r4, r5, r6}
blx r7
stm r8, {r0, r1, r2, r3, r4, r5, r6}
pop {r4, r5, r6, r7, r8, pc}

svcControlProcessMemory:
push {r0, r1, r2, r3, r4, r5, lr}
cmp r0, #0
bne svcControlProcessMemory_start

ldr r1, =0xFFFF8001
svc 0x27
mov r0, r1
ldr r1, [sp, #4]
ldr r2, [sp, #8]
ldr r3, [sp, #12]

svcControlProcessMemory_start:
ldr r4, [sp, #28]
ldr r5, [sp, #32]
svc 0x70
add sp, sp, #16
pop {r4, r5, pc}

.global svc_duplicateHandle
.type svc_duplicateHandle, %function
svc_duplicateHandle:
stmfd sp!, {r0}
svc 0x27
ldr r2, [sp], #4
str r1, [r2]
bx lr

