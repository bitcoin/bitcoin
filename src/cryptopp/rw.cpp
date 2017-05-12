// rw.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#include "rw.h"
#include "asn.h"
#include "integer.h"
#include "nbtheory.h"
#include "modarith.h"
#include "asn.h"

#ifndef CRYPTOPP_IMPORTS

#if defined(_OPENMP)
static const bool CRYPTOPP_RW_USE_OMP = true;
#else
static const bool CRYPTOPP_RW_USE_OMP = false;
#endif

NAMESPACE_BEGIN(CryptoPP)

void RWFunction::BERDecode(BufferedTransformation &bt)
{
	BERSequenceDecoder seq(bt);
	m_n.BERDecode(seq);
	seq.MessageEnd();
}

void RWFunction::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder seq(bt);
	m_n.DEREncode(seq);
	seq.MessageEnd();
}

Integer RWFunction::ApplyFunction(const Integer &in) const
{
	DoQuickSanityCheck();

	Integer out = in.Squared()%m_n;
	const word r = 12;
	// this code was written to handle both r = 6 and r = 12,
	// but now only r = 12 is used in P1363
	const word r2 = r/2;
	const word r3a = (16 + 5 - r) % 16;	// n%16 could be 5 or 13
	const word r3b = (16 + 13 - r) % 16;
	const word r4 = (8 + 5 - r/2) % 8;	// n%8 == 5
	switch (out % 16)
	{
	case r:
		break;
	case r2:
	case r2+8:
		out <<= 1;
		break;
	case r3a:
	case r3b:
		out.Negate();
		out += m_n;
		break;
	case r4:
	case r4+8:
		out.Negate();
		out += m_n;
		out <<= 1;
		break;
	default:
		out = Integer::Zero();
	}
	return out;
}

bool RWFunction::Validate(RandomNumberGenerator &rng, unsigned int level) const
{
	CRYPTOPP_UNUSED(rng), CRYPTOPP_UNUSED(level);
	bool pass = true;
	pass = pass && m_n > Integer::One() && m_n%8 == 5;
	return pass;
}

bool RWFunction::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
	return GetValueHelper(this, name, valueType, pValue).Assignable()
		CRYPTOPP_GET_FUNCTION_ENTRY(Modulus)
		;
}

void RWFunction::AssignFrom(const NameValuePairs &source)
{
	AssignFromHelper(this, source)
		CRYPTOPP_SET_FUNCTION_ENTRY(Modulus)
		;
}

// *****************************************************************************
// private key operations:

// generate a random private key
void InvertibleRWFunction::GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &alg)
{
	int modulusSize = 2048;
	alg.GetIntValue("ModulusSize", modulusSize) || alg.GetIntValue("KeySize", modulusSize);

	if (modulusSize < 16)
		throw InvalidArgument("InvertibleRWFunction: specified modulus length is too small");

	AlgorithmParameters primeParam = MakeParametersForTwoPrimesOfEqualSize(modulusSize);
	m_p.GenerateRandom(rng, CombinedNameValuePairs(primeParam, MakeParameters("EquivalentTo", 3)("Mod", 8)));
	m_q.GenerateRandom(rng, CombinedNameValuePairs(primeParam, MakeParameters("EquivalentTo", 7)("Mod", 8)));

	m_n = m_p * m_q;
	m_u = m_q.InverseMod(m_p);

	Precompute();
}

void InvertibleRWFunction::Initialize(const Integer &n, const Integer &p, const Integer &q, const Integer &u)
{
	m_n = n; m_p = p; m_q = q; m_u = u;

	Precompute();
}

void InvertibleRWFunction::PrecomputeTweakedRoots() const
{
	ModularArithmetic modp(m_p), modq(m_q);

	#pragma omp parallel sections if(CRYPTOPP_RW_USE_OMP)
	{
		#pragma omp section
			m_pre_2_9p = modp.Exponentiate(2, (9 * m_p - 11)/8);
		#pragma omp section
			m_pre_2_3q = modq.Exponentiate(2, (3 * m_q - 5)/8);
		#pragma omp section
			m_pre_q_p = modp.Exponentiate(m_q, m_p - 2);
	}

	m_precompute = true;
}

void InvertibleRWFunction::LoadPrecomputation(BufferedTransformation &bt)
{
	BERSequenceDecoder seq(bt);
	m_pre_2_9p.BERDecode(seq);
	m_pre_2_3q.BERDecode(seq);
	m_pre_q_p.BERDecode(seq);
	seq.MessageEnd();

	m_precompute = true;
}

void InvertibleRWFunction::SavePrecomputation(BufferedTransformation &bt) const
{
	if(!m_precompute)
		Precompute();

	DERSequenceEncoder seq(bt);
	m_pre_2_9p.DEREncode(seq);
	m_pre_2_3q.DEREncode(seq);
	m_pre_q_p.DEREncode(seq);
	seq.MessageEnd();
}

