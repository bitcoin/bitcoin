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

#include "macro.s"

.text
.global cdecl(fp_muln_low)
.global cdecl(fp_mulm_low)

cdecl(fp_mulm_low):
	push %r12
	push %r13
	push %r14
	subq $64, %rsp

	movq %rdx,%rcx

	FP_MULN_LOW %rsp, %r8, %r9, %r10, 0(%rsi), 8(%rsi), 16(%rsi), 24(%rsi), 0(%rcx), 8(%rcx), 16(%rcx), 24(%rcx)

	FP_RDCN_LOW %rdi, %rsp

	addq $64, %rsp
	pop %r14
	pop	%r13
	pop	%r12
	ret

cdecl(fp_muln_low):
	movq %rdx,%rcx
	FP_MULN_LOW %rdi, %r8, %r9, %r10, 0(%rsi), 8(%rsi), 16(%rsi), 24(%rsi), 0(%rcx), 8(%rcx), 16(%rcx), 24(%rcx)
	ret
