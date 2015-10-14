/*
 * Copyright 2012-2015 pooler@litecoinpool.org
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#if defined(__linux__) && defined(__ELF__)
    .section .note.GNU-stack,"",%progbits
#endif

#if defined(__x86_64__)
    .data
    .p2align 4
sha256_h:
    .long 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a
    .long 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19

    .data
    .p2align 6
sha256_k:
    .long 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5
    .long 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5
    .long 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3
    .long 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174
    .long 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc
    .long 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da
    .long 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7
    .long 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967
    .long 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13
    .long 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85
    .long 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3
    .long 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070
    .long 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5
    .long 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3
    .long 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208
    .long 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2

bswap_xmm_mask:
    .long 0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f


.macro sha256_mixed_quadround ra, rb, rc, rd, re, rf, rg, rh, x0, x1, x2, x3
    movdqa    \x3, %xmm4
    movl    \re, %eax
    movdqa    \x2, %xmm6
    rorl    $(25-11), %eax
    movl    \ra, %ebx
    pslldq    $12, %xmm4
    rorl    $(22-13), %ebx
    psrldq    $4, %xmm6
    xorl    \re, %eax
    movl    \rf, %ecx
    rorl    $(11-6), %eax
    pxor    %xmm6, %xmm4
    movdqa    \x1, %xmm5
    xorl    \ra, %ebx
    xorl    \rg, %ecx
    xorl    \re, %eax
    paddd    \x0, %xmm4
    movdqa    \x0, %xmm7
    andl    \re, %ecx
    rorl    $(13-2), %ebx
    xorl    \ra, %ebx
    pslldq    $12, %xmm5
    psrldq    $4, %xmm7
    rorl    $6, %eax
    xorl    \rg, %ecx
    pxor    %xmm7, %xmm5
    rorl    $2, %ebx
    addl    %eax, %ecx
    addl    (%rsp) , %ecx
    movdqa    %xmm5, %xmm6
    movl    \ra, %eax
    addl    %ecx, \rh
    movl    \ra, %ecx
    movdqa    %xmm5, %xmm7
    orl    \rc, %eax
    addl    \rh, \rd
    andl    \rc, %ecx
    pslld    $(32-7), %xmm5
    psrld    $7, %xmm6
    andl    \rb, %eax
    addl    %ebx, \rh
    orl    %ecx, %eax
    por    %xmm6, %xmm5
    addl    %eax, \rh
    
    movl    \rd, %eax
    movdqa    %xmm7, %xmm6
    movl    \rh, %ebx
    rorl    $(25-11), %eax
    xorl    \rd, %eax
    movdqa    %xmm7, %xmm8
    movl    \re, %ecx
    rorl    $(22-13), %ebx
    xorl    \rh, %ebx
    pslld    $(32-18), %xmm7
    rorl    $(11-6), %eax
    xorl    \rf, %ecx
    rorl    $(13-2), %ebx
    psrld    $18, %xmm6
    xorl    \rd, %eax
    andl    \rd, %ecx
    rorl    $6, %eax
    pxor    %xmm7, %xmm5
    xorl    \rh, %ebx
    xorl    \rf, %ecx
    psrld    $3, %xmm8
    addl    %eax, %ecx
    addl    1*4(%rsp), %ecx
    rorl    $2, %ebx
    pxor    %xmm6, %xmm5
    movl    \rh, %eax
    addl    %ecx, \rg
    movl    \rh, %ecx
    pxor    %xmm8, %xmm5
    orl    \rb, %eax
    addl    \rg, \rc
    andl    \rb, %ecx
    pshufd    $0xfa, \x3, %xmm6
    andl    \ra, %eax
    addl    %ebx, \rg
    paddd    %xmm5, %xmm4
    orl    %ecx, %eax
    addl    %eax, \rg
    
    movl    \rc, %eax
    movdqa    %xmm6, %xmm7
    movl    \rg, %ebx
    rorl    $(25-11), %eax
    xorl    \rc, %eax
    movdqa    %xmm6, %xmm8
    rorl    $(22-13), %ebx
    movl    \rd, %ecx
    xorl    \rg, %ebx
    psrlq    $17, %xmm6
    psrlq    $19, %xmm7
    rorl    $(11-6), %eax
    xorl    \re, %ecx
    xorl    \rc, %eax
    psrld    $10, %xmm8
    pxor    %xmm7, %xmm6
    andl    \rc, %ecx
    rorl    $(13-2), %ebx
    xorl    \rg, %ebx
    pxor    %xmm6, %xmm8
    xorl    \re, %ecx
    rorl    $6, %eax
    addl    %eax, %ecx
    pshufd    $0x8f, %xmm8, %xmm8
    rorl    $2, %ebx
    addl    2*4(%rsp), %ecx
    movl    \rg, %eax
    psrldq    $8, %xmm8
    addl    %ecx, \rf
    movl    \rg, %ecx
    orl    \ra, %eax
    paddd    %xmm8, %xmm4
    addl    \rf, \rb
    andl    \ra, %ecx
    andl    \rh, %eax
    pshufd    $0x50, %xmm4, %xmm6
    addl    %ebx, \rf
    orl    %ecx, %eax
    addl    %eax, \rf
    
    movdqa    %xmm6, %xmm7
    movl    \rb, %eax
    rorl    $(25-11), %eax
    movl    \rf, %ebx
    movdqa    %xmm6, \x0
    rorl    $(22-13), %ebx
    xorl    \rb, %eax
    movl    \rc, %ecx
    psrlq    $17, %xmm6
    rorl    $(11-6), %eax
    xorl    \rf, %ebx
    xorl    \rd, %ecx
    psrlq    $19, %xmm7
    xorl    \rb, %eax
    andl    \rb, %ecx
    rorl    $(13-2), %ebx
    psrld    $10, \x0
    xorl    \rf, %ebx
    rorl    $6, %eax
    pxor    %xmm7, %xmm6
    xorl    \rd, %ecx
    rorl    $2, %ebx
    addl    %eax, %ecx
    pxor    %xmm6, \x0
    addl    3*4(%rsp), %ecx
    movl    \rf, %eax
    addl    %ecx, \re
    pshufd    $0xf8, \x0, \x0
    movl    \rf, %ecx
    orl    \rh, %eax
    addl    \re, \ra
    pslldq    $8, \x0
    andl    \rh, %ecx
    andl    \rg, %eax
    paddd    %xmm4, \x0
    addl    %ebx, \re
    orl    %ecx, %eax
    addl    %eax, \re
.endm

.macro sha256_main_round i, ra, rb, rc, rd, re, rf, rg, rh
    movl    \re, %eax
    rorl    $(25-11), %eax
    movl    \ra, %ebx
    xorl    \re, %eax
    rorl    $(22-13), %ebx
    movl    \rf, %ecx
    xorl    \ra, %ebx
    rorl    $(11-6), %eax
    xorl    \rg, %ecx
    xorl    \re, %eax
    rorl    $(13-2), %ebx
    andl    \re, %ecx
    xorl    \ra, %ebx
    rorl    $6, %eax
    xorl    \rg, %ecx
    addl    %eax, %ecx
    rorl    $2, %ebx
    addl    \i*4(%rsp), %ecx
    movl    \ra, %eax
    addl    %ecx, \rh
    movl    \ra, %ecx
    orl    \rc, %eax
    addl    \rh, \rd
    andl    \rc, %ecx
    andl    \rb, %eax
    addl    %ebx, \rh
    orl    %ecx, %eax
    addl    %eax, \rh
.endm


    .text
    .p2align 6
sha256_transform_sse2:
    pushq    %rbx
    pushq    %r12
    pushq    %r13
    pushq    %r14
    pushq    %r15
#if defined(_WIN64) || defined(__CYGWIN__)
    pushq    %rdi
    pushq    %rsi
    subq    $5*16, %rsp
    movdqa    %xmm6, 1*16(%rsp)
    movdqa    %xmm7, 2*16(%rsp)
    movdqa    %xmm8, 3*16(%rsp)
    movdqa    %xmm9, 4*16(%rsp)
    movq    %rcx, %rdi
    movq    %rdx, %rsi
    movq    %r8, %rdx
#else
    subq    $16, %rsp
#endif
    
    movl    0*4(%rdi), %r8d
    movl    1*4(%rdi), %r9d
    movl    2*4(%rdi), %r10d
    movl    3*4(%rdi), %r11d
    movl    4*4(%rdi), %r12d
    movl    5*4(%rdi), %r13d
    movl    6*4(%rdi), %r14d
    movl    7*4(%rdi), %r15d
    
    testq    %rdx, %rdx
    jnz sha256_transform_sse2_swap
    
    movdqu    0*16(%rsi), %xmm0
    movdqu    1*16(%rsi), %xmm1
    movdqu    2*16(%rsi), %xmm2
    movdqu    3*16(%rsi), %xmm3
    jmp sha256_transform_sse2_core
    
sha256_transform_sse2_swap:
    movdqu    0*16(%rsi), %xmm0
    movdqu    1*16(%rsi), %xmm1
    movdqu    2*16(%rsi), %xmm2
    movdqu    3*16(%rsi), %xmm3
    pshuflw    $0xb1, %xmm0, %xmm0
    pshuflw    $0xb1, %xmm1, %xmm1
    pshuflw    $0xb1, %xmm2, %xmm2
    pshuflw    $0xb1, %xmm3, %xmm3
    pshufhw    $0xb1, %xmm0, %xmm0
    pshufhw    $0xb1, %xmm1, %xmm1
    pshufhw    $0xb1, %xmm2, %xmm2
    pshufhw    $0xb1, %xmm3, %xmm3
    movdqa    %xmm0, %xmm4
    movdqa    %xmm1, %xmm5
    movdqa    %xmm2, %xmm6
    movdqa    %xmm3, %xmm7
    psrlw    $8, %xmm4
    psrlw    $8, %xmm5
    psrlw    $8, %xmm6
    psrlw    $8, %xmm7
    psllw    $8, %xmm0
    psllw    $8, %xmm1
    psllw    $8, %xmm2
    psllw    $8, %xmm3
    pxor    %xmm4, %xmm0
    pxor    %xmm5, %xmm1
    pxor    %xmm6, %xmm2
    pxor    %xmm7, %xmm3
    
sha256_transform_sse2_core:
    leaq    sha256_k(%rip), %rdx
    movq    $48, %rsi
    .p2align 4
sha256_transform_sse2_loop:
    movdqa    0*16(%rdx), %xmm9
    paddd    %xmm0, %xmm9
    movdqa    %xmm9, (%rsp)
    sha256_mixed_quadround %r8d, %r9d, %r10d, %r11d, %r12d, %r13d, %r14d, %r15d, %xmm0, %xmm1, %xmm2, %xmm3
    movdqa    1*16(%rdx), %xmm9
    paddd    %xmm1, %xmm9
    movdqa    %xmm9, (%rsp)
    sha256_mixed_quadround %r12d, %r13d, %r14d, %r15d, %r8d, %r9d, %r10d, %r11d, %xmm1, %xmm2, %xmm3, %xmm0
    movdqa    2*16(%rdx), %xmm9
    paddd    %xmm2, %xmm9
    movdqa    %xmm9, (%rsp)
    sha256_mixed_quadround %r8d, %r9d, %r10d, %r11d, %r12d, %r13d, %r14d, %r15d, %xmm2, %xmm3, %xmm0, %xmm1
    movdqa    3*16(%rdx), %xmm9
    paddd    %xmm3, %xmm9
    movdqa    %xmm9, (%rsp)
    addq    $4*16, %rdx
    sha256_mixed_quadround %r12d, %r13d, %r14d, %r15d, %r8d, %r9d, %r10d, %r11d, %xmm3, %xmm0, %xmm1, %xmm2
    
    subq    $16, %rsi
    jne sha256_transform_sse2_loop
    
    paddd    0*16(%rdx), %xmm0
    movdqa    %xmm0, (%rsp)
    sha256_main_round 0, %r8d, %r9d, %r10d, %r11d, %r12d, %r13d, %r14d, %r15d
    sha256_main_round 1, %r15d, %r8d, %r9d, %r10d, %r11d, %r12d, %r13d, %r14d
    sha256_main_round 2, %r14d, %r15d, %r8d, %r9d, %r10d, %r11d, %r12d, %r13d
    sha256_main_round 3, %r13d, %r14d, %r15d, %r8d, %r9d, %r10d, %r11d, %r12d
    paddd    1*16(%rdx), %xmm1
    movdqa    %xmm1, (%rsp)
    sha256_main_round 0, %r12d, %r13d, %r14d, %r15d, %r8d, %r9d, %r10d, %r11d
    sha256_main_round 1, %r11d, %r12d, %r13d, %r14d, %r15d, %r8d, %r9d, %r10d
    sha256_main_round 2, %r10d, %r11d, %r12d, %r13d, %r14d, %r15d, %r8d, %r9d
    sha256_main_round 3, %r9d, %r10d, %r11d, %r12d, %r13d, %r14d, %r15d, %r8d
    paddd    2*16(%rdx), %xmm2
    movdqa    %xmm2, (%rsp)
    sha256_main_round 0, %r8d, %r9d, %r10d, %r11d, %r12d, %r13d, %r14d, %r15d
    sha256_main_round 1, %r15d, %r8d, %r9d, %r10d, %r11d, %r12d, %r13d, %r14d
    sha256_main_round 2, %r14d, %r15d, %r8d, %r9d, %r10d, %r11d, %r12d, %r13d
    sha256_main_round 3, %r13d, %r14d, %r15d, %r8d, %r9d, %r10d, %r11d, %r12d
    paddd    3*16(%rdx), %xmm3
    movdqa    %xmm3, (%rsp)
    sha256_main_round 0, %r12d, %r13d, %r14d, %r15d, %r8d, %r9d, %r10d, %r11d
    sha256_main_round 1, %r11d, %r12d, %r13d, %r14d, %r15d, %r8d, %r9d, %r10d
    sha256_main_round 2, %r10d, %r11d, %r12d, %r13d, %r14d, %r15d, %r8d, %r9d
    sha256_main_round 3, %r9d, %r10d, %r11d, %r12d, %r13d, %r14d, %r15d, %r8d
    
    addl    %r8d, 0*4(%rdi)
    addl    %r9d, 1*4(%rdi)
    addl    %r10d, 2*4(%rdi)
    addl    %r11d, 3*4(%rdi)
    addl    %r12d, 4*4(%rdi)
    addl    %r13d, 5*4(%rdi)
    addl    %r14d, 6*4(%rdi)
    addl    %r15d, 7*4(%rdi)
    
#if defined(_WIN64) || defined(__CYGWIN__)
    movdqa    1*16(%rsp), %xmm6
    movdqa    2*16(%rsp), %xmm7
    movdqa    3*16(%rsp), %xmm8
    movdqa    4*16(%rsp), %xmm9
    addq    $5*16, %rsp
    popq    %rsi
    popq    %rdi
#else
    addq    $16, %rsp
#endif
    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %rbx
    ret


    .text
    .p2align 6
sha256_transform_phe:
#if defined(_WIN64) || defined(__CYGWIN__)
    pushq    %rdi
    pushq    %rsi
    movq    %rcx, %rdi
    movq    %rdx, %rsi
    movq    %r8, %rdx
#endif
    movq    %rsp, %r8
    subq    $64, %rsp
    andq    $-64, %rsp
    
    testq    %rdx, %rdx
    jnz sha256_transform_phe_noswap
    
    movl    0*4(%rsi), %eax
    movl    1*4(%rsi), %ecx
    movl    2*4(%rsi), %edx
    movl    3*4(%rsi), %r9d
    bswapl    %eax
    bswapl    %ecx
    bswapl    %edx
    bswapl    %r9d
    movl    %eax, 0*4(%rsp)
    movl    %ecx, 1*4(%rsp)
    movl    %edx, 2*4(%rsp)
    movl    %r9d, 3*4(%rsp)
    movl    4*4(%rsi), %eax
    movl    5*4(%rsi), %ecx
    movl    6*4(%rsi), %edx
    movl    7*4(%rsi), %r9d
    bswapl    %eax
    bswapl    %ecx
    bswapl    %edx
    bswapl    %r9d
    movl    %eax, 4*4(%rsp)
    movl    %ecx, 5*4(%rsp)
    movl    %edx, 6*4(%rsp)
    movl    %r9d, 7*4(%rsp)
    
    movdqu    2*16(%rsi), %xmm0
    movdqu    3*16(%rsi), %xmm2
    pshuflw    $0xb1, %xmm0, %xmm0
    pshuflw    $0xb1, %xmm2, %xmm2
    pshufhw    $0xb1, %xmm0, %xmm0
    pshufhw    $0xb1, %xmm2, %xmm2
    movdqa    %xmm0, %xmm1
    movdqa    %xmm2, %xmm3
    psrlw    $8, %xmm1
    psrlw    $8, %xmm3
    psllw    $8, %xmm0
    psllw    $8, %xmm2
    pxor    %xmm1, %xmm0
    pxor    %xmm3, %xmm2
    movdqa    %xmm0, 2*16(%rsp)
    movdqa    %xmm2, 3*16(%rsp)
    
    jmp sha256_transform_phe_core
    
sha256_transform_phe_noswap:
    movdqu    0*16(%rsi), %xmm0
    movdqu    1*16(%rsi), %xmm1
    movdqu    2*16(%rsi), %xmm2
    movdqu    3*16(%rsi), %xmm3
    movdqa    %xmm0, 0*16(%rsp)
    movdqa    %xmm1, 1*16(%rsp)
    movdqa    %xmm2, 2*16(%rsp)
    movdqa    %xmm3, 3*16(%rsp)
    
sha256_transform_phe_core:
    movq    %rsp, %rsi
    movq    $-1, %rax
    movq    $1, %rcx
    /* rep xsha256 */
    .byte 0xf3, 0x0f, 0xa6, 0xd0
    
    movq    %r8, %rsp
