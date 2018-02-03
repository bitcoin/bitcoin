/* $Id: simd.c 227 2010-06-16 17:28:38Z tp $ */
/*
 * SIMD implementation.
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
#include <limits.h>

#include "sph_simd.h"

#ifdef __cplusplus
extern "C"{
#endif

#if SPH_SMALL_FOOTPRINT && !defined SPH_SMALL_FOOTPRINT_SIMD
#define SPH_SMALL_FOOTPRINT_SIMD   1
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4146)
#endif

typedef sph_u32 u32;
typedef sph_s32 s32;
#define C32     SPH_C32
#define T32     SPH_T32
#define ROL32   SPH_ROTL32

#define XCAT(x, y)    XCAT_(x, y)
#define XCAT_(x, y)   x ## y

/*
 * The powers of 41 modulo 257. We use exponents from 0 to 255, inclusive.
 */
static const s32 alpha_tab[] = {
	  1,  41, 139,  45,  46,  87, 226,  14,  60, 147, 116, 130,
	190,  80, 196,  69,   2,  82,  21,  90,  92, 174, 195,  28,
	120,  37, 232,   3, 123, 160, 135, 138,   4, 164,  42, 180,
	184,  91, 133,  56, 240,  74, 207,   6, 246,  63,  13,  19,
	  8,  71,  84, 103, 111, 182,   9, 112, 223, 148, 157,  12,
	235, 126,  26,  38,  16, 142, 168, 206, 222, 107,  18, 224,
	189,  39,  57,  24, 213, 252,  52,  76,  32,  27,  79, 155,
	187, 214,  36, 191, 121,  78, 114,  48, 169, 247, 104, 152,
	 64,  54, 158,  53, 117, 171,  72, 125, 242, 156, 228,  96,
	 81, 237, 208,  47, 128, 108,  59, 106, 234,  85, 144, 250,
	227,  55, 199, 192, 162, 217, 159,  94, 256, 216, 118, 212,
	211, 170,  31, 243, 197, 110, 141, 127,  67, 177,  61, 188,
	255, 175, 236, 167, 165,  83,  62, 229, 137, 220,  25, 254,
	134,  97, 122, 119, 253,  93, 215,  77,  73, 166, 124, 201,
	 17, 183,  50, 251,  11, 194, 244, 238, 249, 186, 173, 154,
	146,  75, 248, 145,  34, 109, 100, 245,  22, 131, 231, 219,
	241, 115,  89,  51,  35, 150, 239,  33,  68, 218, 200, 233,
	 44,   5, 205, 181, 225, 230, 178, 102,  70,  43, 221,  66,
	136, 179, 143, 209,  88,  10, 153, 105, 193, 203,  99, 204,
	140,  86, 185, 132,  15, 101,  29, 161, 176,  20,  49, 210,
	129, 149, 198, 151,  23, 172, 113,   7,  30, 202,  58,  65,
	 95,  40,  98, 163
};

/*
 * Ranges:
 *   REDS1: from -32768..98302 to -383..383
 *   REDS2: from -2^31..2^31-1 to -32768..98302
 */
#define REDS1(x)    (((x) & 0xFF) - ((x) >> 8))
#define REDS2(x)    (((x) & 0xFFFF) + ((x) >> 16))

/*
 * If, upon entry, the values of q[] are all in the -N..N range (where
 * N >= 98302) then the new values of q[] are in the -2N..2N range.
 *
 * Since alpha_tab[v] <= 256, maximum allowed range is for N = 8388608.
 */
#define FFT_LOOP(rb, hk, as, id)   do { \
		size_t u, v; \
		s32 m = q[(rb)]; \
		s32 n = q[(rb) + (hk)]; \
		q[(rb)] = m + n; \
		q[(rb) + (hk)] = m - n; \
		u = v = 0; \
		goto id; \
		for (; u < (hk); u += 4, v += 4 * (as)) { \
			s32 t; \
			m = q[(rb) + u + 0]; \
			n = q[(rb) + u + 0 + (hk)]; \
			t = REDS2(n * alpha_tab[v + 0 * (as)]); \
			q[(rb) + u + 0] = m + t; \
			q[(rb) + u + 0 + (hk)] = m - t; \
		id: \
			m = q[(rb) + u + 1]; \
			n = q[(rb) + u + 1 + (hk)]; \
			t = REDS2(n * alpha_tab[v + 1 * (as)]); \
			q[(rb) + u + 1] = m + t; \
			q[(rb) + u + 1 + (hk)] = m - t; \
			m = q[(rb) + u + 2]; \
			n = q[(rb) + u + 2 + (hk)]; \
			t = REDS2(n * alpha_tab[v + 2 * (as)]); \
			q[(rb) + u + 2] = m + t; \
			q[(rb) + u + 2 + (hk)] = m - t; \
			m = q[(rb) + u + 3]; \
			n = q[(rb) + u + 3 + (hk)]; \
			t = REDS2(n * alpha_tab[v + 3 * (as)]); \
			q[(rb) + u + 3] = m + t; \
			q[(rb) + u + 3 + (hk)] = m - t; \
		} \
	} while (0)

/*
 * Output ranges:
 *   d0:   min=    0   max= 1020
 *   d1:   min=  -67   max= 4587
 *   d2:   min=-4335   max= 4335
 *   d3:   min=-4147   max=  507
 *   d4:   min= -510   max=  510
 *   d5:   min= -252   max= 4402
 *   d6:   min=-4335   max= 4335
 *   d7:   min=-4332   max=  322
 */
#define FFT8(xb, xs, d)   do { \
		s32 x0 = x[(xb)]; \
		s32 x1 = x[(xb) + (xs)]; \
		s32 x2 = x[(xb) + 2 * (xs)]; \
		s32 x3 = x[(xb) + 3 * (xs)]; \
		s32 a0 = x0 + x2; \
		s32 a1 = x0 + (x2 << 4); \
		s32 a2 = x0 - x2; \
		s32 a3 = x0 - (x2 << 4); \
		s32 b0 = x1 + x3; \
		s32 b1 = REDS1((x1 << 2) + (x3 << 6)); \
		s32 b2 = (x1 << 4) - (x3 << 4); \
		s32 b3 = REDS1((x1 << 6) + (x3 << 2)); \
		d ## 0 = a0 + b0; \
		d ## 1 = a1 + b1; \
		d ## 2 = a2 + b2; \
		d ## 3 = a3 + b3; \
		d ## 4 = a0 - b0; \
		d ## 5 = a1 - b1; \
		d ## 6 = a2 - b2; \
		d ## 7 = a3 - b3; \
	} while (0)

/*
 * When k=16, we have alpha=2. Multiplication by alpha^i is then reduced
 * to some shifting.
 *
 * Output: within -591471..591723
 */
#define FFT16(xb, xs, rb)   do { \
		s32 d1_0, d1_1, d1_2, d1_3, d1_4, d1_5, d1_6, d1_7; \
		s32 d2_0, d2_1, d2_2, d2_3, d2_4, d2_5, d2_6, d2_7; \
		FFT8(xb, (xs) << 1, d1_); \
		FFT8((xb) + (xs), (xs) << 1, d2_); \
		q[(rb) +  0] = d1_0 + d2_0; \
		q[(rb) +  1] = d1_1 + (d2_1 << 1); \
		q[(rb) +  2] = d1_2 + (d2_2 << 2); \
		q[(rb) +  3] = d1_3 + (d2_3 << 3); \
		q[(rb) +  4] = d1_4 + (d2_4 << 4); \
		q[(rb) +  5] = d1_5 + (d2_5 << 5); \
		q[(rb) +  6] = d1_6 + (d2_6 << 6); \
		q[(rb) +  7] = d1_7 + (d2_7 << 7); \
		q[(rb) +  8] = d1_0 - d2_0; \
		q[(rb) +  9] = d1_1 - (d2_1 << 1); \
		q[(rb) + 10] = d1_2 - (d2_2 << 2); \
		q[(rb) + 11] = d1_3 - (d2_3 << 3); \
		q[(rb) + 12] = d1_4 - (d2_4 << 4); \
		q[(rb) + 13] = d1_5 - (d2_5 << 5); \
		q[(rb) + 14] = d1_6 - (d2_6 << 6); \
		q[(rb) + 15] = d1_7 - (d2_7 << 7); \
	} while (0)

/*
 * Output range: |q| <= 1183446
 */
#define FFT32(xb, xs, rb, id)   do { \
		FFT16(xb, (xs) << 1, rb); \
		FFT16((xb) + (xs), (xs) << 1, (rb) + 16); \
		FFT_LOOP(rb, 16, 8, id); \
	} while (0)

/*
 * Output range: |q| <= 2366892
 */
#define FFT64(xb, xs, rb, id)   do { \
		FFT32(xb, (xs) << 1, rb, XCAT(id, a)); \
		FFT32((xb) + (xs), (xs) << 1, (rb) + 32, XCAT(id, b)); \
		FFT_LOOP(rb, 32, 4, id); \
	} while (0)

#if SPH_SMALL_FOOTPRINT_SIMD

static void
fft32(unsigned char *x, size_t xs, s32 *q)
{
	size_t xd;

	xd = xs << 1;
	FFT16(0, xd, 0);
	FFT16(xs, xd, 16);
	FFT_LOOP(0, 16, 8, label_);
}

