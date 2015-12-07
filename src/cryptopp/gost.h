// gost.h - written and placed in the public domain by Wei Dai

//! \file gost.h
//! \brief Classes for the GIST block cipher

#ifndef CRYPTOPP_GOST_H
#define CRYPTOPP_GOST_H

#include "seckey.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! _
struct GOST_Info : public FixedBlockSize<8>, public FixedKeyLength<32>
{
	static const char *StaticAlgorithmName() {return "GOST";}
};

/// <a href="http://www.weidai.com/scan-mirror/cs.html#GOST">GOST</a>
class GOST : public GOST_Info, public BlockCipherDocumentation
{
	class CRYPTOPP_NO_VTABLE Base : public BlockCipherImpl<GOST_Info>
	{
	public:
		void UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &params);

	protected:
		static void PrecalculateSTable();

		static const byte sBox[8][16];
		static volatile bool sTableCalculated;
		static word32 sTable[4][256];

		FixedSizeSecBlock<word32, 8> key;
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

typedef GOST::Encryption GOSTEncryption;
typedef GOST::Decryption GOSTDecryption;

NAMESPACE_END

#endif
