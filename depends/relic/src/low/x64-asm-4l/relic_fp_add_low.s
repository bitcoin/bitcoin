/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2013 RELIC Authors
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

.text

.global cdecl(fp_add1_low)
.global cdecl(fp_addn_low)
.global cdecl(fp_addm_low)
.global cdecl(fp_addd_low)
.global cdecl(fp_addc_low)
.global cdecl(fp_sub1_low)
.global cdecl(fp_subn_low)
.global cdecl(fp_subm_low)
.global cdecl(fp_subc_low)
.global cdecl(fp_subd_low)
.global cdecl(fp_negm_low)
.global cdecl(fp_dbln_low)
.global cdecl(fp_dblm_low)
.global cdecl(fp_hlvm_low)
.global cdecl(fp_hlvd_low)

/*
 * Function: fp_add1_low
 * Inputs: rdi = c, rsi = a, rdx = digit
 * Output: rax
 */
cdecl(fp_add1_low):
    xorq	%rax, %rax
	movq	0(%rsi), %r10
	addq	%rdx, %r10
	movq	%r10, 0(%rdi)

	ADD1_STEP 1, (RLC_FP_DIGS - 1)

    adcq    $0, %rax

	ret

/*
 * Function: fp_addn_low
 * Inputs: rdi = c, rsi = a, rdx = b
 * Output: rax
 */
cdecl(fp_addn_low):
    xorq	%rax, %rax
	movq	0(%rdx), %r11
	addq	0(%rsi), %r11
	movq	%r11, 0(%rdi)

	ADDN_STEP 1, (RLC_FP_DIGS - 1)

	adcq    $0, %rax

	ret

/*
 * Function: fp_addm_low
 * Inputs: rdi = c, rsi = a, rdx = b
 * Output: (a+b) mod p
 */
cdecl(fp_addm_low):
	push	%r12
	movq	0(%rdx), %r8
	addq	0(%rsi), %r8
	movq	8(%rdx), %r9
	adcq	8(%rsi), %r9
	movq	16(%rdx), %r10
	adcq	16(%rsi), %r10
	movq	24(%rdx), %r11
	adcq	24(%rsi), %r11

	xorq	%rax, %rax
	movq 	%r8, %rax
	movq 	%r9, %rcx
	movq 	%r10, %rdx
	movq 	%r11, %rsi

	movq	P0, %r12
	subq	%r12, %rax
	movq	P1, %r12
	sbbq	%r12, %rcx
	movq	P2, %r12
	sbbq	%r12, %rdx
	movq	P3, %r12
	sbbq	%r12, %rsi

	cmovnc	%rax, %r8
	cmovnc	%rcx, %r9
	cmovnc	%rdx, %r10
	cmovnc	%rsi, %r11

	movq	%r8,0(%rdi)
	movq	%r9,8(%rdi)
	movq	%r10,16(%rdi)
	movq	%r11,24(%rdi)
	xorq	%rax, %rax
	pop		%r12
	ret

cdecl(fp_addd_low):
	movq	0(%rdx), %r11
	addq	0(%rsi), %r11
	movq	%r11, 0(%rdi)

	ADDN_STEP 1, (2 * RLC_FP_DIGS - 1)

	ret

cdecl(fp_addc_low):
	push    %r12
	push    %r13
	push    %r14
	movq    0(%rsi), %r8
	addq    0(%rdx), %r8
	movq    %r8,0(%rdi)
	movq    8(%rsi), %r8
	adcq    8(%rdx), %r8
	movq    %r8,8(%rdi)
	movq    16(%rsi), %r8
	adcq    16(%rdx), %r8
	movq    %r8,16(%rdi)
	movq    24(%rsi), %r8
	adcq    24(%rdx), %r8
	movq    %r8,24(%rdi)
	movq    32(%rsi), %r8
	adcq    32(%rdx), %r8
	movq    %r8,%r12
	movq    40(%rsi), %r9
	adcq    40(%rdx), %r9
	movq    %r9,%r13
	movq    48(%rsi), %r10
	adcq    48(%rdx), %r10
	movq    %r10,%r14
	movq    56(%rsi), %r11
	adcq    56(%rdx), %r11
	movq    %r11,%rcx

	movq    P0,%rax
	subq    %rax,%r8
	movq    P1,%rax
	sbbq    %rax,%r9
	movq    P2,%rax
	sbbq    %rax,%r10
	movq    P3,%rax
	sbbq    %rax,%r11
	cmovc   %r12, %r8
	cmovc   %r13, %r9
	cmovc   %r14, %r10
	cmovc   %rcx, %r11
	movq    %r8,32(%rdi)
	movq    %r9,40(%rdi)
	movq    %r10,48(%rdi)
	movq    %r11,56(%rdi)

	pop	%r14
	pop	%r13
	pop	%r12
	ret

