/* $Id: echo.c 227 2010-06-16 17:28:38Z tp $ */
/*
 * ECHO implementation.
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

#include "sph_echo.h"

extern sapphire::dispatch::AESRoundFn aes_round;
extern sapphire::dispatch::AESRoundFnNk aes_round_nk;

/*
 * We can use a 64-bit implementation only if a 64-bit type is available.
 */
#ifdef _MSC_VER
#pragma warning (disable: 4146)
#endif

#define T32   SPH_T32
#define C64   SPH_C64

#define DECL_STATE_BIG   \
	sph_u64 W[16][2];

#define INPUT_BLOCK_BIG(sc)   do { \
		unsigned u; \
		memcpy(W, sc->u.Vb, 16 * sizeof(sph_u64)); \
		for (u = 0; u < 8; u ++) { \
			W[u + 8][0] = sph_dec64le_aligned( \
				sc->buf + 16 * u); \
			W[u + 8][1] = sph_dec64le_aligned( \
				sc->buf + 16 * u + 8); \
		} \
	} while (0)

static void
aes_2rounds_all(sph_u64 W[16][2],
	sph_u32 *pK0, sph_u32 *pK1, sph_u32 *pK2, sph_u32 *pK3)
{
	int n;
	sph_u32 K0 = *pK0;
	sph_u32 K1 = *pK1;
	sph_u32 K2 = *pK2;
	sph_u32 K3 = *pK3;

	for (n = 0; n < 16; n ++) {
		sph_u64 Wl = W[n][0];
		sph_u64 Wh = W[n][1];
		sph_u32 X0 = (sph_u32)Wl;
		sph_u32 X1 = (sph_u32)(Wl >> 32);
		sph_u32 X2 = (sph_u32)Wh;
		sph_u32 X3 = (sph_u32)(Wh >> 32);
		sph_u32 Y0, Y1, Y2, Y3; \
		aes_round(X0, X1, X2, X3, K0, K1, K2, K3, Y0, Y1, Y2, Y3);
		aes_round_nk(Y0, Y1, Y2, Y3, X0, X1, X2, X3);
		W[n][0] = (sph_u64)X0 | ((sph_u64)X1 << 32);
		W[n][1] = (sph_u64)X2 | ((sph_u64)X3 << 32);
		if ((K0 = T32(K0 + 1)) == 0) {
			if ((K1 = T32(K1 + 1)) == 0)
				if ((K2 = T32(K2 + 1)) == 0)
					K3 = T32(K3 + 1);
		}
	}
	*pK0 = K0;
	*pK1 = K1;
	*pK2 = K2;
	*pK3 = K3;
}

#define BIG_SUB_WORDS   do { \
		aes_2rounds_all(W, &K0, &K1, &K2, &K3); \
	} while (0)

#define SHIFT_ROW1(a, b, c, d)   do { \
		sph_u64 tmp; \
		tmp = W[a][0]; \
		W[a][0] = W[b][0]; \
		W[b][0] = W[c][0]; \
		W[c][0] = W[d][0]; \
		W[d][0] = tmp; \
		tmp = W[a][1]; \
		W[a][1] = W[b][1]; \
		W[b][1] = W[c][1]; \
		W[c][1] = W[d][1]; \
		W[d][1] = tmp; \
	} while (0)

#define SHIFT_ROW2(a, b, c, d)   do { \
		sph_u64 tmp; \
		tmp = W[a][0]; \
		W[a][0] = W[c][0]; \
		W[c][0] = tmp; \
		tmp = W[b][0]; \
		W[b][0] = W[d][0]; \
		W[d][0] = tmp; \
		tmp = W[a][1]; \
		W[a][1] = W[c][1]; \
		W[c][1] = tmp; \
		tmp = W[b][1]; \
		W[b][1] = W[d][1]; \
		W[d][1] = tmp; \
	} while (0)

#define SHIFT_ROW3(a, b, c, d)   SHIFT_ROW1(d, c, b, a)

#define BIG_SHIFT_ROWS   do { \
		SHIFT_ROW1(1, 5, 9, 13); \
		SHIFT_ROW2(2, 6, 10, 14); \
		SHIFT_ROW3(3, 7, 11, 15); \
	} while (0)

