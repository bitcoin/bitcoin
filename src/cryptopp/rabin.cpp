// rabin.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "rabin.h"
#include "integer.h"
#include "nbtheory.h"
#include "modarith.h"
#include "asn.h"
#include "sha.h"

NAMESPACE_BEGIN(CryptoPP)

void RabinFunction::BERDecode(BufferedTransformation &bt)
{
	BERSequenceDecoder seq(bt);
	m_n.BERDecode(seq);
	m_r.BERDecode(seq);
	m_s.BERDecode(seq);
	seq.MessageEnd();
}

void RabinFunction::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder seq(bt);
	m_n.DEREncode(seq);
	m_r.DEREncode(seq);
	m_s.DEREncode(seq);
	seq.MessageEnd();
}

Integer RabinFunction::ApplyFunction(const Integer &in) const
{
	DoQuickSanityCheck();

	Integer out = in.Squared()%m_n;
	if (in.IsOdd())
		out = out*m_r%m_n;
	if (Jacobi(in, m_n)==-1)
		out = out*m_s%m_n;
	return out;
}

bool RabinFunction::Validate(RandomNumberGenerator& /*rng*/, unsigned int level) const
{
	bool pass = true;
	pass = pass && m_n > Integer::One() && m_n%4 == 1;
	pass = pass && m_r > Integer::One() && m_r < m_n;
	pass = pass && m_s > Integer::One() && m_s < m_n;
	if (level >= 1)
		pass = pass && Jacobi(m_r, m_n) == -1 && Jacobi(m_s, m_n) == -1;
	return pass;
}

bool RabinFunction::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
	return GetValueHelper(this, name, valueType, pValue).Assignable()
		CRYPTOPP_GET_FUNCTION_ENTRY(Modulus)
		CRYPTOPP_GET_FUNCTION_ENTRY(QuadraticResidueModPrime1)
		CRYPTOPP_GET_FUNCTION_ENTRY(QuadraticResidueModPrime2)
		;
}

void RabinFunction::AssignFrom(const NameValuePairs &source)
{
	AssignFromHelper(this, source)
		CRYPTOPP_SET_FUNCTION_ENTRY(Modulus)
		CRYPTOPP_SET_FUNCTION_ENTRY(QuadraticResidueModPrime1)
		CRYPTOPP_SET_FUNCTION_ENTRY(QuadraticResidueModPrime2)
		;
}

// *****************************************************************************
// private key operations:

// generate a random private key
void InvertibleRabinFunction::GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &alg)
{
	int modulusSize = 2048;
	alg.GetIntValue("ModulusSize", modulusSize) || alg.GetIntValue("KeySize", modulusSize);

	if (modulusSize < 16)
		throw InvalidArgument("InvertibleRabinFunction: specified modulus size is too small");

	// VC70 workaround: putting these after primeParam causes overlapped stack allocation
	bool rFound=false, sFound=false;
	Integer t=2;

	AlgorithmParameters primeParam = MakeParametersForTwoPrimesOfEqualSize(modulusSize)
		("EquivalentTo", 3)("Mod", 4);
	m_p.GenerateRandom(rng, primeParam);
	m_q.GenerateRandom(rng, primeParam);

	while (!(rFound && sFound))
	{
		int jp = Jacobi(t, m_p);
		int jq = Jacobi(t, m_q);

		if (!rFound && jp==1 && jq==-1)
		{
			m_r = t;
			rFound = true;
		}

		if (!sFound && jp==-1 && jq==1)
		{
			m_s = t;
			sFound = true;
		}

		++t;
	}

	m_n = m_p * m_q;
	m_u = m_q.InverseMod(m_p);
}

void InvertibleRabinFunction::BERDecode(BufferedTransformation &bt)
{
	BERSequenceDecoder seq(bt);
	m_n.BERDecode(seq);
	m_r.BERDecode(seq);
	m_s.BERDecode(seq);
	m_p.BERDecode(seq);
	m_q.BERDecode(seq);
	m_u.BERDecode(seq);
	seq.MessageEnd();
}

void InvertibleRabinFunction::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder seq(bt);
	m_n.DEREncode(seq);
	m_r.DEREncode(seq);
	m_s.DEREncode(seq);
	m_p.DEREncode(seq);
	m_q.DEREncode(seq);
	m_u.DEREncode(seq);
	seq.MessageEnd();
}

Integer InvertibleRabinFunction::CalculateInverse(RandomNumberGenerator &rng, const Integer &in) const
{
	DoQuickSanityCheck();

	ModularArithmetic modn(m_n);
	Integer r(rng, Integer::One(), m_n - Integer::One());
	r = modn.Square(r);
	Integer r2 = modn.Square(r);
	Integer c = modn.Multiply(in, r2);		// blind

	Integer cp=c%m_p, cq=c%m_q;

	int jp = Jacobi(cp, m_p);
	int jq = Jacobi(cq, m_q);

	if (jq==-1)
	{
		cp = cp*EuclideanMultiplicativeInverse(m_r, m_p)%m_p;
		cq = cq*EuclideanMultiplicativeInverse(m_r, m_q)%m_q;
	}

	if (jp==-1)
	{
		cp = cp*EuclideanMultiplicativeInverse(m_s, m_p)%m_p;
		cq = cq*EuclideanMultiplicativeInverse(m_s, m_q)%m_q;
	}

	cp = ModularSquareRoot(cp, m_p);
	cq = ModularSquareRoot(cq, m_q);

	if (jp==-1)
		cp = m_p-cp;

	Integer out = CRT(cq, m_q, cp, m_p, m_u);

	out = modn.Divide(out, r);	// unblind

	if ((jq==-1 && out.IsEven()) || (jq==1 && out.IsOdd()))
		out = m_n-out;

	return out;
}

bool InvertibleRabinFunction::Validate(RandomNumberGenerator &rng, unsigned int level) const
{
	bool pass = RabinFunction::Validate(rng, level);
	pass = pass && m_p > Integer::One() && m_p%4 == 3 && m_p < m_n;
	pass = pass && m_q > Integer::One() && m_q%4 == 3 && m_q < m_n;
	pass = pass && m_u.IsPositive() && m_u < m_p;
	if (level >= 1)
	{
		pass = pass && m_p * m_q == m_n;
		pass = pass && m_u * m_q % m_p == 1;
		pass = pass && Jacobi(m_r, m_p) == 1;
		pass = pass && Jacobi(m_r, m_q) == -1;
		pass = pass && Jacobi(m_s, m_p) == -1;
		pass = pass && Jacobi(m_s, m_q) == 1;
	}
	if (level >= 2)
		pass = pass && VerifyPrime(rng, m_p, level-2) && VerifyPrime(rng, m_q, level-2);
	return pass;
}

bool InvertibleRabinFunction::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
	return GetValueHelper<RabinFunction>(this, name, valueType, pValue).Assignable()
		CRYPTOPP_GET_FUNCTION_ENTRY(Prime1)
		CRYPTOPP_GET_FUNCTION_ENTRY(Prime2)
		CRYPTOPP_GET_FUNCTION_ENTRY(MultiplicativeInverseOfPrime2ModPrime1)
		;
}

void InvertibleRabinFunction::AssignFrom(const NameValuePairs &source)
{
	AssignFromHelper<RabinFunction>(this, source)
		CRYPTOPP_SET_FUNCTION_ENTRY(Prime1)
		CRYPTOPP_SET_FUNCTION_ENTRY(Prime2)
		CRYPTOPP_SET_FUNCTION_ENTRY(MultiplicativeInverseOfPrime2ModPrime1)
		;
}

NAMESPACE_END
