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
 * Implementation of the low-level prime field multiplication functions.
 *
 * @version $Id: relic_fp_mul_low.c 683 2011-03-10 23:51:23Z dfaranha $
 * @ingroup bn
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

.global fp_muln_low
.global fp_mulm_low

fp_muln_low:
	movq %rdx,%rcx
	FP_MULN_LOW %rdi, %r8, %r9, %r10, %rsi, %rcx
	ret

fp_mulm_low:
	push	%r12
	push	%r13
	push	%r14
	push	%r15
	push 	%rbx
	push	%rbp
	subq 	$96, %rsp

	movq 	%rdx,%rcx
	leaq 	p0, %rbx

	FP_MULN_LOW %rsp, %r8, %r9, %r10, %rsi, %rcx

	FP_RDCN_LOW %rdi, %r8, %r9, %r10, %rsp, %rbx

	addq	$96, %rsp

	pop		%rbp
	pop		%rbx
	pop		%r15
	pop		%r14
	pop		%r13
	pop		%r12
	ret
