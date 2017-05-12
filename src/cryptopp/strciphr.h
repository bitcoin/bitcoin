// strciphr.h - written and placed in the public domain by Wei Dai

//! \file strciphr.h
//! \brief Classes for implementing stream ciphers
//! \details This file contains helper classes for implementing stream ciphers.
//!   All this infrastructure may look very complex compared to what's in Crypto++ 4.x,
//!   but stream ciphers implementations now support a lot of new functionality,
//!   including better performance (minimizing copying), resetting of keys and IVs, and methods to
//!   query which features are supported by a cipher.
//! \details Here's an explanation of these classes. The word "policy" is used here to mean a class with a
//!   set of methods that must be implemented by individual stream cipher implementations.
//!   This is usually much simpler than the full stream cipher API, which is implemented by
//!   either AdditiveCipherTemplate or CFB_CipherTemplate using the policy. So for example, an
//!   implementation of SEAL only needs to implement the AdditiveCipherAbstractPolicy interface
//!   (since it's an additive cipher, i.e., it xors a keystream into the plaintext).
//!   See this line in seal.h:
//! <pre>
//!     typedef SymmetricCipherFinal\<ConcretePolicyHolder\<SEAL_Policy\<B\>, AdditiveCipherTemplate\<\> \> \> Encryption;
//! </pre>
//! \details AdditiveCipherTemplate and CFB_CipherTemplate are designed so that they don't need
//!   to take a policy class as a template parameter (although this is allowed), so that
//!   their code is not duplicated for each new cipher. Instead they each
//!   get a reference to an abstract policy interface by calling AccessPolicy() on itself, so
//!   AccessPolicy() must be overriden to return the actual policy reference. This is done
//!   by the ConceretePolicyHolder class. Finally, SymmetricCipherFinal implements the constructors and
//!   other functions that must be implemented by the most derived class.

#ifndef CRYPTOPP_STRCIPHR_H
#define CRYPTOPP_STRCIPHR_H

#include "config.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(push)
# pragma warning(disable: 4127 4189)
#endif

#include "cryptlib.h"
#include "seckey.h"
#include "secblock.h"
#include "argnames.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class AbstractPolicyHolder
//! \brief Access a stream cipher policy object
//! \tparam POLICY_INTERFACE class implementing AbstractPolicyHolder
//! \tparam BASE class or type to use as a base class
template <class POLICY_INTERFACE, class BASE = Empty>
class CRYPTOPP_NO_VTABLE AbstractPolicyHolder : public BASE
{
public:
	typedef POLICY_INTERFACE PolicyInterface;
	virtual ~AbstractPolicyHolder() {}

protected:
	virtual const POLICY_INTERFACE & GetPolicy() const =0;
	virtual POLICY_INTERFACE & AccessPolicy() =0;
};

//! \class ConcretePolicyHolder
//! \brief Stream cipher policy object
//! \tparam POLICY class implementing AbstractPolicyHolder
//! \tparam BASE class or type to use as a base class
template <class POLICY, class BASE, class POLICY_INTERFACE = CPP_TYPENAME BASE::PolicyInterface>
class ConcretePolicyHolder : public BASE, protected POLICY
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~ConcretePolicyHolder() {}
#endif
protected:
	const POLICY_INTERFACE & GetPolicy() const {return *this;}
	POLICY_INTERFACE & AccessPolicy() {return *this;}
};

//! \brief Keystream operation flags
//! \sa AdditiveCipherAbstractPolicy::GetBytesPerIteration(), AdditiveCipherAbstractPolicy::GetOptimalBlockSize()
//!   and AdditiveCipherAbstractPolicy::GetAlignment()
enum KeystreamOperationFlags {
	//! \brief Output buffer is aligned
	OUTPUT_ALIGNED=1,
	//! \brief Input buffer is aligned
	INPUT_ALIGNED=2,
	//! \brief Input buffer is NULL
	INPUT_NULL = 4
};

