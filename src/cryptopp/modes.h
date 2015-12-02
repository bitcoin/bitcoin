// modes.h - written and placed in the public domain by Wei Dai

//! \file modes.h
//! \brief Class file for modes of operation.

#ifndef CRYPTOPP_MODES_H
#define CRYPTOPP_MODES_H

#include "cryptlib.h"
#include "secblock.h"
#include "misc.h"
#include "strciphr.h"
#include "argnames.h"
#include "algparam.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class CipherModeDocumentation
//! \brief Classes for operating block cipher modes of operation
//! \details Each class derived from this one defines two types, Encryption and Decryption, 
//!   both of which implement the SymmetricCipher interface.
//!   For each mode there are two classes, one of which is a template class,
//!   and the other one has a name that ends in "_ExternalCipher".
//!   The "external cipher" mode objects hold a reference to the underlying block cipher,
//!   instead of holding an instance of it. The reference must be passed in to the constructor.
//!   For the "cipher holder" classes, the CIPHER template parameter should be a class
//!   derived from BlockCipherDocumentation, for example DES or AES.
//! \details See NIST SP 800-38A for definitions of these modes. See
//!   AuthenticatedSymmetricCipherDocumentation for authenticated encryption modes.
struct CipherModeDocumentation : public SymmetricCipherDocumentation
{
};

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CipherModeBase : public SymmetricCipher
{
public:
	size_t MinKeyLength() const {return m_cipher->MinKeyLength();}
	size_t MaxKeyLength() const {return m_cipher->MaxKeyLength();}
	size_t DefaultKeyLength() const {return m_cipher->DefaultKeyLength();}
	size_t GetValidKeyLength(size_t n) const {return m_cipher->GetValidKeyLength(n);}
	bool IsValidKeyLength(size_t n) const {return m_cipher->IsValidKeyLength(n);}

	unsigned int OptimalDataAlignment() const {return m_cipher->OptimalDataAlignment();}

	unsigned int IVSize() const {return BlockSize();}
	virtual IV_Requirement IVRequirement() const =0;

	void SetCipher(BlockCipher &cipher)
	{
		this->ThrowIfResynchronizable();
		this->m_cipher = &cipher;
		this->ResizeBuffers();
	}

	void SetCipherWithIV(BlockCipher &cipher, const byte *iv, int feedbackSize = 0)
	{
		this->ThrowIfInvalidIV(iv);
		this->m_cipher = &cipher;
		this->ResizeBuffers();
		this->SetFeedbackSize(feedbackSize);
		if (this->IsResynchronizable())
			this->Resynchronize(iv);
	}

protected:
	CipherModeBase() : m_cipher(NULL) {}
	inline unsigned int BlockSize() const {assert(m_register.size() > 0); return (unsigned int)m_register.size();}
	virtual void SetFeedbackSize(unsigned int feedbackSize)
	{
		if (!(feedbackSize == 0 || feedbackSize == BlockSize()))
			throw InvalidArgument("CipherModeBase: feedback size cannot be specified for this cipher mode");
	}
	
// Thanks to Zireael, http://github.com/weidai11/cryptopp/pull/46
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual void ResizeBuffers();
#else
	virtual void ResizeBuffers()
	{
		m_register.New(m_cipher->BlockSize());
	}
#endif

	BlockCipher *m_cipher;
	AlignedSecByteBlock m_register;
};

template <class POLICY_INTERFACE>
class CRYPTOPP_NO_VTABLE ModePolicyCommonTemplate : public CipherModeBase, public POLICY_INTERFACE
{
	unsigned int GetAlignment() const {return m_cipher->OptimalDataAlignment();}
	void CipherSetKey(const NameValuePairs &params, const byte *key, size_t length);
};

template <class POLICY_INTERFACE>
void ModePolicyCommonTemplate<POLICY_INTERFACE>::CipherSetKey(const NameValuePairs &params, const byte *key, size_t length)
{
	m_cipher->SetKey(key, length, params);
	ResizeBuffers();
	int feedbackSize = params.GetIntValueWithDefault(Name::FeedbackSize(), 0);
	SetFeedbackSize(feedbackSize);
}

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CFB_ModePolicy : public ModePolicyCommonTemplate<CFB_CipherAbstractPolicy>
{
public:
	IV_Requirement IVRequirement() const {return RANDOM_IV;}
	static const char * CRYPTOPP_API StaticAlgorithmName() {return "CFB";}

protected:
	unsigned int GetBytesPerIteration() const {return m_feedbackSize;}
	byte * GetRegisterBegin() {return m_register + BlockSize() - m_feedbackSize;}
	bool CanIterate() const {return m_feedbackSize == BlockSize();}
	void Iterate(byte *output, const byte *input, CipherDir dir, size_t iterationCount);
	void TransformRegister();
	void CipherResynchronize(const byte *iv, size_t length);
	void SetFeedbackSize(unsigned int feedbackSize);
	void ResizeBuffers();

	SecByteBlock m_temp;
	unsigned int m_feedbackSize;
};

