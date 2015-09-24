.global _init
.global _fini
.global launchcode_kernelmode
.global call_arbitaryfuncptr
.global svcControlProcessMemory

.type _init STT_FUNC
.type _fini STT_FUNC
.type launchcode_kernelmode STT_FUNC
.type call_arbitaryfuncptr STT_FUNC
.type svcControlProcessMemory STT_FUNC

.global PROCESSNAME
.global arm11kernel_textvaddr

@---------------------------------------------------------------------------------
@ 3DS processor selection
@---------------------------------------------------------------------------------
	.cpu mpcore
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
	.section ".init"
	.global _start, __service_ptr, __apt_appid, __heap_size, __linear_heap_size, __system_arglist, __system_runflags
@---------------------------------------------------------------------------------
	.align 2
	.arm
@---------------------------------------------------------------------------------
_start:
@---------------------------------------------------------------------------------
b startup
	.ascii "_prm"
__service_ptr:
	.word 0 @ Pointer to service handle override list -- if non-NULL it is assumed that we have been launched from a homebrew launcher
__apt_appid:
	.word 0x300 @ Program APPID
__heap_size:
	.word 24*1024*1024 @ Default heap size (24 MiB)
__linear_heap_size:
	.word 32*1024*1024 @ Default linear heap size (32 MiB)
__system_arglist:
	.word 0 @ Pointer to argument list (argc (u32) followed by that many NULL terminated strings)
__system_runflags:
	.word 0 @ Flags to signal runtime restrictions to ctrulib

.word 0x58584148 @ "HAXX", indicating that the following parameters can be set by the code loader.

PROCESSNAME:
.word 0x434f5250 @ "PROC"
.word 0x454d414e @ "NAME"

arm11kernel_textvaddr:
.word 0x5458544b

startup:
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

ldr r3, =__end__
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
ldr r1, =__bss_start__ @ Clear .bss
ldr r2, =__bss_end__
mov r3, #0

bss_clr:
cmp r1, r2
beq _start_done
str r3, [r1]
add r1, r1, #4
b bss_clr

_start_done:
@ System initialization
mov r0, r4
bl initSystem

pop {r4, r5, r6, r7, r8, lr}

@ Set up argc/argv arguments for main()
ldr r0, =__system_argc
ldr r1, =__system_argv
ldr r0, [r0]
ldr r1, [r1]

@ Jump to user code
ldr r3, =main
ldr lr, =__ctru_exit
bx  r3

.pool

_init:
bx lr

_fini:
bx lr

fail:
.word 0xffffffff

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

.global initsrvhandle_allservices
.type initsrvhandle_allservices, %function
initsrvhandle_allservices: @ Init a srv handle which has access to all services.
push {r4, r5, r6, r7, lr}
sub sp, sp, #0x20

mov r7, #0

mov r0, sp
ldr r1, =0xffff8001
bl svcGetProcessId

mov r7, r0
cmp r7, #0
bne initsrvhandle_allservices_end

mov r4, #0
ldr r5, [sp, #0]
mov r6, #0

adr r0, kernelmode_searchval_overwrite @ r4=address(0=cur kprocess), r5=searchval, r6=val to write
svc 0x7b @ Overwrite kprocess PID with 0.

mov r4, r3
ldr r5, [sp, #0]

bl srvInit
mov r7, r0

adr r0, kernelmode_writeval @ r4=addr, r5=u32val
svc 0x7b @ Restore the original PID.

initsrvhandle_allservices_end:
mov r0, r7
add sp, sp, #0x20
pop {r4, r5, r6, r7, pc}
.pool

kernelmode_searchval_overwrite: @ r4=kprocess, r5=searchval, r6=val to write. out r3 = overwritten addr.
cpsid i @ disable IRQs
push {r4, r5, r6}

cmp r4, #0
bne kernelmode_searchval_overwrite_lp
ldr r4, =0xffff9004
ldr r4, [r4]

kernelmode_searchval_overwrite_lp:
ldr r0, [r4]
cmp r0, r5
addne r4, r4, #4
bne kernelmode_searchval_overwrite_lp

str r6, [r4]
mov r3, r4
pop {r4, r5, r6}
bx lr
.pool

kernelmode_writeval: @ r4=addr, r5=u32val
cpsid i @ disable IRQs
str r5, [r4]
bx lr

.global kernelmode_cachestuff
.type kernelmode_cachestuff, %function
kernelmode_cachestuff:
mov r0, #0
mcr p15, 0, r0, c7, c14, 0 @ "Clean and Invalidate Entire Data Cache"
mcr p15, 0, r0, c7, c10, 5 @ "Data Memory Barrier"
mcr p15, 0, r0, c7, c5, 0 @ "Invalidate Entire Instruction Cache. Also flushes the branch target cache"
mcr p15, 0, r0, c7, c5, 4 @ "Flush Prefetch Buffer"
mcr p15, 0, r0, c7, c5, 6 @ "Flush Entire Branch Target Cache"
mcr p15, 0, r0, c7, c10, 4 @ "Data Synchronization Barrier"
bx lr

