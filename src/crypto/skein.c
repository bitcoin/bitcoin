/* $Id: skein.c 254 2011-06-07 19:38:58Z tp $ */
/*
 * Skein implementation.
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

#include "sph_skein.h"

#ifdef __cplusplus
extern "C"{
#endif


#if SPH_SMALL_FOOTPRINT && !defined SPH_SMALL_FOOTPRINT_SKEIN
#define SPH_SMALL_FOOTPRINT_SKEIN   1
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4146)
#endif

#if SPH_64

#if 0
/* obsolete */
/*
 * M5_ ## s ## _ ## i  evaluates to s+i mod 5 (0 <= s <= 18, 0 <= i <= 3).
 */

#define M5_0_0    0
#define M5_0_1    1
#define M5_0_2    2
#define M5_0_3    3

#define M5_1_0    1
#define M5_1_1    2
#define M5_1_2    3
#define M5_1_3    4

#define M5_2_0    2
#define M5_2_1    3
#define M5_2_2    4
#define M5_2_3    0

#define M5_3_0    3
#define M5_3_1    4
#define M5_3_2    0
#define M5_3_3    1

#define M5_4_0    4
#define M5_4_1    0
#define M5_4_2    1
#define M5_4_3    2

#define M5_5_0    0
#define M5_5_1    1
#define M5_5_2    2
#define M5_5_3    3

#define M5_6_0    1
#define M5_6_1    2
#define M5_6_2    3
#define M5_6_3    4

#define M5_7_0    2
#define M5_7_1    3
#define M5_7_2    4
#define M5_7_3    0

#define M5_8_0    3
#define M5_8_1    4
#define M5_8_2    0
#define M5_8_3    1

#define M5_9_0    4
#define M5_9_1    0
#define M5_9_2    1
#define M5_9_3    2

#define M5_10_0   0
#define M5_10_1   1
#define M5_10_2   2
#define M5_10_3   3

#define M5_11_0   1
#define M5_11_1   2
#define M5_11_2   3
#define M5_11_3   4

#define M5_12_0   2
#define M5_12_1   3
#define M5_12_2   4
#define M5_12_3   0

#define M5_13_0   3
#define M5_13_1   4
#define M5_13_2   0
#define M5_13_3   1

#define M5_14_0   4
#define M5_14_1   0
#define M5_14_2   1
#define M5_14_3   2

#define M5_15_0   0
#define M5_15_1   1
#define M5_15_2   2
#define M5_15_3   3

#define M5_16_0   1
#define M5_16_1   2
#define M5_16_2   3
#define M5_16_3   4

#define M5_17_0   2
#define M5_17_1   3
#define M5_17_2   4
#define M5_17_3   0

#define M5_18_0   3
#define M5_18_1   4
#define M5_18_2   0
#define M5_18_3   1
#endif

/*
 * M9_ ## s ## _ ## i  evaluates to s+i mod 9 (0 <= s <= 18, 0 <= i <= 7).
 */

#define M9_0_0    0
#define M9_0_1    1
#define M9_0_2    2
#define M9_0_3    3
#define M9_0_4    4
#define M9_0_5    5
#define M9_0_6    6
#define M9_0_7    7

#define M9_1_0    1
#define M9_1_1    2
#define M9_1_2    3
#define M9_1_3    4
#define M9_1_4    5
#define M9_1_5    6
#define M9_1_6    7
#define M9_1_7    8

#define M9_2_0    2
#define M9_2_1    3
#define M9_2_2    4
#define M9_2_3    5
#define M9_2_4    6
#define M9_2_5    7
#define M9_2_6    8
#define M9_2_7    0

#define M9_3_0    3
#define M9_3_1    4
#define M9_3_2    5
#define M9_3_3    6
#define M9_3_4    7
#define M9_3_5    8
#define M9_3_6    0
#define M9_3_7    1

#define M9_4_0    4
#define M9_4_1    5
#define M9_4_2    6
#define M9_4_3    7
#define M9_4_4    8
#define M9_4_5    0
#define M9_4_6    1
#define M9_4_7    2

#define M9_5_0    5
#define M9_5_1    6
#define M9_5_2    7
#define M9_5_3    8
#define M9_5_4    0
#define M9_5_5    1
#define M9_5_6    2
#define M9_5_7    3

#define M9_6_0    6
#define M9_6_1    7
#define M9_6_2    8
#define M9_6_3    0
#define M9_6_4    1
#define M9_6_5    2
#define M9_6_6    3
#define M9_6_7    4

#define M9_7_0    7
#define M9_7_1    8
#define M9_7_2    0
#define M9_7_3    1
#define M9_7_4    2
#define M9_7_5    3
#define M9_7_6    4
#define M9_7_7    5

#define M9_8_0    8
#define M9_8_1    0
#define M9_8_2    1
#define M9_8_3    2
#define M9_8_4    3
#define M9_8_5    4
#define M9_8_6    5
#define M9_8_7    6

#define M9_9_0    0
#define M9_9_1    1
#define M9_9_2    2
#define M9_9_3    3
#define M9_9_4    4
#define M9_9_5    5
#define M9_9_6    6
#define M9_9_7    7

