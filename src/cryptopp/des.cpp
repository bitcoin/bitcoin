// des.cpp - modified by Wei Dai from Phil Karn's des.c
// The original code and all modifications are in the public domain.

/*
 * This is a major rewrite of my old public domain DES code written
 * circa 1987, which in turn borrowed heavily from Jim Gillogly's 1977
 * public domain code. I pretty much kept my key scheduling code, but
 * the actual encrypt/decrypt routines are taken from from Richard
 * Outerbridge's DES code as printed in Schneier's "Applied Cryptography."
 *
 * This code is in the public domain. I would appreciate bug reports and
 * enhancements.
 *
 * Phil Karn KA9Q, karn@unix.ka9q.ampr.org, August 1994.
 */

#include "pch.h"
#include "misc.h"
#include "des.h"

NAMESPACE_BEGIN(CryptoPP)

typedef BlockGetAndPut<word32, BigEndian> Block;

// Richard Outerbridge's initial permutation algorithm
/*
inline void IPERM(word32 &left, word32 &right)
{
	word32 work;

	work = ((left >> 4) ^ right) & 0x0f0f0f0f;
	right ^= work;
	left ^= work << 4;
	work = ((left >> 16) ^ right) & 0xffff;
	right ^= work;
	left ^= work << 16;
	work = ((right >> 2) ^ left) & 0x33333333;
	left ^= work;
	right ^= (work << 2);
	work = ((right >> 8) ^ left) & 0xff00ff;
	left ^= work;
	right ^= (work << 8);
	right = rotl(right, 1);
	work = (left ^ right) & 0xaaaaaaaa;
	left ^= work;
	right ^= work;
	left = rotl(left, 1);
}
inline void FPERM(word32 &left, word32 &right)
{
	word32 work;

	right = rotr(right, 1);
	work = (left ^ right) & 0xaaaaaaaa;
	left ^= work;
	right ^= work;
	left = rotr(left, 1);
	work = ((left >> 8) ^ right) & 0xff00ff;
	right ^= work;
	left ^= work << 8;
	work = ((left >> 2) ^ right) & 0x33333333;
	right ^= work;
	left ^= work << 2;
	work = ((right >> 16) ^ left) & 0xffff;
	left ^= work;
	right ^= work << 16;
	work = ((right >> 4) ^ left) & 0x0f0f0f0f;
	left ^= work;
	right ^= work << 4;
}
*/

// Wei Dai's modification to Richard Outerbridge's initial permutation 
// algorithm, this one is faster if you have access to rotate instructions 
// (like in MSVC)
static inline void IPERM(word32 &left, word32 &right)
{
	word32 work;

	right = rotlFixed(right, 4U);
	work = (left ^ right) & 0xf0f0f0f0;
	left ^= work;
	right = rotrFixed(right^work, 20U);
	work = (left ^ right) & 0xffff0000;
	left ^= work;
	right = rotrFixed(right^work, 18U);
	work = (left ^ right) & 0x33333333;
	left ^= work;
	right = rotrFixed(right^work, 6U);
	work = (left ^ right) & 0x00ff00ff;
	left ^= work;
	right = rotlFixed(right^work, 9U);
	work = (left ^ right) & 0xaaaaaaaa;
	left = rotlFixed(left^work, 1U);
	right ^= work;
}

static inline void FPERM(word32 &left, word32 &right)
{
	word32 work;

	right = rotrFixed(right, 1U);
	work = (left ^ right) & 0xaaaaaaaa;
	right ^= work;
	left = rotrFixed(left^work, 9U);
	work = (left ^ right) & 0x00ff00ff;
	right ^= work;
	left = rotlFixed(left^work, 6U);
	work = (left ^ right) & 0x33333333;
	right ^= work;
	left = rotlFixed(left^work, 18U);
	work = (left ^ right) & 0xffff0000;
	right ^= work;
	left = rotlFixed(left^work, 20U);
	work = (left ^ right) & 0xf0f0f0f0;
	right ^= work;
	left = rotrFixed(left^work, 4U);
}