#if defined(_WIN64) || defined(__CYGWIN__)
    popq    %rsi
    popq    %rdi
#endif
    ret
    
    
    .data
    .p2align 3
sha256_transform_addr:
    .quad sha256_transform_sse2
    
    .text
    .p2align 3
    .globl sha256_transform
    .globl _sha256_transform
sha256_transform:
_sha256_transform:
    jmp *sha256_transform_addr(%rip)


    .data
    .p2align 7
sha256_4h:
    .long 0x6a09e667, 0x6a09e667, 0x6a09e667, 0x6a09e667
    .long 0xbb67ae85, 0xbb67ae85, 0xbb67ae85, 0xbb67ae85
    .long 0x3c6ef372, 0x3c6ef372, 0x3c6ef372, 0x3c6ef372
    .long 0xa54ff53a, 0xa54ff53a, 0xa54ff53a, 0xa54ff53a
    .long 0x510e527f, 0x510e527f, 0x510e527f, 0x510e527f
    .long 0x9b05688c, 0x9b05688c, 0x9b05688c, 0x9b05688c
    .long 0x1f83d9ab, 0x1f83d9ab, 0x1f83d9ab, 0x1f83d9ab
    .long 0x5be0cd19, 0x5be0cd19, 0x5be0cd19, 0x5be0cd19

    .data
    .p2align 7
sha256_4k:
    .long 0x428a2f98, 0x428a2f98, 0x428a2f98, 0x428a2f98
    .long 0x71374491, 0x71374491, 0x71374491, 0x71374491
    .long 0xb5c0fbcf, 0xb5c0fbcf, 0xb5c0fbcf, 0xb5c0fbcf
    .long 0xe9b5dba5, 0xe9b5dba5, 0xe9b5dba5, 0xe9b5dba5
    .long 0x3956c25b, 0x3956c25b, 0x3956c25b, 0x3956c25b
    .long 0x59f111f1, 0x59f111f1, 0x59f111f1, 0x59f111f1
    .long 0x923f82a4, 0x923f82a4, 0x923f82a4, 0x923f82a4
    .long 0xab1c5ed5, 0xab1c5ed5, 0xab1c5ed5, 0xab1c5ed5
    .long 0xd807aa98, 0xd807aa98, 0xd807aa98, 0xd807aa98
    .long 0x12835b01, 0x12835b01, 0x12835b01, 0x12835b01
    .long 0x243185be, 0x243185be, 0x243185be, 0x243185be
    .long 0x550c7dc3, 0x550c7dc3, 0x550c7dc3, 0x550c7dc3
    .long 0x72be5d74, 0x72be5d74, 0x72be5d74, 0x72be5d74
    .long 0x80deb1fe, 0x80deb1fe, 0x80deb1fe, 0x80deb1fe
    .long 0x9bdc06a7, 0x9bdc06a7, 0x9bdc06a7, 0x9bdc06a7
    .long 0xc19bf174, 0xc19bf174, 0xc19bf174, 0xc19bf174
    .long 0xe49b69c1, 0xe49b69c1, 0xe49b69c1, 0xe49b69c1
    .long 0xefbe4786, 0xefbe4786, 0xefbe4786, 0xefbe4786
    .long 0x0fc19dc6, 0x0fc19dc6, 0x0fc19dc6, 0x0fc19dc6
    .long 0x240ca1cc, 0x240ca1cc, 0x240ca1cc, 0x240ca1cc
    .long 0x2de92c6f, 0x2de92c6f, 0x2de92c6f, 0x2de92c6f
    .long 0x4a7484aa, 0x4a7484aa, 0x4a7484aa, 0x4a7484aa
    .long 0x5cb0a9dc, 0x5cb0a9dc, 0x5cb0a9dc, 0x5cb0a9dc
    .long 0x76f988da, 0x76f988da, 0x76f988da, 0x76f988da
    .long 0x983e5152, 0x983e5152, 0x983e5152, 0x983e5152
    .long 0xa831c66d, 0xa831c66d, 0xa831c66d, 0xa831c66d
    .long 0xb00327c8, 0xb00327c8, 0xb00327c8, 0xb00327c8
    .long 0xbf597fc7, 0xbf597fc7, 0xbf597fc7, 0xbf597fc7
    .long 0xc6e00bf3, 0xc6e00bf3, 0xc6e00bf3, 0xc6e00bf3
    .long 0xd5a79147, 0xd5a79147, 0xd5a79147, 0xd5a79147
    .long 0x06ca6351, 0x06ca6351, 0x06ca6351, 0x06ca6351
    .long 0x14292967, 0x14292967, 0x14292967, 0x14292967
    .long 0x27b70a85, 0x27b70a85, 0x27b70a85, 0x27b70a85
    .long 0x2e1b2138, 0x2e1b2138, 0x2e1b2138, 0x2e1b2138
    .long 0x4d2c6dfc, 0x4d2c6dfc, 0x4d2c6dfc, 0x4d2c6dfc
    .long 0x53380d13, 0x53380d13, 0x53380d13, 0x53380d13
    .long 0x650a7354, 0x650a7354, 0x650a7354, 0x650a7354
    .long 0x766a0abb, 0x766a0abb, 0x766a0abb, 0x766a0abb
    .long 0x81c2c92e, 0x81c2c92e, 0x81c2c92e, 0x81c2c92e
    .long 0x92722c85, 0x92722c85, 0x92722c85, 0x92722c85
    .long 0xa2bfe8a1, 0xa2bfe8a1, 0xa2bfe8a1, 0xa2bfe8a1
    .long 0xa81a664b, 0xa81a664b, 0xa81a664b, 0xa81a664b
    .long 0xc24b8b70, 0xc24b8b70, 0xc24b8b70, 0xc24b8b70
    .long 0xc76c51a3, 0xc76c51a3, 0xc76c51a3, 0xc76c51a3
    .long 0xd192e819, 0xd192e819, 0xd192e819, 0xd192e819
    .long 0xd6990624, 0xd6990624, 0xd6990624, 0xd6990624
    .long 0xf40e3585, 0xf40e3585, 0xf40e3585, 0xf40e3585
    .long 0x106aa070, 0x106aa070, 0x106aa070, 0x106aa070
    .long 0x19a4c116, 0x19a4c116, 0x19a4c116, 0x19a4c116
    .long 0x1e376c08, 0x1e376c08, 0x1e376c08, 0x1e376c08
    .long 0x2748774c, 0x2748774c, 0x2748774c, 0x2748774c
    .long 0x34b0bcb5, 0x34b0bcb5, 0x34b0bcb5, 0x34b0bcb5
    .long 0x391c0cb3, 0x391c0cb3, 0x391c0cb3, 0x391c0cb3
    .long 0x4ed8aa4a, 0x4ed8aa4a, 0x4ed8aa4a, 0x4ed8aa4a
    .long 0x5b9cca4f, 0x5b9cca4f, 0x5b9cca4f, 0x5b9cca4f
    .long 0x682e6ff3, 0x682e6ff3, 0x682e6ff3, 0x682e6ff3
    .long 0x748f82ee, 0x748f82ee, 0x748f82ee, 0x748f82ee
    .long 0x78a5636f, 0x78a5636f, 0x78a5636f, 0x78a5636f
    .long 0x84c87814, 0x84c87814, 0x84c87814, 0x84c87814
    .long 0x8cc70208, 0x8cc70208, 0x8cc70208, 0x8cc70208
    .long 0x90befffa, 0x90befffa, 0x90befffa, 0x90befffa
    .long 0xa4506ceb, 0xa4506ceb, 0xa4506ceb, 0xa4506ceb
    .long 0xbef9a3f7, 0xbef9a3f7, 0xbef9a3f7, 0xbef9a3f7
    .long 0xc67178f2, 0xc67178f2, 0xc67178f2, 0xc67178f2

    .data
    .p2align 7
