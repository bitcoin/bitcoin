/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2015 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or modify it under the
 * terms of the version 2.1 (or later) of the GNU Lesser General Public License
 * as published by the Free Software Foundation; or version 2.0 of the Apache
 * License as published by the Apache Software Foundation. See the LICENSE files
 * for more details.
 *
 * RELIC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the LICENSE files for more details.
 *
 * You should have received a copy of the GNU Lesser General Public or the
 * Apache License along with RELIC. If not, see <https://www.gnu.org/licenses/>
 * or <https://www.apache.org/licenses/>.
 */

#include "relic_dv_low.h"

#include "macro.s"

.global cdecl(fp2_sqrn_low)
.global cdecl(fp2_sqrm_low)

/*
 * Function: fp2_sqrm_low
 * Inputs: rdi = c, rsi = a
 * Output: c = a * a
 */
cdecl(fp2_sqrm_low):
	push %r12
	push %r13
	push %r14
	push %r15

	subq $96, %rsp

	/* rsp[8..15] = t0 = a0 + a1. */
	movq	0(%rsi), %r8
	addq	32(%rsi), %r8
	movq	%r8, 64(%rsp)
	movq	8(%rsi), %r9
	adcq	40(%rsi), %r9
	movq	%r9, 72(%rsp)
	movq	16(%rsi), %r10
	adcq	48(%rsi), %r10
	movq	%r10, 80(%rsp)
	movq	24(%rsi), %r11
	adcq	56(%rsi), %r11
	movq	%r11, 88(%rsp)

	/* (r15, r14, r3, r12) = t1 = a0 - a1. */
	movq	0(%rsi), %r12
	subq	32(%rsi), %r12
	movq	$0, %rax            ////////try it on ib (w/o interleaving)
	movq	8(%rsi), %r13
	sbbq	40(%rsi), %r13
	movq	$0, %rcx
	movq	16(%rsi), %r14
	sbbq	48(%rsi), %r14
	movq	$0, %rdx
	movq	24(%rsi), %r15
	sbbq	56(%rsi), %r15
	movq	$0, %r11

	movq	P0,%r8
	cmovc	%r8, %rax
	movq	P1,%r9
	cmovc	%r9, %rcx
	movq	P2,%r8
	cmovc	%r8, %rdx
	movq	P3,%r9
	cmovc	%r9, %r11

	addq	%rax,%r12
	adcq	%rcx,%r13
	adcq	%rdx,%r14
	adcq	%r11,%r15

	/* c0 = a0^2 + b_0^2 * u^2. */
	FP_MULN_LOW %rsp, %r8, %r9, %r10, 64(%rsp), 72(%rsp), 80(%rsp), 88(%rsp), %r12, %r13, %r14, %r15

	/* (r11, r10, r9, r8) = t2 = 2 * a0. */
	movq	0(%rsi), %r8
	addq	%r8, %r8
	movq	%r8, 64(%rsp)
	movq	8(%rsi), %r9
	adcq	%r9, %r9
	movq	%r9, 72(%rsp)
	movq	16(%rsi), %r10
	adcq	%r10, %r10
	movq	%r10, 80(%rsp)
	movq	24(%rsi), %r11
	adcq	%r11, %r11
	movq	%r11, 88(%rsp)

	FP_RDCN_LOW %rdi, %rsp

	FP_MULN_LOW %rsp, %r12, %r13, %r14, 64(%rsp), 72(%rsp), 80(%rsp), 88(%rsp), 32(%rsi), 40(%rsi), 48(%rsi), 56(%rsi)
	addq	$(8 * RLC_FP_DIGS), %rdi
	FP_RDCN_LOW %rdi, %rsp

	addq $96, %rsp

	pop %r15
	pop %r14
	pop %r13
	pop %r12
	ret

cdecl(fp2_sqrn_low):
	push %r12
	push %r13
	push %r14
	push %r15

	subq $96, %rsp

	/* rsp[8..15] = t0 = a0 + a1. */
	movq	0(%rsi), %r8
	movq	%r8, %r12
	addq	32(%rsi), %r8
	movq	%r8, 64(%rsp)
	movq	8(%rsi), %r9
	movq	%r9, %r13
	adcq	40(%rsi), %r9
	movq	%r9, 72(%rsp)
	movq	16(%rsi), %r10
	movq	%r10, %r14
	adcq	48(%rsi), %r10
	movq	%r10, 80(%rsp)
	movq	24(%rsi), %r11
	movq	%r11, %r15
	adcq	56(%rsi), %r11
	movq	%r11, 88(%rsp)

	/* (r15, r14, r3, r12) = t1 = a0 - a1. */
	subq	32(%rsi), %r12
	sbbq	40(%rsi), %r13
	sbbq	48(%rsi), %r14
	sbbq	56(%rsi), %r15

	movq	$0, %rax
	movq	$0, %rcx
	movq	$0, %rdx
	movq	$0, %r11
	movq	P0,%r8
	cmovc	%r8, %rax
	movq	P1,%r9
	cmovc	%r9, %rcx
	movq	P2,%r8
	cmovc	%r8, %rdx
	movq	P3,%r9
	cmovc	%r9, %r11

	addq	%rax,%r12
	adcq	%rcx,%r13
	adcq	%rdx,%r14
	adcq	%r11,%r15

	/* c0 = a0^2 + b_0^2 * u^2. */
	FP_MULN_LOW %rdi, %r8, %r9, %r10, 64(%rsp), 72(%rsp), 80(%rsp), 88(%rsp), %r12, %r13, %r14, %r15

	/* (r11, r10, r9, r8) = t2 = 2 * a0. */
	movq	0(%rsi), %r8
	addq	%r8, %r8
	movq	%r8, 64(%rsp)
	movq	8(%rsi), %r9
	adcq	%r9, %r9
	movq	%r9, 72(%rsp)
	movq	16(%rsi), %r10
	adcq	%r10, %r10
	movq	%r10, 80(%rsp)
	movq	24(%rsi), %r11
	adcq	%r11, %r11
	movq	%r11, 88(%rsp)

	addq	$(8*RLC_DV_DIGS), %rdi
	FP_MULN_LOW %rdi, %r12, %r13, %r14, 64(%rsp), 72(%rsp), 80(%rsp), 88(%rsp), 32(%rsi), 40(%rsi), 48(%rsi), 56(%rsi)

	addq $96, %rsp

	pop %r15
	pop %r14
	pop %r13
	pop %r12
	ret
