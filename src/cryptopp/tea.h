// tea.h - written and placed in the public domain by Wei Dai

//! \file tea.h
//! \brief Classes for the TEA, BTEA and XTEA block ciphers

#ifndef CRYPTOPP_TEA_H
#define CRYPTOPP_TEA_H

#include "seckey.h"
#include "secblock.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class TEA_Info
//! \brief TEA block cipher information
struct TEA_Info : public FixedBlockSize<8>, public FixedKeyLength<16>, public VariableRounds<32>
{
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "TEA";}
};

//! \class TEA
//! \brief TEA block cipher
//! \sa <a href="http://www.weidai.com/scan-mirror/cs.html#TEA">TEA</a>
class TEA : public TEA_Info, public BlockCipherDocumentation
{
	//! \class Base
	//! \brief TEA block cipher default operation
	class CRYPTOPP_NO_VTABLE Base : public BlockCipherImpl<TEA_Info>
	{
	public:
		void UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &params);

	protected:
		FixedSizeSecBlock<word32, 4> m_k;
		word32 m_limit;
	};

	//! \class Enc
	//! \brief TEA block cipher encryption operation
	class CRYPTOPP_NO_VTABLE Enc : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

	//! \class Dec
	//! \brief TEA block cipher decryption operation
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

//! \class XTEA_Info
//! \brief XTEA block cipher information
struct XTEA_Info : public FixedBlockSize<8>, public FixedKeyLength<16>, public VariableRounds<32>
{
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "XTEA";}
};

//! \class XTEA
//! \brief XTEA block cipher
//! \sa <a href="http://www.weidai.com/scan-mirror/cs.html#TEA">XTEA</a>
class XTEA : public XTEA_Info, public BlockCipherDocumentation
{
	//! \class Base
	//! \brief XTEA block cipher default operation
	class CRYPTOPP_NO_VTABLE Base : public BlockCipherImpl<XTEA_Info>
	{
	public:
		void UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &params);

	protected:
		FixedSizeSecBlock<word32, 4> m_k;
		word32 m_limit;
	};

	//! \class Enc
	//! \brief XTEA block cipher encryption operation
	class CRYPTOPP_NO_VTABLE Enc : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

	//! \class Dec
	//! \brief XTEA block cipher decryption operation
	class CRYPTOPP_NO_VTABLE Dec : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

public:
	typedef BlockCipherFinal<ENCRYPTION, Enc> Encryption;
	typedef BlockCipherFinal<DECRYPTION, Dec> Decryption;
};

//! \class BTEA_Info
//! \brief BTEA block cipher information
struct BTEA_Info : public FixedKeyLength<16>
{
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "BTEA";}
};

//! \class BTEA
//! \brief BTEA block cipher
//! \details Corrected Block TEA as described in "xxtea". This class hasn't been tested yet.
//! \sa <a href="http://www.weidai.com/scan-mirror/cs.html#TEA">Corrected Block TEA</a>.
class BTEA : public BTEA_Info, public BlockCipherDocumentation
{
	//! \class Base
	//! \brief BTEA block cipher default operation
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

	//! \class Enc
	//! \brief BTEA block cipher encryption operation
	class CRYPTOPP_NO_VTABLE Enc : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

	//! \class Dec
	//! \brief BTEA block cipher decryption operation
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
