/*
 * Copyright 2011-2012 pooler@litecoinpool.org
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(__linux__) && defined(__ELF__)
	.section .note.GNU-stack,"",%progbits
#endif

#if defined(__x86_64__)

.macro scrypt_shuffle src, so, dest, do
	movl	\so+60(\src), %r8d
	movl	\so+44(\src), %r9d
	movl	\so+28(\src), %r10d
	movl	\so+12(\src), %r11d
	movl	%r8d, \do+12(\dest)
	movl	%r9d, \do+28(\dest)
	movl	%r10d, \do+44(\dest)
	movl	%r11d, \do+60(\dest)
	movl	\so+40(\src), %r8d
	movl	\so+8(\src), %r9d
	movl	\so+48(\src), %r10d
	movl	\so+16(\src), %r11d
	movl	%r8d, \do+8(\dest)
	movl	%r9d, \do+40(\dest)
	movl	%r10d, \do+16(\dest)
	movl	%r11d, \do+48(\dest)
	movl	\so+20(\src), %r8d
	movl	\so+4(\src), %r9d
	movl	\so+52(\src), %r10d
	movl	\so+36(\src), %r11d
	movl	%r8d, \do+4(\dest)
	movl	%r9d, \do+20(\dest)
	movl	%r10d, \do+36(\dest)
	movl	%r11d, \do+52(\dest)
	movl	\so+0(\src), %r8d
	movl	\so+24(\src), %r9d
	movl	\so+32(\src), %r10d
	movl	\so+56(\src), %r11d
	movl	%r8d, \do+0(\dest)
	movl	%r9d, \do+24(\dest)
	movl	%r10d, \do+32(\dest)
	movl	%r11d, \do+56(\dest)
.endm


.macro salsa8_core_gen_doubleround
	movq	72(%rsp), %r15
	
	leaq	(%r14, %rdx), %rbp
	roll	$7, %ebp
	xorl	%ebp, %r9d
	leaq	(%rdi, %r15), %rbp
	roll	$7, %ebp
	xorl	%ebp, %r10d
	leaq	(%rdx, %r9), %rbp
	roll	$9, %ebp
	xorl	%ebp, %r11d
	leaq	(%r15, %r10), %rbp
	roll	$9, %ebp
	xorl	%ebp, %r13d
	
	leaq	(%r9, %r11), %rbp
	roll	$13, %ebp
	xorl	%ebp, %r14d
	leaq	(%r10, %r13), %rbp
	roll	$13, %ebp
	xorl	%ebp, %edi
	leaq	(%r11, %r14), %rbp
	roll	$18, %ebp
	xorl	%ebp, %edx
	leaq	(%r13, %rdi), %rbp
	roll	$18, %ebp
	xorl	%ebp, %r15d
	
	movq	48(%rsp), %rbp
	movq	%r15, 72(%rsp)
	
	leaq	(%rax, %rbp), %r15
	roll	$7, %r15d
	xorl	%r15d, %ebx
	leaq	(%rbp, %rbx), %r15
	roll	$9, %r15d
	xorl	%r15d, %ecx
	leaq	(%rbx, %rcx), %r15
	roll	$13, %r15d
	xorl	%r15d, %eax
	leaq	(%rcx, %rax), %r15
	roll	$18, %r15d
	xorl	%r15d, %ebp
	
	movq	88(%rsp), %r15
	movq	%rbp, 48(%rsp)
	
	leaq	(%r12, %r15), %rbp
	roll	$7, %ebp
	xorl	%ebp, %esi
	leaq	(%r15, %rsi), %rbp
	roll	$9, %ebp
	xorl	%ebp, %r8d
	leaq	(%rsi, %r8), %rbp
	roll	$13, %ebp
	xorl	%ebp, %r12d
	leaq	(%r8, %r12), %rbp
	roll	$18, %ebp
	xorl	%ebp, %r15d
	
	movq	%r15, 88(%rsp)
	movq	72(%rsp), %r15
	
	leaq	(%rsi, %rdx), %rbp
	roll	$7, %ebp
	xorl	%ebp, %edi
	leaq	(%r9, %r15), %rbp
	roll	$7, %ebp
	xorl	%ebp, %eax
	leaq	(%rdx, %rdi), %rbp
	roll	$9, %ebp
	xorl	%ebp, %ecx
	leaq	(%r15, %rax), %rbp
	roll	$9, %ebp
	xorl	%ebp, %r8d
	
	leaq	(%rdi, %rcx), %rbp
	roll	$13, %ebp
	xorl	%ebp, %esi
	leaq	(%rax, %r8), %rbp
	roll	$13, %ebp
	xorl	%ebp, %r9d
	leaq	(%rcx, %rsi), %rbp
	roll	$18, %ebp
	xorl	%ebp, %edx
	leaq	(%r8, %r9), %rbp
	roll	$18, %ebp
	xorl	%ebp, %r15d
	
	movq	48(%rsp), %rbp
	movq	%r15, 72(%rsp)
	
	leaq	(%r10, %rbp), %r15
	roll	$7, %r15d
	xorl	%r15d, %r12d
	leaq	(%rbp, %r12), %r15
	roll	$9, %r15d
	xorl	%r15d, %r11d
	leaq	(%r12, %r11), %r15
	roll	$13, %r15d
	xorl	%r15d, %r10d
	leaq	(%r11, %r10), %r15
	roll	$18, %r15d
	xorl	%r15d, %ebp
	
	movq	88(%rsp), %r15
	movq	%rbp, 48(%rsp)
	
	leaq	(%rbx, %r15), %rbp
	roll	$7, %ebp
	xorl	%ebp, %r14d
	leaq	(%r15, %r14), %rbp
	roll	$9, %ebp
	xorl	%ebp, %r13d
	leaq	(%r14, %r13), %rbp
	roll	$13, %ebp
	xorl	%ebp, %ebx
	leaq	(%r13, %rbx), %rbp
	roll	$18, %ebp
	xorl	%ebp, %r15d
	
	movq	%r15, 88(%rsp)
.endm

	.text
	.p2align 6
salsa8_core_gen:
	/* 0: %rdx, %rdi, %rcx, %rsi */
	movq	8(%rsp), %rdi
	movq	%rdi, %rdx
	shrq	$32, %rdi
	movq	16(%rsp), %rsi
	movq	%rsi, %rcx
	shrq	$32, %rsi
	/* 1: %r9, 72(%rsp), %rax, %r8 */
	movq	24(%rsp), %r8
	movq	%r8, %r9
	shrq	$32, %r8
	movq	%r8, 72(%rsp)
	movq	32(%rsp), %r8
	movq	%r8, %rax
	shrq	$32, %r8
	/* 2: %r11, %r10, 48(%rsp), %r12 */
	movq	40(%rsp), %r10
	movq	%r10, %r11
	shrq	$32, %r10
	movq	48(%rsp), %r12
	/* movq	%r12, %r13 */
	/* movq	%r13, 48(%rsp) */
	shrq	$32, %r12
	/* 3: %r14, %r13, %rbx, 88(%rsp) */
	movq	56(%rsp), %r13
	movq	%r13, %r14
	shrq	$32, %r13
	movq	64(%rsp), %r15
	movq	%r15, %rbx
	shrq	$32, %r15
	movq	%r15, 88(%rsp)
	
	salsa8_core_gen_doubleround
	salsa8_core_gen_doubleround
	salsa8_core_gen_doubleround
	salsa8_core_gen_doubleround
	
	shlq	$32, %rdi
	xorq	%rdi, %rdx
	movq	%rdx, 24(%rsp)
	
	shlq	$32, %rsi
	xorq	%rsi, %rcx
	movq	%rcx, 32(%rsp)
	
	movl	72(%rsp), %edi
	shlq	$32, %rdi
	xorq	%rdi, %r9
	movq	%r9, 40(%rsp)
	
	movl	48(%rsp), %ebp
	shlq	$32, %r8
	xorq	%r8, %rax
	movq	%rax, 48(%rsp)
	
	shlq	$32, %r10
	xorq	%r10, %r11
	movq	%r11, 56(%rsp)
	
	shlq	$32, %r12
	xorq	%r12, %rbp
	movq	%rbp, 64(%rsp)
	
	shlq	$32, %r13
	xorq	%r13, %r14
	movq	%r14, 72(%rsp)
	
	movdqa	24(%rsp), %xmm0
	
	shlq	$32, %r15
	xorq	%r15, %rbx
	movq	%rbx, 80(%rsp)
	
	movdqa	40(%rsp), %xmm1
	movdqa	56(%rsp), %xmm2
	movdqa	72(%rsp), %xmm3
	
	ret
	
	
	.text
	.p2align 6
	.globl scrypt_core
	.globl _scrypt_core
