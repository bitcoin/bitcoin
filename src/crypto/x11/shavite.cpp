/* $Id: shavite.c 227 2010-06-16 17:28:38Z tp $ */
/*
 * SHAvite-3 implementation.
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

#include <crypto/x11/dispatch.h>

#include <cstddef>
#include <cstring>

#include "sph_shavite.h"

extern sapphire::dispatch::AESRoundFnNk aes_round_nk;

#ifdef _MSC_VER
#pragma warning (disable: 4146)
#endif

#define C32   SPH_C32

/*
 * As of round 2 of the SHA-3 competition, the published reference
 * implementation and test vectors are wrong, because they use
 * big-endian AES tables while the internal decoding uses little-endian.
 * The code below follows the specification. To turn it into a code
 * which follows the reference implementation (the one called "BugFix"
 * on the SHAvite-3 web site, published on Nov 23rd, 2009), comment out
 * the code below (from the '#define AES_BIG_ENDIAN...' to the definition
 * of the AES_ROUND_NOKEY macro) and replace it with the version which
 * is commented out afterwards.
 */

static const sph_u32 IV512[] = {
	C32(0x72FCCDD8), C32(0x79CA4727), C32(0x128A077B), C32(0x40D55AEC),
	C32(0xD1901A06), C32(0x430AE307), C32(0xB29F5CD1), C32(0xDF07FBFC),
	C32(0x8E45D73D), C32(0x681AB538), C32(0xBDE86578), C32(0xDD577E47),
	C32(0xE275EADE), C32(0x502D9FCD), C32(0xB9357178), C32(0x022A4B9A)
};

#define AES_ROUND_NOKEY(x0, x1, x2, x3)   do { \
		aes_round_nk(x0, x1, x2, x3, x0, x1, x2, x3); \
	} while (0)

/*
 * This function assumes that "msg" is aligned for 32-bit access.
 */
