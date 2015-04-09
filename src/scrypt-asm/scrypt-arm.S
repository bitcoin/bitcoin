/*
 * Copyright 2012 pooler@litecoinpool.org
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#if defined(__arm__) && defined(__APCS_32__)

.macro salsa8_core_doubleround_body
	ldr	r8, [sp, #8*4]
	add	r11, r11, r10
	ldr	lr, [sp, #13*4]
	add	r12, r12, r3
	eor	r2, r2, r11, ror #23
	add	r11, r4, r0
	eor	r7, r7, r12, ror #23
	add	r12, r9, r5
	str	r9, [sp, #9*4]
	eor	r8, r8, r11, ror #23
	str	r10, [sp, #14*4]
	eor	lr, lr, r12, ror #23
	
	ldr	r11, [sp, #11*4]
	add	r9, lr, r9
	ldr	r12, [sp, #12*4]
	add	r10, r2, r10
	eor	r1, r1, r9, ror #19
	add	r9, r7, r3
	eor	r6, r6, r10, ror #19
	add	r10, r8, r4
	str	r8, [sp, #8*4]
	eor	r11, r11, r9, ror #19
	str	lr, [sp, #13*4]
	eor	r12, r12, r10, ror #19
	
	ldr	r9, [sp, #10*4]
	add	r8, r12, r8
	ldr	r10, [sp, #15*4]
	add	lr, r1, lr
	eor	r0, r0, r8, ror #14
	add	r8, r6, r2
	eor	r5, r5, lr, ror #14
	add	lr, r11, r7
	eor	r9, r9, r8, ror #14
	ldr	r8, [sp, #9*4]
	eor	r10, r10, lr, ror #14
	ldr	lr, [sp, #14*4]
	
	
	add	r8, r9, r8
	str	r9, [sp, #10*4]
	add	lr, r10, lr
	str	r10, [sp, #15*4]
	eor	r11, r11, r8, ror #25
	add	r8, r0, r3
	eor	r12, r12, lr, ror #25
	add	lr, r5, r4
	eor	r1, r1, r8, ror #25
	ldr	r8, [sp, #8*4]
	eor	r6, r6, lr, ror #25
	
	add	r9, r11, r9
	ldr	lr, [sp, #13*4]
	add	r10, r12, r10
	eor	r8, r8, r9, ror #23
	add	r9, r1, r0
	eor	lr, lr, r10, ror #23
	add	r10, r6, r5
	str	r11, [sp, #11*4]
	eor	r2, r2, r9, ror #23
	str	r12, [sp, #12*4]
	eor	r7, r7, r10, ror #23
	
	ldr	r9, [sp, #9*4]
	add	r11, r8, r11
	ldr	r10, [sp, #14*4]
	add	r12, lr, r12
	eor	r9, r9, r11, ror #19
	add	r11, r2, r1
	eor	r10, r10, r12, ror #19
	add	r12, r7, r6
	str	r8, [sp, #8*4]
	eor	r3, r3, r11, ror #19
	str	lr, [sp, #13*4]
	eor	r4, r4, r12, ror #19
.endm

.macro salsa8_core
	ldmia	sp, {r0-r7}
	
	ldr	r12, [sp, #15*4]
	ldr	r8, [sp, #11*4]
	ldr	lr, [sp, #12*4]
	
	ldr	r9, [sp, #9*4]
	add	r8, r8, r12
	ldr	r11, [sp, #10*4]
	add	lr, lr, r0
	eor	r3, r3, r8, ror #25
	add	r8, r5, r1
	ldr	r10, [sp, #14*4]
	eor	r4, r4, lr, ror #25
	add	lr, r11, r6
	eor	r9, r9, r8, ror #25
	eor	r10, r10, lr, ror #25
	
	salsa8_core_doubleround_body
	
	ldr	r11, [sp, #10*4]
	add	r8, r9, r8
	ldr	r12, [sp, #15*4]
	add	lr, r10, lr
	eor	r11, r11, r8, ror #14
	str	r9, [sp, #9*4]
	eor	r12, r12, lr, ror #14
	add	r8, r3, r2
	add	lr, r4, r7
	str	r10, [sp, #14*4]
	eor	r0, r0, r8, ror #14
	ldr	r8, [sp, #11*4]
	eor	r5, r5, lr, ror #14
	ldr	lr, [sp, #12*4]
	
	add	r8, r8, r12
	str	r11, [sp, #10*4]
	add	lr, lr, r0
	str	r12, [sp, #15*4]
	eor	r3, r3, r8, ror #25
	add	r8, r5, r1
	eor	r4, r4, lr, ror #25
	add	lr, r11, r6
	eor	r9, r9, r8, ror #25
	eor	r10, r10, lr, ror #25
	
	salsa8_core_doubleround_body
	
	ldr	r11, [sp, #10*4]
	add	r8, r9, r8
	ldr	r12, [sp, #15*4]
	add	lr, r10, lr
	eor	r11, r11, r8, ror #14
	str	r9, [sp, #9*4]
	eor	r12, r12, lr, ror #14
	add	r8, r3, r2
	add	lr, r4, r7
	str	r10, [sp, #14*4]
	eor	r0, r0, r8, ror #14
	ldr	r8, [sp, #11*4]
	eor	r5, r5, lr, ror #14
	ldr	lr, [sp, #12*4]
	
	add	r8, r8, r12
	str	r11, [sp, #10*4]
	add	lr, lr, r0
	str	r12, [sp, #15*4]
	eor	r3, r3, r8, ror #25
	add	r8, r5, r1
	eor	r4, r4, lr, ror #25
	add	lr, r11, r6
	eor	r9, r9, r8, ror #25
	eor	r10, r10, lr, ror #25
	
	salsa8_core_doubleround_body
	
	ldr	r11, [sp, #10*4]
	add	r8, r9, r8
	ldr	r12, [sp, #15*4]
	add	lr, r10, lr
	eor	r11, r11, r8, ror #14
	str	r9, [sp, #9*4]
	eor	r12, r12, lr, ror #14
	add	r8, r3, r2
	add	lr, r4, r7
	str	r10, [sp, #14*4]
	eor	r0, r0, r8, ror #14
	ldr	r8, [sp, #11*4]
	eor	r5, r5, lr, ror #14
	ldr	lr, [sp, #12*4]
	
	add	r8, r8, r12
	str	r11, [sp, #10*4]
	add	lr, lr, r0
	str	r12, [sp, #15*4]
	eor	r3, r3, r8, ror #25
	add	r8, r5, r1
	eor	r4, r4, lr, ror #25
	add	lr, r11, r6
	eor	r9, r9, r8, ror #25
	eor	r10, r10, lr, ror #25
	
	salsa8_core_doubleround_body
	
	ldr	r11, [sp, #10*4]
	add	r8, r9, r8
	ldr	r12, [sp, #15*4]
	add	lr, r10, lr
	str	r9, [sp, #9*4]
	eor	r11, r11, r8, ror #14
	eor	r12, r12, lr, ror #14
	add	r8, r3, r2
	str	r10, [sp, #14*4]
	add	lr, r4, r7
	str	r11, [sp, #10*4]
	eor	r0, r0, r8, ror #14
	str	r12, [sp, #15*4]
	eor	r5, r5, lr, ror #14
	
	stmia	sp, {r0-r7}
.endm


.macro scrypt_core_macro1a_x4
	ldmia	r0, {r4-r7}
	ldmia	lr!, {r8-r11}
	stmia	r1!, {r4-r7}
	stmia	r3!, {r8-r11}
	eor	r4, r4, r8
	eor	r5, r5, r9
	eor	r6, r6, r10
	eor	r7, r7, r11
	stmia	r0!, {r4-r7}
	stmia	r12!, {r4-r7}
.endm

.macro scrypt_core_macro1b_x4
	ldmia	r3!, {r8-r11}
	ldmia	r2, {r4-r7}
	eor	r8, r8, r4
	eor	r9, r9, r5
	eor	r10, r10, r6
	eor	r11, r11, r7
	ldmia	r0, {r4-r7}
	stmia	r2!, {r8-r11}
	eor	r4, r4, r8
	eor	r5, r5, r9
	eor	r6, r6, r10
	eor	r7, r7, r11
	ldmia	r1!, {r8-r11}
	eor	r4, r4, r8
	eor	r5, r5, r9
	eor	r6, r6, r10
	eor	r7, r7, r11
	stmia	r0!, {r4-r7}
	stmia	r12!, {r4-r7}
.endm

.macro scrypt_core_macro2_x4
	ldmia	r12, {r4-r7}
	ldmia	r0, {r8-r11}
	add	r4, r4, r8
	add	r5, r5, r9
	add	r6, r6, r10
	add	r7, r7, r11
	stmia	r0!, {r4-r7}
	ldmia	r2, {r8-r11}
	eor	r4, r4, r8
	eor	r5, r5, r9
	eor	r6, r6, r10
	eor	r7, r7, r11
	stmia	r2!, {r4-r7}
	stmia	r12!, {r4-r7}
.endm

.macro scrypt_core_macro3_x4
	ldmia	r1!, {r4-r7}
	ldmia	r0, {r8-r11}
	add	r4, r4, r8
	add	r5, r5, r9
	add	r6, r6, r10
	add	r7, r7, r11
	stmia	r0!, {r4-r7}
.endm

.macro scrypt_core_macro3_x6
	ldmia	r1!, {r2-r7}
	ldmia	r0, {r8-r12, lr}
	add	r2, r2, r8
	add	r3, r3, r9
	add	r4, r4, r10
	add	r5, r5, r11
	add	r6, r6, r12
	add	r7, r7, lr
	stmia	r0!, {r2-r7}
.endm


	.text
	.code 32
	.align 2
	.globl scrypt_core
	.globl _scrypt_core
#ifdef __ELF__
	.type scrypt_core, %function
#endif
scrypt_core:
_scrypt_core:
	stmfd	sp!, {r4-r11, lr}
	sub	sp, sp, #20*4
	
	str	r0, [sp, #16*4]
	add	r12, r1, #1024*32*4
	str	r12, [sp, #18*4]
scrypt_core_loop1:
	add	lr, r0, #16*4
	add	r3, r1, #16*4
	mov	r12, sp
	scrypt_core_macro1a_x4
	scrypt_core_macro1a_x4
	scrypt_core_macro1a_x4
	scrypt_core_macro1a_x4
	str	r1, [sp, #17*4]
	
	salsa8_core
	
	ldr	r0, [sp, #16*4]
	mov	r12, sp
	add	r2, r0, #16*4
	scrypt_core_macro2_x4
	scrypt_core_macro2_x4
	scrypt_core_macro2_x4
	scrypt_core_macro2_x4
	
	salsa8_core
	
	ldr	r0, [sp, #16*4]
	mov	r1, sp
	add	r0, r0, #16*4
	scrypt_core_macro3_x6
	scrypt_core_macro3_x6
	ldr	r3, [sp, #17*4]
	ldr	r12, [sp, #18*4]
	scrypt_core_macro3_x4
	
	add	r1, r3, #16*4
	sub	r0, r0, #32*4
	cmp	r1, r12
	bne	scrypt_core_loop1
	
	sub	r1, r1, #1024*32*4
	str	r1, [sp, #17*4]
	mov	r12, #1024
scrypt_core_loop2:
	str	r12, [sp, #18*4]
	
	ldr	r4, [r0, #16*4]
	mov	r4, r4, lsl #32-10
	add	r1, r1, r4, lsr #32-10-7
	
	add	r2, r0, #16*4
	add	r3, r1, #16*4
	mov	r12, sp
	scrypt_core_macro1b_x4
	scrypt_core_macro1b_x4
	scrypt_core_macro1b_x4
	scrypt_core_macro1b_x4
	
	salsa8_core
	
	ldr	r0, [sp, #16*4]
	mov	r12, sp
	add	r2, r0, #16*4
	scrypt_core_macro2_x4
	scrypt_core_macro2_x4
	scrypt_core_macro2_x4
	scrypt_core_macro2_x4
	
	salsa8_core
	
	ldr	r0, [sp, #16*4]
	mov	r1, sp
	add	r0, r0, #16*4
	scrypt_core_macro3_x6
	scrypt_core_macro3_x6
	scrypt_core_macro3_x4
	
	ldr	r12, [sp, #18*4]
	sub	r0, r0, #32*4
	ldr	r1, [sp, #17*4]
	subs	r12, r12, #1
	bne	scrypt_core_loop2
	
	add	sp, sp, #20*4
#ifdef __thumb__
	ldmfd	sp!, {r4-r11, lr}
	bx	lr
#else
	ldmfd	sp!, {r4-r11, pc}
#endif

#endif
