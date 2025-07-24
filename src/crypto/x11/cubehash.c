/* $Id: cubehash.c 227 2010-06-16 17:28:38Z tp $ */
/*
 * CubeHash implementation.
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

#include "sph_cubehash.h"
#ifdef __cplusplus
extern "C"{
#endif

#if SPH_SMALL_FOOTPRINT && !defined SPH_SMALL_FOOTPRINT_CUBEHASH
#define SPH_SMALL_FOOTPRINT_CUBEHASH   1
#endif

/*
 * Some tests were conducted on an Intel Core2 Q6600 (32-bit and 64-bit
 * mode), a PowerPC G3, and a MIPS-compatible CPU (Broadcom BCM3302).
 * It appears that the optimal settings are:
 *  -- full unroll, no state copy on the "big" systems (x86, PowerPC)
 *  -- unroll to 4 or 8, state copy on the "small" system (MIPS)
 */

#if SPH_SMALL_FOOTPRINT_CUBEHASH

#if !defined SPH_CUBEHASH_UNROLL
#define SPH_CUBEHASH_UNROLL   4
#endif
#if !defined SPH_CUBEHASH_NOCOPY
#define SPH_CUBEHASH_NOCOPY   1
#endif

#else

#if !defined SPH_CUBEHASH_UNROLL
#define SPH_CUBEHASH_UNROLL   0
#endif
#if !defined SPH_CUBEHASH_NOCOPY
#define SPH_CUBEHASH_NOCOPY   0
#endif

#endif

#ifdef _MSC_VER
#pragma warning (disable: 4146)
#endif

static const sph_u32 IV512[] = {
	SPH_C32(0x2AEA2A61), SPH_C32(0x50F494D4), SPH_C32(0x2D538B8B),
	SPH_C32(0x4167D83E), SPH_C32(0x3FEE2313), SPH_C32(0xC701CF8C),
	SPH_C32(0xCC39968E), SPH_C32(0x50AC5695), SPH_C32(0x4D42C787),
	SPH_C32(0xA647A8B3), SPH_C32(0x97CF0BEF), SPH_C32(0x825B4537),
	SPH_C32(0xEEF864D2), SPH_C32(0xF22090C4), SPH_C32(0xD0E5CD33),
	SPH_C32(0xA23911AE), SPH_C32(0xFCD398D9), SPH_C32(0x148FE485),
	SPH_C32(0x1B017BEF), SPH_C32(0xB6444532), SPH_C32(0x6A536159),
	SPH_C32(0x2FF5781C), SPH_C32(0x91FA7934), SPH_C32(0x0DBADEA9),
	SPH_C32(0xD65C8A2B), SPH_C32(0xA5A70E75), SPH_C32(0xB1C62456),
	SPH_C32(0xBC796576), SPH_C32(0x1921C8F7), SPH_C32(0xE7989AF1),
	SPH_C32(0x7795D246), SPH_C32(0xD43E3B44)
};

#define T32      SPH_T32
#define ROTL32   SPH_ROTL32

#if SPH_CUBEHASH_NOCOPY

#define DECL_STATE
#define READ_STATE(cc)
#define WRITE_STATE(cc)

#define x0   ((sc)->state[ 0])
#define x1   ((sc)->state[ 1])
#define x2   ((sc)->state[ 2])
#define x3   ((sc)->state[ 3])
#define x4   ((sc)->state[ 4])
#define x5   ((sc)->state[ 5])
#define x6   ((sc)->state[ 6])
#define x7   ((sc)->state[ 7])
#define x8   ((sc)->state[ 8])
#define x9   ((sc)->state[ 9])
#define xa   ((sc)->state[10])
#define xb   ((sc)->state[11])
#define xc   ((sc)->state[12])
#define xd   ((sc)->state[13])
#define xe   ((sc)->state[14])
#define xf   ((sc)->state[15])
#define xg   ((sc)->state[16])
#define xh   ((sc)->state[17])
#define xi   ((sc)->state[18])
#define xj   ((sc)->state[19])
#define xk   ((sc)->state[20])
#define xl   ((sc)->state[21])
#define xm   ((sc)->state[22])
#define xn   ((sc)->state[23])
#define xo   ((sc)->state[24])
#define xp   ((sc)->state[25])
#define xq   ((sc)->state[26])
#define xr   ((sc)->state[27])
#define xs   ((sc)->state[28])
#define xt   ((sc)->state[29])
#define xu   ((sc)->state[30])
#define xv   ((sc)->state[31])