//! \brief Keystream operation flags
//! \sa AdditiveCipherAbstractPolicy::GetBytesPerIteration(), AdditiveCipherAbstractPolicy::GetOptimalBlockSize()
//!   and AdditiveCipherAbstractPolicy::GetAlignment()
enum KeystreamOperation {
	//! \brief Wirte the keystream to the output buffer, input is NULL
	WRITE_KEYSTREAM				= INPUT_NULL,
	//! \brief Wirte the keystream to the aligned output buffer, input is NULL
	WRITE_KEYSTREAM_ALIGNED		= INPUT_NULL | OUTPUT_ALIGNED,
	//! \brief XOR the input buffer and keystream, write to the output buffer
	XOR_KEYSTREAM				= 0,
	//! \brief XOR the aligned input buffer and keystream, write to the output buffer
	XOR_KEYSTREAM_INPUT_ALIGNED = INPUT_ALIGNED,
	//! \brief XOR the input buffer and keystream, write to the aligned output buffer
	XOR_KEYSTREAM_OUTPUT_ALIGNED= OUTPUT_ALIGNED,
	//! \brief XOR the aligned input buffer and keystream, write to the aligned output buffer
	XOR_KEYSTREAM_BOTH_ALIGNED	= OUTPUT_ALIGNED | INPUT_ALIGNED};

//! \class AdditiveCipherAbstractPolicy
//! \brief Policy object for additive stream ciphers
struct CRYPTOPP_DLL CRYPTOPP_NO_VTABLE AdditiveCipherAbstractPolicy
{
	virtual ~AdditiveCipherAbstractPolicy() {}

	//! \brief Provides data alignment requirements
	//! \returns data alignment requirements, in bytes
	//! \details Internally, the default implementation returns 1. If the stream cipher is implemented
	//!   using an SSE2 ASM or intrinsics, then the value returned is usually 16.
	virtual unsigned int GetAlignment() const {return 1;}

	//! \brief Provides number of bytes operated upon during an iteration
	//! \returns bytes operated upon during an iteration, in bytes
	//! \sa GetOptimalBlockSize()
	virtual unsigned int GetBytesPerIteration() const =0;

	//! \brief Provides number of ideal bytes to process
	//! \returns the ideal number of bytes to process
	//! \details Internally, the default implementation returns GetBytesPerIteration()
	//! \sa GetBytesPerIteration()
	virtual unsigned int GetOptimalBlockSize() const {return GetBytesPerIteration();}

	//! \brief Provides buffer size based on iterations
	//! \returns the buffer size based on iterations, in bytes
	virtual unsigned int GetIterationsToBuffer() const =0;

	//! \brief Generate the keystream
	//! \param keystream the key stream
	//! \param iterationCount the number of iterations to generate the key stream
	//! \sa CanOperateKeystream(), OperateKeystream(), WriteKeystream()
	virtual void WriteKeystream(byte *keystream, size_t iterationCount)
		{OperateKeystream(KeystreamOperation(INPUT_NULL | (KeystreamOperationFlags)IsAlignedOn(keystream, GetAlignment())), keystream, NULL, iterationCount);}

	//! \brief Flag indicating
	//! \returns true if the stream can be generated independent of the transformation input, false otherwise
	//! \sa CanOperateKeystream(), OperateKeystream(), WriteKeystream()
	virtual bool CanOperateKeystream() const {return false;}

	//! \brief Operates the keystream
	//! \param operation the operation with additional flags
	//! \param output the output buffer
	//! \param input the input buffer
	//! \param iterationCount the number of iterations to perform on the input
	//! \details OperateKeystream() will attempt to operate upon GetOptimalBlockSize() buffer,
	//!   which will be derived from GetBytesPerIteration().
	//! \sa CanOperateKeystream(), OperateKeystream(), WriteKeystream(), KeystreamOperation()
	virtual void OperateKeystream(KeystreamOperation operation, byte *output, const byte *input, size_t iterationCount)
		{CRYPTOPP_UNUSED(operation); CRYPTOPP_UNUSED(output); CRYPTOPP_UNUSED(input); CRYPTOPP_UNUSED(iterationCount); CRYPTOPP_ASSERT(false);}

	//! \brief Key the cipher
	//! \param params set of NameValuePairs use to initialize this object
	//! \param key a byte array used to key the cipher
	//! \param length the size of the key array
	virtual void CipherSetKey(const NameValuePairs &params, const byte *key, size_t length) =0;

