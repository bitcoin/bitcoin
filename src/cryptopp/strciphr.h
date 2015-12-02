/*! \file
 	This file contains helper classes for implementing stream ciphers.

	All this infrastructure may look very complex compared to what's in Crypto++ 4.x,
	but stream ciphers implementations now support a lot of new functionality,
	including better performance (minimizing copying), resetting of keys and IVs, and methods to
	query which features are supported by a cipher.

	Here's an explanation of these classes. The word "policy" is used here to mean a class with a
	set of methods that must be implemented by individual stream cipher implementations.
	This is usually much simpler than the full stream cipher API, which is implemented by
	either AdditiveCipherTemplate or CFB_CipherTemplate using the policy. So for example, an
	implementation of SEAL only needs to implement the AdditiveCipherAbstractPolicy interface
	(since it's an additive cipher, i.e., it xors a keystream into the plaintext).
	See this line in seal.h:

	typedef SymmetricCipherFinal\<ConcretePolicyHolder\<SEAL_Policy\<B\>, AdditiveCipherTemplate\<\> \> \> Encryption;

	AdditiveCipherTemplate and CFB_CipherTemplate are designed so that they don't need
	to take a policy class as a template parameter (although this is allowed), so that
	their code is not duplicated for each new cipher. Instead they each
	get a reference to an abstract policy interface by calling AccessPolicy() on itself, so
	AccessPolicy() must be overriden to return the actual policy reference. This is done
	by the ConceretePolicyHolder class. Finally, SymmetricCipherFinal implements the constructors and
	other functions that must be implemented by the most derived class.
*/

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

template <class POLICY, class BASE, class POLICY_INTERFACE = CPP_TYPENAME BASE::PolicyInterface>
class ConcretePolicyHolder : public BASE, protected POLICY
{
protected:
	const POLICY_INTERFACE & GetPolicy() const {return *this;}
	POLICY_INTERFACE & AccessPolicy() {return *this;}
};

enum KeystreamOperationFlags {OUTPUT_ALIGNED=1, INPUT_ALIGNED=2, INPUT_NULL = 4};
enum KeystreamOperation {
	WRITE_KEYSTREAM				= INPUT_NULL, 
	WRITE_KEYSTREAM_ALIGNED		= INPUT_NULL | OUTPUT_ALIGNED, 
	XOR_KEYSTREAM				= 0, 
	XOR_KEYSTREAM_INPUT_ALIGNED = INPUT_ALIGNED, 
	XOR_KEYSTREAM_OUTPUT_ALIGNED= OUTPUT_ALIGNED, 
	XOR_KEYSTREAM_BOTH_ALIGNED	= OUTPUT_ALIGNED | INPUT_ALIGNED};

struct CRYPTOPP_DLL CRYPTOPP_NO_VTABLE AdditiveCipherAbstractPolicy
{
	virtual ~AdditiveCipherAbstractPolicy() {}
	virtual unsigned int GetAlignment() const {return 1;}
	virtual unsigned int GetBytesPerIteration() const =0;
	virtual unsigned int GetOptimalBlockSize() const {return GetBytesPerIteration();}
	virtual unsigned int GetIterationsToBuffer() const =0;
	virtual void WriteKeystream(byte *keystream, size_t iterationCount)
		{OperateKeystream(KeystreamOperation(INPUT_NULL | (KeystreamOperationFlags)IsAlignedOn(keystream, GetAlignment())), keystream, NULL, iterationCount);}
	virtual bool CanOperateKeystream() const {return false;}
	virtual void OperateKeystream(KeystreamOperation operation, byte *output, const byte *input, size_t iterationCount)
		{CRYPTOPP_UNUSED(operation); CRYPTOPP_UNUSED(output); CRYPTOPP_UNUSED(input); CRYPTOPP_UNUSED(iterationCount); assert(false);}
	virtual void CipherSetKey(const NameValuePairs &params, const byte *key, size_t length) =0;
	virtual void CipherResynchronize(byte *keystreamBuffer, const byte *iv, size_t length)
		{CRYPTOPP_UNUSED(keystreamBuffer); CRYPTOPP_UNUSED(iv); CRYPTOPP_UNUSED(length); throw NotImplemented("SimpleKeyingInterface: this object doesn't support resynchronization");}
	virtual bool CipherIsRandomAccess() const =0;
	virtual void SeekToIteration(lword iterationCount)
		{CRYPTOPP_UNUSED(iterationCount); assert(!CipherIsRandomAccess()); throw NotImplemented("StreamTransformation: this object doesn't support random access");}
};