static void
mix_column(sph_u64 W[16][2], int ia, int ib, int ic, int id)
{
	int n;

	for (n = 0; n < 2; n ++) {
		sph_u64 a = W[ia][n];
		sph_u64 b = W[ib][n];
		sph_u64 c = W[ic][n];
		sph_u64 d = W[id][n];
		sph_u64 ab = a ^ b;
		sph_u64 bc = b ^ c;
		sph_u64 cd = c ^ d;
		sph_u64 abx = ((ab & C64(0x8080808080808080)) >> 7) * 27U
			^ ((ab & C64(0x7F7F7F7F7F7F7F7F)) << 1);
		sph_u64 bcx = ((bc & C64(0x8080808080808080)) >> 7) * 27U
			^ ((bc & C64(0x7F7F7F7F7F7F7F7F)) << 1);
		sph_u64 cdx = ((cd & C64(0x8080808080808080)) >> 7) * 27U
			^ ((cd & C64(0x7F7F7F7F7F7F7F7F)) << 1);
		W[ia][n] = abx ^ bc ^ d;
		W[ib][n] = bcx ^ a ^ cd;
		W[ic][n] = cdx ^ ab ^ d;
		W[id][n] = abx ^ bcx ^ cdx ^ ab ^ c;
	}
}

#define MIX_COLUMN(a, b, c, d)   mix_column(W, a, b, c, d)

#define BIG_MIX_COLUMNS   do { \
		MIX_COLUMN(0, 1, 2, 3); \
		MIX_COLUMN(4, 5, 6, 7); \
		MIX_COLUMN(8, 9, 10, 11); \
		MIX_COLUMN(12, 13, 14, 15); \
	} while (0)

#define BIG_ROUND   do { \
		BIG_SUB_WORDS; \
		BIG_SHIFT_ROWS; \
		BIG_MIX_COLUMNS; \
	} while (0)

#define FINAL_BIG   do { \
		unsigned u; \
		sph_u64 *VV = &sc->u.Vb[0][0]; \
		sph_u64 *WW = &W[0][0]; \
		for (u = 0; u < 16; u ++) { \
			VV[u] ^= sph_dec64le_aligned(sc->buf + (u * 8)) \
				^ WW[u] ^ WW[u + 16]; \
		} \
	} while (0)

#define COMPRESS_BIG(sc)   do { \
		sph_u32 K0 = sc->C0; \
		sph_u32 K1 = sc->C1; \
		sph_u32 K2 = sc->C2; \
		sph_u32 K3 = sc->C3; \
		unsigned u; \
		INPUT_BLOCK_BIG(sc); \
		for (u = 0; u < 10; u ++) { \
			BIG_ROUND; \
		} \
		FINAL_BIG; \
	} while (0)

#define INCR_COUNTER(sc, val)   do { \
		sc->C0 = T32(sc->C0 + (sph_u32)(val)); \
		if (sc->C0 < (sph_u32)(val)) { \
			if ((sc->C1 = T32(sc->C1 + 1)) == 0) \
				if ((sc->C2 = T32(sc->C2 + 1)) == 0) \
					sc->C3 = T32(sc->C3 + 1); \
		} \
	} while (0)

static void
echo_big_init(sph_echo_big_context *sc, unsigned out_len)
{
	sc->u.Vb[0][0] = (sph_u64)out_len;
	sc->u.Vb[0][1] = 0;
	sc->u.Vb[1][0] = (sph_u64)out_len;
	sc->u.Vb[1][1] = 0;
	sc->u.Vb[2][0] = (sph_u64)out_len;
	sc->u.Vb[2][1] = 0;
	sc->u.Vb[3][0] = (sph_u64)out_len;
	sc->u.Vb[3][1] = 0;
	sc->u.Vb[4][0] = (sph_u64)out_len;
	sc->u.Vb[4][1] = 0;
	sc->u.Vb[5][0] = (sph_u64)out_len;
	sc->u.Vb[5][1] = 0;
	sc->u.Vb[6][0] = (sph_u64)out_len;
	sc->u.Vb[6][1] = 0;
	sc->u.Vb[7][0] = (sph_u64)out_len;
	sc->u.Vb[7][1] = 0;
	sc->ptr = 0;
	sc->C0 = sc->C1 = sc->C2 = sc->C3 = 0;
}

