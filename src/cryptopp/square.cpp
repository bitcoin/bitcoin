// square.cpp - written and placed in the public domain by Wei Dai
// Based on Paulo S.L.M. Barreto's public domain implementation

#include "pch.h"
#include "config.h"

#include "square.h"
#include "misc.h"
#include "gf256.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4244)
#endif

#if CRYPTOPP_GCC_DIAGNOSTIC_AVAILABLE
# pragma GCC diagnostic ignored "-Wmissing-braces"
#endif

NAMESPACE_BEGIN(CryptoPP)

// apply theta to a roundkey
static void SquareTransform (word32 in[4], word32 out[4])
{
	static const byte G[4][4] =
	{
		0x02U, 0x01U, 0x01U, 0x03U,
		0x03U, 0x02U, 0x01U, 0x01U,
		0x01U, 0x03U, 0x02U, 0x01U,
		0x01U, 0x01U, 0x03U, 0x02U
	};

	GF256 gf256(0xf5);

	for (int i = 0; i < 4; i++)
	{
		word32 temp = 0;
		for (unsigned int j = 0; j < 4; j++)
			for (unsigned int k = 0; k < 4; k++)
				temp ^= (word32)gf256.Multiply(GETBYTE(in[i], 3-k), G[k][j]) << ((3-j)*8);
		out[i] = temp;
	}
}

#define roundkeys(i, j)		m_roundkeys[(i)*4+(j)]
#define roundkeys4(i)		(m_roundkeys+(i)*4)

void Square::Base::UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &)
{
	AssertValidKeyLength(length);

	static const word32 offset[ROUNDS] = {
	0x01000000UL, 0x02000000UL, 0x04000000UL, 0x08000000UL,
	0x10000000UL, 0x20000000UL, 0x40000000UL, 0x80000000UL,
	};

	GetUserKey(BIG_ENDIAN_ORDER, m_roundkeys.data(), KEYLENGTH/4, userKey, KEYLENGTH);

	/* apply the key evolution function */
	for (int i = 1; i < ROUNDS+1; i++)
	{
		roundkeys(i, 0) = roundkeys(i-1, 0) ^ rotlFixed(roundkeys(i-1, 3), 8U) ^ offset[i-1];
		roundkeys(i, 1) = roundkeys(i-1, 1) ^ roundkeys(i, 0);
		roundkeys(i, 2) = roundkeys(i-1, 2) ^ roundkeys(i, 1);
		roundkeys(i, 3) = roundkeys(i-1, 3) ^ roundkeys(i, 2);
	}

	/* produce the round keys */
	if (IsForwardTransformation())
	{
		for (int i = 0; i < ROUNDS; i++)
			SquareTransform (roundkeys4(i), roundkeys4(i));
	}
	else
	{
		for (int i = 0; i < ROUNDS/2; i++)
			for (int j = 0; j < 4; j++)
				std::swap(roundkeys(i, j), roundkeys(ROUNDS-i, j));
		SquareTransform (roundkeys4(ROUNDS), roundkeys4(ROUNDS));
	}
}

#define MSB(x) (((x) >> 24) & 0xffU)	/* most  significant byte */
#define SSB(x) (((x) >> 16) & 0xffU)	/* second in significance */
#define TSB(x) (((x) >>  8) & 0xffU)	/* third  in significance */
#define LSB(x) (((x)      ) & 0xffU)	/* least significant byte */

#define squareRound(text, temp, T0, T1, T2, T3, roundkey) \
{ \
	temp[0] = T0[MSB (text[0])] \
			^ T1[MSB (text[1])] \
			^ T2[MSB (text[2])] \
			^ T3[MSB (text[3])] \
			^ roundkey[0]; \
	temp[1] = T0[SSB (text[0])] \
			^ T1[SSB (text[1])] \
			^ T2[SSB (text[2])] \
			^ T3[SSB (text[3])] \
			^ roundkey[1]; \
	temp[2] = T0[TSB (text[0])] \
			^ T1[TSB (text[1])] \
			^ T2[TSB (text[2])] \
			^ T3[TSB (text[3])] \
			^ roundkey[2]; \
	temp[3] = T0[LSB (text[0])] \
			^ T1[LSB (text[1])] \
			^ T2[LSB (text[2])] \
			^ T3[LSB (text[3])] \
			^ roundkey[3]; \
} /* squareRound */