template <typename WT, unsigned int W, unsigned int X = 1, class BASE = AdditiveCipherAbstractPolicy>
struct CRYPTOPP_NO_VTABLE AdditiveCipherConcretePolicy : public BASE
{
	typedef WT WordType;
	CRYPTOPP_CONSTANT(BYTES_PER_ITERATION = sizeof(WordType) * W)

#if !(CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X64)
	unsigned int GetAlignment() const {return GetAlignmentOf<WordType>();}
#endif
	unsigned int GetBytesPerIteration() const {return BYTES_PER_ITERATION;}
	unsigned int GetIterationsToBuffer() const {return X;}
	bool CanOperateKeystream() const {return true;}
	virtual void OperateKeystream(KeystreamOperation operation, byte *output, const byte *input, size_t iterationCount) =0;
};

// use these to implement OperateKeystream
#define CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, b, i, a)	\
	PutWord(bool(x & OUTPUT_ALIGNED), b, output+i*sizeof(WordType), (x & INPUT_NULL) ? (a) : (a) ^ GetWord<WordType>(bool(x & INPUT_ALIGNED), b, input+i*sizeof(WordType)));
#define CRYPTOPP_KEYSTREAM_OUTPUT_XMM(x, i, a)	{\
	__m128i t = (x & INPUT_NULL) ? a : _mm_xor_si128(a, (x & INPUT_ALIGNED) ? _mm_load_si128((__m128i *)input+i) : _mm_loadu_si128((__m128i *)input+i));\
	if (x & OUTPUT_ALIGNED) _mm_store_si128((__m128i *)output+i, t);\
	else _mm_storeu_si128((__m128i *)output+i, t);}
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

template <class BASE = AbstractPolicyHolder<AdditiveCipherAbstractPolicy, SymmetricCipher> >
class CRYPTOPP_NO_VTABLE AdditiveCipherTemplate : public BASE, public RandomNumberGenerator
{
public:
	void GenerateBlock(byte *output, size_t size);
    void ProcessData(byte *outString, const byte *inString, size_t length);
	void Resynchronize(const byte *iv, int length=-1);
	unsigned int OptimalBlockSize() const {return this->GetPolicy().GetOptimalBlockSize();}
	unsigned int GetOptimalNextBlockSize() const {return (unsigned int)this->m_leftOver;}
	unsigned int OptimalDataAlignment() const {return this->GetPolicy().GetAlignment();}
	bool IsSelfInverting() const {return true;}
	bool IsForwardTransformation() const {return true;}
	bool IsRandomAccess() const {return this->GetPolicy().CipherIsRandomAccess();}
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

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CFB_CipherAbstractPolicy
{
public:
	virtual ~CFB_CipherAbstractPolicy() {}
	virtual unsigned int GetAlignment() const =0;
	virtual unsigned int GetBytesPerIteration() const =0;
	virtual byte * GetRegisterBegin() =0;
	virtual void TransformRegister() =0;
	virtual bool CanIterate() const {return false;}
	virtual void Iterate(byte *output, const byte *input, CipherDir dir, size_t iterationCount)
		{CRYPTOPP_UNUSED(output); CRYPTOPP_UNUSED(input); CRYPTOPP_UNUSED(dir); CRYPTOPP_UNUSED(iterationCount);
		 assert(false); /*throw 0;*/ throw Exception(Exception::OTHER_ERROR, "SimpleKeyingInterface: unexpected error");}
	virtual void CipherSetKey(const NameValuePairs &params, const byte *key, size_t length) =0;
	virtual void CipherResynchronize(const byte *iv, size_t length)
		{CRYPTOPP_UNUSED(iv); CRYPTOPP_UNUSED(length); throw NotImplemented("SimpleKeyingInterface: this object doesn't support resynchronization");}
};

template <typename WT, unsigned int W, class BASE = CFB_CipherAbstractPolicy>
struct CRYPTOPP_NO_VTABLE CFB_CipherConcretePolicy : public BASE
{
	typedef WT WordType;

