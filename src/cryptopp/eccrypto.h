// eccrypto.h - written and placed in the public domain by Wei Dai

//! \file eccrypto.h
//! \brief Classes and functions for Elliptic Curves over prime and binary fields

#ifndef CRYPTOPP_ECCRYPTO_H
#define CRYPTOPP_ECCRYPTO_H

#include "config.h"
#include "cryptlib.h"
#include "pubkey.h"
#include "integer.h"
#include "asn.h"
#include "hmac.h"
#include "sha.h"
#include "gfpcrypt.h"
#include "dh.h"
#include "mqv.h"
#include "ecp.h"
#include "ec2n.h"

NAMESPACE_BEGIN(CryptoPP)

//! Elliptic Curve Parameters
/*! This class corresponds to the ASN.1 sequence of the same name
    in ANSI X9.62 (also SEC 1).
*/
template <class EC>
class DL_GroupParameters_EC : public DL_GroupParametersImpl<EcPrecomputation<EC> >
{
	typedef DL_GroupParameters_EC<EC> ThisClass;

public:
	typedef EC EllipticCurve;
	typedef typename EllipticCurve::Point Point;
	typedef Point Element;
	typedef IncompatibleCofactorMultiplication DefaultCofactorOption;

	DL_GroupParameters_EC() : m_compress(false), m_encodeAsOID(false) {}
	DL_GroupParameters_EC(const OID &oid)
		: m_compress(false), m_encodeAsOID(false) {Initialize(oid);}
	DL_GroupParameters_EC(const EllipticCurve &ec, const Point &G, const Integer &n, const Integer &k = Integer::Zero())
		: m_compress(false), m_encodeAsOID(false) {Initialize(ec, G, n, k);}
	DL_GroupParameters_EC(BufferedTransformation &bt)
		: m_compress(false), m_encodeAsOID(false) {BERDecode(bt);}

	void Initialize(const EllipticCurve &ec, const Point &G, const Integer &n, const Integer &k = Integer::Zero())
	{
		this->m_groupPrecomputation.SetCurve(ec);
		this->SetSubgroupGenerator(G);
		m_n = n;
		m_k = k;
	}
	void Initialize(const OID &oid);

	// NameValuePairs
	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;
	void AssignFrom(const NameValuePairs &source);

