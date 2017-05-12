// mars.cpp - written and placed in the public domain by Wei Dai

// includes IBM's key setup "tweak" proposed in August 1999 (http://www.research.ibm.com/security/key-setup.txt)

#include "pch.h"
#include "mars.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

void MARS::Base::UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &)
{
	AssertValidKeyLength(length);

	// Initialize T[] with the key data
	FixedSizeSecBlock<word32, 15> T;
	GetUserKey(LITTLE_ENDIAN_ORDER, T.begin(), 15, userKey, length);
	T[length/4] = length/4;

	for (unsigned int j=0; j<4; j++)	// compute 10 words of K[] in each iteration
	{
		unsigned int i;
		// Do linear transformation
		for (i=0; i<15; i++)
			T[i] = T[i] ^ rotlFixed(T[(i+8)%15] ^ T[(i+13)%15], 3) ^ (4*i+j);

		// Do four rounds of stirring
		for (unsigned int k=0; k<4; k++)
			for (i=0; i<15; i++)
			   T[i] = rotlFixed(T[i] + Sbox[T[(i+14)%15]%512], 9);

		// Store next 10 key words into K[]
		for (i=0; i<10; i++)
			m_k[10*j+i] = T[4*i%15];
	}

	// Modify multiplication key-words
	for(unsigned int i = 5; i < 37; i += 2)
	{
		word32 m, w = m_k[i] | 3;
		m = (~w ^ (w<<1)) & (~w ^ (w>>1)) & 0x7ffffffe;
		m &= m>>1; m &= m>>2; m &= m>>4;
		m |= m<<1; m |= m<<2; m |= m<<4;
		m &= 0x7ffffffc;
		w ^= rotlMod(Sbox[265 + (m_k[i] & 3)], m_k[i-1]) & m;
		m_k[i] = w;
	}
}

#define S(a)	Sbox[(a)&0x1ff]
#define S0(a)	Sbox[(a)&0xff]
#define S1(a)	Sbox[((a)&0xff) + 256]

typedef BlockGetAndPut<word32, LittleEndian> Block;

void MARS::Enc::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	unsigned int i;
	word32 a, b, c, d, l, m, r, t;
	const word32 *k = m_k;

	Block::Get(inBlock)(a)(b)(c)(d);

	a += k[0];	b += k[1];	c += k[2];	d += k[3];

	for (i=0; i<8; i++)
	{
		b = (b ^ S0(a)) + S1(a>>8);
		c += S0(a>>16);
		a = rotrFixed(a, 24);
		d ^= S1(a);
		a += (i%4==0) ? d : 0;
		a += (i%4==1) ? b : 0;
		t = a;	a = b;	b = c;	c = d;	d = t;
	}

	for (i=0; i<16; i++)
	{
		t = rotlFixed(a, 13);
		r = rotlFixed(t * k[2*i+5], 10);
		m = a + k[2*i+4];
		l = rotlMod((S(m) ^ rotrFixed(r, 5) ^ r), r);
		c += rotlMod(m, rotrFixed(r, 5));
		(i<8 ? b : d) += l;
		(i<8 ? d : b) ^= r;
		a = b;	b = c;	c = d;	d = t;
	}

	for (i=0; i<8; i++)
	{
		a -= (i%4==2) ? d : 0;
		a -= (i%4==3) ? b : 0;
		b ^= S1(a);
		c -= S0(a>>24);
		t = rotlFixed(a, 24);
		d = (d - S1(a>>16)) ^ S0(t);
		a = b;	b = c;	c = d;	d = t;
	}

	a -= k[36];	b -= k[37];	c -= k[38];	d -= k[39];

	Block::Put(xorBlock, outBlock)(a)(b)(c)(d);
}

void MARS::Dec::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	unsigned int i;
	word32 a, b, c, d, l, m, r, t;
	const word32 *k = m_k;

	Block::Get(inBlock)(d)(c)(b)(a);

	d += k[36];	c += k[37];	b += k[38];	a += k[39];

	for (i=0; i<8; i++)
	{
		b = (b ^ S0(a)) + S1(a>>8);
		c += S0(a>>16);
		a = rotrFixed(a, 24);
		d ^= S1(a);
		a += (i%4==0) ? d : 0;
		a += (i%4==1) ? b : 0;
		t = a;	a = b;	b = c;	c = d;	d = t;
	}

	for (i=0; i<16; i++)
	{
		t = rotrFixed(a, 13);
		r = rotlFixed(a * k[35-2*i], 10);
		m = t + k[34-2*i];
		l = rotlMod((S(m) ^ rotrFixed(r, 5) ^ r), r);
		c -= rotlMod(m, rotrFixed(r, 5));
		(i<8 ? b : d) -= l;
		(i<8 ? d : b) ^= r;
		a = b;	b = c;	c = d;	d = t;
	}

	for (i=0; i<8; i++)
	{
		a -= (i%4==2) ? d : 0;
		a -= (i%4==3) ? b : 0;
		b ^= S1(a);
		c -= S0(a>>24);
		t = rotlFixed(a, 24);
		d = (d - S1(a>>16)) ^ S0(t);
		a = b;	b = c;	c = d;	d = t;
	}

	d -= k[0];	c -= k[1];	b -= k[2];	a -= k[3];

	Block::Put(xorBlock, outBlock)(d)(c)(b)(a);
}

NAMESPACE_END
