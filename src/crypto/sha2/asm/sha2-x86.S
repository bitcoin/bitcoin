/*
 * Copyright 2012 pooler@litecoinpool.org
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#if defined(__linux__) && defined(__ELF__)
    .section .note.GNU-stack,"",%progbits
#endif

#if defined(__i386__)

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

    .text
    .p2align 5
    .globl sha256_init_4way
    .globl _sha256_init_4way
sha256_init_4way:
_sha256_init_4way:
    movl    4(%esp), %edx
    movdqa    sha256_4h+0, %xmm0
    movdqa    sha256_4h+16, %xmm1
    movdqa    sha256_4h+32, %xmm2
    movdqa    sha256_4h+48, %xmm3
    movdqu    %xmm0, 0(%edx)
    movdqu    %xmm1, 16(%edx)
    movdqu    %xmm2, 32(%edx)
    movdqu    %xmm3, 48(%edx)
    movdqa    sha256_4h+64, %xmm0
    movdqa    sha256_4h+80, %xmm1
    movdqa    sha256_4h+96, %xmm2
    movdqa    sha256_4h+112, %xmm3
    movdqu    %xmm0, 64(%edx)
    movdqu    %xmm1, 80(%edx)
    movdqu    %xmm2, 96(%edx)
    movdqu    %xmm3, 112(%edx)
    ret


.macro sha256_sse2_extend_round i
    movdqa    (\i-15)*16(%eax), %xmm0
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
    paddd    (\i-16)*16(%eax), %xmm0
    paddd    (\i-7)*16(%eax), %xmm0

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
    movdqa    %xmm3, \i*16(%eax)
.endm

.macro sha256_sse2_extend_doubleround i
    movdqa    (\i-15)*16(%eax), %xmm0
    movdqa    (\i-14)*16(%eax), %xmm4
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

    paddd    (\i-16)*16(%eax), %xmm0
    paddd    (\i-15)*16(%eax), %xmm4

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

    paddd    (\i-7)*16(%eax), %xmm0
    paddd    (\i-6)*16(%eax), %xmm4

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
    movdqa    %xmm3, \i*16(%eax)
    movdqa    %xmm7, (\i+1)*16(%eax)
.endm

.macro sha256_sse2_main_round i
    movdqa    16*(\i)(%eax), %xmm6

    movdqa    %xmm0, %xmm1
    movdqa    16(%esp), %xmm2
    pandn    %xmm2, %xmm1
    paddd    32(%esp), %xmm6

    movdqa    %xmm2, 32(%esp)
    movdqa    0(%esp), %xmm2
    movdqa    %xmm2, 16(%esp)

    pand    %xmm0, %xmm2
    pxor    %xmm2, %xmm1
    movdqa    %xmm0, 0(%esp)

    paddd    %xmm1, %xmm6

    movdqa    %xmm0, %xmm1
    psrld    $6, %xmm0
    paddd    16*(\i)+sha256_4k, %xmm6
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


.macro p2bswap_esi_esp i
    movdqu    \i*16(%esi), %xmm0
    movdqu    (\i+1)*16(%esi), %xmm2
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
    movdqa    %xmm0, (\i+3)*16(%esp)
    movdqa    %xmm2, (\i+4)*16(%esp)
.endm

    .text
    .p2align 5
    .globl sha256_transform_4way
    .globl _sha256_transform_4way
sha256_transform_4way:
_sha256_transform_4way:
    pushl    %edi
    pushl    %esi
    movl    12(%esp), %edi
    movl    16(%esp), %esi
    movl    20(%esp), %ecx
    movl    %esp, %edx
    subl    $67*16, %esp
    andl    $-128, %esp
    
    testl    %ecx, %ecx
    jnz sha256_transform_4way_swap
    
    movdqu    0*16(%esi), %xmm0
    movdqu    1*16(%esi), %xmm1
    movdqu    2*16(%esi), %xmm2
    movdqu    3*16(%esi), %xmm3
    movdqu    4*16(%esi), %xmm4
    movdqu    5*16(%esi), %xmm5
    movdqu    6*16(%esi), %xmm6
    movdqu    7*16(%esi), %xmm7
    movdqa    %xmm0, 3*16(%esp)
    movdqa    %xmm1, 4*16(%esp)
    movdqa    %xmm2, 5*16(%esp)
    movdqa    %xmm3, 6*16(%esp)
    movdqa    %xmm4, 7*16(%esp)
    movdqa    %xmm5, 8*16(%esp)
    movdqa    %xmm6, 9*16(%esp)
    movdqa    %xmm7, 10*16(%esp)
    movdqu    8*16(%esi), %xmm0
    movdqu    9*16(%esi), %xmm1
    movdqu    10*16(%esi), %xmm2
    movdqu    11*16(%esi), %xmm3
    movdqu    12*16(%esi), %xmm4
    movdqu    13*16(%esi), %xmm5
    movdqu    14*16(%esi), %xmm6
    movdqu    15*16(%esi), %xmm7
    movdqa    %xmm0, 11*16(%esp)
    movdqa    %xmm1, 12*16(%esp)
    movdqa    %xmm2, 13*16(%esp)
    movdqa    %xmm3, 14*16(%esp)
    movdqa    %xmm4, 15*16(%esp)
    movdqa    %xmm5, 16*16(%esp)
    movdqa    %xmm6, 17*16(%esp)
    movdqa    %xmm7, 18*16(%esp)
    jmp sha256_transform_4way_extend
    
    .p2align 5
sha256_transform_4way_swap:
    p2bswap_esi_esp 0
    p2bswap_esi_esp 2
    p2bswap_esi_esp 4
    p2bswap_esi_esp 6
    p2bswap_esi_esp 8
    p2bswap_esi_esp 10
    p2bswap_esi_esp 12
    p2bswap_esi_esp 14
    
sha256_transform_4way_extend:
    leal    19*16(%esp), %ecx
    leal    48*16(%ecx), %eax
    movdqa    -2*16(%ecx), %xmm3
    movdqa    -1*16(%ecx), %xmm7
sha256_transform_4way_extend_loop:
    movdqa    -15*16(%ecx), %xmm0
    movdqa    -14*16(%ecx), %xmm4
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

    paddd    -16*16(%ecx), %xmm0
    paddd    -15*16(%ecx), %xmm4

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

    paddd    -7*16(%ecx), %xmm0
    paddd    -6*16(%ecx), %xmm4

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
    movdqa    %xmm3, (%ecx)
    movdqa    %xmm7, 16(%ecx)
    addl    $2*16, %ecx
    cmpl    %ecx, %eax
    jne sha256_transform_4way_extend_loop
    
    movdqu    0(%edi), %xmm7
    movdqu    16(%edi), %xmm5
    movdqu    32(%edi), %xmm4
    movdqu    48(%edi), %xmm3
    movdqu    64(%edi), %xmm0
    movdqu    80(%edi), %xmm1
    movdqu    96(%edi), %xmm2
    movdqu    112(%edi), %xmm6
    movdqa    %xmm1, 0(%esp)
    movdqa    %xmm2, 16(%esp)
    movdqa    %xmm6, 32(%esp)
    
    xorl    %eax, %eax
sha256_transform_4way_main_loop:
    movdqa    3*16(%esp, %eax), %xmm6
    paddd    sha256_4k(%eax), %xmm6
    paddd    32(%esp), %xmm6

    movdqa    %xmm0, %xmm1
    movdqa    16(%esp), %xmm2
    pandn    %xmm2, %xmm1

    movdqa    %xmm2, 32(%esp)
    movdqa    0(%esp), %xmm2
    movdqa    %xmm2, 16(%esp)

    pand    %xmm0, %xmm2
    pxor    %xmm2, %xmm1
    movdqa    %xmm0, 0(%esp)

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
    
    addl    $16, %eax
    cmpl    $16*64, %eax
    jne sha256_transform_4way_main_loop
    
    movdqu    0(%edi), %xmm1
    movdqu    16(%edi), %xmm2
    paddd    %xmm1, %xmm7
    paddd    %xmm2, %xmm5
    movdqu    32(%edi), %xmm1
    movdqu    48(%edi), %xmm2
    paddd    %xmm1, %xmm4
    paddd    %xmm2, %xmm3
    
    movdqu    %xmm7, 0(%edi)
    movdqu    %xmm5, 16(%edi)
    movdqu    %xmm4, 32(%edi)
    movdqu    %xmm3, 48(%edi)
    
    movdqu    64(%edi), %xmm1
    movdqu    80(%edi), %xmm2
    movdqu    96(%edi), %xmm6
    movdqu    112(%edi), %xmm7
    paddd    %xmm1, %xmm0
    paddd    0(%esp), %xmm2
    paddd    16(%esp), %xmm6
    paddd    32(%esp), %xmm7
    
    movdqu    %xmm0, 64(%edi)
    movdqu    %xmm2, 80(%edi)
    movdqu    %xmm6, 96(%edi)
    movdqu    %xmm7, 112(%edi)
    
    movl    %edx, %esp
    popl    %esi
    popl    %edi
    ret

.macro sha256_sse2_main_round_red i, r7
    movdqa    16*(\i)(%eax), %xmm6
    paddd    16*(\i)+sha256_4k, %xmm6
    paddd    32(%esp), %xmm6
    movdqa    %xmm0, %xmm1
    movdqa    16(%esp), %xmm2
    paddd    \r7, %xmm6
    pandn    %xmm2, %xmm1
    movdqa    %xmm2, 32(%esp)
    movdqa    0(%esp), %xmm2
    movdqa    %xmm2, 16(%esp)
    pand    %xmm0, %xmm2
    pxor    %xmm2, %xmm1
    movdqa    %xmm0, 0(%esp)
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

    .text
    .p2align 5
    .globl sha256_use_4way
    .globl _sha256_use_4way
sha256_use_4way:
_sha256_use_4way:
    pushl    %ebx
    
    /* Check for SSE2 availability */
    movl    $1, %eax
    cpuid
    andl    $0x04000000, %edx
    jnz sha256_use_4way_sse2
    xorl    %eax, %eax
    popl    %ebx
    ret
    
sha256_use_4way_sse2:
    movl    $1, %eax
    popl    %ebx
    ret

    .text
    .p2align 5
    .globl sha256_use_ssse3
    .globl _sha256_use_ssse3
sha256_use_ssse3:
_sha256_use_ssse3:
    pushl    %ebx
    
    movl    $1, %eax
    cpuid
    andl    $0x00000200, %ecx
    jnz sha256_use_ssse3_done
    xorl    %eax, %eax
    popl    %ebx
    ret
    
sha256_use_ssse3_done:
    movl    $1, %eax
    popl    %ebx
    ret

#endif
