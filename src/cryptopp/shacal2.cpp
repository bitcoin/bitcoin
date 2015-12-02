// shacal2.cpp - by Kevin Springle, 2003
//
// Portions of this code were derived from
// Wei Dai's implementation of SHA-2
//
// The original code and all modifications are in the public domain.

#include "pch.h"
#include "shacal2.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

// SHACAL-2 function and round definitions

#define S0(x) (rotrFixed(x,2)^rotrFixed(x,13)^rotrFixed(x,22))
#define S1(x) (rotrFixed(x,6)^rotrFixed(x,11)^rotrFixed(x,25))
#define s0(x) (rotrFixed(x,7)^rotrFixed(x,18)^(x>>3))
#define s1(x) (rotrFixed(x,17)^rotrFixed(x,19)^(x>>10))

#define Ch(x,y,z) (z^(x&(y^z)))
#define Maj(x,y,z) ((x&y)|(z&(x|y)))

/* R is the SHA-256 round function. */
/* This macro increments the k argument as a side effect. */
#define R(a,b,c,d,e,f,g,h,k) \
	h+=S1(e)+Ch(e,f,g)+*k++;d+=h;h+=S0(a)+Maj(a,b,c);

/* P is the inverse of the SHA-256 round function. */
/* This macro decrements the k argument as a side effect. */
#define P(a,b,c,d,e,f,g,h,k) \
	h-=S0(a)+Maj(a,b,c);d-=h;h-=S1(e)+Ch(e,f,g)+*--k;

void SHACAL2::Base::UncheckedSetKey(const byte *userKey, unsigned int keylen, const NameValuePairs &)
{
	AssertValidKeyLength(keylen);

	word32 *rk = m_key;
	unsigned int i;

	GetUserKey(BIG_ENDIAN_ORDER, rk, m_key.size(), userKey, keylen);
	for (i = 0; i < 48; i++, rk++)
	{
		rk[16] = rk[0] + s0(rk[1]) + rk[9] + s1(rk[14]);
		rk[0] += K[i];
	}
	for (i = 48; i < 64; i++, rk++)
	{
		rk[0] += K[i];
	}
}

typedef BlockGetAndPut<word32, BigEndian> Block;

void SHACAL2::Enc::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word32 a, b, c, d, e, f, g, h;
	const word32 *rk = m_key;

	/*
	 * map byte array block to cipher state:
	 */
	Block::Get(inBlock)(a)(b)(c)(d)(e)(f)(g)(h);

	// Perform SHA-256 transformation.

	/* 64 operations, partially loop unrolled */
	for (unsigned int j=0; j<64; j+=8)
	{
		R(a,b,c,d,e,f,g,h,rk);
		R(h,a,b,c,d,e,f,g,rk);
		R(g,h,a,b,c,d,e,f,rk);
		R(f,g,h,a,b,c,d,e,rk);
		R(e,f,g,h,a,b,c,d,rk);
		R(d,e,f,g,h,a,b,c,rk);
		R(c,d,e,f,g,h,a,b,rk);
		R(b,c,d,e,f,g,h,a,rk);
	}

	/*
	 * map cipher state to byte array block:
	 */

	Block::Put(xorBlock, outBlock)(a)(b)(c)(d)(e)(f)(g)(h);
}

void SHACAL2::Dec::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word32 a, b, c, d, e, f, g, h;
	const word32 *rk = m_key + 64;

	/*
	 * map byte array block to cipher state:
	 */
	Block::Get(inBlock)(a)(b)(c)(d)(e)(f)(g)(h);

	// Perform inverse SHA-256 transformation.

	/* 64 operations, partially loop unrolled */
	for (unsigned int j=0; j<64; j+=8)
	{
		P(b,c,d,e,f,g,h,a,rk);
		P(c,d,e,f,g,h,a,b,rk);
		P(d,e,f,g,h,a,b,c,rk);
		P(e,f,g,h,a,b,c,d,rk);
		P(f,g,h,a,b,c,d,e,rk);
		P(g,h,a,b,c,d,e,f,rk);
		P(h,a,b,c,d,e,f,g,rk);
		P(a,b,c,d,e,f,g,h,rk);
	}

	/*
	 * map cipher state to byte array block:
	 */

	Block::Put(xorBlock, outBlock)(a)(b)(c)(d)(e)(f)(g)(h);
}

// The SHACAL-2 round constants are identical to the SHA-256 round constants.
const word32 SHACAL2::Base::K[64] =
{
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
	0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
	0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
	0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
	0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

NAMESPACE_END