	// GeneratibleCryptoMaterial interface
	//! this implementation doesn't actually generate a curve, it just initializes the parameters with existing values
	/*! parameters: (Curve, SubgroupGenerator, SubgroupOrder, Cofactor (optional)), or (GroupOID) */
	void GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &alg);

	// DL_GroupParameters
	const DL_FixedBasePrecomputation<Element> & GetBasePrecomputation() const {return this->m_gpc;}
	DL_FixedBasePrecomputation<Element> & AccessBasePrecomputation() {return this->m_gpc;}
	const Integer & GetSubgroupOrder() const {return m_n;}
	Integer GetCofactor() const;
	bool ValidateGroup(RandomNumberGenerator &rng, unsigned int level) const;
	bool ValidateElement(unsigned int level, const Element &element, const DL_FixedBasePrecomputation<Element> *precomp) const;
	bool FastSubgroupCheckAvailable() const {return false;}
	void EncodeElement(bool reversible, const Element &element, byte *encoded) const
	{
		if (reversible)
			GetCurve().EncodePoint(encoded, element, m_compress);
		else
			element.x.Encode(encoded, GetEncodedElementSize(false));
	}
	virtual unsigned int GetEncodedElementSize(bool reversible) const
	{
		if (reversible)
			return GetCurve().EncodedPointSize(m_compress);
		else
			return GetCurve().GetField().MaxElementByteLength();
	}
	Element DecodeElement(const byte *encoded, bool checkForGroupMembership) const
	{
		Point result;
		if (!GetCurve().DecodePoint(result, encoded, GetEncodedElementSize(true)))
			throw DL_BadElement();
		if (checkForGroupMembership && !ValidateElement(1, result, NULL))
			throw DL_BadElement();
		return result;
	}
	Integer ConvertElementToInteger(const Element &element) const;
	Integer GetMaxExponent() const {return GetSubgroupOrder()-1;}
	bool IsIdentity(const Element &element) const {return element.identity;}
	void SimultaneousExponentiate(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const;
	static std::string CRYPTOPP_API StaticAlgorithmNamePrefix() {return "EC";}

	// ASN1Key
	OID GetAlgorithmID() const;

	// used by MQV
	Element MultiplyElements(const Element &a, const Element &b) const;
	Element CascadeExponentiate(const Element &element1, const Integer &exponent1, const Element &element2, const Integer &exponent2) const;

	// non-inherited

	// enumerate OIDs for recommended parameters, use OID() to get first one
	static OID CRYPTOPP_API GetNextRecommendedParametersOID(const OID &oid);

	void BERDecode(BufferedTransformation &bt);
	void DEREncode(BufferedTransformation &bt) const;

	void SetPointCompression(bool compress) {m_compress = compress;}
	bool GetPointCompression() const {return m_compress;}

	void SetEncodeAsOID(bool encodeAsOID) {m_encodeAsOID = encodeAsOID;}
	bool GetEncodeAsOID() const {return m_encodeAsOID;}

	const EllipticCurve& GetCurve() const {return this->m_groupPrecomputation.GetCurve();}

	bool operator==(const ThisClass &rhs) const
		{return this->m_groupPrecomputation.GetCurve() == rhs.m_groupPrecomputation.GetCurve() && this->m_gpc.GetBase(this->m_groupPrecomputation) == rhs.m_gpc.GetBase(rhs.m_groupPrecomputation);}

#ifdef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY
	const Point& GetBasePoint() const {return this->GetSubgroupGenerator();}
	const Integer& GetBasePointOrder() const {return this->GetSubgroupOrder();}
	void LoadRecommendedParameters(const OID &oid) {Initialize(oid);}
#endif
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_GroupParameters_EC() {}
#endif

protected:
	unsigned int FieldElementLength() const {return GetCurve().GetField().MaxElementByteLength();}
	unsigned int ExponentLength() const {return m_n.ByteCount();}

	OID m_oid;			// set if parameters loaded from a recommended curve
	Integer m_n;		// order of base point
	mutable Integer m_k;		// cofactor
	mutable bool m_compress, m_encodeAsOID;		// presentation details
};

//! EC public key
template <class EC>
class DL_PublicKey_EC : public DL_PublicKeyImpl<DL_GroupParameters_EC<EC> >
{
public:
	typedef typename EC::Point Element;

	void Initialize(const DL_GroupParameters_EC<EC> &params, const Element &Q)
		{this->AccessGroupParameters() = params; this->SetPublicElement(Q);}
	void Initialize(const EC &ec, const Element &G, const Integer &n, const Element &Q)
		{this->AccessGroupParameters().Initialize(ec, G, n); this->SetPublicElement(Q);}

	// X509PublicKey
	void BERDecodePublicKey(BufferedTransformation &bt, bool parametersPresent, size_t size);
	void DEREncodePublicKey(BufferedTransformation &bt) const;
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_PublicKey_EC() {}
#endif
};

//! EC private key
template <class EC>
class DL_PrivateKey_EC : public DL_PrivateKeyImpl<DL_GroupParameters_EC<EC> >
{
public:
	typedef typename EC::Point Element;

	void Initialize(const DL_GroupParameters_EC<EC> &params, const Integer &x)
		{this->AccessGroupParameters() = params; this->SetPrivateExponent(x);}
	void Initialize(const EC &ec, const Element &G, const Integer &n, const Integer &x)
		{this->AccessGroupParameters().Initialize(ec, G, n); this->SetPrivateExponent(x);}
	void Initialize(RandomNumberGenerator &rng, const DL_GroupParameters_EC<EC> &params)
		{this->GenerateRandom(rng, params);}
	void Initialize(RandomNumberGenerator &rng, const EC &ec, const Element &G, const Integer &n)
		{this->GenerateRandom(rng, DL_GroupParameters_EC<EC>(ec, G, n));}