inline void CopyOrZero(void *dest, const void *src, size_t s)
{
	if (src)
		memcpy_s(dest, s, src, s);
	else
		memset(dest, 0, s);
}

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE OFB_ModePolicy : public ModePolicyCommonTemplate<AdditiveCipherAbstractPolicy>
{
public:
	bool CipherIsRandomAccess() const {return false;}
	IV_Requirement IVRequirement() const {return UNIQUE_IV;}
	static const char * CRYPTOPP_API StaticAlgorithmName() {return "OFB";}

private:
	unsigned int GetBytesPerIteration() const {return BlockSize();}
	unsigned int GetIterationsToBuffer() const {return m_cipher->OptimalNumberOfParallelBlocks();}
	void WriteKeystream(byte *keystreamBuffer, size_t iterationCount);
	void CipherResynchronize(byte *keystreamBuffer, const byte *iv, size_t length);
};

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CTR_ModePolicy : public ModePolicyCommonTemplate<AdditiveCipherAbstractPolicy>
{
public:
	bool CipherIsRandomAccess() const {return true;}
	IV_Requirement IVRequirement() const {return RANDOM_IV;}
	static const char * CRYPTOPP_API StaticAlgorithmName() {return "CTR";}

protected:
	virtual void IncrementCounterBy256();

	unsigned int GetAlignment() const {return m_cipher->OptimalDataAlignment();}
	unsigned int GetBytesPerIteration() const {return BlockSize();}
	unsigned int GetIterationsToBuffer() const {return m_cipher->OptimalNumberOfParallelBlocks();}
	void WriteKeystream(byte *buffer, size_t iterationCount)
		{OperateKeystream(WRITE_KEYSTREAM, buffer, NULL, iterationCount);}
	bool CanOperateKeystream() const {return true;}
	void OperateKeystream(KeystreamOperation operation, byte *output, const byte *input, size_t iterationCount);
	void CipherResynchronize(byte *keystreamBuffer, const byte *iv, size_t length);
	void SeekToIteration(lword iterationCount);

	AlignedSecByteBlock m_counterArray;
};

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE BlockOrientedCipherModeBase : public CipherModeBase
{
public:
	void UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &params);
	unsigned int MandatoryBlockSize() const {return BlockSize();}
	bool IsRandomAccess() const {return false;}
	bool IsSelfInverting() const {return false;}
	bool IsForwardTransformation() const {return m_cipher->IsForwardTransformation();}
	void Resynchronize(const byte *iv, int length=-1) {memcpy_s(m_register, m_register.size(), iv, ThrowIfInvalidIVLength(length));}

protected:
	bool RequireAlignedInput() const {return true;}
	
	// Thanks to Zireael, http://github.com/weidai11/cryptopp/pull/46
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	void ResizeBuffers();
#else
	void ResizeBuffers()
	{
		CipherModeBase::ResizeBuffers();
		m_buffer.New(BlockSize());
	}
