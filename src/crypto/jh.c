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

#if !defined SPH_JH_64 && SPH_64_TRUE
#define SPH_JH_64   1
#endif

#if !SPH_64
#undef SPH_JH_64
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

#define C32e(x)     ((SPH_C32(x) >> 24) \
                    | ((SPH_C32(x) >>  8) & SPH_C32(0x0000FF00)) \
                    | ((SPH_C32(x) <<  8) & SPH_C32(0x00FF0000)) \
                    | ((SPH_C32(x) << 24) & SPH_C32(0xFF000000)))
#define dec32e_aligned   sph_dec32le_aligned
#define enc32e           sph_enc32le

#if SPH_64
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
#endif

#else

#define C32e(x)     SPH_C32(x)
#define dec32e_aligned   sph_dec32be_aligned
#define enc32e           sph_enc32be
#if SPH_64
#define C64e(x)     SPH_C64(x)
#define dec64e_aligned   sph_dec64be_aligned
#define enc64e           sph_enc64be
#endif

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

#if SPH_JH_64

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

static const sph_u64 IV224[] = {
	C64e(0x2dfedd62f99a98ac), C64e(0xae7cacd619d634e7),
	C64e(0xa4831005bc301216), C64e(0xb86038c6c9661494),
	C64e(0x66d9899f2580706f), C64e(0xce9ea31b1d9b1adc),
	C64e(0x11e8325f7b366e10), C64e(0xf994857f02fa06c1),
	C64e(0x1b4f1b5cd8c840b3), C64e(0x97f6a17f6e738099),
	C64e(0xdcdf93a5adeaa3d3), C64e(0xa431e8dec9539a68),
	C64e(0x22b4a98aec86a1e4), C64e(0xd574ac959ce56cf0),
	C64e(0x15960deab5ab2bbf), C64e(0x9611dcf0dd64ea6e)
};

static const sph_u64 IV256[] = {
	C64e(0xeb98a3412c20d3eb), C64e(0x92cdbe7b9cb245c1),
	C64e(0x1c93519160d4c7fa), C64e(0x260082d67e508a03),
	C64e(0xa4239e267726b945), C64e(0xe0fb1a48d41a9477),
	C64e(0xcdb5ab26026b177a), C64e(0x56f024420fff2fa8),
	C64e(0x71a396897f2e4d75), C64e(0x1d144908f77de262),
	C64e(0x277695f776248f94), C64e(0x87d5b6574780296c),
	C64e(0x5c5e272dac8e0d6c), C64e(0x518450c657057a0f),
	C64e(0x7be4d367702412ea), C64e(0x89e3ab13d31cd769)
};

static const sph_u64 IV384[] = {
	C64e(0x481e3bc6d813398a), C64e(0x6d3b5e894ade879b),
	C64e(0x63faea68d480ad2e), C64e(0x332ccb21480f8267),
	C64e(0x98aec84d9082b928), C64e(0xd455ea3041114249),
	C64e(0x36f555b2924847ec), C64e(0xc7250a93baf43ce1),
	C64e(0x569b7f8a27db454c), C64e(0x9efcbd496397af0e),
	C64e(0x589fc27d26aa80cd), C64e(0x80c08b8c9deb2eda),
	C64e(0x8a7981e8f8d5373a), C64e(0xf43967adddd17a71),
	C64e(0xa9b4d3bda475d394), C64e(0x976c3fba9842737f)
};

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

#else