	// PKCS8PrivateKey
	void BERDecodePrivateKey(BufferedTransformation &bt, bool parametersPresent, size_t size);
	void DEREncodePrivateKey(BufferedTransformation &bt) const;
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_PrivateKey_EC() {}
#endif
};

//! Elliptic Curve Diffie-Hellman, AKA <a href="http://www.weidai.com/scan-mirror/ka.html#ECDH">ECDH</a>
template <class EC, class COFACTOR_OPTION = CPP_TYPENAME DL_GroupParameters_EC<EC>::DefaultCofactorOption>
struct ECDH
{
	typedef DH_Domain<DL_GroupParameters_EC<EC>, COFACTOR_OPTION> Domain;
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~ECDH() {}
#endif
};

/// Elliptic Curve Menezes-Qu-Vanstone, AKA <a href="http://www.weidai.com/scan-mirror/ka.html#ECMQV">ECMQV</a>
template <class EC, class COFACTOR_OPTION = CPP_TYPENAME DL_GroupParameters_EC<EC>::DefaultCofactorOption>
struct ECMQV
{
	typedef MQV_Domain<DL_GroupParameters_EC<EC>, COFACTOR_OPTION> Domain;
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~ECMQV() {}
#endif
};

//! EC keys
template <class EC>
struct DL_Keys_EC
{
	typedef DL_PublicKey_EC<EC> PublicKey;
	typedef DL_PrivateKey_EC<EC> PrivateKey;
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_Keys_EC() {}
#endif
};

template <class EC, class H>
struct ECDSA;

//! ECDSA keys
template <class EC>
struct DL_Keys_ECDSA
{
	typedef DL_PublicKey_EC<EC> PublicKey;
	typedef DL_PrivateKey_WithSignaturePairwiseConsistencyTest<DL_PrivateKey_EC<EC>, ECDSA<EC, SHA256> > PrivateKey;
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_Keys_ECDSA() {}
#endif
};

//! ECDSA algorithm
template <class EC>
class DL_Algorithm_ECDSA : public DL_Algorithm_GDSA<typename EC::Point>
{
public:
	static const char * CRYPTOPP_API StaticAlgorithmName() {return "ECDSA";}
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_Algorithm_ECDSA() {}
#endif
};

//! ECNR algorithm
template <class EC>
class DL_Algorithm_ECNR : public DL_Algorithm_NR<typename EC::Point>
{
public:
	static const char * CRYPTOPP_API StaticAlgorithmName() {return "ECNR";}
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_Algorithm_ECNR() {}
#endif
};

//! <a href="http://www.weidai.com/scan-mirror/sig.html#ECDSA">ECDSA</a>
template <class EC, class H>
struct ECDSA : public DL_SS<DL_Keys_ECDSA<EC>, DL_Algorithm_ECDSA<EC>, DL_SignatureMessageEncodingMethod_DSA, H>
{
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~ECDSA() {}
#endif
};

//! ECNR
template <class EC, class H = SHA>
struct ECNR : public DL_SS<DL_Keys_EC<EC>, DL_Algorithm_ECNR<EC>, DL_SignatureMessageEncodingMethod_NR, H>
{
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~ECNR() {}
#endif
};

//! Elliptic Curve Integrated Encryption Scheme, AKA <a href="http://www.weidai.com/scan-mirror/ca.html#ECIES">ECIES</a>
/*! Default to (NoCofactorMultiplication and DHAES_MODE = false) for compatibilty with SEC1 and Crypto++ 4.2.
	The combination of (IncompatibleCofactorMultiplication and DHAES_MODE = true) is recommended for best
	efficiency and security. */
