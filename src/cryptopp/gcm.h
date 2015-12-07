// gcm.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile gcm.h
//! \brief GCM block cipher mode of operation

#ifndef CRYPTOPP_GCM_H
#define CRYPTOPP_GCM_H

#include "authenc.h"
#include "modes.h"

NAMESPACE_BEGIN(CryptoPP)

//! \enum GCM_TablesOption
//! \brief Use either 2K or 64K size tables.
enum GCM_TablesOption {GCM_2K_Tables, GCM_64K_Tables};

//! \class GCM_Base
//! \brief CCM block cipher mode of operation.
//! \details Implementations and overrides in \p GCM_Base apply to both \p ENCRYPTION and \p DECRYPTION directions
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE GCM_Base : public AuthenticatedSymmetricCipherBase
{
public:
	// AuthenticatedSymmetricCipher
	std::string AlgorithmName() const
		{return GetBlockCipher().AlgorithmName() + std::string("/GCM");}
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
	unsigned int OptimalDataAlignment() const;
	IV_Requirement IVRequirement() const
		{return UNIQUE_IV;}
	unsigned int IVSize() const
		{return 12;}
	unsigned int MinIVLength() const
		{return 1;}
	unsigned int MaxIVLength() const
		{return UINT_MAX;}		// (W64LIT(1)<<61)-1 in the standard
	unsigned int DigestSize() const
		{return 16;}
	lword MaxHeaderLength() const
		{return (W64LIT(1)<<61)-1;}
	lword MaxMessageLength() const
		{return ((W64LIT(1)<<39)-256)/8;}

protected:
	// AuthenticatedSymmetricCipherBase
	bool AuthenticationIsOnPlaintext() const
		{return false;}
	unsigned int AuthenticationBlockSize() const
		{return HASH_BLOCKSIZE;}
	void SetKeyWithoutResync(const byte *userKey, size_t keylength, const NameValuePairs &params);
	void Resync(const byte *iv, size_t len);
	size_t AuthenticateBlocks(const byte *data, size_t len);
	void AuthenticateLastHeaderBlock();
	void AuthenticateLastConfidentialBlock();
	void AuthenticateLastFooterBlock(byte *mac, size_t macSize);
	SymmetricCipher & AccessSymmetricCipher() {return m_ctr;}

	virtual BlockCipher & AccessBlockCipher() =0;
	virtual GCM_TablesOption GetTablesOption() const =0;

	const BlockCipher & GetBlockCipher() const {return const_cast<GCM_Base *>(this)->AccessBlockCipher();};
	byte *HashBuffer() {return m_buffer+REQUIRED_BLOCKSIZE;}
	byte *HashKey() {return m_buffer+2*REQUIRED_BLOCKSIZE;}
	byte *MulTable() {return m_buffer+3*REQUIRED_BLOCKSIZE;}
	inline void ReverseHashBufferIfNeeded();

	class CRYPTOPP_DLL GCTR : public CTR_Mode_ExternalCipher::Encryption
	{
	protected:
		void IncrementCounterBy256();
	};

	GCTR m_ctr;
	static word16 s_reductionTable[256];
	static volatile bool s_reductionTableInitialized;
	enum {REQUIRED_BLOCKSIZE = 16, HASH_BLOCKSIZE = 16};
};

//! \class GCM_Final
//! \brief Class specific methods used to operate the cipher.
//! \tparam T_BlockCipher block cipher
//! \tparam T_TablesOption table size, either \p GCM_2K_Tables or \p GCM_64K_Tables
//! \tparam T_IsEncryption direction in which to operate the cipher
//! \details Implementations and overrides in \p GCM_Final apply to either
//!   \p ENCRYPTION or \p DECRYPTION, depending on the template parameter \p T_IsEncryption.
//! \details \p GCM_Final does not use inner classes \p Enc and \p Dec.
template <class T_BlockCipher, GCM_TablesOption T_TablesOption, bool T_IsEncryption>
class GCM_Final : public GCM_Base
{
public:
	static std::string StaticAlgorithmName()
		{return T_BlockCipher::StaticAlgorithmName() + std::string("/GCM");}
	bool IsForwardTransformation() const
		{return T_IsEncryption;}

private:
	GCM_TablesOption GetTablesOption() const {return T_TablesOption;}
	BlockCipher & AccessBlockCipher() {return m_cipher;}
	typename T_BlockCipher::Encryption m_cipher;
};

//! \class GCM
//! \brief The GCM mode of operation
//! \tparam T_BlockCipher block cipher
//! \tparam T_TablesOption table size, either \p GCM_2K_Tables or \p GCM_64K_Tables
//! \details \p GCM provides the \p Encryption and \p Decryption typedef.
//! \sa <a href="http://www.cryptolounge.org/wiki/GCM">GCM</a> at the Crypto Lounge
template <class T_BlockCipher, GCM_TablesOption T_TablesOption=GCM_2K_Tables>
struct GCM : public AuthenticatedSymmetricCipherDocumentation
{
	typedef GCM_Final<T_BlockCipher, T_TablesOption, true> Encryption;
	typedef GCM_Final<T_BlockCipher, T_TablesOption, false> Decryption;
};

NAMESPACE_END

#endif