sha256_8h:
    .long 0x6a09e667, 0x6a09e667, 0x6a09e667, 0x6a09e667, 0x6a09e667, 0x6a09e667, 0x6a09e667, 0x6a09e667
    .long 0xbb67ae85, 0xbb67ae85, 0xbb67ae85, 0xbb67ae85, 0xbb67ae85, 0xbb67ae85, 0xbb67ae85, 0xbb67ae85
    .long 0x3c6ef372, 0x3c6ef372, 0x3c6ef372, 0x3c6ef372, 0x3c6ef372, 0x3c6ef372, 0x3c6ef372, 0x3c6ef372
    .long 0xa54ff53a, 0xa54ff53a, 0xa54ff53a, 0xa54ff53a, 0xa54ff53a, 0xa54ff53a, 0xa54ff53a, 0xa54ff53a
    .long 0x510e527f, 0x510e527f, 0x510e527f, 0x510e527f, 0x510e527f, 0x510e527f, 0x510e527f, 0x510e527f
    .long 0x9b05688c, 0x9b05688c, 0x9b05688c, 0x9b05688c, 0x9b05688c, 0x9b05688c, 0x9b05688c, 0x9b05688c
    .long 0x1f83d9ab, 0x1f83d9ab, 0x1f83d9ab, 0x1f83d9ab, 0x1f83d9ab, 0x1f83d9ab, 0x1f83d9ab, 0x1f83d9ab
    .long 0x5be0cd19, 0x5be0cd19, 0x5be0cd19, 0x5be0cd19, 0x5be0cd19, 0x5be0cd19, 0x5be0cd19, 0x5be0cd19

    .data
    .p2align 7
sha256_8k:
    .long 0x428a2f98, 0x428a2f98, 0x428a2f98, 0x428a2f98, 0x428a2f98, 0x428a2f98, 0x428a2f98, 0x428a2f98
    .long 0x71374491, 0x71374491, 0x71374491, 0x71374491, 0x71374491, 0x71374491, 0x71374491, 0x71374491
    .long 0xb5c0fbcf, 0xb5c0fbcf, 0xb5c0fbcf, 0xb5c0fbcf, 0xb5c0fbcf, 0xb5c0fbcf, 0xb5c0fbcf, 0xb5c0fbcf
    .long 0xe9b5dba5, 0xe9b5dba5, 0xe9b5dba5, 0xe9b5dba5, 0xe9b5dba5, 0xe9b5dba5, 0xe9b5dba5, 0xe9b5dba5
    .long 0x3956c25b, 0x3956c25b, 0x3956c25b, 0x3956c25b, 0x3956c25b, 0x3956c25b, 0x3956c25b, 0x3956c25b
    .long 0x59f111f1, 0x59f111f1, 0x59f111f1, 0x59f111f1, 0x59f111f1, 0x59f111f1, 0x59f111f1, 0x59f111f1
    .long 0x923f82a4, 0x923f82a4, 0x923f82a4, 0x923f82a4, 0x923f82a4, 0x923f82a4, 0x923f82a4, 0x923f82a4
    .long 0xab1c5ed5, 0xab1c5ed5, 0xab1c5ed5, 0xab1c5ed5, 0xab1c5ed5, 0xab1c5ed5, 0xab1c5ed5, 0xab1c5ed5
    .long 0xd807aa98, 0xd807aa98, 0xd807aa98, 0xd807aa98, 0xd807aa98, 0xd807aa98, 0xd807aa98, 0xd807aa98
    .long 0x12835b01, 0x12835b01, 0x12835b01, 0x12835b01, 0x12835b01, 0x12835b01, 0x12835b01, 0x12835b01
    .long 0x243185be, 0x243185be, 0x243185be, 0x243185be, 0x243185be, 0x243185be, 0x243185be, 0x243185be
    .long 0x550c7dc3, 0x550c7dc3, 0x550c7dc3, 0x550c7dc3, 0x550c7dc3, 0x550c7dc3, 0x550c7dc3, 0x550c7dc3
    .long 0x72be5d74, 0x72be5d74, 0x72be5d74, 0x72be5d74, 0x72be5d74, 0x72be5d74, 0x72be5d74, 0x72be5d74
    .long 0x80deb1fe, 0x80deb1fe, 0x80deb1fe, 0x80deb1fe, 0x80deb1fe, 0x80deb1fe, 0x80deb1fe, 0x80deb1fe
    .long 0x9bdc06a7, 0x9bdc06a7, 0x9bdc06a7, 0x9bdc06a7, 0x9bdc06a7, 0x9bdc06a7, 0x9bdc06a7, 0x9bdc06a7
    .long 0xc19bf174, 0xc19bf174, 0xc19bf174, 0xc19bf174, 0xc19bf174, 0xc19bf174, 0xc19bf174, 0xc19bf174
    .long 0xe49b69c1, 0xe49b69c1, 0xe49b69c1, 0xe49b69c1, 0xe49b69c1, 0xe49b69c1, 0xe49b69c1, 0xe49b69c1
    .long 0xefbe4786, 0xefbe4786, 0xefbe4786, 0xefbe4786, 0xefbe4786, 0xefbe4786, 0xefbe4786, 0xefbe4786
    .long 0x0fc19dc6, 0x0fc19dc6, 0x0fc19dc6, 0x0fc19dc6, 0x0fc19dc6, 0x0fc19dc6, 0x0fc19dc6, 0x0fc19dc6
    .long 0x240ca1cc, 0x240ca1cc, 0x240ca1cc, 0x240ca1cc, 0x240ca1cc, 0x240ca1cc, 0x240ca1cc, 0x240ca1cc
    .long 0x2de92c6f, 0x2de92c6f, 0x2de92c6f, 0x2de92c6f, 0x2de92c6f, 0x2de92c6f, 0x2de92c6f, 0x2de92c6f
    .long 0x4a7484aa, 0x4a7484aa, 0x4a7484aa, 0x4a7484aa, 0x4a7484aa, 0x4a7484aa, 0x4a7484aa, 0x4a7484aa
    .long 0x5cb0a9dc, 0x5cb0a9dc, 0x5cb0a9dc, 0x5cb0a9dc, 0x5cb0a9dc, 0x5cb0a9dc, 0x5cb0a9dc, 0x5cb0a9dc
    .long 0x76f988da, 0x76f988da, 0x76f988da, 0x76f988da, 0x76f988da, 0x76f988da, 0x76f988da, 0x76f988da
    .long 0x983e5152, 0x983e5152, 0x983e5152, 0x983e5152, 0x983e5152, 0x983e5152, 0x983e5152, 0x983e5152
    .long 0xa831c66d, 0xa831c66d, 0xa831c66d, 0xa831c66d, 0xa831c66d, 0xa831c66d, 0xa831c66d, 0xa831c66d
    .long 0xb00327c8, 0xb00327c8, 0xb00327c8, 0xb00327c8, 0xb00327c8, 0xb00327c8, 0xb00327c8, 0xb00327c8
    .long 0xbf597fc7, 0xbf597fc7, 0xbf597fc7, 0xbf597fc7, 0xbf597fc7, 0xbf597fc7, 0xbf597fc7, 0xbf597fc7
    .long 0xc6e00bf3, 0xc6e00bf3, 0xc6e00bf3, 0xc6e00bf3, 0xc6e00bf3, 0xc6e00bf3, 0xc6e00bf3, 0xc6e00bf3
    .long 0xd5a79147, 0xd5a79147, 0xd5a79147, 0xd5a79147, 0xd5a79147, 0xd5a79147, 0xd5a79147, 0xd5a79147
    .long 0x06ca6351, 0x06ca6351, 0x06ca6351, 0x06ca6351, 0x06ca6351, 0x06ca6351, 0x06ca6351, 0x06ca6351
    .long 0x14292967, 0x14292967, 0x14292967, 0x14292967, 0x14292967, 0x14292967, 0x14292967, 0x14292967
    .long 0x27b70a85, 0x27b70a85, 0x27b70a85, 0x27b70a85, 0x27b70a85, 0x27b70a85, 0x27b70a85, 0x27b70a85
    .long 0x2e1b2138, 0x2e1b2138, 0x2e1b2138, 0x2e1b2138, 0x2e1b2138, 0x2e1b2138, 0x2e1b2138, 0x2e1b2138
    .long 0x4d2c6dfc, 0x4d2c6dfc, 0x4d2c6dfc, 0x4d2c6dfc, 0x4d2c6dfc, 0x4d2c6dfc, 0x4d2c6dfc, 0x4d2c6dfc
    .long 0x53380d13, 0x53380d13, 0x53380d13, 0x53380d13, 0x53380d13, 0x53380d13, 0x53380d13, 0x53380d13
    .long 0x650a7354, 0x650a7354, 0x650a7354, 0x650a7354, 0x650a7354, 0x650a7354, 0x650a7354, 0x650a7354
    .long 0x766a0abb, 0x766a0abb, 0x766a0abb, 0x766a0abb, 0x766a0abb, 0x766a0abb, 0x766a0abb, 0x766a0abb
    .long 0x81c2c92e, 0x81c2c92e, 0x81c2c92e, 0x81c2c92e, 0x81c2c92e, 0x81c2c92e, 0x81c2c92e, 0x81c2c92e
    .long 0x92722c85, 0x92722c85, 0x92722c85, 0x92722c85, 0x92722c85, 0x92722c85, 0x92722c85, 0x92722c85
    .long 0xa2bfe8a1, 0xa2bfe8a1, 0xa2bfe8a1, 0xa2bfe8a1, 0xa2bfe8a1, 0xa2bfe8a1, 0xa2bfe8a1, 0xa2bfe8a1
    .long 0xa81a664b, 0xa81a664b, 0xa81a664b, 0xa81a664b, 0xa81a664b, 0xa81a664b, 0xa81a664b, 0xa81a664b
    .long 0xc24b8b70, 0xc24b8b70, 0xc24b8b70, 0xc24b8b70, 0xc24b8b70, 0xc24b8b70, 0xc24b8b70, 0xc24b8b70
    .long 0xc76c51a3, 0xc76c51a3, 0xc76c51a3, 0xc76c51a3, 0xc76c51a3, 0xc76c51a3, 0xc76c51a3, 0xc76c51a3
    .long 0xd192e819, 0xd192e819, 0xd192e819, 0xd192e819, 0xd192e819, 0xd192e819, 0xd192e819, 0xd192e819
    .long 0xd6990624, 0xd6990624, 0xd6990624, 0xd6990624, 0xd6990624, 0xd6990624, 0xd6990624, 0xd6990624
    .long 0xf40e3585, 0xf40e3585, 0xf40e3585, 0xf40e3585, 0xf40e3585, 0xf40e3585, 0xf40e3585, 0xf40e3585
    .long 0x106aa070, 0x106aa070, 0x106aa070, 0x106aa070, 0x106aa070, 0x106aa070, 0x106aa070, 0x106aa070
    .long 0x19a4c116, 0x19a4c116, 0x19a4c116, 0x19a4c116, 0x19a4c116, 0x19a4c116, 0x19a4c116, 0x19a4c116
    .long 0x1e376c08, 0x1e376c08, 0x1e376c08, 0x1e376c08, 0x1e376c08, 0x1e376c08, 0x1e376c08, 0x1e376c08
    .long 0x2748774c, 0x2748774c, 0x2748774c, 0x2748774c, 0x2748774c, 0x2748774c, 0x2748774c, 0x2748774c
    .long 0x34b0bcb5, 0x34b0bcb5, 0x34b0bcb5, 0x34b0bcb5, 0x34b0bcb5, 0x34b0bcb5, 0x34b0bcb5, 0x34b0bcb5
    .long 0x391c0cb3, 0x391c0cb3, 0x391c0cb3, 0x391c0cb3, 0x391c0cb3, 0x391c0cb3, 0x391c0cb3, 0x391c0cb3
    .long 0x4ed8aa4a, 0x4ed8aa4a, 0x4ed8aa4a, 0x4ed8aa4a, 0x4ed8aa4a, 0x4ed8aa4a, 0x4ed8aa4a, 0x4ed8aa4a
    .long 0x5b9cca4f, 0x5b9cca4f, 0x5b9cca4f, 0x5b9cca4f, 0x5b9cca4f, 0x5b9cca4f, 0x5b9cca4f, 0x5b9cca4f
    .long 0x682e6ff3, 0x682e6ff3, 0x682e6ff3, 0x682e6ff3, 0x682e6ff3, 0x682e6ff3, 0x682e6ff3, 0x682e6ff3
    .long 0x748f82ee, 0x748f82ee, 0x748f82ee, 0x748f82ee, 0x748f82ee, 0x748f82ee, 0x748f82ee, 0x748f82ee
    .long 0x78a5636f, 0x78a5636f, 0x78a5636f, 0x78a5636f, 0x78a5636f, 0x78a5636f, 0x78a5636f, 0x78a5636f
    .long 0x84c87814, 0x84c87814, 0x84c87814, 0x84c87814, 0x84c87814, 0x84c87814, 0x84c87814, 0x84c87814
    .long 0x8cc70208, 0x8cc70208, 0x8cc70208, 0x8cc70208, 0x8cc70208, 0x8cc70208, 0x8cc70208, 0x8cc70208
    .long 0x90befffa, 0x90befffa, 0x90befffa, 0x90befffa, 0x90befffa, 0x90befffa, 0x90befffa, 0x90befffa
    .long 0xa4506ceb, 0xa4506ceb, 0xa4506ceb, 0xa4506ceb, 0xa4506ceb, 0xa4506ceb, 0xa4506ceb, 0xa4506ceb
    .long 0xbef9a3f7, 0xbef9a3f7, 0xbef9a3f7, 0xbef9a3f7, 0xbef9a3f7, 0xbef9a3f7, 0xbef9a3f7, 0xbef9a3f7
    .long 0xc67178f2, 0xc67178f2, 0xc67178f2, 0xc67178f2, 0xc67178f2, 0xc67178f2, 0xc67178f2, 0xc67178f2

    .text
    .p2align 6
    .globl sha256_init_4way
    .globl _sha256_init_4way
