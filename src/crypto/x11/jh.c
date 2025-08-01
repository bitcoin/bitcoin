/* $Id: jh.c 255 2011-06-07 19:50:20Z tp $ */
/*
 * JH implementation.
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

#include "sph_jh.h"

#ifdef __cplusplus
extern "C"{
#endif


#if SPH_SMALL_FOOTPRINT && !defined SPH_SMALL_FOOTPRINT_JH
#define SPH_SMALL_FOOTPRINT_JH   1
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4146)
#endif

/*
 * The internal bitslice representation may use either big-endian or
 * little-endian (true bitslice operations do not care about the bit
 * ordering, and the bit-swapping linear operations in JH happen to
 * be invariant through endianness-swapping). The constants must be
 * defined according to the chosen endianness; we use some
 * byte-swapping macros for that.
 */

#if SPH_LITTLE_ENDIAN

#define C64e(x)     ((SPH_C64(x) >> 56) \
                    | ((SPH_C64(x) >> 40) & SPH_C64(0x000000000000FF00)) \
                    | ((SPH_C64(x) >> 24) & SPH_C64(0x0000000000FF0000)) \
                    | ((SPH_C64(x) >>  8) & SPH_C64(0x00000000FF000000)) \
                    | ((SPH_C64(x) <<  8) & SPH_C64(0x000000FF00000000)) \
                    | ((SPH_C64(x) << 24) & SPH_C64(0x0000FF0000000000)) \
                    | ((SPH_C64(x) << 40) & SPH_C64(0x00FF000000000000)) \
                    | ((SPH_C64(x) << 56) & SPH_C64(0xFF00000000000000)))
#define dec64e_aligned   sph_dec64le_aligned
#define enc64e           sph_enc64le

#else

#define C64e(x)     SPH_C64(x)
#define dec64e_aligned   sph_dec64be_aligned
#define enc64e           sph_enc64be

#endif

#define Sb(x0, x1, x2, x3, c)   do { \
		x3 = ~x3; \
		x0 ^= (c) & ~x2; \
		tmp = (c) ^ (x0 & x1); \
		x0 ^= x2 & x3; \
		x3 ^= ~x1 & x2; \
		x1 ^= x0 & x2; \
		x2 ^= x0 & ~x3; \
		x0 ^= x1 | x3; \
		x3 ^= x1 & x2; \
		x1 ^= tmp & x0; \
		x2 ^= tmp; \
	} while (0)

#define Lb(x0, x1, x2, x3, x4, x5, x6, x7)   do { \
		x4 ^= x1; \
		x5 ^= x2; \
		x6 ^= x3 ^ x0; \
		x7 ^= x0; \
		x0 ^= x5; \
		x1 ^= x6; \
		x2 ^= x7 ^ x4; \
		x3 ^= x4; \
	} while (0)