#else

#define DECL_STATE \
	sph_u32 x0, x1, x2, x3, x4, x5, x6, x7; \
	sph_u32 x8, x9, xa, xb, xc, xd, xe, xf; \
	sph_u32 xg, xh, xi, xj, xk, xl, xm, xn; \
	sph_u32 xo, xp, xq, xr, xs, xt, xu, xv;

#define READ_STATE(cc)   do { \
		x0 = (cc)->state[ 0]; \
		x1 = (cc)->state[ 1]; \
		x2 = (cc)->state[ 2]; \
		x3 = (cc)->state[ 3]; \
		x4 = (cc)->state[ 4]; \
		x5 = (cc)->state[ 5]; \
		x6 = (cc)->state[ 6]; \
		x7 = (cc)->state[ 7]; \
		x8 = (cc)->state[ 8]; \
		x9 = (cc)->state[ 9]; \
		xa = (cc)->state[10]; \
		xb = (cc)->state[11]; \
		xc = (cc)->state[12]; \
		xd = (cc)->state[13]; \
		xe = (cc)->state[14]; \
		xf = (cc)->state[15]; \
		xg = (cc)->state[16]; \
		xh = (cc)->state[17]; \
		xi = (cc)->state[18]; \
		xj = (cc)->state[19]; \
		xk = (cc)->state[20]; \
		xl = (cc)->state[21]; \
		xm = (cc)->state[22]; \
		xn = (cc)->state[23]; \
		xo = (cc)->state[24]; \
		xp = (cc)->state[25]; \
		xq = (cc)->state[26]; \
		xr = (cc)->state[27]; \
		xs = (cc)->state[28]; \
		xt = (cc)->state[29]; \
		xu = (cc)->state[30]; \
		xv = (cc)->state[31]; \
	} while (0)

#define WRITE_STATE(cc)   do { \
		(cc)->state[ 0] = x0; \
		(cc)->state[ 1] = x1; \
		(cc)->state[ 2] = x2; \
		(cc)->state[ 3] = x3; \
		(cc)->state[ 4] = x4; \
		(cc)->state[ 5] = x5; \
		(cc)->state[ 6] = x6; \
		(cc)->state[ 7] = x7; \
		(cc)->state[ 8] = x8; \
		(cc)->state[ 9] = x9; \
		(cc)->state[10] = xa; \
		(cc)->state[11] = xb; \
		(cc)->state[12] = xc; \
		(cc)->state[13] = xd; \
		(cc)->state[14] = xe; \
		(cc)->state[15] = xf; \
		(cc)->state[16] = xg; \
		(cc)->state[17] = xh; \
		(cc)->state[18] = xi; \
		(cc)->state[19] = xj; \
		(cc)->state[20] = xk; \
		(cc)->state[21] = xl; \
		(cc)->state[22] = xm; \
		(cc)->state[23] = xn; \
		(cc)->state[24] = xo; \
		(cc)->state[25] = xp; \
		(cc)->state[26] = xq; \
		(cc)->state[27] = xr; \
		(cc)->state[28] = xs; \
		(cc)->state[29] = xt; \
		(cc)->state[30] = xu; \
		(cc)->state[31] = xv; \
	} while (0)

#endif

#define INPUT_BLOCK   do { \
		x0 ^= sph_dec32le_aligned(buf +  0); \
		x1 ^= sph_dec32le_aligned(buf +  4); \
		x2 ^= sph_dec32le_aligned(buf +  8); \
		x3 ^= sph_dec32le_aligned(buf + 12); \
		x4 ^= sph_dec32le_aligned(buf + 16); \
		x5 ^= sph_dec32le_aligned(buf + 20); \
		x6 ^= sph_dec32le_aligned(buf + 24); \
		x7 ^= sph_dec32le_aligned(buf + 28); \
	} while (0)

