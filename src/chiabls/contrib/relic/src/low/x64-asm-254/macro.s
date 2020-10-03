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
 * @ingroup fp
 */

#define P0 $0xA700000000000013
#define P1 $0x6121000000000013
#define P2 $0xBA344D8000000008
#define P3 $0x2523648240000001
#define U0 $0x08435E50D79435E5

#define NP40 $0xC000000000000000
#define NP41 $0xE9C0000000000004
#define NP42 $0x1848400000000004
#define NP43 $0x6E8D136000000002
#define NP44 $0x0948D92090000000

#define NP20 $0x8000000000000000         // N*p/2
#define NP21 $0xD380000000000009
#define NP22 $0x3090800000000009
#define NP23 $0xDD1A26C000000004
#define NP24 $0x1291B24120000000

#if defined(__APPLE__)
#define cdecl(S) _PREFIX(,S)
#else
#define cdecl(S) S
#endif

.text

.macro ADD1_STEP i, j
	movq	8*\i(%rsi), %r10
	adcq	$0, %r10
	movq	%r10, 8*\i(%rdi)
	.if \i - \j
		ADD1_STEP "(\i + 1)", \j
	.endif
.endm

.macro ADDN_STEP i, j
	movq	8*\i(%rdx), %r11
	adcq	8*\i(%rsi), %r11
	movq	%r11, 8*\i(%rdi)
	.if \i - \j
		ADDN_STEP "(\i + 1)", \j
	.endif
.endm

.macro SUB1_STEP i, j
	movq	8*\i(%rsi),%r10
	sbbq	$0, %r10
	movq	%r10,8*\i(%rdi)
	.if \i - \j
		SUB1_STEP "(\i + 1)", \j
	.endif
.endm

.macro SUBN_STEP i, j
	movq	8*\i(%rsi), %r8
	sbbq	8*\i(%rdx), %r8
	movq	%r8, 8*\i(%rdi)
	.if \i - \j
		SUBN_STEP "(\i + 1)", \j
	.endif
.endm

.macro DBLN_STEP i, j
	movq	8*\i(%rsi), %r8
	adcq	%r8, %r8
	movq	%r8, 8*\i(%rdi)
	.if \i - \j
		DBLN_STEP "(\i + 1)", \j
	.endif
.endm

/* Montgomery reduction, comba, optimized for BN254 */
.macro FP_RDCN_LOW C, A
	xorq	%r10, %r10    
	movq	U0, %rcx
// i=0
	movq	0(\A), %r8
	movq	%r8, %rax
	mulq	%rcx         // v*U0
	movq	%rax, %r14
	movq	P0, %r11
	mulq	%r11          // z0*m0
	addq	%rax,%r8
	movq	P1, %r12
	adcq	8(\A),%rdx
    movq    %rdx,%r9
    adcq    $0,%r10      
// i=1, j=0    
	movq	%r14, %rax
	mulq	%r12          // z0*m1
	addq	%rax,%r9
	movq	%r9, %rax     
	adcq	%rdx,%r10

	mulq	%rcx          // v*U0
	movq	%rax, 8(\A)
	xorq    %r8,%r8       
	mulq	%r11          // z1*m0
	addq	%rax,%r9
	movq	P2, %r13
	adcq	%rdx,%r10
	adcq	$0,%r8
// i=2, j=0,1 
	movq	%r14, %rax
	mulq	%r13         // z0*m2
	addq	16(\A),%r10
	adcq	$0,%r8

	addq	%rax,%r10
	adcq	%rdx,%r8

    xorq    %r9,%r9  
	movq	8(\A), %rax  
	mulq	%r12         // z1*m1
	addq	%rax,%r10
	adcq	%rdx,%r8
	adcq	$0,%r9
	movq	%r10, %rax   
	mulq	%rcx         // v*U0
	movq	%rax, 16(\A)
	mulq	%r11         // z2*m0
	addq	%rax,%r10
	movq	%r14, %rax
	adcq	%rdx,%r8
	movq	P3, %r14
	adcq	$0,%r9
// i=3, j=0,1,2  
	mulq	%r14           // z0*m3
	addq    24(\A),%r8    
	adcq	$0,%r9          

	addq	%rax,%r8
	adcq	%rdx,%r9
	     
	movq	8(\A), %rax 
	mulq	%r13           // z1*m2
	addq	%rax,%r8
	movq	16(\A), %rax  
	adcq	%rdx,%r9
 
	xorq	%r10, %r10  
	mulq	%r12           // z2*m1
	addq	%rax,%r8
	movq	%r8, %rax 
	adcq	%rdx,%r9
	adcq	$0,%r10 

	mulq	%rcx           // v*U0
	movq    %rax, 24(\A)
	mulq    %r11           // z3*m0
	addq	%rax,%r8
	adcq	%rdx,%r9
	adcq	$0,%r10