sha256_init_4way:
_sha256_init_4way:
#if defined(_WIN64) || defined(__CYGWIN__)
    pushq    %rdi
    movq    %rcx, %rdi
#endif
    movdqa    sha256_4h+0(%rip), %xmm0
    movdqa    sha256_4h+16(%rip), %xmm1
    movdqa    sha256_4h+32(%rip), %xmm2
    movdqa    sha256_4h+48(%rip), %xmm3
    movdqu    %xmm0, 0(%rdi)
    movdqu    %xmm1, 16(%rdi)
    movdqu    %xmm2, 32(%rdi)
    movdqu    %xmm3, 48(%rdi)
    movdqa    sha256_4h+64(%rip), %xmm0
    movdqa    sha256_4h+80(%rip), %xmm1
    movdqa    sha256_4h+96(%rip), %xmm2
    movdqa    sha256_4h+112(%rip), %xmm3
    movdqu    %xmm0, 64(%rdi)
    movdqu    %xmm1, 80(%rdi)
    movdqu    %xmm2, 96(%rdi)
    movdqu    %xmm3, 112(%rdi)
#if defined(_WIN64) || defined(__CYGWIN__)
    popq    %rdi
#endif
    ret

    .text
    .p2align 6
    .globl sha256_init_8way
    .globl _sha256_init_8way
sha256_init_8way:
_sha256_init_8way:
#if defined(_WIN64) || defined(__CYGWIN__)
    pushq    %rdi
    movq    %rcx, %rdi
#endif
    vpbroadcastd    sha256_4h+0(%rip), %ymm0
    vpbroadcastd    sha256_4h+16(%rip), %ymm1
    vpbroadcastd    sha256_4h+32(%rip), %ymm2
    vpbroadcastd    sha256_4h+48(%rip), %ymm3
    vmovdqu    %ymm0, 0*32(%rdi)
    vmovdqu    %ymm1, 1*32(%rdi)
    vmovdqu    %ymm2, 2*32(%rdi)
    vmovdqu    %ymm3, 3*32(%rdi)
    vpbroadcastd    sha256_4h+64(%rip), %ymm0
    vpbroadcastd    sha256_4h+80(%rip), %ymm1
    vpbroadcastd    sha256_4h+96(%rip), %ymm2
    vpbroadcastd    sha256_4h+112(%rip), %ymm3
    vmovdqu    %ymm0, 4*32(%rdi)
    vmovdqu    %ymm1, 5*32(%rdi)
    vmovdqu    %ymm2, 6*32(%rdi)
    vmovdqu    %ymm3, 7*32(%rdi)
#if defined(_WIN64) || defined(__CYGWIN__)
    popq    %rdi
#endif
    ret

.macro sha256_sse2_extend_round i
    movdqa    (\i-15)*16(%rax), %xmm0
    movdqa    %xmm0, %xmm2
    psrld    $3, %xmm0
    movdqa    %xmm0, %xmm1
    pslld    $14, %xmm2
    psrld    $4, %xmm1
    pxor    %xmm1, %xmm0
    pxor    %xmm2, %xmm0
    psrld    $11, %xmm1
    pslld    $11, %xmm2
    pxor    %xmm1, %xmm0
    pxor    %xmm2, %xmm0
    paddd    (\i-16)*16(%rax), %xmm0
    paddd    (\i-7)*16(%rax), %xmm0

    movdqa    %xmm3, %xmm2
    psrld    $10, %xmm3
    pslld    $13, %xmm2
    movdqa    %xmm3, %xmm1
    psrld    $7, %xmm1
    pxor    %xmm1, %xmm3
    pxor    %xmm2, %xmm3
    psrld    $2, %xmm1
    pslld    $2, %xmm2
    pxor    %xmm1, %xmm3
    pxor    %xmm2, %xmm3
    paddd    %xmm0, %xmm3
    movdqa    %xmm3, \i*16(%rax)
.endm

.macro sha256_sse2_extend_doubleround i
    movdqa    (\i-15)*16(%rax), %xmm0
    movdqa    (\i-14)*16(%rax), %xmm4
    movdqa    %xmm0, %xmm2
    movdqa    %xmm4, %xmm6
    psrld    $3, %xmm0
    psrld    $3, %xmm4
    movdqa    %xmm0, %xmm1
    movdqa    %xmm4, %xmm5
    pslld    $14, %xmm2
    pslld    $14, %xmm6
    psrld    $4, %xmm1
    psrld    $4, %xmm5
    pxor    %xmm1, %xmm0
    pxor    %xmm5, %xmm4
    psrld    $11, %xmm1
    psrld    $11, %xmm5
    pxor    %xmm2, %xmm0
    pxor    %xmm6, %xmm4
    pslld    $11, %xmm2
    pslld    $11, %xmm6
    pxor    %xmm1, %xmm0
    pxor    %xmm5, %xmm4
    pxor    %xmm2, %xmm0
    pxor    %xmm6, %xmm4

    paddd    (\i-16)*16(%rax), %xmm0
    paddd    (\i-15)*16(%rax), %xmm4

    movdqa    %xmm3, %xmm2
    movdqa    %xmm7, %xmm6
    psrld    $10, %xmm3
    psrld    $10, %xmm7
    movdqa    %xmm3, %xmm1
    movdqa    %xmm7, %xmm5
    pslld    $13, %xmm2
    pslld    $13, %xmm6
    psrld    $7, %xmm1
    psrld    $7, %xmm5

    paddd    (\i-7)*16(%rax), %xmm0
    paddd    (\i-6)*16(%rax), %xmm4

    pxor    %xmm1, %xmm3
    pxor    %xmm5, %xmm7
    psrld    $2, %xmm1
    psrld    $2, %xmm5
    pxor    %xmm2, %xmm3
    pxor    %xmm6, %xmm7
    pslld    $2, %xmm2
    pslld    $2, %xmm6
    pxor    %xmm1, %xmm3
    pxor    %xmm5, %xmm7
    pxor    %xmm2, %xmm3
    pxor    %xmm6, %xmm7

    paddd    %xmm0, %xmm3
    paddd    %xmm4, %xmm7
    movdqa    %xmm3, \i*16(%rax)
    movdqa    %xmm7, (\i+1)*16(%rax)
.endm

.macro sha256_sse2_main_round i
    movdqa    16*(\i)(%rax), %xmm6

    movdqa    %xmm0, %xmm1
    movdqa    16(%rsp), %xmm2
    pandn    %xmm2, %xmm1
    paddd    32(%rsp), %xmm6

    movdqa    %xmm2, 32(%rsp)
    movdqa    0(%rsp), %xmm2
    movdqa    %xmm2, 16(%rsp)

    pand    %xmm0, %xmm2
    pxor    %xmm2, %xmm1
    movdqa    %xmm0, 0(%rsp)

    paddd    %xmm1, %xmm6

    movdqa    %xmm0, %xmm1
    psrld    $6, %xmm0
    paddd    16*(\i)(%rcx), %xmm6
    movdqa    %xmm0, %xmm2
    pslld    $7, %xmm1
    psrld    $5, %xmm2
    pxor    %xmm1, %xmm0
    pxor    %xmm2, %xmm0
    pslld    $14, %xmm1
    psrld    $14, %xmm2
    pxor    %xmm1, %xmm0
    pslld    $5, %xmm1
    pxor    %xmm2, %xmm0
    pxor    %xmm1, %xmm0
    movdqa    %xmm5, %xmm1
    paddd    %xmm0, %xmm6

    movdqa    %xmm3, %xmm0
    movdqa    %xmm4, %xmm3
    movdqa    %xmm4, %xmm2
    paddd    %xmm6, %xmm0
    pand    %xmm5, %xmm2
    pand    %xmm7, %xmm1
    pand    %xmm7, %xmm4
    pxor    %xmm4, %xmm1
    movdqa    %xmm5, %xmm4
    movdqa    %xmm7, %xmm5
    pxor    %xmm2, %xmm1
    paddd    %xmm1, %xmm6

    movdqa    %xmm7, %xmm2
    psrld    $2, %xmm7
    movdqa    %xmm7, %xmm1
    pslld    $10, %xmm2
    psrld    $11, %xmm1
    pxor    %xmm2, %xmm7
    pslld    $9, %xmm2
    pxor    %xmm1, %xmm7
    psrld    $9, %xmm1
    pxor    %xmm2, %xmm7
    pslld    $11, %xmm2
    pxor    %xmm1, %xmm7
    pxor    %xmm2, %xmm7
    paddd    %xmm6, %xmm7
.endm

.macro sha256_sse2_main_quadround i
    sha256_sse2_main_round \i+0
    sha256_sse2_main_round \i+1
    sha256_sse2_main_round \i+2
    sha256_sse2_main_round \i+3
.endm


.macro sha256_avx_extend_round i
    vmovdqa    (\i-15)*16(%rax), %xmm0
    vpslld    $14, %xmm0, %xmm2
    vpsrld    $3, %xmm0, %xmm0
    vpsrld    $4, %xmm0, %xmm1
    vpxor    %xmm1, %xmm0, %xmm0
    vpxor    %xmm2, %xmm0, %xmm0
    vpsrld    $11, %xmm1, %xmm1
    vpslld    $11, %xmm2, %xmm2
    vpxor    %xmm1, %xmm0, %xmm0
    vpxor    %xmm2, %xmm0, %xmm0
    vpaddd    (\i-16)*16(%rax), %xmm0, %xmm0
    vpaddd    (\i-7)*16(%rax), %xmm0, %xmm0

    vpslld    $13, %xmm3, %xmm2
    vpsrld    $10, %xmm3, %xmm3
    vpsrld    $7, %xmm3, %xmm1
    vpxor    %xmm1, %xmm3, %xmm3
    vpxor    %xmm2, %xmm3, %xmm3
    vpsrld    $2, %xmm1, %xmm1
    vpslld    $2, %xmm2, %xmm2
    vpxor    %xmm1, %xmm3, %xmm3
    vpxor    %xmm2, %xmm3, %xmm3
    vpaddd    %xmm0, %xmm3, %xmm3
    vmovdqa    %xmm3, \i*16(%rax)
.endm

