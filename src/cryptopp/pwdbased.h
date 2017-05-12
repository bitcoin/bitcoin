// pwdbased.h - written and placed in the public domain by Wei Dai

//! \file pwdbased.h
//! \brief Password based key derivation functions

#ifndef CRYPTOPP_PWDBASED_H
#define CRYPTOPP_PWDBASED_H

#include "cryptlib.h"
#include "hrtimer.h"
#include "integer.h"
#include "hmac.h"

NAMESPACE_BEGIN(CryptoPP)

//! \brief Abstract base class for password based key derivation function
class PasswordBasedKeyDerivationFunction
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~PasswordBasedKeyDerivationFunction() {}
#endif

	//! \brief Provides the maximum derived key length
	//! \returns maximum derived key length, in bytes
	virtual size_t MaxDerivedKeyLength() const =0;

	//! \brief Determines if the derivation function uses the purpose byte
	//! \returns true if the derivation function uses the purpose byte, false otherwise
	virtual bool UsesPurposeByte() const =0;

	//! \brief Derive key from the password
	//! \param derived the byte buffer to receive the derived password
	//! \param derivedLen the size of the byte buffer to receive the derived password
	//! \param purpose an octet indicating the purpose of the derivation
	//! \param password the byte buffer with the password
	//! \param passwordLen the size of the password, in bytes
	//! \param salt the byte buffer with the salt
	//! \param saltLen the size of the salt, in bytes
	//! \param iterations the number of iterations to attempt
	//! \param timeInSeconds the length of time the derivation function should execute
	//! \returns iteration count achieved
	//! \details DeriveKey returns the actual iteration count achieved. If <tt>timeInSeconds == 0</tt>, then the complete number
	//!   of iterations will be obtained. If <tt>timeInSeconds != 0</tt>, then DeriveKey will iterate until time elapsed, as
	//!   measured by ThreadUserTimer.
	virtual unsigned int DeriveKey(byte *derived, size_t derivedLen, byte purpose, const byte *password, size_t passwordLen, const byte *salt, size_t saltLen, unsigned int iterations, double timeInSeconds=0) const =0;
};

//! \brief PBKDF1 from PKCS #5
//! \tparam T a HashTransformation class
template <class T>
class PKCS5_PBKDF1 : public PasswordBasedKeyDerivationFunction
{
public:
	size_t MaxDerivedKeyLength() const {return T::DIGESTSIZE;}
	bool UsesPurposeByte() const {return false;}
	// PKCS #5 says PBKDF1 should only take 8-byte salts. This implementation allows salts of any length.
	unsigned int DeriveKey(byte *derived, size_t derivedLen, byte purpose, const byte *password, size_t passwordLen, const byte *salt, size_t saltLen, unsigned int iterations, double timeInSeconds=0) const;
};

//! \brief PBKDF2 from PKCS #5
//! \tparam T a HashTransformation class
template <class T>
class PKCS5_PBKDF2_HMAC : public PasswordBasedKeyDerivationFunction
{
public:
	size_t MaxDerivedKeyLength() const {return 0xffffffffU;}	// should multiply by T::DIGESTSIZE, but gets overflow that way
	bool UsesPurposeByte() const {return false;}
	unsigned int DeriveKey(byte *derived, size_t derivedLen, byte purpose, const byte *password, size_t passwordLen, const byte *salt, size_t saltLen, unsigned int iterations, double timeInSeconds=0) const;
};

/*
class PBKDF2Params
{
public:
	SecByteBlock m_salt;
	unsigned int m_interationCount;
	ASNOptional<ASNUnsignedWrapper<word32> > m_keyLength;
};
*/

template <class T>
unsigned int PKCS5_PBKDF1<T>::DeriveKey(byte *derived, size_t derivedLen, byte purpose, const byte *password, size_t passwordLen, const byte *salt, size_t saltLen, unsigned int iterations, double timeInSeconds) const
{
	CRYPTOPP_UNUSED(purpose);
	CRYPTOPP_ASSERT(derivedLen <= MaxDerivedKeyLength());
	CRYPTOPP_ASSERT(iterations > 0 || timeInSeconds > 0);

	if (!iterations)
		iterations = 1;

	T hash;
	hash.Update(password, passwordLen);
	hash.Update(salt, saltLen);

	SecByteBlock buffer(hash.DigestSize());
	hash.Final(buffer);

	unsigned int i;
	ThreadUserTimer timer;

	if (timeInSeconds)
		timer.StartTimer();

	for (i=1; i<iterations || (timeInSeconds && (i%128!=0 || timer.ElapsedTimeAsDouble() < timeInSeconds)); i++)
		hash.CalculateDigest(buffer, buffer, buffer.size());

	memcpy(derived, buffer, derivedLen);
	return i;
}

