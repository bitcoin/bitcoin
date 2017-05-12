// camellia.cpp - by Kevin Springle, 2003
// This code is hereby placed in the public domain.

/*
Optimisations and defense against timing attacks added in Jan 2007 by Wei Dai.

The first 2 rounds and the last round seem especially vulnerable to timing
attacks. The protection is similar to what was implemented for Rijndael.
See comments at top of rijndael.cpp for more details.
*/

#include "pch.h"
#include "config.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4456)
# if (CRYPTOPP_MSC_VERSION >= 1400)
#  pragma warning(disable: 6246)
# endif
#endif

#include "camellia.h"
#include "misc.h"
#include "cpu.h"

NAMESPACE_BEGIN(CryptoPP)

// round implementation that uses a small table for protection against timing attacks
#define SLOW_ROUND(lh, ll, rh, rl, kh, kl)	{							\
	word32 zr = ll ^ kl;												\
	word32 zl = lh ^ kh;												\
	zr=	rotlFixed(s1[GETBYTE(zr, 3)], 1) |								\
		(rotrFixed(s1[GETBYTE(zr, 2)], 1) << 24) |						\
		(s1[rotlFixed(CRYPTOPP_GET_BYTE_AS_BYTE(zr, 1),1)] << 16) |		\
		(s1[GETBYTE(zr, 0)] << 8);										\
	zl=	(s1[GETBYTE(zl, 3)] << 24) |									\
		(rotlFixed(s1[GETBYTE(zl, 2)], 1) << 16) |						\
		(rotrFixed(s1[GETBYTE(zl, 1)], 1) << 8) |						\
		s1[rotlFixed(CRYPTOPP_GET_BYTE_AS_BYTE(zl, 0), 1)];				\
	zl ^= zr;															\
	zr = zl ^ rotlFixed(zr, 8);											\
	zl = zr ^ rotrFixed(zl, 8);											\
	rh ^= rotlFixed(zr, 16);											\
	rh ^= zl;															\
	rl ^= rotlFixed(zl, 8);												\
	}

// normal round - same output as above but using larger tables for faster speed
#define ROUND(lh, ll, rh, rl, kh, kl)	{	\
	word32 th = lh ^ kh;					\
	word32 tl = ll ^ kl;					\
	word32 d = SP[0][GETBYTE(tl,0)] ^ SP[1][GETBYTE(tl,3)] ^ SP[2][GETBYTE(tl,2)] ^ SP[3][GETBYTE(tl,1)];	\
	word32 u = SP[0][GETBYTE(th,3)] ^ SP[1][GETBYTE(th,2)] ^ SP[2][GETBYTE(th,1)] ^ SP[3][GETBYTE(th,0)];	\
	d ^= u;									\
	rh ^= d;								\
	rl ^= d;								\
	rl ^= rotrFixed(u, 8);}

#define DOUBLE_ROUND(lh, ll, rh, rl, k0, k1, k2, k3)	\
	ROUND(lh, ll, rh, rl, k0, k1)						\
	ROUND(rh, rl, lh, ll, k2, k3)

#ifdef IS_LITTLE_ENDIAN
#define EFI(i) (1-(i))
#else
#define EFI(i) (i)
#endif