#endif

	SecByteBlock m_buffer;
};

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE ECB_OneWay : public BlockOrientedCipherModeBase
{
public:
	void SetKey(const byte *key, size_t length, const NameValuePairs &params = g_nullNameValuePairs)
		{m_cipher->SetKey(key, length, params); BlockOrientedCipherModeBase::ResizeBuffers();}
	IV_Requirement IVRequirement() const {return NOT_RESYNCHRONIZABLE;}
	unsigned int OptimalBlockSize() const {return BlockSize() * m_cipher->OptimalNumberOfParallelBlocks();}
	void ProcessData(byte *outString, const byte *inString, size_t length);
	static const char * CRYPTOPP_API StaticAlgorithmName() {return "ECB";}
};

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CBC_ModeBase : public BlockOrientedCipherModeBase
{
public:
	IV_Requirement IVRequirement() const {return UNPREDICTABLE_RANDOM_IV;}
	bool RequireAlignedInput() const {return false;}
	unsigned int MinLastBlockSize() const {return 0;}
	static const char * CRYPTOPP_API StaticAlgorithmName() {return "CBC";}
};

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CBC_Encryption : public CBC_ModeBase
{
public:
	void ProcessData(byte *outString, const byte *inString, size_t length);
};

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CBC_CTS_Encryption : public CBC_Encryption
{
public:
	void SetStolenIV(byte *iv) {m_stolenIV = iv;}
	unsigned int MinLastBlockSize() const {return BlockSize()+1;}
	void ProcessLastBlock(byte *outString, const byte *inString, size_t length);
	static const char * CRYPTOPP_API StaticAlgorithmName() {return "CBC/CTS";}

protected:
	void UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &params)
	{
		CBC_Encryption::UncheckedSetKey(key, length, params);
		m_stolenIV = params.GetValueWithDefault(Name::StolenIV(), (byte *)NULL);
	}

	byte *m_stolenIV;
};

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CBC_Decryption : public CBC_ModeBase
{
public:
	void ProcessData(byte *outString, const byte *inString, size_t length);
	
protected:
	
	// Thanks to Zireael, http://github.com/weidai11/cryptopp/pull/46
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	void ResizeBuffers();
#else
	void ResizeBuffers()
	{
		BlockOrientedCipherModeBase::ResizeBuffers();
		m_temp.New(BlockSize());
	}
#endif

	AlignedSecByteBlock m_temp;
};

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CBC_CTS_Decryption : public CBC_Decryption
{
public:
	unsigned int MinLastBlockSize() const {return BlockSize()+1;}
	void ProcessLastBlock(byte *outString, const byte *inString, size_t length);
};

//! _
template <class CIPHER, class BASE>
class CipherModeFinalTemplate_CipherHolder : protected ObjectHolder<CIPHER>, public AlgorithmImpl<BASE, CipherModeFinalTemplate_CipherHolder<CIPHER, BASE> >
{
public:
	CipherModeFinalTemplate_CipherHolder()
	{
		this->m_cipher = &this->m_object;
		this->ResizeBuffers();
	}
	CipherModeFinalTemplate_CipherHolder(const byte *key, size_t length)
	{
		this->m_cipher = &this->m_object;
		this->SetKey(key, length);
	}
	CipherModeFinalTemplate_CipherHolder(const byte *key, size_t length, const byte *iv)
	{
		this->m_cipher = &this->m_object;
		this->SetKey(key, length, MakeParameters(Name::IV(), ConstByteArrayParameter(iv, this->m_cipher->BlockSize())));
	}
	CipherModeFinalTemplate_CipherHolder(const byte *key, size_t length, const byte *iv, int feedbackSize)
	{
		this->m_cipher = &this->m_object;
		this->SetKey(key, length, MakeParameters(Name::IV(), ConstByteArrayParameter(iv, this->m_cipher->BlockSize()))(Name::FeedbackSize(), feedbackSize));
	}

	static std::string CRYPTOPP_API StaticAlgorithmName()
		{return CIPHER::StaticAlgorithmName() + "/" + BASE::StaticAlgorithmName();}
};

//! \class CipherModeFinalTemplate_ExternalCipher
//! \tparam BASE CipherModeFinalTemplate_CipherHolder class
//! \brief OFB block cipher mode of operation.
template <class BASE>
class CipherModeFinalTemplate_ExternalCipher : public BASE
{
public:
	CipherModeFinalTemplate_ExternalCipher() {}
	CipherModeFinalTemplate_ExternalCipher(BlockCipher &cipher)
		{this->SetCipher(cipher);}
	CipherModeFinalTemplate_ExternalCipher(BlockCipher &cipher, const byte *iv, int feedbackSize = 0)
		{this->SetCipherWithIV(cipher, iv, feedbackSize);}

	std::string AlgorithmName() const
		{return (this->m_cipher ? this->m_cipher->AlgorithmName() + "/" : std::string("")) + BASE::StaticAlgorithmName();}
};

CRYPTOPP_DLL_TEMPLATE_CLASS CFB_CipherTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, CFB_ModePolicy> >;
CRYPTOPP_DLL_TEMPLATE_CLASS CFB_EncryptionTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, CFB_ModePolicy> >;
CRYPTOPP_DLL_TEMPLATE_CLASS CFB_DecryptionTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, CFB_ModePolicy> >;