#define ROUND_EVEN   do { \
		xg = T32(x0 + xg); \
		x0 = ROTL32(x0, 7); \
		xh = T32(x1 + xh); \
		x1 = ROTL32(x1, 7); \
		xi = T32(x2 + xi); \
		x2 = ROTL32(x2, 7); \
		xj = T32(x3 + xj); \
		x3 = ROTL32(x3, 7); \
		xk = T32(x4 + xk); \
		x4 = ROTL32(x4, 7); \
		xl = T32(x5 + xl); \
		x5 = ROTL32(x5, 7); \
		xm = T32(x6 + xm); \
		x6 = ROTL32(x6, 7); \
		xn = T32(x7 + xn); \
		x7 = ROTL32(x7, 7); \
		xo = T32(x8 + xo); \
		x8 = ROTL32(x8, 7); \
		xp = T32(x9 + xp); \
		x9 = ROTL32(x9, 7); \
		xq = T32(xa + xq); \
		xa = ROTL32(xa, 7); \
		xr = T32(xb + xr); \
		xb = ROTL32(xb, 7); \
		xs = T32(xc + xs); \
		xc = ROTL32(xc, 7); \
		xt = T32(xd + xt); \
		xd = ROTL32(xd, 7); \
		xu = T32(xe + xu); \
		xe = ROTL32(xe, 7); \
		xv = T32(xf + xv); \
		xf = ROTL32(xf, 7); \
		x8 ^= xg; \
		x9 ^= xh; \
		xa ^= xi; \
		xb ^= xj; \
		xc ^= xk; \
		xd ^= xl; \
		xe ^= xm; \
		xf ^= xn; \
		x0 ^= xo; \
		x1 ^= xp; \
		x2 ^= xq; \
		x3 ^= xr; \
		x4 ^= xs; \
		x5 ^= xt; \
		x6 ^= xu; \
		x7 ^= xv; \
		xi = T32(x8 + xi); \
		x8 = ROTL32(x8, 11); \
		xj = T32(x9 + xj); \
		x9 = ROTL32(x9, 11); \
		xg = T32(xa + xg); \
		xa = ROTL32(xa, 11); \
		xh = T32(xb + xh); \
		xb = ROTL32(xb, 11); \
		xm = T32(xc + xm); \
		xc = ROTL32(xc, 11); \
		xn = T32(xd + xn); \
		xd = ROTL32(xd, 11); \
		xk = T32(xe + xk); \
		xe = ROTL32(xe, 11); \
		xl = T32(xf + xl); \
		xf = ROTL32(xf, 11); \
		xq = T32(x0 + xq); \
		x0 = ROTL32(x0, 11); \
		xr = T32(x1 + xr); \
		x1 = ROTL32(x1, 11); \
		xo = T32(x2 + xo); \
		x2 = ROTL32(x2, 11); \
		xp = T32(x3 + xp); \
		x3 = ROTL32(x3, 11); \
		xu = T32(x4 + xu); \
		x4 = ROTL32(x4, 11); \
		xv = T32(x5 + xv); \
		x5 = ROTL32(x5, 11); \
		xs = T32(x6 + xs); \
		x6 = ROTL32(x6, 11); \
		xt = T32(x7 + xt); \
		x7 = ROTL32(x7, 11); \
		xc ^= xi; \
		xd ^= xj; \
		xe ^= xg; \
		xf ^= xh; \
		x8 ^= xm; \
		x9 ^= xn; \
		xa ^= xk; \
		xb ^= xl; \
		x4 ^= xq; \
		x5 ^= xr; \
		x6 ^= xo; \
		x7 ^= xp; \
		x0 ^= xu; \
		x1 ^= xv; \
		x2 ^= xs; \
		x3 ^= xt; \
	} while (0)