#define M9_10_0   1
#define M9_10_1   2
#define M9_10_2   3
#define M9_10_3   4
#define M9_10_4   5
#define M9_10_5   6
#define M9_10_6   7
#define M9_10_7   8

#define M9_11_0   2
#define M9_11_1   3
#define M9_11_2   4
#define M9_11_3   5
#define M9_11_4   6
#define M9_11_5   7
#define M9_11_6   8
#define M9_11_7   0

#define M9_12_0   3
#define M9_12_1   4
#define M9_12_2   5
#define M9_12_3   6
#define M9_12_4   7
#define M9_12_5   8
#define M9_12_6   0
#define M9_12_7   1

#define M9_13_0   4
#define M9_13_1   5
#define M9_13_2   6
#define M9_13_3   7
#define M9_13_4   8
#define M9_13_5   0
#define M9_13_6   1
#define M9_13_7   2

#define M9_14_0   5
#define M9_14_1   6
#define M9_14_2   7
#define M9_14_3   8
#define M9_14_4   0
#define M9_14_5   1
#define M9_14_6   2
#define M9_14_7   3

#define M9_15_0   6
#define M9_15_1   7
#define M9_15_2   8
#define M9_15_3   0
#define M9_15_4   1
#define M9_15_5   2
#define M9_15_6   3
#define M9_15_7   4

#define M9_16_0   7
#define M9_16_1   8
#define M9_16_2   0
#define M9_16_3   1
#define M9_16_4   2
#define M9_16_5   3
#define M9_16_6   4
#define M9_16_7   5

#define M9_17_0   8
#define M9_17_1   0
#define M9_17_2   1
#define M9_17_3   2
#define M9_17_4   3
#define M9_17_5   4
#define M9_17_6   5
#define M9_17_7   6

#define M9_18_0   0
#define M9_18_1   1
#define M9_18_2   2
#define M9_18_3   3
#define M9_18_4   4
#define M9_18_5   5
#define M9_18_6   6
#define M9_18_7   7

/*
 * M3_ ## s ## _ ## i  evaluates to s+i mod 3 (0 <= s <= 18, 0 <= i <= 1).
 */

#define M3_0_0    0
#define M3_0_1    1
#define M3_1_0    1
#define M3_1_1    2
#define M3_2_0    2
#define M3_2_1    0
#define M3_3_0    0
#define M3_3_1    1
#define M3_4_0    1
#define M3_4_1    2
#define M3_5_0    2
#define M3_5_1    0
#define M3_6_0    0
#define M3_6_1    1
#define M3_7_0    1
#define M3_7_1    2
#define M3_8_0    2
#define M3_8_1    0
#define M3_9_0    0
#define M3_9_1    1
#define M3_10_0   1
#define M3_10_1   2
#define M3_11_0   2
#define M3_11_1   0
#define M3_12_0   0
#define M3_12_1   1
#define M3_13_0   1
#define M3_13_1   2
#define M3_14_0   2
#define M3_14_1   0
#define M3_15_0   0
#define M3_15_1   1
#define M3_16_0   1
#define M3_16_1   2
#define M3_17_0   2
#define M3_17_1   0
#define M3_18_0   0
#define M3_18_1   1

#define XCAT(x, y)     XCAT_(x, y)
#define XCAT_(x, y)    x ## y

#if 0
/* obsolete */
#define SKSI(k, s, i)   XCAT(k, XCAT(XCAT(XCAT(M5_, s), _), i))
#define SKST(t, s, v)   XCAT(t, XCAT(XCAT(XCAT(M3_, s), _), v))
#endif

#define SKBI(k, s, i)   XCAT(k, XCAT(XCAT(XCAT(M9_, s), _), i))
#define SKBT(t, s, v)   XCAT(t, XCAT(XCAT(XCAT(M3_, s), _), v))

#if 0
/* obsolete */
#define TFSMALL_KINIT(k0, k1, k2, k3, k4, t0, t1, t2)   do { \
		k4 = (k0 ^ k1) ^ (k2 ^ k3) ^ SPH_C64(0x1BD11BDAA9FC1A22); \
		t2 = t0 ^ t1; \
	} while (0)
#endif

#define TFBIG_KINIT(k0, k1, k2, k3, k4, k5, k6, k7, k8, t0, t1, t2)   do { \
		k8 = ((k0 ^ k1) ^ (k2 ^ k3)) ^ ((k4 ^ k5) ^ (k6 ^ k7)) \
			^ SPH_C64(0x1BD11BDAA9FC1A22); \
		t2 = t0 ^ t1; \
	} while (0)

#if 0
/* obsolete */
#define TFSMALL_ADDKEY(w0, w1, w2, w3, k, t, s)   do { \
		w0 = SPH_T64(w0 + SKSI(k, s, 0)); \
		w1 = SPH_T64(w1 + SKSI(k, s, 1) + SKST(t, s, 0)); \
		w2 = SPH_T64(w2 + SKSI(k, s, 2) + SKST(t, s, 1)); \
		w3 = SPH_T64(w3 + SKSI(k, s, 3) + (sph_u64)s); \
	} while (0)