void Camellia::Base::UncheckedSetKey(const byte *key, unsigned int keylen, const NameValuePairs &)
{
	m_rounds = (keylen >= 24) ? 4 : 3;
	unsigned int kslen = (8 * m_rounds + 2);
	m_key.New(kslen*2);
	word32 *ks32 = m_key.data();
	int m=0, a=0;
	if (!IsForwardTransformation())
		m = -1, a = kslen-1;

	word32 kl0, kl1, kl2, kl3;
	GetBlock<word32, BigEndian> getBlock(key);
	getBlock(kl0)(kl1)(kl2)(kl3);
	word32 k0=kl0, k1=kl1, k2=kl2, k3=kl3;

#define CALC_ADDR2(base, i, j)	((byte *)(base)+8*(i)+4*(j)+((-16*(i))&m))
#define CALC_ADDR(base, i)	CALC_ADDR2(base, i, 0)

#if 1
	word64 kwl, kwr;
	ks32 += 2*a;
#define PREPARE_KS_ROUNDS			\
	kwl = (word64(k0) << 32) | k1;	\
	kwr = (word64(k2) << 32) | k3
#define KS_ROUND_0(i)							\
	CRYPTOPP_ASSERT(IsAlignedOn(CALC_ADDR(ks32, i+EFI(0)),GetAlignmentOf<word64>()));	\
	CRYPTOPP_ASSERT(IsAlignedOn(CALC_ADDR(ks32, i+EFI(1)),GetAlignmentOf<word64>()));	\
	*(word64*)(void*)CALC_ADDR(ks32, i+EFI(0)) = kwl;	\
	*(word64*)(void*)CALC_ADDR(ks32, i+EFI(1)) = kwr
#define KS_ROUND(i, r, which)																						\
	CRYPTOPP_ASSERT(IsAlignedOn(CALC_ADDR(ks32, i+EFI(r<64)),GetAlignmentOf<word64>()));	\
	CRYPTOPP_ASSERT(IsAlignedOn(CALC_ADDR(ks32, i+EFI(r>64)),GetAlignmentOf<word64>()));	\
	if (which & (1<<int(r<64))) *(word64*)(void*)CALC_ADDR(ks32, i+EFI(r<64)) = (kwr << (r%64)) | (kwl >> (64 - (r%64)));	\
	if (which & (1<<int(r>64))) *(word64*)(void*)CALC_ADDR(ks32, i+EFI(r>64)) = (kwl << (r%64)) | (kwr >> (64 - (r%64)))
#else
	// SSE2 version is 30% faster on Intel Core 2. Doesn't seem worth the hassle of maintenance, but left here
	// #if'd out in case someone needs it.
	__m128i kw, kw2;
	__m128i *ks128 = (__m128i *)ks32+a/2;
	ks32 += 2*a;
#define PREPARE_KS_ROUNDS													\
	kw = _mm_set_epi32(k0, k1, k2, k3);										\
	if (m) kw2 = kw, kw = _mm_shuffle_epi32(kw, _MM_SHUFFLE(1, 0, 3, 2));	\
	else kw2 = _mm_shuffle_epi32(kw, _MM_SHUFFLE(1, 0, 3, 2))
#define KS_ROUND_0(i)										\
	_mm_store_si128((__m128i *)CALC_ADDR(ks128, i), kw)
#define KS_ROUND(i, r, which)	{																				\
	__m128i temp;																								\
	if (r<64 && (which!=1 || m)) temp = _mm_or_si128(_mm_slli_epi64(kw, r%64), _mm_srli_epi64(kw2, 64-r%64));	\
	else temp = _mm_or_si128(_mm_slli_epi64(kw2, r%64), _mm_srli_epi64(kw, 64-r%64));							\
	if (which & 2) _mm_store_si128((__m128i *)CALC_ADDR(ks128, i), temp);										\
	else _mm_storel_epi64((__m128i*)CALC_ADDR(ks32, i+EFI(0)), temp);											\
	}
#endif

	if (keylen == 16)
	{
		// KL
		PREPARE_KS_ROUNDS;
		KS_ROUND_0(0);
		KS_ROUND(4, 15, 3);
		KS_ROUND(10, 45, 3);
		KS_ROUND(12, 60, 2);
		KS_ROUND(16, 77, 3);
		KS_ROUND(18, 94, 3);
		KS_ROUND(22, 111, 3);

		// KA
		k0=kl0, k1=kl1, k2=kl2, k3=kl3;
		DOUBLE_ROUND(k0, k1, k2, k3, 0xA09E667Ful, 0x3BCC908Bul, 0xB67AE858ul, 0x4CAA73B2ul);
		k0^=kl0, k1^=kl1, k2^=kl2, k3^=kl3;
		DOUBLE_ROUND(k0, k1, k2, k3, 0xC6EF372Ful, 0xE94F82BEul, 0x54FF53A5ul, 0xF1D36F1Cul);

		PREPARE_KS_ROUNDS;
		KS_ROUND_0(2);
		KS_ROUND(6, 15, 3);
		KS_ROUND(8, 30, 3);
		KS_ROUND(12, 45, 1);
		KS_ROUND(14, 60, 3);
		KS_ROUND(20, 94, 3);
		KS_ROUND(24, 47, 3);
	}
	else
	{
		// KL
		PREPARE_KS_ROUNDS;
		KS_ROUND_0(0);
		KS_ROUND(12, 45, 3);
		KS_ROUND(16, 60, 3);
		KS_ROUND(22, 77, 3);
		KS_ROUND(30, 111, 3);

		// KR
		word32 kr0, kr1, kr2, kr3;
		GetBlock<word32, BigEndian>(key+16)(kr0)(kr1);
		if (keylen == 24)
			kr2 = ~kr0, kr3 = ~kr1;
		else
			GetBlock<word32, BigEndian>(key+24)(kr2)(kr3);
		k0=kr0, k1=kr1, k2=kr2, k3=kr3;

		PREPARE_KS_ROUNDS;
		KS_ROUND(4, 15, 3);
		KS_ROUND(8, 30, 3);
		KS_ROUND(18, 60, 3);
		KS_ROUND(26, 94, 3);

		// KA
		k0^=kl0, k1^=kl1, k2^=kl2, k3^=kl3;
		DOUBLE_ROUND(k0, k1, k2, k3, 0xA09E667Ful, 0x3BCC908Bul, 0xB67AE858ul, 0x4CAA73B2ul);
		k0^=kl0, k1^=kl1, k2^=kl2, k3^=kl3;
		DOUBLE_ROUND(k0, k1, k2, k3, 0xC6EF372Ful, 0xE94F82BEul, 0x54FF53A5ul, 0xF1D36F1Cul);

		PREPARE_KS_ROUNDS;
		KS_ROUND(6, 15, 3);
		KS_ROUND(14, 45, 3);
		KS_ROUND(24, 77, 3);
		KS_ROUND(28, 94, 3);

		// KB
		k0^=kr0, k1^=kr1, k2^=kr2, k3^=kr3;
		DOUBLE_ROUND(k0, k1, k2, k3, 0x10E527FAul, 0xDE682D1Dul, 0xB05688C2ul, 0xB3E6C1FDul);

		PREPARE_KS_ROUNDS;
		KS_ROUND_0(2);
		KS_ROUND(10, 30, 3);
		KS_ROUND(20, 60, 3);
		KS_ROUND(32, 47, 3);
	}
}

