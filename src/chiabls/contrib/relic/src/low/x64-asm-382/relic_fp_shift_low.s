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

.text
.global fp_rsh1_low
.global fp_lsh1_low

fp_rsh1_low:
	movq	0(%rsi), %r8
	movq	8(%rsi), %r9
	movq	16(%rsi), %r10
	movq	24(%rsi), %r11
	movq	32(%rsi), %rax
	movq	40(%rsi), %rcx
	shrd	$1, %r9, %r8
	shrd	$1, %r10, %r9
	shrd	$1, %r11, %r10
	shrd	$1, %rax, %r11
	shrd	$1, %rcx, %rax
	shr	$1, %rcx
	movq	%r8,0(%rdi)
	movq	%r9,8(%rdi)
	movq	%r10,16(%rdi)
	movq	%r11,24(%rdi)
	movq	%rax,32(%rdi)
	movq	%rcx,40(%rdi)
	ret

fp_lsh1_low:
	movq	0(%rsi), %r8
	movq	8(%rsi), %r9
	movq	16(%rsi), %r10
	movq	24(%rsi), %r11
	movq	32(%rsi), %rax
	movq	40(%rsi), %rcx
	shld	$1, %rax, %rcx
	shld	$1, %r11, %rax
	shld	$1, %r10, %r11
	shld	$1, %r9, %r10
	shld	$1, %r8, %r9
	shl	$1, %r8
	movq	%r8,0(%rdi)
	movq	%r9,8(%rdi)
	movq	%r10,16(%rdi)
	movq	%r11,24(%rdi)
	movq	%rax,32(%rdi)
	movq	%rcx,40(%rdi)
	xorq	%rax, %rax
	ret
