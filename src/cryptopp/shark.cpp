// shark.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "shark.h"
#include "misc.h"
#include "modes.h"
#include "gf256.h"

#if CRYPTOPP_GCC_DIAGNOSTIC_AVAILABLE
# pragma GCC diagnostic ignored "-Wmissing-braces"
#endif

NAMESPACE_BEGIN(CryptoPP)

static word64 SHARKTransform(word64 a)
{
	static const byte iG[8][8] = {
		0xe7, 0x30, 0x90, 0x85, 0xd0, 0x4b, 0x91, 0x41, 
		0x53, 0x95, 0x9b, 0xa5, 0x96, 0xbc, 0xa1, 0x68, 
		0x02, 0x45, 0xf7, 0x65, 0x5c, 0x1f, 0xb6, 0x52, 
		0xa2, 0xca, 0x22, 0x94, 0x44, 0x63, 0x2a, 0xa2, 
		0xfc, 0x67, 0x8e, 0x10, 0x29, 0x75, 0x85, 0x71, 
		0x24, 0x45, 0xa2, 0xcf, 0x2f, 0x22, 0xc1, 0x0e, 
		0xa1, 0xf1, 0x71, 0x40, 0x91, 0x27, 0x18, 0xa5, 
		0x56, 0xf4, 0xaf, 0x32, 0xd2, 0xa4, 0xdc, 0x71, 
	};

	word64 result=0;
	GF256 gf256(0xf5);
	for (unsigned int i=0; i<8; i++)
		for(unsigned int j=0; j<8; j++) 
			result ^= word64(gf256.Multiply(iG[i][j], GF256::Element(a>>(56-8*j)))) << (56-8*i);
	return result;
}

void SHARK::Base::UncheckedSetKey(const byte *key, unsigned int keyLen, const NameValuePairs &params)
{
	AssertValidKeyLength(keyLen);

	m_rounds = GetRoundsAndThrowIfInvalid(params, this);
	m_roundKeys.New(m_rounds+1);

	// concatenate key enought times to fill a
	for (unsigned int i=0; i<(m_rounds+1)*8; i++)
		((byte *)m_roundKeys.begin())[i] = key[i%keyLen];

	SHARK::Encryption e;
	e.InitForKeySetup();
	byte IV[8] = {0,0,0,0,0,0,0,0};
	CFB_Mode_ExternalCipher::Encryption cfb(e, IV);

	cfb.ProcessString((byte *)m_roundKeys.begin(), (m_rounds+1)*8);

	ConditionalByteReverse(BIG_ENDIAN_ORDER, m_roundKeys.begin(), m_roundKeys.begin(), (m_rounds+1)*8);

	m_roundKeys[m_rounds] = SHARKTransform(m_roundKeys[m_rounds]);

	if (!IsForwardTransformation())
	{
		unsigned int i;

		// transform encryption round keys into decryption round keys
		for (i=0; i<m_rounds/2; i++)
			std::swap(m_roundKeys[i], m_roundKeys[m_rounds-i]);

		for (i=1; i<m_rounds; i++)
			m_roundKeys[i] = SHARKTransform(m_roundKeys[i]);
	}

#ifdef IS_LITTLE_ENDIAN
	m_roundKeys[0] = ByteReverse(m_roundKeys[0]);
	m_roundKeys[m_rounds] = ByteReverse(m_roundKeys[m_rounds]);
#endif
}

// construct an SHARK_Enc object with fixed round keys, to be used to initialize actual round keys
void SHARK::Enc::InitForKeySetup()
{
	m_rounds = DEFAULT_ROUNDS;
	m_roundKeys.New(DEFAULT_ROUNDS+1);

	for (unsigned int i=0; i<DEFAULT_ROUNDS; i++)
		m_roundKeys[i] = cbox[0][i];

	m_roundKeys[DEFAULT_ROUNDS] = SHARKTransform(cbox[0][DEFAULT_ROUNDS]);

#ifdef IS_LITTLE_ENDIAN
	m_roundKeys[0] = ByteReverse(m_roundKeys[0]);
	m_roundKeys[m_rounds] = ByteReverse(m_roundKeys[m_rounds]);
#endif
}

typedef word64 ArrayOf256Word64s[256];

template <const byte *sbox, const ArrayOf256Word64s *cbox>
struct SharkProcessAndXorBlock{		// VC60 workaround: problem with template functions
inline SharkProcessAndXorBlock(const word64 *roundKeys, unsigned int rounds, const byte *inBlock, const byte *xorBlock, byte *outBlock)
{
	word64 tmp = *(word64 *)inBlock ^ roundKeys[0];

	ByteOrder order = GetNativeByteOrder();
	tmp = cbox[0][GetByte(order, tmp, 0)] ^ cbox[1][GetByte(order, tmp, 1)] 
		^ cbox[2][GetByte(order, tmp, 2)] ^ cbox[3][GetByte(order, tmp, 3)] 
		^ cbox[4][GetByte(order, tmp, 4)] ^ cbox[5][GetByte(order, tmp, 5)] 
		^ cbox[6][GetByte(order, tmp, 6)] ^ cbox[7][GetByte(order, tmp, 7)]
		^ roundKeys[1];

	for(unsigned int i=2; i<rounds; i++) 
	{
		tmp = cbox[0][GETBYTE(tmp, 7)] ^ cbox[1][GETBYTE(tmp, 6)] 
			^ cbox[2][GETBYTE(tmp, 5)] ^ cbox[3][GETBYTE(tmp, 4)] 
			^ cbox[4][GETBYTE(tmp, 3)] ^ cbox[5][GETBYTE(tmp, 2)] 
			^ cbox[6][GETBYTE(tmp, 1)] ^ cbox[7][GETBYTE(tmp, 0)]
			^ roundKeys[i];
	}

	PutBlock<byte, BigEndian>(xorBlock, outBlock)
		(sbox[GETBYTE(tmp, 7)])
		(sbox[GETBYTE(tmp, 6)])
		(sbox[GETBYTE(tmp, 5)])
		(sbox[GETBYTE(tmp, 4)])
		(sbox[GETBYTE(tmp, 3)])
		(sbox[GETBYTE(tmp, 2)])
		(sbox[GETBYTE(tmp, 1)])
		(sbox[GETBYTE(tmp, 0)]);

	*(word64 *)outBlock ^= roundKeys[rounds];
}};

void SHARK::Enc::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	SharkProcessAndXorBlock<sbox, cbox>(m_roundKeys, m_rounds, inBlock, xorBlock, outBlock);
}

void SHARK::Dec::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	SharkProcessAndXorBlock<sbox, cbox>(m_roundKeys, m_rounds, inBlock, xorBlock, outBlock);
}

NAMESPACE_END
