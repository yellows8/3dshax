.arm

.arch armv5te
.fpu softvfp

.global launchcode_kernelmode

.global svcSignalEvent
.global svcFlushProcessDataCache
.global svcGetSystemTick
.global svcCreateThread
.global svcExitThread
.global svcSleepThread
.global svcCloseHandle

.type launchcode_kernelmode STT_FUNC

.type svcSignalEvent STT_FUNC
.type svcFlushProcessDataCache STT_FUNC
.type svcGetSystemTick STT_FUNC
.type svcCreateThread STT_FUNC
.type svcExitThread STT_FUNC
.type svcSleepThread STT_FUNC
.type svcCloseHandle STT_FUNC

.text

launchcode_kernelmode_stub:
mov r0, sp
push {r0, r5, lr}
mrs r5, CPSR
orr r2, r5, #0x80
msr CPSR_c, r2
blx r4
msr CPSR_c, r5
pop {r0, r5, lr}
mov sp, r0
bx lr

launchcode_kernelmode:
push {r4}
mov r4, r0
adr r0, launchcode_kernelmode_stub
svc 0x7b
pop {r4}
bx lr

svcSignalEvent:
svc 0x18
bx lr

svcFlushProcessDataCache:
mov r2, r1
mov r1, r0
ldr r0, =0xffff8001
svc 0x54
bx lr
.pool

svcGetSystemTick:
svc 0x28
bx lr

svcCreateThread:
push {r0, r4}
ldr r0, [sp, #8]
ldr r4, [sp, #12]
svc 0x08
ldr r2, [sp, #0]
str r1, [r2]
add sp, sp, #4
pop {r4}
bx lr

svcExitThread:
svc 0x9
bx lr

svcSleepThread:
svc 0xa
bx lr

svcCloseHandle:
svc 0x23
bx lr

.global svcStartInterProcessDma
.type svcStartInterProcessDma, %function
svcStartInterProcessDma:
	push {r0, r4, r5}
	ldr  r0, [sp, #0xc]
	ldr  r4, [sp, #0xc+0x4]
	ldr  r5, [sp, #0xc+0x8]
	svc  0x55
	ldr  r2, [sp], #4
	str  r1, [r2]
	ldr  r4, [sp], #4
	ldr  r5, [sp], #4
	bx   lr