#endif

#if SPH_SMALL_FOOTPRINT_SKEIN

#define TFBIG_ADDKEY(s, tt0, tt1)   do { \
		p0 = SPH_T64(p0 + h[s + 0]); \
		p1 = SPH_T64(p1 + h[s + 1]); \
		p2 = SPH_T64(p2 + h[s + 2]); \
		p3 = SPH_T64(p3 + h[s + 3]); \
		p4 = SPH_T64(p4 + h[s + 4]); \
		p5 = SPH_T64(p5 + h[s + 5] + tt0); \
		p6 = SPH_T64(p6 + h[s + 6] + tt1); \
		p7 = SPH_T64(p7 + h[s + 7] + (sph_u64)s); \
	} while (0)

#else

#define TFBIG_ADDKEY(w0, w1, w2, w3, w4, w5, w6, w7, k, t, s)   do { \
		w0 = SPH_T64(w0 + SKBI(k, s, 0)); \
		w1 = SPH_T64(w1 + SKBI(k, s, 1)); \
		w2 = SPH_T64(w2 + SKBI(k, s, 2)); \
		w3 = SPH_T64(w3 + SKBI(k, s, 3)); \
		w4 = SPH_T64(w4 + SKBI(k, s, 4)); \
		w5 = SPH_T64(w5 + SKBI(k, s, 5) + SKBT(t, s, 0)); \
		w6 = SPH_T64(w6 + SKBI(k, s, 6) + SKBT(t, s, 1)); \
		w7 = SPH_T64(w7 + SKBI(k, s, 7) + (sph_u64)s); \
	} while (0)

#endif

#if 0
/* obsolete */
#define TFSMALL_MIX(x0, x1, rc)   do { \
		x0 = SPH_T64(x0 + x1); \
		x1 = SPH_ROTL64(x1, rc) ^ x0; \
	} while (0)
#endif

#define TFBIG_MIX(x0, x1, rc)   do { \
		x0 = SPH_T64(x0 + x1); \
		x1 = SPH_ROTL64(x1, rc) ^ x0; \
	} while (0)

#if 0
/* obsolete */
#define TFSMALL_MIX4(w0, w1, w2, w3, rc0, rc1)  do { \
		TFSMALL_MIX(w0, w1, rc0); \
		TFSMALL_MIX(w2, w3, rc1); \
	} while (0)
#endif

#define TFBIG_MIX8(w0, w1, w2, w3, w4, w5, w6, w7, rc0, rc1, rc2, rc3)  do { \
		TFBIG_MIX(w0, w1, rc0); \
		TFBIG_MIX(w2, w3, rc1); \
		TFBIG_MIX(w4, w5, rc2); \
		TFBIG_MIX(w6, w7, rc3); \
	} while (0)

#if 0
/* obsolete */
#define TFSMALL_4e(s)   do { \
		TFSMALL_ADDKEY(p0, p1, p2, p3, h, t, s); \
		TFSMALL_MIX4(p0, p1, p2, p3, 14, 16); \
		TFSMALL_MIX4(p0, p3, p2, p1, 52, 57); \
		TFSMALL_MIX4(p0, p1, p2, p3, 23, 40); \
		TFSMALL_MIX4(p0, p3, p2, p1,  5, 37); \
	} while (0)

#define TFSMALL_4o(s)   do { \
		TFSMALL_ADDKEY(p0, p1, p2, p3, h, t, s); \
		TFSMALL_MIX4(p0, p1, p2, p3, 25, 33); \
		TFSMALL_MIX4(p0, p3, p2, p1, 46, 12); \
		TFSMALL_MIX4(p0, p1, p2, p3, 58, 22); \
		TFSMALL_MIX4(p0, p3, p2, p1, 32, 32); \
	} while (0)
#endif

#if SPH_SMALL_FOOTPRINT_SKEIN

#define TFBIG_4e(s)   do { \
		TFBIG_ADDKEY(s, t0, t1); \
		TFBIG_MIX8(p0, p1, p2, p3, p4, p5, p6, p7, 46, 36, 19, 37); \
		TFBIG_MIX8(p2, p1, p4, p7, p6, p5, p0, p3, 33, 27, 14, 42); \
		TFBIG_MIX8(p4, p1, p6, p3, p0, p5, p2, p7, 17, 49, 36, 39); \
		TFBIG_MIX8(p6, p1, p0, p7, p2, p5, p4, p3, 44,  9, 54, 56); \
	} while (0)

#define TFBIG_4o(s)   do { \
		TFBIG_ADDKEY(s, t1, t2); \
		TFBIG_MIX8(p0, p1, p2, p3, p4, p5, p6, p7, 39, 30, 34, 24); \
		TFBIG_MIX8(p2, p1, p4, p7, p6, p5, p0, p3, 13, 50, 10, 17); \
		TFBIG_MIX8(p4, p1, p6, p3, p0, p5, p2, p7, 25, 29, 39, 43); \
		TFBIG_MIX8(p6, p1, p0, p7, p2, p5, p4, p3,  8, 35, 56, 22); \
	} while (0)