//! \class CFB_Mode
//! \brief CFB block cipher mode of operation.
template <class CIPHER>
struct CFB_Mode : public CipherModeDocumentation
{
	typedef CipherModeFinalTemplate_CipherHolder<CPP_TYPENAME CIPHER::Encryption, ConcretePolicyHolder<Empty, CFB_EncryptionTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, CFB_ModePolicy> > > > Encryption;
	typedef CipherModeFinalTemplate_CipherHolder<CPP_TYPENAME CIPHER::Encryption, ConcretePolicyHolder<Empty, CFB_DecryptionTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, CFB_ModePolicy> > > > Decryption;
};

//! \class CFB_Mode_ExternalCipher
//! \brief CFB mode, external cipher.
struct CFB_Mode_ExternalCipher : public CipherModeDocumentation
{
	typedef CipherModeFinalTemplate_ExternalCipher<ConcretePolicyHolder<Empty, CFB_EncryptionTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, CFB_ModePolicy> > > > Encryption;
	typedef CipherModeFinalTemplate_ExternalCipher<ConcretePolicyHolder<Empty, CFB_DecryptionTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, CFB_ModePolicy> > > > Decryption;
};

//! \class CFB_FIPS_Mode
//! \brief CFB block cipher mode of operation providing FIPS validated cryptography.
//! \details Requires full block plaintext according to FIPS 800-38A
template <class CIPHER>
struct CFB_FIPS_Mode : public CipherModeDocumentation
{
	typedef CipherModeFinalTemplate_CipherHolder<CPP_TYPENAME CIPHER::Encryption, ConcretePolicyHolder<Empty, CFB_RequireFullDataBlocks<CFB_EncryptionTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, CFB_ModePolicy> > > > > Encryption;
	typedef CipherModeFinalTemplate_CipherHolder<CPP_TYPENAME CIPHER::Encryption, ConcretePolicyHolder<Empty, CFB_RequireFullDataBlocks<CFB_DecryptionTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, CFB_ModePolicy> > > > > Decryption;
};

//! \class CFB_FIPS_Mode_ExternalCipher
//! \brief CFB mode, external cipher, providing FIPS validated cryptography.
//! \details Requires full block plaintext according to FIPS 800-38A
struct CFB_FIPS_Mode_ExternalCipher : public CipherModeDocumentation
{
	typedef CipherModeFinalTemplate_ExternalCipher<ConcretePolicyHolder<Empty, CFB_RequireFullDataBlocks<CFB_EncryptionTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, CFB_ModePolicy> > > > > Encryption;
	typedef CipherModeFinalTemplate_ExternalCipher<ConcretePolicyHolder<Empty, CFB_RequireFullDataBlocks<CFB_DecryptionTemplate<AbstractPolicyHolder<CFB_CipherAbstractPolicy, CFB_ModePolicy> > > > > Decryption;
};

CRYPTOPP_DLL_TEMPLATE_CLASS AdditiveCipherTemplate<AbstractPolicyHolder<AdditiveCipherAbstractPolicy, OFB_ModePolicy> >;

//! \class OFB_Mode
//! \brief OFB block cipher mode of operation.
template <class CIPHER>
struct OFB_Mode : public CipherModeDocumentation
{
	typedef CipherModeFinalTemplate_CipherHolder<CPP_TYPENAME CIPHER::Encryption, ConcretePolicyHolder<Empty, AdditiveCipherTemplate<AbstractPolicyHolder<AdditiveCipherAbstractPolicy, OFB_ModePolicy> > > > Encryption;
	typedef Encryption Decryption;
};

//! \class OFB_Mode_ExternalCipher
//! \brief OFB mode, external cipher.
struct OFB_Mode_ExternalCipher : public CipherModeDocumentation
{
	typedef CipherModeFinalTemplate_ExternalCipher<ConcretePolicyHolder<Empty, AdditiveCipherTemplate<AbstractPolicyHolder<AdditiveCipherAbstractPolicy, OFB_ModePolicy> > > > Encryption;
	typedef Encryption Decryption;
};

CRYPTOPP_DLL_TEMPLATE_CLASS AdditiveCipherTemplate<AbstractPolicyHolder<AdditiveCipherAbstractPolicy, CTR_ModePolicy> >;
CRYPTOPP_DLL_TEMPLATE_CLASS CipherModeFinalTemplate_ExternalCipher<ConcretePolicyHolder<Empty, AdditiveCipherTemplate<AbstractPolicyHolder<AdditiveCipherAbstractPolicy, CTR_ModePolicy> > > >;