scrypt_core:
_scrypt_core:
	pushq	%rbx
	pushq	%rbp
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
#if defined(WIN64)
	subq	$176, %rsp
	movdqa	%xmm6, 8(%rsp)
	movdqa	%xmm7, 24(%rsp)
	movdqa	%xmm8, 40(%rsp)
	movdqa	%xmm9, 56(%rsp)
	movdqa	%xmm10, 72(%rsp)
	movdqa	%xmm11, 88(%rsp)
	movdqa	%xmm12, 104(%rsp)
	movdqa	%xmm13, 120(%rsp)
	movdqa	%xmm14, 136(%rsp)
	movdqa	%xmm15, 152(%rsp)
	pushq	%rdi
	pushq	%rsi
	movq	%rcx, %rdi
	movq	%rdx, %rsi
#endif

.macro scrypt_core_cleanup
#if defined(WIN64)
	popq	%rsi
	popq	%rdi
	movdqa	8(%rsp), %xmm6
	movdqa	24(%rsp), %xmm7
	movdqa	40(%rsp), %xmm8
	movdqa	56(%rsp), %xmm9
	movdqa	72(%rsp), %xmm10
	movdqa	88(%rsp), %xmm11
	movdqa	104(%rsp), %xmm12
	movdqa	120(%rsp), %xmm13
	movdqa	136(%rsp), %xmm14
	movdqa	152(%rsp), %xmm15
	addq	$176, %rsp