void InvertibleRWFunction::BERDecode(BufferedTransformation &bt)
{
	BERSequenceDecoder seq(bt);
	m_n.BERDecode(seq);
	m_p.BERDecode(seq);
	m_q.BERDecode(seq);
	m_u.BERDecode(seq);
	seq.MessageEnd();

	m_precompute = false;
}

void InvertibleRWFunction::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder seq(bt);
	m_n.DEREncode(seq);
	m_p.DEREncode(seq);
	m_q.DEREncode(seq);
	m_u.DEREncode(seq);
	seq.MessageEnd();
}

// DJB's "RSA signatures and Rabin-Williams signatures..." (http://cr.yp.to/sigs/rwsota-20080131.pdf).
Integer InvertibleRWFunction::CalculateInverse(RandomNumberGenerator &rng, const Integer &x) const
{
	DoQuickSanityCheck();

	if(!m_precompute)
		Precompute();

	ModularArithmetic modn(m_n), modp(m_p), modq(m_q);
	Integer r, rInv;

	do
	{
		// Do this in a loop for people using small numbers for testing
		r.Randomize(rng, Integer::One(), m_n - Integer::One());
		// Fix for CVE-2015-2141. Thanks to Evgeny Sidorov for reporting.
		// Squaring to satisfy Jacobi requirements suggested by Jean-Pierre Munch.
		r = modn.Square(r);
		rInv = modn.MultiplicativeInverse(r);
	} while (rInv.IsZero());

	Integer re = modn.Square(r);
	re = modn.Multiply(re, x);    // blind

	const Integer &h = re, &p = m_p, &q = m_q;
	Integer e, f;

	const Integer U = modq.Exponentiate(h, (q+1)/8);
	if(((modq.Exponentiate(U, 4) - h) % q).IsZero())
		e = Integer::One();
	else
		e = -1;

	const Integer eh = e*h, V = modp.Exponentiate(eh, (p-3)/8);
	if(((modp.Multiply(modp.Exponentiate(V, 4), modp.Exponentiate(eh, 2)) - eh) % p).IsZero())
		f = Integer::One();
	else
		f = 2;

	Integer W, X;
	#pragma omp parallel sections if(CRYPTOPP_RW_USE_OMP)
	{
		#pragma omp section
		{
			W = (f.IsUnit() ? U : modq.Multiply(m_pre_2_3q, U));
		}
		#pragma omp section
		{
			const Integer t = modp.Multiply(modp.Exponentiate(V, 3), eh);
			X = (f.IsUnit() ? t : modp.Multiply(m_pre_2_9p, t));
		}
	}
	const Integer Y = W + q * modp.Multiply(m_pre_q_p, (X - W));

	// Signature
	Integer s = modn.Multiply(modn.Square(Y), rInv);
	CRYPTOPP_ASSERT((e * f * s.Squared()) % m_n == x);

	// IEEE P1363, Section 8.2.8 IFSP-RW, p.44
	s = STDMIN(s, m_n - s);
	if (ApplyFunction(s) != x)                      // check
		throw Exception(Exception::OTHER_ERROR, "InvertibleRWFunction: computational error during private key operation");

	return s;
}

bool InvertibleRWFunction::Validate(RandomNumberGenerator &rng, unsigned int level) const
{
	bool pass = RWFunction::Validate(rng, level);
	pass = pass && m_p > Integer::One() && m_p%8 == 3 && m_p < m_n;
	pass = pass && m_q > Integer::One() && m_q%8 == 7 && m_q < m_n;
	pass = pass && m_u.IsPositive() && m_u < m_p;
	if (level >= 1)
	{
		pass = pass && m_p * m_q == m_n;
		pass = pass && m_u * m_q % m_p == 1;
	}
	if (level >= 2)
		pass = pass && VerifyPrime(rng, m_p, level-2) && VerifyPrime(rng, m_q, level-2);
	return pass;
}

bool InvertibleRWFunction::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
	return GetValueHelper<RWFunction>(this, name, valueType, pValue).Assignable()
		CRYPTOPP_GET_FUNCTION_ENTRY(Prime1)
		CRYPTOPP_GET_FUNCTION_ENTRY(Prime2)
		CRYPTOPP_GET_FUNCTION_ENTRY(MultiplicativeInverseOfPrime2ModPrime1)
		;
}

void InvertibleRWFunction::AssignFrom(const NameValuePairs &source)
{
	AssignFromHelper<RWFunction>(this, source)
		CRYPTOPP_SET_FUNCTION_ENTRY(Prime1)
		CRYPTOPP_SET_FUNCTION_ENTRY(Prime2)
		CRYPTOPP_SET_FUNCTION_ENTRY(MultiplicativeInverseOfPrime2ModPrime1)
		;

	m_precompute = false;
}

NAMESPACE_END

#endif
