#ifndef CRYPTOPP_ESIGN_H
#define CRYPTOPP_ESIGN_H

/** \file
	This file contains classes that implement the
	ESIGN signature schemes as defined in IEEE P1363a.
*/

#include "cryptlib.h"
#include "pubkey.h"
#include "integer.h"
#include "asn.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

//! _
class ESIGNFunction : public TrapdoorFunction, public ASN1CryptoMaterial<PublicKey>
{
	typedef ESIGNFunction ThisClass;

public:
	void Initialize(const Integer &n, const Integer &e)
		{m_n = n; m_e = e;}

	// PublicKey
	void BERDecode(BufferedTransformation &bt);
	void DEREncode(BufferedTransformation &bt) const;

	// CryptoMaterial
	bool Validate(RandomNumberGenerator &rng, unsigned int level) const;
	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;
	void AssignFrom(const NameValuePairs &source);

	// TrapdoorFunction
	Integer ApplyFunction(const Integer &x) const;
	Integer PreimageBound() const {return m_n;}
	Integer ImageBound() const {return Integer::Power2(GetK());}

	// non-derived
	const Integer & GetModulus() const {return m_n;}
	const Integer & GetPublicExponent() const {return m_e;}

	void SetModulus(const Integer &n) {m_n = n;}
	void SetPublicExponent(const Integer &e) {m_e = e;}

protected:
	// Covertiy finding on overflow. The library allows small values for research purposes.
	unsigned int GetK() const {return SaturatingSubtract(m_n.BitCount()/3, 1U);}

	Integer m_n, m_e;
};

//! _
class InvertibleESIGNFunction : public ESIGNFunction, public RandomizedTrapdoorFunctionInverse, public PrivateKey
{
	typedef InvertibleESIGNFunction ThisClass;

public:
	void Initialize(const Integer &n, const Integer &e, const Integer &p, const Integer &q)
		{m_n = n; m_e = e; m_p = p; m_q = q;}
	// generate a random private key
	void Initialize(RandomNumberGenerator &rng, unsigned int modulusBits)
		{GenerateRandomWithKeySize(rng, modulusBits);}

	void BERDecode(BufferedTransformation &bt);
	void DEREncode(BufferedTransformation &bt) const;

	Integer CalculateRandomizedInverse(RandomNumberGenerator &rng, const Integer &x) const;

	// GeneratibleCryptoMaterial
	bool Validate(RandomNumberGenerator &rng, unsigned int level) const;
	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;
	void AssignFrom(const NameValuePairs &source);
	/*! parameters: (ModulusSize) */
	void GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &alg);

	const Integer& GetPrime1() const {return m_p;}
	const Integer& GetPrime2() const {return m_q;}

	void SetPrime1(const Integer &p) {m_p = p;}
	void SetPrime2(const Integer &q) {m_q = q;}

protected:
	Integer m_p, m_q;
};

//! _
template <class T>
class EMSA5Pad : public PK_DeterministicSignatureMessageEncodingMethod
{
public:
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "EMSA5";}

	void ComputeMessageRepresentative(RandomNumberGenerator &rng,
		const byte *recoverableMessage, size_t recoverableMessageLength,
		HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
		byte *representative, size_t representativeBitLength) const
	{
		CRYPTOPP_UNUSED(rng), CRYPTOPP_UNUSED(recoverableMessage), CRYPTOPP_UNUSED(recoverableMessageLength);
		CRYPTOPP_UNUSED(messageEmpty), CRYPTOPP_UNUSED(hashIdentifier);
		SecByteBlock digest(hash.DigestSize());
		hash.Final(digest);
		size_t representativeByteLength = BitsToBytes(representativeBitLength);
		T mgf;
		mgf.GenerateAndMask(hash, representative, representativeByteLength, digest, digest.size(), false);
		if (representativeBitLength % 8 != 0)
			representative[0] = (byte)Crop(representative[0], representativeBitLength % 8);
	}
};

//! EMSA5, for use with ESIGN
struct P1363_EMSA5 : public SignatureStandard
{
	typedef EMSA5Pad<P1363_MGF1> SignatureMessageEncodingMethod;
};

struct ESIGN_Keys
{
	static std::string StaticAlgorithmName() {return "ESIGN";}
	typedef ESIGNFunction PublicKey;
	typedef InvertibleESIGNFunction PrivateKey;
};

//! ESIGN, as defined in IEEE P1363a
template <class H, class STANDARD = P1363_EMSA5>
struct ESIGN : public TF_SS<STANDARD, H, ESIGN_Keys>
{
};

NAMESPACE_END

#endif