//! \class CTR_Mode
//! \brief CTR block cipher mode of operation.
template <class CIPHER>
struct CTR_Mode : public CipherModeDocumentation
{
	typedef CipherModeFinalTemplate_CipherHolder<CPP_TYPENAME CIPHER::Encryption, ConcretePolicyHolder<Empty, AdditiveCipherTemplate<AbstractPolicyHolder<AdditiveCipherAbstractPolicy, CTR_ModePolicy> > > > Encryption;
	typedef Encryption Decryption;
};

//! \class CTR_Mode_ExternalCipher
//! \brief CTR mode, external cipher.
struct CTR_Mode_ExternalCipher : public CipherModeDocumentation
{
	typedef CipherModeFinalTemplate_ExternalCipher<ConcretePolicyHolder<Empty, AdditiveCipherTemplate<AbstractPolicyHolder<AdditiveCipherAbstractPolicy, CTR_ModePolicy> > > > Encryption;
	typedef Encryption Decryption;
};

//! \class ECB_Mode
//! \brief ECB block cipher mode of operation.
template <class CIPHER>
struct ECB_Mode : public CipherModeDocumentation
{
	typedef CipherModeFinalTemplate_CipherHolder<CPP_TYPENAME CIPHER::Encryption, ECB_OneWay> Encryption;
	typedef CipherModeFinalTemplate_CipherHolder<CPP_TYPENAME CIPHER::Decryption, ECB_OneWay> Decryption;
};

CRYPTOPP_DLL_TEMPLATE_CLASS CipherModeFinalTemplate_ExternalCipher<ECB_OneWay>;

//! \class ECB_Mode_ExternalCipher
//! \brief ECB mode, external cipher.
struct ECB_Mode_ExternalCipher : public CipherModeDocumentation
{
	typedef CipherModeFinalTemplate_ExternalCipher<ECB_OneWay> Encryption;
	typedef Encryption Decryption;
};

//! CBC mode
template <class CIPHER>
struct CBC_Mode : public CipherModeDocumentation
{
	typedef CipherModeFinalTemplate_CipherHolder<CPP_TYPENAME CIPHER::Encryption, CBC_Encryption> Encryption;
	typedef CipherModeFinalTemplate_CipherHolder<CPP_TYPENAME CIPHER::Decryption, CBC_Decryption> Decryption;
};

CRYPTOPP_DLL_TEMPLATE_CLASS CipherModeFinalTemplate_ExternalCipher<CBC_Encryption>;
CRYPTOPP_DLL_TEMPLATE_CLASS CipherModeFinalTemplate_ExternalCipher<CBC_Decryption>;

//! CBC mode, external cipher
struct CBC_Mode_ExternalCipher : public CipherModeDocumentation
{
	typedef CipherModeFinalTemplate_ExternalCipher<CBC_Encryption> Encryption;
	typedef CipherModeFinalTemplate_ExternalCipher<CBC_Decryption> Decryption;
};

//! CBC mode with ciphertext stealing
template <class CIPHER>
struct CBC_CTS_Mode : public CipherModeDocumentation
{
	typedef CipherModeFinalTemplate_CipherHolder<CPP_TYPENAME CIPHER::Encryption, CBC_CTS_Encryption> Encryption;
	typedef CipherModeFinalTemplate_CipherHolder<CPP_TYPENAME CIPHER::Decryption, CBC_CTS_Decryption> Decryption;
};

CRYPTOPP_DLL_TEMPLATE_CLASS CipherModeFinalTemplate_ExternalCipher<CBC_CTS_Encryption>;
CRYPTOPP_DLL_TEMPLATE_CLASS CipherModeFinalTemplate_ExternalCipher<CBC_CTS_Decryption>;

//! \class CBC_CTS_Mode_ExternalCipher
//! \brief CBC mode with ciphertext stealing, external cipher
struct CBC_CTS_Mode_ExternalCipher : public CipherModeDocumentation
{
	typedef CipherModeFinalTemplate_ExternalCipher<CBC_CTS_Encryption> Encryption;
	typedef CipherModeFinalTemplate_ExternalCipher<CBC_CTS_Decryption> Decryption;
};

#ifdef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY
typedef CFB_Mode_ExternalCipher::Encryption CFBEncryption;
typedef CFB_Mode_ExternalCipher::Decryption CFBDecryption;
typedef OFB_Mode_ExternalCipher::Encryption OFB;
typedef CTR_Mode_ExternalCipher::Encryption CounterMode;
#endif

NAMESPACE_END

#endif
