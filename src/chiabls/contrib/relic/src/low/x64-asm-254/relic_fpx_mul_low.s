/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2017 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * RELIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RELIC. If not, see <http://www.gnu.org/licenses/>.
 */

#include "relic_dv_low.h"

#include "macro.s"

.global cdecl(fp2_mulm_low)
.global cdecl(fp2_muln_low)
.global cdecl(fp2_mulc_low)

/*
 * Function: fp2_mulm_low
 * Inputs: rdi = c, rsi = a, rcx = b
 * Output: c = a * b
 */
cdecl(fp2_mulm_low):
	push %r12
	push %r13
	push %r14
	push %r15
	push %rbx
	push %rbp

	subq $256, %rsp
	movq %rdx, %rcx

	/* rsp[0..7] = t0 = a0 * b0. */
	FP_MULN_LOW %rsp, %r8, %r9, %r10, 0(%rsi), 8(%rsi), 16(%rsi), 24(%rsi), 0(%rcx), 8(%rcx), 16(%rcx), 24(%rcx)

	addq $64, %rsp
	/* rsp[8..15] = t4 = a1 * b1. */
	FP_MULN_LOW %rsp, %r8, %r9, %r10, 32(%rsi), 40(%rsi), 48(%rsi), 56(%rsi), 32(%rcx), 40(%rcx), 48(%rcx), 56(%rcx)

	/* t2 = (r11, r10, r9, r8) = a0 + a1. */
	movq	0(%rsi), %r8
	addq	32(%rsi), %r8
	movq	8(%rsi), %r9
	adcq	40(%rsi), %r9
	movq	16(%rsi), %r10
	adcq	48(%rsi), %r10
	movq	24(%rsi), %r11
	adcq	56(%rsi), %r11

	/* t1 = (r15, r14, r3, r12) = b0 + b1. */
	movq	0(%rcx), %r12
	addq	32(%rcx), %r12
	movq	8(%rcx), %r13
	adcq	40(%rcx), %r13
	movq	16(%rcx), %r14
	adcq	48(%rcx), %r14
	movq	24(%rcx), %r15
	adcq	56(%rcx), %r15

	addq $64, %rsp
	/* rsp[16..23] = t3 = (a0 + a1) * (b0 + b1). */
	FP_MULN_LOW %rsp, %rbx, %rbp, %rcx, %r8, %r9, %r10, %r11, %r12, %r13, %r14, %r15
	subq $128, %rsp

	/* rsp[24..31] = t2 = t0 + t4 = (a0 * b0) + (a1 * b1). */
	xorq	%rdx, %rdx
	xorq	%rsi, %rsi
	movq	0(%rsp), %r8
	addq	64(%rsp), %r8
	movq	%r8, 192(%rsp)
	.irp i, 8, 16, 24, 32, 40, 48, 56
		movq	\i(%rsp), %r8
		adcq	(64+\i)(%rsp), %r8
		movq	%r8, (192+\i)(%rsp)
	.endr
	xorq	%rax,%rax
	xorq	%rcx,%rcx
	movq	0(%rsp), %r8
	subq	64(%rsp), %r8
	movq	%r8, 0(%rsp)
	.irp i, 8, 16, 24
		movq	\i(%rsp), %r8
		sbbq	(64+\i)(%rsp), %r8
		movq	%r8, \i(%rsp)
	.endr
	movq    32(%rsp), %r8
	sbbq    96(%rsp), %r8
	movq    40(%rsp), %r9
	sbbq    104(%rsp), %r9
	movq    48(%rsp), %r10
	sbbq    112(%rsp), %r10
	movq    56(%rsp), %r11
	sbbq    120(%rsp), %r11

	movq	P0, %r12
	cmovc 	%r12, %rax
	movq	P1, %r12
	cmovc 	%r12, %rcx
	movq	P2, %r12
	cmovc 	%r12, %rdx
	movq	P3, %r12
	cmovc 	%r12, %rsi
    addq	%rax, %r8
	movq	%r8, 32(%rsp)
    adcq	%rcx, %r9
	movq	%r9, 40(%rsp)
    adcq	%rdx, %r10
	movq	%r10, 48(%rsp)
    adcq	%rsi, %r11
	movq	%r11, 56(%rsp)

	FP_RDCN_LOW %rdi, %rsp
	/* rsp[0..8] = t4 = t3 - t2. */
	movq	128(%rsp), %r8
	subq	192(%rsp), %r8
	movq	%r8, 0(%rsp)
	.irp i, 8, 16, 24, 32, 40, 48, 56
		movq	(128+\i)(%rsp), %r8
		sbbq	(192+\i)(%rsp), %r8
		movq	%r8, \i(%rsp)
	.endr

	addq $(FP_DIGS * 8), %rdi
	FP_RDCN_LOW %rdi, %rsp

	addq $256, %rsp
	pop %rbp
	pop %rbx
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	ret

