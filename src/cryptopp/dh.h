// dh.h - written and placed in the public domain by Wei Dai

//! \file dh.h
//! \brief Classes for Diffie-Hellman key exchange

#ifndef CRYPTOPP_DH_H
#define CRYPTOPP_DH_H

#include "cryptlib.h"
#include "gfpcrypt.h"
#include "algebra.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class DH_Domain
//! \brief Diffie-Hellman domain
//! \tparam GROUP_PARAMETERS group parameters
//! \tparam COFACTOR_OPTION \ref CofactorMultiplicationOption "cofactor multiplication option"
//! \details A Diffie-Hellman domain is a set of parameters that must be shared
//!   by two parties in a key agreement protocol, along with the algorithms
//!   for generating key pairs and deriving agreed values.
template <class GROUP_PARAMETERS, class COFACTOR_OPTION = CPP_TYPENAME GROUP_PARAMETERS::DefaultCofactorOption>
class DH_Domain : public DL_SimpleKeyAgreementDomainBase<typename GROUP_PARAMETERS::Element>
{
	typedef DL_SimpleKeyAgreementDomainBase<typename GROUP_PARAMETERS::Element> Base;

public:
	typedef GROUP_PARAMETERS GroupParameters;
	typedef typename GroupParameters::Element Element;
	typedef DL_KeyAgreementAlgorithm_DH<Element, COFACTOR_OPTION> DH_Algorithm;
	typedef DH_Domain<GROUP_PARAMETERS, COFACTOR_OPTION> Domain;

	//! \brief Construct a Diffie-Hellman domain
	DH_Domain() {}

	//! \brief Construct a Diffie-Hellman domain
	//! \param params group parameters and options
	DH_Domain(const GroupParameters &params)
		: m_groupParameters(params) {}

	//! \brief Construct a Diffie-Hellman domain
	//! \param bt BufferedTransformation with group parameters and options
	DH_Domain(BufferedTransformation &bt)
		{m_groupParameters.BERDecode(bt);}

	//! \brief Construct a Diffie-Hellman domain
	//! \tparam T2 template parameter used as a constructor parameter
	//! \param v1 RandomNumberGenerator derived class
	//! \param v2 second parameter
	//! \details v1 and v2 are passed directly to the GROUP_PARAMETERS object.
	template <class T2>
	DH_Domain(RandomNumberGenerator &v1, const T2 &v2)
		{m_groupParameters.Initialize(v1, v2);}

	//! \brief Construct a Diffie-Hellman domain
	//! \tparam T2 template parameter used as a constructor parameter
	//! \tparam T3 template parameter used as a constructor parameter
	//! \param v1 RandomNumberGenerator derived class
	//! \param v2 second parameter
	//! \param v3 third parameter
	//! \details v1, v2 and v3 are passed directly to the GROUP_PARAMETERS object.
	template <class T2, class T3>
	DH_Domain(RandomNumberGenerator &v1, const T2 &v2, const T3 &v3)
		{m_groupParameters.Initialize(v1, v2, v3);}

	//! \brief Construct a Diffie-Hellman domain
	//! \tparam T2 template parameter used as a constructor parameter
	//! \tparam T3 template parameter used as a constructor parameter
	//! \tparam T4 template parameter used as a constructor parameter
	//! \param v1 RandomNumberGenerator derived class
	//! \param v2 second parameter
	//! \param v3 third parameter
	//! \param v4 fourth parameter
	//! \details v1, v2, v3 and v4 are passed directly to the GROUP_PARAMETERS object.
	template <class T2, class T3, class T4>
	DH_Domain(RandomNumberGenerator &v1, const T2 &v2, const T3 &v3, const T4 &v4)
		{m_groupParameters.Initialize(v1, v2, v3, v4);}

	//! \brief Construct a Diffie-Hellman domain
	//! \tparam T1 template parameter used as a constructor parameter
	//! \tparam T2 template parameter used as a constructor parameter
	//! \param v1 first parameter
	//! \param v2 second parameter
	//! \details v1 and v2 are passed directly to the GROUP_PARAMETERS object.
	template <class T1, class T2>
	DH_Domain(const T1 &v1, const T2 &v2)
		{m_groupParameters.Initialize(v1, v2);}

