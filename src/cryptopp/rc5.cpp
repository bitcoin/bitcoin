// rc5.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "rc5.h"
#include "misc.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

void RC5::Base::UncheckedSetKey(const byte *k, unsigned int keylen, const NameValuePairs &params)
{
	AssertValidKeyLength(keylen);

	r = GetRoundsAndThrowIfInvalid(params, this);
	sTable.New(2*(r+1));

	static const RC5_WORD MAGIC_P = 0xb7e15163L;    // magic constant P for wordsize
	static const RC5_WORD MAGIC_Q = 0x9e3779b9L;    // magic constant Q for wordsize
	static const int U=sizeof(RC5_WORD);

	const unsigned int c = STDMAX((keylen+U-1)/U, 1U);	// RC6 paper says c=1 if keylen==0
	SecBlock<RC5_WORD> l(c);

	GetUserKey(LITTLE_ENDIAN_ORDER, l.begin(), c, k, keylen);

	sTable[0] = MAGIC_P;
	for (unsigned j=1; j<sTable.size();j++)
		sTable[j] = sTable[j-1] + MAGIC_Q;

	RC5_WORD a=0, b=0;
	const unsigned n = 3*STDMAX((unsigned int)sTable.size(), c);

	for (unsigned h=0; h < n; h++)
	{
		a = sTable[h % sTable.size()] = rotlFixed((sTable[h % sTable.size()] + a + b), 3);
		b = l[h % c] = rotlMod((l[h % c] + a + b), (a+b));
	}
}

typedef BlockGetAndPut<RC5::RC5_WORD, LittleEndian> Block;

void RC5::Enc::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	const RC5_WORD *sptr = sTable;
	RC5_WORD a, b;

	Block::Get(inBlock)(a)(b);
	a += sptr[0];
	b += sptr[1];
	sptr += 2;

	for(unsigned i=0; i<r; i++)
	{
		a = rotlMod(a^b,b) + sptr[2*i+0];
		b = rotlMod(a^b,a) + sptr[2*i+1];
	}

	Block::Put(xorBlock, outBlock)(a)(b);
}

void RC5::Dec::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	const RC5_WORD *sptr = sTable.end();
	RC5_WORD a, b;

	Block::Get(inBlock)(a)(b);

	for (unsigned i=0; i<r; i++)
	{
		sptr-=2;
		b = rotrMod(b-sptr[1], a) ^ a;
		a = rotrMod(a-sptr[0], b) ^ b;
	}
	b -= sTable[1];
	a -= sTable[0];

	Block::Put(xorBlock, outBlock)(a)(b);
}

NAMESPACE_END