.macro sha256_avx_extend_doubleround i
    vmovdqa    (\i-15)*16(%rax), %xmm0
    vmovdqa    (\i-14)*16(%rax), %xmm4
    vpslld    $14, %xmm0, %xmm2
    vpslld    $14, %xmm4, %xmm6
    vpsrld    $3, %xmm0, %xmm8
    vpsrld    $3, %xmm4, %xmm4
    vpsrld    $7, %xmm0, %xmm1
    vpsrld    $4, %xmm4, %xmm5
    vpxor    %xmm1, %xmm8, %xmm8
    vpxor    %xmm5, %xmm4, %xmm4
    vpsrld    $11, %xmm1, %xmm1
    vpsrld    $11, %xmm5, %xmm5
    vpxor    %xmm2, %xmm8, %xmm8
    vpxor    %xmm6, %xmm4, %xmm4
    vpslld    $11, %xmm2, %xmm2
    vpslld    $11, %xmm6, %xmm6
    vpxor    %xmm1, %xmm8, %xmm8
    vpxor    %xmm5, %xmm4, %xmm4
    vpxor    %xmm2, %xmm8, %xmm8
    vpxor    %xmm6, %xmm4, %xmm4

    vpaddd    %xmm0, %xmm4, %xmm4
    vpaddd    (\i-16)*16(%rax), %xmm8, %xmm0

    vpslld    $13, %xmm3, %xmm2
    vpslld    $13, %xmm7, %xmm6
    vpsrld    $10, %xmm3, %xmm3
    vpsrld    $10, %xmm7, %xmm7

    vpaddd    (\i-7)*16(%rax), %xmm0, %xmm0
    vpaddd    (\i-6)*16(%rax), %xmm4, %xmm4

    vpsrld    $7, %xmm3, %xmm1
    vpsrld    $7, %xmm7, %xmm5
    vpxor    %xmm1, %xmm3, %xmm3
    vpxor    %xmm5, %xmm7, %xmm7
    vpsrld    $2, %xmm1, %xmm1
    vpsrld    $2, %xmm5, %xmm5
    vpxor    %xmm2, %xmm3, %xmm3
    vpxor    %xmm6, %xmm7, %xmm7
    vpslld    $2, %xmm2, %xmm2
    vpslld    $2, %xmm6, %xmm6
    vpxor    %xmm1, %xmm3, %xmm3
    vpxor    %xmm5, %xmm7, %xmm7
    vpxor    %xmm2, %xmm3, %xmm3
    vpxor    %xmm6, %xmm7, %xmm7

    vpaddd    %xmm0, %xmm3, %xmm3
    vpaddd    %xmm4, %xmm7, %xmm7
    vmovdqa    %xmm3, \i*16(%rax)
    vmovdqa    %xmm7, (\i+1)*16(%rax)
.endm

.macro sha256_avx_main_round i, r0, r1, r2, r3, r4, r5, r6, r7
    vpaddd    16*(\i)(%rax), \r0, %xmm6
    vpaddd    16*(\i)(%rcx), %xmm6, %xmm6

    vpandn    \r1, \r3, %xmm1
    vpand    \r3, \r2, %xmm2
    vpxor    %xmm2, %xmm1, %xmm1
    vpaddd    %xmm1, %xmm6, %xmm6

    vpslld    $7, \r3, %xmm1
    vpsrld    $6, \r3, \r0
    vpsrld    $5, \r0, %xmm2
    vpxor    %xmm1, \r0, \r0
    vpxor    %xmm2, \r0, \r0
    vpslld    $14, %xmm1, %xmm1
    vpsrld    $14, %xmm2, %xmm2
    vpxor    %xmm1, \r0, \r0
    vpxor    %xmm2, \r0, \r0
    vpslld    $5, %xmm1, %xmm1
    vpxor    %xmm1, \r0, \r0
    vpaddd    \r0, %xmm6, %xmm6
    vpaddd    %xmm6, \r4, \r0

    vpand    \r6, \r5, %xmm2
    vpand    \r7, \r5, \r4
    vpand    \r7, \r6, %xmm1
    vpxor    \r4, %xmm1, %xmm1
    vpxor    %xmm2, %xmm1, %xmm1
    vpaddd    %xmm1, %xmm6, %xmm6

    vpslld    $10, \r7, %xmm2
    vpsrld    $2, \r7, \r4
    vpsrld    $11, \r4, %xmm1
    vpxor    %xmm2, \r4, \r4
    vpxor    %xmm1, \r4, \r4
    vpslld    $9, %xmm2, %xmm2
    vpsrld    $9, %xmm1, %xmm1
    vpxor    %xmm2, \r4, \r4
    vpxor    %xmm1, \r4, \r4
    vpslld    $11, %xmm2, %xmm2
    vpxor    %xmm2, \r4, \r4
    vpaddd    %xmm6, \r4, \r4
.endm

.macro sha256_avx_main_quadround i
    sha256_avx_main_round \i+0, %xmm10, %xmm9, %xmm8, %xmm0, %xmm3, %xmm4, %xmm5, %xmm7
    sha256_avx_main_round \i+1, %xmm9, %xmm8, %xmm0, %xmm10, %xmm4, %xmm5, %xmm7, %xmm3
    sha256_avx_main_round \i+2, %xmm8, %xmm0, %xmm10, %xmm9, %xmm5, %xmm7, %xmm3, %xmm4
    sha256_avx_main_round \i+3, %xmm0, %xmm10, %xmm9, %xmm8, %xmm7, %xmm3, %xmm4, %xmm5
.endm


.macro sha256_avx2_extend_round i
    vmovdqa    (\i-15)*32(%rax), %ymm0
    vpslld    $14, %ymm0, %ymm2
    vpsrld    $3, %ymm0, %ymm0
    vpsrld    $4, %ymm0, %ymm1
    vpxor    %ymm1, %ymm0, %ymm0
    vpxor    %ymm2, %ymm0, %ymm0
    vpsrld    $11, %ymm1, %ymm1
    vpslld    $11, %ymm2, %ymm2
    vpxor    %ymm1, %ymm0, %ymm0
    vpxor    %ymm2, %ymm0, %ymm0
    vpaddd    (\i-16)*32(%rax), %ymm0, %ymm0
    vpaddd    (\i-7)*32(%rax), %ymm0, %ymm0

    vpslld    $13, %ymm3, %ymm2
    vpsrld    $10, %ymm3, %ymm3
    vpsrld    $7, %ymm3, %ymm1
    vpxor    %ymm1, %ymm3, %ymm3
    vpxor    %ymm2, %ymm3, %ymm3
    vpsrld    $2, %ymm1, %ymm1
    vpslld    $2, %ymm2, %ymm2
    vpxor    %ymm1, %ymm3, %ymm3
    vpxor    %ymm2, %ymm3, %ymm3
    vpaddd    %ymm0, %ymm3, %ymm3
    vmovdqa    %ymm3, \i*32(%rax)
.endm

.macro sha256_avx2_extend_doubleround i
    vmovdqa    (\i-15)*32(%rax), %ymm0
    vmovdqa    (\i-14)*32(%rax), %ymm4
    vpslld    $14, %ymm0, %ymm2
    vpslld    $14, %ymm4, %ymm6
    vpsrld    $3, %ymm0, %ymm8
    vpsrld    $3, %ymm4, %ymm4
    vpsrld    $7, %ymm0, %ymm1
    vpsrld    $4, %ymm4, %ymm5
    vpxor    %ymm1, %ymm8, %ymm8
    vpxor    %ymm5, %ymm4, %ymm4
    vpsrld    $11, %ymm1, %ymm1
    vpsrld    $11, %ymm5, %ymm5
    vpxor    %ymm2, %ymm8, %ymm8
    vpxor    %ymm6, %ymm4, %ymm4
    vpslld    $11, %ymm2, %ymm2
    vpslld    $11, %ymm6, %ymm6
    vpxor    %ymm1, %ymm8, %ymm8
    vpxor    %ymm5, %ymm4, %ymm4
    vpxor    %ymm2, %ymm8, %ymm8
    vpxor    %ymm6, %ymm4, %ymm4

    vpaddd    %ymm0, %ymm4, %ymm4
    vpaddd    (\i-16)*32(%rax), %ymm8, %ymm0

    vpslld    $13, %ymm3, %ymm2
    vpslld    $13, %ymm7, %ymm6
    vpsrld    $10, %ymm3, %ymm3
    vpsrld    $10, %ymm7, %ymm7

    vpaddd    (\i-7)*32(%rax), %ymm0, %ymm0
    vpaddd    (\i-6)*32(%rax), %ymm4, %ymm4

    vpsrld    $7, %ymm3, %ymm1
    vpsrld    $7, %ymm7, %ymm5
    vpxor    %ymm1, %ymm3, %ymm3
    vpxor    %ymm5, %ymm7, %ymm7
    vpsrld    $2, %ymm1, %ymm1
    vpsrld    $2, %ymm5, %ymm5
    vpxor    %ymm2, %ymm3, %ymm3
    vpxor    %ymm6, %ymm7, %ymm7
    vpslld    $2, %ymm2, %ymm2
    vpslld    $2, %ymm6, %ymm6
    vpxor    %ymm1, %ymm3, %ymm3
    vpxor    %ymm5, %ymm7, %ymm7
    vpxor    %ymm2, %ymm3, %ymm3
    vpxor    %ymm6, %ymm7, %ymm7

    vpaddd    %ymm0, %ymm3, %ymm3
    vpaddd    %ymm4, %ymm7, %ymm7
    vmovdqa    %ymm3, \i*32(%rax)
    vmovdqa    %ymm7, (\i+1)*32(%rax)
.endm

.macro sha256_avx2_main_round i, r0, r1, r2, r3, r4, r5, r6, r7
    vpaddd    32*(\i)(%rax), \r0, %ymm6
    vpaddd    32*(\i)(%rcx), %ymm6, %ymm6

    vpandn    \r1, \r3, %ymm1
    vpand    \r3, \r2, %ymm2
    vpxor    %ymm2, %ymm1, %ymm1
    vpaddd    %ymm1, %ymm6, %ymm6

    vpslld    $7, \r3, %ymm1
    vpsrld    $6, \r3, \r0
    vpsrld    $5, \r0, %ymm2
    vpxor    %ymm1, \r0, \r0
    vpxor    %ymm2, \r0, \r0
    vpslld    $14, %ymm1, %ymm1
    vpsrld    $14, %ymm2, %ymm2
    vpxor    %ymm1, \r0, \r0
    vpxor    %ymm2, \r0, \r0
    vpslld    $5, %ymm1, %ymm1
    vpxor    %ymm1, \r0, \r0
    vpaddd    \r0, %ymm6, %ymm6
    vpaddd    %ymm6, \r4, \r0

    vpand    \r6, \r5, %ymm2
    vpand    \r7, \r5, \r4
    vpand    \r7, \r6, %ymm1
    vpxor    \r4, %ymm1, %ymm1
    vpxor    %ymm2, %ymm1, %ymm1
    vpaddd    %ymm1, %ymm6, %ymm6

    vpslld    $10, \r7, %ymm2
    vpsrld    $2, \r7, \r4
    vpsrld    $11, \r4, %ymm1
    vpxor    %ymm2, \r4, \r4
    vpxor    %ymm1, \r4, \r4
    vpslld    $9, %ymm2, %ymm2
    vpsrld    $9, %ymm1, %ymm1
    vpxor    %ymm2, \r4, \r4
    vpxor    %ymm1, \r4, \r4
    vpslld    $11, %ymm2, %ymm2
    vpxor    %ymm2, \r4, \r4
    vpaddd    %ymm6, \r4, \r4
.endm

.macro sha256_avx2_main_quadround i
    sha256_avx2_main_round \i+0, %ymm10, %ymm9, %ymm8, %ymm0, %ymm3, %ymm4, %ymm5, %ymm7
    sha256_avx2_main_round \i+1, %ymm9, %ymm8, %ymm0, %ymm10, %ymm4, %ymm5, %ymm7, %ymm3
    sha256_avx2_main_round \i+2, %ymm8, %ymm0, %ymm10, %ymm9, %ymm5, %ymm7, %ymm3, %ymm4
    sha256_avx2_main_round \i+3, %ymm0, %ymm10, %ymm9, %ymm8, %ymm7, %ymm3, %ymm4, %ymm5
.endm