static const sph_u64 C[] = {
	C64e(0x72d5dea2df15f867), C64e(0x7b84150ab7231557),
	C64e(0x81abd6904d5a87f6), C64e(0x4e9f4fc5c3d12b40),
	C64e(0xea983ae05c45fa9c), C64e(0x03c5d29966b2999a),
	C64e(0x660296b4f2bb538a), C64e(0xb556141a88dba231),
	C64e(0x03a35a5c9a190edb), C64e(0x403fb20a87c14410),
	C64e(0x1c051980849e951d), C64e(0x6f33ebad5ee7cddc),
	C64e(0x10ba139202bf6b41), C64e(0xdc786515f7bb27d0),
	C64e(0x0a2c813937aa7850), C64e(0x3f1abfd2410091d3),
	C64e(0x422d5a0df6cc7e90), C64e(0xdd629f9c92c097ce),
	C64e(0x185ca70bc72b44ac), C64e(0xd1df65d663c6fc23),
	C64e(0x976e6c039ee0b81a), C64e(0x2105457e446ceca8),
	C64e(0xeef103bb5d8e61fa), C64e(0xfd9697b294838197),
	C64e(0x4a8e8537db03302f), C64e(0x2a678d2dfb9f6a95),
	C64e(0x8afe7381f8b8696c), C64e(0x8ac77246c07f4214),
	C64e(0xc5f4158fbdc75ec4), C64e(0x75446fa78f11bb80),
	C64e(0x52de75b7aee488bc), C64e(0x82b8001e98a6a3f4),
	C64e(0x8ef48f33a9a36315), C64e(0xaa5f5624d5b7f989),
	C64e(0xb6f1ed207c5ae0fd), C64e(0x36cae95a06422c36),
	C64e(0xce2935434efe983d), C64e(0x533af974739a4ba7),
	C64e(0xd0f51f596f4e8186), C64e(0x0e9dad81afd85a9f),
	C64e(0xa7050667ee34626a), C64e(0x8b0b28be6eb91727),
	C64e(0x47740726c680103f), C64e(0xe0a07e6fc67e487b),
	C64e(0x0d550aa54af8a4c0), C64e(0x91e3e79f978ef19e),
	C64e(0x8676728150608dd4), C64e(0x7e9e5a41f3e5b062),
	C64e(0xfc9f1fec4054207a), C64e(0xe3e41a00cef4c984),
	C64e(0x4fd794f59dfa95d8), C64e(0x552e7e1124c354a5),
	C64e(0x5bdf7228bdfe6e28), C64e(0x78f57fe20fa5c4b2),
	C64e(0x05897cefee49d32e), C64e(0x447e9385eb28597f),
	C64e(0x705f6937b324314a), C64e(0x5e8628f11dd6e465),
	C64e(0xc71b770451b920e7), C64e(0x74fe43e823d4878a),
	C64e(0x7d29e8a3927694f2), C64e(0xddcb7a099b30d9c1),
	C64e(0x1d1b30fb5bdc1be0), C64e(0xda24494ff29c82bf),
	C64e(0xa4e7ba31b470bfff), C64e(0x0d324405def8bc48),
	C64e(0x3baefc3253bbd339), C64e(0x459fc3c1e0298ba0),
	C64e(0xe5c905fdf7ae090f), C64e(0x947034124290f134),
	C64e(0xa271b701e344ed95), C64e(0xe93b8e364f2f984a),
	C64e(0x88401d63a06cf615), C64e(0x47c1444b8752afff),
	C64e(0x7ebb4af1e20ac630), C64e(0x4670b6c5cc6e8ce6),
	C64e(0xa4d5a456bd4fca00), C64e(0xda9d844bc83e18ae),
	C64e(0x7357ce453064d1ad), C64e(0xe8a6ce68145c2567),
	C64e(0xa3da8cf2cb0ee116), C64e(0x33e906589a94999a),
	C64e(0x1f60b220c26f847b), C64e(0xd1ceac7fa0d18518),
	C64e(0x32595ba18ddd19d3), C64e(0x509a1cc0aaa5b446),
	C64e(0x9f3d6367e4046bba), C64e(0xf6ca19ab0b56ee7e),
	C64e(0x1fb179eaa9282174), C64e(0xe9bdf7353b3651ee),
	C64e(0x1d57ac5a7550d376), C64e(0x3a46c2fea37d7001),
	C64e(0xf735c1af98a4d842), C64e(0x78edec209e6b6779),
	C64e(0x41836315ea3adba8), C64e(0xfac33b4d32832c83),
	C64e(0xa7403b1f1c2747f3), C64e(0x5940f034b72d769a),
	C64e(0xe73e4e6cd2214ffd), C64e(0xb8fd8d39dc5759ef),
	C64e(0x8d9b0c492b49ebda), C64e(0x5ba2d74968f3700d),
	C64e(0x7d3baed07a8d5584), C64e(0xf5a5e9f0e4f88e65),
	C64e(0xa0b8a2f436103b53), C64e(0x0ca8079e753eec5a),
	C64e(0x9168949256e8884f), C64e(0x5bb05c55f8babc4c),
	C64e(0xe3bb3b99f387947b), C64e(0x75daf4d6726b1c5d),
	C64e(0x64aeac28dc34b36d), C64e(0x6c34a550b828db71),
	C64e(0xf861e2f2108d512a), C64e(0xe3db643359dd75fc),
	C64e(0x1cacbcf143ce3fa2), C64e(0x67bbd13c02e843b0),
	C64e(0x330a5bca8829a175), C64e(0x7f34194db416535c),
	C64e(0x923b94c30e794d1e), C64e(0x797475d7b6eeaf3f),
	C64e(0xeaa8d4f7be1a3921), C64e(0x5cf47e094c232751),
	C64e(0x26a32453ba323cd2), C64e(0x44a3174a6da6d5ad),
	C64e(0xb51d3ea6aff2c908), C64e(0x83593d98916b3c56),
	C64e(0x4cf87ca17286604d), C64e(0x46e23ecc086ec7f6),
	C64e(0x2f9833b3b1bc765e), C64e(0x2bd666a5efc4e62a),
	C64e(0x06f4b6e8bec1d436), C64e(0x74ee8215bcef2163),
	C64e(0xfdc14e0df453c969), C64e(0xa77d5ac406585826),
	C64e(0x7ec1141606e0fa16), C64e(0x7e90af3d28639d3f),
	C64e(0xd2c9f2e3009bd20c), C64e(0x5faace30b7d40c30),
	C64e(0x742a5116f2e03298), C64e(0x0deb30d8e3cef89a),
	C64e(0x4bc59e7bb5f17992), C64e(0xff51e66e048668d3),
	C64e(0x9b234d57e6966731), C64e(0xcce6a6f3170a7505),
	C64e(0xb17681d913326cce), C64e(0x3c175284f805a262),
	C64e(0xf42bcbb378471547), C64e(0xff46548223936a48),
	C64e(0x38df58074e5e6565), C64e(0xf2fc7c89fc86508e),
	C64e(0x31702e44d00bca86), C64e(0xf04009a23078474e),
	C64e(0x65a0ee39d1f73883), C64e(0xf75ee937e42c3abd),
	C64e(0x2197b2260113f86f), C64e(0xa344edd1ef9fdee7),
	C64e(0x8ba0df15762592d9), C64e(0x3c85f7f612dc42be),
	C64e(0xd8a7ec7cab27b07e), C64e(0x538d7ddaaa3ea8de),
	C64e(0xaa25ce93bd0269d8), C64e(0x5af643fd1a7308f9),
	C64e(0xc05fefda174a19a5), C64e(0x974d66334cfd216a),
	C64e(0x35b49831db411570), C64e(0xea1e0fbbedcd549b),
	C64e(0x9ad063a151974072), C64e(0xf6759dbf91476fe2)
};

