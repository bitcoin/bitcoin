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
 * Implementation of low-level prime field multiplication.
 *
 * @version $Id: relic_fp_add_low.c 88 2009-09-06 21:27:19Z dfaranha $
 * @ingroup fp
 */

#if FP_PRIME == 381
#define P0	0xB9FEFFFFFFFFAAAB
#define P1	0x1EABFFFEB153FFFF
#define P2	0x6730D2A0F6B0F624
#define P3	0x64774B84F38512BF
#define P4	0x4B1BA7B6434BACD7
#define P5	0x1A0111EA397FE69A
#define U0	0x89F3FFFCFFFCFFFD
#elif FP_PRIME == 382
#define P0	0x004E000000000013
#define P1	0x09480097801382BE
#define P2	0xA6E58DBE43002A06
#define P3	0x6F82CEFBE47879BB
#define P4	0x2D996CC179C6D166
#define P5	0x24009015183F9489
#define U0	0xDF615E50D79435E5
#elif FP_PRIME == 383
#define P0	0xDA371D6485AAB0AB
#define P1	0x7A8C3F298A64852B
#define P2	0xAC31B801696124F4
#define P3	0xA0AD462CF365A511
#define P4	0xA06DADC41FEA9284
#define P5	0x5565569564AB6EB5
#define U0	0x69BC0571073435FD
#endif

.text

.macro ADD1 i j
	movq	8*\i(%rsi), %r10
	adcq	$0, %r10
	movq	%r10, 8*\i(%rdi)
	.if \i - \j
		ADD1 "(\i + 1)" \j
	.endif
.endm

.macro ADDN i j
	movq	8*\i(%rdx), %r11
	adcq	8*\i(%rsi), %r11
	movq	%r11, 8*\i(%rdi)
	.if \i - \j
		ADDN "(\i + 1)" \j
	.endif
.endm

.macro SUB1 i j
	movq	8*\i(%rsi),%r10
	sbbq	$0, %r10
	movq	%r10,8*\i(%rdi)
	.if \i - \j
		SUB1 "(\i + 1)" \j
	.endif
.endm

.macro SUBN i j
	movq	8*\i(%rsi), %r8
	sbbq	8*\i(%rdx), %r8
	movq	%r8, 8*\i(%rdi)
	.if \i - \j
		SUBN "(\i + 1)" \j
	.endif
.endm

.macro DBLN i j
	movq	8*\i(%rsi), %r8
	adcq	%r8, %r8
	movq	%r8, 8*\i(%rdi)
	.if \i - \j
		DBLN "(\i + 1)" \j
	.endif
.endm

.macro MULN i, j, k, C, R0, R1, R2, A, B
	.if \j > \k
		movq	8*\i(\A), %rax
		mulq	8*\j(\B)
		addq	%rax    , \R0
		adcq	%rdx    , \R1
		adcq	$0      , \R2
		MULN	"(\i + 1)", "(\j - 1)", \k, \C, \R0, \R1, \R2, \A, \B
	.else
		movq	8*\i(\A), %rax
		mulq	8*\j(\B)
		addq	%rax    , \R0
		movq	\R0     , 8*(\i+\j)(\C)
		adcq	%rdx    , \R1
		adcq	$0      , \R2
	.endif
.endm

.macro FP_MULN_LOW C, R0, R1, R2, A, B
	movq 	0(\A),%rax
	mulq 	0(\B)
	movq 	%rax ,0(\C)
	movq 	%rdx ,\R0

	xorq 	\R1,\R1
	xorq 	\R2,\R2
	MULN 	0, 1, 0, \C, \R0, \R1, \R2, \A, \B
	xorq 	\R0,\R0
	MULN	0, 2, 0, \C, \R1, \R2, \R0, \A, \B
	xorq 	\R1,\R1
	MULN	0, 3, 0, \C, \R2, \R0, \R1, \A, \B
	xorq 	\R2,\R2
	MULN	0, 4, 0, \C, \R0, \R1, \R2, \A, \B
	xorq 	\R0,\R0
	MULN	0, 5, 0, \C, \R1, \R2, \R0, \A, \B
	xorq 	\R1,\R1
	MULN	1, 5, 1, \C, \R2, \R0, \R1, \A, \B
	xorq 	\R2,\R2
	MULN	2, 5, 2, \C, \R0, \R1, \R2, \A, \B
	xorq 	\R0,\R0
	MULN	3, 5, 3, \C, \R1, \R2, \R0, \A, \B
	xorq 	\R1,\R1
	MULN	4, 5, 4, \C, \R2, \R0, \R1, \A, \B

	movq	40(\A),%rax
	mulq	40(\B)
	addq	%rax  ,\R0
	movq	\R0   ,80(\C)
	adcq	%rdx  ,\R1
	movq	\R1   ,88(\C)