#define ROUND_ODD   do { \
		xj = T32(xc + xj); \
		xc = ROTL32(xc, 7); \
		xi = T32(xd + xi); \
		xd = ROTL32(xd, 7); \
		xh = T32(xe + xh); \
		xe = ROTL32(xe, 7); \
		xg = T32(xf + xg); \
		xf = ROTL32(xf, 7); \
		xn = T32(x8 + xn); \
		x8 = ROTL32(x8, 7); \
		xm = T32(x9 + xm); \
		x9 = ROTL32(x9, 7); \
		xl = T32(xa + xl); \
		xa = ROTL32(xa, 7); \
		xk = T32(xb + xk); \
		xb = ROTL32(xb, 7); \
		xr = T32(x4 + xr); \
		x4 = ROTL32(x4, 7); \
		xq = T32(x5 + xq); \
		x5 = ROTL32(x5, 7); \
		xp = T32(x6 + xp); \
		x6 = ROTL32(x6, 7); \
		xo = T32(x7 + xo); \
		x7 = ROTL32(x7, 7); \
		xv = T32(x0 + xv); \
		x0 = ROTL32(x0, 7); \
		xu = T32(x1 + xu); \
		x1 = ROTL32(x1, 7); \
		xt = T32(x2 + xt); \
		x2 = ROTL32(x2, 7); \
		xs = T32(x3 + xs); \
		x3 = ROTL32(x3, 7); \
		x4 ^= xj; \
		x5 ^= xi; \
		x6 ^= xh; \
		x7 ^= xg; \
		x0 ^= xn; \
		x1 ^= xm; \
		x2 ^= xl; \
		x3 ^= xk; \
		xc ^= xr; \
		xd ^= xq; \
		xe ^= xp; \
		xf ^= xo; \
		x8 ^= xv; \
		x9 ^= xu; \
		xa ^= xt; \
		xb ^= xs; \
		xh = T32(x4 + xh); \
		x4 = ROTL32(x4, 11); \
		xg = T32(x5 + xg); \
		x5 = ROTL32(x5, 11); \
		xj = T32(x6 + xj); \
		x6 = ROTL32(x6, 11); \
		xi = T32(x7 + xi); \
		x7 = ROTL32(x7, 11); \
		xl = T32(x0 + xl); \
		x0 = ROTL32(x0, 11); \
		xk = T32(x1 + xk); \
		x1 = ROTL32(x1, 11); \
		xn = T32(x2 + xn); \
		x2 = ROTL32(x2, 11); \
		xm = T32(x3 + xm); \
		x3 = ROTL32(x3, 11); \
		xp = T32(xc + xp); \
		xc = ROTL32(xc, 11); \
		xo = T32(xd + xo); \
		xd = ROTL32(xd, 11); \
		xr = T32(xe + xr); \
		xe = ROTL32(xe, 11); \
		xq = T32(xf + xq); \
		xf = ROTL32(xf, 11); \
		xt = T32(x8 + xt); \
		x8 = ROTL32(x8, 11); \
		xs = T32(x9 + xs); \
		x9 = ROTL32(x9, 11); \
		xv = T32(xa + xv); \
		xa = ROTL32(xa, 11); \
		xu = T32(xb + xu); \
		xb = ROTL32(xb, 11); \
		x0 ^= xh; \
		x1 ^= xg; \
		x2 ^= xj; \
		x3 ^= xi; \
		x4 ^= xl; \
		x5 ^= xk; \
		x6 ^= xn; \
		x7 ^= xm; \
		x8 ^= xp; \
		x9 ^= xo; \
		xa ^= xr; \
		xb ^= xq; \
		xc ^= xt; \
		xd ^= xs; \
		xe ^= xv; \
		xf ^= xu; \
	} while (0)

/*
 * There is no need to unroll all 16 rounds. The word-swapping permutation
 * is an involution, so we need to unroll an even number of rounds. On
 * "big" systems, unrolling 4 rounds yields about 97% of the speed
 * achieved with full unrolling; and it keeps the code more compact
 * for small architectures.
 */

#if SPH_CUBEHASH_UNROLL == 2

#define SIXTEEN_ROUNDS   do { \
		int j; \
		for (j = 0; j < 8; j ++) { \
			ROUND_EVEN; \
			ROUND_ODD; \
		} \
	} while (0)

