// gf2_32.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "misc.h"
#include "gf2_32.h"

NAMESPACE_BEGIN(CryptoPP)

GF2_32::Element GF2_32::Multiply(Element a, Element b) const
{
	word32 table[4];
	table[0] = 0;
	table[1] = m_modulus;
	if (a & 0x80000000)
	{
		table[2] = m_modulus ^ (a<<1);
		table[3] = a<<1;
	}
	else
	{
		table[2] = a<<1;
		table[3] = m_modulus ^ (a<<1);
	}

#if CRYPTOPP_FAST_ROTATE(32)
	b = rotrFixed(b, 30U);
	word32 result = table[b&2];

	for (int i=29; i>=0; --i)
	{
		b = rotlFixed(b, 1U);
		result = (result<<1) ^ table[(b&2) + (result>>31)];
	}

	return (b&1) ? result ^ a : result;
#else
	word32 result = table[(b>>30) & 2];

	for (int i=29; i>=0; --i)
		result = (result<<1) ^ table[((b>>i)&2) + (result>>31)];

	return (b&1) ? result ^ a : result;
#endif
}

GF2_32::Element GF2_32::MultiplicativeInverse(Element a) const
{
	if (a <= 1)		// 1 is a special case
		return a;

	// warning - don't try to adapt this algorithm for another situation
	word32 g0=m_modulus, g1=a, g2=a;
	word32 v0=0, v1=1, v2=1;

	CRYPTOPP_ASSERT(g1);

	while (!(g2 & 0x80000000))
	{
		g2 <<= 1;
		v2 <<= 1;
	}

	g2 <<= 1;
	v2 <<= 1;

	g0 ^= g2;
	v0 ^= v2;

	while (g0 != 1)
	{
		if (g1 < g0 || ((g0^g1) < g0 && (g0^g1) < g1))
		{
			CRYPTOPP_ASSERT(BitPrecision(g1) <= BitPrecision(g0));
			g2 = g1;
			v2 = v1;
		}
		else
		{
			CRYPTOPP_ASSERT(BitPrecision(g1) > BitPrecision(g0));
			g2 = g0; g0 = g1; g1 = g2;
			v2 = v0; v0 = v1; v1 = v2;
		}

		while ((g0^g2) >= g2)
		{
			CRYPTOPP_ASSERT(BitPrecision(g0) > BitPrecision(g2));
			g2 <<= 1;
			v2 <<= 1;
		}

		CRYPTOPP_ASSERT(BitPrecision(g0) == BitPrecision(g2));
		g0 ^= g2;
		v0 ^= v2;
	}

	return v0;
}

NAMESPACE_END