void DES::Base::UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &)
{
	AssertValidKeyLength(length);

	RawSetKey(GetCipherDirection(), userKey);
}

#ifndef CRYPTOPP_IMPORTS

/* Tables defined in the Data Encryption Standard documents
 * Three of these tables, the initial permutation, the final
 * permutation and the expansion operator, are regular enough that
 * for speed, we hard-code them. They're here for reference only.
 * Also, the S and P boxes are used by a separate program, gensp.c,
 * to build the combined SP box, Spbox[]. They're also here just
 * for reference.
 */
#ifdef notdef
/* initial permutation IP */
static byte ip[] = {
	   58, 50, 42, 34, 26, 18, 10,  2,
	   60, 52, 44, 36, 28, 20, 12,  4,
	   62, 54, 46, 38, 30, 22, 14,  6,
	   64, 56, 48, 40, 32, 24, 16,  8,
	   57, 49, 41, 33, 25, 17,  9,  1,
	   59, 51, 43, 35, 27, 19, 11,  3,
	   61, 53, 45, 37, 29, 21, 13,  5,
	   63, 55, 47, 39, 31, 23, 15,  7
};

/* final permutation IP^-1 */
static byte fp[] = {
	   40,  8, 48, 16, 56, 24, 64, 32,
	   39,  7, 47, 15, 55, 23, 63, 31,
	   38,  6, 46, 14, 54, 22, 62, 30,
	   37,  5, 45, 13, 53, 21, 61, 29,
	   36,  4, 44, 12, 52, 20, 60, 28,
	   35,  3, 43, 11, 51, 19, 59, 27,
	   34,  2, 42, 10, 50, 18, 58, 26,
	   33,  1, 41,  9, 49, 17, 57, 25
};
/* expansion operation matrix */
static byte ei[] = {
	   32,  1,  2,  3,  4,  5,
		4,  5,  6,  7,  8,  9,
		8,  9, 10, 11, 12, 13,
	   12, 13, 14, 15, 16, 17,
	   16, 17, 18, 19, 20, 21,
	   20, 21, 22, 23, 24, 25,
	   24, 25, 26, 27, 28, 29,
	   28, 29, 30, 31, 32,  1
};
/* The (in)famous S-boxes */
static byte sbox[8][64] = {
	   /* S1 */
	   14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
		0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
		4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
	   15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13,

	   /* S2 */
	   15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
		3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
		0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
	   13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9,

	   /* S3 */
	   10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
	   13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
	   13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
		1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12,

	   /* S4 */
		7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
	   13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
	   10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
		3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14,

	   /* S5 */
		2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
	   14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
		4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
	   11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3,

	   /* S6 */
	   12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
	   10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
		9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
		4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13,

	   /* S7 */
		4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
	   13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
		1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
		6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12,

	   /* S8 */
	   13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
		1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
		7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
		2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11
};

/* 32-bit permutation function P used on the output of the S-boxes */
static byte p32i[] = {
	   16,  7, 20, 21,
	   29, 12, 28, 17,
		1, 15, 23, 26,
		5, 18, 31, 10,
		2,  8, 24, 14,
	   32, 27,  3,  9,
	   19, 13, 30,  6,
	   22, 11,  4, 25
};
#endif

/* permuted choice table (key) */
static const byte pc1[] = {
	   57, 49, 41, 33, 25, 17,  9,
		1, 58, 50, 42, 34, 26, 18,
	   10,  2, 59, 51, 43, 35, 27,
	   19, 11,  3, 60, 52, 44, 36,

	   63, 55, 47, 39, 31, 23, 15,
		7, 62, 54, 46, 38, 30, 22,
	   14,  6, 61, 53, 45, 37, 29,
	   21, 13,  5, 28, 20, 12,  4
};