template <class T>
unsigned int PKCS5_PBKDF2_HMAC<T>::DeriveKey(byte *derived, size_t derivedLen, byte purpose, const byte *password, size_t passwordLen, const byte *salt, size_t saltLen, unsigned int iterations, double timeInSeconds) const
{
	CRYPTOPP_UNUSED(purpose);
	CRYPTOPP_ASSERT(derivedLen <= MaxDerivedKeyLength());
	CRYPTOPP_ASSERT(iterations > 0 || timeInSeconds > 0);

	if (!iterations)
		iterations = 1;

	HMAC<T> hmac(password, passwordLen);
	SecByteBlock buffer(hmac.DigestSize());
	ThreadUserTimer timer;

	unsigned int i=1;
	while (derivedLen > 0)
	{
		hmac.Update(salt, saltLen);
		unsigned int j;
		for (j=0; j<4; j++)
		{
			byte b = byte(i >> ((3-j)*8));
			hmac.Update(&b, 1);
		}
		hmac.Final(buffer);

#if CRYPTOPP_MSC_VERSION
		const size_t segmentLen = STDMIN(derivedLen, buffer.size());
		memcpy_s(derived, segmentLen, buffer, segmentLen);
#else
		const size_t segmentLen = STDMIN(derivedLen, buffer.size());
		memcpy(derived, buffer, segmentLen);
#endif

		if (timeInSeconds)
		{
			timeInSeconds = timeInSeconds / ((derivedLen + buffer.size() - 1) / buffer.size());
			timer.StartTimer();
		}

		for (j=1; j<iterations || (timeInSeconds && (j%128!=0 || timer.ElapsedTimeAsDouble() < timeInSeconds)); j++)
		{
			hmac.CalculateDigest(buffer, buffer, buffer.size());
			xorbuf(derived, buffer, segmentLen);
		}

		if (timeInSeconds)
		{
			iterations = j;
			timeInSeconds = 0;
		}

		derived += segmentLen;
		derivedLen -= segmentLen;
		i++;
	}

	return iterations;
}

//! \brief PBKDF from PKCS #12, appendix B
//! \tparam T a HashTransformation class
template <class T>
class PKCS12_PBKDF : public PasswordBasedKeyDerivationFunction
{
public:
	size_t MaxDerivedKeyLength() const {return size_t(0)-1;}
	bool UsesPurposeByte() const {return true;}
	unsigned int DeriveKey(byte *derived, size_t derivedLen, byte purpose, const byte *password, size_t passwordLen, const byte *salt, size_t saltLen, unsigned int iterations, double timeInSeconds) const;
};

template <class T>
unsigned int PKCS12_PBKDF<T>::DeriveKey(byte *derived, size_t derivedLen, byte purpose, const byte *password, size_t passwordLen, const byte *salt, size_t saltLen, unsigned int iterations, double timeInSeconds) const
{
	CRYPTOPP_ASSERT(derivedLen <= MaxDerivedKeyLength());
	CRYPTOPP_ASSERT(iterations > 0 || timeInSeconds > 0);

	if (!iterations)
		iterations = 1;

	const size_t v = T::BLOCKSIZE;	// v is in bytes rather than bits as in PKCS #12
	const size_t DLen = v, SLen = RoundUpToMultipleOf(saltLen, v);
	const size_t PLen = RoundUpToMultipleOf(passwordLen, v), ILen = SLen + PLen;
	SecByteBlock buffer(DLen + SLen + PLen);
	byte *D = buffer, *S = buffer+DLen, *P = buffer+DLen+SLen, *I = S;

	memset(D, purpose, DLen);
	size_t i;
	for (i=0; i<SLen; i++)
		S[i] = salt[i % saltLen];
	for (i=0; i<PLen; i++)
		P[i] = password[i % passwordLen];


	T hash;
	SecByteBlock Ai(T::DIGESTSIZE), B(v);
	ThreadUserTimer timer;

	while (derivedLen > 0)
	{
		hash.CalculateDigest(Ai, buffer, buffer.size());

		if (timeInSeconds)
		{
			timeInSeconds = timeInSeconds / ((derivedLen + Ai.size() - 1) / Ai.size());
			timer.StartTimer();
		}

		for (i=1; i<iterations || (timeInSeconds && (i%128!=0 || timer.ElapsedTimeAsDouble() < timeInSeconds)); i++)
			hash.CalculateDigest(Ai, Ai, Ai.size());

		if (timeInSeconds)
		{
			iterations = (unsigned int)i;
			timeInSeconds = 0;
		}

		for (i=0; i<B.size(); i++)
			B[i] = Ai[i % Ai.size()];

		Integer B1(B, B.size());
		++B1;
		for (i=0; i<ILen; i+=v)
			(Integer(I+i, v) + B1).Encode(I+i, v);

#if CRYPTOPP_MSC_VERSION
		const size_t segmentLen = STDMIN(derivedLen, Ai.size());
		memcpy_s(derived, segmentLen, Ai, segmentLen);
#else
		const size_t segmentLen = STDMIN(derivedLen, Ai.size());
		std::memcpy(derived, Ai, segmentLen);
#endif

		derived += segmentLen;
		derivedLen -= segmentLen;
	}

	return iterations;
}

NAMESPACE_END

#endif