static void
c512(sph_shavite_big_context *sc, const void *msg)
{
	sph_u32 p0, p1, p2, p3, p4, p5, p6, p7;
	sph_u32 p8, p9, pA, pB, pC, pD, pE, pF;
	sph_u32 rk[448];
	size_t u;
	int r, s;

#if SPH_LITTLE_ENDIAN
	memcpy(rk, msg, 128);
#else
	for (u = 0; u < 32; u += 4) {
		rk[u + 0] = sph_dec32le_aligned(
			(const unsigned char *)msg + (u << 2) +  0);
		rk[u + 1] = sph_dec32le_aligned(
			(const unsigned char *)msg + (u << 2) +  4);
		rk[u + 2] = sph_dec32le_aligned(
			(const unsigned char *)msg + (u << 2) +  8);
		rk[u + 3] = sph_dec32le_aligned(
			(const unsigned char *)msg + (u << 2) + 12);
	}
#endif
	u = 32;
	for (;;) {
		for (s = 0; s < 4; s ++) {
			sph_u32 x0, x1, x2, x3;

			x0 = rk[u - 31];
			x1 = rk[u - 30];
			x2 = rk[u - 29];
			x3 = rk[u - 32];
			AES_ROUND_NOKEY(x0, x1, x2, x3);
			rk[u + 0] = x0 ^ rk[u - 4];
			rk[u + 1] = x1 ^ rk[u - 3];
			rk[u + 2] = x2 ^ rk[u - 2];
			rk[u + 3] = x3 ^ rk[u - 1];
			if (u == 32) {
				rk[ 32] ^= sc->count0;
				rk[ 33] ^= sc->count1;
				rk[ 34] ^= sc->count2;
				rk[ 35] ^= SPH_T32(~sc->count3);
			} else if (u == 440) {
				rk[440] ^= sc->count1;
				rk[441] ^= sc->count0;
				rk[442] ^= sc->count3;
				rk[443] ^= SPH_T32(~sc->count2);
			}
			u += 4;

			x0 = rk[u - 31];
			x1 = rk[u - 30];
			x2 = rk[u - 29];
			x3 = rk[u - 32];
			AES_ROUND_NOKEY(x0, x1, x2, x3);
			rk[u + 0] = x0 ^ rk[u - 4];
			rk[u + 1] = x1 ^ rk[u - 3];
			rk[u + 2] = x2 ^ rk[u - 2];
			rk[u + 3] = x3 ^ rk[u - 1];
			if (u == 164) {
				rk[164] ^= sc->count3;
				rk[165] ^= sc->count2;
				rk[166] ^= sc->count1;
				rk[167] ^= SPH_T32(~sc->count0);
			} else if (u == 316) {
				rk[316] ^= sc->count2;
				rk[317] ^= sc->count3;
				rk[318] ^= sc->count0;
				rk[319] ^= SPH_T32(~sc->count1);
			}
			u += 4;
		}
		if (u == 448)
			break;
		for (s = 0; s < 8; s ++) {
			rk[u + 0] = rk[u - 32] ^ rk[u - 7];
			rk[u + 1] = rk[u - 31] ^ rk[u - 6];
			rk[u + 2] = rk[u - 30] ^ rk[u - 5];
			rk[u + 3] = rk[u - 29] ^ rk[u - 4];
			u += 4;
		}
	}

	p0 = sc->h[0x0];
	p1 = sc->h[0x1];
	p2 = sc->h[0x2];
	p3 = sc->h[0x3];
	p4 = sc->h[0x4];
	p5 = sc->h[0x5];
	p6 = sc->h[0x6];
	p7 = sc->h[0x7];
	p8 = sc->h[0x8];
	p9 = sc->h[0x9];
	pA = sc->h[0xA];
	pB = sc->h[0xB];
	pC = sc->h[0xC];
	pD = sc->h[0xD];
	pE = sc->h[0xE];
	pF = sc->h[0xF];
	u = 0;
	for (r = 0; r < 14; r ++) {
#define C512_ELT(l0, l1, l2, l3, r0, r1, r2, r3)   do { \
		sph_u32 x0, x1, x2, x3; \
		x0 = r0 ^ rk[u ++]; \
		x1 = r1 ^ rk[u ++]; \
		x2 = r2 ^ rk[u ++]; \
		x3 = r3 ^ rk[u ++]; \
		AES_ROUND_NOKEY(x0, x1, x2, x3); \
		x0 ^= rk[u ++]; \
		x1 ^= rk[u ++]; \
		x2 ^= rk[u ++]; \
		x3 ^= rk[u ++]; \
		AES_ROUND_NOKEY(x0, x1, x2, x3); \
		x0 ^= rk[u ++]; \
		x1 ^= rk[u ++]; \
		x2 ^= rk[u ++]; \
		x3 ^= rk[u ++]; \
		AES_ROUND_NOKEY(x0, x1, x2, x3); \
		x0 ^= rk[u ++]; \
		x1 ^= rk[u ++]; \
		x2 ^= rk[u ++]; \
		x3 ^= rk[u ++]; \
		AES_ROUND_NOKEY(x0, x1, x2, x3); \
		l0 ^= x0; \
		l1 ^= x1; \
		l2 ^= x2; \
		l3 ^= x3; \
	} while (0)

#define WROT(a, b, c, d)   do { \
		sph_u32 t = d; \
		d = c; \
		c = b; \
		b = a; \
		a = t; \
	} while (0)

		C512_ELT(p0, p1, p2, p3, p4, p5, p6, p7);
		C512_ELT(p8, p9, pA, pB, pC, pD, pE, pF);

		WROT(p0, p4, p8, pC);
		WROT(p1, p5, p9, pD);
		WROT(p2, p6, pA, pE);
		WROT(p3, p7, pB, pF);

#undef C512_ELT
#undef WROT
	}
	sc->h[0x0] ^= p0;
	sc->h[0x1] ^= p1;
	sc->h[0x2] ^= p2;
	sc->h[0x3] ^= p3;
	sc->h[0x4] ^= p4;
	sc->h[0x5] ^= p5;
	sc->h[0x6] ^= p6;
	sc->h[0x7] ^= p7;
	sc->h[0x8] ^= p8;
	sc->h[0x9] ^= p9;
	sc->h[0xA] ^= pA;
	sc->h[0xB] ^= pB;
	sc->h[0xC] ^= pC;
	sc->h[0xD] ^= pD;
	sc->h[0xE] ^= pE;
	sc->h[0xF] ^= pF;
}