void Camellia::Base::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
#define KS(i, j) ks[i*4 + EFI(j/2)*2 + EFI(j%2)]

#define FL(klh, kll, krh, krl)		\
	ll ^= rotlFixed(lh & klh, 1);	\
	lh ^= (ll | kll);				\
	rh ^= (rl | krl);				\
	rl ^= rotlFixed(rh & krh, 1);

	word32 lh, ll, rh, rl;
	typedef BlockGetAndPut<word32, BigEndian> Block;
	Block::Get(inBlock)(lh)(ll)(rh)(rl);
	const word32 *ks = m_key.data();
	lh ^= KS(0,0);
	ll ^= KS(0,1);
	rh ^= KS(0,2);
	rl ^= KS(0,3);

	// timing attack countermeasure. see comments at top for more details
	const int cacheLineSize = GetCacheLineSize();
	unsigned int i;
	volatile word32 _u = 0;
	word32 u = _u;

	CRYPTOPP_ASSERT(IsAlignedOn(s1,GetAlignmentOf<word32>()));
	for (i=0; i<256; i+=cacheLineSize)
		u &= *(const word32 *)(void*)(s1+i);
	u &= *(const word32 *)(void*)(s1+252);
	lh |= u; ll |= u;

	SLOW_ROUND(lh, ll, rh, rl, KS(1,0), KS(1,1))
	SLOW_ROUND(rh, rl, lh, ll, KS(1,2), KS(1,3))
	for (i = m_rounds-1; i > 0; --i)
	{
		DOUBLE_ROUND(lh, ll, rh, rl, KS(2,0), KS(2,1), KS(2,2), KS(2,3))
		DOUBLE_ROUND(lh, ll, rh, rl, KS(3,0), KS(3,1), KS(3,2), KS(3,3))
		FL(KS(4,0), KS(4,1), KS(4,2), KS(4,3));
		DOUBLE_ROUND(lh, ll, rh, rl, KS(5,0), KS(5,1), KS(5,2), KS(5,3))
		ks += 16;
	}
	DOUBLE_ROUND(lh, ll, rh, rl, KS(2,0), KS(2,1), KS(2,2), KS(2,3))
	ROUND(lh, ll, rh, rl, KS(3,0), KS(3,1))
	SLOW_ROUND(rh, rl, lh, ll, KS(3,2), KS(3,3))
	lh ^= KS(4,0);
	ll ^= KS(4,1);
	rh ^= KS(4,2);
	rl ^= KS(4,3);
	Block::Put(xorBlock, outBlock)(rh)(rl)(lh)(ll);
}

// The Camellia s-boxes

CRYPTOPP_ALIGN_DATA(4)
const byte Camellia::Base::s1[256] =
{
	112,130,44,236,179,39,192,229,228,133,87,53,234,12,174,65,
	35,239,107,147,69,25,165,33,237,14,79,78,29,101,146,189,
	134,184,175,143,124,235,31,206,62,48,220,95,94,197,11,26,
	166,225,57,202,213,71,93,61,217,1,90,214,81,86,108,77,
	139,13,154,102,251,204,176,45,116,18,43,32,240,177,132,153,
	223,76,203,194,52,126,118,5,109,183,169,49,209,23,4,215,
	20,88,58,97,222,27,17,28,50,15,156,22,83,24,242,34,
	254,68,207,178,195,181,122,145,36,8,232,168,96,252,105,80,
	170,208,160,125,161,137,98,151,84,91,30,149,224,255,100,210,
	16,196,0,72,163,247,117,219,138,3,230,218,9,63,221,148,
	135,92,131,2,205,74,144,51,115,103,246,243,157,127,191,226,
	82,155,216,38,200,55,198,59,129,150,111,75,19,190,99,46,
	233,121,167,140,159,110,188,142,41,245,249,182,47,253,180,89,
	120,152,6,106,231,70,113,186,212,37,171,66,136,162,141,250,
	114,7,185,85,248,238,172,10,54,73,42,104,60,56,241,164,
	64,40,211,123,187,201,67,193,21,227,173,244,119,199,128,158
};

