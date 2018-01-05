/* $Id: sha2.c 227 2010-06-16 17:28:38Z tp $ */
/*
 * SHA-224 / SHA-256 implementation.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2007-2010  Projet RNRT SAPHIR
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#include <stddef.h>
#include <string.h>

#include "sph_sha2.h"

#ifdef __cplusplus
 extern "C"{
#endif

#if SPH_SMALL_FOOTPRINT && !defined SPH_SMALL_FOOTPRINT_SHA2
#define SPH_SMALL_FOOTPRINT_SHA2   1
#endif

#define CH(X, Y, Z)    ((((Y) ^ (Z)) & (X)) ^ (Z))
#define MAJ(X, Y, Z)   (((Y) & (Z)) | (((Y) | (Z)) & (X)))

#define ROTR    SPH_ROTR32

#define BSG2_0(x)      (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define BSG2_1(x)      (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SSG2_0(x)      (ROTR(x, 7) ^ ROTR(x, 18) ^ SPH_T32((x) >> 3))
#define SSG2_1(x)      (ROTR(x, 17) ^ ROTR(x, 19) ^ SPH_T32((x) >> 10))

static const sph_u32 H224[8] = {
	SPH_C32(0xC1059ED8), SPH_C32(0x367CD507), SPH_C32(0x3070DD17),
	SPH_C32(0xF70E5939), SPH_C32(0xFFC00B31), SPH_C32(0x68581511),
	SPH_C32(0x64F98FA7), SPH_C32(0xBEFA4FA4)
};

static const sph_u32 H256[8] = {
	SPH_C32(0x6A09E667), SPH_C32(0xBB67AE85), SPH_C32(0x3C6EF372),
	SPH_C32(0xA54FF53A), SPH_C32(0x510E527F), SPH_C32(0x9B05688C),
	SPH_C32(0x1F83D9AB), SPH_C32(0x5BE0CD19)
};

/*
 * The SHA2_ROUND_BODY defines the body for a SHA-224 / SHA-256
 * compression function implementation. The "in" parameter should
 * evaluate, when applied to a numerical input parameter from 0 to 15,
 * to an expression which yields the corresponding input block. The "r"
 * parameter should evaluate to an array or pointer expression
 * designating the array of 8 words which contains the input and output
 * of the compression function.
 */

#if SPH_SMALL_FOOTPRINT_SHA2

static const sph_u32 K[64] = {
	SPH_C32(0x428A2F98), SPH_C32(0x71374491),
	SPH_C32(0xB5C0FBCF), SPH_C32(0xE9B5DBA5),
	SPH_C32(0x3956C25B), SPH_C32(0x59F111F1),
	SPH_C32(0x923F82A4), SPH_C32(0xAB1C5ED5),
	SPH_C32(0xD807AA98), SPH_C32(0x12835B01),
	SPH_C32(0x243185BE), SPH_C32(0x550C7DC3),
	SPH_C32(0x72BE5D74), SPH_C32(0x80DEB1FE),
	SPH_C32(0x9BDC06A7), SPH_C32(0xC19BF174),
	SPH_C32(0xE49B69C1), SPH_C32(0xEFBE4786),
	SPH_C32(0x0FC19DC6), SPH_C32(0x240CA1CC),
	SPH_C32(0x2DE92C6F), SPH_C32(0x4A7484AA),
	SPH_C32(0x5CB0A9DC), SPH_C32(0x76F988DA),
	SPH_C32(0x983E5152), SPH_C32(0xA831C66D),
	SPH_C32(0xB00327C8), SPH_C32(0xBF597FC7),
	SPH_C32(0xC6E00BF3), SPH_C32(0xD5A79147),
	SPH_C32(0x06CA6351), SPH_C32(0x14292967),
	SPH_C32(0x27B70A85), SPH_C32(0x2E1B2138),
	SPH_C32(0x4D2C6DFC), SPH_C32(0x53380D13),
	SPH_C32(0x650A7354), SPH_C32(0x766A0ABB),
	SPH_C32(0x81C2C92E), SPH_C32(0x92722C85),
	SPH_C32(0xA2BFE8A1), SPH_C32(0xA81A664B),
	SPH_C32(0xC24B8B70), SPH_C32(0xC76C51A3),
	SPH_C32(0xD192E819), SPH_C32(0xD6990624),
	SPH_C32(0xF40E3585), SPH_C32(0x106AA070),
	SPH_C32(0x19A4C116), SPH_C32(0x1E376C08),
	SPH_C32(0x2748774C), SPH_C32(0x34B0BCB5),
	SPH_C32(0x391C0CB3), SPH_C32(0x4ED8AA4A),
	SPH_C32(0x5B9CCA4F), SPH_C32(0x682E6FF3),
	SPH_C32(0x748F82EE), SPH_C32(0x78A5636F),
	SPH_C32(0x84C87814), SPH_C32(0x8CC70208),
	SPH_C32(0x90BEFFFA), SPH_C32(0xA4506CEB),
	SPH_C32(0xBEF9A3F7), SPH_C32(0xC67178F2)
};

