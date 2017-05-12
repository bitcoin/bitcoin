// crc.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile crc.h
//! \brief Classes for CRC-32 and CRC-32C checksum algorithm

#ifndef CRYPTOPP_CRC32_H
#define CRYPTOPP_CRC32_H

#include "cryptlib.h"

NAMESPACE_BEGIN(CryptoPP)

const word32 CRC32_NEGL = 0xffffffffL;

#ifdef IS_LITTLE_ENDIAN
#define CRC32_INDEX(c) (c & 0xff)
#define CRC32_SHIFTED(c) (c >> 8)
#else
#define CRC32_INDEX(c) (c >> 24)
#define CRC32_SHIFTED(c) (c << 8)
#endif

//! \brief CRC-32 Checksum Calculation
//! \details Uses CRC polynomial 0xEDB88320
class CRC32 : public HashTransformation
{
public:
	CRYPTOPP_CONSTANT(DIGESTSIZE = 4)
	CRC32();
	void Update(const byte *input, size_t length);
	void TruncatedFinal(byte *hash, size_t size);
	unsigned int DigestSize() const {return DIGESTSIZE;}
    CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "CRC32";}
    std::string AlgorithmName() const {return StaticAlgorithmName();}

	void UpdateByte(byte b) {m_crc = m_tab[CRC32_INDEX(m_crc) ^ b] ^ CRC32_SHIFTED(m_crc);}
	byte GetCrcByte(size_t i) const {return ((byte *)&(m_crc))[i];}

protected:
	void Reset() {m_crc = CRC32_NEGL;}

private:
	static const word32 m_tab[256];
	word32 m_crc;
};

//! \brief CRC-32C Checksum Calculation
//! \details Uses CRC polynomial 0x82F63B78
//! \since Crypto++ 5.6.4
class CRC32C : public HashTransformation
{
public:
	CRYPTOPP_CONSTANT(DIGESTSIZE = 4)
	CRC32C();
	void Update(const byte *input, size_t length);
	void TruncatedFinal(byte *hash, size_t size);
	unsigned int DigestSize() const {return DIGESTSIZE;}
    CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "CRC32C";}
    std::string AlgorithmName() const {return StaticAlgorithmName();}

	void UpdateByte(byte b) {m_crc = m_tab[CRC32_INDEX(m_crc) ^ b] ^ CRC32_SHIFTED(m_crc);}
	byte GetCrcByte(size_t i) const {return ((byte *)&(m_crc))[i];}

protected:
	void Reset() {m_crc = CRC32_NEGL;}

private:
	static const word32 m_tab[256];
	word32 m_crc;
};

NAMESPACE_END

#endif