#else

#define TFBIG_4e(s)   do { \
		TFBIG_ADDKEY(p0, p1, p2, p3, p4, p5, p6, p7, h, t, s); \
		TFBIG_MIX8(p0, p1, p2, p3, p4, p5, p6, p7, 46, 36, 19, 37); \
		TFBIG_MIX8(p2, p1, p4, p7, p6, p5, p0, p3, 33, 27, 14, 42); \
		TFBIG_MIX8(p4, p1, p6, p3, p0, p5, p2, p7, 17, 49, 36, 39); \
		TFBIG_MIX8(p6, p1, p0, p7, p2, p5, p4, p3, 44,  9, 54, 56); \
	} while (0)

#define TFBIG_4o(s)   do { \
		TFBIG_ADDKEY(p0, p1, p2, p3, p4, p5, p6, p7, h, t, s); \
		TFBIG_MIX8(p0, p1, p2, p3, p4, p5, p6, p7, 39, 30, 34, 24); \
		TFBIG_MIX8(p2, p1, p4, p7, p6, p5, p0, p3, 13, 50, 10, 17); \
		TFBIG_MIX8(p4, p1, p6, p3, p0, p5, p2, p7, 25, 29, 39, 43); \
		TFBIG_MIX8(p6, p1, p0, p7, p2, p5, p4, p3,  8, 35, 56, 22); \
	} while (0)

#endif

#if 0
/* obsolete */
#define UBI_SMALL(etype, extra)  do { \
		sph_u64 h4, t0, t1, t2; \
		sph_u64 m0 = sph_dec64le(buf +  0); \
		sph_u64 m1 = sph_dec64le(buf +  8); \
		sph_u64 m2 = sph_dec64le(buf + 16); \
		sph_u64 m3 = sph_dec64le(buf + 24); \
		sph_u64 p0 = m0; \
		sph_u64 p1 = m1; \
		sph_u64 p2 = m2; \
		sph_u64 p3 = m3; \
		t0 = SPH_T64(bcount << 5) + (sph_u64)(extra); \
		t1 = (bcount >> 59) + ((sph_u64)(etype) << 55); \
		TFSMALL_KINIT(h0, h1, h2, h3, h4, t0, t1, t2); \
		TFSMALL_4e(0); \
		TFSMALL_4o(1); \
		TFSMALL_4e(2); \
		TFSMALL_4o(3); \
		TFSMALL_4e(4); \
		TFSMALL_4o(5); \
		TFSMALL_4e(6); \
		TFSMALL_4o(7); \
		TFSMALL_4e(8); \
		TFSMALL_4o(9); \
		TFSMALL_4e(10); \
		TFSMALL_4o(11); \
		TFSMALL_4e(12); \
		TFSMALL_4o(13); \
		TFSMALL_4e(14); \
		TFSMALL_4o(15); \
		TFSMALL_4e(16); \
		TFSMALL_4o(17); \
		TFSMALL_ADDKEY(p0, p1, p2, p3, h, t, 18); \
		h0 = m0 ^ p0; \
		h1 = m1 ^ p1; \
		h2 = m2 ^ p2; \
		h3 = m3 ^ p3; \
	} while (0)
#endif

#if SPH_SMALL_FOOTPRINT_SKEIN

#define UBI_BIG(etype, extra)  do { \
		sph_u64 t0, t1, t2; \
		unsigned u; \
		sph_u64 m0 = sph_dec64le_aligned(buf +  0); \
		sph_u64 m1 = sph_dec64le_aligned(buf +  8); \
		sph_u64 m2 = sph_dec64le_aligned(buf + 16); \
		sph_u64 m3 = sph_dec64le_aligned(buf + 24); \
		sph_u64 m4 = sph_dec64le_aligned(buf + 32); \
		sph_u64 m5 = sph_dec64le_aligned(buf + 40); \
		sph_u64 m6 = sph_dec64le_aligned(buf + 48); \
		sph_u64 m7 = sph_dec64le_aligned(buf + 56); \
		sph_u64 p0 = m0; \
		sph_u64 p1 = m1; \
		sph_u64 p2 = m2; \
		sph_u64 p3 = m3; \
		sph_u64 p4 = m4; \
		sph_u64 p5 = m5; \
		sph_u64 p6 = m6; \
		sph_u64 p7 = m7; \
		t0 = SPH_T64(bcount << 6) + (sph_u64)(extra); \
		t1 = (bcount >> 58) + ((sph_u64)(etype) << 55); \
		TFBIG_KINIT(h[0], h[1], h[2], h[3], h[4], h[5], \
			h[6], h[7], h[8], t0, t1, t2); \
		for (u = 0; u <= 15; u += 3) { \
			h[u +  9] = h[u + 0]; \
			h[u + 10] = h[u + 1]; \
			h[u + 11] = h[u + 2]; \
		} \
		for (u = 0; u < 9; u ++) { \
			sph_u64 s = u << 1; \
			sph_u64 tmp; \
			TFBIG_4e(s); \
			TFBIG_4o(s + 1); \
			tmp = t2; \
			t2 = t1; \
			t1 = t0; \
			t0 = tmp; \
		} \
		TFBIG_ADDKEY(18, t0, t1); \
		h[0] = m0 ^ p0; \
		h[1] = m1 ^ p1; \
		h[2] = m2 ^ p2; \
		h[3] = m3 ^ p3; \
		h[4] = m4 ^ p4; \
		h[5] = m5 ^ p5; \
		h[6] = m6 ^ p6; \
		h[7] = m7 ^ p7; \
	} while (0)