#define SHA2_MEXP1(in, pc)   do { \
		W[pc] = in(pc); \
	} while (0)

#define SHA2_MEXP2(in, pc)   do { \
		W[(pc) & 0x0F] = SPH_T32(SSG2_1(W[((pc) - 2) & 0x0F]) \
			+ W[((pc) - 7) & 0x0F] \
			+ SSG2_0(W[((pc) - 15) & 0x0F]) + W[(pc) & 0x0F]); \
	} while (0)

#define SHA2_STEPn(n, a, b, c, d, e, f, g, h, in, pc)   do { \
		sph_u32 t1, t2; \
		SHA2_MEXP ## n(in, pc); \
		t1 = SPH_T32(h + BSG2_1(e) + CH(e, f, g) \
			+ K[pcount + (pc)] + W[(pc) & 0x0F]); \
		t2 = SPH_T32(BSG2_0(a) + MAJ(a, b, c)); \
		d = SPH_T32(d + t1); \
		h = SPH_T32(t1 + t2); \
	} while (0)

#define SHA2_STEP1(a, b, c, d, e, f, g, h, in, pc) \
	SHA2_STEPn(1, a, b, c, d, e, f, g, h, in, pc)
#define SHA2_STEP2(a, b, c, d, e, f, g, h, in, pc) \
	SHA2_STEPn(2, a, b, c, d, e, f, g, h, in, pc)

#define SHA2_ROUND_BODY(in, r)   do { \
		sph_u32 A, B, C, D, E, F, G, H; \
		sph_u32 W[16]; \
		unsigned pcount; \
 \
		A = (r)[0]; \
		B = (r)[1]; \
		C = (r)[2]; \
		D = (r)[3]; \
		E = (r)[4]; \
		F = (r)[5]; \
		G = (r)[6]; \
		H = (r)[7]; \
		pcount = 0; \
		SHA2_STEP1(A, B, C, D, E, F, G, H, in,  0); \
		SHA2_STEP1(H, A, B, C, D, E, F, G, in,  1); \
		SHA2_STEP1(G, H, A, B, C, D, E, F, in,  2); \
		SHA2_STEP1(F, G, H, A, B, C, D, E, in,  3); \
		SHA2_STEP1(E, F, G, H, A, B, C, D, in,  4); \
		SHA2_STEP1(D, E, F, G, H, A, B, C, in,  5); \
		SHA2_STEP1(C, D, E, F, G, H, A, B, in,  6); \
		SHA2_STEP1(B, C, D, E, F, G, H, A, in,  7); \
		SHA2_STEP1(A, B, C, D, E, F, G, H, in,  8); \
		SHA2_STEP1(H, A, B, C, D, E, F, G, in,  9); \
		SHA2_STEP1(G, H, A, B, C, D, E, F, in, 10); \
		SHA2_STEP1(F, G, H, A, B, C, D, E, in, 11); \
		SHA2_STEP1(E, F, G, H, A, B, C, D, in, 12); \
		SHA2_STEP1(D, E, F, G, H, A, B, C, in, 13); \
		SHA2_STEP1(C, D, E, F, G, H, A, B, in, 14); \
		SHA2_STEP1(B, C, D, E, F, G, H, A, in, 15); \
		for (pcount = 16; pcount < 64; pcount += 16) { \
			SHA2_STEP2(A, B, C, D, E, F, G, H, in,  0); \
			SHA2_STEP2(H, A, B, C, D, E, F, G, in,  1); \
			SHA2_STEP2(G, H, A, B, C, D, E, F, in,  2); \
			SHA2_STEP2(F, G, H, A, B, C, D, E, in,  3); \
			SHA2_STEP2(E, F, G, H, A, B, C, D, in,  4); \
			SHA2_STEP2(D, E, F, G, H, A, B, C, in,  5); \
			SHA2_STEP2(C, D, E, F, G, H, A, B, in,  6); \
			SHA2_STEP2(B, C, D, E, F, G, H, A, in,  7); \
			SHA2_STEP2(A, B, C, D, E, F, G, H, in,  8); \
			SHA2_STEP2(H, A, B, C, D, E, F, G, in,  9); \
			SHA2_STEP2(G, H, A, B, C, D, E, F, in, 10); \
			SHA2_STEP2(F, G, H, A, B, C, D, E, in, 11); \
			SHA2_STEP2(E, F, G, H, A, B, C, D, in, 12); \
			SHA2_STEP2(D, E, F, G, H, A, B, C, in, 13); \
			SHA2_STEP2(C, D, E, F, G, H, A, B, in, 14); \
			SHA2_STEP2(B, C, D, E, F, G, H, A, in, 15); \
		} \
		(r)[0] = SPH_T32((r)[0] + A); \
		(r)[1] = SPH_T32((r)[1] + B); \
		(r)[2] = SPH_T32((r)[2] + C); \
		(r)[3] = SPH_T32((r)[3] + D); \
		(r)[4] = SPH_T32((r)[4] + E); \
		(r)[5] = SPH_T32((r)[5] + F); \
		(r)[6] = SPH_T32((r)[6] + G); \
		(r)[7] = SPH_T32((r)[7] + H); \
	} while (0)

