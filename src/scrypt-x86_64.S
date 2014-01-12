# Copyright 2011-2012 pooler@litecoinpool.org
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.


#if defined(OPTIMIZED_SALSA) && defined(__x86_64__)

#if defined(__linux__) && defined(__ELF__)
	.section .note.GNU-stack,"",%progbits
#endif

#define scrypt_shuffle(src, so, dest, do) \
	movl	so+60(src), %r8d; \
	movl	so+44(src), %r9d; \
	movl	so+28(src), %r10d; \
	movl	so+12(src), %r11d; \
	movl	%r8d, do+12(dest); \
	movl	%r9d, do+28(dest); \
	movl	%r10d, do+44(dest); \
	movl	%r11d, do+60(dest); \
	movl	so+40(src), %r8d; \
	movl	so+8(src), %r9d; \
	movl	so+48(src), %r10d; \
	movl	so+16(src), %r11d; \
	movl	%r8d, do+8(dest); \
	movl	%r9d, do+40(dest); \
	movl	%r10d, do+16(dest); \
	movl	%r11d, do+48(dest); \
	movl	so+20(src), %r8d; \
	movl	so+4(src), %r9d; \
	movl	so+52(src), %r10d; \
	movl	so+36(src), %r11d; \
	movl	%r8d, do+4(dest); \
	movl	%r9d, do+20(dest); \
	movl	%r10d, do+36(dest); \
	movl	%r11d, do+52(dest); \
	movl	so+0(src), %r8d; \
	movl	so+24(src), %r9d; \
	movl	so+32(src), %r10d; \
	movl	so+56(src), %r11d; \
	movl	%r8d, do+0(dest); \
	movl	%r9d, do+24(dest); \
	movl	%r10d, do+32(dest); \
	movl	%r11d, do+56(dest); \


#define gen_salsa8_core_doubleround() \
	movq	72(%rsp), %r15; \
	leaq	(%r14, %rdx), %rbp; \
	roll	$7, %ebp; \
	xorq	%rbp, %r9; \
	leaq	(%rdi, %r15), %rbp; \
	roll	$7, %ebp; \
	xorq	%rbp, %r10; \
	leaq	(%rdx, %r9), %rbp; \
	roll	$9, %ebp; \
	xorq	%rbp, %r11; \
	leaq	(%r15, %r10), %rbp; \
	roll	$9, %ebp; \
	xorq	%rbp, %r13; \
	leaq	(%r9, %r11), %rbp; \
	roll	$13, %ebp; \
	xorq	%rbp, %r14; \
	leaq	(%r10, %r13), %rbp; \
	roll	$13, %ebp; \
	xorq	%rbp, %rdi; \
	leaq	(%r11, %r14), %rbp; \
	roll	$18, %ebp; \
	xorq	%rbp, %rdx; \
	leaq	(%r13, %rdi), %rbp; \
	roll	$18, %ebp; \
	xorq	%rbp, %r15; \
	movq	48(%rsp), %rbp; \
	movq	%r15, 72(%rsp); \
	leaq	(%rax, %rbp), %r15; \
	roll	$7, %r15d; \
	xorq	%r15, %rbx; \
	leaq	(%rbp, %rbx), %r15; \
	roll	$9, %r15d; \
	xorq	%r15, %rcx; \
	leaq	(%rbx, %rcx), %r15; \
	roll	$13, %r15d; \
	xorq	%r15, %rax; \
	leaq	(%rcx, %rax), %r15; \
	roll	$18, %r15d; \
	xorq	%r15, %rbp; \
	movq	88(%rsp), %r15; \
	movq	%rbp, 48(%rsp); \
	leaq	(%r12, %r15), %rbp; \
	roll	$7, %ebp; \
	xorq	%rbp, %rsi; \
	leaq	(%r15, %rsi), %rbp; \
	roll	$9, %ebp; \
	xorq	%rbp, %r8; \
	leaq	(%rsi, %r8), %rbp; \
	roll	$13, %ebp; \
	xorq	%rbp, %r12; \
	leaq	(%r8, %r12), %rbp; \
	roll	$18, %ebp; \
	xorq	%rbp, %r15; \
	movq	%r15, 88(%rsp); \
	movq	72(%rsp), %r15; \
	leaq	(%rsi, %rdx), %rbp; \
	roll	$7, %ebp; \
	xorq	%rbp, %rdi; \
	leaq	(%r9, %r15), %rbp; \
	roll	$7, %ebp; \
	xorq	%rbp, %rax; \
	leaq	(%rdx, %rdi), %rbp; \
	roll	$9, %ebp; \
	xorq	%rbp, %rcx; \
	leaq	(%r15, %rax), %rbp; \
	roll	$9, %ebp; \
	xorq	%rbp, %r8; \
	leaq	(%rdi, %rcx), %rbp; \
	roll	$13, %ebp; \
	xorq	%rbp, %rsi; \
	leaq	(%rax, %r8), %rbp; \
	roll	$13, %ebp; \
	xorq	%rbp, %r9; \
	leaq	(%rcx, %rsi), %rbp; \
	roll	$18, %ebp; \
	xorq	%rbp, %rdx; \
	leaq	(%r8, %r9), %rbp; \
	roll	$18, %ebp; \
	xorq	%rbp, %r15; \
	movq	48(%rsp), %rbp; \
	movq	%r15, 72(%rsp); \
	leaq	(%r10, %rbp), %r15; \
	roll	$7, %r15d; \
	xorq	%r15, %r12; \
	leaq	(%rbp, %r12), %r15; \
	roll	$9, %r15d; \
	xorq	%r15, %r11; \
	leaq	(%r12, %r11), %r15; \
	roll	$13, %r15d; \
	xorq	%r15, %r10; \
	leaq	(%r11, %r10), %r15; \
	roll	$18, %r15d; \
	xorq	%r15, %rbp; \
	movq	88(%rsp), %r15; \
	movq	%rbp, 48(%rsp); \
	leaq	(%rbx, %r15), %rbp; \
	roll	$7, %ebp; \
	xorq	%rbp, %r14; \
	leaq	(%r15, %r14), %rbp; \
	roll	$9, %ebp; \
	xorq	%rbp, %r13; \
	leaq	(%r14, %r13), %rbp; \
	roll	$13, %ebp; \
	xorq	%rbp, %rbx; \
	leaq	(%r13, %rbx), %rbp; \
	roll	$18, %ebp; \
	xorq	%rbp, %r15; \
	movq	%r15, 88(%rsp); \


	.text
	.align 32