#else

#define UBI_BIG(etype, extra)  do { \
		sph_u64 h8, t0, t1, t2; \
		sph_u64 m0 = sph_dec64le_aligned(buf +  0); \
		sph_u64 m1 = sph_dec64le_aligned(buf +  8); \
		sph_u64 m2 = sph_dec64le_aligned(buf + 16); \
		sph_u64 m3 = sph_dec64le_aligned(buf + 24); \
		sph_u64 m4 = sph_dec64le_aligned(buf + 32); \
		sph_u64 m5 = sph_dec64le_aligned(buf + 40); \
		sph_u64 m6 = sph_dec64le_aligned(buf + 48); \
		sph_u64 m7 = sph_dec64le_aligned(buf + 56); \
		sph_u64 p0 = m0; \
		sph_u64 p1 = m1; \
		sph_u64 p2 = m2; \
		sph_u64 p3 = m3; \
		sph_u64 p4 = m4; \
		sph_u64 p5 = m5; \
		sph_u64 p6 = m6; \
		sph_u64 p7 = m7; \
		t0 = SPH_T64(bcount << 6) + (sph_u64)(extra); \
		t1 = (bcount >> 58) + ((sph_u64)(etype) << 55); \
		TFBIG_KINIT(h0, h1, h2, h3, h4, h5, h6, h7, h8, t0, t1, t2); \
		TFBIG_4e(0); \
		TFBIG_4o(1); \
		TFBIG_4e(2); \
		TFBIG_4o(3); \
		TFBIG_4e(4); \
		TFBIG_4o(5); \
		TFBIG_4e(6); \
		TFBIG_4o(7); \
		TFBIG_4e(8); \
		TFBIG_4o(9); \
		TFBIG_4e(10); \
		TFBIG_4o(11); \
		TFBIG_4e(12); \
		TFBIG_4o(13); \
		TFBIG_4e(14); \
		TFBIG_4o(15); \
		TFBIG_4e(16); \
		TFBIG_4o(17); \
		TFBIG_ADDKEY(p0, p1, p2, p3, p4, p5, p6, p7, h, t, 18); \
		h0 = m0 ^ p0; \
		h1 = m1 ^ p1; \
		h2 = m2 ^ p2; \
		h3 = m3 ^ p3; \
		h4 = m4 ^ p4; \
		h5 = m5 ^ p5; \
		h6 = m6 ^ p6; \
		h7 = m7 ^ p7; \
	} while (0)

#endif

#if 0
/* obsolete */
#define DECL_STATE_SMALL \
	sph_u64 h0, h1, h2, h3; \
	sph_u64 bcount;

#define READ_STATE_SMALL(sc)   do { \
		h0 = (sc)->h0; \
		h1 = (sc)->h1; \
		h2 = (sc)->h2; \
		h3 = (sc)->h3; \
		bcount = sc->bcount; \
	} while (0)

#define WRITE_STATE_SMALL(sc)   do { \
		(sc)->h0 = h0; \
		(sc)->h1 = h1; \
		(sc)->h2 = h2; \
		(sc)->h3 = h3; \
		sc->bcount = bcount; \
	} while (0)
#endif

#if SPH_SMALL_FOOTPRINT_SKEIN

#define DECL_STATE_BIG \
	sph_u64 h[27]; \
	sph_u64 bcount;

#define READ_STATE_BIG(sc)   do { \
		h[0] = (sc)->h0; \
		h[1] = (sc)->h1; \
		h[2] = (sc)->h2; \
		h[3] = (sc)->h3; \
		h[4] = (sc)->h4; \
		h[5] = (sc)->h5; \
		h[6] = (sc)->h6; \
		h[7] = (sc)->h7; \
		bcount = sc->bcount; \
	} while (0)

#define WRITE_STATE_BIG(sc)   do { \
		(sc)->h0 = h[0]; \
		(sc)->h1 = h[1]; \
		(sc)->h2 = h[2]; \
		(sc)->h3 = h[3]; \
		(sc)->h4 = h[4]; \
		(sc)->h5 = h[5]; \
		(sc)->h6 = h[6]; \
		(sc)->h7 = h[7]; \
		sc->bcount = bcount; \
	} while (0)

#else

#define DECL_STATE_BIG \
	sph_u64 h0, h1, h2, h3, h4, h5, h6, h7; \
	sph_u64 bcount;

