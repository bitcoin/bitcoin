// randpool.h - written and placed in the public domain by Wei Dai

//! \file randpool.h
//! \brief Class file for Randomness Pool
//! \details RandomPool can be used to generate cryptographic quality pseudorandom bytes
//!   after seeding the pool with IncorporateEntropy(). Internally, the generator uses
//!   AES-256 to produce the stream. Entropy is stirred in using SHA-256.
//! \details RandomPool used to follow the design of randpool in PGP 2.6.x. At version 5.5
//!   RandomPool was redesigned to reduce the risk of reusing random numbers after state
//!   rollback (which may occur when running in a virtual machine like VMware or a hosted
//!   environment).
//! \details If you need the pre-Crypto++ 5.5 generator then you can find it with:
//! <pre>
//!    $ git clone https://github.com/weidai11/cryptopp cryptopp-ancient
//!    $ cryptopp-ancient
//!
//!    # Checkout the RandomPool change
//!    $ git checkout f41245df6fb9b85574260eca9cd32777e8ab5136
//!
//!    # Go back one more
//!    git checkout HEAD~1
//!
//!    $ grep 'MDC<SHA>' *.h *.cpp
//!    randpool.cpp:typedef MDC<SHA> RandomPoolCipher;
//! </pre>
//! \since Crypto++ 4.0 (PGP 2.6.x style), Crypto++ 5.5 (AES-256 based)

#ifndef CRYPTOPP_RANDPOOL_H
#define CRYPTOPP_RANDPOOL_H

#include "cryptlib.h"
#include "filters.h"
#include "secblock.h"
#include "smartptr.h"
#include "aes.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class RandomPool
//! \brief Randomness Pool based on AES-256
//! \details RandomPool can be used to generate cryptographic quality pseudorandom bytes
//!   after seeding the pool with IncorporateEntropy(). Internally, the generator uses
//!   AES-256 to produce the stream. Entropy is stirred in using SHA-256.
//! \details RandomPool used to follow the design of randpool in PGP 2.6.x. At version 5.5
//!   RandomPool was redesigned to reduce the risk of reusing random numbers after state
//!   rollback (which may occur when running in a virtual machine like VMware or a hosted
//!   environment).
//! \since Crypto++ 4.0 (PGP 2.6.x style), Crypto++ 5.5 (AES-256 based)
class CRYPTOPP_DLL RandomPool : public RandomNumberGenerator, public NotCopyable
{
public:
	//! \brief Construct a RandomPool
	RandomPool();

	bool CanIncorporateEntropy() const {return true;}
	void IncorporateEntropy(const byte *input, size_t length);
	void GenerateIntoBufferedTransformation(BufferedTransformation &target, const std::string &channel, lword size);

	// for backwards compatibility. use RandomNumberSource, RandomNumberStore, and RandomNumberSink for other BufferTransformation functionality
	void Put(const byte *input, size_t length) {IncorporateEntropy(input, length);}

private:
	FixedSizeAlignedSecBlock<byte, 16, true> m_seed;
	FixedSizeAlignedSecBlock<byte, 32> m_key;
	member_ptr<BlockCipher> m_pCipher;
	bool m_keySet;
};

NAMESPACE_END

#endif
