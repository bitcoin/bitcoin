// mqv.h - written and placed in the public domain by Wei Dai

//! \file mqv.h
//! \brief Classes for Menezes–Qu–Vanstone (MQV) key agreement

#ifndef CRYPTOPP_MQV_H
#define CRYPTOPP_MQV_H

#include "cryptlib.h"
#include "gfpcrypt.h"
#include "modarith.h"
#include "integer.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class MQV_Domain
//! \brief MQV domain for performing authenticated key agreement
//! \tparam GROUP_PARAMETERS doamin parameters
//! \tparam COFACTOR_OPTION cofactor option
//! \details GROUP_PARAMETERS paramters include the curve coefcients and the base point.
//!   Binary curves use a polynomial to represent its characteristic, while prime curves
//!   use a prime number.
template <class GROUP_PARAMETERS, class COFACTOR_OPTION = CPP_TYPENAME GROUP_PARAMETERS::DefaultCofactorOption>
class MQV_Domain : public AuthenticatedKeyAgreementDomain
{
public:
	typedef GROUP_PARAMETERS GroupParameters;
	typedef typename GroupParameters::Element Element;
	typedef MQV_Domain<GROUP_PARAMETERS, COFACTOR_OPTION> Domain;

	MQV_Domain() {}

	MQV_Domain(const GroupParameters &params)
		: m_groupParameters(params) {}

	MQV_Domain(BufferedTransformation &bt)
		{m_groupParameters.BERDecode(bt);}

	template <class T1, class T2>
	MQV_Domain(T1 v1, T2 v2)
		{m_groupParameters.Initialize(v1, v2);}
	
	template <class T1, class T2, class T3>
	MQV_Domain(T1 v1, T2 v2, T3 v3)
		{m_groupParameters.Initialize(v1, v2, v3);}
	
	template <class T1, class T2, class T3, class T4>
	MQV_Domain(T1 v1, T2 v2, T3 v3, T4 v4)
		{m_groupParameters.Initialize(v1, v2, v3, v4);}

	const GroupParameters & GetGroupParameters() const {return m_groupParameters;}
	GroupParameters & AccessGroupParameters() {return m_groupParameters;}

	CryptoParameters & AccessCryptoParameters() {return AccessAbstractGroupParameters();}

	unsigned int AgreedValueLength() const {return GetAbstractGroupParameters().GetEncodedElementSize(false);}
	unsigned int StaticPrivateKeyLength() const {return GetAbstractGroupParameters().GetSubgroupOrder().ByteCount();}
	unsigned int StaticPublicKeyLength() const {return GetAbstractGroupParameters().GetEncodedElementSize(true);}

	void GenerateStaticPrivateKey(RandomNumberGenerator &rng, byte *privateKey) const
	{
		Integer x(rng, Integer::One(), GetAbstractGroupParameters().GetMaxExponent());
		x.Encode(privateKey, StaticPrivateKeyLength());
	}

	void GenerateStaticPublicKey(RandomNumberGenerator &rng, const byte *privateKey, byte *publicKey) const
	{
		CRYPTOPP_UNUSED(rng);
		const DL_GroupParameters<Element> &params = GetAbstractGroupParameters();
		Integer x(privateKey, StaticPrivateKeyLength());
		Element y = params.ExponentiateBase(x);
		params.EncodeElement(true, y, publicKey);
	}

	unsigned int EphemeralPrivateKeyLength() const {return StaticPrivateKeyLength() + StaticPublicKeyLength();}
	unsigned int EphemeralPublicKeyLength() const {return StaticPublicKeyLength();}

	void GenerateEphemeralPrivateKey(RandomNumberGenerator &rng, byte *privateKey) const
	{
		const DL_GroupParameters<Element> &params = GetAbstractGroupParameters();
		Integer x(rng, Integer::One(), params.GetMaxExponent());
		x.Encode(privateKey, StaticPrivateKeyLength());
		Element y = params.ExponentiateBase(x);
		params.EncodeElement(true, y, privateKey+StaticPrivateKeyLength());
	}

	void GenerateEphemeralPublicKey(RandomNumberGenerator &rng, const byte *privateKey, byte *publicKey) const
	{
		CRYPTOPP_UNUSED(rng);
		memcpy(publicKey, privateKey+StaticPrivateKeyLength(), EphemeralPublicKeyLength());
	}

	bool Agree(byte *agreedValue,
		const byte *staticPrivateKey, const byte *ephemeralPrivateKey, 
		const byte *staticOtherPublicKey, const byte *ephemeralOtherPublicKey,
		bool validateStaticOtherPublicKey=true) const
	{
		try
		{
			const DL_GroupParameters<Element> &params = GetAbstractGroupParameters();
			Element WW = params.DecodeElement(staticOtherPublicKey, validateStaticOtherPublicKey);
			Element VV = params.DecodeElement(ephemeralOtherPublicKey, true);

			Integer s(staticPrivateKey, StaticPrivateKeyLength());
			Integer u(ephemeralPrivateKey, StaticPrivateKeyLength());
			Element V = params.DecodeElement(ephemeralPrivateKey+StaticPrivateKeyLength(), false);

			const Integer &r = params.GetSubgroupOrder();
			Integer h2 = Integer::Power2((r.BitCount()+1)/2);
			Integer e = ((h2+params.ConvertElementToInteger(V)%h2)*s+u) % r;
			Integer tt = h2 + params.ConvertElementToInteger(VV) % h2;

			if (COFACTOR_OPTION::ToEnum() == NO_COFACTOR_MULTIPLICTION)
			{
				Element P = params.ExponentiateElement(WW, tt);
				P = m_groupParameters.MultiplyElements(P, VV);
				Element R[2];
				const Integer e2[2] = {r, e};
				params.SimultaneousExponentiate(R, P, e2, 2);
				if (!params.IsIdentity(R[0]) || params.IsIdentity(R[1]))
					return false;
				params.EncodeElement(false, R[1], agreedValue);
			}
			else
			{
				const Integer &k = params.GetCofactor();
				if (COFACTOR_OPTION::ToEnum() == COMPATIBLE_COFACTOR_MULTIPLICTION)
					e = ModularArithmetic(r).Divide(e, k);
				Element P = m_groupParameters.CascadeExponentiate(VV, k*e, WW, k*(e*tt%r));
				if (params.IsIdentity(P))
					return false;
				params.EncodeElement(false, P, agreedValue);
			}
		}
		catch (DL_BadElement &)
		{
			return false;
		}
		return true;
	}

private:
	DL_GroupParameters<Element> & AccessAbstractGroupParameters() {return m_groupParameters;}
	const DL_GroupParameters<Element> & GetAbstractGroupParameters() const {return m_groupParameters;}

	GroupParameters m_groupParameters;
};

//! Menezes-Qu-Vanstone in GF(p) with key validation, AKA <a href="http://www.weidai.com/scan-mirror/ka.html#MQV">MQV</a>
typedef MQV_Domain<DL_GroupParameters_GFP_DefaultSafePrime> MQV;

NAMESPACE_END

#endif
