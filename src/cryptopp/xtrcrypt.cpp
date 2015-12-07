// xtrcrypt.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#include "asn.h"
#include "integer.h"
#include "xtrcrypt.h"
#include "nbtheory.h"
#include "modarith.h"
#include "argnames.h"

NAMESPACE_BEGIN(CryptoPP)

XTR_DH::XTR_DH(const Integer &p, const Integer &q, const GFP2Element &g)
	: m_p(p), m_q(q), m_g(g)
{
}

XTR_DH::XTR_DH(RandomNumberGenerator &rng, unsigned int pbits, unsigned int qbits)
{
	XTR_FindPrimesAndGenerator(rng, m_p, m_q, m_g, pbits, qbits);
}

XTR_DH::XTR_DH(BufferedTransformation &bt)
{
	BERSequenceDecoder seq(bt);
	m_p.BERDecode(seq);
	m_q.BERDecode(seq);
	m_g.c1.BERDecode(seq);
	m_g.c2.BERDecode(seq);
	seq.MessageEnd();
}

void XTR_DH::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder seq(bt);
	m_p.DEREncode(seq);
	m_q.DEREncode(seq);
	m_g.c1.DEREncode(seq);
	m_g.c2.DEREncode(seq);
	seq.MessageEnd();
}

bool XTR_DH::Validate(RandomNumberGenerator &rng, unsigned int level) const
{
	bool pass = true;
	pass = pass && m_p > Integer::One() && m_p.IsOdd();
	pass = pass && m_q > Integer::One() && m_q.IsOdd();
	GFP2Element three = GFP2_ONB<ModularArithmetic>(m_p).ConvertIn(3);
	pass = pass && !(m_g.c1.IsNegative() || m_g.c2.IsNegative() || m_g.c1 >= m_p || m_g.c2 >= m_p || m_g == three);
	if (level >= 1)
		pass = pass && ((m_p.Squared()-m_p+1)%m_q).IsZero();
	if (level >= 2)
	{
		pass = pass && VerifyPrime(rng, m_p, level-2) && VerifyPrime(rng, m_q, level-2);
		pass = pass && XTR_Exponentiate(m_g, (m_p.Squared()-m_p+1)/m_q, m_p) != three;
		pass = pass && XTR_Exponentiate(m_g, m_q, m_p) == three;
	}
	return pass;
}

bool XTR_DH::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
	return GetValueHelper(this, name, valueType, pValue).Assignable()
		CRYPTOPP_GET_FUNCTION_ENTRY(Modulus)
		CRYPTOPP_GET_FUNCTION_ENTRY(SubgroupOrder)
		CRYPTOPP_GET_FUNCTION_ENTRY(SubgroupGenerator)
		;
}

void XTR_DH::AssignFrom(const NameValuePairs &source)
{
	AssignFromHelper(this, source)
		CRYPTOPP_SET_FUNCTION_ENTRY(Modulus)
		CRYPTOPP_SET_FUNCTION_ENTRY(SubgroupOrder)
		CRYPTOPP_SET_FUNCTION_ENTRY(SubgroupGenerator)
		;
}

void XTR_DH::GeneratePrivateKey(RandomNumberGenerator &rng, byte *privateKey) const
{
	Integer x(rng, Integer::Zero(), m_q-1);
	x.Encode(privateKey, PrivateKeyLength());
}

void XTR_DH::GeneratePublicKey(RandomNumberGenerator &rng, const byte *privateKey, byte *publicKey) const
{
	CRYPTOPP_UNUSED(rng);
	Integer x(privateKey, PrivateKeyLength());
	GFP2Element y = XTR_Exponentiate(m_g, x, m_p);
	y.Encode(publicKey, PublicKeyLength());
}

bool XTR_DH::Agree(byte *agreedValue, const byte *privateKey, const byte *otherPublicKey, bool validateOtherPublicKey) const
{
	GFP2Element w(otherPublicKey, PublicKeyLength());
	if (validateOtherPublicKey)
	{
		GFP2_ONB<ModularArithmetic> gfp2(m_p);
		GFP2Element three = gfp2.ConvertIn(3);
		if (w.c1.IsNegative() || w.c2.IsNegative() || w.c1 >= m_p || w.c2 >= m_p || w == three)
			return false;
		if (XTR_Exponentiate(w, m_q, m_p) != three)
			return false;
	}
	Integer s(privateKey, PrivateKeyLength());
	GFP2Element z = XTR_Exponentiate(w, s, m_p);
	z.Encode(agreedValue, AgreedValueLength());
	return true;
}

NAMESPACE_END
