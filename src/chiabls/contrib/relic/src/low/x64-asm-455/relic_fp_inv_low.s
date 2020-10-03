/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2012 RELIC Authors
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

/**
 * @file
 *
 * Implementation of the low-le&vel in&version functions.
 *
 * @&version $Id: relic_fp_inv_low.c 401 2010-07-05 01:40:18Z dfaranha $
 * @ingroup fp
 */

#include "macro.s"


/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

/* No carry */
.macro _DBL R, i
	movq	\i+0(\R), %rdx
	addq	%rdx, \i+0(\R)
	movq	\i+8(\R), %rdx
	adcq	%rdx, \i+8(\R)
	movq	\i+16(\R), %rdx
	adcq	%rdx, \i+16(\R)
	movq	\i+24(\R), %rdx
	adcq	%rdx, \i+24(\R)
	movq	\i+32(\R), %rdx
	adcq	%rdx, \i+32(\R)
	movq	\i+40(\R), %rdx
	adcq	%rdx, \i+40(\R)
	movq	\i+48(\R), %rdx
	adcq	%rdx, \i+48(\R)
	movq	\i+56(\R), %rdx
	adcq	%rdx, \i+56(\R)
.endm

.global fp_invn_asm

/**
 * rdi = x1, rsi = a
 */
fp_invn_asm:
	push	%r12
	push	%r13
	push	%r14
	push	%r15
	push	%rbp
	push	%rbx
	subq 	$256, %rsp

	/* v = p. */
	movq	P0, %r8
	movq	P1, %r9
	movq	P2, %r10
	movq	P3, %r11
	movq	P4, %r12
	movq	P5, %r13
	movq	P6, %r14
	movq	P7, %r15

	/* rsp[0..7] = u = a. */
	movq	0(%rsi),%rax
	movq	%rax,0(%rsp)
	movq	8(%rsi),%rax
	movq	%rax,8(%rsp)
	movq	16(%rsi),%rax
	movq	%rax,16(%rsp)
	movq	24(%rsi),%rax
	movq	%rax,24(%rsp)
	movq	32(%rsi),%rax
	movq	%rax,32(%rsp)
	movq	40(%rsi),%rax
	movq	%rax,40(%rsp)
	movq	48(%rsi),%rax
	movq	%rax,48(%rsp)
	movq	56(%rsi),%rax
	movq	%rax,56(%rsp)
	xorq	%rax, %rax

	/* rsp[7..15] = x1 = 1. */
	movq	$1,64(%rsp)
	movq	$0,72(%rsp)
	movq	$0,80(%rsp)
	movq	$0,88(%rsp)
	movq	$0,96(%rsp)
	movq	$0,104(%rsp)
	movq	$0,112(%rsp)
	movq	$0,120(%rsp)

	/* rsp[16..23] = x2 = 0. */
	movq	$0,128(%rsp)
	movq	$0,136(%rsp)
	movq	$0,144(%rsp)
	movq	$0,152(%rsp)
	movq	$0,160(%rsp)
	movq	$0,168(%rsp)
	movq	$0,176(%rsp)
	movq	$0,184(%rsp)

loop:
	movq	%r8,%rdx
	orq		%r9,%rdx
	orq		%r10,%rdx
	orq		%r11,%rdx
	orq		%r12,%rdx
	orq		%r13,%rdx
	orq		%r14,%rdx
	orq		%r15,%rdx
	jz		end

	inc		%rax
	test	$1, %r8
	jnz 	v_odd

	/* fp_rsh1_low(v). */
	shrd    $1, %r9, %r8
	shrd    $1, %r10, %r9
	shrd    $1, %r11, %r10
	shrd    $1, %r12, %r11
	shrd    $1, %r13, %r12
	shrd    $1, %r14, %r13
	shrd    $1, %r15, %r14
	shr		$1, %r15

	/* fp_dbln_low(x1). */
	_DBL 	%rsp, 64
	jmp 	loop
v_odd:
	movq	0(%rsp),%rdx
	test	$1, %rdx
	jnz 	u_odd

	/* fp_rsh1_low(u). */
	movq	8(%rsp), %rdx
	shrd    $1, %rdx, 0(%rsp)
	movq	16(%rsp), %rdx
	shrd    $1, %rdx, 8(%rsp)
	movq	24(%rsp), %rdx
	shrd    $1, %rdx, 16(%rsp)
	movq	32(%rsp), %rdx
	shrd    $1, %rdx, 24(%rsp)
	movq	40(%rsp), %rdx
	shrd    $1, %rdx, 32(%rsp)
	movq	48(%rsp), %rdx
	shrd    $1, %rdx, 40(%rsp)
	movq	56(%rsp), %rdx
	shrd    $1, %rdx, 48(%rsp)
	shr		$1, %rdx
	movq	%rdx, 56(%rsp)

	/* fp_dbln_low(x2). */
	_DBL	%rsp, 128
	jmp 	loop
