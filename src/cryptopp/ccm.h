// ccm.h - written and placed in the public domain by Wei Dai

//! \file ccm.h
//! \brief CCM block cipher mode of operation
//! \since Crypto++ 5.6.0

#ifndef CRYPTOPP_CCM_H
#define CRYPTOPP_CCM_H

#include "authenc.h"
#include "modes.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class CCM_Base
//! \brief CCM block cipher base implementation
//! \details Base implementation of the AuthenticatedSymmetricCipher interface
//! \since Crypto++ 5.6.0
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CCM_Base : public AuthenticatedSymmetricCipherBase
{
public:
	CCM_Base()
		: m_digestSize(0), m_L(0), m_messageLength(0), m_aadLength(0) {}

	// AuthenticatedSymmetricCipher
	std::string AlgorithmName() const
		{return GetBlockCipher().AlgorithmName() + std::string("/CCM");}
	size_t MinKeyLength() const
		{return GetBlockCipher().MinKeyLength();}
	size_t MaxKeyLength() const
		{return GetBlockCipher().MaxKeyLength();}
	size_t DefaultKeyLength() const
		{return GetBlockCipher().DefaultKeyLength();}
	size_t GetValidKeyLength(size_t n) const
		{return GetBlockCipher().GetValidKeyLength(n);}
	bool IsValidKeyLength(size_t n) const
		{return GetBlockCipher().IsValidKeyLength(n);}
	unsigned int OptimalDataAlignment() const
		{return GetBlockCipher().OptimalDataAlignment();}
	IV_Requirement IVRequirement() const
		{return UNIQUE_IV;}
	unsigned int IVSize() const
		{return 8;}
	unsigned int MinIVLength() const
		{return 7;}
	unsigned int MaxIVLength() const
		{return 13;}
	unsigned int DigestSize() const
		{return m_digestSize;}
	lword MaxHeaderLength() const
		{return W64LIT(0)-1;}
	lword MaxMessageLength() const
		{return m_L<8 ? (W64LIT(1)<<(8*m_L))-1 : W64LIT(0)-1;}
	bool NeedsPrespecifiedDataLengths() const
		{return true;}
	void UncheckedSpecifyDataLengths(lword headerLength, lword messageLength, lword footerLength);

protected:
	// AuthenticatedSymmetricCipherBase
	bool AuthenticationIsOnPlaintext() const
		{return true;}
	unsigned int AuthenticationBlockSize() const
		{return GetBlockCipher().BlockSize();}
	void SetKeyWithoutResync(const byte *userKey, size_t keylength, const NameValuePairs &params);
	void Resync(const byte *iv, size_t len);
	size_t AuthenticateBlocks(const byte *data, size_t len);
	void AuthenticateLastHeaderBlock();
	void AuthenticateLastConfidentialBlock();
	void AuthenticateLastFooterBlock(byte *mac, size_t macSize);
	SymmetricCipher & AccessSymmetricCipher() {return m_ctr;}

	virtual BlockCipher & AccessBlockCipher() =0;
	virtual int DefaultDigestSize() const =0;

	const BlockCipher & GetBlockCipher() const {return const_cast<CCM_Base *>(this)->AccessBlockCipher();};
	byte *CBC_Buffer() {return m_buffer+REQUIRED_BLOCKSIZE;}

	enum {REQUIRED_BLOCKSIZE = 16};
	int m_digestSize, m_L;
	word64 m_messageLength, m_aadLength;
	CTR_Mode_ExternalCipher::Encryption m_ctr;
};

//! \class CCM_Final
//! \brief CCM block cipher final implementation
//! \tparam T_BlockCipher block cipher
//! \tparam T_DefaultDigestSize default digest size, in bytes
//! \tparam T_IsEncryption direction in which to operate the cipher
//! \since Crypto++ 5.6.0
template <class T_BlockCipher, int T_DefaultDigestSize, bool T_IsEncryption>
class CCM_Final : public CCM_Base
{
public:
	static std::string StaticAlgorithmName()
		{return T_BlockCipher::StaticAlgorithmName() + std::string("/CCM");}
	bool IsForwardTransformation() const
		{return T_IsEncryption;}

private:
	BlockCipher & AccessBlockCipher() {return m_cipher;}
	int DefaultDigestSize() const {return T_DefaultDigestSize;}
	typename T_BlockCipher::Encryption m_cipher;
};

//! \class CCM
//! \brief CCM block cipher mode of operation
//! \tparam T_BlockCipher block cipher
//! \tparam T_DefaultDigestSize default digest size, in bytes
//! \details \p CCM provides the \p Encryption and \p Decryption typedef. See GCM_Base
//!   and GCM_Final for the AuthenticatedSymmetricCipher implementation.
//! \sa <a href="http://www.cryptolounge.org/wiki/CCM">CCM</a> at the Crypto Lounge
//! \since Crypto++ 5.6.0
template <class T_BlockCipher, int T_DefaultDigestSize = 16>
struct CCM : public AuthenticatedSymmetricCipherDocumentation
{
	typedef CCM_Final<T_BlockCipher, T_DefaultDigestSize, true> Encryption;
	typedef CCM_Final<T_BlockCipher, T_DefaultDigestSize, false> Decryption;
};

NAMESPACE_END

#endif
