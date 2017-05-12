// seckey.h - written and placed in the public domain by Wei Dai

//! \file
//! \brief Classes and functions for implementing secret key algorithms.

#ifndef CRYPTOPP_SECKEY_H
#define CRYPTOPP_SECKEY_H

#include "config.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(push)
# pragma warning(disable: 4189)
#endif

#include "cryptlib.h"
#include "misc.h"
#include "simple.h"

NAMESPACE_BEGIN(CryptoPP)

//! \brief Inverts the cipher's direction
//! \param dir the cipher's direction
//! \returns DECRYPTION if \ref CipherDir "dir" is ENCRYPTION, DECRYPTION otherwise
inline CipherDir ReverseCipherDir(CipherDir dir)
{
	return (dir == ENCRYPTION) ? DECRYPTION : ENCRYPTION;
}

//! \class FixedBlockSize
//! \brief Inherited by algorithms with fixed block size
//! \tparam N the blocksize of the algorithm
template <unsigned int N>
class FixedBlockSize
{
public:
	//! \brief The block size of the algorithm provided as a constant.
	CRYPTOPP_CONSTANT(BLOCKSIZE = N)
};

// ************** rounds ***************

//! \class FixedRounds
//! \brief Inherited by algorithms with fixed number of rounds
//! \tparam R the number of rounds used by the algorithm
template <unsigned int R>
class FixedRounds
{
public:
	//! \brief The number of rounds for the algorithm provided as a constant.
	CRYPTOPP_CONSTANT(ROUNDS = R)
};

//! \class VariableRounds
//! \brief Inherited by algorithms with variable number of rounds
//! \tparam D Default number of rounds
//! \tparam N Minimum number of rounds
//! \tparam D Maximum number of rounds
template <unsigned int D, unsigned int N=1, unsigned int M=INT_MAX>		// use INT_MAX here because enums are treated as signed ints
class VariableRounds
{
public:
	//! \brief The default number of rounds for the algorithm provided as a constant.
	CRYPTOPP_CONSTANT(DEFAULT_ROUNDS = D)
	//! \brief The minimum number of rounds for the algorithm provided as a constant.
	CRYPTOPP_CONSTANT(MIN_ROUNDS = N)
	//! \brief The maximum number of rounds for the algorithm provided as a constant.
	CRYPTOPP_CONSTANT(MAX_ROUNDS = M)
	//! \brief The default number of rounds for the algorithm based on key length
	//!   provided by a static function.
	//! \param keylength the size of the key, in bytes
	//! \details keylength is unused in the default implementation.
	CRYPTOPP_CONSTEXPR static unsigned int StaticGetDefaultRounds(size_t keylength)
	{
		// Comma operator breaks Debug builds with GCC 4.0 - 4.6.
		// Also see http://github.com/weidai11/cryptopp/issues/255
#if defined(CRYPTOPP_CXX11_CONSTEXPR)
		return CRYPTOPP_UNUSED(keylength), static_cast<unsigned int>(DEFAULT_ROUNDS);
#else
		CRYPTOPP_UNUSED(keylength);
		return static_cast<unsigned int>(DEFAULT_ROUNDS);
#endif
	}

protected:
	//! \brief Validates the number of rounds for an algorithm.
	//! \param rounds the candidate number of rounds
	//! \param alg an Algorithm object used if the number of rounds are invalid
	//! \throws InvalidRounds if the number of rounds are invalid
	//! \details ThrowIfInvalidRounds() validates the number of rounds and throws if invalid.
	inline void ThrowIfInvalidRounds(int rounds, const Algorithm *alg)
	{
		if (M == INT_MAX) // Coverity and result_independent_of_operands
		{
			if (rounds < MIN_ROUNDS)
				throw InvalidRounds(alg ? alg->AlgorithmName() : std::string("VariableRounds"), rounds);
		}
		else
		{
			if (rounds < MIN_ROUNDS || rounds > MAX_ROUNDS)
				throw InvalidRounds(alg ? alg->AlgorithmName() : std::string("VariableRounds"), rounds);
		}
	}

