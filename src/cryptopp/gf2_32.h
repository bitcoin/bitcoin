#ifndef CRYPTOPP_GF2_32_H
#define CRYPTOPP_GF2_32_H

#include "cryptlib.h"
#include "secblock.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

//! GF(2^32) with polynomial basis
class GF2_32
{
public:
	typedef word32 Element;
	typedef int RandomizationParameter;

	GF2_32(word32 modulus=0x0000008D) : m_modulus(modulus) {}

	Element RandomElement(RandomNumberGenerator &rng, int ignored = 0) const
		{CRYPTOPP_UNUSED(ignored); return rng.GenerateWord32();}

	bool Equal(Element a, Element b) const
		{return a==b;}

	Element Identity() const
		{return 0;}

	Element Add(Element a, Element b) const
		{return a^b;}

	Element& Accumulate(Element &a, Element b) const
		{return a^=b;}

	Element Inverse(Element a) const
		{return a;}

	Element Subtract(Element a, Element b) const
		{return a^b;}

	Element& Reduce(Element &a, Element b) const
		{return a^=b;}

	Element Double(Element a) const
		{CRYPTOPP_UNUSED(a); return 0;}

	Element MultiplicativeIdentity() const
		{return 1;}

	Element Multiply(Element a, Element b) const;

	Element Square(Element a) const
		{return Multiply(a, a);}

	bool IsUnit(Element a) const
		{return a != 0;}

	Element MultiplicativeInverse(Element a) const;

	Element Divide(Element a, Element b) const
		{return Multiply(a, MultiplicativeInverse(b));}

private:
	word32 m_modulus;
};

NAMESPACE_END

#endif