	//! \brief Resynchronize the cipher
	//! \param keystreamBuffer the keystream buffer
	//! \param iv a byte array used to resynchronize the cipher
	//! \param length the size of the IV array
	virtual void CipherResynchronize(byte *keystreamBuffer, const byte *iv, size_t length)
		{CRYPTOPP_UNUSED(keystreamBuffer); CRYPTOPP_UNUSED(iv); CRYPTOPP_UNUSED(length); throw NotImplemented("SimpleKeyingInterface: this object doesn't support resynchronization");}

	//! \brief Flag indicating random access
	//! \returns true if the cipher is seekable, false otherwise
	//! \sa SeekToIteration()
	virtual bool CipherIsRandomAccess() const =0;

	//! \brief Seeks to a random position in the stream
	//! \returns iterationCount
	//! \sa CipherIsRandomAccess()
	virtual void SeekToIteration(lword iterationCount)
		{CRYPTOPP_UNUSED(iterationCount); CRYPTOPP_ASSERT(!CipherIsRandomAccess()); throw NotImplemented("StreamTransformation: this object doesn't support random access");}
};

//! \class AdditiveCipherConcretePolicy
//! \brief Base class for additive stream ciphers
//! \tparam WT word type
//! \tparam W count of words
//! \tparam X bytes per iteration count
//! \tparam BASE AdditiveCipherAbstractPolicy derived base class
template <typename WT, unsigned int W, unsigned int X = 1, class BASE = AdditiveCipherAbstractPolicy>
struct CRYPTOPP_NO_VTABLE AdditiveCipherConcretePolicy : public BASE
{
	typedef WT WordType;
	CRYPTOPP_CONSTANT(BYTES_PER_ITERATION = sizeof(WordType) * W)

#if !(CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X64)
	//! \brief Provides data alignment requirements
	//! \returns data alignment requirements, in bytes
	//! \details Internally, the default implementation returns 1. If the stream cipher is implemented
	//!   using an SSE2 ASM or intrinsics, then the value returned is usually 16.
	unsigned int GetAlignment() const {return GetAlignmentOf<WordType>();}
#endif

	//! \brief Provides number of bytes operated upon during an iteration
	//! \returns bytes operated upon during an iteration, in bytes
	//! \sa GetOptimalBlockSize()
	unsigned int GetBytesPerIteration() const {return BYTES_PER_ITERATION;}

	//! \brief Provides buffer size based on iterations
	//! \returns the buffer size based on iterations, in bytes
	unsigned int GetIterationsToBuffer() const {return X;}

	//! \brief Flag indicating
	//! \returns true if the stream can be generated independent of the transformation input, false otherwise
	//! \sa CanOperateKeystream(), OperateKeystream(), WriteKeystream()
	bool CanOperateKeystream() const {return true;}

	//! \brief Operates the keystream
	//! \param operation the operation with additional flags
	//! \param output the output buffer
	//! \param input the input buffer
	//! \param iterationCount the number of iterations to perform on the input
	//! \details OperateKeystream() will attempt to operate upon GetOptimalBlockSize() buffer,
	//!   which will be derived from GetBytesPerIteration().
	//! \sa CanOperateKeystream(), OperateKeystream(), WriteKeystream(), KeystreamOperation()
	virtual void OperateKeystream(KeystreamOperation operation, byte *output, const byte *input, size_t iterationCount) =0;
};

//! \brief Helper macro to implement OperateKeystream
#define CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, b, i, a)	\
	PutWord(bool(x & OUTPUT_ALIGNED), b, output+i*sizeof(WordType), (x & INPUT_NULL) ? (a) : (a) ^ GetWord<WordType>(bool(x & INPUT_ALIGNED), b, input+i*sizeof(WordType)));

//! \brief Helper macro to implement OperateKeystream
#define CRYPTOPP_KEYSTREAM_OUTPUT_XMM(x, i, a)	{\
	__m128i t = (x & INPUT_NULL) ? a : _mm_xor_si128(a, (x & INPUT_ALIGNED) ? _mm_load_si128((__m128i *)input+i) : _mm_loadu_si128((__m128i *)input+i));\
	if (x & OUTPUT_ALIGNED) _mm_store_si128((__m128i *)output+i, t);\
	else _mm_storeu_si128((__m128i *)output+i, t);}