#define READ_STATE_BIG(sc)   do { \
		h0 = (sc)->h0; \
		h1 = (sc)->h1; \
		h2 = (sc)->h2; \
		h3 = (sc)->h3; \
		h4 = (sc)->h4; \
		h5 = (sc)->h5; \
		h6 = (sc)->h6; \
		h7 = (sc)->h7; \
		bcount = sc->bcount; \
	} while (0)

#define WRITE_STATE_BIG(sc)   do { \
		(sc)->h0 = h0; \
		(sc)->h1 = h1; \
		(sc)->h2 = h2; \
		(sc)->h3 = h3; \
		(sc)->h4 = h4; \
		(sc)->h5 = h5; \
		(sc)->h6 = h6; \
		(sc)->h7 = h7; \
		sc->bcount = bcount; \
	} while (0)

#endif

#if 0
/* obsolete */
static void
skein_small_init(sph_skein_small_context *sc, const sph_u64 *iv)
{
	sc->h0 = iv[0];
	sc->h1 = iv[1];
	sc->h2 = iv[2];
	sc->h3 = iv[3];
	sc->bcount = 0;
	sc->ptr = 0;
}
#endif

static void
skein_big_init(sph_skein_big_context *sc, const sph_u64 *iv)
{
	sc->h0 = iv[0];
	sc->h1 = iv[1];
	sc->h2 = iv[2];
	sc->h3 = iv[3];
	sc->h4 = iv[4];
	sc->h5 = iv[5];
	sc->h6 = iv[6];
	sc->h7 = iv[7];
	sc->bcount = 0;
	sc->ptr = 0;
}

#if 0
/* obsolete */
static void
skein_small_core(sph_skein_small_context *sc, const void *data, size_t len)
{
	unsigned char *buf;
	size_t ptr, clen;
	unsigned first;
	DECL_STATE_SMALL

	buf = sc->buf;
	ptr = sc->ptr;
	clen = (sizeof sc->buf) - ptr;
	if (len <= clen) {
		memcpy(buf + ptr, data, len);
		sc->ptr = ptr + len;
		return;
	}
	if (clen != 0) {
		memcpy(buf + ptr, data, clen);
		data = (const unsigned char *)data + clen;
		len -= clen;
	}

#if SPH_SMALL_FOOTPRINT_SKEIN

	READ_STATE_SMALL(sc);
	first = (bcount == 0) << 7;
	for (;;) {
		bcount ++;
		UBI_SMALL(96 + first, 0);
		if (len <= sizeof sc->buf)
			break;
		first = 0;
		memcpy(buf, data, sizeof sc->buf);
		data = (const unsigned char *)data + sizeof sc->buf;
		len -= sizeof sc->buf;
	}
	WRITE_STATE_SMALL(sc);
	sc->ptr = len;
	memcpy(buf, data, len);

#else

	/*
	 * Unrolling the loop yields a slight performance boost, while
	 * keeping the code size aorund 24 kB on 32-bit x86.
	 */
	READ_STATE_SMALL(sc);
	first = (bcount == 0) << 7;
	for (;;) {
		bcount ++;
		UBI_SMALL(96 + first, 0);
		if (len <= sizeof sc->buf)
			break;
		buf = (unsigned char *)data;
		bcount ++;
		UBI_SMALL(96, 0);
		if (len <= 2 * sizeof sc->buf) {
			data = buf + sizeof sc->buf;
			len -= sizeof sc->buf;
			break;
		}
		buf += sizeof sc->buf;
		data = buf + sizeof sc->buf;
		first = 0;
		len -= 2 * sizeof sc->buf;
	}
	WRITE_STATE_SMALL(sc);
	sc->ptr = len;
	memcpy(sc->buf, data, len);

#endif
}
#endif

static void
skein_big_core(sph_skein_big_context *sc, const void *data, size_t len)
{
	/*
	 * The Skein "final bit" in the tweak is troublesome here,
	 * because if the input has a length which is a multiple of the
	 * block size (512 bits) then that bit must be set for the
	 * final block, which is full of message bits (padding in
	 * Skein can be reduced to no extra bit at all). However, this
	 * function cannot know whether it processes the last chunks of
	 * the message or not. Hence we may keep a full block of buffered
	 * data (64 bytes).
	 */
	unsigned char *buf;
	size_t ptr;
	unsigned first;
	DECL_STATE_BIG

	buf = sc->buf;
	ptr = sc->ptr;
	if (len <= (sizeof sc->buf) - ptr) {
		memcpy(buf + ptr, data, len);
		ptr += len;
		sc->ptr = ptr;
		return;
	}

	READ_STATE_BIG(sc);
	first = (bcount == 0) << 7;
	do {
		size_t clen;

		if (ptr == sizeof sc->buf) {
			bcount ++;
			UBI_BIG(96 + first, 0);
			first = 0;
			ptr = 0;
		}
		clen = (sizeof sc->buf) - ptr;
		if (clen > len)
			clen = len;
		memcpy(buf + ptr, data, clen);
		ptr += clen;
		data = (const unsigned char *)data + clen;
		len -= clen;
	} while (len > 0);
	WRITE_STATE_BIG(sc);
	sc->ptr = ptr;
}