gen_salsa8_core:
	# 0: %rdx, %rdi, %rcx, %rsi
	movq	8(%rsp), %rdi
	movq	%rdi, %rdx
	shrq	$32, %rdi
	movq	16(%rsp), %rsi
	movq	%rsi, %rcx
	shrq	$32, %rsi
	# 1: %r9, 72(%rsp), %rax, %r8
	movq	24(%rsp), %r8
	movq	%r8, %r9
	shrq	$32, %r8
	movq	%r8, 72(%rsp)
	movq	32(%rsp), %r8
	movq	%r8, %rax
	shrq	$32, %r8
	# 2: %r11, %r10, 48(%rsp), %r12
	movq	40(%rsp), %r10
	movq	%r10, %r11
	shrq	$32, %r10
	movq	48(%rsp), %r12
	#movq	%r12, %r13
	#movq	%r13, 48(%rsp)
	shrq	$32, %r12
	# 3: %r14, %r13, %rbx, 88(%rsp)
	movq	56(%rsp), %r13
	movq	%r13, %r14
	shrq	$32, %r13
	movq	64(%rsp), %r15
	movq	%r15, %rbx
	shrq	$32, %r15
	movq	%r15, 88(%rsp)

	gen_salsa8_core_doubleround()
	gen_salsa8_core_doubleround()
	gen_salsa8_core_doubleround()
	gen_salsa8_core_doubleround()

	movl	%edx, %edx
	shlq	$32, %rdi
	addq	%rdi, %rdx
	movd	%rdx, %xmm0

	movl	%ecx, %ecx
	shlq	$32, %rsi
	addq	%rsi, %rcx
	movd	%rcx, %xmm4

	movq	72(%rsp), %rdi
	movl	%r9d, %r9d
	shlq	$32, %rdi
	addq	%rdi, %r9
	movd	%r9, %xmm1

	movl	%eax, %eax
	shlq	$32, %r8
	addq	%r8, %rax
	movd	%rax, %xmm5

	movl	%r11d, %r11d
	shlq	$32, %r10
	addq	%r10, %r11
	movd	%r11, %xmm2

	movl	48(%rsp), %r8d
	shlq	$32, %r12
	addq	%r12, %r8
	movd	%r8, %xmm6

	movl	%r14d, %r14d
	shlq	$32, %r13
	addq	%r13, %r14
	movd	%r14, %xmm3

	movq	88(%rsp), %rdi
	movl	%ebx, %ebx
	shlq	$32, %rdi
	addq	%rdi, %rbx
	movd	%rbx, %xmm7

	punpcklqdq	%xmm4, %xmm0
	punpcklqdq	%xmm5, %xmm1
	punpcklqdq	%xmm6, %xmm2
	punpcklqdq	%xmm7, %xmm3

	#movq	%rdx, 8(%rsp)
	#movq	%rcx, 16(%rsp)
	#movq	%r9, 24(%rsp)
	#movq	%rax, 32(%rsp)
	#movq	%r11, 40(%rsp)
	#movq	%r8, 48(%rsp)
	#movq	%r14, 56(%rsp)
	#movq	%rbx, 64(%rsp)

	ret


	.text
	.align 32
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