//! \brief Helper macro to implement OperateKeystream
#define CRYPTOPP_KEYSTREAM_OUTPUT_SWITCH(x, y)	\
	switch (operation)							\
	{											\
		case WRITE_KEYSTREAM:					\
			x(WRITE_KEYSTREAM)					\
			break;								\
		case XOR_KEYSTREAM:						\
			x(XOR_KEYSTREAM)					\
			input += y;							\
			break;								\
		case XOR_KEYSTREAM_INPUT_ALIGNED:		\
			x(XOR_KEYSTREAM_INPUT_ALIGNED)		\
			input += y;							\
			break;								\
		case XOR_KEYSTREAM_OUTPUT_ALIGNED:		\
			x(XOR_KEYSTREAM_OUTPUT_ALIGNED)		\
			input += y;							\
			break;								\
		case WRITE_KEYSTREAM_ALIGNED:			\
			x(WRITE_KEYSTREAM_ALIGNED)			\
			break;								\
		case XOR_KEYSTREAM_BOTH_ALIGNED:		\
			x(XOR_KEYSTREAM_BOTH_ALIGNED)		\
			input += y;							\
			break;								\
	}											\
	output += y;

//! \class AdditiveCipherTemplate
//! \brief Base class for additive stream ciphers with SymmetricCipher interface
//! \tparam BASE AbstractPolicyHolder base class
template <class BASE = AbstractPolicyHolder<AdditiveCipherAbstractPolicy, SymmetricCipher> >
class CRYPTOPP_NO_VTABLE AdditiveCipherTemplate : public BASE, public RandomNumberGenerator
{
public:
	//! \brief Generate random array of bytes
	//! \param output the byte buffer
	//! \param size the length of the buffer, in bytes
	//! \details All generated values are uniformly distributed over the range specified within the
	//!   the contraints of a particular generator.
	void GenerateBlock(byte *output, size_t size);

	//! \brief Apply keystream to data
	//! \param outString a buffer to write the transformed data
	//! \param inString a buffer to read the data
	//! \param length the size fo the buffers, in bytes
	//! \details This is the primary method to operate a stream cipher. For example:
	//! <pre>
	//!     size_t size = 30;
	//!     byte plain[size] = "Do or do not; there is no try";
	//!     byte cipher[size];
	//!     ...
	//!     ChaCha20 chacha(key, keySize);
	//!     chacha.ProcessData(cipher, plain, size);
	//! </pre>
    void ProcessData(byte *outString, const byte *inString, size_t length);

	//! \brief Resynchronize the cipher
	//! \param iv a byte array used to resynchronize the cipher
	//! \param length the size of the IV array
	void Resynchronize(const byte *iv, int length=-1);

	//! \brief Provides number of ideal bytes to process
	//! \returns the ideal number of bytes to process
	//! \details Internally, the default implementation returns GetBytesPerIteration()
	//! \sa GetBytesPerIteration() and GetOptimalNextBlockSize()
	unsigned int OptimalBlockSize() const {return this->GetPolicy().GetOptimalBlockSize();}

	//! \brief Provides number of ideal bytes to process
	//! \returns the ideal number of bytes to process
	//! \details Internally, the default implementation returns remaining unprocessed bytes
	//! \sa GetBytesPerIteration() and OptimalBlockSize()
	unsigned int GetOptimalNextBlockSize() const {return (unsigned int)this->m_leftOver;}

	//! \brief Provides number of ideal data alignment
	//! \returns the ideal data alignment, in bytes
	//! \sa GetAlignment() and OptimalBlockSize()
	unsigned int OptimalDataAlignment() const {return this->GetPolicy().GetAlignment();}

	//! \brief Determines if the cipher is self inverting
	//! \returns true if the stream cipher is self inverting, false otherwise
	bool IsSelfInverting() const {return true;}

	//! \brief Determines if the cipher is a forward transformation
	//! \returns true if the stream cipher is a forward transformation, false otherwise
	bool IsForwardTransformation() const {return true;}

	//! \brief Flag indicating random access
	//! \returns true if the cipher is seekable, false otherwise
	//! \sa Seek()
	bool IsRandomAccess() const {return this->GetPolicy().CipherIsRandomAccess();}

	//! \brief Seeks to a random position in the stream
	//! \param position the absolute position in the stream
	//! \sa IsRandomAccess()
	void Seek(lword position);

