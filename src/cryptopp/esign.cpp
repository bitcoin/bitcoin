// esign.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "config.h"

// TODO: fix the C4589 warnings
#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4589)
#endif

#include "esign.h"
#include "modarith.h"
#include "integer.h"
#include "nbtheory.h"
#include "algparam.h"
#include "sha.h"
#include "asn.h"

NAMESPACE_BEGIN(CryptoPP)

#if !defined(NDEBUG) && !defined(CRYPTOPP_DOXYGEN_PROCESSING)
void ESIGN_TestInstantiations()
{
	ESIGN<SHA>::Verifier x1(1, 1);
	ESIGN<SHA>::Signer x2(NullRNG(), 1);
	ESIGN<SHA>::Verifier x3(x2);
	ESIGN<SHA>::Verifier x4(x2.GetKey());
	ESIGN<SHA>::Verifier x5(x3);
	ESIGN<SHA>::Signer x6 = x2;

	x6 = x2;
	x3 = ESIGN<SHA>::Verifier(x2);
	x4 = x2.GetKey();
}
#endif

void ESIGNFunction::BERDecode(BufferedTransformation &bt)
{
	BERSequenceDecoder seq(bt);
		m_n.BERDecode(seq);
		m_e.BERDecode(seq);
	seq.MessageEnd();
}

void ESIGNFunction::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder seq(bt);
		m_n.DEREncode(seq);
		m_e.DEREncode(seq);
	seq.MessageEnd();
}

Integer ESIGNFunction::ApplyFunction(const Integer &x) const
{
	DoQuickSanityCheck();
	return STDMIN(a_exp_b_mod_c(x, m_e, m_n) >> (2*GetK()+2), MaxImage());
}

bool ESIGNFunction::Validate(RandomNumberGenerator& rng, unsigned int level) const
{
	CRYPTOPP_UNUSED(rng), CRYPTOPP_UNUSED(level);
	bool pass = true;
	pass = pass && m_n > Integer::One() && m_n.IsOdd();
	pass = pass && m_e >= 8 && m_e < m_n;
	return pass;
}

bool ESIGNFunction::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
	return GetValueHelper(this, name, valueType, pValue).Assignable()
		CRYPTOPP_GET_FUNCTION_ENTRY(Modulus)
		CRYPTOPP_GET_FUNCTION_ENTRY(PublicExponent)
		;
}

void ESIGNFunction::AssignFrom(const NameValuePairs &source)
{
	AssignFromHelper(this, source)
		CRYPTOPP_SET_FUNCTION_ENTRY(Modulus)
		CRYPTOPP_SET_FUNCTION_ENTRY(PublicExponent)
		;
}

// *****************************************************************************

void InvertibleESIGNFunction::GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &param)
{
	int modulusSize = 1023*2;
	param.GetIntValue("ModulusSize", modulusSize) || param.GetIntValue("KeySize", modulusSize);

	if (modulusSize < 24)
		throw InvalidArgument("InvertibleESIGNFunction: specified modulus size is too small");

	if (modulusSize % 3 != 0)
		throw InvalidArgument("InvertibleESIGNFunction: modulus size must be divisible by 3");

	m_e = param.GetValueWithDefault("PublicExponent", Integer(32));

	if (m_e < 8)
		throw InvalidArgument("InvertibleESIGNFunction: public exponents less than 8 may not be secure");

	// VC70 workaround: putting these after primeParam causes overlapped stack allocation
	ConstByteArrayParameter seedParam;
	SecByteBlock seed;

	const Integer minP = Integer(204) << (modulusSize/3-8);
	const Integer maxP = Integer::Power2(modulusSize/3)-1;
	AlgorithmParameters primeParam = MakeParameters("Min", minP)("Max", maxP)("RandomNumberType", Integer::PRIME);

	if (param.GetValue("Seed", seedParam))
	{
		seed.resize(seedParam.size() + 4);
		memcpy(seed + 4, seedParam.begin(), seedParam.size());

		PutWord(false, BIG_ENDIAN_ORDER, seed, (word32)0);
		m_p.GenerateRandom(rng, CombinedNameValuePairs(primeParam, MakeParameters("Seed", ConstByteArrayParameter(seed))));
		PutWord(false, BIG_ENDIAN_ORDER, seed, (word32)1);
		m_q.GenerateRandom(rng, CombinedNameValuePairs(primeParam, MakeParameters("Seed", ConstByteArrayParameter(seed))));
	}
	else
	{
		m_p.GenerateRandom(rng, primeParam);
		m_q.GenerateRandom(rng, primeParam);
	}

	m_n = m_p * m_p * m_q;

	assert(m_n.BitCount() == (unsigned int)modulusSize);
}