static const sph_u32 C[] = {
	C32e(0x72d5dea2), C32e(0xdf15f867), C32e(0x7b84150a),
	C32e(0xb7231557), C32e(0x81abd690), C32e(0x4d5a87f6),
	C32e(0x4e9f4fc5), C32e(0xc3d12b40), C32e(0xea983ae0),
	C32e(0x5c45fa9c), C32e(0x03c5d299), C32e(0x66b2999a),
	C32e(0x660296b4), C32e(0xf2bb538a), C32e(0xb556141a),
	C32e(0x88dba231), C32e(0x03a35a5c), C32e(0x9a190edb),
	C32e(0x403fb20a), C32e(0x87c14410), C32e(0x1c051980),
	C32e(0x849e951d), C32e(0x6f33ebad), C32e(0x5ee7cddc),
	C32e(0x10ba1392), C32e(0x02bf6b41), C32e(0xdc786515),
	C32e(0xf7bb27d0), C32e(0x0a2c8139), C32e(0x37aa7850),
	C32e(0x3f1abfd2), C32e(0x410091d3), C32e(0x422d5a0d),
	C32e(0xf6cc7e90), C32e(0xdd629f9c), C32e(0x92c097ce),
	C32e(0x185ca70b), C32e(0xc72b44ac), C32e(0xd1df65d6),
	C32e(0x63c6fc23), C32e(0x976e6c03), C32e(0x9ee0b81a),
	C32e(0x2105457e), C32e(0x446ceca8), C32e(0xeef103bb),
	C32e(0x5d8e61fa), C32e(0xfd9697b2), C32e(0x94838197),
	C32e(0x4a8e8537), C32e(0xdb03302f), C32e(0x2a678d2d),
	C32e(0xfb9f6a95), C32e(0x8afe7381), C32e(0xf8b8696c),
	C32e(0x8ac77246), C32e(0xc07f4214), C32e(0xc5f4158f),
	C32e(0xbdc75ec4), C32e(0x75446fa7), C32e(0x8f11bb80),
	C32e(0x52de75b7), C32e(0xaee488bc), C32e(0x82b8001e),
	C32e(0x98a6a3f4), C32e(0x8ef48f33), C32e(0xa9a36315),
	C32e(0xaa5f5624), C32e(0xd5b7f989), C32e(0xb6f1ed20),
	C32e(0x7c5ae0fd), C32e(0x36cae95a), C32e(0x06422c36),
	C32e(0xce293543), C32e(0x4efe983d), C32e(0x533af974),
	C32e(0x739a4ba7), C32e(0xd0f51f59), C32e(0x6f4e8186),
	C32e(0x0e9dad81), C32e(0xafd85a9f), C32e(0xa7050667),
	C32e(0xee34626a), C32e(0x8b0b28be), C32e(0x6eb91727),
	C32e(0x47740726), C32e(0xc680103f), C32e(0xe0a07e6f),
	C32e(0xc67e487b), C32e(0x0d550aa5), C32e(0x4af8a4c0),
	C32e(0x91e3e79f), C32e(0x978ef19e), C32e(0x86767281),
	C32e(0x50608dd4), C32e(0x7e9e5a41), C32e(0xf3e5b062),
	C32e(0xfc9f1fec), C32e(0x4054207a), C32e(0xe3e41a00),
	C32e(0xcef4c984), C32e(0x4fd794f5), C32e(0x9dfa95d8),
	C32e(0x552e7e11), C32e(0x24c354a5), C32e(0x5bdf7228),
	C32e(0xbdfe6e28), C32e(0x78f57fe2), C32e(0x0fa5c4b2),
	C32e(0x05897cef), C32e(0xee49d32e), C32e(0x447e9385),
	C32e(0xeb28597f), C32e(0x705f6937), C32e(0xb324314a),
	C32e(0x5e8628f1), C32e(0x1dd6e465), C32e(0xc71b7704),
	C32e(0x51b920e7), C32e(0x74fe43e8), C32e(0x23d4878a),
	C32e(0x7d29e8a3), C32e(0x927694f2), C32e(0xddcb7a09),
	C32e(0x9b30d9c1), C32e(0x1d1b30fb), C32e(0x5bdc1be0),
	C32e(0xda24494f), C32e(0xf29c82bf), C32e(0xa4e7ba31),
	C32e(0xb470bfff), C32e(0x0d324405), C32e(0xdef8bc48),
	C32e(0x3baefc32), C32e(0x53bbd339), C32e(0x459fc3c1),
	C32e(0xe0298ba0), C32e(0xe5c905fd), C32e(0xf7ae090f),
	C32e(0x94703412), C32e(0x4290f134), C32e(0xa271b701),
	C32e(0xe344ed95), C32e(0xe93b8e36), C32e(0x4f2f984a),
	C32e(0x88401d63), C32e(0xa06cf615), C32e(0x47c1444b),
	C32e(0x8752afff), C32e(0x7ebb4af1), C32e(0xe20ac630),
	C32e(0x4670b6c5), C32e(0xcc6e8ce6), C32e(0xa4d5a456),
	C32e(0xbd4fca00), C32e(0xda9d844b), C32e(0xc83e18ae),
	C32e(0x7357ce45), C32e(0x3064d1ad), C32e(0xe8a6ce68),
	C32e(0x145c2567), C32e(0xa3da8cf2), C32e(0xcb0ee116),
	C32e(0x33e90658), C32e(0x9a94999a), C32e(0x1f60b220),
	C32e(0xc26f847b), C32e(0xd1ceac7f), C32e(0xa0d18518),
	C32e(0x32595ba1), C32e(0x8ddd19d3), C32e(0x509a1cc0),
	C32e(0xaaa5b446), C32e(0x9f3d6367), C32e(0xe4046bba),
	C32e(0xf6ca19ab), C32e(0x0b56ee7e), C32e(0x1fb179ea),
	C32e(0xa9282174), C32e(0xe9bdf735), C32e(0x3b3651ee),
	C32e(0x1d57ac5a), C32e(0x7550d376), C32e(0x3a46c2fe),
	C32e(0xa37d7001), C32e(0xf735c1af), C32e(0x98a4d842),
	C32e(0x78edec20), C32e(0x9e6b6779), C32e(0x41836315),
	C32e(0xea3adba8), C32e(0xfac33b4d), C32e(0x32832c83),
	C32e(0xa7403b1f), C32e(0x1c2747f3), C32e(0x5940f034),
	C32e(0xb72d769a), C32e(0xe73e4e6c), C32e(0xd2214ffd),
	C32e(0xb8fd8d39), C32e(0xdc5759ef), C32e(0x8d9b0c49),
	C32e(0x2b49ebda), C32e(0x5ba2d749), C32e(0x68f3700d),
	C32e(0x7d3baed0), C32e(0x7a8d5584), C32e(0xf5a5e9f0),
	C32e(0xe4f88e65), C32e(0xa0b8a2f4), C32e(0x36103b53),
	C32e(0x0ca8079e), C32e(0x753eec5a), C32e(0x91689492),
	C32e(0x56e8884f), C32e(0x5bb05c55), C32e(0xf8babc4c),
	C32e(0xe3bb3b99), C32e(0xf387947b), C32e(0x75daf4d6),
	C32e(0x726b1c5d), C32e(0x64aeac28), C32e(0xdc34b36d),
	C32e(0x6c34a550), C32e(0xb828db71), C32e(0xf861e2f2),
	C32e(0x108d512a), C32e(0xe3db6433), C32e(0x59dd75fc),
	C32e(0x1cacbcf1), C32e(0x43ce3fa2), C32e(0x67bbd13c),
	C32e(0x02e843b0), C32e(0x330a5bca), C32e(0x8829a175),
	C32e(0x7f34194d), C32e(0xb416535c), C32e(0x923b94c3),
	C32e(0x0e794d1e), C32e(0x797475d7), C32e(0xb6eeaf3f),
	C32e(0xeaa8d4f7), C32e(0xbe1a3921), C32e(0x5cf47e09),
	C32e(0x4c232751), C32e(0x26a32453), C32e(0xba323cd2),
	C32e(0x44a3174a), C32e(0x6da6d5ad), C32e(0xb51d3ea6),
	C32e(0xaff2c908), C32e(0x83593d98), C32e(0x916b3c56),
	C32e(0x4cf87ca1), C32e(0x7286604d), C32e(0x46e23ecc),
	C32e(0x086ec7f6), C32e(0x2f9833b3), C32e(0xb1bc765e),
	C32e(0x2bd666a5), C32e(0xefc4e62a), C32e(0x06f4b6e8),
	C32e(0xbec1d436), C32e(0x74ee8215), C32e(0xbcef2163),
	C32e(0xfdc14e0d), C32e(0xf453c969), C32e(0xa77d5ac4),
	C32e(0x06585826), C32e(0x7ec11416), C32e(0x06e0fa16),
	C32e(0x7e90af3d), C32e(0x28639d3f), C32e(0xd2c9f2e3),
	C32e(0x009bd20c), C32e(0x5faace30), C32e(0xb7d40c30),
	C32e(0x742a5116), C32e(0xf2e03298), C32e(0x0deb30d8),
	C32e(0xe3cef89a), C32e(0x4bc59e7b), C32e(0xb5f17992),
	C32e(0xff51e66e), C32e(0x048668d3), C32e(0x9b234d57),
	C32e(0xe6966731), C32e(0xcce6a6f3), C32e(0x170a7505),
	C32e(0xb17681d9), C32e(0x13326cce), C32e(0x3c175284),
	C32e(0xf805a262), C32e(0xf42bcbb3), C32e(0x78471547),
	C32e(0xff465482), C32e(0x23936a48), C32e(0x38df5807),
	C32e(0x4e5e6565), C32e(0xf2fc7c89), C32e(0xfc86508e),
	C32e(0x31702e44), C32e(0xd00bca86), C32e(0xf04009a2),
	C32e(0x3078474e), C32e(0x65a0ee39), C32e(0xd1f73883),
	C32e(0xf75ee937), C32e(0xe42c3abd), C32e(0x2197b226),
	C32e(0x0113f86f), C32e(0xa344edd1), C32e(0xef9fdee7),
	C32e(0x8ba0df15), C32e(0x762592d9), C32e(0x3c85f7f6),
	C32e(0x12dc42be), C32e(0xd8a7ec7c), C32e(0xab27b07e),
	C32e(0x538d7dda), C32e(0xaa3ea8de), C32e(0xaa25ce93),
	C32e(0xbd0269d8), C32e(0x5af643fd), C32e(0x1a7308f9),
	C32e(0xc05fefda), C32e(0x174a19a5), C32e(0x974d6633),
	C32e(0x4cfd216a), C32e(0x35b49831), C32e(0xdb411570),
	C32e(0xea1e0fbb), C32e(0xedcd549b), C32e(0x9ad063a1),
	C32e(0x51974072), C32e(0xf6759dbf), C32e(0x91476fe2)
};