#define squareFinal(text, temp, S, roundkey) \
{ \
	text[0] = ((word32) (S[MSB (temp[0])]) << 24) \
			^ ((word32) (S[MSB (temp[1])]) << 16) \
			^ ((word32) (S[MSB (temp[2])]) <<  8) \
			^  (word32) (S[MSB (temp[3])]) \
			^ roundkey[0]; \
	text[1] = ((word32) (S[SSB (temp[0])]) << 24) \
			^ ((word32) (S[SSB (temp[1])]) << 16) \
			^ ((word32) (S[SSB (temp[2])]) <<  8) \
			^  (word32) (S[SSB (temp[3])]) \
			^ roundkey[1]; \
	text[2] = ((word32) (S[TSB (temp[0])]) << 24) \
			^ ((word32) (S[TSB (temp[1])]) << 16) \
			^ ((word32) (S[TSB (temp[2])]) <<  8) \
			^  (word32) (S[TSB (temp[3])]) \
			^ roundkey[2]; \
	text[3] = ((word32) (S[LSB (temp[0])]) << 24) \
			^ ((word32) (S[LSB (temp[1])]) << 16) \
			^ ((word32) (S[LSB (temp[2])]) <<  8) \
			^  (word32) (S[LSB (temp[3])]) \
			^ roundkey[3]; \
} /* squareFinal */

typedef BlockGetAndPut<word32, BigEndian> Block;

void Square::Enc::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word32 text[4], temp[4];
	Block::Get(inBlock)(text[0])(text[1])(text[2])(text[3]);

	/* initial key addition */
	text[0] ^= roundkeys(0, 0);
	text[1] ^= roundkeys(0, 1);
	text[2] ^= roundkeys(0, 2);
	text[3] ^= roundkeys(0, 3);

	/* ROUNDS - 1 full rounds */
	for (int i=1; i+1<ROUNDS; i+=2)
	{
		squareRound (text, temp, Te[0], Te[1], Te[2], Te[3], roundkeys4(i));
		squareRound (temp, text, Te[0], Te[1], Te[2], Te[3], roundkeys4(i+1));
	}
	squareRound (text, temp, Te[0], Te[1], Te[2], Te[3], roundkeys4(ROUNDS-1));

	/* last round (diffusion becomes only transposition) */
	squareFinal (text, temp, Se, roundkeys4(ROUNDS));

	Block::Put(xorBlock, outBlock)(text[0])(text[1])(text[2])(text[3]);
}

void Square::Dec::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word32 text[4], temp[4];
	Block::Get(inBlock)(text[0])(text[1])(text[2])(text[3]);

	/* initial key addition */
	text[0] ^= roundkeys(0, 0);
	text[1] ^= roundkeys(0, 1);
	text[2] ^= roundkeys(0, 2);
	text[3] ^= roundkeys(0, 3);

	/* ROUNDS - 1 full rounds */
	for (int i=1; i+1<ROUNDS; i+=2)
	{
		squareRound (text, temp, Td[0], Td[1], Td[2], Td[3], roundkeys4(i));
		squareRound (temp, text, Td[0], Td[1], Td[2], Td[3], roundkeys4(i+1));
	}
	squareRound (text, temp, Td[0], Td[1], Td[2], Td[3], roundkeys4(ROUNDS-1));

	/* last round (diffusion becomes only transposition) */
	squareFinal (text, temp, Sd, roundkeys4(ROUNDS));

	Block::Put(xorBlock, outBlock)(text[0])(text[1])(text[2])(text[3]);
}

NAMESPACE_END
