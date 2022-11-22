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

#include "macro.s"

.text
.global cdecl(fp_rsh1_low)
.global cdecl(fp_lsh1_low)

cdecl(fp_rsh1_low):
	movq	0(%rsi), %r8
	movq	8(%rsi), %r9
	movq	16(%rsi), %r10
	movq	24(%rsi), %r11
	shrd	$1, %r9, %r8
	shrd	$1, %r10, %r9
	shrd	$1, %r11, %r10
	shr	$1, %r11
	movq	%r8,0(%rdi)
	movq	%r9,8(%rdi)
	movq	%r10,16(%rdi)
	movq	%r11,24(%rdi)
	ret

cdecl(fp_lsh1_low):
        movq    0(%rsi), %r8
        movq    8(%rsi), %r9
        movq    16(%rsi), %r10
        movq    24(%rsi), %r11
        shld    $1, %r10, %r11
        shld    $1, %r9, %r10
        shld    $1, %r8, %r9
        shl     $1, %r8
        movq    %r8,0(%rdi)
        movq    %r9,8(%rdi)
        movq    %r10,16(%rdi)
        movq    %r11,24(%rdi)
        xorq    %rax, %rax
        ret