#else

#define SHA2_ROUND_BODY(in, r)   do { \
		sph_u32 A, B, C, D, E, F, G, H, T1, T2; \
		sph_u32 W00, W01, W02, W03, W04, W05, W06, W07; \
		sph_u32 W08, W09, W10, W11, W12, W13, W14, W15; \
		int i; \
 \
/* for (i=0;i<8;i++) {printf("in[%d]=%08x in[%d]=%08x \n",2*i,in(2*i),2*i+1,in(2*i+1));} */ \
 		A = (r)[0]; \
		B = (r)[1]; \
		C = (r)[2]; \
		D = (r)[3]; \
		E = (r)[4]; \
		F = (r)[5]; \
		G = (r)[6]; \
		H = (r)[7]; \
		W00 = in(0); \
		T1 = SPH_T32(H + BSG2_1(E) + CH(E, F, G) \
			+ SPH_C32(0x428A2F98) + W00); \
		T2 = SPH_T32(BSG2_0(A) + MAJ(A, B, C)); \
		D = SPH_T32(D + T1); \
		H = SPH_T32(T1 + T2); \
		W01 = in(1); \
		T1 = SPH_T32(G + BSG2_1(D) + CH(D, E, F) \
			+ SPH_C32(0x71374491) + W01); \
		T2 = SPH_T32(BSG2_0(H) + MAJ(H, A, B)); \
		C = SPH_T32(C + T1); \
		G = SPH_T32(T1 + T2); \
		W02 = in(2); \
		T1 = SPH_T32(F + BSG2_1(C) + CH(C, D, E) \
			+ SPH_C32(0xB5C0FBCF) + W02); \
		T2 = SPH_T32(BSG2_0(G) + MAJ(G, H, A)); \
		B = SPH_T32(B + T1); \
		F = SPH_T32(T1 + T2); \
		W03 = in(3); \
		T1 = SPH_T32(E + BSG2_1(B) + CH(B, C, D) \
			+ SPH_C32(0xE9B5DBA5) + W03); \
		T2 = SPH_T32(BSG2_0(F) + MAJ(F, G, H)); \
		A = SPH_T32(A + T1); \
		E = SPH_T32(T1 + T2); \
		W04 = in(4); \
		T1 = SPH_T32(D + BSG2_1(A) + CH(A, B, C) \
			+ SPH_C32(0x3956C25B) + W04); \
		T2 = SPH_T32(BSG2_0(E) + MAJ(E, F, G)); \
		H = SPH_T32(H + T1); \
		D = SPH_T32(T1 + T2); \
		W05 = in(5); \
		T1 = SPH_T32(C + BSG2_1(H) + CH(H, A, B) \
			+ SPH_C32(0x59F111F1) + W05); \
		T2 = SPH_T32(BSG2_0(D) + MAJ(D, E, F)); \
		G = SPH_T32(G + T1); \
		C = SPH_T32(T1 + T2); \
		W06 = in(6); \
		T1 = SPH_T32(B + BSG2_1(G) + CH(G, H, A) \
			+ SPH_C32(0x923F82A4) + W06); \
		T2 = SPH_T32(BSG2_0(C) + MAJ(C, D, E)); \
		F = SPH_T32(F + T1); \
		B = SPH_T32(T1 + T2); \
		W07 = in(7); \
		T1 = SPH_T32(A + BSG2_1(F) + CH(F, G, H) \
			+ SPH_C32(0xAB1C5ED5) + W07); \
		T2 = SPH_T32(BSG2_0(B) + MAJ(B, C, D)); \
		E = SPH_T32(E + T1); \
		A = SPH_T32(T1 + T2); \
		W08 = in(8); \
		T1 = SPH_T32(H + BSG2_1(E) + CH(E, F, G) \
			+ SPH_C32(0xD807AA98) + W08); \
		T2 = SPH_T32(BSG2_0(A) + MAJ(A, B, C)); \
		D = SPH_T32(D + T1); \
		H = SPH_T32(T1 + T2); \
		W09 = in(9); \
		T1 = SPH_T32(G + BSG2_1(D) + CH(D, E, F) \
			+ SPH_C32(0x12835B01) + W09); \
		T2 = SPH_T32(BSG2_0(H) + MAJ(H, A, B)); \
		C = SPH_T32(C + T1); \
		G = SPH_T32(T1 + T2); \
		W10 = in(10); \
		T1 = SPH_T32(F + BSG2_1(C) + CH(C, D, E) \
			+ SPH_C32(0x243185BE) + W10); \
		T2 = SPH_T32(BSG2_0(G) + MAJ(G, H, A)); \
		B = SPH_T32(B + T1); \
		F = SPH_T32(T1 + T2); \
		W11 = in(11); \
		T1 = SPH_T32(E + BSG2_1(B) + CH(B, C, D) \
			+ SPH_C32(0x550C7DC3) + W11); \
		T2 = SPH_T32(BSG2_0(F) + MAJ(F, G, H)); \
		A = SPH_T32(A + T1); \
		E = SPH_T32(T1 + T2); \
		W12 = in(12); \
		T1 = SPH_T32(D + BSG2_1(A) + CH(A, B, C) \
			+ SPH_C32(0x72BE5D74) + W12); \
		T2 = SPH_T32(BSG2_0(E) + MAJ(E, F, G)); \
		H = SPH_T32(H + T1); \
		D = SPH_T32(T1 + T2); \
		W13 = in(13); \
		T1 = SPH_T32(C + BSG2_1(H) + CH(H, A, B) \
			+ SPH_C32(0x80DEB1FE) + W13); \
		T2 = SPH_T32(BSG2_0(D) + MAJ(D, E, F)); \
		G = SPH_T32(G + T1); \
		C = SPH_T32(T1 + T2); \
		W14 = in(14); \
		T1 = SPH_T32(B + BSG2_1(G) + CH(G, H, A) \
			+ SPH_C32(0x9BDC06A7) + W14); \
		T2 = SPH_T32(BSG2_0(C) + MAJ(C, D, E)); \
		F = SPH_T32(F + T1); \
		B = SPH_T32(T1 + T2); \
		W15 = in(15); \
		T1 = SPH_T32(A + BSG2_1(F) + CH(F, G, H) \
			+ SPH_C32(0xC19BF174) + W15); \
		T2 = SPH_T32(BSG2_0(B) + MAJ(B, C, D)); \
		E = SPH_T32(E + T1); \
		A = SPH_T32(T1 + T2); \
		W00 = SPH_T32(SSG2_1(W14) + W09 + SSG2_0(W01) + W00); \
		T1 = SPH_T32(H + BSG2_1(E) + CH(E, F, G) \
			+ SPH_C32(0xE49B69C1) + W00); \
		T2 = SPH_T32(BSG2_0(A) + MAJ(A, B, C)); \
		D = SPH_T32(D + T1); \
		H = SPH_T32(T1 + T2); \
		W01 = SPH_T32(SSG2_1(W15) + W10 + SSG2_0(W02) + W01); \
		T1 = SPH_T32(G + BSG2_1(D) + CH(D, E, F) \
			+ SPH_C32(0xEFBE4786) + W01); \
		T2 = SPH_T32(BSG2_0(H) + MAJ(H, A, B)); \
		C = SPH_T32(C + T1); \
		G = SPH_T32(T1 + T2); \
		W02 = SPH_T32(SSG2_1(W00) + W11 + SSG2_0(W03) + W02); \
		T1 = SPH_T32(F + BSG2_1(C) + CH(C, D, E) \
			+ SPH_C32(0x0FC19DC6) + W02); \
		T2 = SPH_T32(BSG2_0(G) + MAJ(G, H, A)); \
		B = SPH_T32(B + T1); \
		F = SPH_T32(T1 + T2); \
		W03 = SPH_T32(SSG2_1(W01) + W12 + SSG2_0(W04) + W03); \
		T1 = SPH_T32(E + BSG2_1(B) + CH(B, C, D) \
			+ SPH_C32(0x240CA1CC) + W03); \
		T2 = SPH_T32(BSG2_0(F) + MAJ(F, G, H)); \
		A = SPH_T32(A + T1); \
		E = SPH_T32(T1 + T2); \
		W04 = SPH_T32(SSG2_1(W02) + W13 + SSG2_0(W05) + W04); \
		T1 = SPH_T32(D + BSG2_1(A) + CH(A, B, C) \
			+ SPH_C32(0x2DE92C6F) + W04); \
		T2 = SPH_T32(BSG2_0(E) + MAJ(E, F, G)); \
		H = SPH_T32(H + T1); \
		D = SPH_T32(T1 + T2); \
		W05 = SPH_T32(SSG2_1(W03) + W14 + SSG2_0(W06) + W05); \
		T1 = SPH_T32(C + BSG2_1(H) + CH(H, A, B) \
			+ SPH_C32(0x4A7484AA) + W05); \
		T2 = SPH_T32(BSG2_0(D) + MAJ(D, E, F)); \
		G = SPH_T32(G + T1); \
		C = SPH_T32(T1 + T2); \
		W06 = SPH_T32(SSG2_1(W04) + W15 + SSG2_0(W07) + W06); \
		T1 = SPH_T32(B + BSG2_1(G) + CH(G, H, A) \
			+ SPH_C32(0x5CB0A9DC) + W06); \
		T2 = SPH_T32(BSG2_0(C) + MAJ(C, D, E)); \
		F = SPH_T32(F + T1); \
		B = SPH_T32(T1 + T2); \
		W07 = SPH_T32(SSG2_1(W05) + W00 + SSG2_0(W08) + W07); \
		T1 = SPH_T32(A + BSG2_1(F) + CH(F, G, H) \
			+ SPH_C32(0x76F988DA) + W07); \
		T2 = SPH_T32(BSG2_0(B) + MAJ(B, C, D)); \
		E = SPH_T32(E + T1); \
		A = SPH_T32(T1 + T2); \
		W08 = SPH_T32(SSG2_1(W06) + W01 + SSG2_0(W09) + W08); \
		T1 = SPH_T32(H + BSG2_1(E) + CH(E, F, G) \
			+ SPH_C32(0x983E5152) + W08); \
		T2 = SPH_T32(BSG2_0(A) + MAJ(A, B, C)); \
		D = SPH_T32(D + T1); \
		H = SPH_T32(T1 + T2); \
		W09 = SPH_T32(SSG2_1(W07) + W02 + SSG2_0(W10) + W09); \
		T1 = SPH_T32(G + BSG2_1(D) + CH(D, E, F) \
			+ SPH_C32(0xA831C66D) + W09); \
		T2 = SPH_T32(BSG2_0(H) + MAJ(H, A, B)); \
		C = SPH_T32(C + T1); \
		G = SPH_T32(T1 + T2); \
		W10 = SPH_T32(SSG2_1(W08) + W03 + SSG2_0(W11) + W10); \
		T1 = SPH_T32(F + BSG2_1(C) + CH(C, D, E) \
			+ SPH_C32(0xB00327C8) + W10); \
		T2 = SPH_T32(BSG2_0(G) + MAJ(G, H, A)); \
		B = SPH_T32(B + T1); \
		F = SPH_T32(T1 + T2); \
		W11 = SPH_T32(SSG2_1(W09) + W04 + SSG2_0(W12) + W11); \
		T1 = SPH_T32(E + BSG2_1(B) + CH(B, C, D) \
			+ SPH_C32(0xBF597FC7) + W11); \
		T2 = SPH_T32(BSG2_0(F) + MAJ(F, G, H)); \
		A = SPH_T32(A + T1); \
		E = SPH_T32(T1 + T2); \
		W12 = SPH_T32(SSG2_1(W10) + W05 + SSG2_0(W13) + W12); \
		T1 = SPH_T32(D + BSG2_1(A) + CH(A, B, C) \
			+ SPH_C32(0xC6E00BF3) + W12); \
		T2 = SPH_T32(BSG2_0(E) + MAJ(E, F, G)); \
		H = SPH_T32(H + T1); \
		D = SPH_T32(T1 + T2); \
		W13 = SPH_T32(SSG2_1(W11) + W06 + SSG2_0(W14) + W13); \
		T1 = SPH_T32(C + BSG2_1(H) + CH(H, A, B) \
			+ SPH_C32(0xD5A79147) + W13); \
		T2 = SPH_T32(BSG2_0(D) + MAJ(D, E, F)); \
		G = SPH_T32(G + T1); \
		C = SPH_T32(T1 + T2); \
		W14 = SPH_T32(SSG2_1(W12) + W07 + SSG2_0(W15) + W14); \
		T1 = SPH_T32(B + BSG2_1(G) + CH(G, H, A) \
			+ SPH_C32(0x06CA6351) + W14); \
		T2 = SPH_T32(BSG2_0(C) + MAJ(C, D, E)); \
		F = SPH_T32(F + T1); \
		B = SPH_T32(T1 + T2); \
		W15 = SPH_T32(SSG2_1(W13) + W08 + SSG2_0(W00) + W15); \
		T1 = SPH_T32(A + BSG2_1(F) + CH(F, G, H) \
			+ SPH_C32(0x14292967) + W15); \
		T2 = SPH_T32(BSG2_0(B) + MAJ(B, C, D)); \
		E = SPH_T32(E + T1); \
		A = SPH_T32(T1 + T2); \
		W00 = SPH_T32(SSG2_1(W14) + W09 + SSG2_0(W01) + W00); \
		T1 = SPH_T32(H + BSG2_1(E) + CH(E, F, G) \
			+ SPH_C32(0x27B70A85) + W00); \
		T2 = SPH_T32(BSG2_0(A) + MAJ(A, B, C)); \
		D = SPH_T32(D + T1); \
		H = SPH_T32(T1 + T2); \
		W01 = SPH_T32(SSG2_1(W15) + W10 + SSG2_0(W02) + W01); \
		T1 = SPH_T32(G + BSG2_1(D) + CH(D, E, F) \
			+ SPH_C32(0x2E1B2138) + W01); \
		T2 = SPH_T32(BSG2_0(H) + MAJ(H, A, B)); \
		C = SPH_T32(C + T1); \
		G = SPH_T32(T1 + T2); \
		W02 = SPH_T32(SSG2_1(W00) + W11 + SSG2_0(W03) + W02); \
		T1 = SPH_T32(F + BSG2_1(C) + CH(C, D, E) \
			+ SPH_C32(0x4D2C6DFC) + W02); \
		T2 = SPH_T32(BSG2_0(G) + MAJ(G, H, A)); \
		B = SPH_T32(B + T1); \
		F = SPH_T32(T1 + T2); \
		W03 = SPH_T32(SSG2_1(W01) + W12 + SSG2_0(W04) + W03); \
		T1 = SPH_T32(E + BSG2_1(B) + CH(B, C, D) \
			+ SPH_C32(0x53380D13) + W03); \
		T2 = SPH_T32(BSG2_0(F) + MAJ(F, G, H)); \
		A = SPH_T32(A + T1); \
		E = SPH_T32(T1 + T2); \
		W04 = SPH_T32(SSG2_1(W02) + W13 + SSG2_0(W05) + W04); \
		T1 = SPH_T32(D + BSG2_1(A) + CH(A, B, C) \
			+ SPH_C32(0x650A7354) + W04); \
		T2 = SPH_T32(BSG2_0(E) + MAJ(E, F, G)); \
		H = SPH_T32(H + T1); \
		D = SPH_T32(T1 + T2); \
		W05 = SPH_T32(SSG2_1(W03) + W14 + SSG2_0(W06) + W05); \
		T1 = SPH_T32(C + BSG2_1(H) + CH(H, A, B) \
			+ SPH_C32(0x766A0ABB) + W05); \
		T2 = SPH_T32(BSG2_0(D) + MAJ(D, E, F)); \
		G = SPH_T32(G + T1); \
		C = SPH_T32(T1 + T2); \
		W06 = SPH_T32(SSG2_1(W04) + W15 + SSG2_0(W07) + W06); \
		T1 = SPH_T32(B + BSG2_1(G) + CH(G, H, A) \
			+ SPH_C32(0x81C2C92E) + W06); \
		T2 = SPH_T32(BSG2_0(C) + MAJ(C, D, E)); \
		F = SPH_T32(F + T1); \
		B = SPH_T32(T1 + T2); \
		W07 = SPH_T32(SSG2_1(W05) + W00 + SSG2_0(W08) + W07); \
		T1 = SPH_T32(A + BSG2_1(F) + CH(F, G, H) \
			+ SPH_C32(0x92722C85) + W07); \
		T2 = SPH_T32(BSG2_0(B) + MAJ(B, C, D)); \
		E = SPH_T32(E + T1); \
		A = SPH_T32(T1 + T2); \
		W08 = SPH_T32(SSG2_1(W06) + W01 + SSG2_0(W09) + W08); \
		T1 = SPH_T32(H + BSG2_1(E) + CH(E, F, G) \
			+ SPH_C32(0xA2BFE8A1) + W08); \
		T2 = SPH_T32(BSG2_0(A) + MAJ(A, B, C)); \
		D = SPH_T32(D + T1); \
		H = SPH_T32(T1 + T2); \
		W09 = SPH_T32(SSG2_1(W07) + W02 + SSG2_0(W10) + W09); \
		T1 = SPH_T32(G + BSG2_1(D) + CH(D, E, F) \
			+ SPH_C32(0xA81A664B) + W09); \
		T2 = SPH_T32(BSG2_0(H) + MAJ(H, A, B)); \
		C = SPH_T32(C + T1); \
		G = SPH_T32(T1 + T2); \
		W10 = SPH_T32(SSG2_1(W08) + W03 + SSG2_0(W11) + W10); \
		T1 = SPH_T32(F + BSG2_1(C) + CH(C, D, E) \
			+ SPH_C32(0xC24B8B70) + W10); \
		T2 = SPH_T32(BSG2_0(G) + MAJ(G, H, A)); \
		B = SPH_T32(B + T1); \
		F = SPH_T32(T1 + T2); \
		W11 = SPH_T32(SSG2_1(W09) + W04 + SSG2_0(W12) + W11); \
		T1 = SPH_T32(E + BSG2_1(B) + CH(B, C, D) \
			+ SPH_C32(0xC76C51A3) + W11); \
		T2 = SPH_T32(BSG2_0(F) + MAJ(F, G, H)); \
		A = SPH_T32(A + T1); \
		E = SPH_T32(T1 + T2); \
		W12 = SPH_T32(SSG2_1(W10) + W05 + SSG2_0(W13) + W12); \
		T1 = SPH_T32(D + BSG2_1(A) + CH(A, B, C) \
			+ SPH_C32(0xD192E819) + W12); \
		T2 = SPH_T32(BSG2_0(E) + MAJ(E, F, G)); \
		H = SPH_T32(H + T1); \
		D = SPH_T32(T1 + T2); \
		W13 = SPH_T32(SSG2_1(W11) + W06 + SSG2_0(W14) + W13); \
		T1 = SPH_T32(C + BSG2_1(H) + CH(H, A, B) \
			+ SPH_C32(0xD6990624) + W13); \
		T2 = SPH_T32(BSG2_0(D) + MAJ(D, E, F)); \
		G = SPH_T32(G + T1); \
		C = SPH_T32(T1 + T2); \
		W14 = SPH_T32(SSG2_1(W12) + W07 + SSG2_0(W15) + W14); \
		T1 = SPH_T32(B + BSG2_1(G) + CH(G, H, A) \
			+ SPH_C32(0xF40E3585) + W14); \
		T2 = SPH_T32(BSG2_0(C) + MAJ(C, D, E)); \
		F = SPH_T32(F + T1); \
		B = SPH_T32(T1 + T2); \
		W15 = SPH_T32(SSG2_1(W13) + W08 + SSG2_0(W00) + W15); \
		T1 = SPH_T32(A + BSG2_1(F) + CH(F, G, H) \
			+ SPH_C32(0x106AA070) + W15); \
		T2 = SPH_T32(BSG2_0(B) + MAJ(B, C, D)); \
		E = SPH_T32(E + T1); \
		A = SPH_T32(T1 + T2); \
		W00 = SPH_T32(SSG2_1(W14) + W09 + SSG2_0(W01) + W00); \
		T1 = SPH_T32(H + BSG2_1(E) + CH(E, F, G) \
			+ SPH_C32(0x19A4C116) + W00); \
		T2 = SPH_T32(BSG2_0(A) + MAJ(A, B, C)); \
		D = SPH_T32(D + T1); \
		H = SPH_T32(T1 + T2); \
		W01 = SPH_T32(SSG2_1(W15) + W10 + SSG2_0(W02) + W01); \
		T1 = SPH_T32(G + BSG2_1(D) + CH(D, E, F) \
			+ SPH_C32(0x1E376C08) + W01); \
		T2 = SPH_T32(BSG2_0(H) + MAJ(H, A, B)); \
		C = SPH_T32(C + T1); \
		G = SPH_T32(T1 + T2); \
		W02 = SPH_T32(SSG2_1(W00) + W11 + SSG2_0(W03) + W02); \
		T1 = SPH_T32(F + BSG2_1(C) + CH(C, D, E) \
			+ SPH_C32(0x2748774C) + W02); \
		T2 = SPH_T32(BSG2_0(G) + MAJ(G, H, A)); \
		B = SPH_T32(B + T1); \
		F = SPH_T32(T1 + T2); \
		W03 = SPH_T32(SSG2_1(W01) + W12 + SSG2_0(W04) + W03); \
		T1 = SPH_T32(E + BSG2_1(B) + CH(B, C, D) \
			+ SPH_C32(0x34B0BCB5) + W03); \
		T2 = SPH_T32(BSG2_0(F) + MAJ(F, G, H)); \
		A = SPH_T32(A + T1); \
		E = SPH_T32(T1 + T2); \
		W04 = SPH_T32(SSG2_1(W02) + W13 + SSG2_0(W05) + W04); \
		T1 = SPH_T32(D + BSG2_1(A) + CH(A, B, C) \
			+ SPH_C32(0x391C0CB3) + W04); \
		T2 = SPH_T32(BSG2_0(E) + MAJ(E, F, G)); \
		H = SPH_T32(H + T1); \
		D = SPH_T32(T1 + T2); \
		W05 = SPH_T32(SSG2_1(W03) + W14 + SSG2_0(W06) + W05); \
		T1 = SPH_T32(C + BSG2_1(H) + CH(H, A, B) \
			+ SPH_C32(0x4ED8AA4A) + W05); \
		T2 = SPH_T32(BSG2_0(D) + MAJ(D, E, F)); \
		G = SPH_T32(G + T1); \
		C = SPH_T32(T1 + T2); \
		W06 = SPH_T32(SSG2_1(W04) + W15 + SSG2_0(W07) + W06); \
		T1 = SPH_T32(B + BSG2_1(G) + CH(G, H, A) \
			+ SPH_C32(0x5B9CCA4F) + W06); \
		T2 = SPH_T32(BSG2_0(C) + MAJ(C, D, E)); \
		F = SPH_T32(F + T1); \
		B = SPH_T32(T1 + T2); \
		W07 = SPH_T32(SSG2_1(W05) + W00 + SSG2_0(W08) + W07); \
		T1 = SPH_T32(A + BSG2_1(F) + CH(F, G, H) \
			+ SPH_C32(0x682E6FF3) + W07); \
		T2 = SPH_T32(BSG2_0(B) + MAJ(B, C, D)); \
		E = SPH_T32(E + T1); \
		A = SPH_T32(T1 + T2); \
		W08 = SPH_T32(SSG2_1(W06) + W01 + SSG2_0(W09) + W08); \
		T1 = SPH_T32(H + BSG2_1(E) + CH(E, F, G) \
			+ SPH_C32(0x748F82EE) + W08); \
		T2 = SPH_T32(BSG2_0(A) + MAJ(A, B, C)); \
		D = SPH_T32(D + T1); \
		H = SPH_T32(T1 + T2); \
		W09 = SPH_T32(SSG2_1(W07) + W02 + SSG2_0(W10) + W09); \
		T1 = SPH_T32(G + BSG2_1(D) + CH(D, E, F) \
			+ SPH_C32(0x78A5636F) + W09); \
		T2 = SPH_T32(BSG2_0(H) + MAJ(H, A, B)); \
		C = SPH_T32(C + T1); \
		G = SPH_T32(T1 + T2); \
		W10 = SPH_T32(SSG2_1(W08) + W03 + SSG2_0(W11) + W10); \
		T1 = SPH_T32(F + BSG2_1(C) + CH(C, D, E) \
			+ SPH_C32(0x84C87814) + W10); \
		T2 = SPH_T32(BSG2_0(G) + MAJ(G, H, A)); \
		B = SPH_T32(B + T1); \
		F = SPH_T32(T1 + T2); \
		W11 = SPH_T32(SSG2_1(W09) + W04 + SSG2_0(W12) + W11); \
		T1 = SPH_T32(E + BSG2_1(B) + CH(B, C, D) \
			+ SPH_C32(0x8CC70208) + W11); \
		T2 = SPH_T32(BSG2_0(F) + MAJ(F, G, H)); \
		A = SPH_T32(A + T1); \
		E = SPH_T32(T1 + T2); \
		W12 = SPH_T32(SSG2_1(W10) + W05 + SSG2_0(W13) + W12); \
		T1 = SPH_T32(D + BSG2_1(A) + CH(A, B, C) \
			+ SPH_C32(0x90BEFFFA) + W12); \
		T2 = SPH_T32(BSG2_0(E) + MAJ(E, F, G)); \
		H = SPH_T32(H + T1); \
		D = SPH_T32(T1 + T2); \
		W13 = SPH_T32(SSG2_1(W11) + W06 + SSG2_0(W14) + W13); \
		T1 = SPH_T32(C + BSG2_1(H) + CH(H, A, B) \
			+ SPH_C32(0xA4506CEB) + W13); \
		T2 = SPH_T32(BSG2_0(D) + MAJ(D, E, F)); \
		G = SPH_T32(G + T1); \
		C = SPH_T32(T1 + T2); \
		W14 = SPH_T32(SSG2_1(W12) + W07 + SSG2_0(W15) + W14); \
		T1 = SPH_T32(B + BSG2_1(G) + CH(G, H, A) \
			+ SPH_C32(0xBEF9A3F7) + W14); \
		T2 = SPH_T32(BSG2_0(C) + MAJ(C, D, E)); \
		F = SPH_T32(F + T1); \
		B = SPH_T32(T1 + T2); \
		W15 = SPH_T32(SSG2_1(W13) + W08 + SSG2_0(W00) + W15); \
		T1 = SPH_T32(A + BSG2_1(F) + CH(F, G, H) \
			+ SPH_C32(0xC67178F2) + W15); \
		T2 = SPH_T32(BSG2_0(B) + MAJ(B, C, D)); \
		E = SPH_T32(E + T1); \
		A = SPH_T32(T1 + T2); \
		(r)[0] = SPH_T32((r)[0] + A); \
		(r)[1] = SPH_T32((r)[1] + B); \
		(r)[2] = SPH_T32((r)[2] + C); \
		(r)[3] = SPH_T32((r)[3] + D); \
		(r)[4] = SPH_T32((r)[4] + E); \
		(r)[5] = SPH_T32((r)[5] + F); \
		(r)[6] = SPH_T32((r)[6] + G); \
		(r)[7] = SPH_T32((r)[7] + H); \