#define FFT128(xb, xs, rb, id)   do { \
		fft32(x + (xb) + ((xs) * 0), (xs) << 2, &q[(rb) +  0]); \
		fft32(x + (xb) + ((xs) * 2), (xs) << 2, &q[(rb) + 32]); \
		FFT_LOOP(rb, 32, 4, XCAT(id, aa)); \
		fft32(x + (xb) + ((xs) * 1), (xs) << 2, &q[(rb) + 64]); \
		fft32(x + (xb) + ((xs) * 3), (xs) << 2, &q[(rb) + 96]); \
		FFT_LOOP((rb) + 64, 32, 4, XCAT(id, ab)); \
		FFT_LOOP(rb, 64, 2, XCAT(id, a)); \
	} while (0)

#else

/*
 * Output range: |q| <= 4733784
 */
#define FFT128(xb, xs, rb, id)   do { \
		FFT64(xb, (xs) << 1, rb, XCAT(id, a)); \
		FFT64((xb) + (xs), (xs) << 1, (rb) + 64, XCAT(id, b)); \
		FFT_LOOP(rb, 64, 2, id); \
	} while (0)

#endif

/*
 * For SIMD-384 / SIMD-512, the fully unrolled FFT yields a compression
 * function which does not fit in the 32 kB L1 cache of a typical x86
 * Intel. We therefore add a function call layer at the FFT64 level.
 */

static void
fft64(unsigned char *x, size_t xs, s32 *q)
{
	size_t xd;

	xd = xs << 1;
	FFT32(0, xd, 0, label_a);
	FFT32(xs, xd, 32, label_b);
	FFT_LOOP(0, 32, 4, label_);
}

/*
 * Output range: |q| <= 9467568
 */
#define FFT256(xb, xs, rb, id)   do { \
		fft64(x + (xb) + ((xs) * 0), (xs) << 2, &q[(rb) +   0]); \
		fft64(x + (xb) + ((xs) * 2), (xs) << 2, &q[(rb) +  64]); \
		FFT_LOOP(rb, 64, 2, XCAT(id, aa)); \
		fft64(x + (xb) + ((xs) * 1), (xs) << 2, &q[(rb) + 128]); \
		fft64(x + (xb) + ((xs) * 3), (xs) << 2, &q[(rb) + 192]); \
		FFT_LOOP((rb) + 128, 64, 2, XCAT(id, ab)); \
		FFT_LOOP(rb, 128, 1, XCAT(id, a)); \
	} while (0)

/*
 * alpha^(127*i) mod 257
 */
static const unsigned short yoff_s_n[] = {
	  1,  98,  95,  58,  30, 113,  23, 198, 129,  49, 176,  29,
	 15, 185, 140,  99, 193, 153,  88, 143, 136, 221,  70, 178,
	225, 205,  44, 200,  68, 239,  35,  89, 241, 231,  22, 100,
	 34, 248, 146, 173, 249, 244,  11,  50,  17, 124,  73, 215,
	253, 122, 134,  25, 137,  62, 165, 236, 255,  61,  67, 141,
	197,  31, 211, 118, 256, 159, 162, 199, 227, 144, 234,  59,
	128, 208,  81, 228, 242,  72, 117, 158,  64, 104, 169, 114,
	121,  36, 187,  79,  32,  52, 213,  57, 189,  18, 222, 168,
	 16,  26, 235, 157, 223,   9, 111,  84,   8,  13, 246, 207,
	240, 133, 184,  42,   4, 135, 123, 232, 120, 195,  92,  21,
	  2, 196, 190, 116,  60, 226,  46, 139
};

/*
 * alpha^(127*i) + alpha^(125*i) mod 257
 */
static const unsigned short yoff_s_f[] = {
	  2, 156, 118, 107,  45, 212, 111, 162,  97, 249, 211,   3,
	 49, 101, 151, 223, 189, 178, 253, 204,  76,  82, 232,  65,
	 96, 176, 161,  47, 189,  61, 248, 107,   0, 131, 133, 113,
	 17,  33,  12, 111, 251, 103,  57, 148,  47,  65, 249, 143,
	189,   8, 204, 230, 205, 151, 187, 227, 247, 111, 140,   6,
	 77,  10,  21, 149, 255, 101, 139, 150, 212,  45, 146,  95,
	160,   8,  46, 254, 208, 156, 106,  34,  68,  79,   4,  53,
	181, 175,  25, 192, 161,  81,  96, 210,  68, 196,   9, 150,
	  0, 126, 124, 144, 240, 224, 245, 146,   6, 154, 200, 109,
	210, 192,   8, 114,  68, 249,  53,  27,  52, 106,  70,  30,
	 10, 146, 117, 251, 180, 247, 236, 108
};

/*
 * beta^(255*i) mod 257
 */
static const unsigned short yoff_b_n[] = {
	  1, 163,  98,  40,  95,  65,  58, 202,  30,   7, 113, 172,
	 23, 151, 198, 149, 129, 210,  49,  20, 176, 161,  29, 101,
	 15, 132, 185,  86, 140, 204,  99, 203, 193, 105, 153,  10,
	 88, 209, 143, 179, 136,  66, 221,  43,  70, 102, 178, 230,
	225, 181, 205,   5,  44, 233, 200, 218,  68,  33, 239, 150,
	 35,  51,  89, 115, 241, 219, 231, 131,  22, 245, 100, 109,
	 34, 145, 248,  75, 146, 154, 173, 186, 249, 238, 244, 194,
	 11, 251,  50, 183,  17, 201, 124, 166,  73,  77, 215,  93,
	253, 119, 122,  97, 134, 254,  25, 220, 137, 229,  62,  83,
	165, 167, 236, 175, 255, 188,  61, 177,  67, 127, 141, 110,
	197, 243,  31, 170, 211, 212, 118, 216, 256,  94, 159, 217,
	162, 192, 199,  55, 227, 250, 144,  85, 234, 106,  59, 108,
	128,  47, 208, 237,  81,  96, 228, 156, 242, 125,  72, 171,
	117,  53, 158,  54,  64, 152, 104, 247, 169,  48, 114,  78,
	121, 191,  36, 214, 187, 155,  79,  27,  32,  76,  52, 252,
	213,  24,  57,  39, 189, 224,  18, 107, 222, 206, 168, 142,
	 16,  38,  26, 126, 235,  12, 157, 148, 223, 112,   9, 182,
	111, 103,  84,  71,   8,  19,  13,  63, 246,   6, 207,  74,
	240,  56, 133,  91, 184, 180,  42, 164,   4, 138, 135, 160,
	123,   3, 232,  37, 120,  28, 195, 174,  92,  90,  21,  82,
	  2,  69, 196,  80, 190, 130, 116, 147,  60,  14, 226,  87,
	 46,  45, 139,  41
};

/*
 * beta^(255*i) + beta^(253*i) mod 257
 */
static const unsigned short yoff_b_f[] = {
	  2, 203, 156,  47, 118, 214, 107, 106,  45,  93, 212,  20,
	111,  73, 162, 251,  97, 215, 249,  53, 211,  19,   3,  89,
	 49, 207, 101,  67, 151, 130, 223,  23, 189, 202, 178, 239,
	253, 127, 204,  49,  76, 236,  82, 137, 232, 157,  65,  79,
	 96, 161, 176, 130, 161,  30,  47,   9, 189, 247,  61, 226,
	248,  90, 107,  64,   0,  88, 131, 243, 133,  59, 113, 115,
	 17, 236,  33, 213,  12, 191, 111,  19, 251,  61, 103, 208,
	 57,  35, 148, 248,  47, 116,  65, 119, 249, 178, 143,  40,
	189, 129,   8, 163, 204, 227, 230, 196, 205, 122, 151,  45,
	187,  19, 227,  72, 247, 125, 111, 121, 140, 220,   6, 107,
	 77,  69,  10, 101,  21,  65, 149, 171, 255,  54, 101, 210,
	139,  43, 150, 151, 212, 164,  45, 237, 146, 184,  95,   6,
	160,  42,   8, 204,  46, 238, 254, 168, 208,  50, 156, 190,
	106, 127,  34, 234,  68,  55,  79,  18,   4, 130,  53, 208,
	181,  21, 175, 120,  25, 100, 192, 178, 161,  96,  81, 127,
	 96, 227, 210, 248,  68,  10, 196,  31,   9, 167, 150, 193,
	  0, 169, 126,  14, 124, 198, 144, 142, 240,  21, 224,  44,
	245,  66, 146, 238,   6, 196, 154,  49, 200, 222, 109,   9,
	210, 141, 192, 138,   8,  79, 114, 217,  68, 128, 249,  94,
	 53,  30,  27,  61,  52, 135, 106, 212,  70, 238,  30, 185,
	 10, 132, 146, 136, 117,  37, 251, 150, 180, 188, 247, 156,
	236, 192, 108,  86
};

#define INNER(l, h, mm)   (((u32)((l) * (mm)) & 0xFFFFU) \
                          + ((u32)((h) * (mm)) << 16))

