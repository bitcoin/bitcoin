/*
 * Copyright 2012 pooler@litecoinpool.org
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#if defined(__arm__) && defined(__APCS_32__)

.macro sha256_k
	.align 2
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
.endm

.macro sha256_extend_doubleround_core i, rw, ra, rb, ry, rz
	mov	r12, \ry, ror #17
	add	r11, r11, \ra
	eor	r12, r12, \ry, ror #19
	mov	\ra, lr, ror #7
	eor	r12, r12, \ry, lsr #10
	eor	\ra, \ra, lr, ror #18
	add	r12, r12, r11
	ldr	r11, [\rw, #(\i+2)*4]
	eor	\ra, \ra, lr, lsr #3
	add	\ra, \ra, r12

	mov	r12, \rz, ror #17
	str	\ra, [\rw, #(\i+16)*4]
	add	lr, lr, \rb
	eor	r12, r12, \rz, ror #19
	mov	\rb, r11, ror #7
	eor	r12, r12, \rz, lsr #10
	eor	\rb, \rb, r11, ror #18
	add	lr, lr, r12
	eor	\rb, \rb, r11, lsr #3
	add	\rb, \rb, lr
.endm

.macro sha256_extend_doubleround_head i, rw, ra, rb, ry, rz
	ldr	lr, [\rw, #(\i+1)*4]
	sha256_extend_doubleround_core \i, \rw, \ra, \rb, \ry, \rz
	ldr	lr, [\rw, #(\i+3)*4]
.endm

.macro sha256_extend_doubleround_body i, rw, ra, rb, ry, rz
	str	\rz, [\rw, #(\i+15)*4]
	sha256_extend_doubleround_core \i, \rw, \ra, \rb, \ry, \rz
	ldr	lr, [\rw, #(\i+3)*4]
.endm

.macro sha256_extend_doubleround_foot i, rw, ra, rb, ry, rz
	str	\rz, [\rw, #(\i+15)*4]
	sha256_extend_doubleround_core \i, \rw, \ra, \rb, \ry, \rz
	str	\rb, [\rw, #(\i+17)*4]
.endm

.macro sha256_main_round i, ka, rw, ra, rb, rc, rd, re, rf, rg, rh
	ldr	r12, [\rw, #(\i)*4]
	and	r3, \rf, \re
	bic	lr, \rg, \re
	orr	lr, lr, r3
	ldr	r3, \ka + (\i)*4
	add	\rh, \rh, lr
	eor	lr, \re, \re, ror #5
	add	\rh, \rh, r12
	eor	lr, lr, \re, ror #19
	add	\rh, \rh, r3
	eor	r3, \ra, \rb
	add	\rh, \rh, lr, ror #6

	and	r3, r3, \rc
	eor	r12, \ra, \ra, ror #11
	and	lr, \ra, \rb
	eor	r12, r12, \ra, ror #20
	eor	lr, lr, r3
	add	r3, \rh, lr
	add	\rh, \rh, \rd
	add	\rd, r3, r12, ror #2
.endm

.macro sha256_main_quadround i, ka, rw
	sha256_main_round \i+0, \ka, \rw, r4, r5, r6, r7, r8, r9, r10, r11
	sha256_main_round \i+1, \ka, \rw, r7, r4, r5, r6, r11, r8, r9, r10
	sha256_main_round \i+2, \ka, \rw, r6, r7, r4, r5, r10, r11, r8, r9
	sha256_main_round \i+3, \ka, \rw, r5, r6, r7, r4, r9, r10, r11, r8
.endm


	.text
	.code 32
	.align 2
	.globl sha256_transform
	.globl _sha256_transform
#ifdef __ELF__
	.type sha256_transform, %function
#endif
sha256_transform:
_sha256_transform:
	stmfd	sp!, {r4-r11, lr}
	cmp	r2, #0
	sub	sp, sp, #64*4
	bne	sha256_transform_swap
	
	ldmia	r1!, {r4-r11}
	stmia	sp, {r4-r11}
	add	r3, sp, #8*4
	ldmia	r1, {r4-r11}
	stmia	r3, {r4-r11}
	b	sha256_transform_extend

.macro bswap rd, rn
	eor	r12, \rn, \rn, ror #16
	bic	r12, r12, #0x00ff0000
	mov	\rd, \rn, ror #8
	eor	\rd, \rd, r12, lsr #8
.endm

sha256_transform_swap:
	ldmia	r1!, {r4-r11}
	bswap	r4, r4
	bswap	r5, r5
	bswap	r6, r6
	bswap	r7, r7
	bswap	r8, r8
	bswap	r9, r9
	bswap	r10, r10
	bswap	r11, r11
	stmia	sp, {r4-r11}
	add	r3, sp, #8*4
	ldmia	r1, {r4-r11}
	bswap	r4, r4
	bswap	r5, r5
	bswap	r6, r6
	bswap	r7, r7
	bswap	r8, r8
	bswap	r9, r9
	bswap	r10, r10
	bswap	r11, r11
	stmia	r3, {r4-r11}
	
sha256_transform_extend:
	add	r12, sp, #9*4
	ldr	r11, [sp, #0*4]
	ldmia	r12, {r4-r10}
	sha256_extend_doubleround_head  0, sp, r4, r5, r9, r10
	sha256_extend_doubleround_body  2, sp, r6, r7, r4, r5
	sha256_extend_doubleround_body  4, sp, r8, r9, r6, r7
	sha256_extend_doubleround_body  6, sp, r10, r4, r8, r9
	sha256_extend_doubleround_body  8, sp, r5, r6, r10, r4
	sha256_extend_doubleround_body 10, sp, r7, r8, r5, r6
	sha256_extend_doubleround_body 12, sp, r9, r10, r7, r8
	sha256_extend_doubleround_body 14, sp, r4, r5, r9, r10
	sha256_extend_doubleround_body 16, sp, r6, r7, r4, r5
	sha256_extend_doubleround_body 18, sp, r8, r9, r6, r7
	sha256_extend_doubleround_body 20, sp, r10, r4, r8, r9
	sha256_extend_doubleround_body 22, sp, r5, r6, r10, r4
	sha256_extend_doubleround_body 24, sp, r7, r8, r5, r6
	sha256_extend_doubleround_body 26, sp, r9, r10, r7, r8
	sha256_extend_doubleround_body 28, sp, r4, r5, r9, r10
	sha256_extend_doubleround_body 30, sp, r6, r7, r4, r5
	sha256_extend_doubleround_body 32, sp, r8, r9, r6, r7
	sha256_extend_doubleround_body 34, sp, r10, r4, r8, r9
	sha256_extend_doubleround_body 36, sp, r5, r6, r10, r4
	sha256_extend_doubleround_body 38, sp, r7, r8, r5, r6
	sha256_extend_doubleround_body 40, sp, r9, r10, r7, r8
	sha256_extend_doubleround_body 42, sp, r4, r5, r9, r10
	sha256_extend_doubleround_body 44, sp, r6, r7, r4, r5
	sha256_extend_doubleround_foot 46, sp, r8, r9, r6, r7
	
	ldmia	r0, {r4-r11}
	sha256_main_quadround  0, sha256_transform_k, sp
	sha256_main_quadround  4, sha256_transform_k, sp
	sha256_main_quadround  8, sha256_transform_k, sp
	sha256_main_quadround 12, sha256_transform_k, sp
	sha256_main_quadround 16, sha256_transform_k, sp
	sha256_main_quadround 20, sha256_transform_k, sp
	sha256_main_quadround 24, sha256_transform_k, sp
	sha256_main_quadround 28, sha256_transform_k, sp
	b	sha256_transform_k_over
sha256_transform_k:
	sha256_k
sha256_transform_k_over:
	sha256_main_quadround 32, sha256_transform_k, sp
	sha256_main_quadround 36, sha256_transform_k, sp
	sha256_main_quadround 40, sha256_transform_k, sp
	sha256_main_quadround 44, sha256_transform_k, sp
	sha256_main_quadround 48, sha256_transform_k, sp
	sha256_main_quadround 52, sha256_transform_k, sp
	sha256_main_quadround 56, sha256_transform_k, sp
	sha256_main_quadround 60, sha256_transform_k, sp
	
	ldmia	r0, {r1, r2, r3, r12}
	add	r4, r4, r1
	add	r5, r5, r2
	add	r6, r6, r3
	add	r7, r7, r12
	stmia	r0!, {r4-r7}
	ldmia	r0, {r1, r2, r3, r12}
	add	r8, r8, r1
	add	r9, r9, r2
	add	r10, r10, r3
	add	r11, r11, r12
	stmia	r0, {r8-r11}
	
	add	sp, sp, #64*4
#ifdef __thumb__
	ldmfd	sp!, {r4-r11, lr}
	bx	lr
#else
	ldmfd	sp!, {r4-r11, pc}
#endif

.macro sha256_main_round_red i, ka, rw, rd, re, rf, rg, rh
	ldr	r12, [\rw, #(\i)*4]
	and	r3, \rf, \re
	bic	lr, \rg, \re
	add	\rh, \rh, \rd
	orr	lr, lr, r3
	ldr	r3, \ka + (\i)*4
	add	\rh, \rh, lr
	eor	lr, \re, \re, ror #5
	add	\rh, \rh, r12
	eor	lr, lr, \re, ror #19
	add	\rh, \rh, r3
	add	\rh, \rh, lr, ror #6
.endm
	
	.text
	.code 32
	.align 2
	.globl sha256_init_4way
	.globl _sha256_init_4way
#ifdef __ELF__
	.type sha256_init_4way, %function
#endif
sha256_init_4way:
_sha256_init_4way:
	adr	r12, sha256_4h
	vldmia	r12, {q8-q15}
	vstmia	r0, {q8-q15}
	bx	lr
	.align 4
sha256_4h:
	.long 0x6a09e667, 0x6a09e667, 0x6a09e667, 0x6a09e667
	.long 0xbb67ae85, 0xbb67ae85, 0xbb67ae85, 0xbb67ae85
	.long 0x3c6ef372, 0x3c6ef372, 0x3c6ef372, 0x3c6ef372
	.long 0xa54ff53a, 0xa54ff53a, 0xa54ff53a, 0xa54ff53a
	.long 0x510e527f, 0x510e527f, 0x510e527f, 0x510e527f
	.long 0x9b05688c, 0x9b05688c, 0x9b05688c, 0x9b05688c
	.long 0x1f83d9ab, 0x1f83d9ab, 0x1f83d9ab, 0x1f83d9ab
	.long 0x5be0cd19, 0x5be0cd19, 0x5be0cd19, 0x5be0cd19

.macro sha256_4k
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
.endm

.macro sha256_4way_extend_doubleround_core i, rr, rw, ra, rb, ry, rz
	vadd.u32	q5, q5, \ra
	veor.u32	q4, q4, q0
	vshr.u32	q0, \ry, #19
	vshl.u32	q1, \ry, #32-19
	veor.u32	q4, q4, q0
	vshr.u32	\ra, q6, #7
	vshl.u32	q0, q6, #32-7
	veor.u32	q4, q4, q1
	veor.u32	\ra, \ra, q0
	vshr.u32	q1, \ry, #10
	vshr.u32	q0, q6, #18
	veor.u32	q4, q4, q1
	veor.u32	\ra, \ra, q0
	vshl.u32	q1, q6, #32-18
	vshr.u32	q0, q6, #3
	veor.u32	\ra, \ra, q1
	vadd.u32	q4, q4, q5
	veor.u32	\ra, \ra, q0
	vld1.u32	{q5}, [\rr]!
	vadd.u32	\ra, \ra, q4

	vshr.u32	q4, \rz, #17
	vshl.u32	q0, \rz, #32-17
	vadd.u32	q6, q6, \rb
	vst1.u32	{\ra}, [\rw]!
	veor.u32	q4, q4, q0
	vshr.u32	q0, \rz, #19
	vshl.u32	q1, \rz, #32-19
	veor.u32	q4, q4, q0
	vshr.u32	\rb, q5, #7
	veor.u32	q4, q4, q1
	vshl.u32	q0, q5, #32-7
	vshr.u32	q1, \rz, #10
	veor.u32	\rb, \rb, q0
	vshr.u32	q0, q5, #18
	veor.u32	q4, q4, q1
	veor.u32	\rb, \rb, q0
	vshl.u32	q1, q5, #32-18
	vshr.u32	q0, q5, #3
	veor.u32	\rb, \rb, q1
	vadd.u32	q1, q6, q4
	veor.u32	\rb, \rb, q0
.endm

.macro sha256_4way_extend_doubleround_head i, rr, rw, ra, rb, ry, rz
	vld1.u32	{q6}, [\rr]!
	vshr.u32	q4, \ry, #17
	vshl.u32	q0, \ry, #32-17
	sha256_4way_extend_doubleround_core \i, \rr, \rw, \ra, \rb, \ry, \rz
	vld1.u32	{q6}, [\rr]!
	vadd.u32	\rb, \rb, q1
.endm

.macro sha256_4way_extend_doubleround_body i, rr, rw, ra, rb, ry, rz
	vshr.u32	q4, \ry, #17
	vshl.u32	q0, \ry, #32-17
	vst1.u32	{\rz}, [\rw]!
	sha256_4way_extend_doubleround_core \i, \rr, \rw, \ra, \rb, \ry, \rz
	vld1.u32	{q6}, [\rr]!
	vadd.u32	\rb, \rb, q1
.endm

.macro sha256_4way_extend_doubleround_foot i, rr, rw, ra, rb, ry, rz
	vshr.u32	q4, \ry, #17
	vshl.u32	q0, \ry, #32-17
	vst1.u32	{\rz}, [\rw]!
	sha256_4way_extend_doubleround_core \i, \rr, \rw, \ra, \rb, \ry, \rz
	vadd.u32	\rb, \rb, q1
	vst1.u32	{\rb}, [\rw]!
.endm

.macro sha256_4way_main_round i, rk, rw, ra, rb, rc, rd, re, rf, rg, rh
	vld1.u32	{q8}, [\rw]!
	vand.u32	q9, \rf, \re
	vbic.u32	q10, \rg, \re
	vshr.u32	q11, \re, #5
	vorr.u32	q10, q10, q9
	vld1.u32	{q9}, [\rk]!
	vadd.u32	\rh, \rh, q10
	vshl.u32	q12, \re, #32-5
	veor.u32	q10, \re, q11
	vshr.u32	q11, \re, #19
	veor.u32	q10, q10, q12
	vshl.u32	q12, \re, #32-19
	veor.u32	q10, q10, q11
	vadd.u32	\rh, \rh, q8
	veor.u32	q10, q10, q12
	vadd.u32	\rh, \rh, q9
	veor.u32	q9, \ra, \rb
	vshr.u32	q11, q10, #6
	vshl.u32	q13, q10, #32-6
	vadd.u32	\rh, \rh, q11

	vshr.u32	q11, \ra, #11
	vshl.u32	q12, \ra, #32-11
	veor.u32	q8, \ra, q11
	vand.u32	q10, \ra, \rb
	veor.u32	q8, q8, q12
	vshr.u32	q11, \ra, #20
	vshl.u32	q12, \ra, #32-20
	veor.u32	q8, q8, q11
	vand.u32	q9, q9, \rc
	veor.u32	q8, q8, q12
	vadd.u32	\rh, \rh, q13
	veor.u32	q10, q10, q9
	vshr.u32	q11, q8, #2
	vshl.u32	q12, q8, #32-2
	vadd.u32	q9, \rh, q10
	vadd.u32	q12, q12, q11
	vadd.u32	\rh, \rh, \rd
	vadd.u32	\rd, q9, q12
.endm

.macro sha256_4way_main_quadround i, rk, rw
	sha256_4way_main_round \i+0, \rk, \rw, q0, q1, q2, q3, q4, q5, q6, q7
	sha256_4way_main_round \i+1, \rk, \rw, q3, q0, q1, q2, q7, q4, q5, q6
	sha256_4way_main_round \i+2, \rk, \rw, q2, q3, q0, q1, q6, q7, q4, q5
	sha256_4way_main_round \i+3, \rk, \rw, q1, q2, q3, q0, q5, q6, q7, q4
.endm


	.text
	.code 32
	.align 2
	.globl sha256_transform_4way
	.globl _sha256_transform_4way
#ifdef __ELF__
	.type sha256_transform_4way, %function
#endif
sha256_transform_4way:
_sha256_transform_4way:
	stmfd	sp!, {r4, lr}
	vpush	{q4-q7}
	mov	r12, sp
	sub	sp, sp, #64*16
	bic	sp, sp, #63
	cmp	r2, #0
	bne	sha256_transform_4way_swap
	
	vldmia	r1!, {q0-q7}
	vstmia	sp, {q0-q7}
	add	r3, sp, #8*16
	vldmia	r1, {q8-q15}
	vstmia	r3, {q8-q15}
	b	sha256_transform_4way_extend

sha256_transform_4way_swap:
	vldmia	r1!, {q0-q7}
	vrev32.8	q0, q0
	vrev32.8	q1, q1
	vrev32.8	q2, q2
	vrev32.8	q3, q3
	vldmia	r1, {q8-q15}
	vrev32.8	q4, q4
	vrev32.8	q5, q5
	vrev32.8	q6, q6
	vrev32.8	q7, q7
	vstmia	sp, {q0-q7}
	vrev32.8	q8, q8
	vrev32.8	q9, q9
	vrev32.8	q10, q10
	vrev32.8	q11, q11
	vrev32.8	q12, q12
	vrev32.8	q13, q13
	vrev32.8	q14, q14
	vrev32.8	q15, q15
	add	r3, sp, #8*16
	vstmia	r3, {q8-q15}
	
sha256_transform_4way_extend:
	add	r1, sp, #1*16
	add	r2, sp, #16*16
	vmov.u32	q5, q0
	sha256_4way_extend_doubleround_head  0, r1, r2,  q9, q10, q14, q15
	sha256_4way_extend_doubleround_body  2, r1, r2, q11, q12,  q9, q10
	sha256_4way_extend_doubleround_body  4, r1, r2, q13, q14, q11, q12
	sha256_4way_extend_doubleround_body  6, r1, r2, q15,  q9, q13, q14
	sha256_4way_extend_doubleround_body  8, r1, r2, q10, q11, q15,  q9
	sha256_4way_extend_doubleround_body 10, r1, r2, q12, q13, q10, q11
	sha256_4way_extend_doubleround_body 12, r1, r2, q14, q15, q12, q13
	sha256_4way_extend_doubleround_body 14, r1, r2,  q9, q10, q14, q15
	sha256_4way_extend_doubleround_body 16, r1, r2, q11, q12,  q9, q10
	sha256_4way_extend_doubleround_body 18, r1, r2, q13, q14, q11, q12
	sha256_4way_extend_doubleround_body 20, r1, r2, q15,  q9, q13, q14
	sha256_4way_extend_doubleround_body 22, r1, r2, q10, q11, q15,  q9
	sha256_4way_extend_doubleround_body 24, r1, r2, q12, q13, q10, q11
	sha256_4way_extend_doubleround_body 26, r1, r2, q14, q15, q12, q13
	sha256_4way_extend_doubleround_body 28, r1, r2,  q9, q10, q14, q15
	sha256_4way_extend_doubleround_body 30, r1, r2, q11, q12,  q9, q10
	sha256_4way_extend_doubleround_body 32, r1, r2, q13, q14, q11, q12
	sha256_4way_extend_doubleround_body 34, r1, r2, q15,  q9, q13, q14
	sha256_4way_extend_doubleround_body 36, r1, r2, q10, q11, q15,  q9
	sha256_4way_extend_doubleround_body 38, r1, r2, q12, q13, q10, q11
	sha256_4way_extend_doubleround_body 40, r1, r2, q14, q15, q12, q13
	sha256_4way_extend_doubleround_body 42, r1, r2,  q9, q10, q14, q15
	sha256_4way_extend_doubleround_body 44, r1, r2, q11, q12,  q9, q10
	sha256_4way_extend_doubleround_foot 46, r1, r2, q13, q14, q11, q12
	
	vldmia	r0, {q0-q7}
	adr	r4, sha256_transform_4way_4k
	b	sha256_transform_4way_4k_over
	.align 4
sha256_transform_4way_4k:
	sha256_4k
sha256_transform_4way_4k_over:
	sha256_4way_main_quadround  0, r4, sp
	sha256_4way_main_quadround  4, r4, sp
	sha256_4way_main_quadround  8, r4, sp
	sha256_4way_main_quadround 12, r4, sp
	sha256_4way_main_quadround 16, r4, sp
	sha256_4way_main_quadround 20, r4, sp
	sha256_4way_main_quadround 24, r4, sp
	sha256_4way_main_quadround 28, r4, sp
	sha256_4way_main_quadround 32, r4, sp
	sha256_4way_main_quadround 36, r4, sp
	sha256_4way_main_quadround 40, r4, sp
	sha256_4way_main_quadround 44, r4, sp
	sha256_4way_main_quadround 48, r4, sp
	sha256_4way_main_quadround 52, r4, sp
	sha256_4way_main_quadround 56, r4, sp
	sha256_4way_main_quadround 60, r4, sp
	
	vldmia	r0, {q8-q15}
	vadd.u32	q0, q0, q8
	vadd.u32	q1, q1, q9
	vadd.u32	q2, q2, q10
	vadd.u32	q3, q3, q11
	vadd.u32	q4, q4, q12
	vadd.u32	q5, q5, q13
	vadd.u32	q6, q6, q14
	vadd.u32	q7, q7, q15
	vstmia	r0, {q0-q7}
	
	mov	sp, r12
	vpop	{q4-q7}
	ldmfd	sp!, {r4, pc}
	

.macro sha256_4way_main_round_red i, rk, rw, rd, re, rf, rg, rh
	vld1.u32	{q8}, [\rw]!
	vand.u32	q9, \rf, \re
	vbic.u32	q10, \rg, \re
	vshr.u32	q11, \re, #5
	vorr.u32	q10, q10, q9
	vshl.u32	q12, \re, #32-5
	vadd.u32	\rh, \rh, q10
	veor.u32	q10, \re, q11
	vshr.u32	q11, \re, #19
	veor.u32	q10, q10, q12
	vshl.u32	q12, \re, #32-19
	veor.u32	q10, q10, q11
	vadd.u32	\rh, \rh, q8
	veor.u32	q10, q10, q12
	vld1.u32	{q9}, [\rk]!
	vadd.u32	\rh, \rh, \rd
	vshr.u32	q11, q10, #6
	vadd.u32	\rh, \rh, q9
	vshl.u32	q13, q10, #32-6
	vadd.u32	\rh, \rh, q11
	vadd.u32	\rh, \rh, q13
.endm

	.text
	.code 32
	.align 2
	.globl sha256_use_4way
	.globl _sha256_use_4way
#ifdef __ELF__
	.type sha256_use_4way, %function
#endif
sha256_use_4way:
_sha256_use_4way:
	mov	r0, #1
	bx	lr

#endif
