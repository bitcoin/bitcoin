#ifndef CRYPTOPP_VMAC_H
#define CRYPTOPP_VMAC_H

#include "cryptlib.h"
#include "iterhash.h"
#include "seckey.h"

#if CRYPTOPP_BOOL_X32
# define CRYPTOPP_DISABLE_VMAC_ASM
#endif

NAMESPACE_BEGIN(CryptoPP)

//! \class VMAC_Base
//! \brief Class specific methods used to operate the MAC.
class VMAC_Base : public IteratedHashBase<word64, MessageAuthenticationCode>
{
public:
	std::string AlgorithmName() const {return std::string("VMAC(") + GetCipher().AlgorithmName() + ")-" + IntToString(DigestSize()*8);}
	unsigned int IVSize() const {return GetCipher().BlockSize();}
	unsigned int MinIVLength() const {return 1;}
	void Resynchronize(const byte *nonce, int length=-1);
	void GetNextIV(RandomNumberGenerator &rng, byte *IV);
	unsigned int DigestSize() const {return m_is128 ? 16 : 8;};
	void UncheckedSetKey(const byte *userKey, unsigned int keylength, const NameValuePairs &params);
	void TruncatedFinal(byte *mac, size_t size);
	unsigned int BlockSize() const {return m_L1KeyLength;}
	ByteOrder GetByteOrder() const {return LITTLE_ENDIAN_ORDER;}
	unsigned int OptimalDataAlignment() const;

protected:
	virtual BlockCipher & AccessCipher() =0;
	virtual int DefaultDigestSize() const =0;
	const BlockCipher & GetCipher() const {return const_cast<VMAC_Base *>(this)->AccessCipher();}
	void HashEndianCorrectedBlock(const word64 *data);
	size_t HashMultipleBlocks(const word64 *input, size_t length);
	void Init() {}
	word64* StateBuf() {return NULL;}
	word64* DataBuf() {return (word64 *)m_data();}

	void VHASH_Update_SSE2(const word64 *data, size_t blocksRemainingInWord64, int tagPart);
#if !(defined(_MSC_VER) && _MSC_VER < 1300)		// can't use function template here with VC6
	template <bool T_128BitTag>
#endif
	void VHASH_Update_Template(const word64 *data, size_t blockRemainingInWord128);
	void VHASH_Update(const word64 *data, size_t blocksRemainingInWord128);

#if CRYPTOPP_DOXYGEN_PROCESSING
	private:  // hide from documentation
#endif

	CRYPTOPP_BLOCK_1(polyState, word64, 4*(m_is128+1))
	CRYPTOPP_BLOCK_2(nhKey, word64, m_L1KeyLength/sizeof(word64) + 2*m_is128)
	CRYPTOPP_BLOCK_3(data, byte, m_L1KeyLength)
	CRYPTOPP_BLOCK_4(l3Key, word64, 2*(m_is128+1))
	CRYPTOPP_BLOCK_5(nonce, byte, IVSize())
	CRYPTOPP_BLOCK_6(pad, byte, IVSize())
	CRYPTOPP_BLOCKS_END(6)

	bool m_is128, m_padCached, m_isFirstBlock;
	int m_L1KeyLength;
};

//! \class VMAC
//! \brief The VMAC message authentication code
//! \details VMAC is a block cipher-based message authentication code algorithm
//!   using a universal hash proposed by Ted Krovetz and Wei Dai in April 2007. The
//!   algorithm was designed for high performance backed by a formal analysis.
//! \tparam T_BlockCipher block cipher
//! \tparam T_DigestBitSize digest size, in bits
//! \sa <a href="http://www.cryptolounge.org/wiki/VMAC">VMAC</a> at the Crypto Lounge.
template <class T_BlockCipher, int T_DigestBitSize = 128>
class VMAC : public SimpleKeyingInterfaceImpl<VMAC_Base, SameKeyLengthAs<T_BlockCipher, SimpleKeyingInterface::UNIQUE_IV, T_BlockCipher::BLOCKSIZE> >
{
public:
	static std::string StaticAlgorithmName() {return std::string("VMAC(") + T_BlockCipher::StaticAlgorithmName() + ")-" + IntToString(T_DigestBitSize);}

private:
	BlockCipher & AccessCipher() {return m_cipher;}
	int DefaultDigestSize() const {return T_DigestBitSize/8;}
	typename T_BlockCipher::Encryption m_cipher;
};

NAMESPACE_END

#endif
