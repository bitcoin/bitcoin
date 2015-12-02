// serpent.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#include "serpent.h"
#include "secblock.h"
#include "misc.h"

#include "serpentp.h"

NAMESPACE_BEGIN(CryptoPP)

void Serpent_KeySchedule(word32 *k, unsigned int rounds, const byte *userKey, size_t keylen)
{
	FixedSizeSecBlock<word32, 8> k0;
	GetUserKey(LITTLE_ENDIAN_ORDER, k0.begin(), 8, userKey, keylen);
	if (keylen < 32)
		k0[keylen/4] |= word32(1) << ((keylen%4)*8);

	word32 t = k0[7];
	unsigned int i;
	for (i = 0; i < 8; ++i)
		k[i] = k0[i] = t = rotlFixed(k0[i] ^ k0[(i+3)%8] ^ k0[(i+5)%8] ^ t ^ 0x9e3779b9 ^ i, 11);
	for (i = 8; i < 4*(rounds+1); ++i)
		k[i] = t = rotlFixed(k[i-8] ^ k[i-5] ^ k[i-3] ^ t ^ 0x9e3779b9 ^ i, 11);
	k -= 20;

	word32 a,b,c,d,e;
	for (i=0; i<rounds/8; i++)
	{
		afterS2(LK); afterS2(S3); afterS3(SK);
		afterS1(LK); afterS1(S2); afterS2(SK);
		afterS0(LK); afterS0(S1); afterS1(SK);
		beforeS0(LK); beforeS0(S0); afterS0(SK);
		k += 8*4;
		afterS6(LK); afterS6(S7); afterS7(SK);
		afterS5(LK); afterS5(S6); afterS6(SK);
		afterS4(LK); afterS4(S5); afterS5(SK);
		afterS3(LK); afterS3(S4); afterS4(SK);
	}
	afterS2(LK); afterS2(S3); afterS3(SK);
}

void Serpent::Base::UncheckedSetKey(const byte *userKey, unsigned int keylen, const NameValuePairs &)
{
	AssertValidKeyLength(keylen);
	Serpent_KeySchedule(m_key, 32, userKey, keylen);
}

typedef BlockGetAndPut<word32, LittleEndian> Block;

void Serpent::Enc::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word32 a, b, c, d, e;
	
	Block::Get(inBlock)(a)(b)(c)(d);

	const word32 *k = m_key;
	unsigned int i=1;

	do
	{
		beforeS0(KX); beforeS0(S0); afterS0(LT);
		afterS0(KX); afterS0(S1); afterS1(LT);
		afterS1(KX); afterS1(S2); afterS2(LT);
		afterS2(KX); afterS2(S3); afterS3(LT);
		afterS3(KX); afterS3(S4); afterS4(LT);
		afterS4(KX); afterS4(S5); afterS5(LT);
		afterS5(KX); afterS5(S6); afterS6(LT);
		afterS6(KX); afterS6(S7);

		if (i == 4)
			break;

		++i;
		c = b;
		b = e;
		e = d;
		d = a;
		a = e;
		k += 32;
		beforeS0(LT);
	}
	while (true);

	afterS7(KX);
	
	Block::Put(xorBlock, outBlock)(d)(e)(b)(a);
}

void Serpent::Dec::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	word32 a, b, c, d, e;
	
	Block::Get(inBlock)(a)(b)(c)(d);

	const word32 *k = m_key + 96;
	unsigned int i=4;

	beforeI7(KX);
	goto start;

	do
	{
		c = b;
		b = d;
		d = e;
		k -= 32;
		beforeI7(ILT);
start:
		            beforeI7(I7); afterI7(KX); 
		afterI7(ILT); afterI7(I6); afterI6(KX); 
		afterI6(ILT); afterI6(I5); afterI5(KX); 
		afterI5(ILT); afterI5(I4); afterI4(KX); 
		afterI4(ILT); afterI4(I3); afterI3(KX); 
		afterI3(ILT); afterI3(I2); afterI2(KX); 
		afterI2(ILT); afterI2(I1); afterI1(KX); 
		afterI1(ILT); afterI1(I0); afterI0(KX);
	}
	while (--i != 0);
	
	Block::Put(xorBlock, outBlock)(a)(d)(b)(e);
}

NAMESPACE_END