/*
 * Function: fp2_muln_low
 * Inputs: rdi = c, rsi = a, rcx = b
 * Output: c = a * b
 */
cdecl(fp2_muln_low):
	push %r12
	push %r13
	push %r14
	push %r15
	push %rbx
	push %rbp

	subq $256, %rsp
	movq %rdx, %rcx

	/* rsp[0..7] = t0 = a0 * b0. */
	FP_MULN_LOW %rsp, %r8, %r9, %r10, 0(%rsi), 8(%rsi), 16(%rsi), 24(%rsi), 0(%rcx), 8(%rcx), 16(%rcx), 24(%rcx)

	addq $64, %rsp
	/* rsp[8..15] = t4 = a1 * b1. */
	FP_MULN_LOW %rsp, %r8, %r9, %r10, 32(%rsi), 40(%rsi), 48(%rsi), 56(%rsi), 32(%rcx), 40(%rcx), 48(%rcx), 56(%rcx)

	/* t2 = (r11, r10, r9, r8) = a0 + a1. */
	movq	0(%rsi), %r8
	addq	32(%rsi), %r8
	movq	8(%rsi), %r9
	adcq	40(%rsi), %r9
	movq	16(%rsi), %r10
	adcq	48(%rsi), %r10
	movq	24(%rsi), %r11
	adcq	56(%rsi), %r11

	/* t1 = (r15, r14, r3, r12) = b0 + b1. */
	movq	0(%rcx), %r12
	addq	32(%rcx), %r12
	movq	8(%rcx), %r13
	adcq	40(%rcx), %r13
	movq	16(%rcx), %r14
	adcq	48(%rcx), %r14
	movq	24(%rcx), %r15
	adcq	56(%rcx), %r15

	addq $64, %rsp
	/* rsp[16..23] = t3 = (a0 + a1) * (b0 + b1). */
	FP_MULN_LOW %rsp, %rbx, %rbp, %rcx, %r8, %r9, %r10, %r11, %r12, %r13, %r14, %r15
	subq $128, %rsp

	/* rsp[24..31] = t2 = t0 + t4 = (a0 * b0) + (a1 * b1) */
	xorq	%rdx, %rdx
	xorq	%rsi, %rsi
	movq	0(%rsp), %r8
	addq	64(%rsp), %r8
	movq	8(%rsp), %r9
	adcq	72(%rsp), %r9
	movq	16(%rsp), %r10
	adcq	80(%rsp), %r10
	movq	24(%rsp), %r11
	adcq	88(%rsp), %r11
	movq	32(%rsp), %r12
	adcq	96(%rsp), %r12
	movq	40(%rsp), %r13
	adcq	104(%rsp), %r13
	movq	48(%rsp), %r14
	adcq	112(%rsp), %r14
	movq	56(%rsp), %r15
	adcq	120(%rsp), %r15

	/* rsp[0..8] = t4 = t3 - t2 */
	movq	128(%rsp), %rcx
	subq	%r8, %rcx
	movq	%rcx, 8*DV_DIGS(%rdi)
	movq	136(%rsp), %rcx
	sbbq	%r9, %rcx
	movq	%rcx, 8*DV_DIGS+8(%rdi)
	movq	144(%rsp), %rcx
	sbbq	%r10, %rcx
	movq	%rcx, 8*DV_DIGS+16(%rdi)
	movq	152(%rsp), %rcx
	sbbq	%r11, %rcx
	movq	%rcx, 8*DV_DIGS+24(%rdi)
	movq	160(%rsp), %rcx
	sbbq	%r12, %rcx
	movq	%rcx, 8*DV_DIGS+32(%rdi)
	movq	168(%rsp), %rcx
	sbbq	%r13, %rcx
	movq	%rcx, 8*DV_DIGS+40(%rdi)
	movq	176(%rsp), %rcx
	sbbq	%r14, %rcx
	movq	%rcx, 8*DV_DIGS+48(%rdi)
	movq	184(%rsp), %rcx
	sbbq	%r15, %rcx
	movq	%rcx, 8*DV_DIGS+56(%rdi)
	xorq	%rax,%rax
	xorq	%rcx,%rcx
	movq	0(%rsp), %r8
	subq	64(%rsp), %r8
	movq	%r8, 0(%rdi)
	.irp i, 8, 16, 24
		movq	\i(%rsp), %r8
		sbbq	(64+\i)(%rsp), %r8
		movq	%r8, \i(%rdi)
	.endr
	movq    32(%rsp), %r8
	sbbq    96(%rsp), %r8
	movq    40(%rsp), %r9
	sbbq    104(%rsp), %r9
	movq    48(%rsp), %r10
	sbbq    112(%rsp), %r10
	movq    56(%rsp), %r11
	sbbq    120(%rsp), %r11

	movq	P0, %r12
	cmovc 	%r12, %rax
	movq	P1, %r12
	cmovc 	%r12, %rcx
	movq	P2, %r12
	cmovc 	%r12, %rdx
	movq	P3, %r12
	cmovc 	%r12, %rsi
    addq	%rax, %r8
	movq	%r8, 32(%rdi)
    adcq	%rcx, %r9
	movq	%r9, 40(%rdi)
    adcq	%rdx, %r10
	movq	%r10, 48(%rdi)
    adcq	%rsi, %r11
	movq	%r11, 56(%rdi)

	addq $256, %rsp
	pop %rbp
	pop %rbx
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	ret