#if 0
/* obsolete */
static void
skein_small_close(sph_skein_small_context *sc, unsigned ub, unsigned n,
	void *dst, size_t out_len)
{
	unsigned char *buf;
	size_t ptr;
	unsigned et;
	int i;
	DECL_STATE_SMALL

	if (n != 0) {
		unsigned z;
		unsigned char x;

		z = 0x80 >> n;
		x = ((ub & -z) | z) & 0xFF;
		skein_small_core(sc, &x, 1);
	}

	buf = sc->buf;
	ptr = sc->ptr;
	READ_STATE_SMALL(sc);
	memset(buf + ptr, 0, (sizeof sc->buf) - ptr);
	et = 352 + ((bcount == 0) << 7) + (n != 0);
	for (i = 0; i < 2; i ++) {
		UBI_SMALL(et, ptr);
		if (i == 0) {
			memset(buf, 0, sizeof sc->buf);
			bcount = 0;
			et = 510;
			ptr = 8;
		}
	}

	sph_enc64le_aligned(buf +  0, h0);
	sph_enc64le_aligned(buf +  8, h1);
	sph_enc64le_aligned(buf + 16, h2);
	sph_enc64le_aligned(buf + 24, h3);
	memcpy(dst, buf, out_len);
}
#endif

static void
skein_big_close(sph_skein_big_context *sc, unsigned ub, unsigned n,
	void *dst, size_t out_len)
{
	unsigned char *buf;
	size_t ptr;
	unsigned et;
	int i;
#if SPH_SMALL_FOOTPRINT_SKEIN
	size_t u;
#endif
	DECL_STATE_BIG

	/*
	 * Add bit padding if necessary.
	 */
	if (n != 0) {
		unsigned z;
		unsigned char x;

		z = 0x80 >> n;
		x = ((ub & -z) | z) & 0xFF;
		skein_big_core(sc, &x, 1);
	}

	buf = sc->buf;
	ptr = sc->ptr;

	/*
	 * At that point, if ptr == 0, then the message was empty;
	 * otherwise, there is between 1 and 64 bytes (inclusive) which
	 * are yet to be processed. Either way, we complete the buffer
	 * to a full block with zeros (the Skein specification mandates
	 * that an empty message is padded so that there is at least
	 * one block to process).
	 *
	 * Once this block has been processed, we do it again, with
	 * a block full of zeros, for the output (that block contains
	 * the encoding of "0", over 8 bytes, then padded with zeros).
	 */
	READ_STATE_BIG(sc);
	memset(buf + ptr, 0, (sizeof sc->buf) - ptr);
	et = 352 + ((bcount == 0) << 7) + (n != 0);
	for (i = 0; i < 2; i ++) {
		UBI_BIG(et, ptr);
		if (i == 0) {
			memset(buf, 0, sizeof sc->buf);
			bcount = 0;
			et = 510;
			ptr = 8;
		}
	}

#if SPH_SMALL_FOOTPRINT_SKEIN

	/*
	 * We use a temporary buffer because we must support the case
	 * where output size is not a multiple of 64 (namely, a 224-bit
	 * output).
	 */
	for (u = 0; u < out_len; u += 8)
		sph_enc64le_aligned(buf + u, h[u >> 3]);
	memcpy(dst, buf, out_len);

#else

	sph_enc64le_aligned(buf +  0, h0);
	sph_enc64le_aligned(buf +  8, h1);
	sph_enc64le_aligned(buf + 16, h2);
	sph_enc64le_aligned(buf + 24, h3);
	sph_enc64le_aligned(buf + 32, h4);
	sph_enc64le_aligned(buf + 40, h5);
	sph_enc64le_aligned(buf + 48, h6);
	sph_enc64le_aligned(buf + 56, h7);
	memcpy(dst, buf, out_len);

#endif
}

#if 0
/* obsolete */
static const sph_u64 IV224[] = {
	SPH_C64(0xC6098A8C9AE5EA0B), SPH_C64(0x876D568608C5191C),
	SPH_C64(0x99CB88D7D7F53884), SPH_C64(0x384BDDB1AEDDB5DE)
};

static const sph_u64 IV256[] = {
	SPH_C64(0xFC9DA860D048B449), SPH_C64(0x2FCA66479FA7D833),
	SPH_C64(0xB33BC3896656840F), SPH_C64(0x6A54E920FDE8DA69)
};
#endif

static const sph_u64 IV224[] = {
	SPH_C64(0xCCD0616248677224), SPH_C64(0xCBA65CF3A92339EF),
	SPH_C64(0x8CCD69D652FF4B64), SPH_C64(0x398AED7B3AB890B4),
	SPH_C64(0x0F59D1B1457D2BD0), SPH_C64(0x6776FE6575D4EB3D),
	SPH_C64(0x99FBC70E997413E9), SPH_C64(0x9E2CFCCFE1C41EF7)
};

