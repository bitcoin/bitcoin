// luc.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "luc.h"
#include "asn.h"
#include "sha.h"
#include "integer.h"
#include "nbtheory.h"
#include "algparam.h"

NAMESPACE_BEGIN(CryptoPP)

#if !defined(NDEBUG) && !defined(CRYPTOPP_DOXYGEN_PROCESSING)
void LUC_TestInstantiations()
{
	LUC_HMP<SHA>::Signer t1;
	LUCFunction t2;
	InvertibleLUCFunction t3;
}
#endif

void DL_Algorithm_LUC_HMP::Sign(const DL_GroupParameters<Integer> &params, const Integer &x, const Integer &k, const Integer &e, Integer &r, Integer &s) const
{
	const Integer &q = params.GetSubgroupOrder();
	r = params.ExponentiateBase(k);
	s = (k + x*(r+e)) % q;
}

bool DL_Algorithm_LUC_HMP::Verify(const DL_GroupParameters<Integer> &params, const DL_PublicKey<Integer> &publicKey, const Integer &e, const Integer &r, const Integer &s) const
{
	const Integer p = params.GetGroupOrder()-1;
	const Integer &q = params.GetSubgroupOrder();

	Integer Vsg = params.ExponentiateBase(s);
	Integer Vry = publicKey.ExponentiatePublicElement((r+e)%q);
	return (Vsg*Vsg + Vry*Vry + r*r) % p == (Vsg * Vry * r + 4) % p;
}

Integer DL_BasePrecomputation_LUC::Exponentiate(const DL_GroupPrecomputation<Element> &group, const Integer &exponent) const
{
	return Lucas(exponent, m_g, static_cast<const DL_GroupPrecomputation_LUC &>(group).GetModulus());
}

void DL_GroupParameters_LUC::SimultaneousExponentiate(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const
{
	for (unsigned int i=0; i<exponentsCount; i++)
		results[i] = Lucas(exponents[i], base, GetModulus());
}

void LUCFunction::BERDecode(BufferedTransformation &bt)
{
	BERSequenceDecoder seq(bt);
	m_n.BERDecode(seq);
	m_e.BERDecode(seq);
	seq.MessageEnd();
}

void LUCFunction::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder seq(bt);
	m_n.DEREncode(seq);
	m_e.DEREncode(seq);
	seq.MessageEnd();
}

Integer LUCFunction::ApplyFunction(const Integer &x) const
{
	DoQuickSanityCheck();
	return Lucas(m_e, x, m_n);
}

bool LUCFunction::Validate(RandomNumberGenerator &rng, unsigned int level) const
{
	CRYPTOPP_UNUSED(rng), CRYPTOPP_UNUSED(level);
	bool pass = true;
	pass = pass && m_n > Integer::One() && m_n.IsOdd();
	pass = pass && m_e > Integer::One() && m_e.IsOdd() && m_e < m_n;
	return pass;
}

bool LUCFunction::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
	return GetValueHelper(this, name, valueType, pValue).Assignable()
		CRYPTOPP_GET_FUNCTION_ENTRY(Modulus)
		CRYPTOPP_GET_FUNCTION_ENTRY(PublicExponent)
		;
}

void LUCFunction::AssignFrom(const NameValuePairs &source)
{
	AssignFromHelper(this, source)
		CRYPTOPP_SET_FUNCTION_ENTRY(Modulus)
		CRYPTOPP_SET_FUNCTION_ENTRY(PublicExponent)
		;
}

// *****************************************************************************
// private key operations:

class LUCPrimeSelector : public PrimeSelector
{
public:
	LUCPrimeSelector(const Integer &e) : m_e(e) {}
	bool IsAcceptable(const Integer &candidate) const
	{
		return RelativelyPrime(m_e, candidate+1) && RelativelyPrime(m_e, candidate-1);
	}
	Integer m_e;
};

