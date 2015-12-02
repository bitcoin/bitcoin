// dsa.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "config.h"

// TODO: fix the C4589 warnings
#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4189 4589)
#endif

#ifndef CRYPTOPP_IMPORTS

#include "gfpcrypt.h"
#include "nbtheory.h"
#include "modarith.h"
#include "integer.h"
#include "asn.h"
#include "oids.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

#if !defined(NDEBUG) && !defined(CRYPTOPP_DOXYGEN_PROCESSING)
void TestInstantiations_gfpcrypt()
{
	GDSA<SHA>::Signer test;
	GDSA<SHA>::Verifier test1;
	DSA::Signer test5(NullRNG(), 100);
	DSA::Signer test2(test5);
	NR<SHA>::Signer test3;
	NR<SHA>::Verifier test4;
	DLIES<>::Encryptor test6;
	DLIES<>::Decryptor test7;
}
#endif

void DL_GroupParameters_DSA::GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &alg)
{
	Integer p, q, g;

	if (alg.GetValue("Modulus", p) && alg.GetValue("SubgroupGenerator", g))
	{
		q = alg.GetValueWithDefault("SubgroupOrder", ComputeGroupOrder(p)/2);
		Initialize(p, q, g);
	}
	else
	{
		int modulusSize = 1024, defaultSubgroupOrderSize;
		alg.GetIntValue("ModulusSize", modulusSize) || alg.GetIntValue("KeySize", modulusSize);

		switch (modulusSize)
		{
		case 1024:
			defaultSubgroupOrderSize = 160;
			break;
		case 2048:
			defaultSubgroupOrderSize = 224;
			break;
		case 3072:
			defaultSubgroupOrderSize = 256;
			break;
		default:
			throw InvalidArgument("DSA: not a valid prime length");
		}

		DL_GroupParameters_GFP::GenerateRandom(rng, CombinedNameValuePairs(alg, MakeParameters(Name::SubgroupOrderSize(), defaultSubgroupOrderSize, false)));
	}
}

bool DL_GroupParameters_DSA::ValidateGroup(RandomNumberGenerator &rng, unsigned int level) const
{
	bool pass = DL_GroupParameters_GFP::ValidateGroup(rng, level);
	int pSize = GetModulus().BitCount(), qSize = GetSubgroupOrder().BitCount();
	pass = pass && ((pSize==1024 && qSize==160) || (pSize==2048 && qSize==224) || (pSize==2048 && qSize==256) || (pSize==3072 && qSize==256));
	return pass;
}

void DL_SignatureMessageEncodingMethod_DSA::ComputeMessageRepresentative(RandomNumberGenerator &rng, 
	const byte *recoverableMessage, size_t recoverableMessageLength,
	HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
	byte *representative, size_t representativeBitLength) const
{
	CRYPTOPP_UNUSED(rng), CRYPTOPP_UNUSED(recoverableMessage), CRYPTOPP_UNUSED(recoverableMessageLength);
	CRYPTOPP_UNUSED(messageEmpty), CRYPTOPP_UNUSED(hashIdentifier);
	assert(recoverableMessageLength == 0);
	assert(hashIdentifier.second == 0);

	const size_t representativeByteLength = BitsToBytes(representativeBitLength);
	const size_t digestSize = hash.DigestSize();
	const size_t paddingLength = SaturatingSubtract(representativeByteLength, digestSize);

	memset(representative, 0, paddingLength);
	hash.TruncatedFinal(representative+paddingLength, STDMIN(representativeByteLength, digestSize));

	if (digestSize*8 > representativeBitLength)
	{
		Integer h(representative, representativeByteLength);
		h >>= representativeByteLength*8 - representativeBitLength;
		h.Encode(representative, representativeByteLength);
	}
}