	//! \brief Construct a Diffie-Hellman domain
	//! \tparam T1 template parameter used as a constructor parameter
	//! \tparam T2 template parameter used as a constructor parameter
	//! \tparam T3 template parameter used as a constructor parameter
	//! \param v1 first parameter
	//! \param v2 second parameter
	//! \param v3 third parameter
	//! \details v1, v2 and v3 are passed directly to the GROUP_PARAMETERS object.
	template <class T1, class T2, class T3>
	DH_Domain(const T1 &v1, const T2 &v2, const T3 &v3)
		{m_groupParameters.Initialize(v1, v2, v3);}

	//! \brief Construct a Diffie-Hellman domain
	//! \tparam T1 template parameter used as a constructor parameter
	//! \tparam T2 template parameter used as a constructor parameter
	//! \tparam T3 template parameter used as a constructor parameter
	//! \tparam T4 template parameter used as a constructor parameter
	//! \param v1 first parameter
	//! \param v2 second parameter
	//! \param v3 third parameter
	//! \param v4 fourth parameter
	//! \details v1, v2, v3 and v4 are passed directly to the GROUP_PARAMETERS object.
	template <class T1, class T2, class T3, class T4>
	DH_Domain(const T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4)
		{m_groupParameters.Initialize(v1, v2, v3, v4);}

	//! \brief Retrieves the group parameters for this domain
	//! \return the group parameters for this domain as a const reference
	const GroupParameters & GetGroupParameters() const {return m_groupParameters;}
	//! \brief Retrieves the group parameters for this domain
	//! \return the group parameters for this domain as a non-const reference
	GroupParameters & AccessGroupParameters() {return m_groupParameters;}

	//! \brief Generate a public key from a private key in this domain
	//! \param rng RandomNumberGenerator derived class
	//! \param privateKey byte buffer with the previously generated private key
	//! \param publicKey byte buffer for the generated public key in this domain
	//! \details If using a FIPS 140-2 validated library on Windows, then this class will perform
	//!   a self test to ensure the key pair is pairwise consistent. Non-FIPS and non-Windows
	//!   builds of the library do not provide FIPS validated cryptography, so the code should be
	//!   removed by the optimizer.
	//! \pre <tt>COUNTOF(publicKey) == PublicKeyLength()</tt>
	void GeneratePublicKey(RandomNumberGenerator &rng, const byte *privateKey, byte *publicKey) const
	{
		Base::GeneratePublicKey(rng, privateKey, publicKey);

		if (FIPS_140_2_ComplianceEnabled())
		{
			SecByteBlock privateKey2(this->PrivateKeyLength());
			this->GeneratePrivateKey(rng, privateKey2);

			SecByteBlock publicKey2(this->PublicKeyLength());
			Base::GeneratePublicKey(rng, privateKey2, publicKey2);

			SecByteBlock agreedValue(this->AgreedValueLength()), agreedValue2(this->AgreedValueLength());
			bool agreed1 = this->Agree(agreedValue, privateKey, publicKey2);
			bool agreed2 = this->Agree(agreedValue2, privateKey2, publicKey);

			if (!agreed1 || !agreed2 || agreedValue != agreedValue2)
				throw SelfTestFailure(this->AlgorithmName() + ": pairwise consistency test failed");
		}
	}

	static std::string CRYPTOPP_API StaticAlgorithmName()
		{return GroupParameters::StaticAlgorithmNamePrefix() + DH_Algorithm::StaticAlgorithmName();}
	std::string AlgorithmName() const {return StaticAlgorithmName();}

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DH_Domain() {}
#endif

private:
	const DL_KeyAgreementAlgorithm<Element> & GetKeyAgreementAlgorithm() const
		{return Singleton<DH_Algorithm>().Ref();}
	DL_GroupParameters<Element> & AccessAbstractGroupParameters()
		{return m_groupParameters;}

	GroupParameters m_groupParameters;
};

CRYPTOPP_DLL_TEMPLATE_CLASS DH_Domain<DL_GroupParameters_GFP_DefaultSafePrime>;

//! <a href="http://www.weidai.com/scan-mirror/ka.html#DH">Diffie-Hellman</a> in GF(p) with key validation
typedef DH_Domain<DL_GroupParameters_GFP_DefaultSafePrime> DH;

NAMESPACE_END

#endif