void InvertibleLUCFunction::GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &alg)
{
	int modulusSize = 2048;
	alg.GetIntValue("ModulusSize", modulusSize) || alg.GetIntValue("KeySize", modulusSize);

	if (modulusSize < 16)
		throw InvalidArgument("InvertibleLUCFunction: specified modulus size is too small");

	m_e = alg.GetValueWithDefault("PublicExponent", Integer(17));

	if (m_e < 5 || m_e.IsEven())
		throw InvalidArgument("InvertibleLUCFunction: invalid public exponent");

	LUCPrimeSelector selector(m_e);
	AlgorithmParameters primeParam = MakeParametersForTwoPrimesOfEqualSize(modulusSize)
		("PointerToPrimeSelector", selector.GetSelectorPointer());
	m_p.GenerateRandom(rng, primeParam);
	m_q.GenerateRandom(rng, primeParam);

	m_n = m_p * m_q;
	m_u = m_q.InverseMod(m_p);
}

void InvertibleLUCFunction::Initialize(RandomNumberGenerator &rng, unsigned int keybits, const Integer &e)
{
	GenerateRandom(rng, MakeParameters("ModulusSize", (int)keybits)("PublicExponent", e));
}

void InvertibleLUCFunction::BERDecode(BufferedTransformation &bt)
{
	BERSequenceDecoder seq(bt);

	Integer version(seq);
	if (!!version)  // make sure version is 0
		BERDecodeError();

	m_n.BERDecode(seq);
	m_e.BERDecode(seq);
	m_p.BERDecode(seq);
	m_q.BERDecode(seq);
	m_u.BERDecode(seq);
	seq.MessageEnd();
}

void InvertibleLUCFunction::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder seq(bt);

	const byte version[] = {INTEGER, 1, 0};
	seq.Put(version, sizeof(version));
	m_n.DEREncode(seq);
	m_e.DEREncode(seq);
	m_p.DEREncode(seq);
	m_q.DEREncode(seq);
	m_u.DEREncode(seq);
	seq.MessageEnd();
}

Integer InvertibleLUCFunction::CalculateInverse(RandomNumberGenerator &rng, const Integer &x) const
{
	// not clear how to do blinding with LUC
	CRYPTOPP_UNUSED(rng);
	DoQuickSanityCheck();
	return InverseLucas(m_e, x, m_q, m_p, m_u);
}

bool InvertibleLUCFunction::Validate(RandomNumberGenerator &rng, unsigned int level) const
{
	bool pass = LUCFunction::Validate(rng, level);
	pass = pass && m_p > Integer::One() && m_p.IsOdd() && m_p < m_n;
	pass = pass && m_q > Integer::One() && m_q.IsOdd() && m_q < m_n;
	pass = pass && m_u.IsPositive() && m_u < m_p;
	if (level >= 1)
	{
		pass = pass && m_p * m_q == m_n;
		pass = pass && RelativelyPrime(m_e, m_p+1);
		pass = pass && RelativelyPrime(m_e, m_p-1);
		pass = pass && RelativelyPrime(m_e, m_q+1);
		pass = pass && RelativelyPrime(m_e, m_q-1);
		pass = pass && m_u * m_q % m_p == 1;
	}
	if (level >= 2)
		pass = pass && VerifyPrime(rng, m_p, level-2) && VerifyPrime(rng, m_q, level-2);
	return pass;
}

bool InvertibleLUCFunction::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
	return GetValueHelper<LUCFunction>(this, name, valueType, pValue).Assignable()
		CRYPTOPP_GET_FUNCTION_ENTRY(Prime1)
		CRYPTOPP_GET_FUNCTION_ENTRY(Prime2)
		CRYPTOPP_GET_FUNCTION_ENTRY(MultiplicativeInverseOfPrime2ModPrime1)
		;
}

void InvertibleLUCFunction::AssignFrom(const NameValuePairs &source)
{
	AssignFromHelper<LUCFunction>(this, source)
		CRYPTOPP_SET_FUNCTION_ENTRY(Prime1)
		CRYPTOPP_SET_FUNCTION_ENTRY(Prime2)
		CRYPTOPP_SET_FUNCTION_ENTRY(MultiplicativeInverseOfPrime2ModPrime1)
		;
}

NAMESPACE_END