template <class EC, class COFACTOR_OPTION = NoCofactorMultiplication, bool DHAES_MODE = false>
struct ECIES
	: public DL_ES<
		DL_Keys_EC<EC>,
		DL_KeyAgreementAlgorithm_DH<typename EC::Point, COFACTOR_OPTION>,
		DL_KeyDerivationAlgorithm_P1363<typename EC::Point, DHAES_MODE, P1363_KDF2<SHA1> >,
		DL_EncryptionAlgorithm_Xor<HMAC<SHA1>, DHAES_MODE>,
		ECIES<EC> >
{
	static std::string CRYPTOPP_API StaticAlgorithmName() {return "ECIES";}	// TODO: fix this after name is standardized
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~ECIES() {}
#endif
	
#if (CRYPTOPP_GCC_VERSION >= 40500) || (CRYPTOPP_CLANG_VERSION >= 30000) 
} __attribute__((deprecated ("ECIES will be changing in the near future due to (1) an implementation bug and (2) an interop issue.")));
#elif (CRYPTOPP_GCC_VERSION )
} __attribute__((deprecated));
#else
};
#endif

NAMESPACE_END

#ifdef CRYPTOPP_MANUALLY_INSTANTIATE_TEMPLATES
#include "eccrypto.cpp"
#endif

NAMESPACE_BEGIN(CryptoPP)

CRYPTOPP_DLL_TEMPLATE_CLASS DL_GroupParameters_EC<ECP>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_GroupParameters_EC<EC2N>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PublicKeyImpl<DL_GroupParameters_EC<ECP> >;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PublicKeyImpl<DL_GroupParameters_EC<EC2N> >;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PublicKey_EC<ECP>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PublicKey_EC<EC2N>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PrivateKeyImpl<DL_GroupParameters_EC<ECP> >;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PrivateKeyImpl<DL_GroupParameters_EC<EC2N> >;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PrivateKey_EC<ECP>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PrivateKey_EC<EC2N>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_Algorithm_GDSA<ECP::Point>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_Algorithm_GDSA<EC2N::Point>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PrivateKey_WithSignaturePairwiseConsistencyTest<DL_PrivateKey_EC<ECP>, ECDSA<ECP, SHA256> >;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PrivateKey_WithSignaturePairwiseConsistencyTest<DL_PrivateKey_EC<EC2N>, ECDSA<EC2N, SHA256> >;

NAMESPACE_END

#endif
#ifndef CRYPTOPP_ECCRYPTO_H
#define CRYPTOPP_ECCRYPTO_H

/*! \file
*/

#include "cryptlib.h"
#include "pubkey.h"
#include "integer.h"
#include "asn.h"
#include "hmac.h"
#include "sha.h"
#include "gfpcrypt.h"
#include "dh.h"
#include "mqv.h"
#include "ecp.h"
#include "ec2n.h"

NAMESPACE_BEGIN(CryptoPP)

//! Elliptic Curve Parameters
/*! This class corresponds to the ASN.1 sequence of the same name
    in ANSI X9.62 (also SEC 1).
*/
template <class EC>
class DL_GroupParameters_EC : public DL_GroupParametersImpl<EcPrecomputation<EC> >
{
	typedef DL_GroupParameters_EC<EC> ThisClass;

public:
	typedef EC EllipticCurve;
	typedef typename EllipticCurve::Point Point;
	typedef Point Element;
	typedef IncompatibleCofactorMultiplication DefaultCofactorOption;

	DL_GroupParameters_EC() : m_compress(false), m_encodeAsOID(false) {}
	DL_GroupParameters_EC(const OID &oid)
		: m_compress(false), m_encodeAsOID(false) {Initialize(oid);}
	DL_GroupParameters_EC(const EllipticCurve &ec, const Point &G, const Integer &n, const Integer &k = Integer::Zero())
		: m_compress(false), m_encodeAsOID(false) {Initialize(ec, G, n, k);}
	DL_GroupParameters_EC(BufferedTransformation &bt)
		: m_compress(false), m_encodeAsOID(false) {BERDecode(bt);}

	void Initialize(const EllipticCurve &ec, const Point &G, const Integer &n, const Integer &k = Integer::Zero())
	{
		this->m_groupPrecomputation.SetCurve(ec);
		this->SetSubgroupGenerator(G);
		m_n = n;
		m_k = k;
	}
	void Initialize(const OID &oid);