	//! \brief Validates the number of rounds for an algorithm
	//! \param param the candidate number of rounds
	//! \param alg an Algorithm object used if the number of rounds are invalid
	//! \returns the number of rounds for the algorithm
	//! \throws InvalidRounds if the number of rounds are invalid
	//! \details GetRoundsAndThrowIfInvalid() validates the number of rounds and throws if invalid.
	inline unsigned int GetRoundsAndThrowIfInvalid(const NameValuePairs &param, const Algorithm *alg)
	{
		int rounds = param.GetIntValueWithDefault("Rounds", DEFAULT_ROUNDS);
		ThrowIfInvalidRounds(rounds, alg);
		return (unsigned int)rounds;
	}
};

// ************** key length ***************

//! \class FixedKeyLength
//! \brief Inherited by keyed algorithms with fixed key length
//! \tparam N Default key length, in bytes
//! \tparam IV_REQ the \ref SimpleKeyingInterface::IV_Requirement "IV requirements"
//! \tparam IV_L default IV length, in bytes
//! \sa SimpleKeyingInterface
template <unsigned int N, unsigned int IV_REQ = SimpleKeyingInterface::NOT_RESYNCHRONIZABLE, unsigned int IV_L = 0>
class FixedKeyLength
{
public:
	//! \brief The default key length used by the algorithm provided as a constant
	//! \details KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(KEYLENGTH=N)
	//! \brief The minimum key length used by the algorithm provided as a constant
	//! \details MIN_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(MIN_KEYLENGTH=N)
	//! \brief The maximum key length used by the algorithm provided as a constant
	//! \details MAX_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(MAX_KEYLENGTH=N)
	//! \brief The default key length used by the algorithm provided as a constant
	//! \details DEFAULT_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(DEFAULT_KEYLENGTH=N)
	//! \brief The default IV requirements for the algorithm provided as a constant
	//! \details The default value is NOT_RESYNCHRONIZABLE. See IV_Requirement
	//!  in cryptlib.h for allowed values.
	CRYPTOPP_CONSTANT(IV_REQUIREMENT = IV_REQ)
	//! \brief The default IV length used by the algorithm provided as a constant
	//! \details IV_LENGTH is provided in bytes, not bits. The default implementation uses 0.
	CRYPTOPP_CONSTANT(IV_LENGTH = IV_L)
	//! \brief The default key length for the algorithm provided by a static function.
	//! \param keylength the size of the key, in bytes
	//! \details The default implementation returns KEYLENGTH. keylength is unused
	//!   in the default implementation.
	CRYPTOPP_CONSTEXPR static size_t CRYPTOPP_API StaticGetValidKeyLength(size_t keylength)
	{
		// Comma operator breaks Debug builds with GCC 4.0 - 4.6.
		// Also see http://github.com/weidai11/cryptopp/issues/255
#if defined(CRYPTOPP_CXX11_CONSTEXPR)
		return CRYPTOPP_UNUSED(keylength), static_cast<size_t>(KEYLENGTH);
#else
		CRYPTOPP_UNUSED(keylength);
		return static_cast<size_t>(KEYLENGTH);
#endif
	}
};