#define W_SMALL(sb, o1, o2, mm) \
	(INNER(q[8 * (sb) + 2 * 0 + o1], q[8 * (sb) + 2 * 0 + o2], mm), \
	 INNER(q[8 * (sb) + 2 * 1 + o1], q[8 * (sb) + 2 * 1 + o2], mm), \
	 INNER(q[8 * (sb) + 2 * 2 + o1], q[8 * (sb) + 2 * 2 + o2], mm), \
	 INNER(q[8 * (sb) + 2 * 3 + o1], q[8 * (sb) + 2 * 3 + o2], mm)

#define WS_0_0   W_SMALL( 4,    0,    1, 185)
#define WS_0_1   W_SMALL( 6,    0,    1, 185)
#define WS_0_2   W_SMALL( 0,    0,    1, 185)
#define WS_0_3   W_SMALL( 2,    0,    1, 185)
#define WS_0_4   W_SMALL( 7,    0,    1, 185)
#define WS_0_5   W_SMALL( 5,    0,    1, 185)
#define WS_0_6   W_SMALL( 3,    0,    1, 185)
#define WS_0_7   W_SMALL( 1,    0,    1, 185)
#define WS_1_0   W_SMALL(15,    0,    1, 185)
#define WS_1_1   W_SMALL(11,    0,    1, 185)
#define WS_1_2   W_SMALL(12,    0,    1, 185)
#define WS_1_3   W_SMALL( 8,    0,    1, 185)
#define WS_1_4   W_SMALL( 9,    0,    1, 185)
#define WS_1_5   W_SMALL(13,    0,    1, 185)
#define WS_1_6   W_SMALL(10,    0,    1, 185)
#define WS_1_7   W_SMALL(14,    0,    1, 185)
#define WS_2_0   W_SMALL(17, -128,  -64, 233)
#define WS_2_1   W_SMALL(18, -128,  -64, 233)
#define WS_2_2   W_SMALL(23, -128,  -64, 233)
#define WS_2_3   W_SMALL(20, -128,  -64, 233)
#define WS_2_4   W_SMALL(22, -128,  -64, 233)
#define WS_2_5   W_SMALL(21, -128,  -64, 233)
#define WS_2_6   W_SMALL(16, -128,  -64, 233)
#define WS_2_7   W_SMALL(19, -128,  -64, 233)
#define WS_3_0   W_SMALL(30, -191, -127, 233)
#define WS_3_1   W_SMALL(24, -191, -127, 233)
#define WS_3_2   W_SMALL(25, -191, -127, 233)
#define WS_3_3   W_SMALL(31, -191, -127, 233)
#define WS_3_4   W_SMALL(27, -191, -127, 233)
#define WS_3_5   W_SMALL(29, -191, -127, 233)
#define WS_3_6   W_SMALL(28, -191, -127, 233)
#define WS_3_7   W_SMALL(26, -191, -127, 233)

#define W_BIG(sb, o1, o2, mm) \
	(INNER(q[16 * (sb) + 2 * 0 + o1], q[16 * (sb) + 2 * 0 + o2], mm), \
	 INNER(q[16 * (sb) + 2 * 1 + o1], q[16 * (sb) + 2 * 1 + o2], mm), \
	 INNER(q[16 * (sb) + 2 * 2 + o1], q[16 * (sb) + 2 * 2 + o2], mm), \
	 INNER(q[16 * (sb) + 2 * 3 + o1], q[16 * (sb) + 2 * 3 + o2], mm), \
	 INNER(q[16 * (sb) + 2 * 4 + o1], q[16 * (sb) + 2 * 4 + o2], mm), \
	 INNER(q[16 * (sb) + 2 * 5 + o1], q[16 * (sb) + 2 * 5 + o2], mm), \
	 INNER(q[16 * (sb) + 2 * 6 + o1], q[16 * (sb) + 2 * 6 + o2], mm), \
	 INNER(q[16 * (sb) + 2 * 7 + o1], q[16 * (sb) + 2 * 7 + o2], mm)

#define WB_0_0   W_BIG( 4,    0,    1, 185)
#define WB_0_1   W_BIG( 6,    0,    1, 185)
#define WB_0_2   W_BIG( 0,    0,    1, 185)
#define WB_0_3   W_BIG( 2,    0,    1, 185)
#define WB_0_4   W_BIG( 7,    0,    1, 185)
#define WB_0_5   W_BIG( 5,    0,    1, 185)
#define WB_0_6   W_BIG( 3,    0,    1, 185)
#define WB_0_7   W_BIG( 1,    0,    1, 185)
#define WB_1_0   W_BIG(15,    0,    1, 185)
#define WB_1_1   W_BIG(11,    0,    1, 185)
#define WB_1_2   W_BIG(12,    0,    1, 185)
#define WB_1_3   W_BIG( 8,    0,    1, 185)
#define WB_1_4   W_BIG( 9,    0,    1, 185)
#define WB_1_5   W_BIG(13,    0,    1, 185)
#define WB_1_6   W_BIG(10,    0,    1, 185)
#define WB_1_7   W_BIG(14,    0,    1, 185)
#define WB_2_0   W_BIG(17, -256, -128, 233)
#define WB_2_1   W_BIG(18, -256, -128, 233)
#define WB_2_2   W_BIG(23, -256, -128, 233)
#define WB_2_3   W_BIG(20, -256, -128, 233)
#define WB_2_4   W_BIG(22, -256, -128, 233)
#define WB_2_5   W_BIG(21, -256, -128, 233)
#define WB_2_6   W_BIG(16, -256, -128, 233)
#define WB_2_7   W_BIG(19, -256, -128, 233)
#define WB_3_0   W_BIG(30, -383, -255, 233)
#define WB_3_1   W_BIG(24, -383, -255, 233)
#define WB_3_2   W_BIG(25, -383, -255, 233)
#define WB_3_3   W_BIG(31, -383, -255, 233)
#define WB_3_4   W_BIG(27, -383, -255, 233)
#define WB_3_5   W_BIG(29, -383, -255, 233)
#define WB_3_6   W_BIG(28, -383, -255, 233)
#define WB_3_7   W_BIG(26, -383, -255, 233)

#define IF(x, y, z)    ((((y) ^ (z)) & (x)) ^ (z))
#define MAJ(x, y, z)   (((x) & (y)) | (((x) | (y)) & (z)))

#define PP4_0_0   1
#define PP4_0_1   0
#define PP4_0_2   3
#define PP4_0_3   2
#define PP4_1_0   2
#define PP4_1_1   3
#define PP4_1_2   0
#define PP4_1_3   1
#define PP4_2_0   3
#define PP4_2_1   2
#define PP4_2_2   1
#define PP4_2_3   0

#define PP8_0_0   1
#define PP8_0_1   0
#define PP8_0_2   3
#define PP8_0_3   2
#define PP8_0_4   5
#define PP8_0_5   4
#define PP8_0_6   7
#define PP8_0_7   6

#define PP8_1_0   6
#define PP8_1_1   7
#define PP8_1_2   4
#define PP8_1_3   5
#define PP8_1_4   2
#define PP8_1_5   3
#define PP8_1_6   0
#define PP8_1_7   1

#define PP8_2_0   2
#define PP8_2_1   3
#define PP8_2_2   0
#define PP8_2_3   1
#define PP8_2_4   6
#define PP8_2_5   7
#define PP8_2_6   4
#define PP8_2_7   5

#define PP8_3_0   3
#define PP8_3_1   2
#define PP8_3_2   1
#define PP8_3_3   0
#define PP8_3_4   7
#define PP8_3_5   6
#define PP8_3_6   5
#define PP8_3_7   4

#define PP8_4_0   5
#define PP8_4_1   4
#define PP8_4_2   7
#define PP8_4_3   6
#define PP8_4_4   1
#define PP8_4_5   0
#define PP8_4_6   3
#define PP8_4_7   2

#define PP8_5_0   7
#define PP8_5_1   6
#define PP8_5_2   5
#define PP8_5_3   4
#define PP8_5_4   3
#define PP8_5_5   2
#define PP8_5_6   1
#define PP8_5_7   0

#define PP8_6_0   4
#define PP8_6_1   5
#define PP8_6_2   6
#define PP8_6_3   7
#define PP8_6_4   0
#define PP8_6_5   1
#define PP8_6_6   2
#define PP8_6_7   3

#if SPH_SIMD_NOCOPY

#define DECL_STATE_SMALL
#define READ_STATE_SMALL(sc)
#define WRITE_STATE_SMALL(sc)
#define DECL_STATE_BIG
#define READ_STATE_BIG(sc)
#define WRITE_STATE_BIG(sc)

#else

#define DECL_STATE_SMALL   \
	u32 A0, A1, A2, A3, B0, B1, B2, B3, C0, C1, C2, C3, D0, D1, D2, D3;

#define READ_STATE_SMALL(sc)   do { \
		A0 = (sc)->state[ 0]; \
		A1 = (sc)->state[ 1]; \
		A2 = (sc)->state[ 2]; \
		A3 = (sc)->state[ 3]; \
		B0 = (sc)->state[ 4]; \
		B1 = (sc)->state[ 5]; \
		B2 = (sc)->state[ 6]; \
		B3 = (sc)->state[ 7]; \
		C0 = (sc)->state[ 8]; \
		C1 = (sc)->state[ 9]; \
		C2 = (sc)->state[10]; \
		C3 = (sc)->state[11]; \
		D0 = (sc)->state[12]; \
		D1 = (sc)->state[13]; \
		D2 = (sc)->state[14]; \
		D3 = (sc)->state[15]; \
	} while (0)

#define WRITE_STATE_SMALL(sc)   do { \
		(sc)->state[ 0] = A0; \
		(sc)->state[ 1] = A1; \
		(sc)->state[ 2] = A2; \
		(sc)->state[ 3] = A3; \
		(sc)->state[ 4] = B0; \
		(sc)->state[ 5] = B1; \
		(sc)->state[ 6] = B2; \
		(sc)->state[ 7] = B3; \
		(sc)->state[ 8] = C0; \
		(sc)->state[ 9] = C1; \
		(sc)->state[10] = C2; \
		(sc)->state[11] = C3; \
		(sc)->state[12] = D0; \
		(sc)->state[13] = D1; \
		(sc)->state[14] = D2; \
		(sc)->state[15] = D3; \
	} while (0)