/* number left rotations of pc1 */
static const byte totrot[] = {
	   1,2,4,6,8,10,12,14,15,17,19,21,23,25,27,28
};

/* permuted choice key (table) */
static const byte pc2[] = {
	   14, 17, 11, 24,  1,  5,
		3, 28, 15,  6, 21, 10,
	   23, 19, 12,  4, 26,  8,
	   16,  7, 27, 20, 13,  2,
	   41, 52, 31, 37, 47, 55,
	   30, 40, 51, 45, 33, 48,
	   44, 49, 39, 56, 34, 53,
	   46, 42, 50, 36, 29, 32
};

/* End of DES-defined tables */

/* bit 0 is left-most in byte */
static const int bytebit[] = {
	   0200,0100,040,020,010,04,02,01
};

/* Set key (initialize key schedule array) */
void RawDES::RawSetKey(CipherDir dir, const byte *key)
{
#if (_MSC_VER >= 1600) || (__cplusplus >= 201103L)
# define register /* Define to nothing for C++11 and above */
#endif

	SecByteBlock buffer(56+56+8);
	byte *const pc1m=buffer;                 /* place to modify pc1 into */
	byte *const pcr=pc1m+56;                 /* place to rotate pc1 into */
	byte *const ks=pcr+56;
	register int i,j,l;
	int m;
	
	for (j=0; j<56; j++) {          /* convert pc1 to bits of key */
		l=pc1[j]-1;             /* integer bit location  */
		m = l & 07;             /* find bit              */
		pc1m[j]=(key[l>>3] &    /* find which key byte l is in */
			bytebit[m])     /* and which bit of that byte */
			? 1 : 0;        /* and store 1-bit result */
	}
	for (i=0; i<16; i++) {          /* key chunk for each iteration */
		memset(ks,0,8);         /* Clear key schedule */
		for (j=0; j<56; j++)    /* rotate pc1 the right amount */
			pcr[j] = pc1m[(l=j+totrot[i])<(j<28? 28 : 56) ? l: l-28];
		/* rotate left and right halves independently */
		for (j=0; j<48; j++){   /* select bits individually */
			/* check bit that goes to ks[j] */
			if (pcr[pc2[j]-1]){
				/* mask it in if it's there */
				l= j % 6;
				ks[j/6] |= bytebit[l] >> 2;
			}
		}
		/* Now convert to odd/even interleaved form for use in F */
		k[2*i] = ((word32)ks[0] << 24)
			| ((word32)ks[2] << 16)
			| ((word32)ks[4] << 8)
			| ((word32)ks[6]);
		k[2*i+1] = ((word32)ks[1] << 24)
			| ((word32)ks[3] << 16)
			| ((word32)ks[5] << 8)
			| ((word32)ks[7]);
	}
	
	if (dir==DECRYPTION)     // reverse key schedule order
		for (i=0; i<16; i+=2)
		{
			std::swap(k[i], k[32-2-i]);
			std::swap(k[i+1], k[32-1-i]);
		}
}

void RawDES::RawProcessBlock(word32 &l_, word32 &r_) const
{
	word32 l = l_, r = r_;
	const word32 *kptr=k;

	for (unsigned i=0; i<8; i++)
	{
		word32 work = rotrFixed(r, 4U) ^ kptr[4*i+0];
		l ^= Spbox[6][(work) & 0x3f]
		  ^  Spbox[4][(work >> 8) & 0x3f]
		  ^  Spbox[2][(work >> 16) & 0x3f]
		  ^  Spbox[0][(work >> 24) & 0x3f];
		work = r ^ kptr[4*i+1];
		l ^= Spbox[7][(work) & 0x3f]
		  ^  Spbox[5][(work >> 8) & 0x3f]
		  ^  Spbox[3][(work >> 16) & 0x3f]
		  ^  Spbox[1][(work >> 24) & 0x3f];

		work = rotrFixed(l, 4U) ^ kptr[4*i+2];
		r ^= Spbox[6][(work) & 0x3f]
		  ^  Spbox[4][(work >> 8) & 0x3f]
		  ^  Spbox[2][(work >> 16) & 0x3f]
		  ^  Spbox[0][(work >> 24) & 0x3f];
		work = l ^ kptr[4*i+3];
		r ^= Spbox[7][(work) & 0x3f]
		  ^  Spbox[5][(work >> 8) & 0x3f]
		  ^  Spbox[3][(work >> 16) & 0x3f]
		  ^  Spbox[1][(work >> 24) & 0x3f];
	}

	l_ = l; r_ = r;
}