//! \class VariableKeyLength
//! \brief Inherited by keyed algorithms with variable key length
//! \tparam D Default key length, in bytes
//! \tparam N Minimum key length, in bytes
//! \tparam M Maximum key length, in bytes
//! \tparam M Default key length multiple, in bytes. The default multiple is 1.
//! \tparam IV_REQ the \ref SimpleKeyingInterface::IV_Requirement "IV requirements"
//! \tparam IV_L default IV length, in bytes. The default length is 0.
//! \sa SimpleKeyingInterface
template <unsigned int D, unsigned int N, unsigned int M, unsigned int Q = 1, unsigned int IV_REQ = SimpleKeyingInterface::NOT_RESYNCHRONIZABLE, unsigned int IV_L = 0>
class VariableKeyLength
{
	// Make these private to avoid Doxygen documenting them in all derived classes
	CRYPTOPP_COMPILE_ASSERT(Q > 0);
	CRYPTOPP_COMPILE_ASSERT(N % Q == 0);
	CRYPTOPP_COMPILE_ASSERT(M % Q == 0);
	CRYPTOPP_COMPILE_ASSERT(N < M);
	CRYPTOPP_COMPILE_ASSERT(D >= N);
	CRYPTOPP_COMPILE_ASSERT(M >= D);

public:
	//! \brief The minimum key length used by the algorithm provided as a constant
	//! \details MIN_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(MIN_KEYLENGTH=N)
	//! \brief The maximum key length used by the algorithm provided as a constant
	//! \details MAX_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(MAX_KEYLENGTH=M)
	//! \brief The default key length used by the algorithm provided as a constant
	//! \details DEFAULT_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(DEFAULT_KEYLENGTH=D)
	//! \brief The key length multiple used by the algorithm provided as a constant
	//! \details MAX_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(KEYLENGTH_MULTIPLE=Q)
	//! \brief The default IV requirements for the algorithm provided as a constant
	//! \details The default value is NOT_RESYNCHRONIZABLE. See IV_Requirement
	//!  in cryptlib.h for allowed values.
	CRYPTOPP_CONSTANT(IV_REQUIREMENT=IV_REQ)
	//! \brief The default initialization vector length for the algorithm provided as a constant
	//! \details IV_LENGTH is provided in bytes, not bits. The default implementation uses 0.
	CRYPTOPP_CONSTANT(IV_LENGTH=IV_L)
	//! \brief Provides a valid key length for the algorithm provided by a static function.
	//! \param keylength the size of the key, in bytes
	//! \details If keylength is less than MIN_KEYLENGTH, then the function returns
	//!   MIN_KEYLENGTH. If keylength is greater than MAX_KEYLENGTH, then the function
	//!   returns MAX_KEYLENGTH. If keylength is a multiple of KEYLENGTH_MULTIPLE,
	//!   then keylength is returned. Otherwise, the function returns keylength rounded
	//!   \a down to the next smaller multiple of KEYLENGTH_MULTIPLE.
	//! \details keylength is provided in bytes, not bits.
	// TODO: Figure out how to make this CRYPTOPP_CONSTEXPR
	static size_t CRYPTOPP_API StaticGetValidKeyLength(size_t keylength)
	{
		if (keylength < (size_t)MIN_KEYLENGTH)
			return MIN_KEYLENGTH;
		else if (keylength > (size_t)MAX_KEYLENGTH)
			return (size_t)MAX_KEYLENGTH;
		else
		{
			keylength += KEYLENGTH_MULTIPLE-1;
			return keylength - keylength%KEYLENGTH_MULTIPLE;
		}
	}
};