	typedef typename BASE::PolicyInterface PolicyInterface;

protected:
	void UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &params);

	unsigned int GetBufferByteSize(const PolicyInterface &policy) const {return policy.GetBytesPerIteration() * policy.GetIterationsToBuffer();}

	inline byte * KeystreamBufferBegin() {return this->m_buffer.data();}
	inline byte * KeystreamBufferEnd() {return (this->m_buffer.data() + this->m_buffer.size());}

	SecByteBlock m_buffer;
	size_t m_leftOver;
};

//! \class CFB_CipherAbstractPolicy
//! \brief Policy object for feeback based stream ciphers
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CFB_CipherAbstractPolicy
{
public:
	virtual ~CFB_CipherAbstractPolicy() {}

	//! \brief Provides data alignment requirements
	//! \returns data alignment requirements, in bytes
	//! \details Internally, the default implementation returns 1. If the stream cipher is implemented
	//!   using an SSE2 ASM or intrinsics, then the value returned is usually 16.
	virtual unsigned int GetAlignment() const =0;

	//! \brief Provides number of bytes operated upon during an iteration
	//! \returns bytes operated upon during an iteration, in bytes
	//! \sa GetOptimalBlockSize()
	virtual unsigned int GetBytesPerIteration() const =0;

	//! \brief Access the feedback register
	//! \returns pointer to the first byte of the feedback register
	virtual byte * GetRegisterBegin() =0;

	//! \brief TODO
	virtual void TransformRegister() =0;

	//! \brief Flag indicating iteration support
	//! \returns true if the cipher supports iteration, false otherwise
	virtual bool CanIterate() const {return false;}

	//! \brief Iterate the cipher
	//! \param output the output buffer
	//! \param input the input buffer
	//! \param dir the direction of the cipher
	//! \param iterationCount the number of iterations to perform on the input
	//! \sa IsSelfInverting() and IsForwardTransformation()
	virtual void Iterate(byte *output, const byte *input, CipherDir dir, size_t iterationCount)
		{CRYPTOPP_UNUSED(output); CRYPTOPP_UNUSED(input); CRYPTOPP_UNUSED(dir); CRYPTOPP_UNUSED(iterationCount);
		 CRYPTOPP_ASSERT(false); /*throw 0;*/ throw Exception(Exception::OTHER_ERROR, "SimpleKeyingInterface: unexpected error");}

	//! \brief Key the cipher
	//! \param params set of NameValuePairs use to initialize this object
	//! \param key a byte array used to key the cipher
	//! \param length the size of the key array
	virtual void CipherSetKey(const NameValuePairs &params, const byte *key, size_t length) =0;

	//! \brief Resynchronize the cipher
	//! \param iv a byte array used to resynchronize the cipher
	//! \param length the size of the IV array
	virtual void CipherResynchronize(const byte *iv, size_t length)
		{CRYPTOPP_UNUSED(iv); CRYPTOPP_UNUSED(length); throw NotImplemented("SimpleKeyingInterface: this object doesn't support resynchronization");}
};

//! \class CFB_CipherConcretePolicy
//! \brief Base class for feedback based stream ciphers
//! \tparam WT word type
//! \tparam W count of words
//! \tparam BASE CFB_CipherAbstractPolicy derived base class
template <typename WT, unsigned int W, class BASE = CFB_CipherAbstractPolicy>
struct CRYPTOPP_NO_VTABLE CFB_CipherConcretePolicy : public BASE
{
	typedef WT WordType;

	//! \brief Provides data alignment requirements
	//! \returns data alignment requirements, in bytes
	//! \details Internally, the default implementation returns 1. If the stream cipher is implemented
	//!   using an SSE2 ASM or intrinsics, then the value returned is usually 16.
	unsigned int GetAlignment() const {return sizeof(WordType);}

	//! \brief Provides number of bytes operated upon during an iteration
	//! \returns bytes operated upon during an iteration, in bytes
	//! \sa GetOptimalBlockSize()
	unsigned int GetBytesPerIteration() const {return sizeof(WordType) * W;}

	//! \brief Flag indicating iteration support
	//! \returns true if the cipher supports iteration, false otherwise
	bool CanIterate() const {return true;}

	//! \brief Perform one iteration in the forward direction
	void TransformRegister() {this->Iterate(NULL, NULL, ENCRYPTION, 1);}