.macro sha256_xop_extend_round i
    vmovdqa    (\i-15)*16(%rax), %xmm0
    vprotd    $25, %xmm0, %xmm1
    vprotd    $14, %xmm0, %xmm2
    vpsrld    $3, %xmm0, %xmm0
    vpxor    %xmm1, %xmm2, %xmm2
    vpxor    %xmm2, %xmm0, %xmm0

    vpaddd    (\i-16)*16(%rax), %xmm0, %xmm0
    vpaddd    (\i-7)*16(%rax), %xmm0, %xmm0

    vprotd    $15, %xmm3, %xmm1
    vprotd    $13, %xmm3, %xmm2
    vpsrld    $10, %xmm3, %xmm3
    vpxor    %xmm1, %xmm2, %xmm2
    vpxor    %xmm2, %xmm3, %xmm3
    vpaddd    %xmm0, %xmm3, %xmm3
    vmovdqa    %xmm3, \i*16(%rax)
.endm

.macro sha256_xop_extend_doubleround i
    vmovdqa    (\i-15)*16(%rax), %xmm0
    vmovdqa    (\i-14)*16(%rax), %xmm4
    vprotd    $25, %xmm0, %xmm1
    vprotd    $25, %xmm4, %xmm5
    vprotd    $14, %xmm0, %xmm2
    vprotd    $14, %xmm4, %xmm6
    vpxor    %xmm1, %xmm2, %xmm2
    vpxor    %xmm5, %xmm6, %xmm6
    vpsrld    $3, %xmm0, %xmm0
    vpsrld    $3, %xmm4, %xmm4
    vpxor    %xmm2, %xmm0, %xmm0
    vpxor    %xmm6, %xmm4, %xmm4

    vpaddd    (\i-16)*16(%rax), %xmm0, %xmm0
    vpaddd    (\i-15)*16(%rax), %xmm4, %xmm4

    vprotd    $15, %xmm3, %xmm1
    vprotd    $15, %xmm7, %xmm5
    vprotd    $13, %xmm3, %xmm2
    vprotd    $13, %xmm7, %xmm6
    vpxor    %xmm1, %xmm2, %xmm2
    vpxor    %xmm5, %xmm6, %xmm6

    vpaddd    (\i-7)*16(%rax), %xmm0, %xmm0
    vpaddd    (\i-6)*16(%rax), %xmm4, %xmm4

    vpsrld    $10, %xmm3, %xmm3
    vpsrld    $10, %xmm7, %xmm7
    vpxor    %xmm2, %xmm3, %xmm3
    vpxor    %xmm6, %xmm7, %xmm7

    vpaddd    %xmm0, %xmm3, %xmm3
    vpaddd    %xmm4, %xmm7, %xmm7
    vmovdqa    %xmm3, \i*16(%rax)
    vmovdqa    %xmm7, (\i+1)*16(%rax)
.endm
    
.macro sha256_xop_main_round i, r0, r1, r2, r3, r4, r5, r6, r7
    vpaddd    16*(\i)(%rax), \r0, %xmm6
    vpaddd    16*(\i)(%rcx), %xmm6, %xmm6

    vpandn    \r1, \r3, %xmm1
    vpand    \r3, \r2, %xmm2
    vpxor    %xmm2, %xmm1, %xmm1
    vpaddd    %xmm1, %xmm6, %xmm6

    vprotd    $26, \r3, %xmm1
    vprotd    $21, \r3, %xmm2
    vpxor    %xmm1, %xmm2, %xmm2
    vprotd    $7, \r3, \r0
    vpxor    %xmm2, \r0, \r0
    vpaddd    \r0, %xmm6, %xmm6
    vpaddd    %xmm6, \r4, \r0

    vpand    \r6, \r5, %xmm2
    vpand    \r7, \r5, \r4
    vpand    \r7, \r6, %xmm1
    vpxor    \r4, %xmm1, %xmm1
    vpxor    %xmm2, %xmm1, %xmm1
    vpaddd    %xmm1, %xmm6, %xmm6

    vprotd    $30, \r7, %xmm1
    vprotd    $19, \r7, %xmm2
    vpxor    %xmm1, %xmm2, %xmm2
    vprotd    $10, \r7, \r4
    vpxor    %xmm2, \r4, \r4
    vpaddd    %xmm6, \r4, \r4
.endm

.macro sha256_xop_main_quadround i
    sha256_xop_main_round \i+0, %xmm10, %xmm9, %xmm8, %xmm0, %xmm3, %xmm4, %xmm5, %xmm7
    sha256_xop_main_round \i+1, %xmm9, %xmm8, %xmm0, %xmm10, %xmm4, %xmm5, %xmm7, %xmm3
    sha256_xop_main_round \i+2, %xmm8, %xmm0, %xmm10, %xmm9, %xmm5, %xmm7, %xmm3, %xmm4
    sha256_xop_main_round \i+3, %xmm0, %xmm10, %xmm9, %xmm8, %xmm7, %xmm3, %xmm4, %xmm5
.endm

    .text
    .p2align 6
sha256_transform_4way_core_sse2:
    leaq    256(%rsp), %rcx
    leaq    48*16(%rcx), %rax
    movdqa    -2*16(%rcx), %xmm3
    movdqa    -1*16(%rcx), %xmm7
sha256_transform_4way_sse2_extend_loop:
    movdqa    -15*16(%rcx), %xmm0
    movdqa    -14*16(%rcx), %xmm4
    movdqa    %xmm0, %xmm2
    movdqa    %xmm4, %xmm6
    psrld    $3, %xmm0
    psrld    $3, %xmm4
    movdqa    %xmm0, %xmm1
    movdqa    %xmm4, %xmm5
    pslld    $14, %xmm2
    pslld    $14, %xmm6
    psrld    $4, %xmm1
    psrld    $4, %xmm5
    pxor    %xmm1, %xmm0
    pxor    %xmm5, %xmm4
    psrld    $11, %xmm1
    psrld    $11, %xmm5
    pxor    %xmm2, %xmm0
    pxor    %xmm6, %xmm4
    pslld    $11, %xmm2
    pslld    $11, %xmm6
    pxor    %xmm1, %xmm0
    pxor    %xmm5, %xmm4
    pxor    %xmm2, %xmm0
    pxor    %xmm6, %xmm4

    paddd    -16*16(%rcx), %xmm0
    paddd    -15*16(%rcx), %xmm4

    movdqa    %xmm3, %xmm2
    movdqa    %xmm7, %xmm6
    psrld    $10, %xmm3
    psrld    $10, %xmm7
    movdqa    %xmm3, %xmm1
    movdqa    %xmm7, %xmm5
    pslld    $13, %xmm2
    pslld    $13, %xmm6
    psrld    $7, %xmm1
    psrld    $7, %xmm5

    paddd    -7*16(%rcx), %xmm0
    paddd    -6*16(%rcx), %xmm4

    pxor    %xmm1, %xmm3
    pxor    %xmm5, %xmm7
    psrld    $2, %xmm1
    psrld    $2, %xmm5
    pxor    %xmm2, %xmm3
    pxor    %xmm6, %xmm7
    pslld    $2, %xmm2
    pslld    $2, %xmm6
    pxor    %xmm1, %xmm3
    pxor    %xmm5, %xmm7
    pxor    %xmm2, %xmm3
    pxor    %xmm6, %xmm7

    paddd    %xmm0, %xmm3
    paddd    %xmm4, %xmm7
    movdqa    %xmm3, (%rcx)
    movdqa    %xmm7, 16(%rcx)
    addq    $2*16, %rcx
    cmpq    %rcx, %rax
    jne sha256_transform_4way_sse2_extend_loop
    
    movdqu    0(%rdi), %xmm7
    movdqu    16(%rdi), %xmm5
    movdqu    32(%rdi), %xmm4
    movdqu    48(%rdi), %xmm3
    movdqu    64(%rdi), %xmm0
    movdqu    80(%rdi), %xmm8
    movdqu    96(%rdi), %xmm9
    movdqu    112(%rdi), %xmm10
    
    leaq    sha256_4k(%rip), %rcx
    xorq    %rax, %rax
sha256_transform_4way_sse2_main_loop:
    movdqa    (%rsp, %rax), %xmm6
    paddd    (%rcx, %rax), %xmm6
    paddd    %xmm10, %xmm6

    movdqa    %xmm0, %xmm1
    movdqa    %xmm9, %xmm2
    pandn    %xmm2, %xmm1

    movdqa    %xmm2, %xmm10
    movdqa    %xmm8, %xmm2
    movdqa    %xmm2, %xmm9

    pand    %xmm0, %xmm2
    pxor    %xmm2, %xmm1
    movdqa    %xmm0, %xmm8

    paddd    %xmm1, %xmm6

    movdqa    %xmm0, %xmm1
    psrld    $6, %xmm0
    movdqa    %xmm0, %xmm2
    pslld    $7, %xmm1
    psrld    $5, %xmm2
    pxor    %xmm1, %xmm0
    pxor    %xmm2, %xmm0
    pslld    $14, %xmm1
    psrld    $14, %xmm2
    pxor    %xmm1, %xmm0
    pxor    %xmm2, %xmm0
    pslld    $5, %xmm1
    pxor    %xmm1, %xmm0
    paddd    %xmm0, %xmm6

    movdqa    %xmm3, %xmm0
    paddd    %xmm6, %xmm0

    movdqa    %xmm5, %xmm1
    movdqa    %xmm4, %xmm3
    movdqa    %xmm4, %xmm2
    pand    %xmm5, %xmm2
    pand    %xmm7, %xmm4
    pand    %xmm7, %xmm1
    pxor    %xmm4, %xmm1
    movdqa    %xmm5, %xmm4
    movdqa    %xmm7, %xmm5
    pxor    %xmm2, %xmm1
    paddd    %xmm1, %xmm6

    movdqa    %xmm7, %xmm2
    psrld    $2, %xmm7
    movdqa    %xmm7, %xmm1
    pslld    $10, %xmm2
    psrld    $11, %xmm1
    pxor    %xmm2, %xmm7
    pxor    %xmm1, %xmm7
    pslld    $9, %xmm2
    psrld    $9, %xmm1
    pxor    %xmm2, %xmm7
    pxor    %xmm1, %xmm7
    pslld    $11, %xmm2
    pxor    %xmm2, %xmm7
    paddd    %xmm6, %xmm7
    
    addq    $16, %rax
    cmpq    $16*64, %rax
    jne sha256_transform_4way_sse2_main_loop
    jmp sha256_transform_4way_finish

    .text
    .p2align 6
sha256_transform_4way_core_avx:
    leaq    256(%rsp), %rax
    movdqa    -2*16(%rax), %xmm3
    movdqa    -1*16(%rax), %xmm7
    sha256_avx_extend_doubleround 0
    sha256_avx_extend_doubleround 2
    sha256_avx_extend_doubleround 4
    sha256_avx_extend_doubleround 6
    sha256_avx_extend_doubleround 8
    sha256_avx_extend_doubleround 10
    sha256_avx_extend_doubleround 12
    sha256_avx_extend_doubleround 14
    sha256_avx_extend_doubleround 16
    sha256_avx_extend_doubleround 18
    sha256_avx_extend_doubleround 20
    sha256_avx_extend_doubleround 22
    sha256_avx_extend_doubleround 24
    sha256_avx_extend_doubleround 26
    sha256_avx_extend_doubleround 28
    sha256_avx_extend_doubleround 30
    sha256_avx_extend_doubleround 32
    sha256_avx_extend_doubleround 34
    sha256_avx_extend_doubleround 36
    sha256_avx_extend_doubleround 38
    sha256_avx_extend_doubleround 40
    sha256_avx_extend_doubleround 42
    sha256_avx_extend_doubleround 44
    sha256_avx_extend_doubleround 46
    movdqu    0(%rdi), %xmm7
    movdqu    16(%rdi), %xmm5
    movdqu    32(%rdi), %xmm4
    movdqu    48(%rdi), %xmm3
    movdqu    64(%rdi), %xmm0
    movdqu    80(%rdi), %xmm8
    movdqu    96(%rdi), %xmm9
    movdqu    112(%rdi), %xmm10
    movq    %rsp, %rax
    leaq    sha256_4k(%rip), %rcx
    sha256_avx_main_quadround 0
    sha256_avx_main_quadround 4
    sha256_avx_main_quadround 8
    sha256_avx_main_quadround 12
    sha256_avx_main_quadround 16
    sha256_avx_main_quadround 20
    sha256_avx_main_quadround 24
    sha256_avx_main_quadround 28
    sha256_avx_main_quadround 32
    sha256_avx_main_quadround 36
    sha256_avx_main_quadround 40
    sha256_avx_main_quadround 44
    sha256_avx_main_quadround 48
    sha256_avx_main_quadround 52
    sha256_avx_main_quadround 56
    sha256_avx_main_quadround 60
    jmp sha256_transform_4way_finish

    .text
    .p2align 6