// loop 2
// i=4, j=1,2,3  
	movq	8(\A), %rax  
	mulq	%r14           // z1*m3
	addq	32(\A),%r9
	adcq	$0,%r10

	addq	%rax,%r9
	adcq	%rdx,%r10

	movq	16(\A), %rax  
	mulq	%r13           // z2*m2
	addq	%rax,%r9

	adcq	%rdx,%r10
	xorq	%r8, %r8    
	movq	24(\A), %rax  

	mulq	%r12           // z3*m1
	addq	%rax,%r9
	adcq	%rdx,%r10
	movq	%r9,%r12       // Z0
	adcq	$0,%r8
// i=5, j=2,3
	xorq	%r11, %r11     
	movq	16(\A), %rax
	mulq	%r14           // z2*m3
	addq	%rax,%r10
	adcq	%rdx,%r8

	movq	24(\A), %rax
	mulq	%r13           // z3*m2
	addq	%rax,%r10
	adcq	%rdx,%r8

	addq	40(\A),%r10
	movq	%r10,%r13      // Z1
	adcq	$0,%r8
// i=6, j=3
	movq	24(\A), %rax
	mulq	%r14           // z3*m3
	addq	%rax,%r8
	adcq	%rdx,%r11

	addq	48(\A),%r8
	movq	%r8,%r14       // Z2
	adcq	$0,%r11

	addq	56(\A),%r11
	movq	%r11,%rcx      // Z3 

	movq	P0,%rax
	subq	%rax,%r9
	movq	P1,%rax
	sbbq	%rax,%r10
	movq	P2,%rax
	sbbq	%rax,%r8
	movq	P3,%rax
	sbbq	%rax,%r11
	cmovnc	%r9,%r12       
	movq	%r12,0(\C)
	cmovnc	%r10,%r13
	movq	%r13,8(\C)
	cmovnc	%r8,%r14
	movq	%r14,16(\C)
	cmovnc	%r11,%rcx
	movq	%rcx,24(\C)
.endm

.macro FP_MULN_LOW C, R0, R1, R2, A0, A1, A2, A3, B0, B1, B2, B3
	movq \A0,%rax
	mulq \B0
	movq %rax,0(\C)
	movq %rdx,\R0

	movq \A0,%rax
	mulq \B1
	addq %rax,\R0
	movq %rdx,\R1
	adcq $0,\R1

	xorq \R2,\R2
	movq \A1,%rax
	mulq \B0
	addq %rax,\R0
	movq \R0,8(\C)
	adcq %rdx,\R1
	adcq $0,\R2

	movq \A0,%rax
	mulq \B2
	addq %rax,\R1
	adcq %rdx,\R2
	//adcq $0,\R0

	xorq \R0,\R0
	movq \A1,%rax
	mulq \B1
	addq %rax,\R1
	adcq %rdx,\R2
	adcq $0,\R0

	movq \A2,%rax
	mulq \B0
	addq %rax,\R1
	movq \R1,16(\C)
	adcq %rdx,\R2
	adcq $0,\R0

	//xorq \R1,\R1
	movq \A0,%rax
	mulq \B3
	addq %rax,\R2
	adcq %rdx,\R0
	//adcq $0,\R1

	xorq \R1,\R1
	movq \A1,%rax
	mulq \B2
	addq %rax,\R2
	adcq %rdx,\R0
	adcq $0,\R1

	movq \A2,%rax
	mulq \B1
	addq %rax,\R2
	adcq %rdx,\R0
	adcq $0,\R1

	movq \A3,%rax
	mulq \B0
	addq %rax,\R2
	movq \R2,24(\C)
	adcq %rdx,\R0
	adcq $0,\R1

	movq \A1,%rax
	mulq \B3
	addq %rax,\R0
	adcq %rdx,\R1
	//adcq $0,\R2

	xorq \R2,\R2
	movq \A2,%rax
	mulq \B2
	addq %rax,\R0
	adcq %rdx,\R1
	adcq $0,\R2

	movq \A3,%rax
	mulq \B1
	addq %rax,\R0
	movq \R0,32(\C)
	adcq %rdx,\R1
	adcq $0,\R2

	movq \A2,%rax
	mulq \B3
	addq %rax,\R1
	adcq %rdx,\R2

    xorq \R0,\R0	
	movq \A3,%rax
	mulq \B2
	addq %rax,\R1
	movq \R1,40(\C)
	adcq %rdx,\R2
	adcq $0,\R0

	movq \A3,%rax
	mulq \B3
	addq %rax,\R2
	movq \R2,48(\C)
	adcq %rdx,\R0
	movq \R0,56(\C)
.endm