#define scrypt_core_cleanup() \
	popq	%r15; \
	popq	%r14; \
	popq	%r13; \
	popq	%r12; \
	popq	%rbp; \
	popq	%rbx; \


	# GenuineIntel processors have fast SIMD
	xorl	%eax, %eax
	cpuid
	cmpl	$0x6c65746e, %ecx
	jne gen_scrypt_core
	cmpl	$0x49656e69, %edx
	jne gen_scrypt_core
	cmpl	$0x756e6547, %ebx
	je xmm_scrypt_core

gen_scrypt_core:
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
gen_scrypt_core_loop1:
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
	call gen_salsa8_core
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
	call gen_salsa8_core
	movq	128(%rsp), %rsi
	paddd	%xmm0, %xmm12
	paddd	%xmm1, %xmm13
	paddd	%xmm2, %xmm14
	paddd	%xmm3, %xmm15

	addq	$128, %rsi
	movq	120(%rsp), %rcx
	cmpq	%rcx, %rsi
	jne gen_scrypt_core_loop1

	movq	$1024, %rcx
gen_scrypt_core_loop2:
	movq	112(%rsp), %rsi
	movd	%xmm12, %edx
	andl	$1023, %edx
	shll	$7, %edx
	movdqa	0(%rsi, %rdx), %xmm0
	movdqa	16(%rsi, %rdx), %xmm1
	movdqa	32(%rsi, %rdx), %xmm2
	movdqa	48(%rsi, %rdx), %xmm3
	movdqa	64(%rsi, %rdx), %xmm4
	movdqa	80(%rsi, %rdx), %xmm5
	movdqa	96(%rsi, %rdx), %xmm6
	movdqa	112(%rsi, %rdx), %xmm7
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
	call gen_salsa8_core
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
	call gen_salsa8_core
	movq	128(%rsp), %rcx
	paddd	%xmm0, %xmm12
	paddd	%xmm1, %xmm13
	paddd	%xmm2, %xmm14
	paddd	%xmm3, %xmm15

	subq	$1, %rcx
	ja gen_scrypt_core_loop2

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
	scrypt_core_cleanup()
	ret


