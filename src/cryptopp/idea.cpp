// idea.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "idea.h"
#include "misc.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

static const int IDEA_KEYLEN=(6*IDEA::ROUNDS+4);  // key schedule length in # of word16s

#define low16(x) ((x)&0xffff)	// compiler should be able to optimize this away if word is 16 bits
#define high16(x) ((x)>>16)

CRYPTOPP_COMPILE_ASSERT(sizeof(IDEA::Word) >= 2);

// should use an inline function but macros are still faster in MSVC 4.0
#define DirectMUL(a,b)					\
{										\
	word32 p=(word32)low16(a)*b;		\
										\
	if (p)								\
	{									\
		p = low16(p) - high16(p);		\
		a = (IDEA::Word)p - (IDEA::Word)high16(p);	\
	}									\
	else								\
		a = 1-a-b;						\
}

#ifdef IDEA_LARGECACHE
volatile bool IDEA::Base::tablesBuilt = false;
word16 IDEA::Base::log[0x10000];
word16 IDEA::Base::antilog[0x10000];

void IDEA::Base::BuildLogTables()
{
	if (tablesBuilt)
		return;
	else
	{
		tablesBuilt = true;

		IDEA::Word x=1;
		word32 i;

		for (i=0; i<0x10000; i++)
		{
			antilog[i] = (word16)x;
			DirectMUL(x, 3);
		}

		for (i=0; i<0x10000; i++)
			log[antilog[i]] = (word16)i;
	}
}

void IDEA::Base::LookupKeyLogs()
{
	IDEA::Word* Z=key;
	int r=ROUNDS;
	do
	{
		Z[0] = log[Z[0]];
		Z[3] = log[Z[3]];
		Z[4] = log[Z[4]];
		Z[5] = log[Z[5]];
		Z+=6;
	} while (--r);
	Z[0] = log[Z[0]];
	Z[3] = log[Z[3]];
}

inline void IDEA::Base::LookupMUL(IDEA::Word &a, IDEA::Word b)
{
	a = antilog[low16(log[low16(a)]+b)];
}
#endif // IDEA_LARGECACHE

void IDEA::Base::UncheckedSetKey(const byte *userKey, unsigned int length, const NameValuePairs &)
{
	AssertValidKeyLength(length);

#ifdef IDEA_LARGECACHE
	BuildLogTables();
#endif

	EnKey(userKey);

	if (!IsForwardTransformation())
		DeKey();

#ifdef IDEA_LARGECACHE
	LookupKeyLogs();
#endif
}

void IDEA::Base::EnKey (const byte *userKey)
{
	unsigned int i;

	for (i=0; i<8; i++)
		m_key[i] = ((IDEA::Word)userKey[2*i]<<8) | userKey[2*i+1];

	for (; i<IDEA_KEYLEN; i++)
	{
		unsigned int j = RoundDownToMultipleOf(i,8U)-8;
		m_key[i] = low16((m_key[j+(i+1)%8] << 9) | (m_key[j+(i+2)%8] >> 7));
	}
}

static IDEA::Word MulInv(IDEA::Word x)
{
	IDEA::Word y=x;
	for (unsigned i=0; i<15; i++)
	{
		DirectMUL(y,low16(y));
		DirectMUL(y,x);
	}
	return low16(y);
}

static inline IDEA::Word AddInv(IDEA::Word x)
{
	return low16(0-x);
}

void IDEA::Base::DeKey()
{
	FixedSizeSecBlock<IDEA::Word, 6*ROUNDS+4> tempkey;
	size_t i;

	for (i=0; i<ROUNDS; i++)
	{
		tempkey[i*6+0] = MulInv(m_key[(ROUNDS-i)*6+0]);
		tempkey[i*6+1] = AddInv(m_key[(ROUNDS-i)*6+1+(i>0)]);
		tempkey[i*6+2] = AddInv(m_key[(ROUNDS-i)*6+2-(i>0)]);
		tempkey[i*6+3] = MulInv(m_key[(ROUNDS-i)*6+3]);
		tempkey[i*6+4] =        m_key[(ROUNDS-1-i)*6+4];
		tempkey[i*6+5] =        m_key[(ROUNDS-1-i)*6+5];
	}

	tempkey[i*6+0] = MulInv(m_key[(ROUNDS-i)*6+0]);
	tempkey[i*6+1] = AddInv(m_key[(ROUNDS-i)*6+1]);
	tempkey[i*6+2] = AddInv(m_key[(ROUNDS-i)*6+2]);
	tempkey[i*6+3] = MulInv(m_key[(ROUNDS-i)*6+3]);

	m_key = tempkey;
}

#ifdef IDEA_LARGECACHE
#define MUL(a,b) LookupMUL(a,b)
#else
#define MUL(a,b) DirectMUL(a,b)
#endif

void IDEA::Base::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	typedef BlockGetAndPut<word16, BigEndian> Block;

	const IDEA::Word *key = m_key;
	IDEA::Word x0,x1,x2,x3,t0,t1;
	Block::Get(inBlock)(x0)(x1)(x2)(x3);

	for (unsigned int i=0; i<ROUNDS; i++)
	{
		MUL(x0, key[i*6+0]);
		x1 += key[i*6+1];
		x2 += key[i*6+2];
		MUL(x3, key[i*6+3]);
		t0 = x0^x2;
		MUL(t0, key[i*6+4]);
		t1 = t0 + (x1^x3);
		MUL(t1, key[i*6+5]);
		t0 += t1;
		x0 ^= t1;
		x3 ^= t0;
		t0 ^= x1;
		x1 = x2^t1;
		x2 = t0;
	}

	MUL(x0, key[ROUNDS*6+0]);
	x2 += key[ROUNDS*6+1];
	x1 += key[ROUNDS*6+2];
	MUL(x3, key[ROUNDS*6+3]);

	Block::Put(xorBlock, outBlock)(x0)(x2)(x1)(x3);
}

NAMESPACE_END
