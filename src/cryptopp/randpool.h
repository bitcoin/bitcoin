#ifndef CRYPTOPP_RANDPOOL_H
#define CRYPTOPP_RANDPOOL_H

#include "cryptlib.h"
#include "filters.h"
#include "secblock.h"
#include "smartptr.h"
#include "aes.h"

NAMESPACE_BEGIN(CryptoPP)

//! Randomness Pool
/*! This class can be used to generate cryptographic quality
	pseudorandom bytes after seeding the pool with IncorporateEntropy() */
class CRYPTOPP_DLL RandomPool : public RandomNumberGenerator, public NotCopyable
{
public:
	RandomPool();

	bool CanIncorporateEntropy() const {return true;}
	void IncorporateEntropy(const byte *input, size_t length);
	void GenerateIntoBufferedTransformation(BufferedTransformation &target, const std::string &channel, lword size);

	// for backwards compatibility. use RandomNumberSource, RandomNumberStore, and RandomNumberSink for other BufferTransformation functionality
	void Put(const byte *input, size_t length) {IncorporateEntropy(input, length);}

private:
	FixedSizeSecBlock<byte, 32> m_key;
	FixedSizeSecBlock<byte, 16> m_seed;
	member_ptr<BlockCipher> m_pCipher;
	bool m_keySet;
};

NAMESPACE_END

#endif