#define DECL_STATE_BIG   \
	u32 A0, A1, A2, A3, A4, A5, A6, A7; \
	u32 B0, B1, B2, B3, B4, B5, B6, B7; \
	u32 C0, C1, C2, C3, C4, C5, C6, C7; \
	u32 D0, D1, D2, D3, D4, D5, D6, D7;

#define READ_STATE_BIG(sc)   do { \
		A0 = (sc)->state[ 0]; \
		A1 = (sc)->state[ 1]; \
		A2 = (sc)->state[ 2]; \
		A3 = (sc)->state[ 3]; \
		A4 = (sc)->state[ 4]; \
		A5 = (sc)->state[ 5]; \
		A6 = (sc)->state[ 6]; \
		A7 = (sc)->state[ 7]; \
		B0 = (sc)->state[ 8]; \
		B1 = (sc)->state[ 9]; \
		B2 = (sc)->state[10]; \
		B3 = (sc)->state[11]; \
		B4 = (sc)->state[12]; \
		B5 = (sc)->state[13]; \
		B6 = (sc)->state[14]; \
		B7 = (sc)->state[15]; \
		C0 = (sc)->state[16]; \
		C1 = (sc)->state[17]; \
		C2 = (sc)->state[18]; \
		C3 = (sc)->state[19]; \
		C4 = (sc)->state[20]; \
		C5 = (sc)->state[21]; \
		C6 = (sc)->state[22]; \
		C7 = (sc)->state[23]; \
		D0 = (sc)->state[24]; \
		D1 = (sc)->state[25]; \
		D2 = (sc)->state[26]; \
		D3 = (sc)->state[27]; \
		D4 = (sc)->state[28]; \
		D5 = (sc)->state[29]; \
		D6 = (sc)->state[30]; \
		D7 = (sc)->state[31]; \
	} while (0)

#define WRITE_STATE_BIG(sc)   do { \
		(sc)->state[ 0] = A0; \
		(sc)->state[ 1] = A1; \
		(sc)->state[ 2] = A2; \
		(sc)->state[ 3] = A3; \
		(sc)->state[ 4] = A4; \
		(sc)->state[ 5] = A5; \
		(sc)->state[ 6] = A6; \
		(sc)->state[ 7] = A7; \
		(sc)->state[ 8] = B0; \
		(sc)->state[ 9] = B1; \
		(sc)->state[10] = B2; \
		(sc)->state[11] = B3; \
		(sc)->state[12] = B4; \
		(sc)->state[13] = B5; \
		(sc)->state[14] = B6; \
		(sc)->state[15] = B7; \
		(sc)->state[16] = C0; \
		(sc)->state[17] = C1; \
		(sc)->state[18] = C2; \
		(sc)->state[19] = C3; \
		(sc)->state[20] = C4; \
		(sc)->state[21] = C5; \
		(sc)->state[22] = C6; \
		(sc)->state[23] = C7; \
		(sc)->state[24] = D0; \
		(sc)->state[25] = D1; \
		(sc)->state[26] = D2; \
		(sc)->state[27] = D3; \
		(sc)->state[28] = D4; \
		(sc)->state[29] = D5; \
		(sc)->state[30] = D6; \
		(sc)->state[31] = D7; \
	} while (0)

#endif

#define STEP_ELT(n, w, fun, s, ppb)   do { \
		u32 tt = T32(D ## n + (w) + fun(A ## n, B ## n, C ## n)); \
		A ## n = T32(ROL32(tt, s) + XCAT(tA, XCAT(ppb, n))); \
		D ## n = C ## n; \
		C ## n = B ## n; \
		B ## n = tA ## n; \
	} while (0)

#define STEP_SMALL(w0, w1, w2, w3, fun, r, s, pp4b)   do { \
		u32 tA0 = ROL32(A0, r); \
		u32 tA1 = ROL32(A1, r); \
		u32 tA2 = ROL32(A2, r); \
		u32 tA3 = ROL32(A3, r); \
		STEP_ELT(0, w0, fun, s, pp4b); \
		STEP_ELT(1, w1, fun, s, pp4b); \
		STEP_ELT(2, w2, fun, s, pp4b); \
		STEP_ELT(3, w3, fun, s, pp4b); \
	} while (0)

#define STEP_BIG(w0, w1, w2, w3, w4, w5, w6, w7, fun, r, s, pp8b)   do { \
		u32 tA0 = ROL32(A0, r); \
		u32 tA1 = ROL32(A1, r); \
		u32 tA2 = ROL32(A2, r); \
		u32 tA3 = ROL32(A3, r); \
		u32 tA4 = ROL32(A4, r); \
		u32 tA5 = ROL32(A5, r); \
		u32 tA6 = ROL32(A6, r); \
		u32 tA7 = ROL32(A7, r); \
		STEP_ELT(0, w0, fun, s, pp8b); \
		STEP_ELT(1, w1, fun, s, pp8b); \
		STEP_ELT(2, w2, fun, s, pp8b); \
		STEP_ELT(3, w3, fun, s, pp8b); \
		STEP_ELT(4, w4, fun, s, pp8b); \
		STEP_ELT(5, w5, fun, s, pp8b); \
		STEP_ELT(6, w6, fun, s, pp8b); \
		STEP_ELT(7, w7, fun, s, pp8b); \
	} while (0)

#define M3_0_0   0_
#define M3_1_0   1_
#define M3_2_0   2_
#define M3_3_0   0_
#define M3_4_0   1_
#define M3_5_0   2_
#define M3_6_0   0_
#define M3_7_0   1_

#define M3_0_1   1_
#define M3_1_1   2_
#define M3_2_1   0_
#define M3_3_1   1_
#define M3_4_1   2_
#define M3_5_1   0_
#define M3_6_1   1_
#define M3_7_1   2_

#define M3_0_2   2_
#define M3_1_2   0_
#define M3_2_2   1_
#define M3_3_2   2_
#define M3_4_2   0_
#define M3_5_2   1_
#define M3_6_2   2_
#define M3_7_2   0_

#define STEP_SMALL_(w, fun, r, s, pp4b)   STEP_SMALL w, fun, r, s, pp4b)

