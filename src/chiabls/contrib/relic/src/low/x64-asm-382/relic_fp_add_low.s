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

#include "relic_fp_low.h"

/**
 * @file
 *
 * Implementation of the low-level prime field addition and subtraction
 * functions.
 *
 * @version $Id: relic_fp_add_low.c 88 2009-09-06 21:27:19Z dfaranha $
 * @ingroup fp
 */

#include "macro.s"

.data

p0: .quad P0
p1: .quad P1
p2: .quad P2
p3: .quad P3
p4: .quad P4
p5: .quad P5

.text

.global fp_add1_low
.global fp_addn_low
.global fp_addm_low
.global fp_addd_low
.global fp_addc_low
.global fp_sub1_low
.global fp_subn_low
.global fp_subm_low
.global fp_subd_low
.global fp_subc_low
.global fp_negm_low
.global fp_dbln_low
.global fp_dblm_low
.global fp_hlvm_low
.global fp_hlvd_low

fp_add1_low:
	movq	0(%rsi), %r10
	addq	%rdx   , %r10
	movq	%r10   , 0(%rdi)

	ADD1 1 (FP_DIGS - 1)
	xorq	%rax, %rax

	ret

fp_addn_low:
	movq	0(%rdx), %r11
	addq	0(%rsi), %r11
	movq	%r11   , 0(%rdi)

	ADDN 	1 (FP_DIGS - 1)

	xorq	%rax, %rax

	ret

fp_addm_low:
	push	%rbx
	push	%rbp
	push	%r12
	push	%r13

	movq	 0(%rdx), %r8
	addq	 0(%rsi), %r8
	movq	 8(%rdx), %r9
	adcq	 8(%rsi), %r9
	movq	16(%rdx), %r10
	adcq	16(%rsi), %r10
	movq	24(%rdx), %r11
	adcq	24(%rsi), %r11
	movq	32(%rdx), %r12
	adcq	32(%rsi), %r12
	movq	40(%rdx), %r13
	adcq	40(%rsi), %r13

	movq 	%r8 , %rax
	movq 	%r9 , %rcx
	movq 	%r10, %rdx
	movq 	%r11, %rsi
	movq	%r12, %rbx
	movq	%r13, %rbp

	subq	p0, %rax
	sbbq	p1, %rcx
	sbbq	p2, %rdx
	sbbq	p3, %rsi
	sbbq	p4, %rbx
	sbbq	p5, %rbp

	cmovnc	%rax, %r8
	cmovnc	%rcx, %r9
	cmovnc	%rdx, %r10
	cmovnc	%rsi, %r11
	cmovnc	%rbx, %r12
	cmovnc	%rbp, %r13

	movq	%r8 ,  0(%rdi)
	movq	%r9 ,  8(%rdi)
	movq	%r10, 16(%rdi)
	movq	%r11, 24(%rdi)
	movq	%r12, 32(%rdi)
	movq	%r13, 40(%rdi)
	xorq	%rax, %rax

	pop		%r13
	pop		%r12
	pop		%rbp
	pop		%rbx
	ret

fp_addd_low:
	movq	0(%rdx), %r11
	addq	0(%rsi), %r11
	movq	%r11   , 0(%rdi)

	ADDN 	1 (2 * FP_DIGS - 1)

	ret

fp_addc_low:
	push	%rbx
	push	%rbp
	push	%r12
	push	%r13

	movq	0(%rsi), %r8
	addq	0(%rdx), %r8
	movq	%r8    , 0(%rdi)

	ADDN	1 (FP_DIGS - 1)

	movq     48(%rsi), %r8
	adcq     48(%rdx), %r8
	movq     56(%rsi), %r9
	adcq     56(%rdx), %r9
	movq     64(%rsi), %r10
	adcq     64(%rdx), %r10
	movq     72(%rsi), %r11
	adcq     72(%rdx), %r11
	movq     80(%rsi), %r12
	adcq     80(%rdx), %r12
	movq     88(%rsi), %r13
	adcq     88(%rdx), %r13

	movq 	%r8 , %rax
	movq 	%r9 , %rcx
	movq 	%r10, %rdx
	movq 	%r11, %rsi
	movq	%r12, %rbx
	movq	%r13, %rbp

	subq	p0, %rax
	sbbq	p1, %rcx
	sbbq	p2, %rdx
	sbbq	p3, %rsi
	sbbq	p4, %rbx
	sbbq	p5, %rbp

	cmovnc	%rax, %r8
	cmovnc	%rcx, %r9
	cmovnc	%rdx, %r10
	cmovnc	%rsi, %r11
	cmovnc	%rbx, %r12
	cmovnc	%rbp, %r13

	movq	%r8 , 48(%rdi)
	movq	%r9 , 56(%rdi)
	movq	%r10, 64(%rdi)
	movq	%r11, 72(%rdi)
	movq	%r12, 80(%rdi)
	movq	%r13, 88(%rdi)
	xorq	%rax, %rax

	pop	%r13
	pop	%r12
	pop	%rbp
	pop	%rbx
	ret

