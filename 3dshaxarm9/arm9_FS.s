#ifdef ENABLE_OLDFS
.arm
.global arm9_fopen
.type arm9_fopen STT_FUNC
arm9_fopen:
	push {r4, r5, r6, lr}
	mov r5, #0x1F
	ldr r6, =RUNNINGFWVER
	ldr r6, [r6]
	cmp r5, r6
	ldreq r4, =0x0805CF05  // 0x0805CF04 + 1 for thumb mode, FW 0x1F
	ldrne r4, =0x0805ADA1  // 0x0805ADA0 + 1 for thumb mode, FW 0x2E
	blx r4
	pop {r4, r5, r6, pc}
.pool

.global arm9_fclose
.type arm9_fclose STT_FUNC
arm9_fclose:
	push {r4, r5, r6, lr}
	mov r5, #0x1F
	ldr r6, =RUNNINGFWVER
	ldr r6, [r6]
	cmp r5, r6
	ldreq r4, =0x0805CFC5  // 0x0805CFC4 + 1 for thumb mode, FW 0x1F
	ldrne r4, =0x0805AE8D  // 0x0805AE8C + 1 for thumb mode, FW 0x2E
	blx r4
	pop {r4, r5, r6, pc}
.pool

.global arm9_fread
.type arm9_fread STT_FUNC
arm9_fread:
	push {r4, r5, r6, lr}
	mov r5, #0x1F
	ldr r6, =RUNNINGFWVER
	ldr r6, [r6]
	cmp r5, r6
	ldreq r4, =0x0804E315  // 0x0804E314 + 1 for thumb mode, FW 0x1F
	ldrne r4, =0x0804D70D  // 0x0804D70C + 1 for thumb mode, FW 0x2E
	blx r4
	pop {r4, r5, r6, pc}
.pool

.global arm9_fwrite
.type arm9_fwrite STT_FUNC
arm9_fwrite:
	push {r4, r5, r6, lr}
	sub sp, sp, #4
	ldr r4, [sp, #20]
	str r4, [sp, #0]
	mov r5, #0x1F
	ldr r6, =RUNNINGFWVER
	ldr r6, [r6]
	cmp r5, r6
	ldreq r4, =0x0805E181  // 0x0805E180 + 1 for thumb mode, FW 0x1F
	ldrne r4, =0x0805C19B  // 0x0805C19A + 1 for thumb mode, FW 0x2E
	blx r4
	add sp, sp, #4
	pop {r4, r5, r6, pc}
.pool

.global arm9_GetFSize
.type arm9_GetFSize STT_FUNC
arm9_GetFSize:
	push {r4, r5, r6, lr}
	mov r5, #0x1F
	ldr r6, =RUNNINGFWVER
	ldr r6, [r6]
	cmp r5, r6
  	ldreq r4, =0x0805DEF5  // 0x0805DEF4 + 1 for thumb mode, FW 0x1F
  	ldrne r4, =0x0805BF99  // 0x0805BF98 + 1 for thumb mode, FW 0x2E
	blx r4
	pop {r4, r5, r6, pc}
.pool
#endif