//! \class SameKeyLengthAs
//! \brief Provides key lengths based on another class's key length
//! \tparam T another FixedKeyLength or VariableKeyLength class
//! \tparam IV_REQ the \ref SimpleKeyingInterface::IV_Requirement "IV requirements"
//! \tparam IV_L default IV length, in bytes
//! \sa SimpleKeyingInterface
template <class T, unsigned int IV_REQ = SimpleKeyingInterface::NOT_RESYNCHRONIZABLE, unsigned int IV_L = 0>
class SameKeyLengthAs
{
public:
	//! \brief The minimum key length used by the algorithm provided as a constant
	//! \details MIN_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(MIN_KEYLENGTH=T::MIN_KEYLENGTH)
	//! \brief The maximum key length used by the algorithm provided as a constant
	//! \details MIN_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(MAX_KEYLENGTH=T::MAX_KEYLENGTH)
	//! \brief The default key length used by the algorithm provided as a constant
	//! \details MIN_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(DEFAULT_KEYLENGTH=T::DEFAULT_KEYLENGTH)
	//! \brief The default IV requirements for the algorithm provided as a constant
	//! \details The default value is NOT_RESYNCHRONIZABLE. See IV_Requirement
	//!  in cryptlib.h for allowed values.
	CRYPTOPP_CONSTANT(IV_REQUIREMENT=IV_REQ)
	//! \brief The default initialization vector length for the algorithm provided as a constant
	//! \details IV_LENGTH is provided in bytes, not bits. The default implementation uses 0.
	CRYPTOPP_CONSTANT(IV_LENGTH=IV_L)
	//! \brief Provides a valid key length for the algorithm provided by a static function.
	//! \param keylength the size of the key, in bytes
	//! \details If keylength is less than MIN_KEYLENGTH, then the function returns
	//!   MIN_KEYLENGTH. If keylength is greater than MAX_KEYLENGTH, then the function
	//!   returns MAX_KEYLENGTH. If keylength is a multiple of KEYLENGTH_MULTIPLE,
	//!   then keylength is returned. Otherwise, the function returns keylength rounded
	//!   \a down to the next smaller multiple of KEYLENGTH_MULTIPLE.
	//! \details keylength is provided in bytes, not bits.
	CRYPTOPP_CONSTEXPR static size_t CRYPTOPP_API StaticGetValidKeyLength(size_t keylength)
		{return T::StaticGetValidKeyLength(keylength);}
};

// ************** implementation helper for SimpleKeyingInterface ***************

//! \class SimpleKeyingInterfaceImpl
//! \brief Provides a base implementation of SimpleKeyingInterface
//! \tparam BASE a SimpleKeyingInterface derived class
//! \tparam INFO a SimpleKeyingInterface derived class
//! \details SimpleKeyingInterfaceImpl() provides a default implementation for ciphers providing a keying interface.
//!   Functions are virtual and not eligible for C++11 <tt>constexpr</tt>-ness.
//! \sa Algorithm(), SimpleKeyingInterface()
template <class BASE, class INFO = BASE>
class CRYPTOPP_NO_VTABLE SimpleKeyingInterfaceImpl : public BASE
{
public:
	//! \brief The minimum key length used by the algorithm
	//! \returns minimum key length used by the algorithm, in bytes
	size_t MinKeyLength() const
		{return INFO::MIN_KEYLENGTH;}

	//! \brief The maximum key length used by the algorithm
	//! \returns maximum key length used by the algorithm, in bytes
	size_t MaxKeyLength() const
		{return (size_t)INFO::MAX_KEYLENGTH;}

	//! \brief The default key length used by the algorithm
	//! \returns default key length used by the algorithm, in bytes
	size_t DefaultKeyLength() const
		{return INFO::DEFAULT_KEYLENGTH;}

	//! \brief Provides a valid key length for the algorithm
	//! \param keylength the size of the key, in bytes
	//! \returns the valid key lenght, in bytes
	//! \details keylength is provided in bytes, not bits. If keylength is less than MIN_KEYLENGTH,
	//!   then the function returns MIN_KEYLENGTH. If keylength is greater than MAX_KEYLENGTH,
	//!   then the function returns MAX_KEYLENGTH. if If keylength is a multiple of KEYLENGTH_MULTIPLE,
	//!   then keylength is returned. Otherwise, the function returns a \a lower multiple of
	//!   KEYLENGTH_MULTIPLE.
	size_t GetValidKeyLength(size_t keylength) const {return INFO::StaticGetValidKeyLength(keylength);}