	unsigned int GetAlignment() const {return sizeof(WordType);}
	unsigned int GetBytesPerIteration() const {return sizeof(WordType) * W;}
	bool CanIterate() const {return true;}
	void TransformRegister() {this->Iterate(NULL, NULL, ENCRYPTION, 1);}

	template <class B>
	struct RegisterOutput
	{
		RegisterOutput(byte *output, const byte *input, CipherDir dir)
			: m_output(output), m_input(input), m_dir(dir) {}

		inline RegisterOutput& operator()(WordType &registerWord)
		{
			assert(IsAligned<WordType>(m_output));
			assert(IsAligned<WordType>(m_input));

			if (!NativeByteOrderIs(B::ToEnum()))
				registerWord = ByteReverse(registerWord);

			if (m_dir == ENCRYPTION)
			{
				if (m_input == NULL)
					assert(m_output == NULL);
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

template <class BASE>
class CRYPTOPP_NO_VTABLE CFB_CipherTemplate : public BASE
{
public:
	void ProcessData(byte *outString, const byte *inString, size_t length);
	void Resynchronize(const byte *iv, int length=-1);
	unsigned int OptimalBlockSize() const {return this->GetPolicy().GetBytesPerIteration();}
	unsigned int GetOptimalNextBlockSize() const {return (unsigned int)m_leftOver;}
	unsigned int OptimalDataAlignment() const {return this->GetPolicy().GetAlignment();}
	bool IsRandomAccess() const {return false;}
	bool IsSelfInverting() const {return false;}

	typedef typename BASE::PolicyInterface PolicyInterface;

protected:
	virtual void CombineMessageAndShiftRegister(byte *output, byte *reg, const byte *message, size_t length) =0;

	void UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &params);

	size_t m_leftOver;
};

template <class BASE = AbstractPolicyHolder<CFB_CipherAbstractPolicy, SymmetricCipher> >
class CRYPTOPP_NO_VTABLE CFB_EncryptionTemplate : public CFB_CipherTemplate<BASE>
{
	bool IsForwardTransformation() const {return true;}
	void CombineMessageAndShiftRegister(byte *output, byte *reg, const byte *message, size_t length);
};

template <class BASE = AbstractPolicyHolder<CFB_CipherAbstractPolicy, SymmetricCipher> >
class CRYPTOPP_NO_VTABLE CFB_DecryptionTemplate : public CFB_CipherTemplate<BASE>
{
	bool IsForwardTransformation() const {return false;}
	void CombineMessageAndShiftRegister(byte *output, byte *reg, const byte *message, size_t length);
};

template <class BASE>
class CFB_RequireFullDataBlocks : public BASE
{
public:
	unsigned int MandatoryBlockSize() const {return this->OptimalBlockSize();}
};

//! _
template <class BASE, class INFO = BASE>
class SymmetricCipherFinal : public AlgorithmImpl<SimpleKeyingInterfaceImpl<BASE, INFO>, INFO>
{
public:
 	SymmetricCipherFinal() {}
	SymmetricCipherFinal(const byte *key)
		{this->SetKey(key, this->DEFAULT_KEYLENGTH);}
	SymmetricCipherFinal(const byte *key, size_t length)
		{this->SetKey(key, length);}
	SymmetricCipherFinal(const byte *key, size_t length, const byte *iv)
		{this->SetKeyWithIV(key, length, iv);}

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
