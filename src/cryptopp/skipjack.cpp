// skipjack.cpp - modified by Wei Dai from Paulo Barreto's skipjack32.c,
// which is public domain according to his web site.

#include "pch.h"

#ifndef CRYPTOPP_IMPORTS

#include "skipjack.h"

/*
 *	Optimized implementation of SKIPJACK algorithm
 *
 *	originally written by Panu Rissanen <bande@lut.fi> 1998.06.24
 *	optimized by Mark Tillotson <markt@chaos.org.uk> 1998.06.25
 *	optimized by Paulo Barreto <pbarreto@nw.com.br> 1998.06.30
 */

NAMESPACE_BEGIN(CryptoPP)

/**
 * The F-table byte permutation (see description of the G-box permutation)
 */
const byte SKIPJACK::Base::fTable[256] = {
	0xa3,0xd7,0x09,0x83,0xf8,0x48,0xf6,0xf4,0xb3,0x21,0x15,0x78,0x99,0xb1,0xaf,0xf9,
	0xe7,0x2d,0x4d,0x8a,0xce,0x4c,0xca,0x2e,0x52,0x95,0xd9,0x1e,0x4e,0x38,0x44,0x28,
	0x0a,0xdf,0x02,0xa0,0x17,0xf1,0x60,0x68,0x12,0xb7,0x7a,0xc3,0xe9,0xfa,0x3d,0x53,
	0x96,0x84,0x6b,0xba,0xf2,0x63,0x9a,0x19,0x7c,0xae,0xe5,0xf5,0xf7,0x16,0x6a,0xa2,
	0x39,0xb6,0x7b,0x0f,0xc1,0x93,0x81,0x1b,0xee,0xb4,0x1a,0xea,0xd0,0x91,0x2f,0xb8,
	0x55,0xb9,0xda,0x85,0x3f,0x41,0xbf,0xe0,0x5a,0x58,0x80,0x5f,0x66,0x0b,0xd8,0x90,
	0x35,0xd5,0xc0,0xa7,0x33,0x06,0x65,0x69,0x45,0x00,0x94,0x56,0x6d,0x98,0x9b,0x76,
	0x97,0xfc,0xb2,0xc2,0xb0,0xfe,0xdb,0x20,0xe1,0xeb,0xd6,0xe4,0xdd,0x47,0x4a,0x1d,
	0x42,0xed,0x9e,0x6e,0x49,0x3c,0xcd,0x43,0x27,0xd2,0x07,0xd4,0xde,0xc7,0x67,0x18,
	0x89,0xcb,0x30,0x1f,0x8d,0xc6,0x8f,0xaa,0xc8,0x74,0xdc,0xc9,0x5d,0x5c,0x31,0xa4,
	0x70,0x88,0x61,0x2c,0x9f,0x0d,0x2b,0x87,0x50,0x82,0x54,0x64,0x26,0x7d,0x03,0x40,
	0x34,0x4b,0x1c,0x73,0xd1,0xc4,0xfd,0x3b,0xcc,0xfb,0x7f,0xab,0xe6,0x3e,0x5b,0xa5,
	0xad,0x04,0x23,0x9c,0x14,0x51,0x22,0xf0,0x29,0x79,0x71,0x7e,0xff,0x8c,0x0e,0xe2,
	0x0c,0xef,0xbc,0x72,0x75,0x6f,0x37,0xa1,0xec,0xd3,0x8e,0x62,0x8b,0x86,0x10,0xe8,
	0x08,0x77,0x11,0xbe,0x92,0x4f,0x24,0xc5,0x32,0x36,0x9d,0xcf,0xf3,0xa6,0xbb,0xac,
	0x5e,0x6c,0xa9,0x13,0x57,0x25,0xb5,0xe3,0xbd,0xa8,0x3a,0x01,0x05,0x59,0x2a,0x46
};

/**
 * The key-dependent permutation G on V^16 is a four-round Feistel network.
 * The round function is a fixed byte-substitution table (permutation on V^8),
 * the F-table.  Each round of G incorporates a single byte from the key.
 */
#define g(tab, w, i, j, k, l) \
{ \
	w ^= (word)tab[i*256 + (w & 0xff)] << 8; \
	w ^= (word)tab[j*256 + (w >>   8)]; \
	w ^= (word)tab[k*256 + (w & 0xff)] << 8; \
	w ^= (word)tab[l*256 + (w >>   8)]; \
}

#define g0(tab, w) g(tab, w, 0, 1, 2, 3)
#define g1(tab, w) g(tab, w, 4, 5, 6, 7)
#define g2(tab, w) g(tab, w, 8, 9, 0, 1)
#define g3(tab, w) g(tab, w, 2, 3, 4, 5)
#define g4(tab, w) g(tab, w, 6, 7, 8, 9)

/**
 * The inverse of the G permutation.
 */
#define h(tab, w, i, j, k, l) \
{ \
	w ^= (word)tab[l*256 + (w >>   8)]; \
	w ^= (word)tab[k*256 + (w & 0xff)] << 8; \
	w ^= (word)tab[j*256 + (w >>   8)]; \
	w ^= (word)tab[i*256 + (w & 0xff)] << 8; \
}

#define h0(tab, w) h(tab, w, 0, 1, 2, 3)
#define h1(tab, w) h(tab, w, 4, 5, 6, 7)
#define h2(tab, w) h(tab, w, 8, 9, 0, 1)
#define h3(tab, w) h(tab, w, 2, 3, 4, 5)
#define h4(tab, w) h(tab, w, 6, 7, 8, 9)