void DL_SignatureMessageEncodingMethod_NR::ComputeMessageRepresentative(RandomNumberGenerator &rng, 
	const byte *recoverableMessage, size_t recoverableMessageLength,
	HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
	byte *representative, size_t representativeBitLength) const
{
	CRYPTOPP_UNUSED(rng);CRYPTOPP_UNUSED(recoverableMessage); CRYPTOPP_UNUSED(recoverableMessageLength);
	CRYPTOPP_UNUSED(hash); CRYPTOPP_UNUSED(hashIdentifier); CRYPTOPP_UNUSED(messageEmpty);
	CRYPTOPP_UNUSED(representative); CRYPTOPP_UNUSED(representativeBitLength);

	assert(recoverableMessageLength == 0);
	assert(hashIdentifier.second == 0);
	const size_t representativeByteLength = BitsToBytes(representativeBitLength);
	const size_t digestSize = hash.DigestSize();
	const size_t paddingLength = SaturatingSubtract(representativeByteLength, digestSize);

	memset(representative, 0, paddingLength);
	hash.TruncatedFinal(representative+paddingLength, STDMIN(representativeByteLength, digestSize));

	if (digestSize*8 >= representativeBitLength)
	{
		Integer h(representative, representativeByteLength);
		h >>= representativeByteLength*8 - representativeBitLength + 1;
		h.Encode(representative, representativeByteLength);
	}
}

bool DL_GroupParameters_IntegerBased::ValidateGroup(RandomNumberGenerator &rng, unsigned int level) const
{
	const Integer &p = GetModulus(), &q = GetSubgroupOrder();

	bool pass = true;
	pass = pass && p > Integer::One() && p.IsOdd();
	pass = pass && q > Integer::One() && q.IsOdd();

	if (level >= 1)
		pass = pass && GetCofactor() > Integer::One() && GetGroupOrder() % q == Integer::Zero();
	if (level >= 2)
		pass = pass && VerifyPrime(rng, q, level-2) && VerifyPrime(rng, p, level-2);

	return pass;
}

bool DL_GroupParameters_IntegerBased::ValidateElement(unsigned int level, const Integer &g, const DL_FixedBasePrecomputation<Integer> *gpc) const
{
	const Integer &p = GetModulus(), &q = GetSubgroupOrder();

	bool pass = true;
	pass = pass && GetFieldType() == 1 ? g.IsPositive() : g.NotNegative();
	pass = pass && g < p && !IsIdentity(g);

	if (level >= 1)
	{
		if (gpc)
			pass = pass && gpc->Exponentiate(GetGroupPrecomputation(), Integer::One()) == g;
	}
	if (level >= 2)
	{
		if (GetFieldType() == 2)
			pass = pass && Jacobi(g*g-4, p)==-1;

		// verifying that Lucas((p+1)/2, w, p)==2 is omitted because it's too costly
		// and at most 1 bit is leaked if it's false
		bool fullValidate = (GetFieldType() == 2 && level >= 3) || !FastSubgroupCheckAvailable();

		if (fullValidate && pass)
		{
			Integer gp = gpc ? gpc->Exponentiate(GetGroupPrecomputation(), q) : ExponentiateElement(g, q);
			pass = pass && IsIdentity(gp);
		}
		else if (GetFieldType() == 1)
			pass = pass && Jacobi(g, p) == 1;
	}

	return pass;
}

void DL_GroupParameters_IntegerBased::GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &alg)
{
	Integer p, q, g;
	
	if (alg.GetValue("Modulus", p) && alg.GetValue("SubgroupGenerator", g))
	{
		q = alg.GetValueWithDefault("SubgroupOrder", ComputeGroupOrder(p)/2);
	}
	else
	{
		int modulusSize, subgroupOrderSize;

		if (!alg.GetIntValue("ModulusSize", modulusSize))
			modulusSize = alg.GetIntValueWithDefault("KeySize", 2048);

		if (!alg.GetIntValue("SubgroupOrderSize", subgroupOrderSize))
			subgroupOrderSize = GetDefaultSubgroupOrderSize(modulusSize);

		PrimeAndGenerator pg;
		pg.Generate(GetFieldType() == 1 ? 1 : -1, rng, modulusSize, subgroupOrderSize);
		p = pg.Prime();
		q = pg.SubPrime();
		g = pg.Generator();
	}

	Initialize(p, q, g);
}

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
void DL_GroupParameters_IntegerBased::EncodeElement(bool reversible, const Element &element, byte *encoded) const
{
	CRYPTOPP_UNUSED(reversible);
	element.Encode(encoded, GetModulus().ByteCount());	
}