#endif
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%rbp
	popq	%rbx
.endm
	
	/* GenuineIntel processors have fast SIMD */
	xorl	%eax, %eax
	cpuid
	cmpl	$0x6c65746e, %ecx
	jne scrypt_core_gen
	cmpl	$0x49656e69, %edx
	jne scrypt_core_gen
	cmpl	$0x756e6547, %ebx
	je scrypt_core_xmm
	
	.p2align 6
scrypt_core_gen:
	subq	$136, %rsp
	movdqa	0(%rdi), %xmm8
	movdqa	16(%rdi), %xmm9
	movdqa	32(%rdi), %xmm10
	movdqa	48(%rdi), %xmm11
	movdqa	64(%rdi), %xmm12
	movdqa	80(%rdi), %xmm13
	movdqa	96(%rdi), %xmm14
	movdqa	112(%rdi), %xmm15
	
	leaq	131072(%rsi), %rcx
	movq	%rdi, 104(%rsp)
	movq	%rsi, 112(%rsp)
	movq	%rcx, 120(%rsp)
scrypt_core_gen_loop1:
	movdqa	%xmm8, 0(%rsi)
	movdqa	%xmm9, 16(%rsi)
	movdqa	%xmm10, 32(%rsi)
	movdqa	%xmm11, 48(%rsi)
	movdqa	%xmm12, 64(%rsi)
	movdqa	%xmm13, 80(%rsi)
	movdqa	%xmm14, 96(%rsi)
	movdqa	%xmm15, 112(%rsi)
	
	pxor	%xmm12, %xmm8
	pxor	%xmm13, %xmm9
	pxor	%xmm14, %xmm10
	pxor	%xmm15, %xmm11
	movdqa	%xmm8, 0(%rsp)
	movdqa	%xmm9, 16(%rsp)
	movdqa	%xmm10, 32(%rsp)
	movdqa	%xmm11, 48(%rsp)
	movq	%rsi, 128(%rsp)
	call salsa8_core_gen
	paddd	%xmm0, %xmm8
	paddd	%xmm1, %xmm9
	paddd	%xmm2, %xmm10
	paddd	%xmm3, %xmm11
	
	pxor	%xmm8, %xmm12
	pxor	%xmm9, %xmm13
	pxor	%xmm10, %xmm14
	pxor	%xmm11, %xmm15
	movdqa	%xmm12, 0(%rsp)
	movdqa	%xmm13, 16(%rsp)
	movdqa	%xmm14, 32(%rsp)
	movdqa	%xmm15, 48(%rsp)
	call salsa8_core_gen
	movq	128(%rsp), %rsi
	paddd	%xmm0, %xmm12
	paddd	%xmm1, %xmm13
	paddd	%xmm2, %xmm14
	paddd	%xmm3, %xmm15
	
	addq	$128, %rsi
	movq	120(%rsp), %rcx
	cmpq	%rcx, %rsi
	jne scrypt_core_gen_loop1
	
	movq	$1024, %rcx
	movd	%xmm12, %edx
