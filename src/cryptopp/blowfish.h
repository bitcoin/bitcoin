// blowfish.h - written and placed in the public domain by Wei Dai

//! \file blowfish.h
//! \brief Classes for the Blowfish block cipher

#ifndef CRYPTOPP_BLOWFISH_H
#define CRYPTOPP_BLOWFISH_H

#include "seckey.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class Blowfish_Info
//! \brief Blowfish block cipher information
struct Blowfish_Info : public FixedBlockSize<8>, public VariableKeyLength<16, 4, 56>, public FixedRounds<16>
{
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "Blowfish";}
};

// <a href="http://www.weidai.com/scan-mirror/cs.html#Blowfish">Blowfish</a>

//! \class Blowfish_Info
//! \brief Blowfish block cipher
class Blowfish : public Blowfish_Info, public BlockCipherDocumentation
{
	//! \class Base
	//! \brief Class specific implementation and overrides used to operate the cipher.
	//! \details Implementations and overrides in \p Base apply to both \p ENCRYPTION and \p DECRYPTION directions
	class CRYPTOPP_NO_VTABLE Base : public BlockCipherImpl<Blowfish_Info>
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
		void UncheckedSetKey(const byte *key_string, unsigned int keylength, const NameValuePairs &params);

	private:
		void crypt_block(const word32 in[2], word32 out[2]) const;

		static const word32 p_init[ROUNDS+2];
		static const word32 s_init[4*256];

		FixedSizeSecBlock<word32, ROUNDS+2> pbox;
		FixedSizeSecBlock<word32, 4*256> sbox;
	};

public:
	typedef BlockCipherFinal<ENCRYPTION, Base> Encryption;
	typedef BlockCipherFinal<DECRYPTION, Base> Decryption;
};

typedef Blowfish::Encryption BlowfishEncryption;
typedef Blowfish::Decryption BlowfishDecryption;

NAMESPACE_END

#endif
