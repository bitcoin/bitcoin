@ vim: set tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab syntax=armasm:
/**********************************************************************
 * Copyright (c) 2014 Wladimir J. van der Laan                        *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/
/*
ARM implementation of field_10x26 inner loops.

Note:

- To avoid unnecessary loads and make use of available registers, two
  'passes' have every time been interleaved, with the odd passes accumulating c' and d' 
  which will be added to c and d respectively in the even passes

*/

	.syntax unified
	.arch armv7-a
	@ eabi attributes - see readelf -A
	.eabi_attribute 8, 1  @ Tag_ARM_ISA_use = yes
	.eabi_attribute 9, 0  @ Tag_Thumb_ISA_use = no
	.eabi_attribute 10, 0 @ Tag_FP_arch = none
	.eabi_attribute 24, 1 @ Tag_ABI_align_needed = 8-byte
	.eabi_attribute 25, 1 @ Tag_ABI_align_preserved = 8-byte, except leaf SP
	.eabi_attribute 30, 2 @ Tag_ABI_optimization_goals = Agressive Speed
	.eabi_attribute 34, 1 @ Tag_CPU_unaligned_access = v6
	.text

	@ Field constants
	.set field_R0, 0x3d10
	.set field_R1, 0x400
	.set field_not_M, 0xfc000000	@ ~M = ~0x3ffffff

	.align	2
	.global secp256k1_fe_mul_inner
	.type	secp256k1_fe_mul_inner, %function
	@ Arguments:
	@  r0  r      Restrict: can overlap with a, not with b
	@  r1  a
	@  r2  b
	@ Stack (total 4+10*4 = 44)
	@  sp + #0        saved 'r' pointer
	@  sp + #4 + 4*X  t0,t1,t2,t3,t4,t5,t6,t7,u8,t9