scrypt_core_gen_loop2:
	movq	112(%rsp), %rsi
	andl	$1023, %edx
	shll	$7, %edx
	addq	%rsi, %rdx
	movdqa	0(%rdx), %xmm0
	movdqa	16(%rdx), %xmm1
	movdqa	32(%rdx), %xmm2
	movdqa	48(%rdx), %xmm3
	movdqa	64(%rdx), %xmm4
	movdqa	80(%rdx), %xmm5
	movdqa	96(%rdx), %xmm6
	movdqa	112(%rdx), %xmm7
	pxor	%xmm0, %xmm8
	pxor	%xmm1, %xmm9
	pxor	%xmm2, %xmm10
	pxor	%xmm3, %xmm11
	pxor	%xmm4, %xmm12
	pxor	%xmm5, %xmm13
	pxor	%xmm6, %xmm14
	pxor	%xmm7, %xmm15
	
	pxor	%xmm12, %xmm8
	pxor	%xmm13, %xmm9
	pxor	%xmm14, %xmm10
	pxor	%xmm15, %xmm11
	movdqa	%xmm8, 0(%rsp)
	movdqa	%xmm9, 16(%rsp)
	movdqa	%xmm10, 32(%rsp)
	movdqa	%xmm11, 48(%rsp)
	movq	%rcx, 128(%rsp)
	call salsa8_core_gen
	paddd	%xmm0, %xmm8
	paddd	%xmm1, %xmm9
	paddd	%xmm2, %xmm10
	paddd	%xmm3, %xmm11
	
	pxor	%xmm8, %xmm12
	pxor	%xmm9, %xmm13
	pxor	%xmm10, %xmm14
	pxor	%xmm11, %xmm15
	movdqa	%xmm12, 0(%rsp)
	movdqa	%xmm13, 16(%rsp)
	movdqa	%xmm14, 32(%rsp)
	movdqa	%xmm15, 48(%rsp)
	call salsa8_core_gen
	movq	128(%rsp), %rcx
	addl	0(%rsp), %edx
	paddd	%xmm0, %xmm12
	paddd	%xmm1, %xmm13
	paddd	%xmm2, %xmm14
	paddd	%xmm3, %xmm15
	
	subq	$1, %rcx
	ja scrypt_core_gen_loop2
	
	movq	104(%rsp), %rdi
	movdqa	%xmm8, 0(%rdi)
	movdqa	%xmm9, 16(%rdi)
	movdqa	%xmm10, 32(%rdi)
	movdqa	%xmm11, 48(%rdi)
	movdqa	%xmm12, 64(%rdi)
	movdqa	%xmm13, 80(%rdi)
	movdqa	%xmm14, 96(%rdi)
	movdqa	%xmm15, 112(%rdi)
	
	addq	$136, %rsp
	scrypt_core_cleanup
	ret


