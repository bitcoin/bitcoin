// hkdf.h - written and placed in public domain by Jeffrey Walton. Copyright assigned to Crypto++ project.

//! \file hkdf.h
//! \brief Classes for HKDF from RFC 5869
//! \since Crypto++ 5.6.3

#ifndef CRYPTOPP_HASH_KEY_DERIVATION_FUNCTION_H
#define CRYPTOPP_HASH_KEY_DERIVATION_FUNCTION_H

#include "cryptlib.h"
#include "hrtimer.h"
#include "secblock.h"
#include "hmac.h"

NAMESPACE_BEGIN(CryptoPP)

//! abstract base class for key derivation function
class KeyDerivationFunction
{
public:
	//! maximum number of bytes which can be produced under a secuirty context
	virtual size_t MaxDerivedKeyLength() const =0;
	virtual bool Usesinfo() const =0;
	//! derive a key from secret
	virtual unsigned int DeriveKey(byte *derived, size_t derivedLen, const byte *secret, size_t secretLen, const byte *salt, size_t saltLen, const byte* info=NULL, size_t infoLen=0) const =0;

	virtual ~KeyDerivationFunction() {}
};

//! \brief Extract-and-Expand Key Derivation Function (HKDF)
//! \tparam T HashTransformation class
//! \sa <A HREF="http://eprint.iacr.org/2010/264">Cryptographic Extraction and Key Derivation: The HKDF Scheme</A>
//!   and <A HREF="http://tools.ietf.org/html/rfc5869">HMAC-based Extract-and-Expand Key Derivation Function (HKDF)</A>
//! \since Crypto++ 5.6.3
template <class T>
class HKDF : public KeyDerivationFunction
{
public:
	CRYPTOPP_CONSTANT(DIGESTSIZE = T::DIGESTSIZE)
	CRYPTOPP_CONSTANT(SALTSIZE = T::DIGESTSIZE)
	static const char* StaticAlgorithmName () {
		static const std::string name(std::string("HKDF(") + std::string(T::StaticAlgorithmName()) + std::string(")"));
		return name.c_str();
	}
	size_t MaxDerivedKeyLength() const {return static_cast<size_t>(T::DIGESTSIZE) * 255;}
	bool Usesinfo() const {return true;}
	unsigned int DeriveKey(byte *derived, size_t derivedLen, const byte *secret, size_t secretLen, const byte *salt, size_t saltLen, const byte* info, size_t infoLen) const;

protected:
	// If salt is missing (NULL), then use the NULL vector. Missing is different than EMPTY (0 length). The length
	// of s_NullVector used depends on the Hash function. SHA-256 will use 32 bytes of s_NullVector.
	typedef byte NullVectorType[SALTSIZE];
	static const NullVectorType& GetNullVector() {
		static const NullVectorType s_NullVector = {0};
		return s_NullVector;
	}
};

template <class T>
unsigned int HKDF<T>::DeriveKey(byte *derived, size_t derivedLen, const byte *secret, size_t secretLen, const byte *salt, size_t saltLen, const byte* info, size_t infoLen) const
{
	static const size_t DIGEST_SIZE = static_cast<size_t>(T::DIGESTSIZE);
	const unsigned int req = static_cast<unsigned int>(derivedLen);

	CRYPTOPP_ASSERT(secret && secretLen);
	CRYPTOPP_ASSERT(derived && derivedLen);
	CRYPTOPP_ASSERT(derivedLen <= MaxDerivedKeyLength());

	if (derivedLen > MaxDerivedKeyLength())
		throw InvalidArgument("HKDF: derivedLen must be less than or equal to MaxDerivedKeyLength");

	HMAC<T> hmac;
	FixedSizeSecBlock<byte, DIGEST_SIZE> prk, buffer;

	// Extract
	const byte* key = (salt ? salt : GetNullVector());
	const size_t klen = (salt ? saltLen : DIGEST_SIZE);

	hmac.SetKey(key, klen);
	hmac.CalculateDigest(prk, secret, secretLen);

	// Expand
	hmac.SetKey(prk.data(), prk.size());
	byte block = 0;

	while (derivedLen > 0)
	{
		if (block++) {hmac.Update(buffer, buffer.size());}
		if (info && infoLen) {hmac.Update(info, infoLen);}
		hmac.CalculateDigest(buffer, &block, 1);

#if CRYPTOPP_MSC_VERSION
		const size_t segmentLen = STDMIN(derivedLen, DIGEST_SIZE);
		memcpy_s(derived, segmentLen, buffer, segmentLen);
#else
		const size_t segmentLen = STDMIN(derivedLen, DIGEST_SIZE);
		std::memcpy(derived, buffer, segmentLen);
#endif

		derived += segmentLen;
		derivedLen -= segmentLen;
	}

	return req;
}

NAMESPACE_END

#endif // CRYPTOPP_HASH_KEY_DERIVATION_FUNCTION_H