	//! \brief
	//! \tparam B enumeration indicating endianess
	//! \details RegisterOutput() provides alternate access to the feedback register. The
	//!   enumeration B is BigEndian or LittleEndian. Repeatedly applying operator()
	//!   results in advancing in the register.
	template <class B>
	struct RegisterOutput
	{
		RegisterOutput(byte *output, const byte *input, CipherDir dir)
			: m_output(output), m_input(input), m_dir(dir) {}

		//! \brief XOR feedback register with data
		//! \param registerWord data represented as a word type
		//! \returns reference to the next feedback register word
		inline RegisterOutput& operator()(WordType &registerWord)
		{
			CRYPTOPP_ASSERT(IsAligned<WordType>(m_output));
			CRYPTOPP_ASSERT(IsAligned<WordType>(m_input));

			if (!NativeByteOrderIs(B::ToEnum()))
				registerWord = ByteReverse(registerWord);

			if (m_dir == ENCRYPTION)
			{
				if (m_input == NULL)
				{
					CRYPTOPP_ASSERT(m_output == NULL);
				}
				else
				{
					WordType ct = *(const WordType *)m_input ^ registerWord;
					registerWord = ct;
					*(WordType*)m_output = ct;
					m_input += sizeof(WordType);
					m_output += sizeof(WordType);
				}
			}
			else
			{
				WordType ct = *(const WordType *)m_input;
				*(WordType*)m_output = registerWord ^ ct;
				registerWord = ct;
				m_input += sizeof(WordType);
				m_output += sizeof(WordType);
			}

			// registerWord is left unreversed so it can be xor-ed with further input

			return *this;
		}

		byte *m_output;
		const byte *m_input;
		CipherDir m_dir;
	};
};

//! \class CFB_CipherTemplate
//! \brief Base class for feedback based stream ciphers with SymmetricCipher interface
//! \tparam BASE AbstractPolicyHolder base class
template <class BASE>
class CRYPTOPP_NO_VTABLE CFB_CipherTemplate : public BASE
{
public:
	//! \brief Apply keystream to data
	//! \param outString a buffer to write the transformed data
	//! \param inString a buffer to read the data
	//! \param length the size fo the buffers, in bytes
	//! \details This is the primary method to operate a stream cipher. For example:
	//! <pre>
	//!     size_t size = 30;
	//!     byte plain[size] = "Do or do not; there is no try";
	//!     byte cipher[size];
	//!     ...
	//!     ChaCha20 chacha(key, keySize);
	//!     chacha.ProcessData(cipher, plain, size);
	//! </pre>
	void ProcessData(byte *outString, const byte *inString, size_t length);

	//! \brief Resynchronize the cipher
	//! \param iv a byte array used to resynchronize the cipher
	//! \param length the size of the IV array
	void Resynchronize(const byte *iv, int length=-1);

	//! \brief Provides number of ideal bytes to process
	//! \returns the ideal number of bytes to process
	//! \details Internally, the default implementation returns GetBytesPerIteration()
	//! \sa GetBytesPerIteration() and GetOptimalNextBlockSize()
	unsigned int OptimalBlockSize() const {return this->GetPolicy().GetBytesPerIteration();}

	//! \brief Provides number of ideal bytes to process
	//! \returns the ideal number of bytes to process
	//! \details Internally, the default implementation returns remaining unprocessed bytes
	//! \sa GetBytesPerIteration() and OptimalBlockSize()
	unsigned int GetOptimalNextBlockSize() const {return (unsigned int)m_leftOver;}

	//! \brief Provides number of ideal data alignment
	//! \returns the ideal data alignment, in bytes
	//! \sa GetAlignment() and OptimalBlockSize()
	unsigned int OptimalDataAlignment() const {return this->GetPolicy().GetAlignment();}

	//! \brief Flag indicating random access
	//! \returns true if the cipher is seekable, false otherwise
	//! \sa Seek()
	bool IsRandomAccess() const {return false;}

	//! \brief Determines if the cipher is self inverting
	//! \returns true if the stream cipher is self inverting, false otherwise
	bool IsSelfInverting() const {return false;}

	typedef typename BASE::PolicyInterface PolicyInterface;

protected:
	virtual void CombineMessageAndShiftRegister(byte *output, byte *reg, const byte *message, size_t length) =0;

	void UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &params);

	size_t m_leftOver;
};