.macro salsa8_core_xmm_doubleround
	movdqa	%xmm1, %xmm4
	paddd	%xmm0, %xmm4
	movdqa	%xmm4, %xmm5
	pslld	$7, %xmm4
	psrld	$25, %xmm5
	pxor	%xmm4, %xmm3
	movdqa	%xmm0, %xmm4
	pxor	%xmm5, %xmm3
	
	paddd	%xmm3, %xmm4
	movdqa	%xmm4, %xmm5
	pslld	$9, %xmm4
	psrld	$23, %xmm5
	pxor	%xmm4, %xmm2
	movdqa	%xmm3, %xmm4
	pxor	%xmm5, %xmm2
	pshufd	$0x93, %xmm3, %xmm3
	
	paddd	%xmm2, %xmm4
	movdqa	%xmm4, %xmm5
	pslld	$13, %xmm4
	psrld	$19, %xmm5
	pxor	%xmm4, %xmm1
	movdqa	%xmm2, %xmm4
	pxor	%xmm5, %xmm1
	pshufd	$0x4e, %xmm2, %xmm2
	
	paddd	%xmm1, %xmm4
	movdqa	%xmm4, %xmm5
	pslld	$18, %xmm4
	psrld	$14, %xmm5
	pxor	%xmm4, %xmm0
	movdqa	%xmm3, %xmm4
	pxor	%xmm5, %xmm0
	pshufd	$0x39, %xmm1, %xmm1
	
	paddd	%xmm0, %xmm4
	movdqa	%xmm4, %xmm5
	pslld	$7, %xmm4
	psrld	$25, %xmm5
	pxor	%xmm4, %xmm1
	movdqa	%xmm0, %xmm4
	pxor	%xmm5, %xmm1
	
	paddd	%xmm1, %xmm4
	movdqa	%xmm4, %xmm5
	pslld	$9, %xmm4
	psrld	$23, %xmm5
	pxor	%xmm4, %xmm2
	movdqa	%xmm1, %xmm4
	pxor	%xmm5, %xmm2
	pshufd	$0x93, %xmm1, %xmm1
	
	paddd	%xmm2, %xmm4
	movdqa	%xmm4, %xmm5
	pslld	$13, %xmm4
	psrld	$19, %xmm5
	pxor	%xmm4, %xmm3
	movdqa	%xmm2, %xmm4
	pxor	%xmm5, %xmm3
	pshufd	$0x4e, %xmm2, %xmm2
	
	paddd	%xmm3, %xmm4
	movdqa	%xmm4, %xmm5
	pslld	$18, %xmm4
	psrld	$14, %xmm5
	pxor	%xmm4, %xmm0
	pshufd	$0x39, %xmm3, %xmm3
	pxor	%xmm5, %xmm0
.endm

.macro salsa8_core_xmm
	salsa8_core_xmm_doubleround
	salsa8_core_xmm_doubleround
	salsa8_core_xmm_doubleround
	salsa8_core_xmm_doubleround
.endm
	
	.p2align 6