void InvertibleESIGNFunction::BERDecode(BufferedTransformation &bt)
{
	BERSequenceDecoder privateKey(bt);
		m_n.BERDecode(privateKey);
		m_e.BERDecode(privateKey);
		m_p.BERDecode(privateKey);
		m_q.BERDecode(privateKey);
	privateKey.MessageEnd();
}

void InvertibleESIGNFunction::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder privateKey(bt);
		m_n.DEREncode(privateKey);
		m_e.DEREncode(privateKey);
		m_p.DEREncode(privateKey);
		m_q.DEREncode(privateKey);
	privateKey.MessageEnd();
}

Integer InvertibleESIGNFunction::CalculateRandomizedInverse(RandomNumberGenerator &rng, const Integer &x) const 
{
	DoQuickSanityCheck();

	Integer pq = m_p * m_q;
	Integer p2 = m_p * m_p;
	Integer r, z, re, a, w0, w1;

	do
	{
		r.Randomize(rng, Integer::Zero(), pq);
		z = x << (2*GetK()+2);
		re = a_exp_b_mod_c(r, m_e, m_n);
		a = (z - re) % m_n;
		Integer::Divide(w1, w0, a, pq);
		if (w1.NotZero())
		{
			++w0;
			w1 = pq - w1;
		}
	}
	while ((w1 >> (2*GetK()+1)).IsPositive());

	ModularArithmetic modp(m_p);
	Integer t = modp.Divide(w0 * r % m_p, m_e * re % m_p);
	Integer s = r + t*pq;
	assert(s < m_n);
#if 0
	using namespace std;
	cout << "f = " << x << endl;
	cout << "r = " << r << endl;
	cout << "z = " << z << endl;
	cout << "a = " << a << endl;
	cout << "w0 = " << w0 << endl;
	cout << "w1 = " << w1 << endl;
	cout << "t = " << t << endl;
	cout << "s = " << s << endl;
#endif
	return s;
}

bool InvertibleESIGNFunction::Validate(RandomNumberGenerator &rng, unsigned int level) const
{
	bool pass = ESIGNFunction::Validate(rng, level);
	pass = pass && m_p > Integer::One() && m_p.IsOdd() && m_p < m_n;
	pass = pass && m_q > Integer::One() && m_q.IsOdd() && m_q < m_n;
	pass = pass && m_p.BitCount() == m_q.BitCount();
	if (level >= 1)
		pass = pass && m_p * m_p * m_q == m_n;
	if (level >= 2)
		pass = pass && VerifyPrime(rng, m_p, level-2) && VerifyPrime(rng, m_q, level-2);
	return pass;
}

bool InvertibleESIGNFunction::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
	return GetValueHelper<ESIGNFunction>(this, name, valueType, pValue).Assignable()
		CRYPTOPP_GET_FUNCTION_ENTRY(Prime1)
		CRYPTOPP_GET_FUNCTION_ENTRY(Prime2)
		;
}

void InvertibleESIGNFunction::AssignFrom(const NameValuePairs &source)
{
	AssignFromHelper<ESIGNFunction>(this, source)
		CRYPTOPP_SET_FUNCTION_ENTRY(Prime1)
		CRYPTOPP_SET_FUNCTION_ENTRY(Prime2)
		;
}

NAMESPACE_END