/*
 * Function: fp_sub1_low
 * Inputs: rdi = c, rsi = a, rdx = digit
 * Output: rax
 */
cdecl(fp_sub1_low):
	movq	0(%rsi),%r10
	subq	%rdx, %r10
	movq	%r10,0(%rdi)

	SUB1_STEP 1, (RLC_FP_DIGS - 1)

	ret

/*
 * Function: fp_subn_low
 * Inputs: rdi = c, rsi = a, rdx = b
 * Output: rax
 */
cdecl(fp_subn_low):
	xorq	%rax, %rax
	movq	0(%rsi), %r11
	subq	0(%rdx), %r11
	movq	%r11, 0(%rdi)

	SUBN_STEP 1, (RLC_FP_DIGS - 1)

	adcq	$0, %rax

	ret

cdecl(fp_subm_low):
	xorq	%rax,%rax
	xorq	%rcx,%rcx
	movq	0(%rsi), %r8
	subq	0(%rdx), %r8
	movq	%r8,0(%rdi)

	movq	8(%rsi), %r9
	sbbq	8(%rdx), %r9
	movq	%r9,8(%rdi)
	movq	16(%rsi), %r10
	sbbq	16(%rdx), %r10
	movq	%r10,16(%rdi)
	movq	24(%rsi), %r11
	sbbq	24(%rdx), %r11
	movq	%r11,24(%rdi)

	movq	$0, %rdx
	movq	$0, %rsi
	movq	P0,%r8
	movq	P1,%r9
	movq	P2,%r10
	movq	P3,%r11
	cmovc	%r8, %rax
	cmovc	%r9, %rcx
	cmovc	%r10, %rdx
	cmovc	%r11, %rsi
	addq	%rax,0(%rdi)
	adcq	%rcx,8(%rdi)
	adcq	%rdx,16(%rdi)
	adcq	%rsi,24(%rdi)
	ret

cdecl(fp_subc_low):
	xorq    %rax,%rax
	xorq    %rcx,%rcx
	movq    0(%rsi), %r8
	subq    0(%rdx), %r8
	movq    %r8, 0(%rdi)

	SUBN_STEP 1, (2 * RLC_FP_DIGS - 1)

	movq	$0, %rsi
	movq	$0, %rdx
	movq    P0,%r8
	movq    P1,%r9
	movq    P2,%r10
	movq    P3,%r11
	cmovc   %r8, %rax
	cmovc   %r9, %rsi
	cmovc   %r10, %rcx
	cmovc   %r11, %rdx
	addq    %rax,32(%rdi)
	adcq    %rsi,40(%rdi)
	adcq    %rcx,48(%rdi)
	adcq    %rdx,56(%rdi)

	ret

cdecl(fp_subd_low):
	movq	0(%rsi), %r8
	subq	0(%rdx), %r8
	movq	%r8, 0(%rdi)

	SUBN_STEP 1, (2 * RLC_FP_DIGS - 1)

	ret

cdecl(fp_negm_low):
    xorq    %rax, %rax
    mov     P0, %rcx
    mov     P1, %r9
    mov     P2, %r10
    mov     P3, %r11
    subq 	0(%rsi) , %rcx
    sbbq 	8(%rsi) , %r9
    sbbq 	16(%rsi) , %r10
    sbbq 	24(%rsi) , %r11

    movq    0(%rsi) , %r8
    or 	    8(%rsi) , %r8
    or 	    16(%rsi), %r8
    or 	    24(%rsi), %r8
    test    %r8, %r8

    cmovnz 	%rcx, %rax
    movq 	%rax, 0(%rdi)
    cmovnz 	%r9, %rax
    movq 	%rax, 8(%rdi)
    cmovnz 	%r10, %rax
    movq 	%rax, 16(%rdi)
    cmovnz 	%r11, %rax
    movq 	%rax, 24(%rdi)

    ret