scrypt_core_xmm:
	pcmpeqw	%xmm1, %xmm1
	psrlq	$32, %xmm1
	
	movdqa	0(%rdi), %xmm8
	movdqa	16(%rdi), %xmm11
	movdqa	32(%rdi), %xmm10
	movdqa	48(%rdi), %xmm9
	movdqa	%xmm8, %xmm0
	pxor	%xmm11, %xmm8
	pand	%xmm1, %xmm8
	pxor	%xmm11, %xmm8
	pxor	%xmm10, %xmm11
	pand	%xmm1, %xmm11
	pxor	%xmm10, %xmm11
	pxor	%xmm9, %xmm10
	pand	%xmm1, %xmm10
	pxor	%xmm9, %xmm10
	pxor	%xmm0, %xmm9
	pand	%xmm1, %xmm9
	pxor	%xmm0, %xmm9
	movdqa	%xmm8, %xmm0
	pshufd	$0x4e, %xmm10, %xmm10
	punpcklqdq	%xmm10, %xmm8
	punpckhqdq	%xmm0, %xmm10
	movdqa	%xmm11, %xmm0
	pshufd	$0x4e, %xmm9, %xmm9
	punpcklqdq	%xmm9, %xmm11
	punpckhqdq	%xmm0, %xmm9
	
	movdqa	64(%rdi), %xmm12
	movdqa	80(%rdi), %xmm15
	movdqa	96(%rdi), %xmm14
	movdqa	112(%rdi), %xmm13
	movdqa	%xmm12, %xmm0
	pxor	%xmm15, %xmm12
	pand	%xmm1, %xmm12
	pxor	%xmm15, %xmm12
	pxor	%xmm14, %xmm15
	pand	%xmm1, %xmm15
	pxor	%xmm14, %xmm15
	pxor	%xmm13, %xmm14
	pand	%xmm1, %xmm14
	pxor	%xmm13, %xmm14
	pxor	%xmm0, %xmm13
	pand	%xmm1, %xmm13
	pxor	%xmm0, %xmm13
	movdqa	%xmm12, %xmm0
	pshufd	$0x4e, %xmm14, %xmm14
	punpcklqdq	%xmm14, %xmm12
	punpckhqdq	%xmm0, %xmm14
	movdqa	%xmm15, %xmm0
	pshufd	$0x4e, %xmm13, %xmm13
	punpcklqdq	%xmm13, %xmm15
	punpckhqdq	%xmm0, %xmm13
	
	movq	%rsi, %rdx
	leaq	131072(%rsi), %rcx
scrypt_core_xmm_loop1:
	pxor	%xmm12, %xmm8
	pxor	%xmm13, %xmm9
	pxor	%xmm14, %xmm10
	pxor	%xmm15, %xmm11
	movdqa	%xmm8, 0(%rdx)
	movdqa	%xmm9, 16(%rdx)
	movdqa	%xmm10, 32(%rdx)
	movdqa	%xmm11, 48(%rdx)
	movdqa	%xmm12, 64(%rdx)
	movdqa	%xmm13, 80(%rdx)
	movdqa	%xmm14, 96(%rdx)
	movdqa	%xmm15, 112(%rdx)
	
	movdqa	%xmm8, %xmm0
	movdqa	%xmm9, %xmm1
	movdqa	%xmm10, %xmm2
	movdqa	%xmm11, %xmm3
	salsa8_core_xmm
	paddd	%xmm0, %xmm8
	paddd	%xmm1, %xmm9
	paddd	%xmm2, %xmm10
	paddd	%xmm3, %xmm11
	
	pxor	%xmm8, %xmm12
	pxor	%xmm9, %xmm13
	pxor	%xmm10, %xmm14
	pxor	%xmm11, %xmm15
	movdqa	%xmm12, %xmm0
	movdqa	%xmm13, %xmm1
	movdqa	%xmm14, %xmm2
	movdqa	%xmm15, %xmm3
	salsa8_core_xmm
	paddd	%xmm0, %xmm12
	paddd	%xmm1, %xmm13
	paddd	%xmm2, %xmm14
	paddd	%xmm3, %xmm15
	
	addq	$128, %rdx
	cmpq	%rcx, %rdx
	jne scrypt_core_xmm_loop1
	
	movq	$1024, %rcx
