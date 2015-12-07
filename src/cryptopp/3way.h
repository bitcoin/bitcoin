// 3way.h - written and placed in the public domain by Wei Dai

//! \file 3way.h
//! \brief Classes for the 3-Way block cipher

#ifndef CRYPTOPP_THREEWAY_H
#define CRYPTOPP_THREEWAY_H

#include "config.h"
#include "seckey.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class ThreeWay_Info
//! \brief The cipher's key, iv, block size and name information.
struct ThreeWay_Info : public FixedBlockSize<12>, public FixedKeyLength<12>, public VariableRounds<11>
{
	static const char *StaticAlgorithmName() {return "3-Way";}
};

// <a href="http://www.weidai.com/scan-mirror/cs.html#3-Way">3-Way</a>

//! \class ThreeWay
//! \brief Provides 3-Way encryption and decryption
class ThreeWay : public ThreeWay_Info, public BlockCipherDocumentation
{
	//! \class Base
	//! \brief Class specific implementation and overrides used to operate the cipher.
	//! \details Implementations and overrides in \p Base apply to both \p ENCRYPTION and \p DECRYPTION directions
	class CRYPTOPP_NO_VTABLE Base : public BlockCipherImpl<ThreeWay_Info>
	{
	public:
		void UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &params);

	protected:
		unsigned int m_rounds;
		FixedSizeSecBlock<word32, 3> m_k;
	};

	//! \class Enc
	//! \brief Class specific methods used to operate the cipher in the forward direction.
	//! \details Implementations and overrides in \p Enc apply to \p ENCRYPTION.
	class CRYPTOPP_NO_VTABLE Enc : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

	//! \class Dec
	//! \brief Class specific methods used to operate the cipher in the reverse direction.
	//! \details Implementations and overrides in \p Dec apply to \p DECRYPTION.
	class CRYPTOPP_NO_VTABLE Dec : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

public:
	typedef BlockCipherFinal<ENCRYPTION, Enc> Encryption;
	typedef BlockCipherFinal<DECRYPTION, Dec> Decryption;
};

typedef ThreeWay::Encryption ThreeWayEncryption;
typedef ThreeWay::Decryption ThreeWayDecryption;

NAMESPACE_END

#endif