#define xmm_salsa8_core_doubleround() \
	movdqa	%xmm1, %xmm4; \
	paddd	%xmm0, %xmm4; \
	movdqa	%xmm4, %xmm5; \
	pslld	$7, %xmm4; \
	psrld	$25, %xmm5; \
	pxor	%xmm4, %xmm3; \
	pxor	%xmm5, %xmm3; \
	movdqa	%xmm0, %xmm4; \
	paddd	%xmm3, %xmm4; \
	movdqa	%xmm4, %xmm5; \
	pslld	$9, %xmm4; \
	psrld	$23, %xmm5; \
	pxor	%xmm4, %xmm2; \
	movdqa	%xmm3, %xmm4; \
	pshufd	$0x93, %xmm3, %xmm3; \
	pxor	%xmm5, %xmm2; \
	paddd	%xmm2, %xmm4; \
	movdqa	%xmm4, %xmm5; \
	pslld	$13, %xmm4; \
	psrld	$19, %xmm5; \
	pxor	%xmm4, %xmm1; \
	movdqa	%xmm2, %xmm4; \
	pshufd	$0x4e, %xmm2, %xmm2; \
	pxor	%xmm5, %xmm1; \
	paddd	%xmm1, %xmm4; \
	movdqa	%xmm4, %xmm5; \
	pslld	$18, %xmm4; \
	psrld	$14, %xmm5; \
	pxor	%xmm4, %xmm0; \
	pshufd	$0x39, %xmm1, %xmm1; \
	pxor	%xmm5, %xmm0; \
	movdqa	%xmm3, %xmm4; \
	paddd	%xmm0, %xmm4; \
	movdqa	%xmm4, %xmm5; \
	pslld	$7, %xmm4; \
	psrld	$25, %xmm5; \
	pxor	%xmm4, %xmm1; \
	pxor	%xmm5, %xmm1; \
	movdqa	%xmm0, %xmm4; \
	paddd	%xmm1, %xmm4; \
	movdqa	%xmm4, %xmm5; \
	pslld	$9, %xmm4; \
	psrld	$23, %xmm5; \
	pxor	%xmm4, %xmm2; \
	movdqa	%xmm1, %xmm4; \
	pshufd	$0x93, %xmm1, %xmm1; \
	pxor	%xmm5, %xmm2; \
	paddd	%xmm2, %xmm4; \
	movdqa	%xmm4, %xmm5; \
	pslld	$13, %xmm4; \
	psrld	$19, %xmm5; \
	pxor	%xmm4, %xmm3; \
	movdqa	%xmm2, %xmm4; \
	pshufd	$0x4e, %xmm2, %xmm2; \
	pxor	%xmm5, %xmm3; \
	paddd	%xmm3, %xmm4; \
	movdqa	%xmm4, %xmm5; \
	pslld	$18, %xmm4; \
	psrld	$14, %xmm5; \
	pxor	%xmm4, %xmm0; \
	pshufd	$0x39, %xmm3, %xmm3; \
	pxor	%xmm5, %xmm0; \


#define xmm_salsa8_core() \
	xmm_salsa8_core_doubleround(); \
	xmm_salsa8_core_doubleround(); \
	xmm_salsa8_core_doubleround(); \
	xmm_salsa8_core_doubleround(); \


	.align 32
xmm_scrypt_core:
	# shuffle 1st block into %xmm8-%xmm11
	movl	60(%rdi), %edx
	movl	44(%rdi), %ecx
	movl	28(%rdi), %ebx
	movl	12(%rdi), %eax
	movd	%edx, %xmm0
	movd	%ecx, %xmm1
	movd	%ebx, %xmm2
	movd	%eax, %xmm3
	movl	40(%rdi), %ecx
	movl	24(%rdi), %ebx
	movl	8(%rdi), %eax
	movl	56(%rdi), %edx
	pshufd	$0x93, %xmm0, %xmm0
	pshufd	$0x93, %xmm1, %xmm1
	pshufd	$0x93, %xmm2, %xmm2
	pshufd	$0x93, %xmm3, %xmm3
	movd	%ecx, %xmm4
	movd	%ebx, %xmm5
	movd	%eax, %xmm6
	movd	%edx, %xmm7
	paddd	%xmm4, %xmm0
	paddd	%xmm5, %xmm1
	paddd	%xmm6, %xmm2
	paddd	%xmm7, %xmm3
	movl	20(%rdi), %ebx
	movl	4(%rdi), %eax
	movl	52(%rdi), %edx
	movl	36(%rdi), %ecx
	pshufd	$0x93, %xmm0, %xmm0
	pshufd	$0x93, %xmm1, %xmm1
	pshufd	$0x93, %xmm2, %xmm2
	pshufd	$0x93, %xmm3, %xmm3
	movd	%ebx, %xmm4
	movd	%eax, %xmm5
	movd	%edx, %xmm6
	movd	%ecx, %xmm7
	paddd	%xmm4, %xmm0
	paddd	%xmm5, %xmm1
	paddd	%xmm6, %xmm2
	paddd	%xmm7, %xmm3
	movl	0(%rdi), %eax
	movl	48(%rdi), %edx
	movl	32(%rdi), %ecx
	movl	16(%rdi), %ebx
	pshufd	$0x93, %xmm0, %xmm0
	pshufd	$0x93, %xmm1, %xmm1
	pshufd	$0x93, %xmm2, %xmm2
	pshufd	$0x93, %xmm3, %xmm3
	movd	%eax, %xmm8
	movd	%edx, %xmm9
	movd	%ecx, %xmm10
	movd	%ebx, %xmm11
	paddd	%xmm0, %xmm8
	paddd	%xmm1, %xmm9
	paddd	%xmm2, %xmm10
	paddd	%xmm3, %xmm11

	# shuffle 2nd block into %xmm12-%xmm15
	movl	124(%rdi), %edx
	movl	108(%rdi), %ecx
	movl	92(%rdi), %ebx
	movl	76(%rdi), %eax
	movd	%edx, %xmm0
	movd	%ecx, %xmm1
	movd	%ebx, %xmm2
	movd	%eax, %xmm3
	movl	104(%rdi), %ecx
	movl	88(%rdi), %ebx
	movl	72(%rdi), %eax
	movl	120(%rdi), %edx
	pshufd	$0x93, %xmm0, %xmm0
	pshufd	$0x93, %xmm1, %xmm1
	pshufd	$0x93, %xmm2, %xmm2
	pshufd	$0x93, %xmm3, %xmm3
	movd	%ecx, %xmm4
	movd	%ebx, %xmm5
	movd	%eax, %xmm6
	movd	%edx, %xmm7
	paddd	%xmm4, %xmm0
	paddd	%xmm5, %xmm1
	paddd	%xmm6, %xmm2
	paddd	%xmm7, %xmm3
	movl	84(%rdi), %ebx
	movl	68(%rdi), %eax
	movl	116(%rdi), %edx
	movl	100(%rdi), %ecx
	pshufd	$0x93, %xmm0, %xmm0
	pshufd	$0x93, %xmm1, %xmm1
	pshufd	$0x93, %xmm2, %xmm2
	pshufd	$0x93, %xmm3, %xmm3
	movd	%ebx, %xmm4
	movd	%eax, %xmm5
	movd	%edx, %xmm6
	movd	%ecx, %xmm7
	paddd	%xmm4, %xmm0
	paddd	%xmm5, %xmm1
	paddd	%xmm6, %xmm2
	paddd	%xmm7, %xmm3
	movl	64(%rdi), %eax
	movl	112(%rdi), %edx
	movl	96(%rdi), %ecx
	movl	80(%rdi), %ebx
	pshufd	$0x93, %xmm0, %xmm0
	pshufd	$0x93, %xmm1, %xmm1
	pshufd	$0x93, %xmm2, %xmm2
	pshufd	$0x93, %xmm3, %xmm3
	movd	%eax, %xmm12
	movd	%edx, %xmm13
	movd	%ecx, %xmm14
	movd	%ebx, %xmm15
	paddd	%xmm0, %xmm12
	paddd	%xmm1, %xmm13
	paddd	%xmm2, %xmm14
	paddd	%xmm3, %xmm15

	movq	%rsi, %rdx
	leaq	131072(%rsi), %rcx