fp_sub1_low:
	movq	0(%rsi), %r10
	subq	%rdx   , %r10
	movq	%r10   , 0(%rdi)

	SUB1 	1 (FP_DIGS - 1)
	xorq	%rax, %rax

	ret

fp_subn_low:
	xorq	%rax   , %rax
	movq	0(%rsi), %r11
	subq	0(%rdx), %r11
	movq	%r11   , 0(%rdi)

	SUBN 1 (FP_DIGS - 1)

	adcq	$0, %rax

	ret

fp_subm_low:
	xorq	%rax, %rax
	xorq	%rcx, %rcx

	movq	0(%rsi), %r8
	subq	0(%rdx), %r8
	movq	%r8    , 0(%rdi)

	SUBN	1 (FP_DIGS - 1)

	movq	$0, %r8
	movq	$0, %r9
	movq	$0, %r10
	movq	$0, %r11

	cmovc	p0, %rax
	cmovc	p1, %rcx
	cmovc	p2, %r8
	cmovc	p3, %r9
	cmovc	p4, %r10
	cmovc	p5, %r11

	addq	%rax,  0(%rdi)
	adcq	%rcx,  8(%rdi)
	adcq	%r8,  16(%rdi)
	adcq	%r9,  24(%rdi)
	adcq	%r10, 32(%rdi)
	adcq	%r11, 40(%rdi)

	ret

fp_subd_low:
	movq	0(%rsi), %r8
	subq	0(%rdx), %r8
	movq	%r8, 0(%rdi)

	SUBN 	1 (2 * FP_DIGS - 1)

	ret

fp_subc_low:
	xorq    %rax,%rax
	xorq    %rcx,%rcx

	movq    0(%rsi), %r8
	subq    0(%rdx), %r8
	movq    %r8,     0(%rdi)

	SUBN 	1 (2 * FP_DIGS - 1)

	movq	$0, %r8
	movq	$0, %r9
	movq	$0, %r10
	movq	$0, %r11

	cmovc	p0, %rax
	cmovc	p1, %rcx
	cmovc	p2, %r8
	cmovc	p3, %r9
	cmovc	p4, %r10
	cmovc	p5, %r11

	addq	%rax, 48(%rdi)
	adcq	%rcx, 56(%rdi)
	adcq	%r8,  64(%rdi)
	adcq	%r9,  72(%rdi)
	adcq	%r10, 80(%rdi)
	adcq	%r11, 88(%rdi)

	ret

fp_negm_low:
	movq 	$P0      , %r8
	subq 	0(%rsi) , %r8
	movq 	%r8     , 0(%rdi)
	movq 	$P1      , %r8
	sbbq 	8(%rsi) , %r8
	movq 	%r8     , 8(%rdi)
	movq 	$P2      , %r8
	sbbq 	16(%rsi), %r8
	movq 	%r8     , 16(%rdi)
	movq 	$P3      , %r8
	sbbq 	24(%rsi), %r8
	movq 	%r8     , 24(%rdi)
	movq 	$P4      , %r8
	sbbq 	32(%rsi), %r8
	movq 	%r8     , 32(%rdi)
	movq 	$P5      , %r8
	sbbq 	40(%rsi), %r8
	movq 	%r8     , 40(%rdi)
  	ret

fp_dbln_low:
	movq	0(%rsi), %r8
	addq	%r8    , %r8
	movq	%r8    , 0(%rdi)

	DBLN 1 (FP_DIGS - 1)

	xorq	%rax,%rax
	ret

fp_dblm_low:
	push	%rbx
	push	%rbp
	push	%r12
	push	%r13

	xorq	%rax, %rax
	xorq	%rcx, %rcx
	xorq	%rdx, %rdx

	movq	0(%rsi) , %r8
	addq	%r8     , %r8
	movq	8(%rsi) , %r9
	adcq	%r9     , %r9
	movq	16(%rsi), %r10
	adcq	%r10    , %r10
	movq	24(%rsi), %r11
	adcq	%r11    , %r11
	movq	32(%rsi), %r12
	adcq	%r12    , %r12
	movq	40(%rsi), %r13
	adcq	%r13    , %r13

	movq 	%r8 , %rax
	movq 	%r9 , %rcx
	movq 	%r10, %rdx
	movq 	%r11, %rsi
	movq	%r12, %rbx
	movq	%r13, %rbp

	subq	p0, %rax
	sbbq	p1, %rcx
	sbbq	p2, %rdx
	sbbq	p3, %rsi
	sbbq	p4, %rbx
	sbbq	p5, %rbp

	cmovnc	%rax, %r8
	cmovnc	%rcx, %r9
	cmovnc	%rdx, %r10
	cmovnc	%rsi, %r11
	cmovnc	%rbx, %r12
	cmovnc	%rbp, %r13

	movq	%r8 ,  0(%rdi)
	movq	%r9 ,  8(%rdi)
	movq	%r10, 16(%rdi)
	movq	%r11, 24(%rdi)
	movq	%r12, 32(%rdi)
	movq	%r13, 40(%rdi)
	xorq	%rax, %rax

	pop	%r13
	pop	%r12
	pop	%rbp
	pop	%rbx
	ret