scrypt_core_xmm_loop2:
	movd	%xmm12, %edx
	andl	$1023, %edx
	shll	$7, %edx
	pxor	0(%rsi, %rdx), %xmm8
	pxor	16(%rsi, %rdx), %xmm9
	pxor	32(%rsi, %rdx), %xmm10
	pxor	48(%rsi, %rdx), %xmm11
	
	pxor	%xmm12, %xmm8
	pxor	%xmm13, %xmm9
	pxor	%xmm14, %xmm10
	pxor	%xmm15, %xmm11
	movdqa	%xmm8, %xmm0
	movdqa	%xmm9, %xmm1
	movdqa	%xmm10, %xmm2
	movdqa	%xmm11, %xmm3
	salsa8_core_xmm
	paddd	%xmm0, %xmm8
	paddd	%xmm1, %xmm9
	paddd	%xmm2, %xmm10
	paddd	%xmm3, %xmm11
	
	pxor	64(%rsi, %rdx), %xmm12
	pxor	80(%rsi, %rdx), %xmm13
	pxor	96(%rsi, %rdx), %xmm14
	pxor	112(%rsi, %rdx), %xmm15
	pxor	%xmm8, %xmm12
	pxor	%xmm9, %xmm13
	pxor	%xmm10, %xmm14
	pxor	%xmm11, %xmm15
	movdqa	%xmm12, %xmm0
	movdqa	%xmm13, %xmm1
	movdqa	%xmm14, %xmm2
	movdqa	%xmm15, %xmm3
	salsa8_core_xmm
	paddd	%xmm0, %xmm12
	paddd	%xmm1, %xmm13
	paddd	%xmm2, %xmm14
	paddd	%xmm3, %xmm15
	
	subq	$1, %rcx
	ja scrypt_core_xmm_loop2
	
	pcmpeqw	%xmm1, %xmm1
	psrlq	$32, %xmm1
	
	movdqa	%xmm8, %xmm0
	pxor	%xmm9, %xmm8
	pand	%xmm1, %xmm8
	pxor	%xmm9, %xmm8
	pxor	%xmm10, %xmm9
	pand	%xmm1, %xmm9
	pxor	%xmm10, %xmm9
	pxor	%xmm11, %xmm10
	pand	%xmm1, %xmm10
	pxor	%xmm11, %xmm10
	pxor	%xmm0, %xmm11
	pand	%xmm1, %xmm11
	pxor	%xmm0, %xmm11
	movdqa	%xmm8, %xmm0
	pshufd	$0x4e, %xmm10, %xmm10
	punpcklqdq	%xmm10, %xmm8
	punpckhqdq	%xmm0, %xmm10
	movdqa	%xmm9, %xmm0
	pshufd	$0x4e, %xmm11, %xmm11
	punpcklqdq	%xmm11, %xmm9
	punpckhqdq	%xmm0, %xmm11
	movdqa	%xmm8, 0(%rdi)
	movdqa	%xmm11, 16(%rdi)
	movdqa	%xmm10, 32(%rdi)
	movdqa	%xmm9, 48(%rdi)
	
	movdqa	%xmm12, %xmm0
	pxor	%xmm13, %xmm12
	pand	%xmm1, %xmm12
	pxor	%xmm13, %xmm12
	pxor	%xmm14, %xmm13
	pand	%xmm1, %xmm13
	pxor	%xmm14, %xmm13
	pxor	%xmm15, %xmm14
	pand	%xmm1, %xmm14
	pxor	%xmm15, %xmm14
	pxor	%xmm0, %xmm15
	pand	%xmm1, %xmm15
	pxor	%xmm0, %xmm15
	movdqa	%xmm12, %xmm0
	pshufd	$0x4e, %xmm14, %xmm14
	punpcklqdq	%xmm14, %xmm12
	punpckhqdq	%xmm0, %xmm14
	movdqa	%xmm13, %xmm0
	pshufd	$0x4e, %xmm15, %xmm15
	punpcklqdq	%xmm15, %xmm13
	punpckhqdq	%xmm0, %xmm15
	movdqa	%xmm12, 64(%rdi)
	movdqa	%xmm15, 80(%rdi)
	movdqa	%xmm14, 96(%rdi)
	movdqa	%xmm13, 112(%rdi)
	
	scrypt_core_cleanup
	ret

#endif