xmm_scrypt_core_loop1:
	movdqa	%xmm8, 0(%rdx)
	movdqa	%xmm9, 16(%rdx)
	movdqa	%xmm10, 32(%rdx)
	movdqa	%xmm11, 48(%rdx)
	movdqa	%xmm12, 64(%rdx)
	movdqa	%xmm13, 80(%rdx)
	movdqa	%xmm14, 96(%rdx)
	movdqa	%xmm15, 112(%rdx)

	pxor	%xmm12, %xmm8
	pxor	%xmm13, %xmm9
	pxor	%xmm14, %xmm10
	pxor	%xmm15, %xmm11
	movdqa	%xmm8, %xmm0
	movdqa	%xmm9, %xmm1
	movdqa	%xmm10, %xmm2
	movdqa	%xmm11, %xmm3
	xmm_salsa8_core()
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
	xmm_salsa8_core()
	paddd	%xmm0, %xmm12
	paddd	%xmm1, %xmm13
	paddd	%xmm2, %xmm14
	paddd	%xmm3, %xmm15

	addq	$128, %rdx
	cmpq	%rcx, %rdx
	jne xmm_scrypt_core_loop1

	movq	$1024, %rcx
xmm_scrypt_core_loop2:
	movd	%xmm12, %edx
	andl	$1023, %edx
	shll	$7, %edx
	movdqa	0(%rsi, %rdx), %xmm0
	movdqa	16(%rsi, %rdx), %xmm1
	movdqa	32(%rsi, %rdx), %xmm2
	movdqa	48(%rsi, %rdx), %xmm3
	movdqa	64(%rsi, %rdx), %xmm4
	movdqa	80(%rsi, %rdx), %xmm5
	movdqa	96(%rsi, %rdx), %xmm6
	movdqa	112(%rsi, %rdx), %xmm7
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
	movdqa	%xmm8, %xmm0
	movdqa	%xmm9, %xmm1
	movdqa	%xmm10, %xmm2
	movdqa	%xmm11, %xmm3
	xmm_salsa8_core()
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
	xmm_salsa8_core()
	paddd	%xmm0, %xmm12
	paddd	%xmm1, %xmm13
	paddd	%xmm2, %xmm14
	paddd	%xmm3, %xmm15

	subq	$1, %rcx
	ja xmm_scrypt_core_loop2

	# re-shuffle 1st block back
	movd	%xmm8, %eax
	movd	%xmm9, %edx
	movd	%xmm10, %ecx
	movd	%xmm11, %ebx
	pshufd	$0x39, %xmm8, %xmm8
	pshufd	$0x39, %xmm9, %xmm9
	pshufd	$0x39, %xmm10, %xmm10
	pshufd	$0x39, %xmm11, %xmm11
	movl	%eax, 0(%rdi)
	movl	%edx, 48(%rdi)
	movl	%ecx, 32(%rdi)
	movl	%ebx, 16(%rdi)
	movd	%xmm8, %ebx
	movd	%xmm9, %eax
	movd	%xmm10, %edx
	movd	%xmm11, %ecx
	pshufd	$0x39, %xmm8, %xmm8
	pshufd	$0x39, %xmm9, %xmm9
	pshufd	$0x39, %xmm10, %xmm10
	pshufd	$0x39, %xmm11, %xmm11
	movl	%ebx, 20(%rdi)
	movl	%eax, 4(%rdi)
	movl	%edx, 52(%rdi)
	movl	%ecx, 36(%rdi)
	movd	%xmm8, %ecx
	movd	%xmm9, %ebx
	movd	%xmm10, %eax
	movd	%xmm11, %edx
	pshufd	$0x39, %xmm8, %xmm8
	pshufd	$0x39, %xmm9, %xmm9
	pshufd	$0x39, %xmm10, %xmm10
	pshufd	$0x39, %xmm11, %xmm11
	movl	%ecx, 40(%rdi)
	movl	%ebx, 24(%rdi)
	movl	%eax, 8(%rdi)
	movl	%edx, 56(%rdi)
	movd	%xmm8, %edx
	movd	%xmm9, %ecx
	movd	%xmm10, %ebx
	movd	%xmm11, %eax
	movl	%edx, 60(%rdi)
	movl	%ecx, 44(%rdi)
	movl	%ebx, 28(%rdi)
	movl	%eax, 12(%rdi)

	# re-shuffle 2nd block back
	movd	%xmm12, %eax
	movd	%xmm13, %edx
	movd	%xmm14, %ecx
	movd	%xmm15, %ebx
	pshufd	$0x39, %xmm12, %xmm12
	pshufd	$0x39, %xmm13, %xmm13
	pshufd	$0x39, %xmm14, %xmm14
	pshufd	$0x39, %xmm15, %xmm15
	movl	%eax, 64(%rdi)
	movl	%edx, 112(%rdi)
	movl	%ecx, 96(%rdi)
	movl	%ebx, 80(%rdi)
	movd	%xmm12, %ebx
	movd	%xmm13, %eax
	movd	%xmm14, %edx
	movd	%xmm15, %ecx
	pshufd	$0x39, %xmm12, %xmm12
	pshufd	$0x39, %xmm13, %xmm13
	pshufd	$0x39, %xmm14, %xmm14
	pshufd	$0x39, %xmm15, %xmm15
	movl	%ebx, 84(%rdi)
	movl	%eax, 68(%rdi)
	movl	%edx, 116(%rdi)
	movl	%ecx, 100(%rdi)
	movd	%xmm12, %ecx
	movd	%xmm13, %ebx
	movd	%xmm14, %eax
	movd	%xmm15, %edx
	pshufd	$0x39, %xmm12, %xmm12
	pshufd	$0x39, %xmm13, %xmm13
	pshufd	$0x39, %xmm14, %xmm14
	pshufd	$0x39, %xmm15, %xmm15
	movl	%ecx, 104(%rdi)
	movl	%ebx, 88(%rdi)
	movl	%eax, 72(%rdi)
	movl	%edx, 120(%rdi)
	movd	%xmm12, %edx
	movd	%xmm13, %ecx
	movd	%xmm14, %ebx
	movd	%xmm15, %eax
	movl	%edx, 124(%rdi)
	movl	%ecx, 108(%rdi)
	movl	%ebx, 92(%rdi)
	movl	%eax, 76(%rdi)

	scrypt_core_cleanup()
	ret

	addq	$392, %rsp
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
	popq	%rbp
	popq	%rbx
	ret

#endif