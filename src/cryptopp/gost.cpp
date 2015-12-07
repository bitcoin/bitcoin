#include "pch.h"
#include "gost.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

// these are the S-boxes given in Applied Cryptography 2nd Ed., p. 333
const byte GOST::Base::sBox[8][16]={
	{4, 10, 9, 2, 13, 8, 0, 14, 6, 11, 1, 12, 7, 15, 5, 3},
	{14, 11, 4, 12, 6, 13, 15, 10, 2, 3, 8, 1, 0, 7, 5, 9},
	{5, 8, 1, 13, 10, 3, 4, 2, 14, 15, 12, 7, 6, 0, 9, 11},
	{7, 13, 10, 1, 0, 8, 9, 15, 14, 4, 6, 12, 11, 2, 5, 3},
	{6, 12, 7, 1, 5, 15, 13, 8, 4, 10, 9, 14, 0, 3, 11, 2},
	{4, 11, 10, 0, 7, 2, 1, 13, 3, 6, 8, 5, 9, 12, 15, 14},
	{13, 11, 4, 1, 3, 15, 5, 9, 0, 10, 14, 7, 6, 8, 2, 12},
	{1, 15, 13, 0, 5, 7, 10, 4, 9, 2, 3, 14, 6, 11, 8, 12}};

/*	// these are the S-boxes given in the GOST source code listing in Applied
	// Cryptography 2nd Ed., p. 644.  they appear to be from the DES S-boxes
	{13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7 },
	{ 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1 },
	{12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11 },
	{ 2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9 },
	{ 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15 },
	{10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8 },
	{15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10 },
	{14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7 }}; 
*/

volatile bool GOST::Base::sTableCalculated = false;
word32 GOST::Base::sTable[4][256];

void GOST::Base::UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &)
{
	AssertValidKeyLength(length);

	PrecalculateSTable();

	GetUserKey(LITTLE_ENDIAN_ORDER, key.begin(), 8, userKey, KEYLENGTH);
}

void GOST::Base::PrecalculateSTable()
{
	if (!sTableCalculated)
	{
		for (unsigned i = 0; i < 4; i++)
			for (unsigned j = 0; j < 256; j++) 
			{
				word32 temp = sBox[2*i][j%16] | (sBox[2*i+1][j/16] << 4);
				sTable[i][j] = rotlMod(temp, 11+8*i);
			}

		sTableCalculated=true;
	}
}

#define f(x)  ( t=x,												\
				sTable[3][GETBYTE(t, 3)] ^ sTable[2][GETBYTE(t, 2)]	\
			  ^ sTable[1][GETBYTE(t, 1)] ^ sTable[0][GETBYTE(t, 0)]	)

typedef BlockGetAndPut<word32, LittleEndian> Block;

void GOST::Enc::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word32 n1, n2, t;

	Block::Get(inBlock)(n1)(n2);

	for (unsigned int i=0; i<3; i++)
	{
		n2 ^= f(n1+key[0]);
		n1 ^= f(n2+key[1]);
		n2 ^= f(n1+key[2]);
		n1 ^= f(n2+key[3]);
		n2 ^= f(n1+key[4]);
		n1 ^= f(n2+key[5]);
		n2 ^= f(n1+key[6]);
		n1 ^= f(n2+key[7]);
	}

	n2 ^= f(n1+key[7]);
	n1 ^= f(n2+key[6]);
	n2 ^= f(n1+key[5]);
	n1 ^= f(n2+key[4]);
	n2 ^= f(n1+key[3]);
	n1 ^= f(n2+key[2]);
	n2 ^= f(n1+key[1]);
	n1 ^= f(n2+key[0]);

	Block::Put(xorBlock, outBlock)(n2)(n1);
}

void GOST::Dec::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word32 n1, n2, t;

	Block::Get(inBlock)(n1)(n2);

	n2 ^= f(n1+key[0]);
	n1 ^= f(n2+key[1]);
	n2 ^= f(n1+key[2]);
	n1 ^= f(n2+key[3]);
	n2 ^= f(n1+key[4]);
	n1 ^= f(n2+key[5]);
	n2 ^= f(n1+key[6]);
	n1 ^= f(n2+key[7]);

	for (unsigned int i=0; i<3; i++)
	{
		n2 ^= f(n1+key[7]);
		n1 ^= f(n2+key[6]);
		n2 ^= f(n1+key[5]);
		n1 ^= f(n2+key[4]);
		n2 ^= f(n1+key[3]);
		n1 ^= f(n2+key[2]);
		n2 ^= f(n1+key[1]);
		n1 ^= f(n2+key[0]);
	}

	Block::Put(xorBlock, outBlock)(n2)(n1);
}

NAMESPACE_END