static const sph_u64 IV256[] = {
	SPH_C64(0xCCD044A12FDB3E13), SPH_C64(0xE83590301A79A9EB),
	SPH_C64(0x55AEA0614F816E6F), SPH_C64(0x2A2767A4AE9B94DB),
	SPH_C64(0xEC06025E74DD7683), SPH_C64(0xE7A436CDC4746251),
	SPH_C64(0xC36FBAF9393AD185), SPH_C64(0x3EEDBA1833EDFC13)
};

static const sph_u64 IV384[] = {
	SPH_C64(0xA3F6C6BF3A75EF5F), SPH_C64(0xB0FEF9CCFD84FAA4),
	SPH_C64(0x9D77DD663D770CFE), SPH_C64(0xD798CBF3B468FDDA),
	SPH_C64(0x1BC4A6668A0E4465), SPH_C64(0x7ED7D434E5807407),
	SPH_C64(0x548FC1ACD4EC44D6), SPH_C64(0x266E17546AA18FF8)
};

static const sph_u64 IV512[] = {
	SPH_C64(0x4903ADFF749C51CE), SPH_C64(0x0D95DE399746DF03),
	SPH_C64(0x8FD1934127C79BCE), SPH_C64(0x9A255629FF352CB1),
	SPH_C64(0x5DB62599DF6CA7B0), SPH_C64(0xEABE394CA9D5C3F4),
	SPH_C64(0x991112C71A75B523), SPH_C64(0xAE18A40B660FCC33)
};

#if 0
/* obsolete */
/* see sph_skein.h */
void
sph_skein224_init(void *cc)
{
	skein_small_init(cc, IV224);
}

/* see sph_skein.h */
void
sph_skein224(void *cc, const void *data, size_t len)
{
	skein_small_core(cc, data, len);
}

/* see sph_skein.h */
void
sph_skein224_close(void *cc, void *dst)
{
	sph_skein224_addbits_and_close(cc, 0, 0, dst);
}

/* see sph_skein.h */
void
sph_skein224_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	skein_small_close(cc, ub, n, dst, 28);
	sph_skein224_init(cc);
}

/* see sph_skein.h */
void
sph_skein256_init(void *cc)
{
	skein_small_init(cc, IV256);
}

/* see sph_skein.h */
void
sph_skein256(void *cc, const void *data, size_t len)
{
	skein_small_core(cc, data, len);
}

/* see sph_skein.h */
void
sph_skein256_close(void *cc, void *dst)
{
	sph_skein256_addbits_and_close(cc, 0, 0, dst);
}

/* see sph_skein.h */
void
sph_skein256_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	skein_small_close(cc, ub, n, dst, 32);
	sph_skein256_init(cc);
}
#endif

/* see sph_skein.h */
void
sph_skein224_init(void *cc)
{
	skein_big_init(cc, IV224);
}

/* see sph_skein.h */
void
sph_skein224(void *cc, const void *data, size_t len)
{
	skein_big_core(cc, data, len);
}

/* see sph_skein.h */
void
sph_skein224_close(void *cc, void *dst)
{
	sph_skein224_addbits_and_close(cc, 0, 0, dst);
}

/* see sph_skein.h */
void
sph_skein224_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	skein_big_close(cc, ub, n, dst, 28);
	sph_skein224_init(cc);
}

/* see sph_skein.h */
void
sph_skein256_init(void *cc)
{
	skein_big_init(cc, IV256);
}

/* see sph_skein.h */
void
sph_skein256(void *cc, const void *data, size_t len)
{
	skein_big_core(cc, data, len);
}

/* see sph_skein.h */
void
sph_skein256_close(void *cc, void *dst)
{
	sph_skein256_addbits_and_close(cc, 0, 0, dst);
}

/* see sph_skein.h */
void
sph_skein256_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	skein_big_close(cc, ub, n, dst, 32);
	sph_skein256_init(cc);
}

/* see sph_skein.h */
void
sph_skein384_init(void *cc)
{
	skein_big_init(cc, IV384);
}

/* see sph_skein.h */
void
sph_skein384(void *cc, const void *data, size_t len)
{
	skein_big_core(cc, data, len);
}

/* see sph_skein.h */
void
sph_skein384_close(void *cc, void *dst)
{
	sph_skein384_addbits_and_close(cc, 0, 0, dst);
}

/* see sph_skein.h */
void
sph_skein384_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	skein_big_close(cc, ub, n, dst, 48);
	sph_skein384_init(cc);
}

/* see sph_skein.h */
void
sph_skein512_init(void *cc)
{
	skein_big_init(cc, IV512);
}

/* see sph_skein.h */
void
sph_skein512(void *cc, const void *data, size_t len)
{
	skein_big_core(cc, data, len);
}

/* see sph_skein.h */
void
sph_skein512_close(void *cc, void *dst)
{
	sph_skein512_addbits_and_close(cc, 0, 0, dst);
}

/* see sph_skein.h */
void
sph_skein512_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	skein_big_close(cc, ub, n, dst, 64);
	sph_skein512_init(cc);
}

#endif


#ifdef __cplusplus
}
#endif