	//! \brief The default IV requirements for the algorithm
	//! \details The default value is NOT_RESYNCHRONIZABLE. See IV_Requirement
	//!  in cryptlib.h for allowed values.
	SimpleKeyingInterface::IV_Requirement IVRequirement() const
		{return (SimpleKeyingInterface::IV_Requirement)INFO::IV_REQUIREMENT;}

	//! \brief The default initialization vector length for the algorithm
	//! \details IVSize is provided in bytes, not bits. The default implementation uses IV_LENGTH, which is 0.
	unsigned int IVSize() const
		{return INFO::IV_LENGTH;}
};

//! \class BlockCipherImpl
//! \brief Provides a base implementation of Algorithm and SimpleKeyingInterface for block ciphers
//! \tparam INFO a SimpleKeyingInterface derived class
//! \tparam BASE a SimpleKeyingInterface derived class
//! \details BlockCipherImpl() provides a default implementation for block ciphers using AlgorithmImpl()
//!   and SimpleKeyingInterfaceImpl(). Functions are virtual and not eligible for C++11 <tt>constexpr</tt>-ness.
//! \sa Algorithm(), SimpleKeyingInterface(), AlgorithmImpl(), SimpleKeyingInterfaceImpl()
template <class INFO, class BASE = BlockCipher>
class CRYPTOPP_NO_VTABLE BlockCipherImpl : public AlgorithmImpl<SimpleKeyingInterfaceImpl<TwoBases<BASE, INFO> > >
{
public:
	//! Provides the block size of the algorithm
	//! \returns the block size of the algorithm, in bytes
	unsigned int BlockSize() const {return this->BLOCKSIZE;}
};

//! \class BlockCipherFinal
//! \brief Provides class member functions to key a block cipher
//! \tparam DIR a CipherDir
//! \tparam BASE a BlockCipherImpl derived class
template <CipherDir DIR, class BASE>
class BlockCipherFinal : public ClonableImpl<BlockCipherFinal<DIR, BASE>, BASE>
{
public:
	//! \brief Construct a default BlockCipherFinal
	//! \details The cipher is not keyed.
 	BlockCipherFinal() {}

	//! \brief Construct a BlockCipherFinal
	//! \param key a byte array used to key the cipher
	//! \details key must be at least DEFAULT_KEYLENGTH in length. Internally, the function calls
	//!    SimpleKeyingInterface::SetKey.
	BlockCipherFinal(const byte *key)
		{this->SetKey(key, this->DEFAULT_KEYLENGTH);}

	//! \brief Construct a BlockCipherFinal
	//! \param key a byte array used to key the cipher
	//! \param length the length of the byte array
	//! \details key must be at least DEFAULT_KEYLENGTH in length. Internally, the function calls
	//!    SimpleKeyingInterface::SetKey.
	BlockCipherFinal(const byte *key, size_t length)
		{this->SetKey(key, length);}

	//! \brief Construct a BlockCipherFinal
	//! \param key a byte array used to key the cipher
	//! \param length the length of the byte array
	//! \param rounds the number of rounds
	//! \details key must be at least DEFAULT_KEYLENGTH in length. Internally, the function calls
	//!    SimpleKeyingInterface::SetKeyWithRounds.
	BlockCipherFinal(const byte *key, size_t length, unsigned int rounds)
		{this->SetKeyWithRounds(key, length, rounds);}

	//! \brief Provides the direction of the cipher
	//! \returns true if DIR is ENCRYPTION, false otherwise
	//! \sa GetCipherDirection(), IsPermutation()
	bool IsForwardTransformation() const {return DIR == ENCRYPTION;}
};