static void
echo_big_compress(sph_echo_big_context *sc)
{
	DECL_STATE_BIG

	COMPRESS_BIG(sc);
}

static void
echo_big_core(sph_echo_big_context *sc,
	const unsigned char *data, size_t len)
{
	unsigned char *buf;
	size_t ptr;

	buf = sc->buf;
	ptr = sc->ptr;
	if (len < (sizeof sc->buf) - ptr) {
		memcpy(buf + ptr, data, len);
		ptr += len;
		sc->ptr = ptr;
		return;
	}

	while (len > 0) {
		size_t clen;

		clen = (sizeof sc->buf) - ptr;
		if (clen > len)
			clen = len;
		memcpy(buf + ptr, data, clen);
		ptr += clen;
		data += clen;
		len -= clen;
		if (ptr == sizeof sc->buf) {
			INCR_COUNTER(sc, 1024);
			echo_big_compress(sc);
			ptr = 0;
		}
	}
	sc->ptr = ptr;
}

static void
echo_big_close(sph_echo_big_context *sc, unsigned ub, unsigned n,
	void *dst, unsigned out_size_w32)
{
	unsigned char *buf;
	size_t ptr;
	unsigned z;
	unsigned elen;
	union {
		unsigned char tmp[64];
		sph_u32 dummy;
		sph_u64 dummy2;
	} u;
	sph_u64 *VV;
	unsigned k;

	buf = sc->buf;
	ptr = sc->ptr;
	elen = ((unsigned)ptr << 3) + n;
	INCR_COUNTER(sc, elen);
	sph_enc32le_aligned(u.tmp, sc->C0);
	sph_enc32le_aligned(u.tmp + 4, sc->C1);
	sph_enc32le_aligned(u.tmp + 8, sc->C2);
	sph_enc32le_aligned(u.tmp + 12, sc->C3);
	/*
	 * If elen is zero, then this block actually contains no message
	 * bit, only the first padding bit.
	 */
	if (elen == 0) {
		sc->C0 = sc->C1 = sc->C2 = sc->C3 = 0;
	}
	z = 0x80 >> n;
	buf[ptr ++] = ((ub & -z) | z) & 0xFF;
	memset(buf + ptr, 0, (sizeof sc->buf) - ptr);
	if (ptr > ((sizeof sc->buf) - 18)) {
		echo_big_compress(sc);
		sc->C0 = sc->C1 = sc->C2 = sc->C3 = 0;
		memset(buf, 0, sizeof sc->buf);
	}
	sph_enc16le(buf + (sizeof sc->buf) - 18, out_size_w32 << 5);
	memcpy(buf + (sizeof sc->buf) - 16, u.tmp, 16);
	echo_big_compress(sc);
	for (VV = &sc->u.Vb[0][0], k = 0; k < ((out_size_w32 + 1) >> 1); k ++)
		sph_enc64le_aligned(u.tmp + (k << 3), VV[k]);
	memcpy(dst, u.tmp, out_size_w32 << 2);
	echo_big_init(sc, out_size_w32 << 5);
}

/* see sph_echo.h */
void
sph_echo512_init(sph_echo512_context *cc)
{
	echo_big_init(cc, 512);
}

/* see sph_echo.h */
void
sph_echo512(sph_echo512_context *cc, const void *data, size_t len)
{
	echo_big_core(cc, static_cast<const unsigned char*>(data), len);
}

/* see sph_echo.h */
void
sph_echo512_close(sph_echo512_context *cc, void *dst)
{
	echo_big_close(cc, 0, 0, dst, 16);
}

/* see sph_echo.h */
void
sph_echo512_addbits_and_close(sph_echo512_context *cc, unsigned ub, unsigned n, void *dst)
{
	echo_big_close(cc, ub, n, dst, 16);
}