fp_hlvm_low:
	push	%r12
	push	%r13

	xorq	%rdx, %rdx

	movq	$P0, %r8
	movq	$P1, %r9
	movq	$P2, %r10
	movq	$P3, %r11
	movq	$P4, %r12
	movq	$P5, %r13

  	movq 	$1     ,%rax
  	movq 	0(%rsi),%rcx
  	andq 	%rcx   ,%rax

	cmovz	%rdx, %r8
	cmovz	%rdx, %r9
	cmovz	%rdx, %r10
	cmovz	%rdx, %r11
	cmovz	%rdx, %r12
	cmovz	%rdx, %r13

	addq	%rcx    , %r8
	movq	8(%rsi) , %rdx
	adcq	%rdx    , %r9
	movq	16(%rsi), %rdx
	adcq	%rdx    , %r10
	movq	24(%rsi), %rdx
	adcq	%rdx    , %r11
	movq	32(%rsi), %rdx
	adcq	%rdx    , %r12
	movq	40(%rsi), %rdx
	adcq	%rdx    , %r13

	rcrq 	$1, %r13
	rcrq 	$1, %r12
  	rcrq 	$1, %r11
  	rcrq 	$1, %r10
  	rcrq 	$1, %r9
  	rcrq 	$1, %r8

	movq	%r8 ,  0(%rdi)
	movq	%r9 ,  8(%rdi)
	movq	%r10, 16(%rdi)
	movq	%r11, 24(%rdi)
	movq	%r12, 32(%rdi)
	movq	%r13, 40(%rdi)
	xorq	%rax, %rax

	pop		%r13
	pop		%r12
	ret

fp_hlvd_low:
	push	%rbx
	push	%rbp
	push	%r12
	push	%r13
	push	%r14
	push	%r15

	xorq	%rdx, %rdx

	movq	$P0, %r8
	movq	$P1, %r9
	movq	$P2, %r10
	movq	$P3, %r11
	movq	$P4, %r12
	movq	$P5, %r13

  	movq 	$1     ,%rax
  	movq 	0(%rsi),%rcx
  	andq 	%rcx   ,%rax

	cmovz	%rdx, %r8
	cmovz	%rdx, %r9
	cmovz	%rdx, %r10
	cmovz	%rdx, %r11
	cmovz	%rdx, %r12
	cmovz	%rdx, %r13

	addq	%rcx     , %r8
	adcq	8(%rsi)  , %r9
	adcq	16(%rsi) , %r10
	adcq	24(%rsi) , %r11
	adcq	32(%rsi) , %r12
	adcq	40(%rsi) , %r13
	movq	48(%rsi) , %r14
	adcq	$0       , %r14
	movq	56(%rsi) , %r15
	adcq	$0       , %r15
	movq	64(%rsi) , %rax
	adcq	$0       , %rax
	movq	72(%rsi) , %rbx
	adcq	$0       , %rbx
	movq	80(%rsi) , %rcx
	adcq	$0       , %rcx
	movq	88(%rsi) , %rdx
	adcq	$0       , %rdx

  	rcrq 	$1, %rdx
  	rcrq 	$1, %rcx
  	rcrq 	$1, %rbx
  	rcrq 	$1, %rax
	rcrq 	$1, %r15
	rcrq 	$1, %r14
	rcrq 	$1, %r13
	rcrq 	$1, %r12
  	rcrq 	$1, %r11
  	rcrq 	$1, %r10
  	rcrq 	$1, %r9
  	rcrq 	$1, %r8

  	movq 	%rdx, 88(%rdi)
  	movq 	%rcx, 80(%rdi)
  	movq 	%rbx, 72(%rdi)
  	movq 	%rax, 64(%rdi)
  	movq 	%r15, 56(%rdi)
  	movq 	%r14, 48(%rdi)
  	movq 	%r13, 40(%rdi)
  	movq 	%r12, 32(%rdi)
  	movq 	%r11, 24(%rdi)
  	movq 	%r10, 16(%rdi)
  	movq 	%r9 ,  8(%rdi)
  	movq 	%r8 ,  0(%rdi)

	pop		%r15
	pop		%r14
	pop		%r13
	pop		%r12
	pop		%rbp
	pop		%rbx
	ret