/* for (i=0;i<4;i++) {printf("r[%d]=%08x r[%d]=%08x\n",2*i,(r)[2*i],2*i+1,(r)[2*i+1]);}  */ \
	} while (0)

#endif

/*
 * One round of SHA-224 / SHA-256. The data must be aligned for 32-bit access.
 */
static void
sha2_round(const unsigned char *data, sph_u32 r[8])
{
#define SHA2_IN(x)   sph_dec32be_aligned(data + (4 * (x)))
	SHA2_ROUND_BODY(SHA2_IN, r);
#undef SHA2_IN
}

/* see sph_sha2.h */
void
sph_sha224_init(void *cc)
{
	sph_sha224_context *sc;

	sc = cc;
	memcpy(sc->val, H224, sizeof H224);
#if SPH_64
	sc->count = 0;
#else
	sc->count_high = sc->count_low = 0;
#endif
}

/* see sph_sha2.h */
void
sph_sha256_init(void *cc)
{
	sph_sha256_context *sc;

	sc = cc;
	memcpy(sc->val, H256, sizeof H256);
#if SPH_64
	sc->count = 0;
#else
	sc->count_high = sc->count_low = 0;
#endif
}

#define RFUN   sha2_round
#define HASH   sha224
#define BE32   1
#include "md_helper.c"

/* see sph_sha2.h */
void
sph_sha224_close(void *cc, void *dst)
{
	sha224_close(cc, dst, 7);
//	sph_sha224_init(cc);
}

/* see sph_sha2.h */
void
sph_sha224_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	sha224_addbits_and_close(cc, ub, n, dst, 7);
//	sph_sha224_init(cc);
}

/* see sph_sha2.h */
void
sph_sha256_close(void *cc, void *dst)
{
	sha224_close(cc, dst, 8);
//	sph_sha256_init(cc);
}

/* see sph_sha2.h */
void
sph_sha256_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	sha224_addbits_and_close(cc, ub, n, dst, 8);
//	sph_sha256_init(cc);
}

/* see sph_sha2.h */
void
sph_sha224_comp(const sph_u32 msg[16], sph_u32 val[8])
{
#define SHA2_IN(x)   msg[x]
	SHA2_ROUND_BODY(SHA2_IN, val);
#undef SHA2_IN
}

#ifdef __cplusplus
}
#endif
