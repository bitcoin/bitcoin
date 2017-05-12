// twofish.h - written and placed in the public domain by Wei Dai

//! \file twofish.h
//! \brief Classes for the Twofish block cipher

#ifndef CRYPTOPP_TWOFISH_H
#define CRYPTOPP_TWOFISH_H

#include "seckey.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class Twofish_Info
//! \brief Twofish block cipher information
struct Twofish_Info : public FixedBlockSize<16>, public VariableKeyLength<16, 16, 32, 8>, FixedRounds<16>
{
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "Twofish";}
};

//! \class Twofish
//! \brief Twofish block cipher
//~ \sa <a href="http://www.weidai.com/scan-mirror/cs.html#Twofish">Twofish</a>
class Twofish : public Twofish_Info, public BlockCipherDocumentation
{
	class CRYPTOPP_NO_VTABLE Base : public BlockCipherImpl<Twofish_Info>
	{
	public:
		void UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &params);

	protected:
		static word32 h0(word32 x, const word32 *key, unsigned int kLen);
		static word32 h(word32 x, const word32 *key, unsigned int kLen);

		static const byte q[2][256];
		static const word32 mds[4][256];

		FixedSizeSecBlock<word32, 40> m_k;
		FixedSizeSecBlock<word32, 4*256> m_s;
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

typedef Twofish::Encryption TwofishEncryption;
typedef Twofish::Decryption TwofishDecryption;

NAMESPACE_END

#endif
