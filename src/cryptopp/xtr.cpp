// xtr.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#include "xtr.h"
#include "nbtheory.h"
#include "integer.h"
#include "algebra.h"
#include "modarith.h"
#include "algebra.cpp"

NAMESPACE_BEGIN(CryptoPP)

const GFP2Element & GFP2Element::Zero()
{
	return Singleton<GFP2Element>().Ref();
}

void XTR_FindPrimesAndGenerator(RandomNumberGenerator &rng, Integer &p, Integer &q, GFP2Element &g, unsigned int pbits, unsigned int qbits)
{
	CRYPTOPP_ASSERT(qbits > 9);	// no primes exist for pbits = 10, qbits = 9
	CRYPTOPP_ASSERT(pbits > qbits);

	const Integer minQ = Integer::Power2(qbits - 1);
	const Integer maxQ = Integer::Power2(qbits) - 1;
	const Integer minP = Integer::Power2(pbits - 1);
	const Integer maxP = Integer::Power2(pbits) - 1;

	Integer r1, r2;
	do
	{
		bool qFound = q.Randomize(rng, minQ, maxQ, Integer::PRIME, 7, 12);
		CRYPTOPP_UNUSED(qFound); CRYPTOPP_ASSERT(qFound);
		bool solutionsExist = SolveModularQuadraticEquation(r1, r2, 1, -1, 1, q);
		CRYPTOPP_UNUSED(solutionsExist); CRYPTOPP_ASSERT(solutionsExist);
	} while (!p.Randomize(rng, minP, maxP, Integer::PRIME, CRT(rng.GenerateBit()?r1:r2, q, 2, 3, EuclideanMultiplicativeInverse(p, 3)), 3*q));
	CRYPTOPP_ASSERT(((p.Squared() - p + 1) % q).IsZero());

	GFP2_ONB<ModularArithmetic> gfp2(p);
	GFP2Element three = gfp2.ConvertIn(3), t;

	while (true)
	{
		g.c1.Randomize(rng, Integer::Zero(), p-1);
		g.c2.Randomize(rng, Integer::Zero(), p-1);
		t = XTR_Exponentiate(g, p+1, p);
		if (t.c1 == t.c2)
			continue;
		g = XTR_Exponentiate(g, (p.Squared()-p+1)/q, p);
		if (g != three)
			break;
	}
	CRYPTOPP_ASSERT(XTR_Exponentiate(g, q, p) == three);
}

GFP2Element XTR_Exponentiate(const GFP2Element &b, const Integer &e, const Integer &p)
{
	unsigned int bitCount = e.BitCount();
	if (bitCount == 0)
		return GFP2Element(-3, -3);

	// find the lowest bit of e that is 1
	unsigned int lowest1bit;
	for (lowest1bit=0; e.GetBit(lowest1bit) == 0; lowest1bit++) {}

	GFP2_ONB<MontgomeryRepresentation> gfp2(p);
	GFP2Element c = gfp2.ConvertIn(b);
	GFP2Element cp = gfp2.PthPower(c);
	GFP2Element S[5] = {gfp2.ConvertIn(3), c, gfp2.SpecialOperation1(c)};

	// do all exponents bits except the lowest zeros starting from the top
	unsigned int i;
	for (i = e.BitCount() - 1; i>lowest1bit; i--)
	{
		if (e.GetBit(i))
		{
			gfp2.RaiseToPthPower(S[0]);
			gfp2.Accumulate(S[0], gfp2.SpecialOperation2(S[2], c, S[1]));
			S[1] = gfp2.SpecialOperation1(S[1]);
			S[2] = gfp2.SpecialOperation1(S[2]);
			S[0].swap(S[1]);
		}
		else
		{
			gfp2.RaiseToPthPower(S[2]);
			gfp2.Accumulate(S[2], gfp2.SpecialOperation2(S[0], cp, S[1]));
			S[1] = gfp2.SpecialOperation1(S[1]);
			S[0] = gfp2.SpecialOperation1(S[0]);
			S[2].swap(S[1]);
		}
	}

	// now do the lowest zeros
	while (i--)
		S[1] = gfp2.SpecialOperation1(S[1]);

	return gfp2.ConvertOut(S[1]);
}

template class AbstractRing<GFP2Element>;
template class AbstractGroup<GFP2Element>;

NAMESPACE_END