#define Ceven_hi(r)   (C[((r) << 2) + 0])
#define Ceven_lo(r)   (C[((r) << 2) + 1])
#define Codd_hi(r)    (C[((r) << 2) + 2])
#define Codd_lo(r)    (C[((r) << 2) + 3])

#define S(x0, x1, x2, x3, cb, r)   do { \
		Sb(x0 ## h, x1 ## h, x2 ## h, x3 ## h, cb ## hi(r)); \
		Sb(x0 ## l, x1 ## l, x2 ## l, x3 ## l, cb ## lo(r)); \
	} while (0)

#define L(x0, x1, x2, x3, x4, x5, x6, x7)   do { \
		Lb(x0 ## h, x1 ## h, x2 ## h, x3 ## h, \
			x4 ## h, x5 ## h, x6 ## h, x7 ## h); \
		Lb(x0 ## l, x1 ## l, x2 ## l, x3 ## l, \
			x4 ## l, x5 ## l, x6 ## l, x7 ## l); \
	} while (0)

#define Wz(x, c, n)   do { \
		sph_u64 t = (x ## h & (c)) << (n); \
		x ## h = ((x ## h >> (n)) & (c)) | t; \
		t = (x ## l & (c)) << (n); \
		x ## l = ((x ## l >> (n)) & (c)) | t; \
	} while (0)

#define W0(x)   Wz(x, SPH_C64(0x5555555555555555),  1)
#define W1(x)   Wz(x, SPH_C64(0x3333333333333333),  2)
#define W2(x)   Wz(x, SPH_C64(0x0F0F0F0F0F0F0F0F),  4)
#define W3(x)   Wz(x, SPH_C64(0x00FF00FF00FF00FF),  8)
#define W4(x)   Wz(x, SPH_C64(0x0000FFFF0000FFFF), 16)
#define W5(x)   Wz(x, SPH_C64(0x00000000FFFFFFFF), 32)
#define W6(x)   do { \
		sph_u64 t = x ## h; \
		x ## h = x ## l; \
		x ## l = t; \
	} while (0)

#define DECL_STATE \
	sph_u64 h0h, h1h, h2h, h3h, h4h, h5h, h6h, h7h; \
	sph_u64 h0l, h1l, h2l, h3l, h4l, h5l, h6l, h7l; \
	sph_u64 tmp;

#define READ_STATE(state)   do { \
		h0h = (state)->H.wide[ 0]; \
		h0l = (state)->H.wide[ 1]; \
		h1h = (state)->H.wide[ 2]; \
		h1l = (state)->H.wide[ 3]; \
		h2h = (state)->H.wide[ 4]; \
		h2l = (state)->H.wide[ 5]; \
		h3h = (state)->H.wide[ 6]; \
		h3l = (state)->H.wide[ 7]; \
		h4h = (state)->H.wide[ 8]; \
		h4l = (state)->H.wide[ 9]; \
		h5h = (state)->H.wide[10]; \
		h5l = (state)->H.wide[11]; \
		h6h = (state)->H.wide[12]; \
		h6l = (state)->H.wide[13]; \
		h7h = (state)->H.wide[14]; \
		h7l = (state)->H.wide[15]; \
	} while (0)

#define WRITE_STATE(state)   do { \
		(state)->H.wide[ 0] = h0h; \
		(state)->H.wide[ 1] = h0l; \
		(state)->H.wide[ 2] = h1h; \
		(state)->H.wide[ 3] = h1l; \
		(state)->H.wide[ 4] = h2h; \
		(state)->H.wide[ 5] = h2l; \
		(state)->H.wide[ 6] = h3h; \
		(state)->H.wide[ 7] = h3l; \
		(state)->H.wide[ 8] = h4h; \
		(state)->H.wide[ 9] = h4l; \
		(state)->H.wide[10] = h5h; \
		(state)->H.wide[11] = h5l; \
		(state)->H.wide[12] = h6h; \
		(state)->H.wide[13] = h6l; \
		(state)->H.wide[14] = h7h; \
		(state)->H.wide[15] = h7l; \
	} while (0)