cdecl(fp2_mulc_low):
	push %r12
	push %r13
	push %r14
	push %r15
	push %rbx
	push %rbp

	subq $256, %rsp
	movq %rdx, %rcx

	/* rsp[0..7] = t0 = a0 * b0. */
	FP_MULN_LOW %rsp, %r8, %r9, %r10, 0(%rsi), 8(%rsi), 16(%rsi), 24(%rsi), 0(%rcx), 8(%rcx), 16(%rcx), 24(%rcx)

	addq $64, %rsp
	/* rsp[8..15] = t4 = a1 * b1 */
	FP_MULN_LOW %rsp, %r8, %r9, %r10, 32(%rsi), 40(%rsi), 48(%rsi), 56(%rsi), 32(%rcx), 40(%rcx), 48(%rcx), 56(%rcx)

	/* t2 = (r11, r10, r9, r8) = a0 + a1 */
	movq	0(%rsi), %r8
	addq	32(%rsi), %r8
	movq	8(%rsi), %r9
	adcq	40(%rsi), %r9
	movq	16(%rsi), %r10
	adcq	48(%rsi), %r10
	movq	24(%rsi), %r11
	adcq	56(%rsi), %r11

	// t1 = (r15, r14, r3, r12) = b0 + b1
	movq	0(%rcx), %r12
	addq	32(%rcx), %r12
	movq	8(%rcx), %r13
	adcq	40(%rcx), %r13
	movq	16(%rcx), %r14
	adcq	48(%rcx), %r14
	movq	24(%rcx), %r15
	adcq	56(%rcx), %r15

	addq $64, %rsp
	/* rsp[16..23] = t3 = (a0 + a1) * (b0 + b1) */
	FP_MULN_LOW %rsp, %rbx, %rbp, %rcx, %r8, %r9, %r10, %r11, %r12, %r13, %r14, %r15
	subq $128, %rsp

	movq	0(%rsp), %r8
	addq	64(%rsp), %r8
	movq	8(%rsp), %r9
	adcq	72(%rsp), %r9
	movq	16(%rsp), %r10
	adcq	80(%rsp), %r10
	movq	24(%rsp), %r11
	adcq	88(%rsp), %r11
	movq	32(%rsp), %r12
	adcq	96(%rsp), %r12
	movq	40(%rsp), %r13
	adcq	104(%rsp), %r13
	movq	48(%rsp), %r14
	adcq	112(%rsp), %r14
	movq	56(%rsp), %r15
	adcq	120(%rsp), %r15

	/* rsp[0..8] = t4 = t3 - t2 */
	movq	128(%rsp), %rcx
	subq	%r8, %rcx
	movq	%rcx, 8*DV_DIGS(%rdi)
	movq	136(%rsp), %rcx
	sbbq	%r9, %rcx
	movq	%rcx, 8*DV_DIGS+8(%rdi)
	movq	144(%rsp), %rcx
	sbbq	%r10, %rcx
	movq	%rcx, 8*DV_DIGS+16(%rdi)
	movq	152(%rsp), %rcx
	sbbq	%r11, %rcx
	movq	%rcx, 8*DV_DIGS+24(%rdi)
	movq	160(%rsp), %rcx
	sbbq	%r12, %rcx
	movq	%rcx, 8*DV_DIGS+32(%rdi)
	movq	168(%rsp), %rcx
	sbbq	%r13, %rcx
	movq	%rcx, 8*DV_DIGS+40(%rdi)
	movq	176(%rsp), %rcx
	sbbq	%r14, %rcx
	movq	%rcx, 8*DV_DIGS+48(%rdi)
	movq	184(%rsp), %rcx
	sbbq	%r15, %rcx
	movq	%rcx, 8*DV_DIGS+56(%rdi)

	/* rsp[0..8] = t1 = (a0 * a1) + u^2 * (a1 * b1) */
	movq    NP40, %rax
	movq    NP41, %r8
	movq    NP42, %r9
	movq    NP43, %r10
	movq    NP44, %r11
	movq    24(%rsp), %rcx
	addq    %rcx, %rax
	movq    32(%rsp), %rcx
	adcq    %rcx, %r8
	movq    40(%rsp), %rcx
	adcq    %rcx, %r9
	movq    48(%rsp), %rcx
	adcq    %rcx, %r10
	movq    56(%rsp), %rcx
	adcq    %rcx, %r11

	movq    0(%rsp), %rcx
	subq    64(%rsp), %rcx
	movq    %rcx, 0(%rdi)
	movq    8(%rsp), %rcx
	sbbq    72(%rsp), %rcx
	movq    %rcx, 8(%rdi)
	movq    16(%rsp), %rcx
	sbbq    80(%rsp), %rcx
   	movq    %rcx, 16(%rdi)
	sbbq    88(%rsp), %rax
	movq    %rax, 24(%rdi)
	sbbq    96(%rsp), %r8
	movq    %r8, 32(%rdi)
	sbbq    104(%rsp), %r9
	movq    %r9, 40(%rdi)
	sbbq    112(%rsp), %r10
	movq    %r10, 48(%rdi)
	sbbq    120(%rsp), %r11
	movq    %r11, 56(%rdi)

	addq $256, %rsp
	pop %rbp
	pop %rbx
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	ret
