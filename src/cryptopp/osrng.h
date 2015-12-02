// osrng.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile osrng.h
//! \brief Classes for access to the operating system's random number generators

#ifndef CRYPTOPP_OSRNG_H
#define CRYPTOPP_OSRNG_H

#include "config.h"

#ifdef OS_RNG_AVAILABLE

#include "cryptlib.h"
#include "randpool.h"
#include "smartptr.h"
#include "fips140.h"
#include "rng.h"
#include "aes.h"
#include "sha.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class OS_RNG_Err
//! \brief Exception thrown when an operating system error is encountered
class CRYPTOPP_DLL OS_RNG_Err : public Exception
{
public:
	//! \brief Constructs an OS_RNG_Err
	//! \param operation the operation or API call when the error occurs
	OS_RNG_Err(const std::string &operation);
};

#ifdef NONBLOCKING_RNG_AVAILABLE

#ifdef CRYPTOPP_WIN32_AVAILABLE
//! \class MicrosoftCryptoProvider
//! \brief Wrapper for Microsoft crypto service provider
//! \sa \def USE_MS_CRYPTOAPI, \def WORKAROUND_MS_BUG_Q258000
class CRYPTOPP_DLL MicrosoftCryptoProvider
{
public:
	//! \brief Construct a MicrosoftCryptoProvider
	MicrosoftCryptoProvider();
	~MicrosoftCryptoProvider();

// type HCRYPTPROV, avoid #include <windows.h>
#if defined(__CYGWIN__) && defined(__x86_64__)
	typedef unsigned long long ProviderHandle;
#elif defined(WIN64) || defined(_WIN64)
	typedef unsigned __int64 ProviderHandle;
#else
	typedef unsigned long ProviderHandle;
#endif

	//! \brief Retrieves the CryptoAPI provider handle
	//! \returns CryptoAPI provider handle
	//! \details The handle is acquired by a call to CryptAcquireContext().
	//!   CryptReleaseContext() is called upon destruction.
	ProviderHandle GetProviderHandle() const {return m_hProvider;}

private:
	ProviderHandle m_hProvider;
};

#if defined(_MSC_VER)
# pragma comment(lib, "advapi32.lib")
#endif

#endif //CRYPTOPP_WIN32_AVAILABLE

//! \class NonblockingRng
//! \brief Wrapper class for /dev/random and /dev/srandom
//! \details Encapsulates CryptoAPI's CryptGenRandom() on Windows, or /dev/urandom on Unix and compatibles.
class CRYPTOPP_DLL NonblockingRng : public RandomNumberGenerator
{
public:
	//! \brief Construct a NonblockingRng
	NonblockingRng();
	~NonblockingRng();
	
	//! \brief Generate random array of bytes
	//! \param output the byte buffer
	//! \param size the length of the buffer, in bytes
	//! \details GenerateIntoBufferedTransformation() calls are routed to GenerateBlock().
	void GenerateBlock(byte *output, size_t size);

protected:
#ifdef CRYPTOPP_WIN32_AVAILABLE
#	ifndef WORKAROUND_MS_BUG_Q258000
		MicrosoftCryptoProvider m_Provider;
#	endif
#else
	int m_fd;
#endif
};

#endif

#if defined(BLOCKING_RNG_AVAILABLE) || defined(CRYPTOPP_DOXYGEN_PROCESSING)

//! \class BlockingRng
//! \brief Wrapper class for /dev/random and /dev/srandom
//! \details Encapsulates /dev/random on Linux, OS X and Unix; and /dev/srandom on the BSDs.
class CRYPTOPP_DLL BlockingRng : public RandomNumberGenerator
{
public:
	//! \brief Construct a BlockingRng
	BlockingRng();
	~BlockingRng();
	
	//! \brief Generate random array of bytes
	//! \param output the byte buffer
	//! \param size the length of the buffer, in bytes
	//! \details GenerateIntoBufferedTransformation() calls are routed to GenerateBlock().
	void GenerateBlock(byte *output, size_t size);

protected:
	int m_fd;
};

#endif

//! OS_GenerateRandomBlock
//! \brief Generate random array of bytes
//! \param blocking specifies whther a bobcking or non-blocking generator should be used
//! \param output the byte buffer
//! \param size the length of the buffer, in bytes
//! \details OS_GenerateRandomBlock() uses the underlying operating system's
//!   random number generator. On Windows, CryptGenRandom() is called using NonblockingRng.
//! \details On Unix and compatibles, /dev/urandom is called if blocking is false using
//!   NonblockingRng. If blocking is true, then either /dev/randomd or /dev/srandom is used
//!  by way of BlockingRng, if available.
CRYPTOPP_DLL void CRYPTOPP_API OS_GenerateRandomBlock(bool blocking, byte *output, size_t size);


//! \class AutoSeededRandomPool
//! \brief Automatically Seeded Randomness Pool
//! \details This class seeds itself using an operating system provided RNG.
class CRYPTOPP_DLL AutoSeededRandomPool : public RandomPool
{
public:
	//! \brief Construct an AutoSeededRandomPool
	//! \param blocking controls seeding with BlockingRng or NonblockingRng
	//! \param seedSize the size of the seed, in bytes
	//! \details Use blocking to choose seeding with BlockingRng or NonblockingRng.
	//!   The parameter is ignored if only one of these is available.
	explicit AutoSeededRandomPool(bool blocking = false, unsigned int seedSize = 32)
		{Reseed(blocking, seedSize);}
	