	// NameValuePairs
	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;
	void AssignFrom(const NameValuePairs &source);

	// GeneratibleCryptoMaterial interface
	//! this implementation doesn't actually generate a curve, it just initializes the parameters with existing values
	/*! parameters: (Curve, SubgroupGenerator, SubgroupOrder, Cofactor (optional)), or (GroupOID) */
	void GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &alg);

	// DL_GroupParameters
	const DL_FixedBasePrecomputation<Element> & GetBasePrecomputation() const {return this->m_gpc;}
	DL_FixedBasePrecomputation<Element> & AccessBasePrecomputation() {return this->m_gpc;}
	const Integer & GetSubgroupOrder() const {return m_n;}
	Integer GetCofactor() const;
	bool ValidateGroup(RandomNumberGenerator &rng, unsigned int level) const;
	bool ValidateElement(unsigned int level, const Element &element, const DL_FixedBasePrecomputation<Element> *precomp) const;
	bool FastSubgroupCheckAvailable() const {return false;}
	void EncodeElement(bool reversible, const Element &element, byte *encoded) const
	{
		if (reversible)
			GetCurve().EncodePoint(encoded, element, m_compress);
		else
			element.x.Encode(encoded, GetEncodedElementSize(false));
	}
	virtual unsigned int GetEncodedElementSize(bool reversible) const
	{
		if (reversible)
			return GetCurve().EncodedPointSize(m_compress);
		else
			return GetCurve().GetField().MaxElementByteLength();
	}
	Element DecodeElement(const byte *encoded, bool checkForGroupMembership) const
	{
		Point result;
		if (!GetCurve().DecodePoint(result, encoded, GetEncodedElementSize(true)))
			throw DL_BadElement();
		if (checkForGroupMembership && !ValidateElement(1, result, NULL))
			throw DL_BadElement();
		return result;
	}
	Integer ConvertElementToInteger(const Element &element) const;
	Integer GetMaxExponent() const {return GetSubgroupOrder()-1;}
	bool IsIdentity(const Element &element) const {return element.identity;}
	void SimultaneousExponentiate(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const;
	static std::string CRYPTOPP_API StaticAlgorithmNamePrefix() {return "EC";}

	// ASN1Key
	OID GetAlgorithmID() const;

	// used by MQV
	Element MultiplyElements(const Element &a, const Element &b) const;
	Element CascadeExponentiate(const Element &element1, const Integer &exponent1, const Element &element2, const Integer &exponent2) const;

	// non-inherited

	// enumerate OIDs for recommended parameters, use OID() to get first one
	static OID CRYPTOPP_API GetNextRecommendedParametersOID(const OID &oid);

	void BERDecode(BufferedTransformation &bt);
	void DEREncode(BufferedTransformation &bt) const;

	void SetPointCompression(bool compress) {m_compress = compress;}
	bool GetPointCompression() const {return m_compress;}

	void SetEncodeAsOID(bool encodeAsOID) {m_encodeAsOID = encodeAsOID;}
	bool GetEncodeAsOID() const {return m_encodeAsOID;}

	const EllipticCurve& GetCurve() const {return this->m_groupPrecomputation.GetCurve();}

	bool operator==(const ThisClass &rhs) const
		{return this->m_groupPrecomputation.GetCurve() == rhs.m_groupPrecomputation.GetCurve() && this->m_gpc.GetBase(this->m_groupPrecomputation) == rhs.m_gpc.GetBase(rhs.m_groupPrecomputation);}

#ifdef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY
	const Point& GetBasePoint() const {return this->GetSubgroupGenerator();}
	const Integer& GetBasePointOrder() const {return this->GetSubgroupOrder();}
	void LoadRecommendedParameters(const OID &oid) {Initialize(oid);}
#endif
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_GroupParameters_EC() {}
#endif

protected:
	unsigned int FieldElementLength() const {return GetCurve().GetField().MaxElementByteLength();}
	unsigned int ExponentLength() const {return m_n.ByteCount();}

	OID m_oid;			// set if parameters loaded from a recommended curve
	Integer m_n;		// order of base point
	mutable Integer m_k;		// cofactor
	mutable bool m_compress, m_encodeAsOID;		// presentation details
};