unsigned int DL_GroupParameters_IntegerBased::GetEncodedElementSize(bool reversible) const
{
	CRYPTOPP_UNUSED(reversible);
	return GetModulus().ByteCount();
}
#endif

Integer DL_GroupParameters_IntegerBased::DecodeElement(const byte *encoded, bool checkForGroupMembership) const
{
	CRYPTOPP_UNUSED(checkForGroupMembership);
	Integer g(encoded, GetModulus().ByteCount());
	if (!ValidateElement(1, g, NULL))
		throw DL_BadElement();
	return g;
}

void DL_GroupParameters_IntegerBased::BERDecode(BufferedTransformation &bt)
{
	BERSequenceDecoder parameters(bt);
		Integer p(parameters);
		Integer q(parameters);
		Integer g;
		if (parameters.EndReached())
		{
			g = q;
			q = ComputeGroupOrder(p) / 2;
		}
		else
			g.BERDecode(parameters);
	parameters.MessageEnd();

	SetModulusAndSubgroupGenerator(p, g);
	SetSubgroupOrder(q);
}

void DL_GroupParameters_IntegerBased::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder parameters(bt);
		GetModulus().DEREncode(parameters);
		m_q.DEREncode(parameters);
		GetSubgroupGenerator().DEREncode(parameters);
	parameters.MessageEnd();
}

bool DL_GroupParameters_IntegerBased::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
	return GetValueHelper<DL_GroupParameters<Element> >(this, name, valueType, pValue)
		CRYPTOPP_GET_FUNCTION_ENTRY(Modulus);
}

void DL_GroupParameters_IntegerBased::AssignFrom(const NameValuePairs &source)
{
	AssignFromHelper(this, source)
		CRYPTOPP_SET_FUNCTION_ENTRY2(Modulus, SubgroupGenerator)
		CRYPTOPP_SET_FUNCTION_ENTRY(SubgroupOrder)
		;
}

OID DL_GroupParameters_IntegerBased::GetAlgorithmID() const
{
	return ASN1::id_dsa();
}

void DL_GroupParameters_GFP::SimultaneousExponentiate(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const
{
	ModularArithmetic ma(GetModulus());
	ma.SimultaneousExponentiate(results, base, exponents, exponentsCount);
}

DL_GroupParameters_GFP::Element DL_GroupParameters_GFP::MultiplyElements(const Element &a, const Element &b) const
{
	return a_times_b_mod_c(a, b, GetModulus());
}

DL_GroupParameters_GFP::Element DL_GroupParameters_GFP::CascadeExponentiate(const Element &element1, const Integer &exponent1, const Element &element2, const Integer &exponent2) const
{
	ModularArithmetic ma(GetModulus());
	return ma.CascadeExponentiate(element1, exponent1, element2, exponent2);
}

Integer DL_GroupParameters_IntegerBased::GetMaxExponent() const
{
	return STDMIN(GetSubgroupOrder()-1, Integer::Power2(2*DiscreteLogWorkFactor(GetFieldType()*GetModulus().BitCount())));
}

unsigned int DL_GroupParameters_IntegerBased::GetDefaultSubgroupOrderSize(unsigned int modulusSize) const
{
	return 2*DiscreteLogWorkFactor(GetFieldType()*modulusSize);
}

NAMESPACE_END

#endif
