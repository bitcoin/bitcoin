// dh.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile dh.h
//! \brief Classes for Diffie-Hellman key exchange

#ifndef CRYPTOPP_DH_H
#define CRYPTOPP_DH_H

#include "cryptlib.h"
#include "gfpcrypt.h"

NAMESPACE_BEGIN(CryptoPP)

//! ,
template <class GROUP_PARAMETERS, class COFACTOR_OPTION = CPP_TYPENAME GROUP_PARAMETERS::DefaultCofactorOption>
class DH_Domain : public DL_SimpleKeyAgreementDomainBase<typename GROUP_PARAMETERS::Element>
{
	typedef DL_SimpleKeyAgreementDomainBase<typename GROUP_PARAMETERS::Element> Base;

public:
	typedef GROUP_PARAMETERS GroupParameters;
	typedef typename GroupParameters::Element Element;
	typedef DL_KeyAgreementAlgorithm_DH<Element, COFACTOR_OPTION> DH_Algorithm;
	typedef DH_Domain<GROUP_PARAMETERS, COFACTOR_OPTION> Domain;

	DH_Domain() {}

	DH_Domain(const GroupParameters &params)
		: m_groupParameters(params) {}

	DH_Domain(BufferedTransformation &bt)
		{m_groupParameters.BERDecode(bt);}

	template <class T2>
	DH_Domain(RandomNumberGenerator &v1, const T2 &v2)
		{m_groupParameters.Initialize(v1, v2);}
	
	template <class T2, class T3>
	DH_Domain(RandomNumberGenerator &v1, const T2 &v2, const T3 &v3)
		{m_groupParameters.Initialize(v1, v2, v3);}
	
	template <class T2, class T3, class T4>
	DH_Domain(RandomNumberGenerator &v1, const T2 &v2, const T3 &v3, const T4 &v4)
		{m_groupParameters.Initialize(v1, v2, v3, v4);}

	template <class T1, class T2>
	DH_Domain(const T1 &v1, const T2 &v2)
		{m_groupParameters.Initialize(v1, v2);}
	
	template <class T1, class T2, class T3>
	DH_Domain(const T1 &v1, const T2 &v2, const T3 &v3)
		{m_groupParameters.Initialize(v1, v2, v3);}
	
	template <class T1, class T2, class T3, class T4>
	DH_Domain(const T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4)
		{m_groupParameters.Initialize(v1, v2, v3, v4);}

	const GroupParameters & GetGroupParameters() const {return m_groupParameters;}
	GroupParameters & AccessGroupParameters() {return m_groupParameters;}

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