	//! \brief Reseed an AutoSeededRandomPool
	//! \param blocking controls seeding with BlockingRng or NonblockingRng
	//! \param seedSize the size of the seed, in bytes
	void Reseed(bool blocking = false, unsigned int seedSize = 32);
};

//! \class AutoSeededX917RNG
//! \tparam BLOCK_CIPHER a block cipher
//! \brief Automatically Seeded X9.17 RNG
//! \details AutoSeededX917RNG is from ANSI X9.17 Appendix C, seeded using an OS provided RNG.
//!   If 3-key TripleDES (DES_EDE3) is used, then its a X9.17 conforming generator. If AES is
//!   used, then its a X9.31 conforming generator.
//! \details Though ANSI X9 prescribes 3-key TripleDES, the template parameter BLOCK_CIPHER can be any
//!   BlockTransformation derived class.
//! \sa X917RNG, DefaultAutoSeededRNG
template <class BLOCK_CIPHER>
class AutoSeededX917RNG : public RandomNumberGenerator, public NotCopyable
{
public:
	//! \brief Construct an AutoSeededX917RNG
	//! \param blocking controls seeding with BlockingRng or NonblockingRng
	//! \param autoSeed controls auto seeding of the generator
	//! \details Use blocking to choose seeding with BlockingRng or NonblockingRng.
	//!   The parameter is ignored if only one of these is available.
	//! \sa X917RNG
	explicit AutoSeededX917RNG(bool blocking = false, bool autoSeed = true)
		{if (autoSeed) Reseed(blocking);}

	//! \brief Reseed an AutoSeededX917RNG
	//! \param blocking controls seeding with BlockingRng or NonblockingRng
	//! \param additionalEntropy additional entropy to add to the generator
	//! \param length the size of the additional entropy, in bytes
	//! \details Internally, the generator uses SHA256 to extract the entropy from
	//!   from the seed and then stretch the material for the block cipher's key
	//!   and initialization vector.
	void Reseed(bool blocking = false, const byte *additionalEntropy = NULL, size_t length = 0);

	//! \brief Deterministically reseed an AutoSeededX917RNG for testing
	//! \param key the key to use for the deterministic reseeding
	//! \param keylength the size of the key, in bytes
	//! \param seed the seed to use for the deterministic reseeding
	//! \param timeVector a time vector to use for deterministic reseeding
	//! \details This is a testing interface for testing purposes, and should \a NOT
	//!   be used in production.
	void Reseed(const byte *key, size_t keylength, const byte *seed, const byte *timeVector);

	bool CanIncorporateEntropy() const {return true;}
	void IncorporateEntropy(const byte *input, size_t length) {Reseed(false, input, length);}
	void GenerateIntoBufferedTransformation(BufferedTransformation &target, const std::string &channel, lword length)
		{m_rng->GenerateIntoBufferedTransformation(target, channel, length);}

private:
	member_ptr<RandomNumberGenerator> m_rng;
};

template <class BLOCK_CIPHER>
void AutoSeededX917RNG<BLOCK_CIPHER>::Reseed(const byte *key, size_t keylength, const byte *seed, const byte *timeVector)
{
	m_rng.reset(new X917RNG(new typename BLOCK_CIPHER::Encryption(key, keylength), seed, timeVector));
}

template <class BLOCK_CIPHER>
void AutoSeededX917RNG<BLOCK_CIPHER>::Reseed(bool blocking, const byte *input, size_t length)
{
	SecByteBlock seed(BLOCK_CIPHER::BLOCKSIZE + BLOCK_CIPHER::DEFAULT_KEYLENGTH);
	const byte *key;
	do
	{
		OS_GenerateRandomBlock(blocking, seed, seed.size());
		if (length > 0)
		{
			SHA256 hash;
			hash.Update(seed, seed.size());
			hash.Update(input, length);
			hash.TruncatedFinal(seed, UnsignedMin(hash.DigestSize(), seed.size()));
		}
		key = seed + BLOCK_CIPHER::BLOCKSIZE;
	}	// check that seed and key don't have same value
	while (memcmp(key, seed, STDMIN((unsigned int)BLOCK_CIPHER::BLOCKSIZE, (unsigned int)BLOCK_CIPHER::DEFAULT_KEYLENGTH)) == 0);

	Reseed(key, BLOCK_CIPHER::DEFAULT_KEYLENGTH, seed, NULL);
}

CRYPTOPP_DLL_TEMPLATE_CLASS AutoSeededX917RNG<AES>;

#if defined(CRYPTOPP_DOXYGEN_PROCESSING)
//! \class DefaultAutoSeededRNG
//! \brief A typedef providing a default generator
//! \details DefaultAutoSeededRNG is a typedef of either AutoSeededX917RNG<AES> or AutoSeededRandomPool.
//!   If CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2 is defined, then DefaultAutoSeededRNG is
//!   AutoSeededX917RNG<AES>. Otherwise, DefaultAutoSeededRNG is AutoSeededRandomPool.
class DefaultAutoSeededRNG {}
#else
// AutoSeededX917RNG<AES> in FIPS mode, otherwise it's AutoSeededRandomPool
#if CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2
typedef AutoSeededX917RNG<AES> DefaultAutoSeededRNG;
#else
typedef AutoSeededRandomPool DefaultAutoSeededRNG;
#endif
#endif // CRYPTOPP_DOXYGEN_PROCESSING

NAMESPACE_END

#endif

#endif
