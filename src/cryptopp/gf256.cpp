// gf256.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "gf256.h"

NAMESPACE_BEGIN(CryptoPP)

GF256::Element GF256::Multiply(Element a, Element b) const
{
	word result = 0, t = b;

	for (unsigned int i=0; i<8; i++)
	{
		result <<= 1;
		if (result & 0x100)
			result ^= m_modulus;

		t <<= 1;
		if (t & 0x100)
			result ^= a;
	}

	return (GF256::Element) result;
}

GF256::Element GF256::MultiplicativeInverse(Element a) const
{
	Element result = a;
	for (int i=1; i<7; i++)
		result = Multiply(Square(result), a);
	return Square(result);
}

NAMESPACE_END