//! EC public key
template <class EC>
class DL_PublicKey_EC : public DL_PublicKeyImpl<DL_GroupParameters_EC<EC> >
{
public:
	typedef typename EC::Point Element;

	void Initialize(const DL_GroupParameters_EC<EC> &params, const Element &Q)
		{this->AccessGroupParameters() = params; this->SetPublicElement(Q);}
	void Initialize(const EC &ec, const Element &G, const Integer &n, const Element &Q)
		{this->AccessGroupParameters().Initialize(ec, G, n); this->SetPublicElement(Q);}

	// X509PublicKey
	void BERDecodePublicKey(BufferedTransformation &bt, bool parametersPresent, size_t size);
	void DEREncodePublicKey(BufferedTransformation &bt) const;
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_PublicKey_EC() {}
#endif
};

//! EC private key
template <class EC>
class DL_PrivateKey_EC : public DL_PrivateKeyImpl<DL_GroupParameters_EC<EC> >
{
public:
	typedef typename EC::Point Element;

	void Initialize(const DL_GroupParameters_EC<EC> &params, const Integer &x)
		{this->AccessGroupParameters() = params; this->SetPrivateExponent(x);}
	void Initialize(const EC &ec, const Element &G, const Integer &n, const Integer &x)
		{this->AccessGroupParameters().Initialize(ec, G, n); this->SetPrivateExponent(x);}
	void Initialize(RandomNumberGenerator &rng, const DL_GroupParameters_EC<EC> &params)
		{this->GenerateRandom(rng, params);}
	void Initialize(RandomNumberGenerator &rng, const EC &ec, const Element &G, const Integer &n)
		{this->GenerateRandom(rng, DL_GroupParameters_EC<EC>(ec, G, n));}

	// PKCS8PrivateKey
	void BERDecodePrivateKey(BufferedTransformation &bt, bool parametersPresent, size_t size);
	void DEREncodePrivateKey(BufferedTransformation &bt) const;
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_PrivateKey_EC() {}
#endif
};

//! Elliptic Curve Diffie-Hellman, AKA <a href="http://www.weidai.com/scan-mirror/ka.html#ECDH">ECDH</a>
template <class EC, class COFACTOR_OPTION = CPP_TYPENAME DL_GroupParameters_EC<EC>::DefaultCofactorOption>
struct ECDH
{
	typedef DH_Domain<DL_GroupParameters_EC<EC>, COFACTOR_OPTION> Domain;
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~ECDH() {}
#endif
};

/// Elliptic Curve Menezes-Qu-Vanstone, AKA <a href="http://www.weidai.com/scan-mirror/ka.html#ECMQV">ECMQV</a>
template <class EC, class COFACTOR_OPTION = CPP_TYPENAME DL_GroupParameters_EC<EC>::DefaultCofactorOption>
struct ECMQV
{
	typedef MQV_Domain<DL_GroupParameters_EC<EC>, COFACTOR_OPTION> Domain;
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~ECMQV() {}
#endif
};

//! EC keys
template <class EC>
struct DL_Keys_EC
{
	typedef DL_PublicKey_EC<EC> PublicKey;
	typedef DL_PrivateKey_EC<EC> PrivateKey;
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_Keys_EC() {}
#endif
};

template <class EC, class H>
struct ECDSA;

//! ECDSA keys
template <class EC>
struct DL_Keys_ECDSA
{
	typedef DL_PublicKey_EC<EC> PublicKey;
	typedef DL_PrivateKey_WithSignaturePairwiseConsistencyTest<DL_PrivateKey_EC<EC>, ECDSA<EC, SHA256> > PrivateKey;
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_Keys_ECDSA() {}
#endif
};

