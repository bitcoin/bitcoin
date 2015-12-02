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
//! \returns DECRYPTION if dir is ENCRYPTION, DECRYPTION otherwise
inline CipherDir ReverseCipherDir(CipherDir dir)
{
	return (dir == ENCRYPTION) ? DECRYPTION : ENCRYPTION;
}

//! \class FixedBlockSize
//! \brief Inherited by block ciphers with fixed block size
//! \tparam N the blocksize of the cipher
template <unsigned int N>
class FixedBlockSize
{
public:
	//! \brief The block size of the cipher provided as a constant.
	CRYPTOPP_CONSTANT(BLOCKSIZE = N)
};

// ************** rounds ***************

//! \class FixedRounds
//! \brief Inherited by ciphers with fixed number of rounds
//! \tparam R the number of rounds used by the cipher
template <unsigned int R>
class FixedRounds
{
public:
	//! \brief The number of rounds for the cipher provided as a constant.
	CRYPTOPP_CONSTANT(ROUNDS = R)
};

//! \class VariableRounds
//! \brief Inherited by ciphers with variable number of rounds
//! \tparam D Default number of rounds
//! \tparam N Minimum number of rounds
//! \tparam D Maximum number of rounds
template <unsigned int D, unsigned int N=1, unsigned int M=INT_MAX>		// use INT_MAX here because enums are treated as signed ints
class VariableRounds
{
public:
	//! \brief The default number of rounds for the cipher provided as a constant.
	CRYPTOPP_CONSTANT(DEFAULT_ROUNDS = D)
	//! \brief The minimum number of rounds for the cipher provided as a constant.
	CRYPTOPP_CONSTANT(MIN_ROUNDS = N)
	//! \brief The maximum number of rounds for the cipher provided as a constant.
	CRYPTOPP_CONSTANT(MAX_ROUNDS = M)
	//! \brief The default number of rounds for the cipher based on key length 
	//!   provided by a static function.
	//! \param keylength the size of the key, in bytes
	//! \details keylength is unused in the default implementation.
	static unsigned int StaticGetDefaultRounds(size_t keylength)
		{CRYPTOPP_UNUSED(keylength); return DEFAULT_ROUNDS;}

protected:
	//! \brief Validates the number of rounds for a cipher.
	//! \param rounds the canddiate number of rounds
	//! \param alg an Algorithm object used if the number of rounds are invalid
	//! \throws InvalidRounds if the number of rounds are invalid
	inline void ThrowIfInvalidRounds(int rounds, const Algorithm *alg)
	{
#if (M==INT_MAX)	// Coverity and result_independent_of_operands
		if (rounds < MIN_ROUNDS)
			throw InvalidRounds(alg ? alg->AlgorithmName() : "VariableRounds", rounds);	
#else
		if (rounds < MIN_ROUNDS || rounds > MAX_ROUNDS)
			throw InvalidRounds(alg ? alg->AlgorithmName() : "VariableRounds", rounds);
#endif
	}

