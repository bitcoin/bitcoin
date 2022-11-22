/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2017 RELIC Authors
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

#include "relic_fp_low.h"

/**
 * @file
 *
 * Implementation of the low-level prime field addition and subtraction
 * functions.
 *
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
p6: .quad P6
p7: .quad P7
p8: .quad P8
p9: .quad P9

.global p0
.global p1
.global p2
.global p3
.global p4
.global p5
.global p6
.global p7
.global p8
.global p9

.hidden p0
.hidden p1
.hidden p2
.hidden p3
.hidden p4
.hidden p5
.hidden p6
.hidden p7
.hidden p8
.hidden p9

.text

.global cdecl(fp_add1_low)
.global cdecl(fp_addn_low)
.global cdecl(fp_addm_low)
.global cdecl(fp_addd_low)
.global cdecl(fp_addc_low)
.global cdecl(fp_sub1_low)
.global cdecl(fp_subn_low)
.global cdecl(fp_subm_low)
.global cdecl(fp_subd_low)
.global cdecl(fp_subc_low)
.global cdecl(fp_negm_low)
.global cdecl(fp_dbln_low)
.global cdecl(fp_dblm_low)
.global cdecl(fp_hlvm_low)
.global cdecl(fp_hlvd_low)

cdecl(fp_add1_low):
	movq	0(%rsi), %r10
	addq	%rdx   , %r10
	movq	%r10   , 0(%rdi)

	ADD1	1, (RLC_FP_DIGS - 1)

	ret

cdecl(fp_addn_low):
	movq	0(%rdx), %r11
	addq	0(%rsi), %r11
	movq	%r11   , 0(%rdi)

	ADDN 	1, (RLC_FP_DIGS - 1)

	xorq	%rax, %rax

	ret

cdecl(fp_addm_low):
	push	%rbx
	push	%rbp
	push	%r12
	push	%r13
	push	%r14
	push	%r15
	push	%rdi

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
	movq	48(%rdx), %r14
	adcq	48(%rsi), %r14
	movq	56(%rdx), %r15
	adcq	56(%rsi), %r15
	movq	%r15    , 0(%rdi)
	movq	%r15    , 8(%rdi)
	movq	64(%rdx), %rax
	adcq	64(%rsi), %rax
	movq	%rax    , 16(%rdi)
	movq	%rax    , 24(%rdi)
	movq	72(%rdx), %rcx
	adcq	72(%rsi), %rcx
	movq	%rcx    , 32(%rdi)
	movq	%rcx    , 40(%rdi)

	movq	%rdi, %r15

	movq 	%r8 , %rax
	movq 	%r9 , %rcx
	movq 	%r10, %rdx
	movq 	%r11, %rsi
	movq	%r12, %rbx
	movq	%r13, %rbp
	movq	%r14, %rdi

	subq	p0(%rip), %rax
	sbbq	p1(%rip), %rcx
	sbbq	p2(%rip), %rdx
	sbbq	p3(%rip), %rsi
	sbbq	p4(%rip), %rbx
	sbbq	p5(%rip), %rbp
	sbbq	p6(%rip), %rdi

	push	%rdi

	movq	%r15, %rdi
	movq	8(%rdi), %r15
	sbbq	p7(%rip), %r15
	movq	%r15, 8(%rdi)
	movq	24(%rdi), %r15
	sbbq	p8(%rip), %r15
	movq	%r15, 24(%rdi)
	movq	40(%rdi), %r15
	sbbq	p9(%rip), %r15
	movq	%r15, 40(%rdi)

	pop		%rdi
	cmovnc	%rax, %r8
	cmovnc	%rcx, %r9
	cmovnc	%rdx, %r10
	cmovnc	%rsi, %r11
	cmovnc	%rbx, %r12
	cmovnc	%rbp, %r13
	cmovnc	%rdi, %r14
	pop		%rdi
	movq	0(%rdi), %r15
	movq	16(%rdi), %rax
	movq	32(%rdi), %rcx
	cmovnc	8(%rdi), %r15
	cmovnc  24(%rdi), %rax
	cmovnc  40(%rdi), %rcx

	movq	%r8 ,  0(%rdi)
	movq	%r9 ,  8(%rdi)
	movq	%r10, 16(%rdi)
	movq	%r11, 24(%rdi)
	movq	%r12, 32(%rdi)
	movq	%r13, 40(%rdi)
	movq	%r14, 48(%rdi)
	movq	%r15, 56(%rdi)
	movq	%rax, 64(%rdi)
	movq	%rcx, 72(%rdi)
	xorq	%rax, %rax

	pop		%r15
	pop		%r14
	pop		%r13
	pop		%r12
	pop		%rbp
	pop		%rbx
	ret

cdecl(fp_addd_low):
	movq	0(%rdx), %r11
	addq	0(%rsi), %r11
	movq	%r11   , 0(%rdi)

	ADDN 	1, (2 * RLC_FP_DIGS - 1)

	ret

cdecl(fp_addc_low):
	push	%rbx
	push	%rbp
	push	%r12
	push	%r13
	push	%r14
	push	%r15
	push	%rdi

	movq	0(%rsi), %r8
	addq	0(%rdx), %r8
	movq	%r8    , 0(%rdi)

	ADDN	1, (RLC_FP_DIGS - 1)

	movq     80(%rsi), %r8
	adcq     80(%rdx), %r8
	movq     88(%rsi), %r9
	adcq     88(%rdx), %r9
	movq     96(%rsi), %r10
	adcq     96(%rdx), %r10
	movq    104(%rsi), %r11
	adcq    104(%rdx), %r11
	movq    112(%rsi), %r12
	adcq    112(%rdx), %r12
	movq    120(%rsi), %r13
	adcq    120(%rdx), %r13
	movq    128(%rsi), %r14
	adcq    128(%rdx), %r14
	movq    136(%rsi), %r15
	adcq    136(%rdx), %r15
	movq	%r15    , 80(%rdi)
	movq	%r15    , 88(%rdi)
	movq	144(%rdx), %rax
	adcq	144(%rsi), %rax
	movq	%rax    , 96(%rdi)
	movq	%rax    , 104(%rdi)
	movq	152(%rdx), %rcx
	adcq	152(%rsi), %rcx
	movq	%rcx    , 112(%rdi)
	movq	%rcx    , 120(%rdi)

	movq	%rdi, %r15

	movq 	%r8 , %rax
	movq 	%r9 , %rcx
	movq 	%r10, %rdx
	movq 	%r11, %rsi
	movq	%r12, %rbx
	movq	%r13, %rbp
	movq	%r14, %rdi

	subq	p0(%rip), %rax
	sbbq	p1(%rip), %rcx
	sbbq	p2(%rip), %rdx
	sbbq	p3(%rip), %rsi
	sbbq	p4(%rip), %rbx
	sbbq	p5(%rip), %rbp
	sbbq	p6(%rip), %rdi

	push	%rdi

	movq	%r15, %rdi
	movq	88(%rdi), %r15
	sbbq	p7(%rip), %r15
	movq	%r15, 88(%rdi)
	movq	104(%rdi), %r15
	sbbq	p8(%rip), %r15
	movq	%r15, 104(%rdi)
	movq	120(%rdi), %r15
	sbbq	p9(%rip), %r15
	movq	%r15, 120(%rdi)

	pop		%rdi

	cmovnc	%rax, %r8
	cmovnc	%rcx, %r9
	cmovnc	%rdx, %r10
	cmovnc	%rsi, %r11
	cmovnc	%rbx, %r12
	cmovnc	%rbp, %r13
	cmovnc	%rdi, %r14

	pop		%rdi
	movq	80(%rdi), %r15
	movq	96(%rdi), %rax
	movq	112(%rdi), %rcx
	cmovnc	88(%rdi), %r15
	cmovnc  104(%rdi), %rax
	cmovnc  120(%rdi), %rcx

	movq	%r8 , 80(%rdi)
	movq	%r9 , 88(%rdi)
	movq	%r10, 96(%rdi)
	movq	%r11, 104(%rdi)
	movq	%r12, 112(%rdi)
	movq	%r13, 120(%rdi)
	movq	%r14, 128(%rdi)
	movq	%r15, 136(%rdi)
	movq	%rax, 144(%rdi)
	movq	%rcx, 152(%rdi)
	xorq	%rax, %rax

	pop		%r15
	pop		%r14
	pop		%r13
	pop		%r12
	pop		%rbp
	pop		%rbx
	ret

cdecl(fp_sub1_low):
	movq	0(%rsi), %r10
	subq	%rdx   , %r10
	movq	%r10   , 0(%rdi)

	SUB1 	1, (RLC_FP_DIGS - 1)

	ret

cdecl(fp_subn_low):
	xorq	%rax   , %rax
	movq	0(%rsi), %r11
	subq	0(%rdx), %r11
	movq	%r11   , 0(%rdi)

	SUBN 	1, (RLC_FP_DIGS - 1)

	adcq	$0, %rax

	ret

cdecl(fp_subm_low):
	pushq	%r12
	pushq	%r13
	xorq	%rax, %rax
	xorq	%rcx, %rcx

	movq	0(%rsi), %r8
	subq	0(%rdx), %r8
	movq	%r8    , 0(%rdi)

	SUBN	1, (RLC_FP_DIGS - 1)

	movq	$0, %r8
	movq	$0, %r9
	movq	$0, %r10
	movq	$0, %r11
	movq	$0, %rdx
	movq	$0, %rsi
	movq	$0, %r12
	movq	$0, %r13

	cmovc	p0(%rip), %rax
	cmovc	p1(%rip), %rcx
	cmovc	p2(%rip), %r8
	cmovc	p3(%rip), %r9
	cmovc	p4(%rip), %r10
	cmovc	p5(%rip), %r11
	cmovc	p6(%rip), %rdx
	cmovc	p7(%rip), %rsi
	cmovc	p8(%rip), %r12
	cmovc	p9(%rip), %r13

	addq	%rax,  0(%rdi)
	adcq	%rcx,  8(%rdi)
	adcq	%r8,  16(%rdi)
	adcq	%r9,  24(%rdi)
	adcq	%r10, 32(%rdi)
	adcq	%r11, 40(%rdi)
	adcq	%rdx, 48(%rdi)
	adcq	%rsi, 56(%rdi)
	adcq	%r12, 64(%rdi)
	adcq	%r13, 72(%rdi)

	pop		%r13
	pop		%r12
	ret

cdecl(fp_subd_low):
	movq	0(%rsi), %r8
	subq	0(%rdx), %r8
	movq	%r8, 0(%rdi)

	SUBN 	1, (2 * RLC_FP_DIGS - 1)

	ret

cdecl(fp_subc_low):
	push	%r12
	push	%r13
	xorq    %rax,%rax
	xorq    %rcx,%rcx

	movq    0(%rsi), %r8
	subq    0(%rdx), %r8
	movq    %r8,     0(%rdi)

	SUBN 	1, (2 * RLC_FP_DIGS - 1)

	movq	$0, %r8
	movq	$0, %r9
	movq	$0, %r10
	movq	$0, %r11
	movq	$0, %rsi
	movq	$0, %rdx
	movq	$0, %r12
	movq	$0, %r13

	cmovc	p0(%rip), %rax
	cmovc	p1(%rip), %rcx
	cmovc	p2(%rip), %r8
	cmovc	p3(%rip), %r9
	cmovc	p4(%rip), %r10
	cmovc	p5(%rip), %r11
	cmovc	p6(%rip), %rsi
	cmovc	p7(%rip), %rdx
	cmovc	p8(%rip), %r12
	cmovc	p9(%rip), %r13

	addq	%rax,  80(%rdi)
	adcq	%rcx,  88(%rdi)
	adcq	%r8,   96(%rdi)
	adcq	%r9,  104(%rdi)
	adcq	%r10, 112(%rdi)
	adcq	%r11, 120(%rdi)
	adcq	%rsi, 128(%rdi)
	adcq	%rdx, 136(%rdi)
	adcq	%r12, 144(%rdi)
	adcq	%r13, 152(%rdi)

	pop		%r13
	pop		%r12
	ret

cdecl(fp_negm_low):
    movq    0(%rsi) , %r8
    or 	    8(%rsi) , %r8
    or 	    16(%rsi), %r8
    or 	    24(%rsi), %r8
    or 	    32(%rsi), %r8
    or 	    40(%rsi), %r8
    or 	    48(%rsi), %r8
    or 	    56(%rsi), %r8
    or 	    64(%rsi), %r8
    test    %r8, %r8
	cmovnz 	p0(%rip), %r8
	subq 	0(%rsi) , %r8
	movq 	%r8     , 0(%rdi)
	cmovnz 	p1(%rip), %r8
	sbbq 	8(%rsi) , %r8
	movq 	%r8     , 8(%rdi)
	cmovnz 	p2(%rip), %r8
	sbbq 	16(%rsi), %r8
	movq 	%r8     , 16(%rdi)
	cmovnz 	p3(%rip), %r8
	sbbq 	24(%rsi), %r8
	movq 	%r8     , 24(%rdi)
	cmovnz 	p4(%rip), %r8
	sbbq 	32(%rsi), %r8
	movq 	%r8     , 32(%rdi)
	cmovnz 	p5(%rip), %r8
	sbbq 	40(%rsi), %r8
	movq 	%r8     , 40(%rdi)
    cmovnz 	p6(%rip), %r8
	sbbq 	48(%rsi), %r8
	movq 	%r8     , 48(%rdi)
    cmovnz 	p7(%rip), %r8
	sbbq 	56(%rsi), %r8
	movq 	%r8     , 56(%rdi)
    cmovnz 	p8(%rip), %r8
	sbbq 	64(%rsi), %r8
	movq 	%r8     , 64(%rdi)
    cmovnz 	p9(%rip), %r8
	sbbq 	72(%rsi), %r8
	movq 	%r8     , 72(%rdi)
  	ret

cdecl(fp_dbln_low):
	movq	0(%rsi), %r8
	addq	%r8    , %r8
	movq	%r8    , 0(%rdi)

	DBLN 	1, (RLC_FP_DIGS - 1)

	xorq	%rax,%rax
	ret

cdecl(fp_dblm_low):
	push	%rbx
	push	%rbp
	push	%r12
	push	%r13
	push	%r14
	push	%r15
	push	%rdi

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
	movq	48(%rsi), %r14
	adcq	%r14    , %r14
	movq	56(%rsi), %r15
	adcq	%r15    , %r15
	movq	%r15    , 0(%rdi)
	movq	%r15    , 8(%rdi)
	movq	64(%rsi), %rax
	adcq	%rax    , %rax
	movq	%rax    , 16(%rdi)
	movq	%rax    , 24(%rdi)
	movq	72(%rsi), %rcx
	adcq	%rcx    , %rcx
	movq	%rcx    , 32(%rdi)
	movq	%rcx    , 40(%rdi)

	movq	%rdi, %r15

	movq 	%r8 , %rax
	movq 	%r9 , %rcx
	movq 	%r10, %rdx
	movq 	%r11, %rsi
	movq	%r12, %rbx
	movq	%r13, %rbp
	movq	%r14, %rdi

	subq	p0(%rip), %rax
	sbbq	p1(%rip), %rcx
	sbbq	p2(%rip), %rdx
	sbbq	p3(%rip), %rsi
	sbbq	p4(%rip), %rbx
	sbbq	p5(%rip), %rbp
	sbbq	p6(%rip), %rdi

	push	%rdi
	movq	%r15, %rdi

	movq	8(%rdi), %r15
	sbbq	p7(%rip), %r15
	movq	%r15, 8(%rdi)
	movq	24(%rdi), %r15
	sbbq	p8(%rip), %r15
	movq	%r15, 24(%rdi)
	movq	40(%rdi), %r15
	sbbq	p9(%rip), %r15
	movq	%r15, 40(%rdi)

	pop		%rdi

	cmovnc	%rax, %r8
	cmovnc	%rcx, %r9
	cmovnc	%rdx, %r10
	cmovnc	%rsi, %r11
	cmovnc	%rbx, %r12
	cmovnc	%rbp, %r13
	cmovnc	%rdi, %r14

	pop		%rdi

	movq	0(%rdi), %r15
	movq	16(%rdi), %rax
	movq	32(%rdi), %rcx
	cmovnc	8(%rdi), %r15
	cmovnc  24(%rdi), %rax
	cmovnc  40(%rdi), %rcx

	movq	%r8 ,  0(%rdi)
	movq	%r9 ,  8(%rdi)
	movq	%r10, 16(%rdi)
	movq	%r11, 24(%rdi)
	movq	%r12, 32(%rdi)
	movq	%r13, 40(%rdi)
	movq	%r14, 48(%rdi)
	movq	%r15, 56(%rdi)
	movq	%rax, 64(%rdi)
	movq	%rcx, 72(%rdi)
	xorq	%rax, %rax

	pop		%r15
	pop		%r14
	pop		%r13
	pop		%r12
	pop		%rbp
	pop		%rbx
	ret

cdecl(fp_hlvm_low):
	push	%r12
	push	%r13
	push	%r14
	push	%r15
	push	%rbp
	push	%rbx

	xorq	%rdx, %rdx

	movq	$P0, %r8
	movq	$P1, %r9
	movq	$P2, %r10
	movq	$P3, %r11
	movq	$P4, %r12
	movq	$P5, %r13
	movq	$P6, %r14
	movq	$P7, %r15
	movq	$P8, %rbp
	movq	$P9, %rbx

  	movq 	$1     ,%rax
  	movq 	0(%rsi),%rcx
  	andq 	%rcx   ,%rax

	cmovz	%rdx, %r8
	cmovz	%rdx, %r9
	cmovz	%rdx, %r10
	cmovz	%rdx, %r11
	cmovz	%rdx, %r12
	cmovz	%rdx, %r13
	cmovz	%rdx, %r14
	cmovz	%rdx, %r15
	cmovz	%rdx, %rbp
	cmovz	%rdx, %rbx

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
	movq	48(%rsi), %rdx
	adcq	%rdx    , %r14
	movq	56(%rsi), %rdx
	adcq	%rdx    , %r15
	movq	64(%rsi), %rdx
	adcq	%rdx    , %rbp
	mov	72(%rsi), %rdx
	adcq	%rdx    ,%rbx

	rcrq	$1, %rbx
	rcrq    $1, %rbp
	rcrq 	$1, %r15
	rcrq 	$1, %r14
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
	movq	%r14, 48(%rdi)
	movq	%r15, 56(%rdi)
	movq	%rbp, 64(%rdi)
	movq	%rbx, 72(%rdi)
	xorq	%rax, %rax

	pop     %rbx
	pop	%rbp
	pop	%r15
	pop	%r14
	pop	%r13
	pop	%r12
	ret

cdecl(fp_hlvd_low):
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
	movq	$P6, %r14
	movq	$P7, %r15
	movq	$P8, %rax
	movq	$P9, %rbx

  	movq 	$1     ,%rbp
  	movq 	0(%rsi),%rcx
  	andq 	%rcx   ,%rbp

	cmovz	%rdx, %r8
	cmovz	%rdx, %r9
	cmovz	%rdx, %r10
	cmovz	%rdx, %r11
	cmovz	%rdx, %r12
	cmovz	%rdx, %r13
	cmovz	%rdx, %r14
	cmovz	%rdx, %r15
	cmovz	%rdx, %rax
	cmovz	%rdx, %rbx

	addq	%rcx     , %r8
	adcq	8(%rsi)  , %r9
	adcq	16(%rsi) , %r10
	adcq	24(%rsi) , %r11
	adcq	32(%rsi) , %r12
	adcq	40(%rsi) , %r13
	adcq	48(%rsi) , %r14
	adcq	56(%rsi) , %r15
	adcq	64(%rsi) , %rax
	adcq	72(%rsi) , %rbx
	movq	80(%rsi) , %rcx
	adcq	$0       , %rcx
	movq	88(%rsi) , %rdx
	adcq	$0       , %rdx
	movq	96(%rsi) , %rbp
	adcq	$0       , %rbp

	push	%rbp

	movq	104(%rsi), %rbp
	adcq	$0       , %rbp
	movq	%rbp     , 104(%rdi)
	movq	112(%rsi), %rbp
	adcq	$0       , %rbp
	movq	%rbp     , 112(%rdi)
	movq	120(%rsi), %rbp
	adcq	$0       , %rbp
	movq	%rbp     , 120(%rdi)
	movq	128(%rsi), %rbp
	adcq	$0       , %rbp
	movq	%rbp     , 128(%rdi)
	movq	136(%rsi), %rbp
	adcq	$0       , %rbp
	movq	%rbp     , 136(%rdi)
	movq	144(%rsi), %rbp
	adcq	$0       , %rbp
	movq	%rbp     , 144(%rdi)
	movq	152(%rsi), %rbp
	adcq	$0       , %rbp
	movq	%rbp     , 152(%rdi)

	pop		%rbp

	rcrq	$1, 152(%rdi)
	rcrq	$1, 144(%rdi)
	rcrq	$1, 136(%rdi)
	rcrq	$1, 128(%rdi)
	rcrq	$1, 120(%rdi)
	rcrq	$1, 112(%rdi)
	rcrq	$1, 104(%rdi)
  	rcrq	$1, %rbp
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

  	movq 	%rbp, 96(%rdi)
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
