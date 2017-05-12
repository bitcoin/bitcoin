// camellia.h - written and placed in the public domain by Wei Dai

//! \file camellia.h
//! \brief Classes for the Cameliia block cipher

#ifndef CRYPTOPP_CAMELLIA_H
#define CRYPTOPP_CAMELLIA_H

#include "config.h"
#include "seckey.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class Camellia_Info
//! \brief Camellia block cipher information
struct Camellia_Info : public FixedBlockSize<16>, public VariableKeyLength<16, 16, 32, 8>
{
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "Camellia";}
};

//! \class Camellia
//! \brief Camellia block cipher
//! \sa <a href="http://www.weidai.com/scan-mirror/cs.html#Camellia">Camellia</a>
class Camellia : public Camellia_Info, public BlockCipherDocumentation
{
	class CRYPTOPP_NO_VTABLE Base : public BlockCipherImpl<Camellia_Info>
	{
	public:
		void UncheckedSetKey(const byte *key, unsigned int keylen, const NameValuePairs &params);
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;

	protected:
		CRYPTOPP_ALIGN_DATA(4) static const byte s1[256];
		static const word32 SP[4][256];

		unsigned int m_rounds;
		SecBlock<word32> m_key;
	};

public:
	typedef BlockCipherFinal<ENCRYPTION, Base> Encryption;
	typedef BlockCipherFinal<DECRYPTION, Base> Decryption;
};

typedef Camellia::Encryption CamelliaEncryption;
typedef Camellia::Decryption CamelliaDecryption;

NAMESPACE_END

#endif
