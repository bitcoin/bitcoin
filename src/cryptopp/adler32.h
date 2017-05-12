// adler32.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile adler32.h
//! \brief Class file for ADLER-32 checksum calculations

#ifndef CRYPTOPP_ADLER32_H
#define CRYPTOPP_ADLER32_H

#include "cryptlib.h"

NAMESPACE_BEGIN(CryptoPP)

//! ADLER-32 checksum calculations
class Adler32 : public HashTransformation
{
public:
	CRYPTOPP_CONSTANT(DIGESTSIZE = 4)
	Adler32() {Reset();}
	void Update(const byte *input, size_t length);
	void TruncatedFinal(byte *hash, size_t size);
	unsigned int DigestSize() const {return DIGESTSIZE;}
    CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "Adler32";}
    std::string AlgorithmName() const {return StaticAlgorithmName();}

private:
	void Reset() {m_s1 = 1; m_s2 = 0;}

	word16 m_s1, m_s2;
};

NAMESPACE_END

#endif