cdecl(fp_dbln_low):
	movq	0(%rsi), %r8
	addq	%r8, %r8
	movq	%r8, 0(%rdi)

	DBLN_STEP 1, (RLC_FP_DIGS - 1)

	xorq	%rax,%rax
	ret

cdecl(fp_dblm_low):
	push	%r12
	xorq	%rax,%rax
	xorq	%rdx,%rdx
	movq	0(%rsi), %r8
	addq	%r8, %r8
	movq	8(%rsi), %r9
	adcq	%r9, %r9
	movq	16(%rsi), %r10
	adcq	%r10, %r10
	movq	24(%rsi), %r11
	adcq	%r11, %r11
	adcq	%rax,%rax

	movq 	%r8, %rax
	movq 	%r9, %rcx
	movq 	%r10, %rdx
	movq 	%r11, %rsi

	movq	P0, %r12
	subq	%r12, %rax
	movq	P1, %r12
	sbbq	%r12, %rcx
	movq	P2, %r12
	sbbq	%r12, %rdx
	movq	P3, %r12
	sbbq	%r12, %rsi

	cmovnc	%rax, %r8
	cmovnc	%rcx, %r9
	cmovnc	%rdx, %r10
	cmovnc	%rsi, %r11

	movq	%r8,0(%rdi)
	movq	%r9,8(%rdi)
	movq	%r10,16(%rdi)
	movq	%r11,24(%rdi)
	xorq	%rax, %rax
	pop		%r12
	ret

cdecl(fp_hlvm_low):
	xorq	%rdx, %rdx
	movq	P0, %r8
	movq	P1, %r9
	movq	P2, %r10
	movq	P3, %r11
  	movq 	0(%rsi),%rcx
  	test 	$1,%rcx
	cmovz	%rdx,%r8
	cmovz	%rdx,%r9
	cmovz	%rdx,%r10
	cmovz	%rdx,%r11

	addq	%rcx, %r8
	movq	8(%rsi), %rdx
	adcq	%rdx, %r9
	movq	16(%rsi), %rdx
	adcq	%rdx, %r10
	movq	24(%rsi), %rdx
	adcq	%rdx, %r11

	shrd    $1, %r9, %r8
  	movq 	%r8,0(%rdi)
	shrd    $1, %r10, %r9
  	movq 	%r9,8(%rdi)
	shrd    $1, %r11, %r10
  	movq 	%r10,16(%rdi)
	shr     $1, %r11
  	movq 	%r11,24(%rdi)
	ret

cdecl(fp_hlvd_low):
	xorq	%rdx, %rdx
	movq	P0, %r8
	movq	P1, %r9
	movq	P2, %r10
	movq	P3, %r11
  	movq 	0(%rsi),%rcx
  	test 	$1,%rcx
	cmovz	%rdx,%r8
	cmovz	%rdx,%r9
	cmovz	%rdx,%r10
	cmovz	%rdx,%r11

	addq	%rcx, %r8
	movq	8(%rsi), %rdx
	adcq	%rdx, %r9
	movq	16(%rsi), %rdx
	adcq	%rdx, %r10
	movq	24(%rsi), %rdx
	adcq	%rdx, %r11
	movq	32(%rsi), %rax
	adcq	$0, %rax
	movq	40(%rsi), %rcx
	adcq	$0, %rcx
	movq	48(%rsi), %rdx
	adcq	$0, %rdx
	movq	56(%rsi), %rsi
	adcq	$0, %rsi

	shrd    $1, %r9, %r8
	shrd    $1, %r10, %r9
	shrd    $1, %r11, %r10
  	movq 	%r8,0(%rdi)
	shrd    $1, %rax, %r11
  	movq 	%r9,8(%rdi)
	shrd    $1, %rcx, %rax
  	movq 	%r10,16(%rdi)
	shrd    $1, %rdx, %rcx
  	movq 	%r11,24(%rdi)
	shrd    $1, %rsi, %rdx
  	movq 	%rax,32(%rdi)
	shr 	$1, %rsi
  	movq 	%rcx,40(%rdi)
  	movq 	%rdx,48(%rdi)
  	movq 	%rsi,56(%rdi)
	ret
