// rijndael.h - written and placed in the public domain by Wei Dai

//! \file rijndael.h
//! \brief Classes for Rijndael encryption algorithm
//! \details All key sizes are supported. The library only provides Rijndael with 128-bit blocks,
//!   and not 192-bit or 256-bit blocks

#ifndef CRYPTOPP_RIJNDAEL_H
#define CRYPTOPP_RIJNDAEL_H

#include "seckey.h"
#include "secblock.h"

#if CRYPTOPP_BOOL_X32
# define CRYPTOPP_DISABLE_RIJNDAEL_ASM
#endif

NAMESPACE_BEGIN(CryptoPP)

//! \brief Rijndael block cipher information
struct Rijndael_Info : public FixedBlockSize<16>, public VariableKeyLength<16, 16, 32, 8>
{
	CRYPTOPP_DLL static const char * CRYPTOPP_API StaticAlgorithmName() {return CRYPTOPP_RIJNDAEL_NAME;}
};

//! \brief Rijndael block cipher implementation details
//! \sa <a href="http://www.weidai.com/scan-mirror/cs.html#Rijndael">Rijndael</a>
class CRYPTOPP_DLL Rijndael : public Rijndael_Info, public BlockCipherDocumentation
{
	//! \brief Rijndael block cipher data processing functionss
	//! \details Provides implementation common to encryption and decryption
	class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Base : public BlockCipherImpl<Rijndael_Info>
	{
	public:
		void UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &params);

	protected:
		static void FillEncTable();
		static void FillDecTable();

		// VS2005 workaround: have to put these on seperate lines, or error C2487 is triggered in DLL build
		static const byte Se[256];
		static const byte Sd[256];

		static const word32 rcon[];

		unsigned int m_rounds;
		FixedSizeAlignedSecBlock<word32, 4*15> m_key;
	};

	//! \brief Rijndael block cipher data processing functions
	//! \details Provides implementation for encryption transformation
	class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Enc : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
#if CRYPTOPP_BOOL_X64 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X86
		size_t AdvancedProcessBlocks(const byte *inBlocks, const byte *xorBlocks, byte *outBlocks, size_t length, word32 flags) const;
#endif
	};

	//! \brief Rijndael block cipher data processing functions
	//! \details Provides implementation for decryption transformation
	class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Dec : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
#if CRYPTOPP_BOOL_AESNI_INTRINSICS_AVAILABLE
		size_t AdvancedProcessBlocks(const byte *inBlocks, const byte *xorBlocks, byte *outBlocks, size_t length, word32 flags) const;
#endif
	};

public:
	typedef BlockCipherFinal<ENCRYPTION, Enc> Encryption;
	typedef BlockCipherFinal<DECRYPTION, Dec> Decryption;
};

typedef Rijndael::Encryption RijndaelEncryption;
typedef Rijndael::Decryption RijndaelDecryption;

NAMESPACE_END

#endif
