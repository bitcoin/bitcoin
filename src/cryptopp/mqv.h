// mqv.h - written and placed in the public domain by Wei Dai

//! \file mqv.h
//! \brief Classes for Menezes–Qu–Vanstone (MQV) key agreement

#ifndef CRYPTOPP_MQV_H
#define CRYPTOPP_MQV_H

#include "cryptlib.h"
#include "gfpcrypt.h"
#include "modarith.h"
#include "integer.h"
#include "algebra.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class MQV_Domain
//! \brief MQV domain for performing authenticated key agreement
//! \tparam GROUP_PARAMETERS doamin parameters
//! \tparam COFACTOR_OPTION cofactor option
//! \details GROUP_PARAMETERS paramters include the curve coefcients and the base point.
//!   Binary curves use a polynomial to represent its characteristic, while prime curves
//!   use a prime number.
//! \sa MQV, HMQV, FHMQV, and AuthenticatedKeyAgreementDomain
template <class GROUP_PARAMETERS, class COFACTOR_OPTION = CPP_TYPENAME GROUP_PARAMETERS::DefaultCofactorOption>
class MQV_Domain : public AuthenticatedKeyAgreementDomain
{
public:
	typedef GROUP_PARAMETERS GroupParameters;
	typedef typename GroupParameters::Element Element;
	typedef MQV_Domain<GROUP_PARAMETERS, COFACTOR_OPTION> Domain;

	//! \brief Construct a MQV domain
	MQV_Domain() {}

	//! \brief Construct a MQV domain
	//! \param params group parameters and options
	MQV_Domain(const GroupParameters &params)
		: m_groupParameters(params) {}

	//! \brief Construct a MQV domain
	//! \param bt BufferedTransformation with group parameters and options
	MQV_Domain(BufferedTransformation &bt)
		{m_groupParameters.BERDecode(bt);}

	//! \brief Construct a MQV domain
	//! \tparam T1 template parameter used as a constructor parameter
	//! \tparam T2 template parameter used as a constructor parameter
	//! \param v1 first parameter
	//! \param v2 second parameter
	//! \details v1 and v2 are passed directly to the GROUP_PARAMETERS object.
	template <class T1, class T2>
	MQV_Domain(T1 v1, T2 v2)
		{m_groupParameters.Initialize(v1, v2);}

	//! \brief Construct a MQV domain
	//! \tparam T1 template parameter used as a constructor parameter
	//! \tparam T2 template parameter used as a constructor parameter
	//! \tparam T3 template parameter used as a constructor parameter
	//! \param v1 first parameter
	//! \param v2 second parameter
	//! \param v3 third parameter
	//! \details v1, v2 and v3 are passed directly to the GROUP_PARAMETERS object.
	template <class T1, class T2, class T3>
	MQV_Domain(T1 v1, T2 v2, T3 v3)
		{m_groupParameters.Initialize(v1, v2, v3);}

	//! \brief Construct a MQV domain
	//! \tparam T1 template parameter used as a constructor parameter
	//! \tparam T2 template parameter used as a constructor parameter
	//! \tparam T3 template parameter used as a constructor parameter
	//! \tparam T4 template parameter used as a constructor parameter
	//! \param v1 first parameter
	//! \param v2 second parameter
	//! \param v3 third parameter
	//! \param v4 third parameter
	//! \details v1, v2, v3 and v4 are passed directly to the GROUP_PARAMETERS object.
	template <class T1, class T2, class T3, class T4>
	MQV_Domain(T1 v1, T2 v2, T3 v3, T4 v4)
		{m_groupParameters.Initialize(v1, v2, v3, v4);}

	//! \brief Retrieves the group parameters for this domain
	//! \return the group parameters for this domain as a const reference
	const GroupParameters & GetGroupParameters() const {return m_groupParameters;}

	//! \brief Retrieves the group parameters for this domain
	//! \return the group parameters for this domain as a non-const reference
	GroupParameters & AccessGroupParameters() {return m_groupParameters;}

	//! \brief Retrieves the crypto parameters for this domain
	//! \return the crypto parameters for this domain as a non-const reference
	CryptoParameters & AccessCryptoParameters() {return AccessAbstractGroupParameters();}

	//! \brief Provides the size of the agreed value
	//! \return size of agreed value produced  in this domain
	//! \details The length is calculated using <tt>GetEncodedElementSize(false)</tt>, which means the
	//!   element is encoded in a non-reversible format. A non-reversible format means its a raw byte array,
	//!   and it lacks presentation format like an ASN.1 BIT_STRING or OCTET_STRING.
	unsigned int AgreedValueLength() const {return GetAbstractGroupParameters().GetEncodedElementSize(false);}

	//! \brief Provides the size of the static private key
	//! \return size of static private keys in this domain
	//! \details The length is calculated using the byte count of the subgroup order.
	unsigned int StaticPrivateKeyLength() const {return GetAbstractGroupParameters().GetSubgroupOrder().ByteCount();}

	//! \brief Provides the size of the static public key
	//! \return size of static public keys in this domain
	//! \details The length is calculated using <tt>GetEncodedElementSize(true)</tt>, which means the
	//!   element is encoded in a reversible format. A reversible format means it has a presentation format,
	//!   and its an ANS.1 encoded element or point.
	unsigned int StaticPublicKeyLength() const {return GetAbstractGroupParameters().GetEncodedElementSize(true);}

	//! \brief Generate static private key in this domain
	//! \param rng a RandomNumberGenerator derived class
	//! \param privateKey a byte buffer for the generated private key in this domain
	//! \details The private key is a random scalar used as an exponent in the range <tt>[1,MaxExponent()]</tt>.
	//! \pre <tt>COUNTOF(privateKey) == PrivateStaticKeyLength()</tt>
	void GenerateStaticPrivateKey(RandomNumberGenerator &rng, byte *privateKey) const
	{
		Integer x(rng, Integer::One(), GetAbstractGroupParameters().GetMaxExponent());
		x.Encode(privateKey, StaticPrivateKeyLength());
	}

	//! \brief Generate a static public key from a private key in this domain
	//! \param rng a RandomNumberGenerator derived class
	//! \param privateKey a byte buffer with the previously generated private key
	//! \param publicKey a byte buffer for the generated public key in this domain
	//! \details The public key is an element or point on the curve, and its stored in a revrsible format.
	//!    A reversible format means it has a presentation format, and its an ANS.1 encoded element or point.
	//! \pre <tt>COUNTOF(publicKey) == PublicStaticKeyLength()</tt>
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
//! \sa MQV, HMQV_Domain, FHMQV_Domain, AuthenticatedKeyAgreementDomain
typedef MQV_Domain<DL_GroupParameters_GFP_DefaultSafePrime> MQV;

NAMESPACE_END

#endif