	//! \brief Validates the number of rounds for a cipher
	//! \param param the canddiate number of rounds
	//! \param alg an Algorithm object used if the number of rounds are invalid
	//! \returns the number of rounds for the cipher
	//! \throws InvalidRounds if the number of rounds are invalid
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
//! \tparam IV_REQ The IV requirements. See IV_Requirement in cryptlib.h for allowed values
//! \tparam IV_L Default IV length, in bytes
template <unsigned int N, unsigned int IV_REQ = SimpleKeyingInterface::NOT_RESYNCHRONIZABLE, unsigned int IV_L = 0>
class FixedKeyLength
{
public:
	//! \brief The default key length used by the cipher provided as a constant
	//! \details KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(KEYLENGTH=N)
	//! \brief The minimum key length used by the cipher provided as a constant
	//! \details MIN_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(MIN_KEYLENGTH=N)
	//! \brief The maximum key length used by the cipher provided as a constant
	//! \details MAX_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(MAX_KEYLENGTH=N)
	//! \brief The default key length used by the cipher provided as a constant
	//! \details DEFAULT_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(DEFAULT_KEYLENGTH=N)
	//! \brief The default IV requirements for the cipher provided as a constant
	//! \details The default value is NOT_RESYNCHRONIZABLE. See IV_Requirement
	//!  in cryptlib.h for allowed values.
	CRYPTOPP_CONSTANT(IV_REQUIREMENT = IV_REQ)
	//! \brief The default IV length used by the cipher provided as a constant
	//! \details IV_LENGTH is provided in bytes, not bits. The default implementation uses 0.
	CRYPTOPP_CONSTANT(IV_LENGTH = IV_L)
	//! \brief The default key length for the cipher provided by a static function.
	//! \param keylength the size of the key, in bytes
	//! \details The default implementation returns KEYLENGTH. keylength is unused
	//!   in the default implementation.
	static size_t CRYPTOPP_API StaticGetValidKeyLength(size_t keylength)
		{CRYPTOPP_UNUSED(keylength); return KEYLENGTH;}
};

//! \class VariableKeyLength
//! \brief Inherited by keyed algorithms with variable key length
//! \tparam D Default key length, in bytes
//! \tparam N Minimum key length, in bytes
//! \tparam M Maximum key length, in bytes
//! \tparam M Default key length multiple, in bytes. The default multiple is 1.
//! \tparam IV_REQ The IV requirements. See IV_Requirement in cryptlib.h for allowed values
//! \tparam IV_L Default IV length, in bytes. The default length is 0.
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
	//! \brief The minimum key length used by the cipher provided as a constant
	//! \details MIN_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(MIN_KEYLENGTH=N)
	//! \brief The maximum key length used by the cipher provided as a constant
	//! \details MAX_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(MAX_KEYLENGTH=M)
	//! \brief The default key length used by the cipher provided as a constant
	//! \details DEFAULT_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(DEFAULT_KEYLENGTH=D)
	//! \brief The key length multiple used by the cipher provided as a constant
	//! \details MAX_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(KEYLENGTH_MULTIPLE=Q)
	//! \brief The default IV requirements for the cipher provided as a constant
	//! \details The default value is NOT_RESYNCHRONIZABLE. See IV_Requirement
	//!  in cryptlib.h for allowed values.
	CRYPTOPP_CONSTANT(IV_REQUIREMENT=IV_REQ)
	//! \brief The default initialization vector length for the cipher provided as a constant
	//! \details IV_LENGTH is provided in bytes, not bits. The default implementation uses 0.
	CRYPTOPP_CONSTANT(IV_LENGTH=IV_L)
	//! \brief Provides a valid key length for the cipher provided by a static function.
	//! \param keylength the size of the key, in bytes
	//! \details If keylength is less than MIN_KEYLENGTH, then the function returns
	//!   MIN_KEYLENGTH. If keylength is greater than MAX_KEYLENGTH, then the function
	//!   returns MAX_KEYLENGTH. If keylength is a multiple of KEYLENGTH_MULTIPLE,
	//!   then keylength is returned. Otherwise, the function returns keylength rounded
	//!   \a down to the next smaller multiple of KEYLENGTH_MULTIPLE.
	//! \details keylength is provided in bytes, not bits.
	static size_t CRYPTOPP_API StaticGetValidKeyLength(size_t keylength)
	{
#if MIN_KEYLENGTH > 0
		if (keylength < (size_t)MIN_KEYLENGTH)
			return MIN_KEYLENGTH;
		else
#endif
		if (keylength > (size_t)MAX_KEYLENGTH)
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
//! \tparam IV_REQ The IV requirements. See IV_Requirement in cryptlib.h for allowed values
//! \tparam IV_L Default IV length, in bytes
template <class T, unsigned int IV_REQ = SimpleKeyingInterface::NOT_RESYNCHRONIZABLE, unsigned int IV_L = 0>
class SameKeyLengthAs
{
public:
	//! \brief The minimum key length used by the cipher provided as a constant
	//! \details MIN_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(MIN_KEYLENGTH=T::MIN_KEYLENGTH)
	//! \brief The maximum key length used by the cipher provided as a constant
	//! \details MIN_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(MAX_KEYLENGTH=T::MAX_KEYLENGTH)
	//! \brief The default key length used by the cipher provided as a constant
	//! \details MIN_KEYLENGTH is provided in bytes, not bits
	CRYPTOPP_CONSTANT(DEFAULT_KEYLENGTH=T::DEFAULT_KEYLENGTH)
	//! \brief The default IV requirements for the cipher provided as a constant
	//! \details The default value is NOT_RESYNCHRONIZABLE. See IV_Requirement
	//!  in cryptlib.h for allowed values.
	CRYPTOPP_CONSTANT(IV_REQUIREMENT=IV_REQ)
	//! \brief The default initialization vector length for the cipher provided as a constant
	//! \details IV_LENGTH is provided in bytes, not bits. The default implementation uses 0.
	CRYPTOPP_CONSTANT(IV_LENGTH=IV_L)
	//! \brief Provides a valid key length for the cipher provided by a static function.
	//! \param keylength the size of the key, in bytes
	//! \details If keylength is less than MIN_KEYLENGTH, then the function returns
	//!   MIN_KEYLENGTH. If keylength is greater than MAX_KEYLENGTH, then the function
	//!   returns MAX_KEYLENGTH. If keylength is a multiple of KEYLENGTH_MULTIPLE,
	//!   then keylength is returned. Otherwise, the function returns keylength rounded
	//!   \a down to the next smaller multiple of KEYLENGTH_MULTIPLE.
	//! \details keylength is provided in bytes, not bits.
	static size_t CRYPTOPP_API StaticGetValidKeyLength(size_t keylength)
		{return T::StaticGetValidKeyLength(keylength);}
};

// ************** implementation helper for SimpleKeyed ***************

//! \class SimpleKeyingInterfaceImpl
//! \brief Provides class member functions to access SimpleKeyingInterface constants
//! \tparam BASE a SimpleKeyingInterface derived class
//! \tparam INFO a SimpleKeyingInterface derived class
template <class BASE, class INFO = BASE>
class CRYPTOPP_NO_VTABLE SimpleKeyingInterfaceImpl : public BASE
{
public:
	//! \brief The minimum key length used by the cipher
	size_t MinKeyLength() const
		{return INFO::MIN_KEYLENGTH;}

	//! \brief The maximum key length used by the cipher
	size_t MaxKeyLength() const
		{return (size_t)INFO::MAX_KEYLENGTH;}
	
	//! \brief The default key length used by the cipher
	size_t DefaultKeyLength() const
		{return INFO::DEFAULT_KEYLENGTH;}
	
	//! \brief Provides a valid key length for the cipher
	//! \param keylength the size of the key, in bytes
	//! \details keylength is provided in bytes, not bits. If keylength is less than MIN_KEYLENGTH,
	//!   then the function returns MIN_KEYLENGTH. If keylength is greater than MAX_KEYLENGTH,
	//!   then the function returns MAX_KEYLENGTH. if If keylength is a multiple of KEYLENGTH_MULTIPLE,
	//!   then keylength is returned. Otherwise, the function returns a \a lower multiple of
	//!   KEYLENGTH_MULTIPLE.
	size_t GetValidKeyLength(size_t keylength) const {return INFO::StaticGetValidKeyLength(keylength);}

	//! \brief The default IV requirements for the cipher
	//! \details The default value is NOT_RESYNCHRONIZABLE. See IV_Requirement
	//!  in cryptlib.h for allowed values.
	SimpleKeyingInterface::IV_Requirement IVRequirement() const
		{return (SimpleKeyingInterface::IV_Requirement)INFO::IV_REQUIREMENT;}
	
	//! \brief The default initialization vector length for the cipher
	//! \details IVSize is provided in bytes, not bits. The default implementation uses IV_LENGTH, which is 0.
	unsigned int IVSize() const
		{return INFO::IV_LENGTH;}
};

//! \class BlockCipherImpl
//! \brief Provides class member functions to access BlockCipher constants
//! \tparam INFO a SimpleKeyingInterface derived class
//! \tparam BASE a SimpleKeyingInterface derived class
template <class INFO, class BASE = BlockCipher>
class CRYPTOPP_NO_VTABLE BlockCipherImpl : public AlgorithmImpl<SimpleKeyingInterfaceImpl<TwoBases<BASE, INFO> > >
{
public:
	//! Provides the block size of the cipher
	//! \returns the block size of the cipher, in bytes
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
	//! \sa IsForwardTransformation(), IsPermutation(), GetCipherDirection()
	bool IsForwardTransformation() const {return DIR == ENCRYPTION;}
};

//! \class MessageAuthenticationCodeImpl
//! \brief Provides class member functions to access MessageAuthenticationCode constants
//! \tparam INFO a SimpleKeyingInterface derived class
//! \tparam BASE a SimpleKeyingInterface derived class
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
	//! \param key a byte array used to key the cipher
	//! \details key must be at least DEFAULT_KEYLENGTH in length. Internally, the function calls
	//!    SimpleKeyingInterface::SetKey.
	MessageAuthenticationCodeFinal(const byte *key)
		{this->SetKey(key, this->DEFAULT_KEYLENGTH);}
	//! \brief Construct a BlockCipherFinal
	//! \param key a byte array used to key the cipher
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