//! \class MessageAuthenticationCodeImpl
//! \brief Provides a base implementation of Algorithm and SimpleKeyingInterface for message authentication codes
//! \tparam INFO a SimpleKeyingInterface derived class
//! \tparam BASE a SimpleKeyingInterface derived class
//! \details MessageAuthenticationCodeImpl() provides a default implementation for message authentication codes
//!   using AlgorithmImpl() and SimpleKeyingInterfaceImpl(). Functions are virtual and not subject to C++11
//!   <tt>constexpr</tt>.
//! \sa Algorithm(), SimpleKeyingInterface(), AlgorithmImpl(), SimpleKeyingInterfaceImpl()
template <class BASE, class INFO = BASE>
class MessageAuthenticationCodeImpl : public AlgorithmImpl<SimpleKeyingInterfaceImpl<BASE, INFO>, INFO>
{
};

//! \class MessageAuthenticationCodeFinal
//! \brief Provides class member functions to key a message authentication code
//! \tparam DIR a CipherDir
//! \tparam BASE a BlockCipherImpl derived class
template <class BASE>
class MessageAuthenticationCodeFinal : public ClonableImpl<MessageAuthenticationCodeFinal<BASE>, MessageAuthenticationCodeImpl<BASE> >
{
public:
	//! \brief Construct a default MessageAuthenticationCodeFinal
	//! \details The message authentication code is not keyed.
 	MessageAuthenticationCodeFinal() {}
	//! \brief Construct a BlockCipherFinal
	//! \param key a byte array used to key the algorithm
	//! \details key must be at least DEFAULT_KEYLENGTH in length. Internally, the function calls
	//!    SimpleKeyingInterface::SetKey.
	MessageAuthenticationCodeFinal(const byte *key)
		{this->SetKey(key, this->DEFAULT_KEYLENGTH);}
	//! \brief Construct a BlockCipherFinal
	//! \param key a byte array used to key the algorithm
	//! \param length the length of the byte array
	//! \details key must be at least DEFAULT_KEYLENGTH in length. Internally, the function calls
	//!    SimpleKeyingInterface::SetKey.
	MessageAuthenticationCodeFinal(const byte *key, size_t length)
		{this->SetKey(key, length);}
};

// ************** documentation ***************

//! \class BlockCipherDocumentation
//! \brief Provides Encryption and Decryption typedefs used by derived classes to
//!    implement a block cipher
//! \details These objects usually should not be used directly. See CipherModeDocumentation
//!    instead. Each class derived from this one defines two types, Encryption and Decryption,
//!    both of which implement the BlockCipher interface.
struct BlockCipherDocumentation
{
	//! implements the BlockCipher interface
	typedef BlockCipher Encryption;
	//! implements the BlockCipher interface
	typedef BlockCipher Decryption;
};

//! \class SymmetricCipherDocumentation
//! \brief Provides Encryption and Decryption typedefs used by derived classes to
//!    implement a symmetric cipher
//! \details Each class derived from this one defines two types, Encryption and Decryption,
//!    both of which implement the SymmetricCipher interface. Two types of classes derive
//!    from this class: stream ciphers and block cipher modes. Stream ciphers can be used
//!    alone, cipher mode classes need to be used with a block cipher. See CipherModeDocumentation
//!    for more for information about using cipher modes and block ciphers.
struct SymmetricCipherDocumentation
{
	//! implements the SymmetricCipher interface
	typedef SymmetricCipher Encryption;
	//! implements the SymmetricCipher interface
	typedef SymmetricCipher Decryption;
};

//! \class AuthenticatedSymmetricCipherDocumentation
//! \brief Provides Encryption and Decryption typedefs used by derived classes to
//!    implement an authenticated encryption cipher
//! \details Each class derived from this one defines two types, Encryption and Decryption,
//!    both of which implement the AuthenticatedSymmetricCipher interface.
struct AuthenticatedSymmetricCipherDocumentation
{
	//! implements the AuthenticatedSymmetricCipher interface
	typedef AuthenticatedSymmetricCipher Encryption;
	//! implements the AuthenticatedSymmetricCipher interface
	typedef AuthenticatedSymmetricCipher Decryption;
};

NAMESPACE_END

#if CRYPTOPP_MSC_VERSION
# pragma warning(pop)
#endif

#endif