//! ECDSA algorithm
template <class EC>
class DL_Algorithm_ECDSA : public DL_Algorithm_GDSA<typename EC::Point>
{
public:
	static const char * CRYPTOPP_API StaticAlgorithmName() {return "ECDSA";}
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_Algorithm_ECDSA() {}
#endif
};

//! ECNR algorithm
template <class EC>
class DL_Algorithm_ECNR : public DL_Algorithm_NR<typename EC::Point>
{
public:
	static const char * CRYPTOPP_API StaticAlgorithmName() {return "ECNR";}
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~DL_Algorithm_ECNR() {}
#endif
};

//! <a href="http://www.weidai.com/scan-mirror/sig.html#ECDSA">ECDSA</a>
template <class EC, class H>
struct ECDSA : public DL_SS<DL_Keys_ECDSA<EC>, DL_Algorithm_ECDSA<EC>, DL_SignatureMessageEncodingMethod_DSA, H>
{
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~ECDSA() {}
#endif
};

//! ECNR
template <class EC, class H = SHA>
struct ECNR : public DL_SS<DL_Keys_EC<EC>, DL_Algorithm_ECNR<EC>, DL_SignatureMessageEncodingMethod_NR, H>
{
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~ECNR() {}
#endif
};

//! Elliptic Curve Integrated Encryption Scheme, AKA <a href="http://www.weidai.com/scan-mirror/ca.html#ECIES">ECIES</a>
/*! Default to (NoCofactorMultiplication and DHAES_MODE = false) for compatibilty with SEC1 and Crypto++ 4.2.
	The combination of (IncompatibleCofactorMultiplication and DHAES_MODE = true) is recommended for best
	efficiency and security. */
template <class EC, class COFACTOR_OPTION = NoCofactorMultiplication, bool DHAES_MODE = false>
struct ECIES
	: public DL_ES<
		DL_Keys_EC<EC>,
		DL_KeyAgreementAlgorithm_DH<typename EC::Point, COFACTOR_OPTION>,
		DL_KeyDerivationAlgorithm_P1363<typename EC::Point, DHAES_MODE, P1363_KDF2<SHA1> >,
		DL_EncryptionAlgorithm_Xor<HMAC<SHA1>, DHAES_MODE>,
		ECIES<EC> >
{
	static std::string CRYPTOPP_API StaticAlgorithmName() {return "ECIES";}	// TODO: fix this after name is standardized
	
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~ECIES() {}
#endif
	
#if (CRYPTOPP_GCC_VERSION >= 40300) || (CRYPTOPP_CLANG_VERSION >= 20800)
} __attribute__((deprecated ("ECIES will be changing in the near future due to (1) an implementation bug and (2) an interop issue")));
#elif (CRYPTOPP_GCC_VERSION)
} __attribute__((deprecated));
#else
};
#endif

NAMESPACE_END

#ifdef CRYPTOPP_MANUALLY_INSTANTIATE_TEMPLATES
#include "eccrypto.cpp"
#endif

NAMESPACE_BEGIN(CryptoPP)

CRYPTOPP_DLL_TEMPLATE_CLASS DL_GroupParameters_EC<ECP>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_GroupParameters_EC<EC2N>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PublicKeyImpl<DL_GroupParameters_EC<ECP> >;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PublicKeyImpl<DL_GroupParameters_EC<EC2N> >;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PublicKey_EC<ECP>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PublicKey_EC<EC2N>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PrivateKeyImpl<DL_GroupParameters_EC<ECP> >;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PrivateKeyImpl<DL_GroupParameters_EC<EC2N> >;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PrivateKey_EC<ECP>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PrivateKey_EC<EC2N>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_Algorithm_GDSA<ECP::Point>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_Algorithm_GDSA<EC2N::Point>;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PrivateKey_WithSignaturePairwiseConsistencyTest<DL_PrivateKey_EC<ECP>, ECDSA<ECP, SHA256> >;
CRYPTOPP_DLL_TEMPLATE_CLASS DL_PrivateKey_WithSignaturePairwiseConsistencyTest<DL_PrivateKey_EC<EC2N>, ECDSA<EC2N, SHA256> >;

NAMESPACE_END

#endif