sha256_transform_4way_core_xop:
    leaq    256(%rsp), %rax
    movdqa    -2*16(%rax), %xmm3
    movdqa    -1*16(%rax), %xmm7
    sha256_xop_extend_doubleround 0
    sha256_xop_extend_doubleround 2
    sha256_xop_extend_doubleround 4
    sha256_xop_extend_doubleround 6
    sha256_xop_extend_doubleround 8
    sha256_xop_extend_doubleround 10
    sha256_xop_extend_doubleround 12
    sha256_xop_extend_doubleround 14
    sha256_xop_extend_doubleround 16
    sha256_xop_extend_doubleround 18
    sha256_xop_extend_doubleround 20
    sha256_xop_extend_doubleround 22
    sha256_xop_extend_doubleround 24
    sha256_xop_extend_doubleround 26
    sha256_xop_extend_doubleround 28
    sha256_xop_extend_doubleround 30
    sha256_xop_extend_doubleround 32
    sha256_xop_extend_doubleround 34
    sha256_xop_extend_doubleround 36
    sha256_xop_extend_doubleround 38
    sha256_xop_extend_doubleround 40
    sha256_xop_extend_doubleround 42
    sha256_xop_extend_doubleround 44
    sha256_xop_extend_doubleround 46
    movdqu    0(%rdi), %xmm7
    movdqu    16(%rdi), %xmm5
    movdqu    32(%rdi), %xmm4
    movdqu    48(%rdi), %xmm3
    movdqu    64(%rdi), %xmm0
    movdqu    80(%rdi), %xmm8
    movdqu    96(%rdi), %xmm9
    movdqu    112(%rdi), %xmm10
    movq    %rsp, %rax
    leaq    sha256_4k(%rip), %rcx
    sha256_xop_main_quadround 0
    sha256_xop_main_quadround 4
    sha256_xop_main_quadround 8
    sha256_xop_main_quadround 12
    sha256_xop_main_quadround 16
    sha256_xop_main_quadround 20
    sha256_xop_main_quadround 24
    sha256_xop_main_quadround 28
    sha256_xop_main_quadround 32
    sha256_xop_main_quadround 36
    sha256_xop_main_quadround 40
    sha256_xop_main_quadround 44
    sha256_xop_main_quadround 48
    sha256_xop_main_quadround 52
    sha256_xop_main_quadround 56
    sha256_xop_main_quadround 60
    jmp sha256_transform_4way_finish

    .data
    .p2align 3
sha256_transform_4way_core_addr:
    .quad 0x0

.macro p2bswap_rsi_rsp i
    movdqu    \i*16(%rsi), %xmm0
    movdqu    (\i+1)*16(%rsi), %xmm2
    pshuflw    $0xb1, %xmm0, %xmm0
    pshuflw    $0xb1, %xmm2, %xmm2
    pshufhw    $0xb1, %xmm0, %xmm0
    pshufhw    $0xb1, %xmm2, %xmm2
    movdqa    %xmm0, %xmm1
    movdqa    %xmm2, %xmm3
    psrlw    $8, %xmm1
    psrlw    $8, %xmm3
    psllw    $8, %xmm0
    psllw    $8, %xmm2
    pxor    %xmm1, %xmm0
    pxor    %xmm3, %xmm2
    movdqa    %xmm0, \i*16(%rsp)
    movdqa    %xmm2, (\i+1)*16(%rsp)
.endm
    
    .text
    .p2align 6
    .globl sha256_transform_4way
    .globl _sha256_transform_4way
sha256_transform_4way:
_sha256_transform_4way:
#if defined(_WIN64) || defined(__CYGWIN__)
    pushq    %rdi
    subq    $96, %rsp
    movdqa    %xmm6, 0(%rsp)
    movdqa    %xmm7, 16(%rsp)
    movdqa    %xmm8, 32(%rsp)
    movdqa    %xmm9, 48(%rsp)
    movdqa    %xmm10, 64(%rsp)
    movdqa    %xmm11, 80(%rsp)
    pushq    %rsi
    movq    %rcx, %rdi
    movq    %rdx, %rsi
    movq    %r8, %rdx
#endif
    movq    %rsp, %r8
    subq    $1032, %rsp
    andq    $-128, %rsp
    
    testq    %rdx, %rdx
    jnz sha256_transform_4way_swap
    
    movdqu    0*16(%rsi), %xmm0
    movdqu    1*16(%rsi), %xmm1
    movdqu    2*16(%rsi), %xmm2
    movdqu    3*16(%rsi), %xmm3
    movdqu    4*16(%rsi), %xmm4
    movdqu    5*16(%rsi), %xmm5
    movdqu    6*16(%rsi), %xmm6
    movdqu    7*16(%rsi), %xmm7
    movdqa    %xmm0, 0*16(%rsp)
    movdqa    %xmm1, 1*16(%rsp)
    movdqa    %xmm2, 2*16(%rsp)
    movdqa    %xmm3, 3*16(%rsp)
    movdqa    %xmm4, 4*16(%rsp)
    movdqa    %xmm5, 5*16(%rsp)
    movdqa    %xmm6, 6*16(%rsp)
    movdqa    %xmm7, 7*16(%rsp)
    movdqu    8*16(%rsi), %xmm0
    movdqu    9*16(%rsi), %xmm1
    movdqu    10*16(%rsi), %xmm2
    movdqu    11*16(%rsi), %xmm3
    movdqu    12*16(%rsi), %xmm4
    movdqu    13*16(%rsi), %xmm5
    movdqu    14*16(%rsi), %xmm6
    movdqu    15*16(%rsi), %xmm7
    movdqa    %xmm0, 8*16(%rsp)
    movdqa    %xmm1, 9*16(%rsp)
    movdqa    %xmm2, 10*16(%rsp)
    movdqa    %xmm3, 11*16(%rsp)
    movdqa    %xmm4, 12*16(%rsp)
    movdqa    %xmm5, 13*16(%rsp)
    movdqa    %xmm6, 14*16(%rsp)
    movdqa    %xmm7, 15*16(%rsp)
    jmp *sha256_transform_4way_core_addr(%rip)
    
    .p2align 6
sha256_transform_4way_swap:
    p2bswap_rsi_rsp 0
    p2bswap_rsi_rsp 2
    p2bswap_rsi_rsp 4
    p2bswap_rsi_rsp 6
    p2bswap_rsi_rsp 8
    p2bswap_rsi_rsp 10
    p2bswap_rsi_rsp 12
    p2bswap_rsi_rsp 14
    jmp *sha256_transform_4way_core_addr(%rip)
    
    .p2align 6
sha256_transform_4way_finish:
    movdqu    0(%rdi), %xmm2
    movdqu    16(%rdi), %xmm6
    movdqu    32(%rdi), %xmm11
    movdqu    48(%rdi), %xmm1
    paddd    %xmm2, %xmm7
    paddd    %xmm6, %xmm5
    paddd    %xmm11, %xmm4
    paddd    %xmm1, %xmm3
    movdqu    64(%rdi), %xmm2
    movdqu    80(%rdi), %xmm6
    movdqu    96(%rdi), %xmm11
    movdqu    112(%rdi), %xmm1
    paddd    %xmm2, %xmm0
    paddd    %xmm6, %xmm8
    paddd    %xmm11, %xmm9
    paddd    %xmm1, %xmm10
    
    movdqu    %xmm7, 0(%rdi)
    movdqu    %xmm5, 16(%rdi)
    movdqu    %xmm4, 32(%rdi)
    movdqu    %xmm3, 48(%rdi)
    movdqu    %xmm0, 64(%rdi)
    movdqu    %xmm8, 80(%rdi)
    movdqu    %xmm9, 96(%rdi)
    movdqu    %xmm10, 112(%rdi)
    
    movq    %r8, %rsp
#if defined(_WIN64) || defined(__CYGWIN__)
    popq    %rsi
    movdqa    0(%rsp), %xmm6
    movdqa    16(%rsp), %xmm7
    movdqa    32(%rsp), %xmm8
    movdqa    48(%rsp), %xmm9
    movdqa    64(%rsp), %xmm10
    movdqa    80(%rsp), %xmm11
    addq    $96, %rsp
    popq    %rdi
#endif
    ret

    .text
    .p2align 6
sha256_transform_8way_core_avx2:
    leaq    8*64(%rsp), %rax
    vmovdqa    -2*32(%rax), %ymm3
    vmovdqa    -1*32(%rax), %ymm7
    sha256_avx2_extend_doubleround 0
    sha256_avx2_extend_doubleround 2
    sha256_avx2_extend_doubleround 4
    sha256_avx2_extend_doubleround 6
    sha256_avx2_extend_doubleround 8
    sha256_avx2_extend_doubleround 10
    sha256_avx2_extend_doubleround 12
    sha256_avx2_extend_doubleround 14
    sha256_avx2_extend_doubleround 16
    sha256_avx2_extend_doubleround 18
    sha256_avx2_extend_doubleround 20
    sha256_avx2_extend_doubleround 22
    sha256_avx2_extend_doubleround 24
    sha256_avx2_extend_doubleround 26
    sha256_avx2_extend_doubleround 28
    sha256_avx2_extend_doubleround 30
    sha256_avx2_extend_doubleround 32
    sha256_avx2_extend_doubleround 34
    sha256_avx2_extend_doubleround 36
    sha256_avx2_extend_doubleround 38
    sha256_avx2_extend_doubleround 40
    sha256_avx2_extend_doubleround 42
    sha256_avx2_extend_doubleround 44
    sha256_avx2_extend_doubleround 46
    vmovdqu    0*32(%rdi), %ymm7
    vmovdqu    1*32(%rdi), %ymm5
    vmovdqu    2*32(%rdi), %ymm4
    vmovdqu    3*32(%rdi), %ymm3
    vmovdqu    4*32(%rdi), %ymm0
    vmovdqu    5*32(%rdi), %ymm8
    vmovdqu    6*32(%rdi), %ymm9
    vmovdqu    7*32(%rdi), %ymm10
    movq    %rsp, %rax
    leaq    sha256_8k(%rip), %rcx
    sha256_avx2_main_quadround 0
    sha256_avx2_main_quadround 4
    sha256_avx2_main_quadround 8
    sha256_avx2_main_quadround 12
    sha256_avx2_main_quadround 16
    sha256_avx2_main_quadround 20
    sha256_avx2_main_quadround 24
    sha256_avx2_main_quadround 28
    sha256_avx2_main_quadround 32
    sha256_avx2_main_quadround 36
    sha256_avx2_main_quadround 40
    sha256_avx2_main_quadround 44
    sha256_avx2_main_quadround 48
    sha256_avx2_main_quadround 52
    sha256_avx2_main_quadround 56
    sha256_avx2_main_quadround 60
    jmp sha256_transform_8way_finish

.macro p2bswap_avx2_rsi_rsp i
    vmovdqu    \i*32(%rsi), %ymm0
    vmovdqu    (\i+1)*32(%rsi), %ymm2
    vpshuflw    $0xb1, %ymm0, %ymm0
    vpshuflw    $0xb1, %ymm2, %ymm2
    vpshufhw    $0xb1, %ymm0, %ymm0
    vpshufhw    $0xb1, %ymm2, %ymm2
    vpsrlw    $8, %ymm0, %ymm1
    vpsrlw    $8, %ymm2, %ymm3
    vpsllw    $8, %ymm0, %ymm0
    vpsllw    $8, %ymm2, %ymm2
    vpxor    %ymm1, %ymm0, %ymm0
    vpxor    %ymm3, %ymm2, %ymm2
    vmovdqa    %ymm0, \i*32(%rsp)
    vmovdqa    %ymm2, (\i+1)*32(%rsp)
.endm
    
    .text
    .p2align 6
    .globl sha256_transform_8way
    .globl _sha256_transform_8way
sha256_transform_8way:
_sha256_transform_8way:
#if defined(_WIN64) || defined(__CYGWIN__)
    pushq    %rdi
    subq    $96, %rsp
    vmovdqa    %xmm6, 0(%rsp)
    vmovdqa    %xmm7, 16(%rsp)
    vmovdqa    %xmm8, 32(%rsp)
    vmovdqa    %xmm9, 48(%rsp)
    vmovdqa    %xmm10, 64(%rsp)
    vmovdqa    %xmm11, 80(%rsp)
    pushq    %rsi
    movq    %rcx, %rdi
    movq    %rdx, %rsi
    movq    %r8, %rdx