const word32 Camellia::Base::SP[4][256] = {
	{
	0x70707000, 0x82828200, 0x2c2c2c00, 0xececec00,
	0xb3b3b300, 0x27272700, 0xc0c0c000, 0xe5e5e500,
	0xe4e4e400, 0x85858500, 0x57575700, 0x35353500,
	0xeaeaea00, 0x0c0c0c00, 0xaeaeae00, 0x41414100,
	0x23232300, 0xefefef00, 0x6b6b6b00, 0x93939300,
	0x45454500, 0x19191900, 0xa5a5a500, 0x21212100,
	0xededed00, 0x0e0e0e00, 0x4f4f4f00, 0x4e4e4e00,
	0x1d1d1d00, 0x65656500, 0x92929200, 0xbdbdbd00,
	0x86868600, 0xb8b8b800, 0xafafaf00, 0x8f8f8f00,
	0x7c7c7c00, 0xebebeb00, 0x1f1f1f00, 0xcecece00,
	0x3e3e3e00, 0x30303000, 0xdcdcdc00, 0x5f5f5f00,
	0x5e5e5e00, 0xc5c5c500, 0x0b0b0b00, 0x1a1a1a00,
	0xa6a6a600, 0xe1e1e100, 0x39393900, 0xcacaca00,
	0xd5d5d500, 0x47474700, 0x5d5d5d00, 0x3d3d3d00,
	0xd9d9d900, 0x01010100, 0x5a5a5a00, 0xd6d6d600,
	0x51515100, 0x56565600, 0x6c6c6c00, 0x4d4d4d00,
	0x8b8b8b00, 0x0d0d0d00, 0x9a9a9a00, 0x66666600,
	0xfbfbfb00, 0xcccccc00, 0xb0b0b000, 0x2d2d2d00,
	0x74747400, 0x12121200, 0x2b2b2b00, 0x20202000,
	0xf0f0f000, 0xb1b1b100, 0x84848400, 0x99999900,
	0xdfdfdf00, 0x4c4c4c00, 0xcbcbcb00, 0xc2c2c200,
	0x34343400, 0x7e7e7e00, 0x76767600, 0x05050500,
	0x6d6d6d00, 0xb7b7b700, 0xa9a9a900, 0x31313100,
	0xd1d1d100, 0x17171700, 0x04040400, 0xd7d7d700,
	0x14141400, 0x58585800, 0x3a3a3a00, 0x61616100,
	0xdedede00, 0x1b1b1b00, 0x11111100, 0x1c1c1c00,
	0x32323200, 0x0f0f0f00, 0x9c9c9c00, 0x16161600,
	0x53535300, 0x18181800, 0xf2f2f200, 0x22222200,
	0xfefefe00, 0x44444400, 0xcfcfcf00, 0xb2b2b200,
	0xc3c3c300, 0xb5b5b500, 0x7a7a7a00, 0x91919100,
	0x24242400, 0x08080800, 0xe8e8e800, 0xa8a8a800,
	0x60606000, 0xfcfcfc00, 0x69696900, 0x50505000,
	0xaaaaaa00, 0xd0d0d000, 0xa0a0a000, 0x7d7d7d00,
	0xa1a1a100, 0x89898900, 0x62626200, 0x97979700,
	0x54545400, 0x5b5b5b00, 0x1e1e1e00, 0x95959500,
	0xe0e0e000, 0xffffff00, 0x64646400, 0xd2d2d200,
	0x10101000, 0xc4c4c400, 0x00000000, 0x48484800,
	0xa3a3a300, 0xf7f7f700, 0x75757500, 0xdbdbdb00,
	0x8a8a8a00, 0x03030300, 0xe6e6e600, 0xdadada00,
	0x09090900, 0x3f3f3f00, 0xdddddd00, 0x94949400,
	0x87878700, 0x5c5c5c00, 0x83838300, 0x02020200,
	0xcdcdcd00, 0x4a4a4a00, 0x90909000, 0x33333300,
	0x73737300, 0x67676700, 0xf6f6f600, 0xf3f3f300,
	0x9d9d9d00, 0x7f7f7f00, 0xbfbfbf00, 0xe2e2e200,
	0x52525200, 0x9b9b9b00, 0xd8d8d800, 0x26262600,
	0xc8c8c800, 0x37373700, 0xc6c6c600, 0x3b3b3b00,
	0x81818100, 0x96969600, 0x6f6f6f00, 0x4b4b4b00,
	0x13131300, 0xbebebe00, 0x63636300, 0x2e2e2e00,
	0xe9e9e900, 0x79797900, 0xa7a7a700, 0x8c8c8c00,
	0x9f9f9f00, 0x6e6e6e00, 0xbcbcbc00, 0x8e8e8e00,
	0x29292900, 0xf5f5f500, 0xf9f9f900, 0xb6b6b600,
	0x2f2f2f00, 0xfdfdfd00, 0xb4b4b400, 0x59595900,
	0x78787800, 0x98989800, 0x06060600, 0x6a6a6a00,
	0xe7e7e700, 0x46464600, 0x71717100, 0xbababa00,
	0xd4d4d400, 0x25252500, 0xababab00, 0x42424200,
	0x88888800, 0xa2a2a200, 0x8d8d8d00, 0xfafafa00,
	0x72727200, 0x07070700, 0xb9b9b900, 0x55555500,
	0xf8f8f800, 0xeeeeee00, 0xacacac00, 0x0a0a0a00,
	0x36363600, 0x49494900, 0x2a2a2a00, 0x68686800,
	0x3c3c3c00, 0x38383800, 0xf1f1f100, 0xa4a4a400,
	0x40404000, 0x28282800, 0xd3d3d300, 0x7b7b7b00,
	0xbbbbbb00, 0xc9c9c900, 0x43434300, 0xc1c1c100,
	0x15151500, 0xe3e3e300, 0xadadad00, 0xf4f4f400,
	0x77777700, 0xc7c7c700, 0x80808000, 0x9e9e9e00
	},
	{
	0x00e0e0e0, 0x00050505, 0x00585858, 0x00d9d9d9,
	0x00676767, 0x004e4e4e, 0x00818181, 0x00cbcbcb,
	0x00c9c9c9, 0x000b0b0b, 0x00aeaeae, 0x006a6a6a,
	0x00d5d5d5, 0x00181818, 0x005d5d5d, 0x00828282,
	0x00464646, 0x00dfdfdf, 0x00d6d6d6, 0x00272727,
	0x008a8a8a, 0x00323232, 0x004b4b4b, 0x00424242,
	0x00dbdbdb, 0x001c1c1c, 0x009e9e9e, 0x009c9c9c,
	0x003a3a3a, 0x00cacaca, 0x00252525, 0x007b7b7b,
	0x000d0d0d, 0x00717171, 0x005f5f5f, 0x001f1f1f,
	0x00f8f8f8, 0x00d7d7d7, 0x003e3e3e, 0x009d9d9d,
	0x007c7c7c, 0x00606060, 0x00b9b9b9, 0x00bebebe,
	0x00bcbcbc, 0x008b8b8b, 0x00161616, 0x00343434,
	0x004d4d4d, 0x00c3c3c3, 0x00727272, 0x00959595,
	0x00ababab, 0x008e8e8e, 0x00bababa, 0x007a7a7a,
	0x00b3b3b3, 0x00020202, 0x00b4b4b4, 0x00adadad,
	0x00a2a2a2, 0x00acacac, 0x00d8d8d8, 0x009a9a9a,
	0x00171717, 0x001a1a1a, 0x00353535, 0x00cccccc,
	0x00f7f7f7, 0x00999999, 0x00616161, 0x005a5a5a,
	0x00e8e8e8, 0x00242424, 0x00565656, 0x00404040,
	0x00e1e1e1, 0x00636363, 0x00090909, 0x00333333,
	0x00bfbfbf, 0x00989898, 0x00979797, 0x00858585,
	0x00686868, 0x00fcfcfc, 0x00ececec, 0x000a0a0a,
	0x00dadada, 0x006f6f6f, 0x00535353, 0x00626262,
	0x00a3a3a3, 0x002e2e2e, 0x00080808, 0x00afafaf,
	0x00282828, 0x00b0b0b0, 0x00747474, 0x00c2c2c2,
	0x00bdbdbd, 0x00363636, 0x00222222, 0x00383838,
	0x00646464, 0x001e1e1e, 0x00393939, 0x002c2c2c,
	0x00a6a6a6, 0x00303030, 0x00e5e5e5, 0x00444444,
	0x00fdfdfd, 0x00888888, 0x009f9f9f, 0x00656565,
	0x00878787, 0x006b6b6b, 0x00f4f4f4, 0x00232323,
	0x00484848, 0x00101010, 0x00d1d1d1, 0x00515151,
	0x00c0c0c0, 0x00f9f9f9, 0x00d2d2d2, 0x00a0a0a0,
	0x00555555, 0x00a1a1a1, 0x00414141, 0x00fafafa,
	0x00434343, 0x00131313, 0x00c4c4c4, 0x002f2f2f,
	0x00a8a8a8, 0x00b6b6b6, 0x003c3c3c, 0x002b2b2b,
	0x00c1c1c1, 0x00ffffff, 0x00c8c8c8, 0x00a5a5a5,
	0x00202020, 0x00898989, 0x00000000, 0x00909090,
	0x00474747, 0x00efefef, 0x00eaeaea, 0x00b7b7b7,
	0x00151515, 0x00060606, 0x00cdcdcd, 0x00b5b5b5,
	0x00121212, 0x007e7e7e, 0x00bbbbbb, 0x00292929,
	0x000f0f0f, 0x00b8b8b8, 0x00070707, 0x00040404,
	0x009b9b9b, 0x00949494, 0x00212121, 0x00666666,
	0x00e6e6e6, 0x00cecece, 0x00ededed, 0x00e7e7e7,
	0x003b3b3b, 0x00fefefe, 0x007f7f7f, 0x00c5c5c5,
	0x00a4a4a4, 0x00373737, 0x00b1b1b1, 0x004c4c4c,
	0x00919191, 0x006e6e6e, 0x008d8d8d, 0x00767676,
	0x00030303, 0x002d2d2d, 0x00dedede, 0x00969696,
	0x00262626, 0x007d7d7d, 0x00c6c6c6, 0x005c5c5c,
	0x00d3d3d3, 0x00f2f2f2, 0x004f4f4f, 0x00191919,
	0x003f3f3f, 0x00dcdcdc, 0x00797979, 0x001d1d1d,
	0x00525252, 0x00ebebeb, 0x00f3f3f3, 0x006d6d6d,
	0x005e5e5e, 0x00fbfbfb, 0x00696969, 0x00b2b2b2,
	0x00f0f0f0, 0x00313131, 0x000c0c0c, 0x00d4d4d4,
	0x00cfcfcf, 0x008c8c8c, 0x00e2e2e2, 0x00757575,
	0x00a9a9a9, 0x004a4a4a, 0x00575757, 0x00848484,
	0x00111111, 0x00454545, 0x001b1b1b, 0x00f5f5f5,
	0x00e4e4e4, 0x000e0e0e, 0x00737373, 0x00aaaaaa,
	0x00f1f1f1, 0x00dddddd, 0x00595959, 0x00141414,
	0x006c6c6c, 0x00929292, 0x00545454, 0x00d0d0d0,
	0x00787878, 0x00707070, 0x00e3e3e3, 0x00494949,
	0x00808080, 0x00505050, 0x00a7a7a7, 0x00f6f6f6,
	0x00777777, 0x00939393, 0x00868686, 0x00838383,
	0x002a2a2a, 0x00c7c7c7, 0x005b5b5b, 0x00e9e9e9,
	0x00eeeeee, 0x008f8f8f, 0x00010101, 0x003d3d3d
	},
	{
	0x38003838, 0x41004141, 0x16001616, 0x76007676,
	0xd900d9d9, 0x93009393, 0x60006060, 0xf200f2f2,
	0x72007272, 0xc200c2c2, 0xab00abab, 0x9a009a9a,
	0x75007575, 0x06000606, 0x57005757, 0xa000a0a0,
	0x91009191, 0xf700f7f7, 0xb500b5b5, 0xc900c9c9,
	0xa200a2a2, 0x8c008c8c, 0xd200d2d2, 0x90009090,
	0xf600f6f6, 0x07000707, 0xa700a7a7, 0x27002727,
	0x8e008e8e, 0xb200b2b2, 0x49004949, 0xde00dede,
	0x43004343, 0x5c005c5c, 0xd700d7d7, 0xc700c7c7,
	0x3e003e3e, 0xf500f5f5, 0x8f008f8f, 0x67006767,
	0x1f001f1f, 0x18001818, 0x6e006e6e, 0xaf00afaf,
	0x2f002f2f, 0xe200e2e2, 0x85008585, 0x0d000d0d,
	0x53005353, 0xf000f0f0, 0x9c009c9c, 0x65006565,
	0xea00eaea, 0xa300a3a3, 0xae00aeae, 0x9e009e9e,
	0xec00ecec, 0x80008080, 0x2d002d2d, 0x6b006b6b,
	0xa800a8a8, 0x2b002b2b, 0x36003636, 0xa600a6a6,
	0xc500c5c5, 0x86008686, 0x4d004d4d, 0x33003333,
	0xfd00fdfd, 0x66006666, 0x58005858, 0x96009696,
	0x3a003a3a, 0x09000909, 0x95009595, 0x10001010,
	0x78007878, 0xd800d8d8, 0x42004242, 0xcc00cccc,
	0xef00efef, 0x26002626, 0xe500e5e5, 0x61006161,
	0x1a001a1a, 0x3f003f3f, 0x3b003b3b, 0x82008282,
	0xb600b6b6, 0xdb00dbdb, 0xd400d4d4, 0x98009898,
	0xe800e8e8, 0x8b008b8b, 0x02000202, 0xeb00ebeb,
	0x0a000a0a, 0x2c002c2c, 0x1d001d1d, 0xb000b0b0,
	0x6f006f6f, 0x8d008d8d, 0x88008888, 0x0e000e0e,
	0x19001919, 0x87008787, 0x4e004e4e, 0x0b000b0b,
	0xa900a9a9, 0x0c000c0c, 0x79007979, 0x11001111,
	0x7f007f7f, 0x22002222, 0xe700e7e7, 0x59005959,
	0xe100e1e1, 0xda00dada, 0x3d003d3d, 0xc800c8c8,
	0x12001212, 0x04000404, 0x74007474, 0x54005454,
	0x30003030, 0x7e007e7e, 0xb400b4b4, 0x28002828,
	0x55005555, 0x68006868, 0x50005050, 0xbe00bebe,
	0xd000d0d0, 0xc400c4c4, 0x31003131, 0xcb00cbcb,
	0x2a002a2a, 0xad00adad, 0x0f000f0f, 0xca00caca,
	0x70007070, 0xff00ffff, 0x32003232, 0x69006969,
	0x08000808, 0x62006262, 0x00000000, 0x24002424,
	0xd100d1d1, 0xfb00fbfb, 0xba00baba, 0xed00eded,
	0x45004545, 0x81008181, 0x73007373, 0x6d006d6d,
	0x84008484, 0x9f009f9f, 0xee00eeee, 0x4a004a4a,
	0xc300c3c3, 0x2e002e2e, 0xc100c1c1, 0x01000101,
	0xe600e6e6, 0x25002525, 0x48004848, 0x99009999,
	0xb900b9b9, 0xb300b3b3, 0x7b007b7b, 0xf900f9f9,
	0xce00cece, 0xbf00bfbf, 0xdf00dfdf, 0x71007171,
	0x29002929, 0xcd00cdcd, 0x6c006c6c, 0x13001313,
	0x64006464, 0x9b009b9b, 0x63006363, 0x9d009d9d,
	0xc000c0c0, 0x4b004b4b, 0xb700b7b7, 0xa500a5a5,
	0x89008989, 0x5f005f5f, 0xb100b1b1, 0x17001717,
	0xf400f4f4, 0xbc00bcbc, 0xd300d3d3, 0x46004646,
	0xcf00cfcf, 0x37003737, 0x5e005e5e, 0x47004747,
	0x94009494, 0xfa00fafa, 0xfc00fcfc, 0x5b005b5b,
	0x97009797, 0xfe00fefe, 0x5a005a5a, 0xac00acac,
	0x3c003c3c, 0x4c004c4c, 0x03000303, 0x35003535,
	0xf300f3f3, 0x23002323, 0xb800b8b8, 0x5d005d5d,
	0x6a006a6a, 0x92009292, 0xd500d5d5, 0x21002121,
	0x44004444, 0x51005151, 0xc600c6c6, 0x7d007d7d,
	0x39003939, 0x83008383, 0xdc00dcdc, 0xaa00aaaa,
	0x7c007c7c, 0x77007777, 0x56005656, 0x05000505,
	0x1b001b1b, 0xa400a4a4, 0x15001515, 0x34003434,
	0x1e001e1e, 0x1c001c1c, 0xf800f8f8, 0x52005252,
	0x20002020, 0x14001414, 0xe900e9e9, 0xbd00bdbd,
	0xdd00dddd, 0xe400e4e4, 0xa100a1a1, 0xe000e0e0,
	0x8a008a8a, 0xf100f1f1, 0xd600d6d6, 0x7a007a7a,
	0xbb00bbbb, 0xe300e3e3, 0x40004040, 0x4f004f4f
	},
	{
	0x70700070, 0x2c2c002c, 0xb3b300b3, 0xc0c000c0,
	0xe4e400e4, 0x57570057, 0xeaea00ea, 0xaeae00ae,
	0x23230023, 0x6b6b006b, 0x45450045, 0xa5a500a5,
	0xeded00ed, 0x4f4f004f, 0x1d1d001d, 0x92920092,
	0x86860086, 0xafaf00af, 0x7c7c007c, 0x1f1f001f,
	0x3e3e003e, 0xdcdc00dc, 0x5e5e005e, 0x0b0b000b,
	0xa6a600a6, 0x39390039, 0xd5d500d5, 0x5d5d005d,
	0xd9d900d9, 0x5a5a005a, 0x51510051, 0x6c6c006c,
	0x8b8b008b, 0x9a9a009a, 0xfbfb00fb, 0xb0b000b0,
	0x74740074, 0x2b2b002b, 0xf0f000f0, 0x84840084,
	0xdfdf00df, 0xcbcb00cb, 0x34340034, 0x76760076,
	0x6d6d006d, 0xa9a900a9, 0xd1d100d1, 0x04040004,
	0x14140014, 0x3a3a003a, 0xdede00de, 0x11110011,
	0x32320032, 0x9c9c009c, 0x53530053, 0xf2f200f2,
	0xfefe00fe, 0xcfcf00cf, 0xc3c300c3, 0x7a7a007a,
	0x24240024, 0xe8e800e8, 0x60600060, 0x69690069,
	0xaaaa00aa, 0xa0a000a0, 0xa1a100a1, 0x62620062,
	0x54540054, 0x1e1e001e, 0xe0e000e0, 0x64640064,
	0x10100010, 0x00000000, 0xa3a300a3, 0x75750075,
	0x8a8a008a, 0xe6e600e6, 0x09090009, 0xdddd00dd,
	0x87870087, 0x83830083, 0xcdcd00cd, 0x90900090,
	0x73730073, 0xf6f600f6, 0x9d9d009d, 0xbfbf00bf,
	0x52520052, 0xd8d800d8, 0xc8c800c8, 0xc6c600c6,
	0x81810081, 0x6f6f006f, 0x13130013, 0x63630063,
	0xe9e900e9, 0xa7a700a7, 0x9f9f009f, 0xbcbc00bc,
	0x29290029, 0xf9f900f9, 0x2f2f002f, 0xb4b400b4,
	0x78780078, 0x06060006, 0xe7e700e7, 0x71710071,
	0xd4d400d4, 0xabab00ab, 0x88880088, 0x8d8d008d,
	0x72720072, 0xb9b900b9, 0xf8f800f8, 0xacac00ac,
	0x36360036, 0x2a2a002a, 0x3c3c003c, 0xf1f100f1,
	0x40400040, 0xd3d300d3, 0xbbbb00bb, 0x43430043,
	0x15150015, 0xadad00ad, 0x77770077, 0x80800080,
	0x82820082, 0xecec00ec, 0x27270027, 0xe5e500e5,
	0x85850085, 0x35350035, 0x0c0c000c, 0x41410041,
	0xefef00ef, 0x93930093, 0x19190019, 0x21210021,
	0x0e0e000e, 0x4e4e004e, 0x65650065, 0xbdbd00bd,
	0xb8b800b8, 0x8f8f008f, 0xebeb00eb, 0xcece00ce,
	0x30300030, 0x5f5f005f, 0xc5c500c5, 0x1a1a001a,
	0xe1e100e1, 0xcaca00ca, 0x47470047, 0x3d3d003d,
	0x01010001, 0xd6d600d6, 0x56560056, 0x4d4d004d,
	0x0d0d000d, 0x66660066, 0xcccc00cc, 0x2d2d002d,
	0x12120012, 0x20200020, 0xb1b100b1, 0x99990099,
	0x4c4c004c, 0xc2c200c2, 0x7e7e007e, 0x05050005,
	0xb7b700b7, 0x31310031, 0x17170017, 0xd7d700d7,
	0x58580058, 0x61610061, 0x1b1b001b, 0x1c1c001c,
	0x0f0f000f, 0x16160016, 0x18180018, 0x22220022,
	0x44440044, 0xb2b200b2, 0xb5b500b5, 0x91910091,
	0x08080008, 0xa8a800a8, 0xfcfc00fc, 0x50500050,
	0xd0d000d0, 0x7d7d007d, 0x89890089, 0x97970097,
	0x5b5b005b, 0x95950095, 0xffff00ff, 0xd2d200d2,
	0xc4c400c4, 0x48480048, 0xf7f700f7, 0xdbdb00db,
	0x03030003, 0xdada00da, 0x3f3f003f, 0x94940094,
	0x5c5c005c, 0x02020002, 0x4a4a004a, 0x33330033,
	0x67670067, 0xf3f300f3, 0x7f7f007f, 0xe2e200e2,
	0x9b9b009b, 0x26260026, 0x37370037, 0x3b3b003b,
	0x96960096, 0x4b4b004b, 0xbebe00be, 0x2e2e002e,
	0x79790079, 0x8c8c008c, 0x6e6e006e, 0x8e8e008e,
	0xf5f500f5, 0xb6b600b6, 0xfdfd00fd, 0x59590059,
	0x98980098, 0x6a6a006a, 0x46460046, 0xbaba00ba,
	0x25250025, 0x42420042, 0xa2a200a2, 0xfafa00fa,
	0x07070007, 0x55550055, 0xeeee00ee, 0x0a0a000a,
	0x49490049, 0x68680068, 0x38380038, 0xa4a400a4,
	0x28280028, 0x7b7b007b, 0xc9c900c9, 0xc1c100c1,
	0xe3e300e3, 0xf4f400f4, 0xc7c700c7, 0x9e9e009e
	}};

NAMESPACE_END