/**
 * Preprocess a user key into a table to save an XOR at each F-table access.
 */
void SKIPJACK::Base::UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &)
{
	AssertValidKeyLength(length);

	/* tab[i][c] = fTable[c ^ key[i]] */
	int i;
	for (i = 0; i < 10; i++) {
		byte *t = tab+i*256, k = key[9-i];
		int c;
		for (c = 0; c < 256; c++) {
			t[c] = fTable[c ^ k];
		}
	}
}

typedef BlockGetAndPut<word16, LittleEndian> Block;

/**
 * Encrypt a single block of data.
 */
void SKIPJACK::Enc::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word16 w1, w2, w3, w4;
	Block::Get(inBlock)(w4)(w3)(w2)(w1);

	/* stepping rule A: */
	g0(tab, w1); w4 ^= w1 ^ 1;
	g1(tab, w4); w3 ^= w4 ^ 2;
	g2(tab, w3); w2 ^= w3 ^ 3;
	g3(tab, w2); w1 ^= w2 ^ 4;
	g4(tab, w1); w4 ^= w1 ^ 5;
	g0(tab, w4); w3 ^= w4 ^ 6;
	g1(tab, w3); w2 ^= w3 ^ 7;
	g2(tab, w2); w1 ^= w2 ^ 8;

	/* stepping rule B: */
	w2 ^= w1 ^  9; g3(tab, w1);
	w1 ^= w4 ^ 10; g4(tab, w4);
	w4 ^= w3 ^ 11; g0(tab, w3);
	w3 ^= w2 ^ 12; g1(tab, w2);
	w2 ^= w1 ^ 13; g2(tab, w1);
	w1 ^= w4 ^ 14; g3(tab, w4);
	w4 ^= w3 ^ 15; g4(tab, w3);
	w3 ^= w2 ^ 16; g0(tab, w2);

	/* stepping rule A: */
	g1(tab, w1); w4 ^= w1 ^ 17;
	g2(tab, w4); w3 ^= w4 ^ 18;
	g3(tab, w3); w2 ^= w3 ^ 19;
	g4(tab, w2); w1 ^= w2 ^ 20;
	g0(tab, w1); w4 ^= w1 ^ 21;
	g1(tab, w4); w3 ^= w4 ^ 22;
	g2(tab, w3); w2 ^= w3 ^ 23;
	g3(tab, w2); w1 ^= w2 ^ 24;

	/* stepping rule B: */
	w2 ^= w1 ^ 25; g4(tab, w1);
	w1 ^= w4 ^ 26; g0(tab, w4);
	w4 ^= w3 ^ 27; g1(tab, w3);
	w3 ^= w2 ^ 28; g2(tab, w2);
	w2 ^= w1 ^ 29; g3(tab, w1);
	w1 ^= w4 ^ 30; g4(tab, w4);
	w4 ^= w3 ^ 31; g0(tab, w3);
	w3 ^= w2 ^ 32; g1(tab, w2);

	Block::Put(xorBlock, outBlock)(w4)(w3)(w2)(w1);
}

/**
 * Decrypt a single block of data.
 */
void SKIPJACK::Dec::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word16 w1, w2, w3, w4;
	Block::Get(inBlock)(w4)(w3)(w2)(w1);

	/* stepping rule A: */
	h1(tab, w2); w3 ^= w2 ^ 32;
	h0(tab, w3); w4 ^= w3 ^ 31;
	h4(tab, w4); w1 ^= w4 ^ 30;
	h3(tab, w1); w2 ^= w1 ^ 29;
	h2(tab, w2); w3 ^= w2 ^ 28;
	h1(tab, w3); w4 ^= w3 ^ 27;
	h0(tab, w4); w1 ^= w4 ^ 26;
	h4(tab, w1); w2 ^= w1 ^ 25;

	/* stepping rule B: */
	w1 ^= w2 ^ 24; h3(tab, w2);
	w2 ^= w3 ^ 23; h2(tab, w3);
	w3 ^= w4 ^ 22; h1(tab, w4);
	w4 ^= w1 ^ 21; h0(tab, w1);
	w1 ^= w2 ^ 20; h4(tab, w2);
	w2 ^= w3 ^ 19; h3(tab, w3);
	w3 ^= w4 ^ 18; h2(tab, w4);
	w4 ^= w1 ^ 17; h1(tab, w1);

	/* stepping rule A: */
	h0(tab, w2); w3 ^= w2 ^ 16;
	h4(tab, w3); w4 ^= w3 ^ 15;
	h3(tab, w4); w1 ^= w4 ^ 14;
	h2(tab, w1); w2 ^= w1 ^ 13;
	h1(tab, w2); w3 ^= w2 ^ 12;
	h0(tab, w3); w4 ^= w3 ^ 11;
	h4(tab, w4); w1 ^= w4 ^ 10;
	h3(tab, w1); w2 ^= w1 ^  9;

	/* stepping rule B: */
	w1 ^= w2 ^ 8; h2(tab, w2);
	w2 ^= w3 ^ 7; h1(tab, w3);
	w3 ^= w4 ^ 6; h0(tab, w4);
	w4 ^= w1 ^ 5; h4(tab, w1);
	w1 ^= w2 ^ 4; h3(tab, w2);
	w2 ^= w3 ^ 3; h2(tab, w3);
	w3 ^= w4 ^ 2; h1(tab, w4);
	w4 ^= w1 ^ 1; h0(tab, w1);

	Block::Put(xorBlock, outBlock)(w4)(w3)(w2)(w1);
}

NAMESPACE_END

#endif