#endif
    movq    %rsp, %r8
    subq    $64*32, %rsp
    andq    $-128, %rsp
    
    testq    %rdx, %rdx
    jnz sha256_transform_8way_swap
    
    vmovdqu    0*32(%rsi), %ymm0
    vmovdqu    1*32(%rsi), %ymm1
    vmovdqu    2*32(%rsi), %ymm2
    vmovdqu    3*32(%rsi), %ymm3
    vmovdqu    4*32(%rsi), %ymm4
    vmovdqu    5*32(%rsi), %ymm5
    vmovdqu    6*32(%rsi), %ymm6
    vmovdqu    7*32(%rsi), %ymm7
    vmovdqa    %ymm0, 0*32(%rsp)
    vmovdqa    %ymm1, 1*32(%rsp)
    vmovdqa    %ymm2, 2*32(%rsp)
    vmovdqa    %ymm3, 3*32(%rsp)
    vmovdqa    %ymm4, 4*32(%rsp)
    vmovdqa    %ymm5, 5*32(%rsp)
    vmovdqa    %ymm6, 6*32(%rsp)
    vmovdqa    %ymm7, 7*32(%rsp)
    vmovdqu    8*32(%rsi), %ymm0
    vmovdqu    9*32(%rsi), %ymm1
    vmovdqu    10*32(%rsi), %ymm2
    vmovdqu    11*32(%rsi), %ymm3
    vmovdqu    12*32(%rsi), %ymm4
    vmovdqu    13*32(%rsi), %ymm5
    vmovdqu    14*32(%rsi), %ymm6
    vmovdqu    15*32(%rsi), %ymm7
    vmovdqa    %ymm0, 8*32(%rsp)
    vmovdqa    %ymm1, 9*32(%rsp)
    vmovdqa    %ymm2, 10*32(%rsp)
    vmovdqa    %ymm3, 11*32(%rsp)
    vmovdqa    %ymm4, 12*32(%rsp)
    vmovdqa    %ymm5, 13*32(%rsp)
    vmovdqa    %ymm6, 14*32(%rsp)
    vmovdqa    %ymm7, 15*32(%rsp)
    jmp sha256_transform_8way_core_avx2
    
    .p2align 6
sha256_transform_8way_swap:
    p2bswap_avx2_rsi_rsp 0
    p2bswap_avx2_rsi_rsp 2
    p2bswap_avx2_rsi_rsp 4
    p2bswap_avx2_rsi_rsp 6
    p2bswap_avx2_rsi_rsp 8
    p2bswap_avx2_rsi_rsp 10
    p2bswap_avx2_rsi_rsp 12
    p2bswap_avx2_rsi_rsp 14
    jmp sha256_transform_8way_core_avx2
    
    .p2align 6
sha256_transform_8way_finish:
    vmovdqu    0*32(%rdi), %ymm2
    vmovdqu    1*32(%rdi), %ymm6
    vmovdqu    2*32(%rdi), %ymm11
    vmovdqu    3*32(%rdi), %ymm1
    vpaddd    %ymm2, %ymm7, %ymm7
    vpaddd    %ymm6, %ymm5, %ymm5
    vpaddd    %ymm11, %ymm4, %ymm4
    vpaddd    %ymm1, %ymm3, %ymm3
    vmovdqu    4*32(%rdi), %ymm2
    vmovdqu    5*32(%rdi), %ymm6
    vmovdqu    6*32(%rdi), %ymm11
    vmovdqu    7*32(%rdi), %ymm1
    vpaddd    %ymm2, %ymm0, %ymm0
    vpaddd    %ymm6, %ymm8, %ymm8
    vpaddd    %ymm11, %ymm9, %ymm9
    vpaddd    %ymm1, %ymm10, %ymm10
    
    vmovdqu    %ymm7, 0*32(%rdi)
    vmovdqu    %ymm5, 1*32(%rdi)
    vmovdqu    %ymm4, 2*32(%rdi)
    vmovdqu    %ymm3, 3*32(%rdi)
    vmovdqu    %ymm0, 4*32(%rdi)
    vmovdqu    %ymm8, 5*32(%rdi)
    vmovdqu    %ymm9, 6*32(%rdi)
    vmovdqu    %ymm10, 7*32(%rdi)
    
    movq    %r8, %rsp
#if defined(_WIN64) || defined(__CYGWIN__)
    popq    %rsi
    vmovdqa    0(%rsp), %xmm6
    vmovdqa    16(%rsp), %xmm7
    vmovdqa    32(%rsp), %xmm8
    vmovdqa    48(%rsp), %xmm9
    vmovdqa    64(%rsp), %xmm10
    vmovdqa    80(%rsp), %xmm11
    addq    $96, %rsp
    popq    %rdi
#endif
    ret
    
    
.macro sha256_sse2_main_round_red i, r7
    movdqa    16*\i(%rax), %xmm6
    paddd    16*\i(%rcx), %xmm6
    paddd    32(%rsp), %xmm6
    movdqa    %xmm0, %xmm1
    movdqa    16(%rsp), %xmm2
    paddd    \r7, %xmm6
    pandn    %xmm2, %xmm1
    movdqa    %xmm2, 32(%rsp)
    movdqa    0(%rsp), %xmm2
    movdqa    %xmm2, 16(%rsp)
    pand    %xmm0, %xmm2
    pxor    %xmm2, %xmm1
    movdqa    %xmm0, 0(%rsp)
    paddd    %xmm1, %xmm6
    movdqa    %xmm0, %xmm1
    psrld    $6, %xmm0
    movdqa    %xmm0, %xmm2
    pslld    $7, %xmm1
    psrld    $5, %xmm2
    pxor    %xmm1, %xmm0
    pxor    %xmm2, %xmm0
    pslld    $14, %xmm1
    psrld    $14, %xmm2
    pxor    %xmm1, %xmm0
    pxor    %xmm2, %xmm0
    pslld    $5, %xmm1
    pxor    %xmm1, %xmm0
    paddd    %xmm6, %xmm0
.endm

.macro sha256_avx_main_round_red i, r0, r1, r2, r3, r4
    vpaddd    16*\i(%rax), \r0, %xmm6
    vpaddd    16*\i(%rcx), %xmm6, %xmm6
    vpandn    \r1, \r3, %xmm1
    vpand    \r3, \r2, %xmm2
    vpxor    %xmm2, %xmm1, %xmm1
    vpaddd    %xmm1, %xmm6, %xmm6
    vpslld    $7, \r3, %xmm1
    vpsrld    $6, \r3, \r0
    vpsrld    $5, \r0, %xmm2
    vpxor    %xmm1, \r0, \r0
    vpxor    %xmm2, \r0, \r0
    vpslld    $14, %xmm1, %xmm1
    vpsrld    $14, %xmm2, %xmm2
    vpxor    %xmm1, \r0, \r0
    vpxor    %xmm2, \r0, \r0
    vpslld    $5, %xmm1, %xmm1
    vpxor    %xmm1, \r0, \r0
    vpaddd    \r0, %xmm6, %xmm6
    vpaddd    %xmm6, \r4, \r0
.endm

.macro sha256_xop_main_round_red i, r0, r1, r2, r3, r4
    vpaddd    16*\i(%rax), \r0, %xmm6
    vpaddd    16*\i(%rcx), %xmm6, %xmm6
    vpandn    \r1, \r3, %xmm1
    vpand    \r3, \r2, %xmm2
    vpxor    %xmm2, %xmm1, %xmm1
    vpaddd    %xmm1, %xmm6, %xmm6
    vprotd    $26, \r3, %xmm1
    vprotd    $21, \r3, %xmm2
    vpxor    %xmm1, %xmm2, %xmm2
    vprotd    $7, \r3, \r0
    vpxor    %xmm2, \r0, \r0
    vpaddd    \r0, %xmm6, %xmm6
    vpaddd    %xmm6, \r4, \r0
.endm
    
    .text
    .p2align 6
    .globl sha256_use_4way
    .globl _sha256_use_4way
sha256_use_4way:
_sha256_use_4way:
    pushq    %rbx
    pushq    %rcx
    pushq    %rdx
    
    /* Check for VIA PadLock Hash Engine */
    movl    $0xc0000000, %eax
    cpuid
    cmpl    $0xc0000001, %eax
    jb    sha256_use_4way_no_phe
    movl    $0xc0000001, %eax
    cpuid
    andl    $0x00000c00, %edx
    cmpl    $0x00000c00, %edx
    jne    sha256_use_4way_no_phe
    leaq    sha256_transform_phe(%rip), %rdx
    movq    %rdx, sha256_transform_addr(%rip)
    xorl    %eax, %eax
    jmp    sha256_use_4way_exit
sha256_use_4way_no_phe:
    /* Check for AVX and OSXSAVE support */
    movl    $1, %eax
    cpuid
    andl    $0x18000000, %ecx
    cmpl    $0x18000000, %ecx
    jne sha256_use_4way_base
    /* Check for XMM and YMM state support */
    xorl    %ecx, %ecx
    xgetbv
    andl    $0x00000006, %eax
    cmpl    $0x00000006, %eax
    jne sha256_use_4way_base
    /* Check for XOP support */
    movl    $0x80000001, %eax
    cpuid
    andl    $0x00000800, %ecx
    jz sha256_use_4way_avx
    
sha256_use_4way_xop:
    leaq    sha256_transform_4way_core_xop(%rip), %rdx
    jmp sha256_use_4way_done
    
sha256_use_4way_avx:
    leaq    sha256_transform_4way_core_avx(%rip), %rdx
    jmp sha256_use_4way_done
    
sha256_use_4way_base:
    leaq    sha256_transform_4way_core_sse2(%rip), %rdx
    
sha256_use_4way_done:
    movq    %rdx, sha256_transform_4way_core_addr(%rip)
    movl    $1, %eax
sha256_use_4way_exit:
    popq    %rdx
    popq    %rcx
    popq    %rbx
    ret

    .text
    .p2align 6
    .globl sha256_use_ssse3
    .globl _sha256_use_ssse3
sha256_use_ssse3:
_sha256_use_ssse3:
    pushq    %rbx
    pushq    %rcx
    pushq    %rdx
    cpuid
    andl    $0x00000200, %ecx
    jz sha256_use_ssse3_done
    xorl    %eax, %eax
    popq    %rdx
    popq    %rcx
    popq    %rbx
    ret
    
sha256_use_ssse3_done:
    movl    $1, %eax
    popq    %rdx
    popq    %rcx
    popq    %rbx
    ret

.macro sha256_avx2_main_round_red i, r0, r1, r2, r3, r4
    vpaddd    32*\i(%rax), \r0, %ymm6
    vpaddd    32*\i(%rcx), %ymm6, %ymm6
    vpandn    \r1, \r3, %ymm1
    vpand    \r3, \r2, %ymm2
    vpxor    %ymm2, %ymm1, %ymm1
    vpaddd    %ymm1, %ymm6, %ymm6
    vpslld    $7, \r3, %ymm1
    vpsrld    $6, \r3, \r0
    vpsrld    $5, \r0, %ymm2
    vpxor    %ymm1, \r0, \r0
    vpxor    %ymm2, \r0, \r0
    vpslld    $14, %ymm1, %ymm1
    vpsrld    $14, %ymm2, %ymm2
    vpxor    %ymm1, \r0, \r0
    vpxor    %ymm2, \r0, \r0
    vpslld    $5, %ymm1, %ymm1
    vpxor    %ymm1, \r0, \r0
    vpaddd    \r0, %ymm6, %ymm6
    vpaddd    %ymm6, \r4, \r0
.endm

    .text
    .p2align 6
    .globl sha256_use_8way
    .globl _sha256_use_8way
sha256_use_8way:
_sha256_use_8way:
    
    pushq    %rbx
    /* Check for AVX and OSXSAVE support */
    movl    $1, %eax
    cpuid
    andl    $0x18000000, %ecx
    cmpl    $0x18000000, %ecx
    jne sha256_use_8way_no
    /* Check for AVX2 support */
    movl    $7, %eax
    xorl    %ecx, %ecx
    cpuid
    andl    $0x00000020, %ebx
    cmpl    $0x00000020, %ebx
    jne sha256_use_8way_no
    /* Check for XMM and YMM state support */
    xorl    %ecx, %ecx
    xgetbv
    andl    $0x00000006, %eax
    cmpl    $0x00000006, %eax
    jne sha256_use_8way_no
    
sha256_use_8way_yes:
    movl    $1, %eax
    jmp sha256_use_8way_done
    
sha256_use_8way_no:
    xorl    %eax, %eax
    
sha256_use_8way_done:
    popq    %rbx
    ret

#endif