#define Ceven_w3(r)   (C[((r) << 3) + 0])
#define Ceven_w2(r)   (C[((r) << 3) + 1])
#define Ceven_w1(r)   (C[((r) << 3) + 2])
#define Ceven_w0(r)   (C[((r) << 3) + 3])
#define Codd_w3(r)    (C[((r) << 3) + 4])
#define Codd_w2(r)    (C[((r) << 3) + 5])
#define Codd_w1(r)    (C[((r) << 3) + 6])
#define Codd_w0(r)    (C[((r) << 3) + 7])

#define S(x0, x1, x2, x3, cb, r)   do { \
		Sb(x0 ## 3, x1 ## 3, x2 ## 3, x3 ## 3, cb ## w3(r)); \
		Sb(x0 ## 2, x1 ## 2, x2 ## 2, x3 ## 2, cb ## w2(r)); \
		Sb(x0 ## 1, x1 ## 1, x2 ## 1, x3 ## 1, cb ## w1(r)); \
		Sb(x0 ## 0, x1 ## 0, x2 ## 0, x3 ## 0, cb ## w0(r)); \
	} while (0)

#define L(x0, x1, x2, x3, x4, x5, x6, x7)   do { \
		Lb(x0 ## 3, x1 ## 3, x2 ## 3, x3 ## 3, \
			x4 ## 3, x5 ## 3, x6 ## 3, x7 ## 3); \
		Lb(x0 ## 2, x1 ## 2, x2 ## 2, x3 ## 2, \
			x4 ## 2, x5 ## 2, x6 ## 2, x7 ## 2); \
		Lb(x0 ## 1, x1 ## 1, x2 ## 1, x3 ## 1, \
			x4 ## 1, x5 ## 1, x6 ## 1, x7 ## 1); \
		Lb(x0 ## 0, x1 ## 0, x2 ## 0, x3 ## 0, \
			x4 ## 0, x5 ## 0, x6 ## 0, x7 ## 0); \
	} while (0)

#define Wz(x, c, n)   do { \
		sph_u32 t = (x ## 3 & (c)) << (n); \
		x ## 3 = ((x ## 3 >> (n)) & (c)) | t; \
		t = (x ## 2 & (c)) << (n); \
		x ## 2 = ((x ## 2 >> (n)) & (c)) | t; \
		t = (x ## 1 & (c)) << (n); \
		x ## 1 = ((x ## 1 >> (n)) & (c)) | t; \
		t = (x ## 0 & (c)) << (n); \
		x ## 0 = ((x ## 0 >> (n)) & (c)) | t; \
	} while (0)

#define W0(x)   Wz(x, SPH_C32(0x55555555),  1)
#define W1(x)   Wz(x, SPH_C32(0x33333333),  2)
#define W2(x)   Wz(x, SPH_C32(0x0F0F0F0F),  4)
#define W3(x)   Wz(x, SPH_C32(0x00FF00FF),  8)
#define W4(x)   Wz(x, SPH_C32(0x0000FFFF), 16)
#define W5(x)   do { \
		sph_u32 t = x ## 3; \
		x ## 3 = x ## 2; \
		x ## 2 = t; \
		t = x ## 1; \
		x ## 1 = x ## 0; \
		x ## 0 = t; \
	} while (0)
#define W6(x)   do { \
		sph_u32 t = x ## 3; \
		x ## 3 = x ## 1; \
		x ## 1 = t; \
		t = x ## 2; \
		x ## 2 = x ## 0; \
		x ## 0 = t; \
	} while (0)

#define DECL_STATE \
	sph_u32 h03, h02, h01, h00, h13, h12, h11, h10; \
	sph_u32 h23, h22, h21, h20, h33, h32, h31, h30; \
	sph_u32 h43, h42, h41, h40, h53, h52, h51, h50; \
	sph_u32 h63, h62, h61, h60, h73, h72, h71, h70; \
	sph_u32 tmp;

#define READ_STATE(state)   do { \
		h03 = (state)->H.narrow[ 0]; \
		h02 = (state)->H.narrow[ 1]; \
		h01 = (state)->H.narrow[ 2]; \
		h00 = (state)->H.narrow[ 3]; \
		h13 = (state)->H.narrow[ 4]; \
		h12 = (state)->H.narrow[ 5]; \
		h11 = (state)->H.narrow[ 6]; \
		h10 = (state)->H.narrow[ 7]; \
		h23 = (state)->H.narrow[ 8]; \
		h22 = (state)->H.narrow[ 9]; \
		h21 = (state)->H.narrow[10]; \
		h20 = (state)->H.narrow[11]; \
		h33 = (state)->H.narrow[12]; \
		h32 = (state)->H.narrow[13]; \
		h31 = (state)->H.narrow[14]; \
		h30 = (state)->H.narrow[15]; \
		h43 = (state)->H.narrow[16]; \
		h42 = (state)->H.narrow[17]; \
		h41 = (state)->H.narrow[18]; \
		h40 = (state)->H.narrow[19]; \
		h53 = (state)->H.narrow[20]; \
		h52 = (state)->H.narrow[21]; \
		h51 = (state)->H.narrow[22]; \
		h50 = (state)->H.narrow[23]; \
		h63 = (state)->H.narrow[24]; \
		h62 = (state)->H.narrow[25]; \
		h61 = (state)->H.narrow[26]; \
		h60 = (state)->H.narrow[27]; \
		h73 = (state)->H.narrow[28]; \
		h72 = (state)->H.narrow[29]; \
		h71 = (state)->H.narrow[30]; \
		h70 = (state)->H.narrow[31]; \
	} while (0)

#define WRITE_STATE(state)   do { \
		(state)->H.narrow[ 0] = h03; \
		(state)->H.narrow[ 1] = h02; \
		(state)->H.narrow[ 2] = h01; \
		(state)->H.narrow[ 3] = h00; \
		(state)->H.narrow[ 4] = h13; \
		(state)->H.narrow[ 5] = h12; \
		(state)->H.narrow[ 6] = h11; \
		(state)->H.narrow[ 7] = h10; \
		(state)->H.narrow[ 8] = h23; \
		(state)->H.narrow[ 9] = h22; \
		(state)->H.narrow[10] = h21; \
		(state)->H.narrow[11] = h20; \
		(state)->H.narrow[12] = h33; \
		(state)->H.narrow[13] = h32; \
		(state)->H.narrow[14] = h31; \
		(state)->H.narrow[15] = h30; \
		(state)->H.narrow[16] = h43; \
		(state)->H.narrow[17] = h42; \
		(state)->H.narrow[18] = h41; \
		(state)->H.narrow[19] = h40; \
		(state)->H.narrow[20] = h53; \
		(state)->H.narrow[21] = h52; \
		(state)->H.narrow[22] = h51; \
		(state)->H.narrow[23] = h50; \
		(state)->H.narrow[24] = h63; \
		(state)->H.narrow[25] = h62; \
		(state)->H.narrow[26] = h61; \
		(state)->H.narrow[27] = h60; \
		(state)->H.narrow[28] = h73; \
		(state)->H.narrow[29] = h72; \
		(state)->H.narrow[30] = h71; \
		(state)->H.narrow[31] = h70; \
	} while (0)

#define INPUT_BUF1 \
	sph_u32 m03 = dec32e_aligned(buf +  0); \
	sph_u32 m02 = dec32e_aligned(buf +  4); \
	sph_u32 m01 = dec32e_aligned(buf +  8); \
	sph_u32 m00 = dec32e_aligned(buf + 12); \
	sph_u32 m13 = dec32e_aligned(buf + 16); \
	sph_u32 m12 = dec32e_aligned(buf + 20); \
	sph_u32 m11 = dec32e_aligned(buf + 24); \
	sph_u32 m10 = dec32e_aligned(buf + 28); \
	sph_u32 m23 = dec32e_aligned(buf + 32); \
	sph_u32 m22 = dec32e_aligned(buf + 36); \
	sph_u32 m21 = dec32e_aligned(buf + 40); \
	sph_u32 m20 = dec32e_aligned(buf + 44); \
	sph_u32 m33 = dec32e_aligned(buf + 48); \
	sph_u32 m32 = dec32e_aligned(buf + 52); \
	sph_u32 m31 = dec32e_aligned(buf + 56); \
	sph_u32 m30 = dec32e_aligned(buf + 60); \
	h03 ^= m03; \
	h02 ^= m02; \
	h01 ^= m01; \
	h00 ^= m00; \
	h13 ^= m13; \
	h12 ^= m12; \
	h11 ^= m11; \
	h10 ^= m10; \
	h23 ^= m23; \
	h22 ^= m22; \
	h21 ^= m21; \
	h20 ^= m20; \
	h33 ^= m33; \
	h32 ^= m32; \
	h31 ^= m31; \
	h30 ^= m30;

#define INPUT_BUF2 \
	h43 ^= m03; \
	h42 ^= m02; \
	h41 ^= m01; \
	h40 ^= m00; \
	h53 ^= m13; \
	h52 ^= m12; \
	h51 ^= m11; \
	h50 ^= m10; \
	h63 ^= m23; \
	h62 ^= m22; \
	h61 ^= m21; \
	h60 ^= m20; \
	h73 ^= m33; \
	h72 ^= m32; \
	h71 ^= m31; \
	h70 ^= m30;

static const sph_u32 IV224[] = {
	C32e(0x2dfedd62), C32e(0xf99a98ac), C32e(0xae7cacd6), C32e(0x19d634e7),
	C32e(0xa4831005), C32e(0xbc301216), C32e(0xb86038c6), C32e(0xc9661494),
	C32e(0x66d9899f), C32e(0x2580706f), C32e(0xce9ea31b), C32e(0x1d9b1adc),
	C32e(0x11e8325f), C32e(0x7b366e10), C32e(0xf994857f), C32e(0x02fa06c1),
	C32e(0x1b4f1b5c), C32e(0xd8c840b3), C32e(0x97f6a17f), C32e(0x6e738099),
	C32e(0xdcdf93a5), C32e(0xadeaa3d3), C32e(0xa431e8de), C32e(0xc9539a68),
	C32e(0x22b4a98a), C32e(0xec86a1e4), C32e(0xd574ac95), C32e(0x9ce56cf0),
	C32e(0x15960dea), C32e(0xb5ab2bbf), C32e(0x9611dcf0), C32e(0xdd64ea6e)
};

static const sph_u32 IV256[] = {
	C32e(0xeb98a341), C32e(0x2c20d3eb), C32e(0x92cdbe7b), C32e(0x9cb245c1),
	C32e(0x1c935191), C32e(0x60d4c7fa), C32e(0x260082d6), C32e(0x7e508a03),
	C32e(0xa4239e26), C32e(0x7726b945), C32e(0xe0fb1a48), C32e(0xd41a9477),
	C32e(0xcdb5ab26), C32e(0x026b177a), C32e(0x56f02442), C32e(0x0fff2fa8),
	C32e(0x71a39689), C32e(0x7f2e4d75), C32e(0x1d144908), C32e(0xf77de262),
	C32e(0x277695f7), C32e(0x76248f94), C32e(0x87d5b657), C32e(0x4780296c),
	C32e(0x5c5e272d), C32e(0xac8e0d6c), C32e(0x518450c6), C32e(0x57057a0f),
	C32e(0x7be4d367), C32e(0x702412ea), C32e(0x89e3ab13), C32e(0xd31cd769)
};

static const sph_u32 IV384[] = {
	C32e(0x481e3bc6), C32e(0xd813398a), C32e(0x6d3b5e89), C32e(0x4ade879b),
	C32e(0x63faea68), C32e(0xd480ad2e), C32e(0x332ccb21), C32e(0x480f8267),
	C32e(0x98aec84d), C32e(0x9082b928), C32e(0xd455ea30), C32e(0x41114249),
	C32e(0x36f555b2), C32e(0x924847ec), C32e(0xc7250a93), C32e(0xbaf43ce1),
	C32e(0x569b7f8a), C32e(0x27db454c), C32e(0x9efcbd49), C32e(0x6397af0e),
	C32e(0x589fc27d), C32e(0x26aa80cd), C32e(0x80c08b8c), C32e(0x9deb2eda),
	C32e(0x8a7981e8), C32e(0xf8d5373a), C32e(0xf43967ad), C32e(0xddd17a71),
	C32e(0xa9b4d3bd), C32e(0xa475d394), C32e(0x976c3fba), C32e(0x9842737f)
};

static const sph_u32 IV512[] = {
	C32e(0x6fd14b96), C32e(0x3e00aa17), C32e(0x636a2e05), C32e(0x7a15d543),
	C32e(0x8a225e8d), C32e(0x0c97ef0b), C32e(0xe9341259), C32e(0xf2b3c361),
	C32e(0x891da0c1), C32e(0x536f801e), C32e(0x2aa9056b), C32e(0xea2b6d80),
	C32e(0x588eccdb), C32e(0x2075baa6), C32e(0xa90f3a76), C32e(0xbaf83bf7),
	C32e(0x0169e605), C32e(0x41e34a69), C32e(0x46b58a8e), C32e(0x2e6fe65a),
	C32e(0x1047a7d0), C32e(0xc1843c24), C32e(0x3b6e71b1), C32e(0x2d5ac199),
	C32e(0xcf57f6ec), C32e(0x9db1f856), C32e(0xa706887c), C32e(0x5716b156),
	C32e(0xe3c2fcdf), C32e(0xe68517fb), C32e(0x545a4678), C32e(0xcc8cdd4b)
};

#endif

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

#if SPH_JH_64

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

#define E8   do { \
		unsigned r, g; \
		for (r = g = 0; r < 42; r ++) { \
			S(h0, h2, h4, h6, Ceven_, r); \
			S(h1, h3, h5, h7, Codd_, r); \
			L(h0, h2, h4, h6, h1, h3, h5, h7); \
			switch (g) { \
			case 0: \
				W0(h1); \
				W0(h3); \
				W0(h5); \
				W0(h7); \
				break; \
			case 1: \
				W1(h1); \
				W1(h3); \
				W1(h5); \
				W1(h7); \
				break; \
			case 2: \
				W2(h1); \
				W2(h3); \
				W2(h5); \
				W2(h7); \
				break; \
			case 3: \
				W3(h1); \
				W3(h3); \
				W3(h5); \
				W3(h7); \
				break; \
			case 4: \
				W4(h1); \
				W4(h3); \
				W4(h5); \
				W4(h7); \
				break; \
			case 5: \
				W5(h1); \
				W5(h3); \
				W5(h5); \
				W5(h7); \
				break; \
			case 6: \
				W6(h1); \
				W6(h3); \
				W6(h5); \
				W6(h7); \
				break; \
			} \
			if (++ g == 7) \
				g = 0; \
		} \
	} while (0)

#endif

#else

#if SPH_JH_64

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

#else

/*
 * We are not aiming at a small footprint, but we are still using a
 * 32-bit implementation. Full loop unrolling would smash the L1
 * cache on some "big" architectures (32 kB L1 cache).
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

#endif

#endif

static void
jh_init(sph_jh_context *sc, const void *iv)
{
	sc->ptr = 0;
#if SPH_JH_64
	memcpy(sc->H.wide, iv, sizeof sc->H.wide);
#else
	memcpy(sc->H.narrow, iv, sizeof sc->H.narrow);
#endif
#if SPH_64
	sc->block_count = 0;
#else
	sc->block_count_high = 0;
	sc->block_count_low = 0;
#endif
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
#if SPH_64
			sc->block_count ++;
#else
			if ((sc->block_count_low = SPH_T32(
				sc->block_count_low + 1)) == 0)
				sc->block_count_high ++;
#endif
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
#if SPH_64
	sph_u64 l0, l1;
#else
	sph_u32 l0, l1, l2, l3;
#endif

	z = 0x80 >> n;
	buf[0] = ((ub & -z) | z) & 0xFF;
	if (sc->ptr == 0 && n == 0) {
		numz = 47;
	} else {
		numz = 111 - sc->ptr;
	}
	memset(buf + 1, 0, numz);
#if SPH_64
	l0 = SPH_T64(sc->block_count << 9) + (sc->ptr << 3) + n;
	l1 = SPH_T64(sc->block_count >> 55);
	sph_enc64be(buf + numz + 1, l1);
	sph_enc64be(buf + numz + 9, l0);
#else
	l0 = SPH_T32(sc->block_count_low << 9) + (sc->ptr << 3) + n;
	l1 = SPH_T32(sc->block_count_low >> 23)
		+ SPH_T32(sc->block_count_high << 9);
	l2 = SPH_T32(sc->block_count_high >> 23);
	l3 = 0;
	sph_enc32be(buf + numz +  1, l3);
	sph_enc32be(buf + numz +  5, l2);
	sph_enc32be(buf + numz +  9, l1);
	sph_enc32be(buf + numz + 13, l0);
#endif
	jh_core(sc, buf, numz + 17);
#if SPH_JH_64
	for (u = 0; u < 8; u ++)
		enc64e(buf + (u << 3), sc->H.wide[u + 8]);
#else
	for (u = 0; u < 16; u ++)
		enc32e(buf + (u << 2), sc->H.narrow[u + 16]);
#endif
	memcpy(dst, buf + ((16 - out_size_w32) << 2), out_size_w32 << 2);
	jh_init(sc, iv);
}

/* see sph_jh.h */
void
sph_jh224_init(void *cc)
{
	jh_init(cc, IV224);
}

/* see sph_jh.h */
void
sph_jh224(void *cc, const void *data, size_t len)
{
	jh_core(cc, data, len);
}

/* see sph_jh.h */
void
sph_jh224_close(void *cc, void *dst)
{
	jh_close(cc, 0, 0, dst, 7, IV224);
}

/* see sph_jh.h */
void
sph_jh224_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	jh_close(cc, ub, n, dst, 7, IV224);
}

/* see sph_jh.h */
void
sph_jh256_init(void *cc)
{
	jh_init(cc, IV256);
}

/* see sph_jh.h */
void
sph_jh256(void *cc, const void *data, size_t len)
{
	jh_core(cc, data, len);
}

/* see sph_jh.h */
void
sph_jh256_close(void *cc, void *dst)
{
	jh_close(cc, 0, 0, dst, 8, IV256);
}

/* see sph_jh.h */
void
sph_jh256_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	jh_close(cc, ub, n, dst, 8, IV256);
}

/* see sph_jh.h */
void
sph_jh384_init(void *cc)
{
	jh_init(cc, IV384);
}

/* see sph_jh.h */
void
sph_jh384(void *cc, const void *data, size_t len)
{
	jh_core(cc, data, len);
}

/* see sph_jh.h */
void
sph_jh384_close(void *cc, void *dst)
{
	jh_close(cc, 0, 0, dst, 12, IV384);
}

/* see sph_jh.h */
void
sph_jh384_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	jh_close(cc, ub, n, dst, 12, IV384);
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