//! \class CFB_EncryptionTemplate
//! \brief Base class for feedback based stream ciphers in the forward direction with SymmetricCipher interface
//! \tparam BASE AbstractPolicyHolder base class
template <class BASE = AbstractPolicyHolder<CFB_CipherAbstractPolicy, SymmetricCipher> >
class CRYPTOPP_NO_VTABLE CFB_EncryptionTemplate : public CFB_CipherTemplate<BASE>
{
	bool IsForwardTransformation() const {return true;}
	void CombineMessageAndShiftRegister(byte *output, byte *reg, const byte *message, size_t length);
};

//! \class CFB_DecryptionTemplate
//! \brief Base class for feedback based stream ciphers in the reverse direction with SymmetricCipher interface
//! \tparam BASE AbstractPolicyHolder base class
template <class BASE = AbstractPolicyHolder<CFB_CipherAbstractPolicy, SymmetricCipher> >
class CRYPTOPP_NO_VTABLE CFB_DecryptionTemplate : public CFB_CipherTemplate<BASE>
{
	bool IsForwardTransformation() const {return false;}
	void CombineMessageAndShiftRegister(byte *output, byte *reg, const byte *message, size_t length);
};

//! \class CFB_RequireFullDataBlocks
//! \brief Base class for feedback based stream ciphers with a mandatory block size
//! \tparam BASE CFB_EncryptionTemplate or CFB_DecryptionTemplate base class
template <class BASE>
class CFB_RequireFullDataBlocks : public BASE
{
public:
	unsigned int MandatoryBlockSize() const {return this->OptimalBlockSize();}
};

//! \class SymmetricCipherFinal
//! \brief SymmetricCipher implementation
//! \tparam BASE AbstractPolicyHolder derived base class
//! \tparam INFO AbstractPolicyHolder derived information class
//! \sa Weak::ARC4, ChaCha8, ChaCha12, ChaCha20, Salsa20, SEAL, Sosemanuk, WAKE
template <class BASE, class INFO = BASE>
class SymmetricCipherFinal : public AlgorithmImpl<SimpleKeyingInterfaceImpl<BASE, INFO>, INFO>
{
public:
	//! \brief Construct a stream cipher
 	SymmetricCipherFinal() {}

	//! \brief Construct a stream cipher
	//! \param key a byte array used to key the cipher
	//! \details This overload uses DEFAULT_KEYLENGTH
	SymmetricCipherFinal(const byte *key)
		{this->SetKey(key, this->DEFAULT_KEYLENGTH);}

	//! \brief Construct a stream cipher
	//! \param key a byte array used to key the cipher
	//! \param length the size of the key array
	SymmetricCipherFinal(const byte *key, size_t length)
		{this->SetKey(key, length);}

	//! \brief Construct a stream cipher
	//! \param key a byte array used to key the cipher
	//! \param length the size of the key array
	//! \param iv a byte array used as an initialization vector
	SymmetricCipherFinal(const byte *key, size_t length, const byte *iv)
		{this->SetKeyWithIV(key, length, iv);}

	//! \brief Clone a SymmetricCipher
	//! \returns a new SymmetricCipher based on this object
	Clonable * Clone() const {return static_cast<SymmetricCipher *>(new SymmetricCipherFinal<BASE, INFO>(*this));}
};

NAMESPACE_END

#ifdef CRYPTOPP_MANUALLY_INSTANTIATE_TEMPLATES
#include "strciphr.cpp"
#endif

NAMESPACE_BEGIN(CryptoPP)
CRYPTOPP_DLL_TEMPLATE_CLASS AbstractPolicyHolder<AdditiveCipherAbstractPolicy, SymmetricCipher>;
CRYPTOPP_DLL_TEMPLATE_CLASS AdditiveCipherTemplate<AbstractPolicyHolder<AdditiveCipherAbstractPolicy, SymmetricCipher> >;
CRYPTOPP_DLL_TEMPLATE_CLASS CFB_CipherTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, SymmetricCipher> >;
CRYPTOPP_DLL_TEMPLATE_CLASS CFB_EncryptionTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, SymmetricCipher> >;
CRYPTOPP_DLL_TEMPLATE_CLASS CFB_DecryptionTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, SymmetricCipher> >;

NAMESPACE_END

#if CRYPTOPP_MSC_VERSION
# pragma warning(pop)
#endif

#endif