static void
shavite_big_init(sph_shavite_big_context *sc, const sph_u32 *iv)
{
	memcpy(sc->h, iv, sizeof sc->h);
	sc->ptr = 0;
	sc->count0 = 0;
	sc->count1 = 0;
	sc->count2 = 0;
	sc->count3 = 0;
}

static void
shavite_big_core(sph_shavite_big_context *sc, const void *data, size_t len)
{
	unsigned char *buf;
	size_t ptr;

	buf = sc->buf;
	ptr = sc->ptr;
	while (len > 0) {
		size_t clen;

		clen = (sizeof sc->buf) - ptr;
		if (clen > len)
			clen = len;
		memcpy(buf + ptr, data, clen);
		data = (const unsigned char *)data + clen;
		ptr += clen;
		len -= clen;
		if (ptr == sizeof sc->buf) {
			if ((sc->count0 = SPH_T32(sc->count0 + 1024)) == 0) {
				sc->count1 = SPH_T32(sc->count1 + 1);
				if (sc->count1 == 0) {
					sc->count2 = SPH_T32(sc->count2 + 1);
					if (sc->count2 == 0) {
						sc->count3 = SPH_T32(
							sc->count3 + 1);
					}
				}
			}
			c512(sc, buf);
			ptr = 0;
		}
	}
	sc->ptr = ptr;
}

static void
shavite_big_close(sph_shavite_big_context *sc,
	unsigned ub, unsigned n, void *dst, size_t out_size_w32)
{
	unsigned char *buf;
	size_t ptr, u;
	unsigned z;
	sph_u32 count0, count1, count2, count3;

	buf = sc->buf;
	ptr = sc->ptr;
	count0 = (sc->count0 += (ptr << 3) + n);
	count1 = sc->count1;
	count2 = sc->count2;
	count3 = sc->count3;
	z = 0x80 >> n;
	z = ((ub & -z) | z) & 0xFF;
	if (ptr == 0 && n == 0) {
		buf[0] = 0x80;
		memset(buf + 1, 0, 109);
		sc->count0 = sc->count1 = sc->count2 = sc->count3 = 0;
	} else if (ptr < 110) {
		buf[ptr ++] = z;
		memset(buf + ptr, 0, 110 - ptr);
	} else {
		buf[ptr ++] = z;
		memset(buf + ptr, 0, 128 - ptr);
		c512(sc, buf);
		memset(buf, 0, 110);
		sc->count0 = sc->count1 = sc->count2 = sc->count3 = 0;
	}
	sph_enc32le(buf + 110, count0);
	sph_enc32le(buf + 114, count1);
	sph_enc32le(buf + 118, count2);
	sph_enc32le(buf + 122, count3);
	buf[126] = out_size_w32 << 5;
	buf[127] = out_size_w32 >> 3;
	c512(sc, buf);
	for (u = 0; u < out_size_w32; u ++)
		sph_enc32le((unsigned char *)dst + (u << 2), sc->h[u]);
}

/* see sph_shavite.h */
void
sph_shavite512_init(sph_shavite512_context *cc)
{
	shavite_big_init(cc, IV512);
}

/* see sph_shavite.h */
void
sph_shavite512(sph_shavite512_context *cc, const void *data, size_t len)
{
	shavite_big_core(cc, data, len);
}

/* see sph_shavite.h */
void
sph_shavite512_close(sph_shavite512_context *cc, void *dst)
{
	shavite_big_close(cc, 0, 0, dst, 16);
	shavite_big_init(cc, IV512);
}

/* see sph_shavite.h */
void
sph_shavite512_addbits_and_close(sph_shavite512_context *cc, unsigned ub, unsigned n, void *dst)
{
	shavite_big_close(cc, ub, n, dst, 16);
	shavite_big_init(cc, IV512);
}
