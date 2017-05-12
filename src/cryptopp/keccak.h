// keccak.h - written and placed in the public domain by Wei Dai

//! \file keccak.h
//! \brief Classes for Keccak message digests
//! \details The Crypto++ Keccak implementation uses F1600 with XOF d=0x01.
//!   FIPS 202 conformance (XOF d=0x06) is available in SHA3 classes.
//! \details Keccak will likely change in the future to accomodate extensibility of the
//!   round function and the XOF functions.
//! \sa <a href="http://en.wikipedia.org/wiki/Keccak">Keccak</a>
//! \since Crypto++ 5.6.4

#ifndef CRYPTOPP_KECCAK_H
#define CRYPTOPP_KECCAK_H

#include "cryptlib.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class Keccak
//! \brief Keccak message digest base class
//! \details The Crypto++ Keccak implementation uses F1600 with XOF d=0x01.
//!   FIPS 202 conformance (XOF d=0x06) is available in SHA3 classes.
//! \details Keccak is the base class for Keccak_224, Keccak_256, Keccak_384 and Keccak_512.
//!   Library users should instantiate a derived class, and only use Keccak
//!   as a base class reference or pointer.
//! \details Keccak will likely change in the future to accomodate extensibility of the
//!   round function and the XOF functions.
//! \details Perform the following to specify a different digest size. The class will use F1600,
//!   XOF d=0x01, and a new vaue for <tt>r()</tt> (which will be <tt>200-2*24 = 152</tt>).
//!   <pre>  Keccack_192 : public Keccack
//!   {
//!     public:
//!       CRYPTOPP_CONSTANT(DIGESTSIZE = 24)
//!       Keccack_192() : Keccack(DIGESTSIZE) {}
//!   };
//!   </pre>
//!
//! \sa SHA3, Keccak_224, Keccak_256, Keccak_384 and Keccak_512.
//! \since Crypto++ 5.6.4
class Keccak : public HashTransformation
{
public:
	//! \brief Construct a Keccak
	//! \param digestSize the digest size, in bytes
	//! \details Keccak is the base class for Keccak_224, Keccak_256, Keccak_384 and Keccak_512.
	//!   Library users should instantiate a derived class, and only use Keccak
	//!   as a base class reference or pointer.
	//! \since Crypto++ 5.6.4
	Keccak(unsigned int digestSize) : m_digestSize(digestSize) {Restart();}
	unsigned int DigestSize() const {return m_digestSize;}
	std::string AlgorithmName() const {return "Keccak-" + IntToString(m_digestSize*8);}
	CRYPTOPP_CONSTEXPR static const char* StaticAlgorithmName() { return "Keccak"; }
	unsigned int OptimalDataAlignment() const {return GetAlignmentOf<word64>();}

	void Update(const byte *input, size_t length);
	void Restart();
	void TruncatedFinal(byte *hash, size_t size);

	//unsigned int BlockSize() const { return r(); } // that's the idea behind it

protected:
	inline unsigned int r() const {return 200 - 2 * m_digestSize;}

	FixedSizeSecBlock<word64, 25> m_state;
	unsigned int m_digestSize, m_counter;
};

//! \class Keccak_224
//! \tparam DigestSize controls the digest size as a template parameter instead of a per-class constant
//! \brief Keccak-X message digest, template for more fine-grained typedefs
//! \since Crypto++ 5.7.0
template<unsigned int T_DigestSize>
class Keccak_Final : public Keccak
{
public:
	CRYPTOPP_CONSTANT(DIGESTSIZE = T_DigestSize)
	CRYPTOPP_CONSTANT(BLOCKSIZE = 200 - 2 * DIGESTSIZE)

		//! \brief Construct a Keccak-X message digest
	Keccak_Final() : Keccak(DIGESTSIZE) {}
	static std::string StaticAlgorithmName() { return "Keccak-" + IntToString(DIGESTSIZE * 8); }
	unsigned int BlockSize() const { return BLOCKSIZE; }
private:
	CRYPTOPP_COMPILE_ASSERT(BLOCKSIZE < 200); // ensure there was no underflow in the math
	CRYPTOPP_COMPILE_ASSERT(BLOCKSIZE > (int)T_DigestSize); // this is a general expectation by HMAC
};

//! \class Keccak_224
//! \brief Keccak-224 message digest
//! \since Crypto++ 5.6.4
typedef Keccak_Final<28> Keccak_224;
//! \class Keccak_256
//! \brief Keccak-256 message digest
//! \since Crypto++ 5.6.4
typedef Keccak_Final<32> Keccak_256;
//! \class Keccak_384
//! \brief Keccak-384 message digest
//! \since Crypto++ 5.6.4
typedef Keccak_Final<48> Keccak_384;
//! \class Keccak_512
//! \brief Keccak-512 message digest
//! \since Crypto++ 5.6.4
typedef Keccak_Final<64> Keccak_512;

NAMESPACE_END

#endif
