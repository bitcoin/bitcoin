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

p0: .quad 0xAAAAAAAABEAB000B
p1: .quad 0xEAF3FF000AAAAAAA
p2: .quad 0xAAAAAAAAAAAAAA93
p3: .quad 0x00000AC79600D2AB
p4: .quad 0x5C75D6C2AB000000
p5: .quad 0x3955555555529C00
p6: .quad 0x00631BBD42171501
p7: .quad 0xFC01DCDE95D40000
p8: .quad 0xE80015554DD25DB0
p9: .quad 0x3CB868653D300B3F
u0: .quad 0xFAD7AB621D14745D

.text

.global cdecl(fp_muln_low)
.global cdecl(fp_mulm_low)

cdecl(fp_muln_low):
	movq %rdx,%rcx
	FP_MULN_LOW %rdi, %r8, %r9, %r10, %rsi, %rcx
	ret

cdecl(fp_mulm_low):
	push	%r12
	push	%r13
	push	%r14
	push	%r15
	push 	%rbx
	push	%rbp
	subq 	$160, %rsp

	movq 	%rdx,%rcx
	leaq 	p0, %rbx

	FP_MULN_LOW %rsp, %r8, %r9, %r10, %rsi, %rcx

	FP_RDCN_LOW %rdi, %r8, %r9, %r10, %rsp, %rbx

	addq	$160, %rsp

	pop		%rbp
	pop		%rbx
	pop		%r15
	pop		%r14
	pop		%r13
	pop		%r12
	ret
