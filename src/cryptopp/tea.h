// tea.h - written and placed in the public domain by Wei Dai

//! \file tea.h
//! \brief Classes for the TEA, BTEA and XTEA block ciphers

#ifndef CRYPTOPP_TEA_H
#define CRYPTOPP_TEA_H

#include "seckey.h"
#include "secblock.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

//! _
struct TEA_Info : public FixedBlockSize<8>, public FixedKeyLength<16>, public VariableRounds<32>
{
	static const char *StaticAlgorithmName() {return "TEA";}
};

/// <a href="http://www.weidai.com/scan-mirror/cs.html#TEA">TEA</a>
class TEA : public TEA_Info, public BlockCipherDocumentation
{
	class CRYPTOPP_NO_VTABLE Base : public BlockCipherImpl<TEA_Info>
	{
	public:
		void UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &params);

	protected:
		FixedSizeSecBlock<word32, 4> m_k;
		word32 m_limit;
	};

	class CRYPTOPP_NO_VTABLE Enc : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

	class CRYPTOPP_NO_VTABLE Dec : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

public:
	typedef BlockCipherFinal<ENCRYPTION, Enc> Encryption;
	typedef BlockCipherFinal<DECRYPTION, Dec> Decryption;
};

typedef TEA::Encryption TEAEncryption;
typedef TEA::Decryption TEADecryption;

//! _
struct XTEA_Info : public FixedBlockSize<8>, public FixedKeyLength<16>, public VariableRounds<32>
{
	static const char *StaticAlgorithmName() {return "XTEA";}
};

/// <a href="http://www.weidai.com/scan-mirror/cs.html#TEA">XTEA</a>
class XTEA : public XTEA_Info, public BlockCipherDocumentation
{
	class CRYPTOPP_NO_VTABLE Base : public BlockCipherImpl<XTEA_Info>
	{
	public:
		void UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &params);

	protected:
		FixedSizeSecBlock<word32, 4> m_k;
		word32 m_limit;
	};

	class CRYPTOPP_NO_VTABLE Enc : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

	class CRYPTOPP_NO_VTABLE Dec : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

public:
	typedef BlockCipherFinal<ENCRYPTION, Enc> Encryption;
	typedef BlockCipherFinal<DECRYPTION, Dec> Decryption;
};

//! _
struct BTEA_Info : public FixedKeyLength<16>
{
	static const char *StaticAlgorithmName() {return "BTEA";}
};

//! <a href="http://www.weidai.com/scan-mirror/cs.html#TEA">corrected Block TEA</a> (as described in "xxtea").
/*! This class hasn't been tested yet. */
class BTEA : public BTEA_Info, public BlockCipherDocumentation
{
	class CRYPTOPP_NO_VTABLE Base : public AlgorithmImpl<SimpleKeyingInterfaceImpl<BlockCipher, BTEA_Info>, BTEA_Info>, public BTEA_Info
	{
	public:
		void UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &params)
		{
			CRYPTOPP_UNUSED(length), CRYPTOPP_UNUSED(params);
			m_blockSize = params.GetIntValueWithDefault("BlockSize", 60*4);
			GetUserKey(BIG_ENDIAN_ORDER, m_k.begin(), 4, key, KEYLENGTH);
		}

		unsigned int BlockSize() const {return m_blockSize;}

	protected:
		FixedSizeSecBlock<word32, 4> m_k;
		unsigned int m_blockSize;
	};

	class CRYPTOPP_NO_VTABLE Enc : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

	class CRYPTOPP_NO_VTABLE Dec : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

public:
	typedef BlockCipherFinal<ENCRYPTION, Enc> Encryption;
	typedef BlockCipherFinal<DECRYPTION, Dec> Decryption;
};

NAMESPACE_END

#endif