#define INPUT_BUF1 \
	sph_u64 m0h = dec64e_aligned(buf +  0); \
	sph_u64 m0l = dec64e_aligned(buf +  8); \
	sph_u64 m1h = dec64e_aligned(buf + 16); \
	sph_u64 m1l = dec64e_aligned(buf + 24); \
	sph_u64 m2h = dec64e_aligned(buf + 32); \
	sph_u64 m2l = dec64e_aligned(buf + 40); \
	sph_u64 m3h = dec64e_aligned(buf + 48); \
	sph_u64 m3l = dec64e_aligned(buf + 56); \
	h0h ^= m0h; \
	h0l ^= m0l; \
	h1h ^= m1h; \
	h1l ^= m1l; \
	h2h ^= m2h; \
	h2l ^= m2l; \
	h3h ^= m3h; \
	h3l ^= m3l;

#define INPUT_BUF2 \
	h4h ^= m0h; \
	h4l ^= m0l; \
	h5h ^= m1h; \
	h5l ^= m1l; \
	h6h ^= m2h; \
	h6l ^= m2l; \
	h7h ^= m3h; \
	h7l ^= m3l;

static const sph_u64 IV512[] = {
	C64e(0x6fd14b963e00aa17), C64e(0x636a2e057a15d543),
	C64e(0x8a225e8d0c97ef0b), C64e(0xe9341259f2b3c361),
	C64e(0x891da0c1536f801e), C64e(0x2aa9056bea2b6d80),
	C64e(0x588eccdb2075baa6), C64e(0xa90f3a76baf83bf7),
	C64e(0x0169e60541e34a69), C64e(0x46b58a8e2e6fe65a),
	C64e(0x1047a7d0c1843c24), C64e(0x3b6e71b12d5ac199),
	C64e(0xcf57f6ec9db1f856), C64e(0xa706887c5716b156),
	C64e(0xe3c2fcdfe68517fb), C64e(0x545a4678cc8cdd4b)
};

#define SL(ro)   SLu(r + ro, ro)