secp256k1_fe_mul_inner:
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r14}
	sub	sp, sp, #48			@ frame=44 + alignment
	str     r0, [sp, #0]			@ save result address, we need it only at the end

	/******************************************
	 * Main computation code.
	 ******************************************

	Allocation:
	    r0,r14,r7,r8   scratch
	    r1       a (pointer)
	    r2       b (pointer)
	    r3:r4    c
	    r5:r6    d
	    r11:r12  c'
	    r9:r10   d'

	Note: do not write to r[] here, it may overlap with a[]
	*/

	/* A - interleaved with B */
	ldr	r7, [r1, #0*4]			@ a[0]
	ldr	r8, [r2, #9*4]			@ b[9]
	ldr	r0, [r1, #1*4]			@ a[1]
	umull	r5, r6, r7, r8			@ d = a[0] * b[9]
	ldr	r14, [r2, #8*4]			@ b[8]
	umull	r9, r10, r0, r8			@ d' = a[1] * b[9]
	ldr	r7, [r1, #2*4]			@ a[2]
	umlal	r5, r6, r0, r14			@ d += a[1] * b[8]
	ldr	r8, [r2, #7*4] 			@ b[7]
	umlal	r9, r10, r7, r14		@ d' += a[2] * b[8]
	ldr	r0, [r1, #3*4]   		@ a[3]
	umlal	r5, r6, r7, r8   		@ d += a[2] * b[7]
	ldr	r14, [r2, #6*4]   		@ b[6]
	umlal	r9, r10, r0, r8  		@ d' += a[3] * b[7]
	ldr	r7, [r1, #4*4]   		@ a[4]
	umlal	r5, r6, r0, r14   		@ d += a[3] * b[6]
	ldr	r8, [r2, #5*4]   		@ b[5]
	umlal	r9, r10, r7, r14  		@ d' += a[4] * b[6]
	ldr	r0, [r1, #5*4]   		@ a[5]
	umlal	r5, r6, r7, r8   		@ d += a[4] * b[5]
	ldr	r14, [r2, #4*4]   		@ b[4]
	umlal	r9, r10, r0, r8  		@ d' += a[5] * b[5]
	ldr	r7, [r1, #6*4]   		@ a[6]
	umlal	r5, r6, r0, r14   		@ d += a[5] * b[4]
	ldr	r8, [r2, #3*4]   		@ b[3]
	umlal	r9, r10, r7, r14  		@ d' += a[6] * b[4]
	ldr	r0, [r1, #7*4]   		@ a[7]
	umlal	r5, r6, r7, r8   		@ d += a[6] * b[3]
	ldr	r14, [r2, #2*4]   		@ b[2]
	umlal	r9, r10, r0, r8  		@ d' += a[7] * b[3]
	ldr	r7, [r1, #8*4]   		@ a[8]
	umlal	r5, r6, r0, r14   		@ d += a[7] * b[2]
	ldr	r8, [r2, #1*4]   		@ b[1]
	umlal	r9, r10, r7, r14  		@ d' += a[8] * b[2]
	ldr	r0, [r1, #9*4]   		@ a[9]
	umlal	r5, r6, r7, r8   		@ d += a[8] * b[1]
	ldr	r14, [r2, #0*4]   		@ b[0]
	umlal	r9, r10, r0, r8  		@ d' += a[9] * b[1]
	ldr	r7, [r1, #0*4]   		@ a[0]
	umlal	r5, r6, r0, r14   		@ d += a[9] * b[0]
	@ r7,r14 used in B

	bic	r0, r5, field_not_M 		@ t9 = d & M
	str     r0, [sp, #4 + 4*9]
	mov	r5, r5, lsr #26     		@ d >>= 26 
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26

	/* B */
	umull	r3, r4, r7, r14   		@ c = a[0] * b[0]
	adds	r5, r5, r9       		@ d += d'
	adc	r6, r6, r10

	bic	r0, r5, field_not_M 		@ u0 = d & M
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u0 * R0
	umlal   r3, r4, r0, r14

	bic	r14, r3, field_not_M 		@ t0 = c & M
	str	r14, [sp, #4 + 0*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u0 * R1
	umlal   r3, r4, r0, r14

	/* C - interleaved with D */
	ldr	r7, [r1, #0*4]   		@ a[0]
	ldr	r8, [r2, #2*4]   		@ b[2]
	ldr	r14, [r2, #1*4]   		@ b[1]
	umull	r11, r12, r7, r8   		@ c' = a[0] * b[2]
	ldr	r0, [r1, #1*4]   		@ a[1]
	umlal   r3, r4, r7, r14   		@ c += a[0] * b[1]
	ldr	r8, [r2, #0*4]   		@ b[0]
	umlal   r11, r12, r0, r14   		@ c' += a[1] * b[1]
	ldr	r7, [r1, #2*4]   		@ a[2]
	umlal   r3, r4, r0, r8   		@ c += a[1] * b[0]
	ldr	r14, [r2, #9*4]   		@ b[9]
	umlal   r11, r12, r7, r8   		@ c' += a[2] * b[0]
	ldr	r0, [r1, #3*4]   		@ a[3]
	umlal	r5, r6, r7, r14   		@ d += a[2] * b[9]
	ldr	r8, [r2, #8*4]   		@ b[8]
	umull	r9, r10, r0, r14   		@ d' = a[3] * b[9]
	ldr	r7, [r1, #4*4]   		@ a[4]
	umlal	r5, r6, r0, r8   		@ d += a[3] * b[8]
	ldr	r14, [r2, #7*4]   		@ b[7]
	umlal	r9, r10, r7, r8   		@ d' += a[4] * b[8]
	ldr	r0, [r1, #5*4]   		@ a[5]
	umlal	r5, r6, r7, r14   		@ d += a[4] * b[7]
	ldr	r8, [r2, #6*4]   		@ b[6]
	umlal	r9, r10, r0, r14   		@ d' += a[5] * b[7]
	ldr	r7, [r1, #6*4]   		@ a[6]
	umlal	r5, r6, r0, r8   		@ d += a[5] * b[6]
	ldr	r14, [r2, #5*4]   		@ b[5]
	umlal	r9, r10, r7, r8   		@ d' += a[6] * b[6]
	ldr	r0, [r1, #7*4]   		@ a[7]
	umlal	r5, r6, r7, r14   		@ d += a[6] * b[5]
	ldr	r8, [r2, #4*4]   		@ b[4]
	umlal	r9, r10, r0, r14   		@ d' += a[7] * b[5]
	ldr	r7, [r1, #8*4]   		@ a[8]
	umlal	r5, r6, r0, r8   		@ d += a[7] * b[4]
	ldr	r14, [r2, #3*4]   		@ b[3]
	umlal	r9, r10, r7, r8   		@ d' += a[8] * b[4]
	ldr	r0, [r1, #9*4]   		@ a[9]
	umlal	r5, r6, r7, r14   		@ d += a[8] * b[3]
	ldr	r8, [r2, #2*4]   		@ b[2]
	umlal	r9, r10, r0, r14   		@ d' += a[9] * b[3]
	umlal	r5, r6, r0, r8   		@ d += a[9] * b[2]

	bic	r0, r5, field_not_M 		@ u1 = d & M
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u1 * R0
	umlal   r3, r4, r0, r14

	bic	r14, r3, field_not_M 		@ t1 = c & M
	str	r14, [sp, #4 + 1*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u1 * R1
	umlal   r3, r4, r0, r14

	/* D */
	adds	r3, r3, r11			@ c += c'
	adc	r4, r4, r12
	adds	r5, r5, r9			@ d += d'
	adc	r6, r6, r10

	bic	r0, r5, field_not_M 		@ u2 = d & M
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u2 * R0
	umlal   r3, r4, r0, r14

	bic	r14, r3, field_not_M 		@ t2 = c & M
	str	r14, [sp, #4 + 2*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u2 * R1
	umlal   r3, r4, r0, r14

	/* E - interleaved with F */
	ldr	r7, [r1, #0*4]   		@ a[0]
	ldr	r8, [r2, #4*4]   		@ b[4]
	umull	r11, r12, r7, r8   		@ c' = a[0] * b[4]
	ldr	r8, [r2, #3*4]   		@ b[3]
	umlal   r3, r4, r7, r8   		@ c += a[0] * b[3]
	ldr	r7, [r1, #1*4]   		@ a[1]
	umlal   r11, r12, r7, r8   		@ c' += a[1] * b[3]
	ldr	r8, [r2, #2*4]   		@ b[2]
	umlal   r3, r4, r7, r8   		@ c += a[1] * b[2]
	ldr	r7, [r1, #2*4]   		@ a[2]
	umlal   r11, r12, r7, r8   		@ c' += a[2] * b[2]
	ldr	r8, [r2, #1*4]   		@ b[1]
	umlal   r3, r4, r7, r8   		@ c += a[2] * b[1]
	ldr	r7, [r1, #3*4]   		@ a[3]
	umlal   r11, r12, r7, r8   		@ c' += a[3] * b[1]
	ldr	r8, [r2, #0*4]   		@ b[0]
	umlal   r3, r4, r7, r8   		@ c += a[3] * b[0]
	ldr	r7, [r1, #4*4]   		@ a[4]
	umlal   r11, r12, r7, r8   		@ c' += a[4] * b[0]
	ldr	r8, [r2, #9*4]   		@ b[9]
	umlal	r5, r6, r7, r8   		@ d += a[4] * b[9]
	ldr	r7, [r1, #5*4]   		@ a[5]
	umull	r9, r10, r7, r8   		@ d' = a[5] * b[9]
	ldr	r8, [r2, #8*4]   		@ b[8]
	umlal	r5, r6, r7, r8   		@ d += a[5] * b[8]
	ldr	r7, [r1, #6*4]   		@ a[6]
	umlal	r9, r10, r7, r8   		@ d' += a[6] * b[8]
	ldr	r8, [r2, #7*4]   		@ b[7]
	umlal	r5, r6, r7, r8   		@ d += a[6] * b[7]
	ldr	r7, [r1, #7*4]   		@ a[7]
	umlal	r9, r10, r7, r8   		@ d' += a[7] * b[7]
	ldr	r8, [r2, #6*4]   		@ b[6]
	umlal	r5, r6, r7, r8   		@ d += a[7] * b[6]
	ldr	r7, [r1, #8*4]   		@ a[8]
	umlal	r9, r10, r7, r8   		@ d' += a[8] * b[6]
	ldr	r8, [r2, #5*4]   		@ b[5]
	umlal	r5, r6, r7, r8   		@ d += a[8] * b[5]
	ldr	r7, [r1, #9*4]   		@ a[9]
	umlal	r9, r10, r7, r8   		@ d' += a[9] * b[5]
	ldr	r8, [r2, #4*4]   		@ b[4]
	umlal	r5, r6, r7, r8   		@ d += a[9] * b[4]

	bic	r0, r5, field_not_M 		@ u3 = d & M
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u3 * R0
	umlal   r3, r4, r0, r14

	bic	r14, r3, field_not_M 		@ t3 = c & M
	str	r14, [sp, #4 + 3*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u3 * R1
	umlal   r3, r4, r0, r14

	/* F */
	adds	r3, r3, r11			@ c += c'
	adc	r4, r4, r12
	adds	r5, r5, r9			@ d += d'
	adc	r6, r6, r10

	bic	r0, r5, field_not_M 		@ u4 = d & M
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u4 * R0
	umlal   r3, r4, r0, r14

	bic	r14, r3, field_not_M 		@ t4 = c & M
	str	r14, [sp, #4 + 4*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u4 * R1
	umlal   r3, r4, r0, r14

	/* G - interleaved with H */
	ldr	r7, [r1, #0*4]   		@ a[0]
	ldr	r8, [r2, #6*4]   		@ b[6]
	ldr	r14, [r2, #5*4]   		@ b[5]
	umull	r11, r12, r7, r8   		@ c' = a[0] * b[6]
	ldr	r0, [r1, #1*4]   		@ a[1]
	umlal   r3, r4, r7, r14   		@ c += a[0] * b[5]
	ldr	r8, [r2, #4*4]   		@ b[4]
	umlal   r11, r12, r0, r14   		@ c' += a[1] * b[5]
	ldr	r7, [r1, #2*4]   		@ a[2]
	umlal   r3, r4, r0, r8   		@ c += a[1] * b[4]
	ldr	r14, [r2, #3*4]   		@ b[3]
	umlal   r11, r12, r7, r8   		@ c' += a[2] * b[4]
	ldr	r0, [r1, #3*4]   		@ a[3]
	umlal   r3, r4, r7, r14   		@ c += a[2] * b[3]
	ldr	r8, [r2, #2*4]   		@ b[2]
	umlal   r11, r12, r0, r14   		@ c' += a[3] * b[3]
	ldr	r7, [r1, #4*4]   		@ a[4]
	umlal   r3, r4, r0, r8   		@ c += a[3] * b[2]
	ldr	r14, [r2, #1*4]   		@ b[1]
	umlal   r11, r12, r7, r8   		@ c' += a[4] * b[2]
	ldr	r0, [r1, #5*4]   		@ a[5]
	umlal   r3, r4, r7, r14   		@ c += a[4] * b[1]
	ldr	r8, [r2, #0*4]   		@ b[0]
	umlal   r11, r12, r0, r14   		@ c' += a[5] * b[1]
	ldr	r7, [r1, #6*4]   		@ a[6]
	umlal   r3, r4, r0, r8   		@ c += a[5] * b[0]
	ldr	r14, [r2, #9*4]   		@ b[9]
	umlal   r11, r12, r7, r8   		@ c' += a[6] * b[0]
	ldr	r0, [r1, #7*4]   		@ a[7]
	umlal	r5, r6, r7, r14   		@ d += a[6] * b[9]
	ldr	r8, [r2, #8*4]   		@ b[8]
	umull	r9, r10, r0, r14   		@ d' = a[7] * b[9]
	ldr	r7, [r1, #8*4]   		@ a[8]
	umlal	r5, r6, r0, r8   		@ d += a[7] * b[8]
	ldr	r14, [r2, #7*4]   		@ b[7]
	umlal	r9, r10, r7, r8   		@ d' += a[8] * b[8]
	ldr	r0, [r1, #9*4]   		@ a[9]
	umlal	r5, r6, r7, r14   		@ d += a[8] * b[7]
	ldr	r8, [r2, #6*4]   		@ b[6]
	umlal	r9, r10, r0, r14   		@ d' += a[9] * b[7]
	umlal	r5, r6, r0, r8   		@ d += a[9] * b[6]

	bic	r0, r5, field_not_M 		@ u5 = d & M
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u5 * R0
	umlal   r3, r4, r0, r14

	bic	r14, r3, field_not_M 		@ t5 = c & M
	str	r14, [sp, #4 + 5*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u5 * R1
	umlal   r3, r4, r0, r14

	/* H */
	adds	r3, r3, r11			@ c += c'
	adc	r4, r4, r12
	adds	r5, r5, r9			@ d += d'
	adc	r6, r6, r10

	bic	r0, r5, field_not_M 		@ u6 = d & M
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u6 * R0
	umlal   r3, r4, r0, r14

	bic	r14, r3, field_not_M 		@ t6 = c & M
	str	r14, [sp, #4 + 6*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u6 * R1
	umlal   r3, r4, r0, r14

	/* I - interleaved with J */
	ldr	r8, [r2, #8*4]   		@ b[8]
	ldr	r7, [r1, #0*4]   		@ a[0]
	ldr	r14, [r2, #7*4]   		@ b[7]
	umull   r11, r12, r7, r8   		@ c' = a[0] * b[8]
	ldr	r0, [r1, #1*4]   		@ a[1]
	umlal   r3, r4, r7, r14   		@ c += a[0] * b[7]
	ldr	r8, [r2, #6*4]   		@ b[6]
	umlal   r11, r12, r0, r14   		@ c' += a[1] * b[7]
	ldr	r7, [r1, #2*4]   		@ a[2]
	umlal   r3, r4, r0, r8   		@ c += a[1] * b[6]
	ldr	r14, [r2, #5*4]   		@ b[5]
	umlal   r11, r12, r7, r8   		@ c' += a[2] * b[6]
	ldr	r0, [r1, #3*4]   		@ a[3]
	umlal   r3, r4, r7, r14   		@ c += a[2] * b[5]
	ldr	r8, [r2, #4*4]   		@ b[4]
	umlal   r11, r12, r0, r14   		@ c' += a[3] * b[5]
	ldr	r7, [r1, #4*4]   		@ a[4]
	umlal   r3, r4, r0, r8   		@ c += a[3] * b[4]
	ldr	r14, [r2, #3*4]   		@ b[3]
	umlal   r11, r12, r7, r8   		@ c' += a[4] * b[4]
	ldr	r0, [r1, #5*4]   		@ a[5]
	umlal   r3, r4, r7, r14   		@ c += a[4] * b[3]
	ldr	r8, [r2, #2*4]   		@ b[2]
	umlal   r11, r12, r0, r14   		@ c' += a[5] * b[3]
	ldr	r7, [r1, #6*4]   		@ a[6]
	umlal   r3, r4, r0, r8   		@ c += a[5] * b[2]
	ldr	r14, [r2, #1*4]   		@ b[1]
	umlal   r11, r12, r7, r8   		@ c' += a[6] * b[2]
	ldr	r0, [r1, #7*4]   		@ a[7]
	umlal   r3, r4, r7, r14   		@ c += a[6] * b[1]
	ldr	r8, [r2, #0*4]   		@ b[0]
	umlal   r11, r12, r0, r14   		@ c' += a[7] * b[1]
	ldr	r7, [r1, #8*4]   		@ a[8]
	umlal   r3, r4, r0, r8   		@ c += a[7] * b[0]
	ldr	r14, [r2, #9*4]   		@ b[9]
	umlal   r11, r12, r7, r8   		@ c' += a[8] * b[0]
	ldr	r0, [r1, #9*4]   		@ a[9]
	umlal	r5, r6, r7, r14   		@ d += a[8] * b[9]
	ldr	r8, [r2, #8*4]   		@ b[8]
	umull	r9, r10, r0, r14  		@ d' = a[9] * b[9]
	umlal	r5, r6, r0, r8   		@ d += a[9] * b[8]

	bic	r0, r5, field_not_M 		@ u7 = d & M
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u7 * R0
	umlal   r3, r4, r0, r14

	bic	r14, r3, field_not_M 		@ t7 = c & M
	str	r14, [sp, #4 + 7*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u7 * R1
	umlal   r3, r4, r0, r14

	/* J */
	adds	r3, r3, r11			@ c += c'
	adc	r4, r4, r12
	adds	r5, r5, r9			@ d += d'
	adc	r6, r6, r10

	bic	r0, r5, field_not_M 		@ u8 = d & M
	str	r0, [sp, #4 + 8*4]
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u8 * R0
	umlal   r3, r4, r0, r14

	/******************************************
	 * compute and write back result
	 ******************************************
	Allocation:
	    r0    r
	    r3:r4 c
	    r5:r6 d
	    r7    t0
	    r8    t1
	    r9    t2
	    r11   u8
	    r12   t9
	    r1,r2,r10,r14 scratch

	Note: do not read from a[] after here, it may overlap with r[]
	*/
	ldr	r0, [sp, #0]
	add	r1, sp, #4 + 3*4		@ r[3..7] = t3..7, r11=u8, r12=t9
	ldmia	r1, {r2,r7,r8,r9,r10,r11,r12}
	add	r1, r0, #3*4
	stmia	r1, {r2,r7,r8,r9,r10}

	bic	r2, r3, field_not_M 		@ r[8] = c & M
	str	r2, [r0, #8*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u8 * R1
	umlal   r3, r4, r11, r14
	movw    r14, field_R0			@ c += d * R0
	umlal   r3, r4, r5, r14
	adds	r3, r3, r12			@ c += t9
	adc	r4, r4, #0

	add	r1, sp, #4 + 0*4		@ r7,r8,r9 = t0,t1,t2
	ldmia	r1, {r7,r8,r9}

	ubfx	r2, r3, #0, #22     		@ r[9] = c & (M >> 4)
	str	r2, [r0, #9*4]
	mov	r3, r3, lsr #22     		@ c >>= 22
	orr	r3, r3, r4, asl #10
	mov     r4, r4, lsr #22
	movw    r14, field_R1 << 4   		@ c += d * (R1 << 4)
	umlal   r3, r4, r5, r14

	movw    r14, field_R0 >> 4   		@ d = c * (R0 >> 4) + t0 (64x64 multiply+add)
	umull	r5, r6, r3, r14			@ d = c.lo * (R0 >> 4)
	adds	r5, r5, r7	    		@ d.lo += t0
	mla	r6, r14, r4, r6			@ d.hi += c.hi * (R0 >> 4)
	adc	r6, r6, 0	     		@ d.hi += carry

	bic	r2, r5, field_not_M 		@ r[0] = d & M
	str	r2, [r0, #0*4]

	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	
	movw    r14, field_R1 >> 4   		@ d += c * (R1 >> 4) + t1 (64x64 multiply+add)
	umull	r1, r2, r3, r14       		@ tmp = c.lo * (R1 >> 4)
	adds	r5, r5, r8	    		@ d.lo += t1
	adc	r6, r6, #0	    		@ d.hi += carry
	adds	r5, r5, r1	    		@ d.lo += tmp.lo
	mla	r2, r14, r4, r2      		@ tmp.hi += c.hi * (R1 >> 4)
	adc	r6, r6, r2	   		@ d.hi += carry + tmp.hi

	bic	r2, r5, field_not_M 		@ r[1] = d & M
	str	r2, [r0, #1*4]
	mov	r5, r5, lsr #26     		@ d >>= 26 (ignore hi)
	orr	r5, r5, r6, asl #6

	add	r5, r5, r9	  		@ d += t2
	str	r5, [r0, #2*4]      		@ r[2] = d

	add	sp, sp, #48
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, pc}
	.size	secp256k1_fe_mul_inner, .-secp256k1_fe_mul_inner

	.align	2
	.global secp256k1_fe_sqr_inner
	.type	secp256k1_fe_sqr_inner, %function
	@ Arguments:
	@  r0  r	 Can overlap with a
	@  r1  a
	@ Stack (total 4+10*4 = 44)
	@  sp + #0        saved 'r' pointer
	@  sp + #4 + 4*X  t0,t1,t2,t3,t4,t5,t6,t7,u8,t9
secp256k1_fe_sqr_inner:
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, r14}
	sub	sp, sp, #48			@ frame=44 + alignment
	str     r0, [sp, #0]			@ save result address, we need it only at the end
	/******************************************
	 * Main computation code.
	 ******************************************

	Allocation:
	    r0,r14,r2,r7,r8   scratch
	    r1       a (pointer)
	    r3:r4    c
	    r5:r6    d
	    r11:r12  c'
	    r9:r10   d'

	Note: do not write to r[] here, it may overlap with a[]
	*/
	/* A interleaved with B */
	ldr	r0, [r1, #1*4]			@ a[1]*2
	ldr	r7, [r1, #0*4]			@ a[0]
	mov	r0, r0, asl #1
	ldr	r14, [r1, #9*4]			@ a[9]
	umull	r3, r4, r7, r7			@ c = a[0] * a[0]
	ldr	r8, [r1, #8*4]			@ a[8]
	mov	r7, r7, asl #1
	umull	r5, r6, r7, r14			@ d = a[0]*2 * a[9]
	ldr	r7, [r1, #2*4]			@ a[2]*2
	umull	r9, r10, r0, r14		@ d' = a[1]*2 * a[9]
	ldr	r14, [r1, #7*4]			@ a[7]
	umlal	r5, r6, r0, r8			@ d += a[1]*2 * a[8]
	mov	r7, r7, asl #1
	ldr	r0, [r1, #3*4]			@ a[3]*2
	umlal	r9, r10, r7, r8			@ d' += a[2]*2 * a[8]
	ldr	r8, [r1, #6*4]			@ a[6]
	umlal	r5, r6, r7, r14			@ d += a[2]*2 * a[7]
	mov	r0, r0, asl #1
	ldr	r7, [r1, #4*4]			@ a[4]*2
	umlal	r9, r10, r0, r14		@ d' += a[3]*2 * a[7]
	ldr	r14, [r1, #5*4]			@ a[5]
	mov	r7, r7, asl #1
	umlal	r5, r6, r0, r8			@ d += a[3]*2 * a[6]
	umlal	r9, r10, r7, r8			@ d' += a[4]*2 * a[6]
	umlal	r5, r6, r7, r14			@ d += a[4]*2 * a[5]
	umlal	r9, r10, r14, r14		@ d' += a[5] * a[5]

	bic	r0, r5, field_not_M 		@ t9 = d & M
	str     r0, [sp, #4 + 9*4]
	mov	r5, r5, lsr #26     		@ d >>= 26 
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26

	/* B */
	adds	r5, r5, r9			@ d += d'
	adc	r6, r6, r10

	bic	r0, r5, field_not_M 		@ u0 = d & M
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u0 * R0
	umlal   r3, r4, r0, r14
	bic	r14, r3, field_not_M 		@ t0 = c & M
	str	r14, [sp, #4 + 0*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u0 * R1
	umlal   r3, r4, r0, r14

	/* C interleaved with D */
	ldr	r0, [r1, #0*4]			@ a[0]*2
	ldr	r14, [r1, #1*4]			@ a[1]
	mov	r0, r0, asl #1
	ldr	r8, [r1, #2*4]			@ a[2]
	umlal	r3, r4, r0, r14			@ c += a[0]*2 * a[1]
	mov	r7, r8, asl #1                  @ a[2]*2
	umull	r11, r12, r14, r14		@ c' = a[1] * a[1]
	ldr	r14, [r1, #9*4]			@ a[9]
	umlal	r11, r12, r0, r8		@ c' += a[0]*2 * a[2]
	ldr	r0, [r1, #3*4]			@ a[3]*2
	ldr	r8, [r1, #8*4]			@ a[8]
	umlal	r5, r6, r7, r14			@ d += a[2]*2 * a[9]
	mov	r0, r0, asl #1
	ldr	r7, [r1, #4*4]			@ a[4]*2
	umull	r9, r10, r0, r14		@ d' = a[3]*2 * a[9]
	ldr	r14, [r1, #7*4]			@ a[7]
	umlal	r5, r6, r0, r8			@ d += a[3]*2 * a[8]
	mov	r7, r7, asl #1
	ldr	r0, [r1, #5*4]			@ a[5]*2
	umlal	r9, r10, r7, r8			@ d' += a[4]*2 * a[8]
	ldr	r8, [r1, #6*4]			@ a[6]
	mov	r0, r0, asl #1
	umlal	r5, r6, r7, r14			@ d += a[4]*2 * a[7]
	umlal	r9, r10, r0, r14		@ d' += a[5]*2 * a[7]
	umlal	r5, r6, r0, r8			@ d += a[5]*2 * a[6]
	umlal	r9, r10, r8, r8			@ d' += a[6] * a[6]

	bic	r0, r5, field_not_M 		@ u1 = d & M
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u1 * R0
	umlal   r3, r4, r0, r14
	bic	r14, r3, field_not_M 		@ t1 = c & M
	str	r14, [sp, #4 + 1*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u1 * R1
	umlal   r3, r4, r0, r14

	/* D */
	adds	r3, r3, r11			@ c += c'
	adc	r4, r4, r12
	adds	r5, r5, r9			@ d += d'
	adc	r6, r6, r10

	bic	r0, r5, field_not_M 		@ u2 = d & M
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u2 * R0
	umlal   r3, r4, r0, r14
	bic	r14, r3, field_not_M 		@ t2 = c & M
	str	r14, [sp, #4 + 2*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u2 * R1
	umlal   r3, r4, r0, r14

	/* E interleaved with F */
	ldr	r7, [r1, #0*4]			@ a[0]*2
	ldr	r0, [r1, #1*4]			@ a[1]*2
	ldr	r14, [r1, #2*4]			@ a[2]
	mov	r7, r7, asl #1
	ldr	r8, [r1, #3*4]			@ a[3]
	ldr	r2, [r1, #4*4]
	umlal	r3, r4, r7, r8			@ c += a[0]*2 * a[3]
	mov	r0, r0, asl #1
	umull	r11, r12, r7, r2		@ c' = a[0]*2 * a[4]
	mov	r2, r2, asl #1			@ a[4]*2
	umlal	r11, r12, r0, r8		@ c' += a[1]*2 * a[3]
	ldr	r8, [r1, #9*4]			@ a[9]
	umlal	r3, r4, r0, r14			@ c += a[1]*2 * a[2]
	ldr	r0, [r1, #5*4]			@ a[5]*2
	umlal	r11, r12, r14, r14		@ c' += a[2] * a[2]
	ldr	r14, [r1, #8*4]			@ a[8]
	mov	r0, r0, asl #1
	umlal	r5, r6, r2, r8			@ d += a[4]*2 * a[9]
	ldr	r7, [r1, #6*4]			@ a[6]*2
	umull	r9, r10, r0, r8			@ d' = a[5]*2 * a[9]
	mov	r7, r7, asl #1
	ldr	r8, [r1, #7*4]			@ a[7]
	umlal	r5, r6, r0, r14			@ d += a[5]*2 * a[8]
	umlal	r9, r10, r7, r14		@ d' += a[6]*2 * a[8]
	umlal	r5, r6, r7, r8			@ d += a[6]*2 * a[7]
	umlal	r9, r10, r8, r8			@ d' += a[7] * a[7]

	bic	r0, r5, field_not_M 		@ u3 = d & M
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u3 * R0
	umlal   r3, r4, r0, r14
	bic	r14, r3, field_not_M 		@ t3 = c & M
	str	r14, [sp, #4 + 3*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u3 * R1
	umlal   r3, r4, r0, r14

	/* F */
	adds	r3, r3, r11			@ c += c'
	adc	r4, r4, r12
	adds	r5, r5, r9			@ d += d'
	adc	r6, r6, r10

	bic	r0, r5, field_not_M 		@ u4 = d & M
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u4 * R0
	umlal   r3, r4, r0, r14
	bic	r14, r3, field_not_M 		@ t4 = c & M
	str	r14, [sp, #4 + 4*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u4 * R1
	umlal   r3, r4, r0, r14

	/* G interleaved with H */
	ldr	r7, [r1, #0*4]			@ a[0]*2
	ldr	r0, [r1, #1*4]			@ a[1]*2
	mov	r7, r7, asl #1
	ldr	r8, [r1, #5*4]			@ a[5]
	ldr	r2, [r1, #6*4]			@ a[6]
	umlal	r3, r4, r7, r8			@ c += a[0]*2 * a[5]
	ldr	r14, [r1, #4*4]			@ a[4]
	mov	r0, r0, asl #1
	umull	r11, r12, r7, r2		@ c' = a[0]*2 * a[6]
	ldr	r7, [r1, #2*4]			@ a[2]*2
	umlal	r11, r12, r0, r8		@ c' += a[1]*2 * a[5]
	mov	r7, r7, asl #1
	ldr	r8, [r1, #3*4]			@ a[3]
	umlal	r3, r4, r0, r14			@ c += a[1]*2 * a[4]
	mov	r0, r2, asl #1			@ a[6]*2
	umlal	r11, r12, r7, r14		@ c' += a[2]*2 * a[4]
	ldr	r14, [r1, #9*4]			@ a[9]
	umlal	r3, r4, r7, r8			@ c += a[2]*2 * a[3]
	ldr	r7, [r1, #7*4]			@ a[7]*2
	umlal	r11, r12, r8, r8		@ c' += a[3] * a[3]
	mov	r7, r7, asl #1
	ldr	r8, [r1, #8*4]			@ a[8]
	umlal	r5, r6, r0, r14			@ d += a[6]*2 * a[9]
	umull	r9, r10, r7, r14		@ d' = a[7]*2 * a[9]
	umlal	r5, r6, r7, r8			@ d += a[7]*2 * a[8]
	umlal	r9, r10, r8, r8			@ d' += a[8] * a[8]

	bic	r0, r5, field_not_M 		@ u5 = d & M
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u5 * R0
	umlal   r3, r4, r0, r14
	bic	r14, r3, field_not_M 		@ t5 = c & M
	str	r14, [sp, #4 + 5*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u5 * R1
	umlal   r3, r4, r0, r14

	/* H */
	adds	r3, r3, r11			@ c += c'
	adc	r4, r4, r12
	adds	r5, r5, r9			@ d += d'
	adc	r6, r6, r10

	bic	r0, r5, field_not_M 		@ u6 = d & M
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u6 * R0
	umlal   r3, r4, r0, r14
	bic	r14, r3, field_not_M 		@ t6 = c & M
	str	r14, [sp, #4 + 6*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u6 * R1
	umlal   r3, r4, r0, r14

	/* I interleaved with J */
	ldr	r7, [r1, #0*4]			@ a[0]*2
	ldr	r0, [r1, #1*4]			@ a[1]*2
	mov	r7, r7, asl #1
	ldr	r8, [r1, #7*4]			@ a[7]
	ldr	r2, [r1, #8*4]			@ a[8]
	umlal	r3, r4, r7, r8			@ c += a[0]*2 * a[7]
	ldr	r14, [r1, #6*4]			@ a[6]
	mov	r0, r0, asl #1
	umull	r11, r12, r7, r2		@ c' = a[0]*2 * a[8]
	ldr	r7, [r1, #2*4]			@ a[2]*2
	umlal	r11, r12, r0, r8		@ c' += a[1]*2 * a[7]
	ldr	r8, [r1, #5*4]			@ a[5]
	umlal	r3, r4, r0, r14			@ c += a[1]*2 * a[6]
	ldr	r0, [r1, #3*4]			@ a[3]*2
	mov	r7, r7, asl #1
	umlal	r11, r12, r7, r14		@ c' += a[2]*2 * a[6]
	ldr	r14, [r1, #4*4]			@ a[4]
	mov	r0, r0, asl #1
	umlal	r3, r4, r7, r8			@ c += a[2]*2 * a[5]
	mov	r2, r2, asl #1			@ a[8]*2
	umlal	r11, r12, r0, r8		@ c' += a[3]*2 * a[5]
	umlal	r3, r4, r0, r14			@ c += a[3]*2 * a[4]
	umlal	r11, r12, r14, r14		@ c' += a[4] * a[4]
	ldr	r8, [r1, #9*4]			@ a[9]
	umlal	r5, r6, r2, r8			@ d += a[8]*2 * a[9]
	@ r8 will be used in J

	bic	r0, r5, field_not_M 		@ u7 = d & M
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u7 * R0
	umlal   r3, r4, r0, r14
	bic	r14, r3, field_not_M 		@ t7 = c & M
	str	r14, [sp, #4 + 7*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u7 * R1
	umlal   r3, r4, r0, r14

	/* J */
	adds	r3, r3, r11			@ c += c'
	adc	r4, r4, r12
	umlal	r5, r6, r8, r8			@ d += a[9] * a[9]

	bic	r0, r5, field_not_M 		@ u8 = d & M
	str	r0, [sp, #4 + 8*4]
	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	movw    r14, field_R0			@ c += u8 * R0
	umlal   r3, r4, r0, r14

	/******************************************
	 * compute and write back result
	 ******************************************
	Allocation:
	    r0    r
	    r3:r4 c
	    r5:r6 d
	    r7    t0
	    r8    t1
	    r9    t2
	    r11   u8
	    r12   t9
	    r1,r2,r10,r14 scratch

	Note: do not read from a[] after here, it may overlap with r[]
	*/
	ldr	r0, [sp, #0]
	add	r1, sp, #4 + 3*4		@ r[3..7] = t3..7, r11=u8, r12=t9
	ldmia	r1, {r2,r7,r8,r9,r10,r11,r12}
	add	r1, r0, #3*4
	stmia	r1, {r2,r7,r8,r9,r10}

	bic	r2, r3, field_not_M 		@ r[8] = c & M
	str	r2, [r0, #8*4]
	mov	r3, r3, lsr #26     		@ c >>= 26
	orr	r3, r3, r4, asl #6
	mov     r4, r4, lsr #26
	mov     r14, field_R1			@ c += u8 * R1
	umlal   r3, r4, r11, r14
	movw    r14, field_R0			@ c += d * R0
	umlal   r3, r4, r5, r14
	adds	r3, r3, r12			@ c += t9
	adc	r4, r4, #0

	add	r1, sp, #4 + 0*4		@ r7,r8,r9 = t0,t1,t2
	ldmia	r1, {r7,r8,r9}

	ubfx	r2, r3, #0, #22     		@ r[9] = c & (M >> 4)
	str	r2, [r0, #9*4]
	mov	r3, r3, lsr #22     		@ c >>= 22
	orr	r3, r3, r4, asl #10
	mov     r4, r4, lsr #22
	movw    r14, field_R1 << 4   		@ c += d * (R1 << 4)
	umlal   r3, r4, r5, r14

	movw    r14, field_R0 >> 4   		@ d = c * (R0 >> 4) + t0 (64x64 multiply+add)
	umull	r5, r6, r3, r14			@ d = c.lo * (R0 >> 4)
	adds	r5, r5, r7	    		@ d.lo += t0
	mla	r6, r14, r4, r6			@ d.hi += c.hi * (R0 >> 4)
	adc	r6, r6, 0	     		@ d.hi += carry

	bic	r2, r5, field_not_M 		@ r[0] = d & M
	str	r2, [r0, #0*4]

	mov	r5, r5, lsr #26     		@ d >>= 26
	orr	r5, r5, r6, asl #6
	mov     r6, r6, lsr #26
	
	movw    r14, field_R1 >> 4   		@ d += c * (R1 >> 4) + t1 (64x64 multiply+add)
	umull	r1, r2, r3, r14       		@ tmp = c.lo * (R1 >> 4)
	adds	r5, r5, r8	    		@ d.lo += t1
	adc	r6, r6, #0	    		@ d.hi += carry
	adds	r5, r5, r1	    		@ d.lo += tmp.lo
	mla	r2, r14, r4, r2      		@ tmp.hi += c.hi * (R1 >> 4)
	adc	r6, r6, r2	   		@ d.hi += carry + tmp.hi

	bic	r2, r5, field_not_M 		@ r[1] = d & M
	str	r2, [r0, #1*4]
	mov	r5, r5, lsr #26     		@ d >>= 26 (ignore hi)
	orr	r5, r5, r6, asl #6

	add	r5, r5, r9	  		@ d += t2
	str	r5, [r0, #2*4]      		@ r[2] = d

	add	sp, sp, #48
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, pc}
	.size	secp256k1_fe_sqr_inner, .-secp256k1_fe_sqr_inner

