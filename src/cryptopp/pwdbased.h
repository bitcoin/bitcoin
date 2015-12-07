// pwdbased.h - written and placed in the public domain by Wei Dai

#ifndef CRYPTOPP_PWDBASED_H
#define CRYPTOPP_PWDBASED_H

#include "cryptlib.h"
#include "hrtimer.h"
#include "integer.h"
#include "hmac.h"

NAMESPACE_BEGIN(CryptoPP)

//! abstract base class for password based key derivation function
class PasswordBasedKeyDerivationFunction
{
public:
	virtual size_t MaxDerivedKeyLength() const =0;
	virtual bool UsesPurposeByte() const =0;
	//! derive key from password
	/*! If timeInSeconds != 0, will iterate until time elapsed, as measured by ThreadUserTimer
		Returns actual iteration count, which is equal to iterations if timeInSeconds == 0, and not less than iterations otherwise. */
	virtual unsigned int DeriveKey(byte *derived, size_t derivedLen, byte purpose, const byte *password, size_t passwordLen, const byte *salt, size_t saltLen, unsigned int iterations, double timeInSeconds=0) const =0;
};

//! PBKDF1 from PKCS #5, T should be a HashTransformation class
template <class T>
class PKCS5_PBKDF1 : public PasswordBasedKeyDerivationFunction
{
public:
	size_t MaxDerivedKeyLength() const {return T::DIGESTSIZE;}
	bool UsesPurposeByte() const {return false;}
	// PKCS #5 says PBKDF1 should only take 8-byte salts. This implementation allows salts of any length.
	unsigned int DeriveKey(byte *derived, size_t derivedLen, byte purpose, const byte *password, size_t passwordLen, const byte *salt, size_t saltLen, unsigned int iterations, double timeInSeconds=0) const;
};

//! PBKDF2 from PKCS #5, T should be a HashTransformation class
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
	assert(derivedLen <= MaxDerivedKeyLength());
	assert(iterations > 0 || timeInSeconds > 0);

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
	assert(derivedLen <= MaxDerivedKeyLength());
	assert(iterations > 0 || timeInSeconds > 0);

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

//! PBKDF from PKCS #12, appendix B, T should be a HashTransformation class
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
	assert(derivedLen <= MaxDerivedKeyLength());
	assert(iterations > 0 || timeInSeconds > 0);

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
