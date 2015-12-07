// rng.h - written and placed in the public domain by Wei Dai

//! \file rng.h
//! \brief Miscellaneous classes for RNGs
//! \details This file contains miscellaneous classes for RNGs, including LC_RNG(),
//!   X917RNG() and MaurerRandomnessTest()
//! \sa osrng.h, randpool.h

#ifndef CRYPTOPP_RNG_H
#define CRYPTOPP_RNG_H

#include "cryptlib.h"
#include "filters.h"
#include "smartptr.h"

NAMESPACE_BEGIN(CryptoPP)

//! \brief Linear Congruential Generator (LCG)
//! \details Originally propsed by William S. England.
//! \warning LC_RNG is suitable for simulations, where uniformaly distrubuted numbers are
//!   required quickly. It should not be used for cryptographic purposes.
class LC_RNG : public RandomNumberGenerator
{
public:
	//! \brief Construct a Linear Congruential Generator (LCG)
	//! \param init_seed the initial value for the generator
	LC_RNG(word32 init_seed)
		: seed(init_seed) {}

	void GenerateBlock(byte *output, size_t size);

	word32 GetSeed() {return seed;}

private:
	word32 seed;

	static const word32 m;
	static const word32 q;
	static const word16 a;
	static const word16 r;
};

//! \class X917RNG
//! \brief ANSI X9.17 RNG
//! \details X917RNG is from ANSI X9.17 Appendix C.
//! \sa AutoSeededX917RNG, DefaultAutoSeededRNG
class CRYPTOPP_DLL X917RNG : public RandomNumberGenerator, public NotCopyable
{
public:
	//! \brief Construct a X917RNG
	//! \param cipher the block cipher to use for the generator
	//! \param seed a byte buffer to use as a seed
	//! \param deterministicTimeVector additional entropy
	//! \details <tt>cipher</tt> will be deleted by the destructor. <tt>seed</tt> must be at least
	//!   BlockSize() in length. <tt>deterministicTimeVector = 0</tt> means obtain time vector
	//!   from the system.
	//! \details When constructing an AutoSeededX917RNG, the generator must be keyed or an
	//!   access violation will occur because the time vector is encrypted using the block cipher.
	//!   To key the generator during constructions, perform the following:
	//! <pre>
	//!   SecByteBlock key(AES::DEFAULT_KEYLENGTH), seed(AES::BLOCKSIZE);
	//!   OS_GenerateRandomBlock(false, key, key.size());
	//!   OS_GenerateRandomBlock(false, seed, seed.size());
	//!   X917RNG prng(new AES::Encryption(key, AES::DEFAULT_KEYLENGTH), seed, NULL);
	//! </pre>
	//! \sa AutoSeededX917RNG
	X917RNG(BlockTransformation *cipher, const byte *seed, const byte *deterministicTimeVector = 0);

	void GenerateIntoBufferedTransformation(BufferedTransformation &target, const std::string &channel, lword size);

private:
	member_ptr<BlockTransformation> cipher;
	const unsigned int S;	// blocksize of cipher
	SecByteBlock dtbuf; 	// buffer for enciphered timestamp
	SecByteBlock randseed, m_lastBlock, m_deterministicTimeVector;
};

//! \class MaurerRandomnessTest
//! \brief  Maurer's Universal Statistical Test for Random Bit Generators
//! \details This class implements Maurer's Universal Statistical Test for
//!   Random Bit Generators. It is intended for measuring the randomness of
//!   *PHYSICAL* RNGs.
//! \details For more details see Maurer's paper in Journal of Cryptology, 1992.
class MaurerRandomnessTest : public Bufferless<Sink>
{
public:
	MaurerRandomnessTest();

	size_t Put2(const byte *inString, size_t length, int messageEnd, bool blocking);

	//! \brief Provides the number of bytes of input is needed by the test
	//! \returns how many more bytes of input is needed by the test
	// BytesNeeded() returns how many more bytes of input is needed by the test
	// GetTestValue() should not be called before BytesNeeded()==0
	unsigned int BytesNeeded() const {return n >= (Q+K) ? 0 : Q+K-n;}

	// returns a number between 0.0 and 1.0, describing the quality of the
	// random numbers entered
	double GetTestValue() const;

private:
	enum {L=8, V=256, Q=2000, K=2000};
	double sum;
	unsigned int n;
	unsigned int tab[V];
};

NAMESPACE_END

#endif