void DES_EDE2::Base::UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &)
{
	AssertValidKeyLength(length);

	m_des1.RawSetKey(GetCipherDirection(), userKey);
	m_des2.RawSetKey(ReverseCipherDir(GetCipherDirection()), userKey+8);
}

void DES_EDE2::Base::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word32 l,r;
	Block::Get(inBlock)(l)(r);
	IPERM(l,r);
	m_des1.RawProcessBlock(l, r);
	m_des2.RawProcessBlock(r, l);
	m_des1.RawProcessBlock(l, r);
	FPERM(l,r);
	Block::Put(xorBlock, outBlock)(r)(l);
}

void DES_EDE3::Base::UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &)
{
	AssertValidKeyLength(length);

	m_des1.RawSetKey(GetCipherDirection(), userKey + (IsForwardTransformation() ? 0 : 16));
	m_des2.RawSetKey(ReverseCipherDir(GetCipherDirection()), userKey + 8);
	m_des3.RawSetKey(GetCipherDirection(), userKey + (IsForwardTransformation() ? 16 : 0));
}

void DES_EDE3::Base::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word32 l,r;
	Block::Get(inBlock)(l)(r);
	IPERM(l,r);
	m_des1.RawProcessBlock(l, r);
	m_des2.RawProcessBlock(r, l);
	m_des3.RawProcessBlock(l, r);
	FPERM(l,r);
	Block::Put(xorBlock, outBlock)(r)(l);
}

#endif	// #ifndef CRYPTOPP_IMPORTS

static inline bool CheckParity(byte b)
{
	unsigned int a = b ^ (b >> 4);
	return ((a ^ (a>>1) ^ (a>>2) ^ (a>>3)) & 1) == 1;
}

bool DES::CheckKeyParityBits(const byte *key)
{
	for (unsigned int i=0; i<8; i++)
		if (!CheckParity(key[i]))
			return false;
	return true;
}

void DES::CorrectKeyParityBits(byte *key)
{
	for (unsigned int i=0; i<8; i++)
		if (!CheckParity(key[i]))
			key[i] ^= 1;
}

// Encrypt or decrypt a block of data in ECB mode
void DES::Base::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word32 l,r;
	Block::Get(inBlock)(l)(r);
	IPERM(l,r);
	RawProcessBlock(l, r);
	FPERM(l,r);
	Block::Put(xorBlock, outBlock)(r)(l);
}

void DES_XEX3::Base::UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &)
{
	AssertValidKeyLength(length);

	if (!m_des.get())
		m_des.reset(new DES::Encryption);

	memcpy(m_x1, key + (IsForwardTransformation() ? 0 : 16), BLOCKSIZE);
	m_des->RawSetKey(GetCipherDirection(), key + 8);
	memcpy(m_x3, key + (IsForwardTransformation() ? 16 : 0), BLOCKSIZE);
}

void DES_XEX3::Base::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	xorbuf(outBlock, inBlock, m_x1, BLOCKSIZE);
	m_des->ProcessAndXorBlock(outBlock, xorBlock, outBlock);
	xorbuf(outBlock, m_x3, BLOCKSIZE);
}

NAMESPACE_END
