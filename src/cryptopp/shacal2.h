// shacal.h - written and placed in the public domain by Wei Dai

//! \file shacal2.h
//! \brief Classes for the SHACAL-2 block cipher

#ifndef CRYPTOPP_SHACAL2_H
#define CRYPTOPP_SHACAL2_H

#include "seckey.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class SHACAL2_Info
//! \brief SHACAL2 block cipher information
struct SHACAL2_Info : public FixedBlockSize<32>, public VariableKeyLength<16, 16, 64>
{
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "SHACAL-2";}
};

//! \class SHACAL2
//! \brief SHACAL2 block cipher
//! \sa <a href="http://www.weidai.com/scan-mirror/cs.html#SHACAL-2">SHACAL-2</a>
class SHACAL2 : public SHACAL2_Info, public BlockCipherDocumentation
{
	class CRYPTOPP_NO_VTABLE Base : public BlockCipherImpl<SHACAL2_Info>
	{
	public:
		void UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &params);

	protected:
		FixedSizeSecBlock<word32, 64> m_key;

		static const word32 K[64];
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

typedef SHACAL2::Encryption SHACAL2Encryption;
typedef SHACAL2::Decryption SHACAL2Decryption;

NAMESPACE_END

#endif
