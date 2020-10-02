/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2017 RELIC Authors
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
 * Implementation of the low-level inversion functions.
 *
 * @&version $Id$
 * @ingroup fp
 */

#include "macro.s"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

/* No carry */
.macro DBLN R
	addq	%rdi,%rdi
	adcq	%rdx,%rdx
	adcq	%rsi,%rsi
	adcq	%rcx,%rcx
.endm

.macro DBLN2 R
	addq	%rdi,%rdi
	adcq	%rdx,%rdx
.endm

.macro DBLN3 R
	addq	%rdi,%rdi
	adcq	%rdx,%rdx
	adcq	%rsi,%rsi
.endm

#define P0 $0xA700000000000013
#define P1 $0x6121000000000013
#define P2 $0xBA344D8000000008
#define P3 $0x2523648240000001

#define Q3 $0x1B0A32FDF6403A3D
#define Q2 $0x281E3A1B7F86954F
#define Q1 $0x55EFBF6E8C1CC3F1
#define Q0 $0xB3E886745370473D

#define R0 $0x15FFFFFFFFFFFF8E
#define R1 $0xB939FFFFFFFFFF8A
#define R2 $0xA2C62EFFFFFFFFCD
#define R3 $0x212BA4F27FFFFFF5

.global cdecl(fp_invn_asm)

/**
 * rdi = a, rsi = x1, rdx = c
 */
cdecl(fp_invn_asm):
	push	%r12
	push	%r13
	push	%r14
	push	%r15
	push	%rbp
	push	%rbx
	push	%rdi
	subq 	$96, %rsp

	xorq	%rax, %rax
	movq	P0,%r12
	movq	P1,%r13
	movq	P2,%r14
	movq	P3,%r15

	movq	0(%rsi),%rbp
	movq	8(%rsi),%rbx
	movq	16(%rsi),%rcx
	movq	24(%rsi),%rsi

	movq	R0,%r8
	movq	%r8,0(%rdx)
	movq	R1,%r9
	movq	%r9,8(%rdx)
	movq	R2,%r10
	movq	%r10,16(%rdx)
	movq	R3,%r11
	movq	%r11,24(%rdx)

	movq	$1,%rdi
	xorq	%r8, %r8
	xorq	%r9, %r9
	xorq	%r10, %r10
	xorq	%rdx, %rdx

loop:
	movq	%r15,%r11
	orq		%rsi,%r11
	jz 		loop2
	inc		%rax
	test	$1, %r12
	jnz 	v_odd
	shrd    $1, %r13, %r12
	shrd    $1, %r14, %r13
	shrd    $1, %r15, %r14
	shr     $1, %r15
	DBLN2 	%rsp
	jmp 	loop
v_odd:
	test	$1, %rbp
	jnz 	u_odd
	shrd    $1, %rbx, %rbp
	shrd    $1, %rcx, %rbx
	shrd    $1, %rsi, %rcx
	shr     $1, %rsi
	addq	%r8,%r8
	adcq	%r9,%r9
	jmp 	loop
u_odd:
	subq	%rbp, %r12
	sbbq	%rbx, %r13
	sbbq	%rcx, %r14
	sbbq	%rsi, %r15
	jc 	cmp_lt
	shrd    $1, %r13, %r12
	shrd    $1, %r14, %r13
	shrd    $1, %r15, %r14
	shr     $1, %r15
	addq	%rdi,%r8
	adcq	%rdx,%r9
	DBLN2	%rsp
	jmp		loop
cmp_lt:
	addq	%rbp, %r12
	adcq	%rbx, %r13
	adcq	%rcx, %r14
	adcq	%rsi, %r15
	subq	%r12, %rbp
	sbbq	%r13, %rbx
	sbbq	%r14, %rcx
	sbbq	%r15, %rsi
	shrd    $1, %rbx, %rbp
	shrd    $1, %rcx, %rbx
	shrd    $1, %rsi, %rcx
	shr     $1, %rsi
	addq	%r8,%rdi
	adcq	%r9,%rdx
	addq	%r8,%r8
	adcq	%r9,%r9
	jmp loop

loop2:
	movq	%r14,%r11
	orq		%rcx,%r11
	jz loop3
	inc		%rax
	test	$1, %r12
	jnz 	v_odd2
	shrd    $1, %r13, %r12
	shrd    $1, %r14, %r13
	shr     $1, %r14
	DBLN3 	%rsp
	jmp 	loop2
v_odd2:
	test	$1, %rbp
	jnz 	u_odd2
	shrd    $1, %rbx, %rbp
	shrd    $1, %rcx, %rbx
	shr     $1, %rcx
	addq	%r8,%r8
	adcq	%r9,%r9
	adcq	%r10,%r10
	jmp 	loop2
