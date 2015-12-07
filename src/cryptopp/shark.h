// shark.h - written and placed in the public domain by Wei Dai

//! \file shark.h
//! \brief Classes for the SHARK block cipher

#ifndef CRYPTOPP_SHARK_H
#define CRYPTOPP_SHARK_H

#include "config.h"
#include "seckey.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! _
struct SHARK_Info : public FixedBlockSize<8>, public VariableKeyLength<16, 1, 16>, public VariableRounds<6, 2>
{
	static const char *StaticAlgorithmName() {return "SHARK-E";}
};

/// <a href="http://www.weidai.com/scan-mirror/cs.html#SHARK-E">SHARK-E</a>
class SHARK : public SHARK_Info, public BlockCipherDocumentation
{
	class CRYPTOPP_NO_VTABLE Base : public BlockCipherImpl<SHARK_Info>
	{
	public:
		void UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &param);

	protected:
		unsigned int m_rounds;
		SecBlock<word64> m_roundKeys;
	};

	class CRYPTOPP_NO_VTABLE Enc : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;

		// used by Base to do key setup
		void InitForKeySetup();

	private:
		static const byte sbox[256];
		static const word64 cbox[8][256];
	};

	class CRYPTOPP_NO_VTABLE Dec : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;

	private:
		static const byte sbox[256];
		static const word64 cbox[8][256];
	};

public:
	typedef BlockCipherFinal<ENCRYPTION, Enc> Encryption;
	typedef BlockCipherFinal<DECRYPTION, Dec> Decryption;
};

typedef SHARK::Encryption SHARKEncryption;
typedef SHARK::Decryption SHARKDecryption;

NAMESPACE_END

#endif