.endm

.macro _RDCN0 i, j, k, R0, R1, R2 A, P
	movq	8*\i(\A), %rax
	mulq	8*\j(\P)
	addq	%rax, \R0
	adcq	%rdx, \R1
	adcq	$0, \R2
	.if \j > 1
		_RDCN0 "(\i + 1)", "(\j - 1)", \k, \R0, \R1, \R2, \A, \P
	.else
		addq	8*\k(\A), \R0
		adcq	$0, \R1
		adcq	$0, \R2
		movq	\R0, %rax
		mulq	%rcx
		movq	%rax, 8*\k(\A)
		mulq	0(\P)
		addq	%rax , \R0
		adcq	%rdx , \R1
		adcq	$0   , \R2
		xorq	\R0, \R0
	.endif
.endm

.macro RDCN0 i, j, R0, R1, R2, A, P
	_RDCN0	\i, \j, \j, \R0, \R1, \R2, \A, \P
.endm

.macro _RDCN1 i, j, k, l, R0, R1, R2 A, P
	movq	8*\i(\A), %rax
	mulq	8*\j(\P)
	addq	%rax, \R0
	adcq	%rdx, \R1
	adcq	$0, \R2
	.if \j > \l
		_RDCN1 "(\i + 1)", "(\j - 1)", \k, \l, \R0, \R1, \R2, \A, \P
	.else
		addq	8*\k(\A), \R0
		adcq	$0, \R1
		adcq	$0, \R2
		movq	\R0, 8*\k(\A)
		xorq	\R0, \R0
	.endif
.endm

.macro RDCN1 i, j, R0, R1, R2, A, P
	_RDCN1	\i, \j, "(\i + \j)", \i, \R0, \R1, \R2, \A, \P
.endm

// r8, r9, r10, r11, r12, r13, r14, r15, rbp, rbx, rsp, //rsi, rdi, //rax, rcx, rdx
.macro FP_RDCN_LOW C, R0, R1, R2, A, P
	xorq	\R1, \R1
	movq	$U0, %rcx

	movq	0(\A), \R0
	movq	\R0  , %rax
	mulq	%rcx
	movq	%rax , 0(\A)
	mulq	0(\P)
	addq	%rax , \R0
	adcq	%rdx , \R1
	xorq    \R2  , \R2
	xorq    \R0  , \R0

	RDCN0	0, 1, \R1, \R2, \R0, \A, \P
	RDCN0	0, 2, \R2, \R0, \R1, \A, \P
	RDCN0	0, 3, \R0, \R1, \R2, \A, \P
	RDCN0	0, 4, \R1, \R2, \R0, \A, \P
	RDCN0	0, 5, \R2, \R0, \R1, \A, \P
	RDCN1	1, 5, \R0, \R1, \R2, \A, \P
	RDCN1	2, 5, \R1, \R2, \R0, \A, \P
	RDCN1	3, 5, \R2, \R0, \R1, \A, \P
	RDCN1	4, 5, \R0, \R1, \R2, \A, \P
	RDCN1	5, 5, \R1, \R2, \R0, \A, \P
	addq	8*11(\A), \R2
	movq	\R2, 8*11(\A)

	movq	48(\A), %r11
	movq	56(\A), %r12
	movq	64(\A), %r13
	movq	72(\A), %r14
	movq	80(\A), %r15
	movq	88(\A), %rcx

	subq	p0(%rip), %r11
	sbbq	p1(%rip), %r12
	sbbq	p2(%rip), %r13
	sbbq	p3(%rip), %r14
	sbbq	p4(%rip), %r15
	sbbq	p5(%rip), %rcx

	cmovc	48(\A), %r11
	cmovc	56(\A), %r12
	cmovc	64(\A), %r13
	cmovc	72(\A), %r14
	cmovc	80(\A), %r15
	cmovc	88(\A), %rcx
	movq	%r11,0(\C)
	movq	%r12,8(\C)
	movq	%r13,16(\C)
	movq	%r14,24(\C)
	movq	%r15,32(\C)
	movq	%rcx,40(\C)
.endm