#define ONE_ROUND_SMALL(ri, isp, p0, p1, p2, p3)   do { \
		STEP_SMALL_(WS_ ## ri ## 0, \
			IF,  p0, p1, XCAT(PP4_, M3_0_ ## isp)); \
		STEP_SMALL_(WS_ ## ri ## 1, \
			IF,  p1, p2, XCAT(PP4_, M3_1_ ## isp)); \
		STEP_SMALL_(WS_ ## ri ## 2, \
			IF,  p2, p3, XCAT(PP4_, M3_2_ ## isp)); \
		STEP_SMALL_(WS_ ## ri ## 3, \
			IF,  p3, p0, XCAT(PP4_, M3_3_ ## isp)); \
		STEP_SMALL_(WS_ ## ri ## 4, \
			MAJ, p0, p1, XCAT(PP4_, M3_4_ ## isp)); \
		STEP_SMALL_(WS_ ## ri ## 5, \
			MAJ, p1, p2, XCAT(PP4_, M3_5_ ## isp)); \
		STEP_SMALL_(WS_ ## ri ## 6, \
			MAJ, p2, p3, XCAT(PP4_, M3_6_ ## isp)); \
		STEP_SMALL_(WS_ ## ri ## 7, \
			MAJ, p3, p0, XCAT(PP4_, M3_7_ ## isp)); \
	} while (0)

#define M7_0_0   0_
#define M7_1_0   1_
#define M7_2_0   2_
#define M7_3_0   3_
#define M7_4_0   4_
#define M7_5_0   5_
#define M7_6_0   6_
#define M7_7_0   0_

#define M7_0_1   1_
#define M7_1_1   2_
#define M7_2_1   3_
#define M7_3_1   4_
#define M7_4_1   5_
#define M7_5_1   6_
#define M7_6_1   0_
#define M7_7_1   1_

#define M7_0_2   2_
#define M7_1_2   3_
#define M7_2_2   4_
#define M7_3_2   5_
#define M7_4_2   6_
#define M7_5_2   0_
#define M7_6_2   1_
#define M7_7_2   2_

#define M7_0_3   3_
#define M7_1_3   4_
#define M7_2_3   5_
#define M7_3_3   6_
#define M7_4_3   0_
#define M7_5_3   1_
#define M7_6_3   2_
#define M7_7_3   3_

#define STEP_BIG_(w, fun, r, s, pp8b)   STEP_BIG w, fun, r, s, pp8b)

#define ONE_ROUND_BIG(ri, isp, p0, p1, p2, p3)   do { \
		STEP_BIG_(WB_ ## ri ## 0, \
			IF,  p0, p1, XCAT(PP8_, M7_0_ ## isp)); \
		STEP_BIG_(WB_ ## ri ## 1, \
			IF,  p1, p2, XCAT(PP8_, M7_1_ ## isp)); \
		STEP_BIG_(WB_ ## ri ## 2, \
			IF,  p2, p3, XCAT(PP8_, M7_2_ ## isp)); \
		STEP_BIG_(WB_ ## ri ## 3, \
			IF,  p3, p0, XCAT(PP8_, M7_3_ ## isp)); \
		STEP_BIG_(WB_ ## ri ## 4, \
			MAJ, p0, p1, XCAT(PP8_, M7_4_ ## isp)); \
		STEP_BIG_(WB_ ## ri ## 5, \
			MAJ, p1, p2, XCAT(PP8_, M7_5_ ## isp)); \
		STEP_BIG_(WB_ ## ri ## 6, \
			MAJ, p2, p3, XCAT(PP8_, M7_6_ ## isp)); \
		STEP_BIG_(WB_ ## ri ## 7, \
			MAJ, p3, p0, XCAT(PP8_, M7_7_ ## isp)); \
	} while (0)

#if SPH_SMALL_FOOTPRINT_SIMD

#define A0   state[ 0]
#define A1   state[ 1]
#define A2   state[ 2]
#define A3   state[ 3]
#define B0   state[ 4]
#define B1   state[ 5]
#define B2   state[ 6]
#define B3   state[ 7]
#define C0   state[ 8]
#define C1   state[ 9]
#define C2   state[10]
#define C3   state[11]
#define D0   state[12]
#define D1   state[13]
#define D2   state[14]
#define D3   state[15]

#define STEP2_ELT(n, w, fun, s, ppb)   do { \
		u32 tt = T32(D ## n + (w) + fun(A ## n, B ## n, C ## n)); \
		A ## n = T32(ROL32(tt, s) + tA[(ppb) ^ n]); \
		D ## n = C ## n; \
		C ## n = B ## n; \
		B ## n = tA[n]; \
	} while (0)

#define STEP2_SMALL(w0, w1, w2, w3, fun, r, s, pp4b)   do { \
		u32 tA[4]; \
		tA[0] = ROL32(A0, r); \
		tA[1] = ROL32(A1, r); \
		tA[2] = ROL32(A2, r); \
		tA[3] = ROL32(A3, r); \
		STEP2_ELT(0, w0, fun, s, pp4b); \
		STEP2_ELT(1, w1, fun, s, pp4b); \
		STEP2_ELT(2, w2, fun, s, pp4b); \
		STEP2_ELT(3, w3, fun, s, pp4b); \
	} while (0)

static void
one_round_small(u32 *state, u32 *w, int isp, int p0, int p1, int p2, int p3)
{
	static const int pp4k[] = { 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2 };

	STEP2_SMALL(w[ 0], w[ 1], w[ 2], w[ 3], IF,  p0, p1, pp4k[isp + 0]);
	STEP2_SMALL(w[ 4], w[ 5], w[ 6], w[ 7], IF,  p1, p2, pp4k[isp + 1]);
	STEP2_SMALL(w[ 8], w[ 9], w[10], w[11], IF,  p2, p3, pp4k[isp + 2]);
	STEP2_SMALL(w[12], w[13], w[14], w[15], IF,  p3, p0, pp4k[isp + 3]);
	STEP2_SMALL(w[16], w[17], w[18], w[19], MAJ, p0, p1, pp4k[isp + 4]);
	STEP2_SMALL(w[20], w[21], w[22], w[23], MAJ, p1, p2, pp4k[isp + 5]);
	STEP2_SMALL(w[24], w[25], w[26], w[27], MAJ, p2, p3, pp4k[isp + 6]);
	STEP2_SMALL(w[28], w[29], w[30], w[31], MAJ, p3, p0, pp4k[isp + 7]);
}

static void
compress_small(sph_simd_small_context *sc, int last)
{
	unsigned char *x;
	s32 q[128];
	int i;
	u32 w[32];
	u32 state[16];
	size_t u;

	static const size_t wsp[32] = {
		 4 << 3,  6 << 3,  0 << 3,  2 << 3,
		 7 << 3,  5 << 3,  3 << 3,  1 << 3,
		15 << 3, 11 << 3, 12 << 3,  8 << 3,
		 9 << 3, 13 << 3, 10 << 3, 14 << 3,
		17 << 3, 18 << 3, 23 << 3, 20 << 3,
		22 << 3, 21 << 3, 16 << 3, 19 << 3,
		30 << 3, 24 << 3, 25 << 3, 31 << 3,
		27 << 3, 29 << 3, 28 << 3, 26 << 3
	};

	x = sc->buf;
	FFT128(0, 1, 0, ll);
	if (last) {
		for (i = 0; i < 128; i ++) {
			s32 tq;

			tq = q[i] + yoff_s_f[i];
			tq = REDS2(tq);
			tq = REDS1(tq);
			tq = REDS1(tq);
			q[i] = (tq <= 128 ? tq : tq - 257);
		}
	} else {
		for (i = 0; i < 128; i ++) {
			s32 tq;

			tq = q[i] + yoff_s_n[i];
			tq = REDS2(tq);
			tq = REDS1(tq);
			tq = REDS1(tq);
			q[i] = (tq <= 128 ? tq : tq - 257);
		}
	}

	for (i = 0; i < 16; i += 4) {
		state[i + 0] = sc->state[i + 0]
			^ sph_dec32le_aligned(x + 4 * (i + 0));
		state[i + 1] = sc->state[i + 1]
			^ sph_dec32le_aligned(x + 4 * (i + 1));
		state[i + 2] = sc->state[i + 2]
			^ sph_dec32le_aligned(x + 4 * (i + 2));
		state[i + 3] = sc->state[i + 3]
			^ sph_dec32le_aligned(x + 4 * (i + 3));
	}

#define WSREAD(sb, o1, o2, mm)   do { \
		for (u = 0; u < 32; u += 4) { \
			size_t v = wsp[(u >> 2) + (sb)]; \
			w[u + 0] = INNER(q[v + 2 * 0 + (o1)], \
				q[v + 2 * 0 + (o2)], mm); \
			w[u + 1] = INNER(q[v + 2 * 1 + (o1)], \
				q[v + 2 * 1 + (o2)], mm); \
			w[u + 2] = INNER(q[v + 2 * 2 + (o1)], \
				q[v + 2 * 2 + (o2)], mm); \
			w[u + 3] = INNER(q[v + 2 * 3 + (o1)], \
				q[v + 2 * 3 + (o2)], mm); \
		} \
	} while (0)

	WSREAD( 0,    0,    1, 185);
	one_round_small(state, w, 0,  3, 23, 17, 27);
	WSREAD( 8,    0,    1, 185);
	one_round_small(state, w, 2, 28, 19, 22,  7);
	WSREAD(16, -128,  -64, 233);
	one_round_small(state, w, 1, 29,  9, 15,  5);
	WSREAD(24, -191, -127, 233);
	one_round_small(state, w, 0,  4, 13, 10, 25);

#undef WSREAD

	STEP_SMALL(sc->state[ 0], sc->state[ 1], sc->state[ 2], sc->state[ 3],
		IF,  4, 13, PP4_2_);
	STEP_SMALL(sc->state[ 4], sc->state[ 5], sc->state[ 6], sc->state[ 7],
		IF, 13, 10, PP4_0_);
	STEP_SMALL(sc->state[ 8], sc->state[ 9], sc->state[10], sc->state[11],
		IF, 10, 25, PP4_1_);
	STEP_SMALL(sc->state[12], sc->state[13], sc->state[14], sc->state[15],
		IF, 25,  4, PP4_2_);

	memcpy(sc->state, state, sizeof state);
}

#undef A0
#undef A1
#undef A2
#undef A3
#undef B0
#undef B1
#undef B2
#undef B3
#undef C0
#undef C1
#undef C2
#undef C3
#undef D0
#undef D1
#undef D2
#undef D3

#else

#if SPH_SIMD_NOCOPY
#define A0   (sc->state[ 0])
#define A1   (sc->state[ 1])
#define A2   (sc->state[ 2])
#define A3   (sc->state[ 3])
#define B0   (sc->state[ 4])
#define B1   (sc->state[ 5])
#define B2   (sc->state[ 6])
#define B3   (sc->state[ 7])
#define C0   (sc->state[ 8])
#define C1   (sc->state[ 9])
#define C2   (sc->state[10])
#define C3   (sc->state[11])
#define D0   (sc->state[12])
#define D1   (sc->state[13])
#define D2   (sc->state[14])
#define D3   (sc->state[15])
#endif

static void
compress_small(sph_simd_small_context *sc, int last)
{
	unsigned char *x;
	s32 q[128];
	int i;
	DECL_STATE_SMALL
#if SPH_SIMD_NOCOPY
	sph_u32 saved[16];
#endif

#if SPH_SIMD_NOCOPY
	memcpy(saved, sc->state, sizeof saved);
#endif
	x = sc->buf;
	FFT128(0, 1, 0, ll);
	if (last) {
		for (i = 0; i < 128; i ++) {
			s32 tq;

			tq = q[i] + yoff_s_f[i];
			tq = REDS2(tq);
			tq = REDS1(tq);
			tq = REDS1(tq);
			q[i] = (tq <= 128 ? tq : tq - 257);
		}
	} else {
		for (i = 0; i < 128; i ++) {
			s32 tq;

			tq = q[i] + yoff_s_n[i];
			tq = REDS2(tq);
			tq = REDS1(tq);
			tq = REDS1(tq);
			q[i] = (tq <= 128 ? tq : tq - 257);
		}
	}
	READ_STATE_SMALL(sc);
	A0 ^= sph_dec32le_aligned(x +  0);
	A1 ^= sph_dec32le_aligned(x +  4);
	A2 ^= sph_dec32le_aligned(x +  8);
	A3 ^= sph_dec32le_aligned(x + 12);
	B0 ^= sph_dec32le_aligned(x + 16);
	B1 ^= sph_dec32le_aligned(x + 20);
	B2 ^= sph_dec32le_aligned(x + 24);
	B3 ^= sph_dec32le_aligned(x + 28);
	C0 ^= sph_dec32le_aligned(x + 32);
	C1 ^= sph_dec32le_aligned(x + 36);
	C2 ^= sph_dec32le_aligned(x + 40);
	C3 ^= sph_dec32le_aligned(x + 44);
	D0 ^= sph_dec32le_aligned(x + 48);
	D1 ^= sph_dec32le_aligned(x + 52);
	D2 ^= sph_dec32le_aligned(x + 56);
	D3 ^= sph_dec32le_aligned(x + 60);
	ONE_ROUND_SMALL(0_, 0,  3, 23, 17, 27);
	ONE_ROUND_SMALL(1_, 2, 28, 19, 22,  7);
	ONE_ROUND_SMALL(2_, 1, 29,  9, 15,  5);
	ONE_ROUND_SMALL(3_, 0,  4, 13, 10, 25);
#if SPH_SIMD_NOCOPY
	STEP_SMALL(saved[ 0], saved[ 1], saved[ 2], saved[ 3],
		IF,  4, 13, PP4_2_);
	STEP_SMALL(saved[ 4], saved[ 5], saved[ 6], saved[ 7],
		IF, 13, 10, PP4_0_);
	STEP_SMALL(saved[ 8], saved[ 9], saved[10], saved[11],
		IF, 10, 25, PP4_1_);
	STEP_SMALL(saved[12], saved[13], saved[14], saved[15],
		IF, 25,  4, PP4_2_);
#else
	STEP_SMALL(sc->state[ 0], sc->state[ 1], sc->state[ 2], sc->state[ 3],
		IF,  4, 13, PP4_2_);
	STEP_SMALL(sc->state[ 4], sc->state[ 5], sc->state[ 6], sc->state[ 7],
		IF, 13, 10, PP4_0_);
	STEP_SMALL(sc->state[ 8], sc->state[ 9], sc->state[10], sc->state[11],
		IF, 10, 25, PP4_1_);
	STEP_SMALL(sc->state[12], sc->state[13], sc->state[14], sc->state[15],
		IF, 25,  4, PP4_2_);
	WRITE_STATE_SMALL(sc);
#endif
}

#if SPH_SIMD_NOCOPY
#undef A0
#undef A1
#undef A2
#undef A3
#undef B0
#undef B1
#undef B2
#undef B3
#undef C0
#undef C1
#undef C2
#undef C3
#undef D0
#undef D1
#undef D2
#undef D3
#endif

#endif

#if SPH_SMALL_FOOTPRINT_SIMD

#define A0   state[ 0]
#define A1   state[ 1]
#define A2   state[ 2]
#define A3   state[ 3]
#define A4   state[ 4]
#define A5   state[ 5]
#define A6   state[ 6]
#define A7   state[ 7]
#define B0   state[ 8]
#define B1   state[ 9]
#define B2   state[10]
#define B3   state[11]
#define B4   state[12]
#define B5   state[13]
#define B6   state[14]
#define B7   state[15]
#define C0   state[16]
#define C1   state[17]
#define C2   state[18]
#define C3   state[19]
#define C4   state[20]
#define C5   state[21]
#define C6   state[22]
#define C7   state[23]
#define D0   state[24]
#define D1   state[25]
#define D2   state[26]
#define D3   state[27]
#define D4   state[28]
#define D5   state[29]
#define D6   state[30]
#define D7   state[31]

/*
 * Not needed -- already defined for SIMD-224 / SIMD-256
 *
#define STEP2_ELT(n, w, fun, s, ppb)   do { \
		u32 tt = T32(D ## n + (w) + fun(A ## n, B ## n, C ## n)); \
		A ## n = T32(ROL32(tt, s) + tA[(ppb) ^ n]); \
		D ## n = C ## n; \
		C ## n = B ## n; \
		B ## n = tA[n]; \
	} while (0)
 */

#define STEP2_BIG(w0, w1, w2, w3, w4, w5, w6, w7, fun, r, s, pp8b)   do { \
		u32 tA[8]; \
		tA[0] = ROL32(A0, r); \
		tA[1] = ROL32(A1, r); \
		tA[2] = ROL32(A2, r); \
		tA[3] = ROL32(A3, r); \
		tA[4] = ROL32(A4, r); \
		tA[5] = ROL32(A5, r); \
		tA[6] = ROL32(A6, r); \
		tA[7] = ROL32(A7, r); \
		STEP2_ELT(0, w0, fun, s, pp8b); \
		STEP2_ELT(1, w1, fun, s, pp8b); \
		STEP2_ELT(2, w2, fun, s, pp8b); \
		STEP2_ELT(3, w3, fun, s, pp8b); \
		STEP2_ELT(4, w4, fun, s, pp8b); \
		STEP2_ELT(5, w5, fun, s, pp8b); \
		STEP2_ELT(6, w6, fun, s, pp8b); \
		STEP2_ELT(7, w7, fun, s, pp8b); \
	} while (0)

static void
one_round_big(u32 *state, u32 *w, int isp, int p0, int p1, int p2, int p3)
{
	static const int pp8k[] = { 1, 6, 2, 3, 5, 7, 4, 1, 6, 2, 3 };

	STEP2_BIG(w[ 0], w[ 1], w[ 2], w[ 3], w[ 4], w[ 5], w[ 6], w[ 7],
		IF,  p0, p1, pp8k[isp + 0]);
	STEP2_BIG(w[ 8], w[ 9], w[10], w[11], w[12], w[13], w[14], w[15],
		IF,  p1, p2, pp8k[isp + 1]);
	STEP2_BIG(w[16], w[17], w[18], w[19], w[20], w[21], w[22], w[23],
		IF,  p2, p3, pp8k[isp + 2]);
	STEP2_BIG(w[24], w[25], w[26], w[27], w[28], w[29], w[30], w[31],
		IF,  p3, p0, pp8k[isp + 3]);
	STEP2_BIG(w[32], w[33], w[34], w[35], w[36], w[37], w[38], w[39],
		MAJ, p0, p1, pp8k[isp + 4]);
	STEP2_BIG(w[40], w[41], w[42], w[43], w[44], w[45], w[46], w[47],
		MAJ, p1, p2, pp8k[isp + 5]);
	STEP2_BIG(w[48], w[49], w[50], w[51], w[52], w[53], w[54], w[55],
		MAJ, p2, p3, pp8k[isp + 6]);
	STEP2_BIG(w[56], w[57], w[58], w[59], w[60], w[61], w[62], w[63],
		MAJ, p3, p0, pp8k[isp + 7]);
}

static void
compress_big(sph_simd_big_context *sc, int last)
{
	unsigned char *x;
	s32 q[256];
	int i;
	u32 w[64];
	u32 state[32];
	size_t u;

	static const size_t wbp[32] = {
		 4 << 4,  6 << 4,  0 << 4,  2 << 4,
		 7 << 4,  5 << 4,  3 << 4,  1 << 4,
		15 << 4, 11 << 4, 12 << 4,  8 << 4,
		 9 << 4, 13 << 4, 10 << 4, 14 << 4,
		17 << 4, 18 << 4, 23 << 4, 20 << 4,
		22 << 4, 21 << 4, 16 << 4, 19 << 4,
		30 << 4, 24 << 4, 25 << 4, 31 << 4,
		27 << 4, 29 << 4, 28 << 4, 26 << 4
	};

	x = sc->buf;
	FFT256(0, 1, 0, ll);
	if (last) {
		for (i = 0; i < 256; i ++) {
			s32 tq;

			tq = q[i] + yoff_b_f[i];
			tq = REDS2(tq);
			tq = REDS1(tq);
			tq = REDS1(tq);
			q[i] = (tq <= 128 ? tq : tq - 257);
		}
	} else {
		for (i = 0; i < 256; i ++) {
			s32 tq;

			tq = q[i] + yoff_b_n[i];
			tq = REDS2(tq);
			tq = REDS1(tq);
			tq = REDS1(tq);
			q[i] = (tq <= 128 ? tq : tq - 257);
		}
	}

	for (i = 0; i < 32; i += 8) {
		state[i + 0] = sc->state[i + 0]
			^ sph_dec32le_aligned(x + 4 * (i + 0));
		state[i + 1] = sc->state[i + 1]
			^ sph_dec32le_aligned(x + 4 * (i + 1));
		state[i + 2] = sc->state[i + 2]
			^ sph_dec32le_aligned(x + 4 * (i + 2));
		state[i + 3] = sc->state[i + 3]
			^ sph_dec32le_aligned(x + 4 * (i + 3));
		state[i + 4] = sc->state[i + 4]
			^ sph_dec32le_aligned(x + 4 * (i + 4));
		state[i + 5] = sc->state[i + 5]
			^ sph_dec32le_aligned(x + 4 * (i + 5));
		state[i + 6] = sc->state[i + 6]
			^ sph_dec32le_aligned(x + 4 * (i + 6));
		state[i + 7] = sc->state[i + 7]
			^ sph_dec32le_aligned(x + 4 * (i + 7));
	}

#define WBREAD(sb, o1, o2, mm)   do { \
		for (u = 0; u < 64; u += 8) { \
			size_t v = wbp[(u >> 3) + (sb)]; \
			w[u + 0] = INNER(q[v + 2 * 0 + (o1)], \
				q[v + 2 * 0 + (o2)], mm); \
			w[u + 1] = INNER(q[v + 2 * 1 + (o1)], \
				q[v + 2 * 1 + (o2)], mm); \
			w[u + 2] = INNER(q[v + 2 * 2 + (o1)], \
				q[v + 2 * 2 + (o2)], mm); \
			w[u + 3] = INNER(q[v + 2 * 3 + (o1)], \
				q[v + 2 * 3 + (o2)], mm); \
			w[u + 4] = INNER(q[v + 2 * 4 + (o1)], \
				q[v + 2 * 4 + (o2)], mm); \
			w[u + 5] = INNER(q[v + 2 * 5 + (o1)], \
				q[v + 2 * 5 + (o2)], mm); \
			w[u + 6] = INNER(q[v + 2 * 6 + (o1)], \
				q[v + 2 * 6 + (o2)], mm); \
			w[u + 7] = INNER(q[v + 2 * 7 + (o1)], \
				q[v + 2 * 7 + (o2)], mm); \
		} \
	} while (0)

	WBREAD( 0,    0,    1, 185);
	one_round_big(state, w, 0,  3, 23, 17, 27);
	WBREAD( 8,    0,    1, 185);
	one_round_big(state, w, 1, 28, 19, 22,  7);
	WBREAD(16, -256, -128, 233);
	one_round_big(state, w, 2, 29,  9, 15,  5);
	WBREAD(24, -383, -255, 233);
	one_round_big(state, w, 3,  4, 13, 10, 25);

#undef WBREAD

	STEP_BIG(
		sc->state[ 0], sc->state[ 1], sc->state[ 2], sc->state[ 3],
		sc->state[ 4], sc->state[ 5], sc->state[ 6], sc->state[ 7],
		IF,  4, 13, PP8_4_);
	STEP_BIG(
		sc->state[ 8], sc->state[ 9], sc->state[10], sc->state[11],
		sc->state[12], sc->state[13], sc->state[14], sc->state[15],
		IF, 13, 10, PP8_5_);
	STEP_BIG(
		sc->state[16], sc->state[17], sc->state[18], sc->state[19],
		sc->state[20], sc->state[21], sc->state[22], sc->state[23],
		IF, 10, 25, PP8_6_);
	STEP_BIG(
		sc->state[24], sc->state[25], sc->state[26], sc->state[27],
		sc->state[28], sc->state[29], sc->state[30], sc->state[31],
		IF, 25,  4, PP8_0_);

	memcpy(sc->state, state, sizeof state);
}

#undef A0
#undef A1
#undef A2
#undef A3
#undef A4
#undef A5
#undef A6
#undef A7
#undef B0
#undef B1
#undef B2
#undef B3
#undef B4
#undef B5
#undef B6
#undef B7
#undef C0
#undef C1
#undef C2
#undef C3
#undef C4
#undef C5
#undef C6
#undef C7
#undef D0
#undef D1
#undef D2
#undef D3
#undef D4
#undef D5
#undef D6
#undef D7

#else

#if SPH_SIMD_NOCOPY
#define A0   (sc->state[ 0])
#define A1   (sc->state[ 1])
#define A2   (sc->state[ 2])
#define A3   (sc->state[ 3])
#define A4   (sc->state[ 4])
#define A5   (sc->state[ 5])
#define A6   (sc->state[ 6])
#define A7   (sc->state[ 7])
#define B0   (sc->state[ 8])
#define B1   (sc->state[ 9])
#define B2   (sc->state[10])
#define B3   (sc->state[11])
#define B4   (sc->state[12])
#define B5   (sc->state[13])
#define B6   (sc->state[14])
#define B7   (sc->state[15])
#define C0   (sc->state[16])
#define C1   (sc->state[17])
#define C2   (sc->state[18])
#define C3   (sc->state[19])
#define C4   (sc->state[20])
#define C5   (sc->state[21])
#define C6   (sc->state[22])
#define C7   (sc->state[23])
#define D0   (sc->state[24])
#define D1   (sc->state[25])
#define D2   (sc->state[26])
#define D3   (sc->state[27])
#define D4   (sc->state[28])
#define D5   (sc->state[29])
#define D6   (sc->state[30])
#define D7   (sc->state[31])
#endif

static void
compress_big(sph_simd_big_context *sc, int last)
{
	unsigned char *x;
	s32 q[256];
	int i;
	DECL_STATE_BIG
#if SPH_SIMD_NOCOPY
	sph_u32 saved[32];
#endif

#if SPH_SIMD_NOCOPY
	memcpy(saved, sc->state, sizeof saved);
#endif

	x = sc->buf;
	FFT256(0, 1, 0, ll);
	if (last) {
		for (i = 0; i < 256; i ++) {
			s32 tq;

			tq = q[i] + yoff_b_f[i];
			tq = REDS2(tq);
			tq = REDS1(tq);
			tq = REDS1(tq);
			q[i] = (tq <= 128 ? tq : tq - 257);
		}
	} else {
		for (i = 0; i < 256; i ++) {
			s32 tq;

			tq = q[i] + yoff_b_n[i];
			tq = REDS2(tq);
			tq = REDS1(tq);
			tq = REDS1(tq);
			q[i] = (tq <= 128 ? tq : tq - 257);
		}
	}
	READ_STATE_BIG(sc);
	A0 ^= sph_dec32le_aligned(x +   0);
	A1 ^= sph_dec32le_aligned(x +   4);
	A2 ^= sph_dec32le_aligned(x +   8);
	A3 ^= sph_dec32le_aligned(x +  12);
	A4 ^= sph_dec32le_aligned(x +  16);
	A5 ^= sph_dec32le_aligned(x +  20);
	A6 ^= sph_dec32le_aligned(x +  24);
	A7 ^= sph_dec32le_aligned(x +  28);
	B0 ^= sph_dec32le_aligned(x +  32);
	B1 ^= sph_dec32le_aligned(x +  36);
	B2 ^= sph_dec32le_aligned(x +  40);
	B3 ^= sph_dec32le_aligned(x +  44);
	B4 ^= sph_dec32le_aligned(x +  48);
	B5 ^= sph_dec32le_aligned(x +  52);
	B6 ^= sph_dec32le_aligned(x +  56);
	B7 ^= sph_dec32le_aligned(x +  60);
	C0 ^= sph_dec32le_aligned(x +  64);
	C1 ^= sph_dec32le_aligned(x +  68);
	C2 ^= sph_dec32le_aligned(x +  72);
	C3 ^= sph_dec32le_aligned(x +  76);
	C4 ^= sph_dec32le_aligned(x +  80);
	C5 ^= sph_dec32le_aligned(x +  84);
	C6 ^= sph_dec32le_aligned(x +  88);
	C7 ^= sph_dec32le_aligned(x +  92);
	D0 ^= sph_dec32le_aligned(x +  96);
	D1 ^= sph_dec32le_aligned(x + 100);
	D2 ^= sph_dec32le_aligned(x + 104);
	D3 ^= sph_dec32le_aligned(x + 108);
	D4 ^= sph_dec32le_aligned(x + 112);
	D5 ^= sph_dec32le_aligned(x + 116);
	D6 ^= sph_dec32le_aligned(x + 120);
	D7 ^= sph_dec32le_aligned(x + 124);

	ONE_ROUND_BIG(0_, 0,  3, 23, 17, 27);
	ONE_ROUND_BIG(1_, 1, 28, 19, 22,  7);
	ONE_ROUND_BIG(2_, 2, 29,  9, 15,  5);
	ONE_ROUND_BIG(3_, 3,  4, 13, 10, 25);
#if SPH_SIMD_NOCOPY
	STEP_BIG(
		saved[ 0], saved[ 1], saved[ 2], saved[ 3],
		saved[ 4], saved[ 5], saved[ 6], saved[ 7],
		IF,  4, 13, PP8_4_);
	STEP_BIG(
		saved[ 8], saved[ 9], saved[10], saved[11],
		saved[12], saved[13], saved[14], saved[15],
		IF, 13, 10, PP8_5_);
	STEP_BIG(
		saved[16], saved[17], saved[18], saved[19],
		saved[20], saved[21], saved[22], saved[23],
		IF, 10, 25, PP8_6_);
	STEP_BIG(
		saved[24], saved[25], saved[26], saved[27],
		saved[28], saved[29], saved[30], saved[31],
		IF, 25,  4, PP8_0_);
#else
	STEP_BIG(
		sc->state[ 0], sc->state[ 1], sc->state[ 2], sc->state[ 3],
		sc->state[ 4], sc->state[ 5], sc->state[ 6], sc->state[ 7],
		IF,  4, 13, PP8_4_);
	STEP_BIG(
		sc->state[ 8], sc->state[ 9], sc->state[10], sc->state[11],
		sc->state[12], sc->state[13], sc->state[14], sc->state[15],
		IF, 13, 10, PP8_5_);
	STEP_BIG(
		sc->state[16], sc->state[17], sc->state[18], sc->state[19],
		sc->state[20], sc->state[21], sc->state[22], sc->state[23],
		IF, 10, 25, PP8_6_);
	STEP_BIG(
		sc->state[24], sc->state[25], sc->state[26], sc->state[27],
		sc->state[28], sc->state[29], sc->state[30], sc->state[31],
		IF, 25,  4, PP8_0_);
	WRITE_STATE_BIG(sc);
#endif
}

#if SPH_SIMD_NOCOPY
#undef A0
#undef A1
#undef A2
#undef A3
#undef A4
#undef A5
#undef A6
#undef A7
#undef B0
#undef B1
#undef B2
#undef B3
#undef B4
#undef B5
#undef B6
#undef B7
#undef C0
#undef C1
#undef C2
#undef C3
#undef C4
#undef C5
#undef C6
#undef C7
#undef D0
#undef D1
#undef D2
#undef D3
#undef D4
#undef D5
#undef D6
#undef D7
#endif

#endif

static const u32 IV224[] = {
	C32(0x33586E9F), C32(0x12FFF033), C32(0xB2D9F64D), C32(0x6F8FEA53),
	C32(0xDE943106), C32(0x2742E439), C32(0x4FBAB5AC), C32(0x62B9FF96),
	C32(0x22E7B0AF), C32(0xC862B3A8), C32(0x33E00CDC), C32(0x236B86A6),
	C32(0xF64AE77C), C32(0xFA373B76), C32(0x7DC1EE5B), C32(0x7FB29CE8)
};

static const u32 IV256[] = {
	C32(0x4D567983), C32(0x07190BA9), C32(0x8474577B), C32(0x39D726E9),
	C32(0xAAF3D925), C32(0x3EE20B03), C32(0xAFD5E751), C32(0xC96006D3),
	C32(0xC2C2BA14), C32(0x49B3BCB4), C32(0xF67CAF46), C32(0x668626C9),
	C32(0xE2EAA8D2), C32(0x1FF47833), C32(0xD0C661A5), C32(0x55693DE1)
};

static const u32 IV384[] = {
	C32(0x8A36EEBC), C32(0x94A3BD90), C32(0xD1537B83), C32(0xB25B070B),
	C32(0xF463F1B5), C32(0xB6F81E20), C32(0x0055C339), C32(0xB4D144D1),
	C32(0x7360CA61), C32(0x18361A03), C32(0x17DCB4B9), C32(0x3414C45A),
	C32(0xA699A9D2), C32(0xE39E9664), C32(0x468BFE77), C32(0x51D062F8),
	C32(0xB9E3BFE8), C32(0x63BECE2A), C32(0x8FE506B9), C32(0xF8CC4AC2),
	C32(0x7AE11542), C32(0xB1AADDA1), C32(0x64B06794), C32(0x28D2F462),
	C32(0xE64071EC), C32(0x1DEB91A8), C32(0x8AC8DB23), C32(0x3F782AB5),
	C32(0x039B5CB8), C32(0x71DDD962), C32(0xFADE2CEA), C32(0x1416DF71)
};

static const u32 IV512[] = {
	C32(0x0BA16B95), C32(0x72F999AD), C32(0x9FECC2AE), C32(0xBA3264FC),
	C32(0x5E894929), C32(0x8E9F30E5), C32(0x2F1DAA37), C32(0xF0F2C558),
	C32(0xAC506643), C32(0xA90635A5), C32(0xE25B878B), C32(0xAAB7878F),
	C32(0x88817F7A), C32(0x0A02892B), C32(0x559A7550), C32(0x598F657E),
	C32(0x7EEF60A1), C32(0x6B70E3E8), C32(0x9C1714D1), C32(0xB958E2A8),
	C32(0xAB02675E), C32(0xED1C014F), C32(0xCD8D65BB), C32(0xFDB7A257),
	C32(0x09254899), C32(0xD699C7BC), C32(0x9019B6DC), C32(0x2B9022E4),
	C32(0x8FA14956), C32(0x21BF9BD3), C32(0xB94D0943), C32(0x6FFDDC22)
};

static void
init_small(void *cc, const u32 *iv)
{
	sph_simd_small_context *sc;

	sc = cc;
	memcpy(sc->state, iv, sizeof sc->state);
	sc->count_low = sc->count_high = 0;
	sc->ptr = 0;
}

static void
init_big(void *cc, const u32 *iv)
{
	sph_simd_big_context *sc;

	sc = cc;
	memcpy(sc->state, iv, sizeof sc->state);
	sc->count_low = sc->count_high = 0;
	sc->ptr = 0;
}

static void
update_small(void *cc, const void *data, size_t len)
{
	sph_simd_small_context *sc;

	sc = cc;
	while (len > 0) {
		size_t clen;

		clen = (sizeof sc->buf) - sc->ptr;
		if (clen > len)
			clen = len;
		memcpy(sc->buf + sc->ptr, data, clen);
		data = (const unsigned char *)data + clen;
		len -= clen;
		if ((sc->ptr += clen) == sizeof sc->buf) {
			compress_small(sc, 0);
			sc->ptr = 0;
			sc->count_low = T32(sc->count_low + 1);
			if (sc->count_low == 0)
				sc->count_high ++;
		}
	}
}

static void
update_big(void *cc, const void *data, size_t len)
{
	sph_simd_big_context *sc;

	sc = cc;
	while (len > 0) {
		size_t clen;

		clen = (sizeof sc->buf) - sc->ptr;
		if (clen > len)
			clen = len;
		memcpy(sc->buf + sc->ptr, data, clen);
		data = (const unsigned char *)data + clen;
		len -= clen;
		if ((sc->ptr += clen) == sizeof sc->buf) {
			compress_big(sc, 0);
			sc->ptr = 0;
			sc->count_low = T32(sc->count_low + 1);
			if (sc->count_low == 0)
				sc->count_high ++;
		}
	}
}

static void
encode_count_small(unsigned char *dst,
	u32 low, u32 high, size_t ptr, unsigned n)
{
	low = T32(low << 9);
	high = T32(high << 9) + (low >> 23);
	low += (ptr << 3) + n;
	sph_enc32le(dst, low);
	sph_enc32le(dst + 4, high);
}

static void
encode_count_big(unsigned char *dst,
	u32 low, u32 high, size_t ptr, unsigned n)
{
	low = T32(low << 10);
	high = T32(high << 10) + (low >> 22);
	low += (ptr << 3) + n;
	sph_enc32le(dst, low);
	sph_enc32le(dst + 4, high);
}

static void
finalize_small(void *cc, unsigned ub, unsigned n, void *dst, size_t dst_len)
{
	sph_simd_small_context *sc;
	unsigned char *d;
	size_t u;

	sc = cc;
	if (sc->ptr > 0 || n > 0) {
		memset(sc->buf + sc->ptr, 0,
			(sizeof sc->buf) - sc->ptr);
		sc->buf[sc->ptr] = ub & (0xFF << (8 - n));
		compress_small(sc, 0);
	}
	memset(sc->buf, 0, sizeof sc->buf);
	encode_count_small(sc->buf, sc->count_low, sc->count_high, sc->ptr, n);
	compress_small(sc, 1);
	d = dst;
	for (d = dst, u = 0; u < dst_len; u ++)
		sph_enc32le(d + (u << 2), sc->state[u]);
}

static void
finalize_big(void *cc, unsigned ub, unsigned n, void *dst, size_t dst_len)
{
	sph_simd_big_context *sc;
	unsigned char *d;
	size_t u;

	sc = cc;
	if (sc->ptr > 0 || n > 0) {
		memset(sc->buf + sc->ptr, 0,
			(sizeof sc->buf) - sc->ptr);
		sc->buf[sc->ptr] = ub & (0xFF << (8 - n));
		compress_big(sc, 0);
	}
	memset(sc->buf, 0, sizeof sc->buf);
	encode_count_big(sc->buf, sc->count_low, sc->count_high, sc->ptr, n);
	compress_big(sc, 1);
	d = dst;
	for (d = dst, u = 0; u < dst_len; u ++)
		sph_enc32le(d + (u << 2), sc->state[u]);
}

void
sph_simd224_init(void *cc)
{
	init_small(cc, IV224);
}

void
sph_simd224(void *cc, const void *data, size_t len)
{
	update_small(cc, data, len);
}

void
sph_simd224_close(void *cc, void *dst)
{
	sph_simd224_addbits_and_close(cc, 0, 0, dst);
}

void
sph_simd224_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	finalize_small(cc, ub, n, dst, 7);
	sph_simd224_init(cc);
}

void
sph_simd256_init(void *cc)
{
	init_small(cc, IV256);
}

void
sph_simd256(void *cc, const void *data, size_t len)
{
	update_small(cc, data, len);
}

void
sph_simd256_close(void *cc, void *dst)
{
	sph_simd256_addbits_and_close(cc, 0, 0, dst);
}

void
sph_simd256_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	finalize_small(cc, ub, n, dst, 8);
	sph_simd256_init(cc);
}

void
sph_simd384_init(void *cc)
{
	init_big(cc, IV384);
}

void
sph_simd384(void *cc, const void *data, size_t len)
{
	update_big(cc, data, len);
}

void
sph_simd384_close(void *cc, void *dst)
{
	sph_simd384_addbits_and_close(cc, 0, 0, dst);
}

void
sph_simd384_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	finalize_big(cc, ub, n, dst, 12);
	sph_simd384_init(cc);
}

void
sph_simd512_init(void *cc)
{
	init_big(cc, IV512);
}

void
sph_simd512(void *cc, const void *data, size_t len)
{
	update_big(cc, data, len);
}

void
sph_simd512_close(void *cc, void *dst)
{
	sph_simd512_addbits_and_close(cc, 0, 0, dst);
}

void
sph_simd512_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	finalize_big(cc, ub, n, dst, 16);
	sph_simd512_init(cc);
}
#ifdef __cplusplus
}
#endif