// rdrand.h - written and placed in public domain by Jeffrey Walton and Uri Blumenthal.
//            Copyright assigned to Crypto++ project.

//! \file
//! \headerfile rdrand.h
//! \brief Classes for RDRAND and RDSEED

#ifndef CRYPTOPP_RDRAND_H
#define CRYPTOPP_RDRAND_H

#include "cryptlib.h"

// This file (and friends) provides both RDRAND and RDSEED, but its somewhat
//   experimental. They were added at Crypto++ 5.6.3. At compile time, it
//   indirectly uses CRYPTOPP_BOOL_{X86|X32|X64} (via CRYPTOPP_CPUID_AVAILABLE)
//   to select an implementation or "throw NotImplemented". At runtime, the
//   class uses the result of CPUID to determine if RDRAND or RDSEED are
//   available. A lazy throw strategy is used in case the CPU does not support
//   the instruction. I.e., the throw is deferred until GenerateBlock() is called.

// Microsoft added RDRAND in August 2012, VS2012. GCC added RDRAND in December 2010, GCC 4.6.
// Clang added RDRAND in July 2012, Clang 3.2. Intel added RDRAND in September 2011, ICC 12.1.

NAMESPACE_BEGIN(CryptoPP)

//! \brief Exception thrown when a RDRAND generator encounters
//!    a generator related error.
class RDRAND_Err : public Exception
{
public:
	RDRAND_Err(const std::string &operation)
		: Exception(OTHER_ERROR, "RDRAND: " + operation + " operation failed") {}
};

//! \brief Hardware generated random numbers using RDRAND instruction
//! \sa MaurerRandomnessTest() for random bit generators
class RDRAND : public RandomNumberGenerator
{
public:
	std::string AlgorithmName() const {return "RDRAND";}
	
	//! \brief Construct a RDRAND generator
	//! \param retries the number of retries for failed calls to the hardware
	//! \details RDRAND() constructs a generator with a maximum number of retires
	//!   for failed generation attempts.
	RDRAND(unsigned int retries = 8) : m_retries(retries) {}
	
	virtual ~RDRAND() {}
	
	//! \brief Retrieve the number of retries used by the generator
	//! \returns the number of times GenerateBlock() will attempt to recover from a failed generation
	unsigned int GetRetries() const
	{
		return m_retries;
	}
	
	//! \brief Set the number of retries used by the generator
	//! \param retries number of times GenerateBlock() will attempt to recover from a failed generation
	void SetRetries(unsigned int retries)
	{
		m_retries = retries;
	}

	//! \brief Generate random array of bytes
	//! \param output the byte buffer
	//! \param size the length of the buffer, in bytes
#if (CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64)
	virtual void GenerateBlock(byte *output, size_t size);
#else
	virtual void GenerateBlock(byte *output, size_t size) {
		CRYPTOPP_UNUSED(output), CRYPTOPP_UNUSED(size);
		throw NotImplemented("RDRAND: rdrand is not available on this platform");
	}
#endif

	//! \brief Generate and discard n bytes
	//! \param n the number of bytes to generate and discard
	//! \details the RDSEED generator discards words, not bytes. If n is
	//!   not a multiple of a machine word, then it is rounded up to
	//!   that size.
#if (CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64)
	virtual void DiscardBytes(size_t n);
#else
	virtual void DiscardBytes(size_t n) {
		CRYPTOPP_UNUSED(n);
		throw NotImplemented("RDRAND: rdrand is not available on this platform");
	}
#endif

	//! Update RNG state with additional unpredictable values
	//! \param input unused
	//! \param length unused
	//! \details The operation is a nop for this generator.
	virtual void IncorporateEntropy(const byte *input, size_t length)
	{
		// Override to avoid the base class' throw.
		CRYPTOPP_UNUSED(input); CRYPTOPP_UNUSED(length);
		assert(0); // warn in debug builds
	}

private:
	unsigned int m_retries;
};

//! \brief Exception thrown when a RDSEED generator encounters
//!    a generator related error.
class RDSEED_Err : public Exception
{
public:
	RDSEED_Err(const std::string &operation)
		: Exception(OTHER_ERROR, "RDSEED: " + operation + " operation failed") {}
};

//! \brief Hardware generated random numbers using RDSEED instruction
//! \sa MaurerRandomnessTest() for random bit generators
class RDSEED : public RandomNumberGenerator
{
public:
	std::string AlgorithmName() const {return "RDSEED";}
	
	//! \brief Construct a RDSEED generator
	//! \param retries the number of retries for failed calls to the hardware
	//! \details RDSEED() constructs a generator with a maximum number of retires
	//!   for failed generation attempts.
	RDSEED(unsigned int retries = 8) : m_retries(retries) {}
	
	virtual ~RDSEED() {}
	
	//! \brief Retrieve the number of retries used by the generator
	//! \returns the number of times GenerateBlock() will attempt to recover from a failed generation
	unsigned int GetRetries() const
	{
		return m_retries;
	}
	
	//! \brief Set the number of retries used by the generator
	//! \param retries number of times GenerateBlock() will attempt to recover from a failed generation
	void SetRetries(unsigned int retries)
	{
		m_retries = retries;
	}

	//! \brief Generate random array of bytes
	//! \param output the byte buffer
	//! \param size the length of the buffer, in bytes
#if (CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64)
	virtual void GenerateBlock(byte *output, size_t size);
#else
	virtual void GenerateBlock(byte *output, size_t size) {
		CRYPTOPP_UNUSED(output), CRYPTOPP_UNUSED(size);
		throw NotImplemented("RDSEED: rdseed is not available on this platform");
	}
#endif

	//! \brief Generate and discard n bytes
	//! \param n the number of bytes to generate and discard
	//! \details the RDSEED generator discards words, not bytes. If n is
	//!   not a multiple of a machine word, then it is rounded up to
	//!   that size.
#if (CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64)
	virtual void DiscardBytes(size_t n);
#else
	virtual void DiscardBytes(size_t n) {
		CRYPTOPP_UNUSED(n);
		throw NotImplemented("RDSEED: rdseed is not available on this platform");
	}
#endif

	//! Update RNG state with additional unpredictable values
	//! \param input unused
	//! \param length unused
	//! \details The operation is a nop for this generator.
	virtual void IncorporateEntropy(const byte *input, size_t length)
	{
		// Override to avoid the base class' throw.
		CRYPTOPP_UNUSED(input); CRYPTOPP_UNUSED(length);
		assert(0); // warn in debug builds
	}

private:
	unsigned int m_retries;
};

NAMESPACE_END

#endif // CRYPTOPP_RDRAND_H
