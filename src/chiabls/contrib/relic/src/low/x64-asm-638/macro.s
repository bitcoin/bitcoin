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
 * Implementation of low-level prime field multiplication.
 *
 * @version $Id: relic_fp_add_low.c 88 2009-09-06 21:27:19Z dfaranha $
 * @ingroup fp
 */

#define P0	$0xAAAAAAAABEAB000B
#define P1	$0xEAF3FF000AAAAAAA
#define P2	$0xAAAAAAAAAAAAAA93
#define P3	$0x00000AC79600D2AB
#define P4	$0x5C75D6C2AB000000
#define P5	$0x3955555555529C00
#define P6	$0x00631BBD42171501
#define P7	$0xFC01DCDE95D40000
#define P8	$0xE80015554DD25DB0
#define P9	$0x3CB868653D300B3F
#define U0	$0xFAD7AB621D14745D

#define NP40 $0xC000000000000000
#define NP41 $0xE9C0000000000004
#define NP42 $0x1848400000000004
#define NP43 $0x6E8D136000000002
#define NP44 $0x0948D92090000000

#if defined(__APPLE__)
#define cdecl(S) _PREFIX(,S)
#else
#define cdecl(S) S
#endif

.text

.macro ADD1 i, j
	movq	8*\i(%rsi), %r10
	adcq	$0, %r10
	movq	%r10, 8*\i(%rdi)
	.if \i - \j
		ADD1 "(\i + 1)", \j
	.endif
.endm

.macro ADDN i, j
	movq	8*\i(%rdx), %r11
	adcq	8*\i(%rsi), %r11
	movq	%r11, 8*\i(%rdi)
	.if \i - \j
		ADDN "(\i + 1)", \j
	.endif
.endm

.macro SUB1 i, j
	movq	8*\i(%rsi),%r10
	sbbq	$0, %r10
	movq	%r10,8*\i(%rdi)
	.if \i - \j
		SUB1 "(\i + 1)", \j
	.endif
.endm

.macro SUBN i, j
	movq	8*\i(%rsi), %r8
	sbbq	8*\i(%rdx), %r8
	movq	%r8, 8*\i(%rdi)
	.if \i - \j
		SUBN "(\i + 1)", \j
	.endif
.endm

.macro DBLN i, j
	movq	8*\i(%rsi), %r8
	adcq	%r8, %r8
	movq	%r8, 8*\i(%rdi)
	.if \i - \j
		DBLN "(\i + 1)", \j
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
	MULN	0, 6, 0, \C, \R2, \R0, \R1, \A, \B
	xorq 	\R2,\R2
	MULN	0, 7, 0, \C, \R0, \R1, \R2, \A, \B
	xorq 	\R0,\R0
	MULN	0, 8, 0, \C, \R1, \R2, \R0, \A, \B
	xorq 	\R1,\R1
	MULN	0, 9, 0, \C, \R2, \R0, \R1, \A, \B
	xorq 	\R2,\R2
	MULN	1, 9, 1, \C, \R0, \R1, \R2, \A, \B
	xorq 	\R0,\R0
	MULN	2, 9, 2, \C, \R1, \R2, \R0, \A, \B
	xorq 	\R1,\R1
	MULN	3, 9, 3, \C, \R2, \R0, \R1, \A, \B
	xorq 	\R2,\R2
	MULN	4, 9, 4, \C, \R0, \R1, \R2, \A, \B
	xorq 	\R0,\R0
	MULN	5, 9, 5, \C, \R1, \R2, \R0, \A, \B
	xorq 	\R1,\R1
	MULN	6, 9, 6, \C, \R2, \R0, \R1, \A, \B
	xorq 	\R2,\R2
	MULN	7, 9, 7, \C, \R0, \R1, \R2, \A, \B
	xorq 	\R0,\R0
	MULN	8, 9, 8, \C, \R1, \R2, \R0, \A, \B

	movq	72(\A),%rax
	mulq	72(\B)
	addq	%rax  ,\R2
	movq	\R2   ,144(\C)
	adcq	%rdx  ,\R0
	movq	\R0   ,152(\C)
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
	movq	U0, %rcx

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
	RDCN0	0, 6, \R0, \R1, \R2, \A, \P
	RDCN0	0, 7, \R1, \R2, \R0, \A, \P
	RDCN0	0, 8, \R2, \R0, \R1, \A, \P
	RDCN0	0, 9, \R0, \R1, \R2, \A, \P
	RDCN1	1, 9, \R1, \R2, \R0, \A, \P
	RDCN1	2, 9, \R2, \R0, \R1, \A, \P
	RDCN1	3, 9, \R0, \R1, \R2, \A, \P
	RDCN1	4, 9, \R1, \R2, \R0, \A, \P
	RDCN1	5, 9, \R2, \R0, \R1, \A, \P
	RDCN1	6, 9, \R0, \R1, \R2, \A, \P
	RDCN1	7, 9, \R1, \R2, \R0, \A, \P
	RDCN1	8, 9, \R2, \R0, \R1, \A, \P
	RDCN1	9, 9, \R0, \R1, \R2, \A, \P
	addq	152(\A), \R1
	movq	\R1, 152(\A)

	movq	80(\A), %r11
	movq	88(\A), %r12
	movq	96(\A), %r13
	movq	104(\A), %r14
	movq	112(\A), %r15
	movq	120(\A), %rcx
	movq	128(\A), %rbp
	movq	136(\A), %rdx
	movq	144(\A), %r8
	movq	152(\A), %r9

	subq	p0, %r11
	sbbq	p1, %r12
	sbbq	p2, %r13
	sbbq	p3, %r14
	sbbq	p4, %r15
	sbbq	p5, %rcx
	sbbq	p6, %rbp
	sbbq	p7, %rdx
	sbbq	p8, %r8
	sbbq	p9, %r9

	cmovc	80(\A), %r11
	cmovc	88(\A), %r12
	cmovc	96(\A), %r13
	cmovc	104(\A), %r14
	cmovc	112(\A), %r15
	cmovc	120(\A), %rcx
	cmovc	128(\A), %rbp
	cmovc	136(\A), %rdx
	cmovc	144(\A), %r8
	cmovc	152(\A), %r9
	movq	%r11,0(\C)
	movq	%r12,8(\C)
	movq	%r13,16(\C)
	movq	%r14,24(\C)
	movq	%r15,32(\C)
	movq	%rcx,40(\C)
	movq	%rbp,48(\C)
	movq	%rdx,56(\C)
	movq	%r8,64(\C)
	movq	%r9,72(\C)
.endm
