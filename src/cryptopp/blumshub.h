// blumshub.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile blumshub.h
//! \brief Classes for Blum Blum Shub generator

#ifndef CRYPTOPP_BLUMSHUB_H
#define CRYPTOPP_BLUMSHUB_H

#include "cryptlib.h"
#include "modarith.h"
#include "integer.h"

NAMESPACE_BEGIN(CryptoPP)

//! BlumBlumShub without factorization of the modulus
class PublicBlumBlumShub : public RandomNumberGenerator,
						   public StreamTransformation
{
public:
	PublicBlumBlumShub(const Integer &n, const Integer &seed);

	unsigned int GenerateBit();
	byte GenerateByte();
	void GenerateBlock(byte *output, size_t size);
	void ProcessData(byte *outString, const byte *inString, size_t length);

	bool IsSelfInverting() const {return true;}
	bool IsForwardTransformation() const {return true;}

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~PublicBlumBlumShub() {}
#endif

protected:
	ModularArithmetic modn;
	Integer current;
	word maxBits, bitsLeft;
};

//! BlumBlumShub with factorization of the modulus
class BlumBlumShub : public PublicBlumBlumShub
{
public:
	// Make sure p and q are both primes congruent to 3 mod 4 and at least 512 bits long,
	// seed is the secret key and should be about as big as p*q
	BlumBlumShub(const Integer &p, const Integer &q, const Integer &seed);

	bool IsRandomAccess() const {return true;}
	void Seek(lword index);

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~BlumBlumShub() {}
#endif

protected:
	const Integer p, q;
	const Integer x0;
};

NAMESPACE_END

#endif