u_odd:
	subq	0(%rsp), %r8
	sbbq	8(%rsp), %r9
	sbbq	16(%rsp), %r10
	sbbq	24(%rsp), %r11
	sbbq	32(%rsp), %r12
	sbbq	40(%rsp), %r13
	sbbq	48(%rsp), %r14
	sbbq	56(%rsp), %r15
	jc 		cmp_lt

	/* fp_rsh1_low(v). */
	shrd    $1, %r9, %r8
	shrd    $1, %r10, %r9
	shrd    $1, %r11, %r10
	shrd    $1, %r12, %r11
	shrd    $1, %r13, %r12
	shrd    $1, %r14, %r13
	shrd    $1, %r15, %r14
	shr		$1, %r15

	movq	64(%rsp), %rdx
	addq	%rdx, 128(%rsp)
	movq	72(%rsp), %rdx
	adcq	%rdx, 136(%rsp)
	movq	80(%rsp), %rdx
	adcq	%rdx, 144(%rsp)
	movq	88(%rsp), %rdx
	adcq	%rdx, 152(%rsp)
	movq	96(%rsp), %rdx
	adcq	%rdx, 160(%rsp)
	movq	104(%rsp), %rdx
	adcq	%rdx, 168(%rsp)
	movq	112(%rsp), %rdx
	adcq	%rdx, 176(%rsp)
	movq	120(%rsp), %rdx
	adcq	%rdx, 184(%rsp)

	_DBL 	%rsp, 64
	jmp 	loop
cmp_lt:
	addq	0(%rsp), %r8
	adcq	8(%rsp), %r9
	adcq	16(%rsp), %r10
	adcq	24(%rsp), %r11
	adcq	32(%rsp), %r12
	adcq	40(%rsp), %r13
	adcq	48(%rsp), %r14
	adcq	56(%rsp), %r15

	subq	%r8, 0(%rsp)
	sbbq	%r9, 8(%rsp)
	sbbq	%r10, 16(%rsp)
	sbbq	%r11, 24(%rsp)
	sbbq	%r12, 32(%rsp)
	sbbq	%r13, 40(%rsp)
	sbbq	%r14, 48(%rsp)
	sbbq	%r15, 56(%rsp)

	movq	8(%rsp), %rdx
	shrd    $1, %rdx, 0(%rsp)
	movq	16(%rsp), %rdx
	shrd    $1, %rdx, 8(%rsp)
	movq	24(%rsp), %rdx
	shrd    $1, %rdx, 16(%rsp)
	movq	32(%rsp), %rdx
	shrd    $1, %rdx, 24(%rsp)
	movq	40(%rsp), %rdx
	shrd    $1, %rdx, 32(%rsp)
	movq	48(%rsp), %rdx
	shrd    $1, %rdx, 40(%rsp)
	movq	56(%rsp), %rdx
	shrd    $1, %rdx, 48(%rsp)
	shr		$1, %rdx
	movq	%rdx, 56(%rsp)

	movq	128(%rsp), %rdx
	addq	%rdx, 64(%rsp)
	movq	136(%rsp), %rdx
	adcq	%rdx, 72(%rsp)
	movq	144(%rsp), %rdx
	adcq	%rdx, 80(%rsp)
	movq	152(%rsp), %rdx
	adcq	%rdx, 88(%rsp)
	movq	160(%rsp), %rdx
	adcq	%rdx, 96(%rsp)
	movq	168(%rsp), %rdx
	adcq	%rdx, 104(%rsp)
	movq	176(%rsp), %rdx
	adcq	%rdx, 112(%rsp)
	movq	184(%rsp), %rdx
	adcq	%rdx, 120(%rsp)

	_DBL 	%rsp, 128
	jmp 	loop
end:
	movq	64(%rsp), %r9
	movq	%r9, 0(%rdi)
	movq	72(%rsp), %r9
	movq	%r9, 8(%rdi)
	movq	80(%rsp), %r9
	movq	%r9, 16(%rdi)
	movq	88(%rsp), %r9
	movq	%r9, 24(%rdi)
	movq	96(%rsp), %r9
	movq	%r9, 32(%rdi)
	movq	104(%rsp), %r9
	movq	%r9, 40(%rdi)
	movq	112(%rsp), %r9
	movq	%r9, 48(%rdi)
	movq	120(%rsp), %r9
	movq	%r9, 56(%rdi)
exit:
	addq 	$256, %rsp
	pop		%rbx
	pop		%rbp
	pop		%r15
	pop		%r14
	pop		%r13
	pop		%r12
	ret