u_odd2:
	subq	%rbp, %r12
	sbbq	%rbx, %r13
	sbbq	%rcx, %r14
	jc 	cmp_lt2
	shrd    $1, %r13, %r12
	shrd    $1, %r14, %r13
	shr     $1, %r14
	addq	%rdi,%r8
	adcq	%rdx,%r9
	adcq	%rsi,%r10
	DBLN3	%rsp
	jmp		loop2
cmp_lt2:
	addq	%rbp, %r12
	adcq	%rbx, %r13
	adcq	%rcx, %r14
	subq	%r12, %rbp
	sbbq	%r13, %rbx
	sbbq	%r14, %rcx
	shrd    $1, %rbx, %rbp
	shrd    $1, %rcx, %rbx
	shr    $1, %rcx
	addq	%r8,%rdi
	adcq	%r9,%rdx
	adcq	%r10,%rsi
	addq	%r8,%r8
	adcq	%r9,%r9
	adcq	%r10,%r10
	jmp loop2

loop3:
	movq	%r13,%r11
	orq 	%rbx,%r11
	jz 		loop4
	inc		%rax
	test	$1, %r12
	jnz 	v_odd3
	shrd    $1, %r13, %r12
	shr    $1, %r13
	DBLN 	%rsp
	jmp 	loop3
v_odd3:
	test	$1, %rbp
	jnz 	u_odd3
	shrd    $1, %rbx, %rbp
	shr    $1, %rbx
	addq	%r8,%r8
	adcq	%r9,%r9
	adcq	%r10,%r10
	adcq	%r15,%r15
	jmp 	loop3
u_odd3:
	subq	%rbp, %r12
	sbbq	%rbx, %r13
	jc 	cmp_lt3
	shrd    $1, %r13, %r12
	shr    $1, %r13
	addq	%rdi,%r8
	adcq	%rdx,%r9
	adcq	%rsi,%r10
	adcq	%rcx,%r15
	DBLN	%rsp
	jmp		loop3
cmp_lt3:
	addq	%rbp, %r12
	adcq	%rbx, %r13
	subq	%r12, %rbp
	sbbq	%r13, %rbx
	shrd    $1, %rbx, %rbp
	shr     $1, %rbx
	addq	%r8,%rdi
	adcq	%r9,%rdx
	adcq	%r10,%rsi
	adcq	%r15,%rcx
	addq	%r8,%r8
	adcq	%r9,%r9
	adcq	%r10,%r10
	adcq	%r15,%r15
	jmp loop3

loop4:
	movq	%r12,%r11
	orq		%r12,%r11
	jz 		end
	inc		%rax
	test	$1, %r12
	jnz 	v_odd4
	shr    $1, %r12
	DBLN 	%rsp
	jmp 	loop4
v_odd4:
	test	$1, %rbp
	jnz 	u_odd4
	shrd    $1, %rbx, %rbp
	shr    $1, %rbx
	addq	%r8,%r8
	adcq	%r9,%r9
	adcq	%r10,%r10
	adcq	%r15,%r15
	jmp 	loop4
u_odd4:
	subq	%rbp, %r12
	jc		cmp_lt4
	shr		$1, %r12
	addq	%rdi,%r8
	adcq	%rdx,%r9
	adcq	%rsi,%r10
	adcq	%rcx,%r15
	DBLN	%rsp
	jmp		loop4
cmp_lt4:
	addq	%rbp, %r12
	subq	%r12, %rbp
	shr     $1, %rbp
	addq	%r8,%rdi
	adcq	%r9,%rdx
	adcq	%r10,%rsi
	adcq	%r15,%rcx
	addq	%r8,%r8
	adcq	%r9,%r9
	adcq	%r10,%r10
	adcq	%r15,%r15
	jmp loop4

end:
	movq	Q0,%r12
	movq	Q1,%r13
	movq	Q2,%r14
	movq	Q3,%r15
	movq	%rax,%rbx
	movq	%rsp,%rbp
	movq	%rdx,0(%rsp)
	addq	$32,%rbp
	FP_MULN_LOW %rbp, %r8, %r9, %r10, %rdi, 0(%rsp), %rsi, %rcx, %r12, %r13, %r14, %r15
	addq 	$96, %rsp
	pop	%rdi
	FP_RDCN_LOW %rdi, %rbp
	movq 	%rbx,%rax
exit:
	pop		%rbx
	pop		%rbp
	pop		%r15
	pop		%r14
	pop		%r13
	pop		%r12
	ret
