// rc6.cpp - written and placed in the public domain by Sean Woods
// based on Wei Dai's RC5 code.

#include "pch.h"
#include "rc6.h"
#include "misc.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

void RC6::Base::UncheckedSetKey(const byte *k, unsigned int keylen, const NameValuePairs &params)
{
	AssertValidKeyLength(keylen);

	r = GetRoundsAndThrowIfInvalid(params, this);
	sTable.New(2*(r+2));

	static const RC6_WORD MAGIC_P = 0xb7e15163L;    // magic constant P for wordsize
	static const RC6_WORD MAGIC_Q = 0x9e3779b9L;    // magic constant Q for wordsize
	static const int U=sizeof(RC6_WORD);

	const unsigned int c = STDMAX((keylen+U-1)/U, 1U);	// RC6 paper says c=1 if keylen==0
	SecBlock<RC6_WORD> l(c);

	GetUserKey(LITTLE_ENDIAN_ORDER, l.begin(), c, k, keylen);

	sTable[0] = MAGIC_P;
	for (unsigned j=1; j<sTable.size();j++)
		sTable[j] = sTable[j-1] + MAGIC_Q;

	RC6_WORD a=0, b=0;
	const unsigned n = 3*STDMAX((unsigned int)sTable.size(), c);

	for (unsigned h=0; h < n; h++)
	{
		a = sTable[h % sTable.size()] = rotlFixed((sTable[h % sTable.size()] + a + b), 3);
		b = l[h % c] = rotlMod((l[h % c] + a + b), (a+b));
	}
}

typedef BlockGetAndPut<RC6::RC6_WORD, LittleEndian> Block;

void RC6::Enc::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	const RC6_WORD *sptr = sTable;
	RC6_WORD a, b, c, d, t, u;

	Block::Get(inBlock)(a)(b)(c)(d);
	b += sptr[0];
	d += sptr[1];
	sptr += 2;

	for(unsigned i=0; i<r; i++)
	{
		t = rotlFixed(b*(2*b+1), 5);
		u = rotlFixed(d*(2*d+1), 5);
		a = rotlMod(a^t,u) + sptr[0];
		c = rotlMod(c^u,t) + sptr[1];
		t = a; a = b; b = c; c = d; d = t;
		sptr += 2;
	}

	a += sptr[0];
	c += sptr[1];

	Block::Put(xorBlock, outBlock)(a)(b)(c)(d);
}

void RC6::Dec::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	const RC6_WORD *sptr = sTable.end();
	RC6_WORD a, b, c, d, t, u;

	Block::Get(inBlock)(a)(b)(c)(d);

	sptr -= 2;
	c -= sptr[1];
	a -= sptr[0];

	for (unsigned i=0; i < r; i++)
	{
		sptr -= 2;
		t = a; a = d; d = c; c = b; b = t;
		u = rotlFixed(d*(2*d+1), 5);
		t = rotlFixed(b*(2*b+1), 5);
		c = rotrMod(c-sptr[1], t) ^ u;
		a = rotrMod(a-sptr[0], u) ^ t;
	}

	sptr -= 2;
	d -= sTable[1];
	b -= sTable[0];

	Block::Put(xorBlock, outBlock)(a)(b)(c)(d);
}

NAMESPACE_END