#elif SPH_CUBEHASH_UNROLL == 4

#define SIXTEEN_ROUNDS   do { \
		int j; \
		for (j = 0; j < 4; j ++) { \
			ROUND_EVEN; \
			ROUND_ODD; \
			ROUND_EVEN; \
			ROUND_ODD; \
		} \
	} while (0)

#elif SPH_CUBEHASH_UNROLL == 8

#define SIXTEEN_ROUNDS   do { \
		int j; \
		for (j = 0; j < 2; j ++) { \
			ROUND_EVEN; \
			ROUND_ODD; \
			ROUND_EVEN; \
			ROUND_ODD; \
			ROUND_EVEN; \
			ROUND_ODD; \
			ROUND_EVEN; \
			ROUND_ODD; \
		} \
	} while (0)

#else

#define SIXTEEN_ROUNDS   do { \
		ROUND_EVEN; \
		ROUND_ODD; \
		ROUND_EVEN; \
		ROUND_ODD; \
		ROUND_EVEN; \
		ROUND_ODD; \
		ROUND_EVEN; \
		ROUND_ODD; \
		ROUND_EVEN; \
		ROUND_ODD; \
		ROUND_EVEN; \
		ROUND_ODD; \
		ROUND_EVEN; \
		ROUND_ODD; \
		ROUND_EVEN; \
		ROUND_ODD; \
	} while (0)

#endif

static void
cubehash_init(sph_cubehash_context *sc, const sph_u32 *iv)
{
	memcpy(sc->state, iv, sizeof sc->state);
	sc->ptr = 0;
}

static void
cubehash_core(sph_cubehash_context *sc, const void *data, size_t len)
{
	unsigned char *buf;
	size_t ptr;
	DECL_STATE

	buf = sc->buf;
	ptr = sc->ptr;
	if (len < (sizeof sc->buf) - ptr) {
		memcpy(buf + ptr, data, len);
		ptr += len;
		sc->ptr = ptr;
		return;
	}

	READ_STATE(sc);
	while (len > 0) {
		size_t clen;

		clen = (sizeof sc->buf) - ptr;
		if (clen > len)
			clen = len;
		memcpy(buf + ptr, data, clen);
		ptr += clen;
		data = (const unsigned char *)data + clen;
		len -= clen;
		if (ptr == sizeof sc->buf) {
			INPUT_BLOCK;
			SIXTEEN_ROUNDS;
			ptr = 0;
		}
	}
	WRITE_STATE(sc);
	sc->ptr = ptr;
}

static void
cubehash_close(sph_cubehash_context *sc, unsigned ub, unsigned n,
	void *dst, size_t out_size_w32)
{
	unsigned char *buf, *out;
	size_t ptr;
	unsigned z;
	int i;
	DECL_STATE

	buf = sc->buf;
	ptr = sc->ptr;
	z = 0x80 >> n;
	buf[ptr ++] = ((ub & -z) | z) & 0xFF;
	memset(buf + ptr, 0, (sizeof sc->buf) - ptr);
	READ_STATE(sc);
	INPUT_BLOCK;
	for (i = 0; i < 11; i ++) {
		SIXTEEN_ROUNDS;
		if (i == 0)
			xv ^= SPH_C32(1);
	}
	WRITE_STATE(sc);
	out = dst;
	for (z = 0; z < out_size_w32; z ++)
		sph_enc32le(out + (z << 2), sc->state[z]);
}

/* see sph_cubehash.h */
void
sph_cubehash512_init(void *cc)
{
	cubehash_init(cc, IV512);
}

/* see sph_cubehash.h */
void
sph_cubehash512(void *cc, const void *data, size_t len)
{
	cubehash_core(cc, data, len);
}

/* see sph_cubehash.h */
void
sph_cubehash512_close(void *cc, void *dst)
{
	sph_cubehash512_addbits_and_close(cc, 0, 0, dst);
}

/* see sph_cubehash.h */
void
sph_cubehash512_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	cubehash_close(cc, ub, n, dst, 16);
	sph_cubehash512_init(cc);
}
#ifdef __cplusplus
}
#endif