#define SLu(r, ro)   do { \
		S(h0, h2, h4, h6, Ceven_, r); \
		S(h1, h3, h5, h7, Codd_, r); \
		L(h0, h2, h4, h6, h1, h3, h5, h7); \
		W ## ro(h1); \
		W ## ro(h3); \
		W ## ro(h5); \
		W ## ro(h7); \
	} while (0)

#if SPH_SMALL_FOOTPRINT_JH

/*
 * The "small footprint" 64-bit version just uses a partially unrolled
 * loop.
 */

#define E8   do { \
		unsigned r; \
		for (r = 0; r < 42; r += 7) { \
			SL(0); \
			SL(1); \
			SL(2); \
			SL(3); \
			SL(4); \
			SL(5); \
			SL(6); \
		} \
	} while (0)

#else

/*
 * On a "true 64-bit" architecture, we can unroll at will.
 */

#define E8   do { \
		SLu( 0, 0); \
		SLu( 1, 1); \
		SLu( 2, 2); \
		SLu( 3, 3); \
		SLu( 4, 4); \
		SLu( 5, 5); \
		SLu( 6, 6); \
		SLu( 7, 0); \
		SLu( 8, 1); \
		SLu( 9, 2); \
		SLu(10, 3); \
		SLu(11, 4); \
		SLu(12, 5); \
		SLu(13, 6); \
		SLu(14, 0); \
		SLu(15, 1); \
		SLu(16, 2); \
		SLu(17, 3); \
		SLu(18, 4); \
		SLu(19, 5); \
		SLu(20, 6); \
		SLu(21, 0); \
		SLu(22, 1); \
		SLu(23, 2); \
		SLu(24, 3); \
		SLu(25, 4); \
		SLu(26, 5); \
		SLu(27, 6); \
		SLu(28, 0); \
		SLu(29, 1); \
		SLu(30, 2); \
		SLu(31, 3); \
		SLu(32, 4); \
		SLu(33, 5); \
		SLu(34, 6); \
		SLu(35, 0); \
		SLu(36, 1); \
		SLu(37, 2); \
		SLu(38, 3); \
		SLu(39, 4); \
		SLu(40, 5); \
		SLu(41, 6); \
	} while (0)

#endif

static void
jh_init(sph_jh_context *sc, const void *iv)
{
	sc->ptr = 0;
	memcpy(sc->H.wide, iv, sizeof sc->H.wide);
	sc->block_count = 0;
}

static void
jh_core(sph_jh_context *sc, const void *data, size_t len)
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
			INPUT_BUF1;
			E8;
			INPUT_BUF2;
			sc->block_count ++;
			ptr = 0;
		}
	}
	WRITE_STATE(sc);
	sc->ptr = ptr;
}

static void
jh_close(sph_jh_context *sc, unsigned ub, unsigned n,
	void *dst, size_t out_size_w32, const void *iv)
{
	unsigned z;
	unsigned char buf[128];
	size_t numz, u;
	sph_u64 l0, l1;

	z = 0x80 >> n;
	buf[0] = ((ub & -z) | z) & 0xFF;
	if (sc->ptr == 0 && n == 0) {
		numz = 47;
	} else {
		numz = 111 - sc->ptr;
	}
	memset(buf + 1, 0, numz);
	l0 = SPH_T64(sc->block_count << 9) + (sc->ptr << 3) + n;
	l1 = SPH_T64(sc->block_count >> 55);
	sph_enc64be(buf + numz + 1, l1);
	sph_enc64be(buf + numz + 9, l0);
	jh_core(sc, buf, numz + 17);
	for (u = 0; u < 8; u ++)
		enc64e(buf + (u << 3), sc->H.wide[u + 8]);
	memcpy(dst, buf + ((16 - out_size_w32) << 2), out_size_w32 << 2);
	jh_init(sc, iv);
}

/* see sph_jh.h */
void
sph_jh512_init(void *cc)
{
	jh_init(cc, IV512);
}

/* see sph_jh.h */
void
sph_jh512(void *cc, const void *data, size_t len)
{
	jh_core(cc, data, len);
}

/* see sph_jh.h */
void
sph_jh512_close(void *cc, void *dst)
{
	jh_close(cc, 0, 0, dst, 16, IV512);
}

/* see sph_jh.h */
void
sph_jh512_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	jh_close(cc, ub, n, dst, 16, IV512);
}

#ifdef __cplusplus
}
#endif
