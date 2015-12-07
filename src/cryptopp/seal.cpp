// seal.cpp - written and placed in the public domain by Wei Dai
// updated to SEAL 3.0 by Leonard Janke

#include "pch.h"

#include "seal.h"
#include "sha.h"
#include "misc.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

#if !defined(NDEBUG) && !defined(CRYPTOPP_DOXYGEN_PROCESSING)
void SEAL_TestInstantiations()
{
	SEAL<>::Encryption x;
}
#endif

struct SEAL_Gamma
{
	SEAL_Gamma(const byte *key)
		: H(5), Z(5), D(16), lastIndex(0xffffffff)
	{
		GetUserKey(BIG_ENDIAN_ORDER, H.begin(), 5, key, 20);
		memset(D, 0, 64);
	}

	word32 Apply(word32 i);

	SecBlock<word32> H, Z, D;
	word32 lastIndex;
};

word32 SEAL_Gamma::Apply(word32 i)
{
	word32 shaIndex = i/5;
	if (shaIndex != lastIndex)
	{
		memcpy(Z, H, 20);
		D[0] = shaIndex;
		SHA::Transform(Z, D);
		lastIndex = shaIndex;
	}
	return Z[i%5];
}

template <class B>
void SEAL_Policy<B>::CipherSetKey(const NameValuePairs &params, const byte *key, size_t length)
{
	CRYPTOPP_UNUSED(length);
	m_insideCounter = m_outsideCounter = m_startCount = 0;

	unsigned int L = params.GetIntValueWithDefault("NumberOfOutputBitsPerPositionIndex", 32*1024);
	m_iterationsPerCount = L / 8192;

	SEAL_Gamma gamma(key);
	unsigned int i;

	for (i=0; i<512; i++)
		m_T[i] = gamma.Apply(i);

	for (i=0; i<256; i++)
		m_S[i] = gamma.Apply(0x1000+i);

	m_R.New(4*(L/8192));

	for (i=0; i<m_R.size(); i++)
		m_R[i] = gamma.Apply(0x2000+i);
}

template <class B>
void SEAL_Policy<B>::CipherResynchronize(byte *keystreamBuffer, const byte *IV, size_t length)
{
	CRYPTOPP_UNUSED(keystreamBuffer), CRYPTOPP_UNUSED(IV), CRYPTOPP_UNUSED(length);
	assert(length==4);

	m_outsideCounter = IV ? GetWord<word32>(false, BIG_ENDIAN_ORDER, IV) : 0;
	m_startCount = m_outsideCounter;
	m_insideCounter = 0;
}

template <class B>
void SEAL_Policy<B>::SeekToIteration(lword iterationCount)
{
	m_outsideCounter = m_startCount + (unsigned int)(iterationCount / m_iterationsPerCount);
	m_insideCounter = (unsigned int)(iterationCount % m_iterationsPerCount);
}

template <class B>
void SEAL_Policy<B>::OperateKeystream(KeystreamOperation operation, byte *output, const byte *input, size_t iterationCount)
{
	word32 a, b, c, d, n1, n2, n3, n4;
	unsigned int p, q;

	for (size_t iteration = 0; iteration < iterationCount; ++iteration)
	{
#define Ttab(x) *(word32 *)((byte *)m_T.begin()+x)

		a = m_outsideCounter ^ m_R[4*m_insideCounter];
		b = rotrFixed(m_outsideCounter, 8U) ^ m_R[4*m_insideCounter+1];
		c = rotrFixed(m_outsideCounter, 16U) ^ m_R[4*m_insideCounter+2];
		d = rotrFixed(m_outsideCounter, 24U) ^ m_R[4*m_insideCounter+3];

		for (unsigned int j=0; j<2; j++)
		{
			p = a & 0x7fc;
			b += Ttab(p);
			a = rotrFixed(a, 9U);

			p = b & 0x7fc;
			c += Ttab(p);
			b = rotrFixed(b, 9U);

			p = c & 0x7fc;
			d += Ttab(p);
			c = rotrFixed(c, 9U);

			p = d & 0x7fc;
			a += Ttab(p);
			d = rotrFixed(d, 9U);
		}

		n1 = d, n2 = b, n3 = a, n4 = c;

		p = a & 0x7fc;
		b += Ttab(p);
		a = rotrFixed(a, 9U);

		p = b & 0x7fc;
		c += Ttab(p);
		b = rotrFixed(b, 9U);

		p = c & 0x7fc;
		d += Ttab(p);
		c = rotrFixed(c, 9U);

		p = d & 0x7fc;
		a += Ttab(p);
		d = rotrFixed(d, 9U);
		
		// generate 8192 bits
		for (unsigned int i=0; i<64; i++)
		{
			p = a & 0x7fc;
			a = rotrFixed(a, 9U);
			b += Ttab(p);
			b ^= a;

			q = b & 0x7fc;
			b = rotrFixed(b, 9U);
			c ^= Ttab(q);
			c += b;

			p = (p+c) & 0x7fc;
			c = rotrFixed(c, 9U);
			d += Ttab(p);
			d ^= c;

			q = (q+d) & 0x7fc;
			d = rotrFixed(d, 9U);
			a ^= Ttab(q);
			a += d;

			p = (p+a) & 0x7fc;
			b ^= Ttab(p);
			a = rotrFixed(a, 9U);

			q = (q+b) & 0x7fc;
			c += Ttab(q);
			b = rotrFixed(b, 9U);

			p = (p+c) & 0x7fc;
			d ^= Ttab(p);
			c = rotrFixed(c, 9U);

			q = (q+d) & 0x7fc;
			d = rotrFixed(d, 9U);
			a += Ttab(q);

#define SEAL_OUTPUT(x)	\
	CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, B::ToEnum(), 0, b + m_S[4*i+0]);\
	CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, B::ToEnum(), 1, c ^ m_S[4*i+1]);\
	CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, B::ToEnum(), 2, d + m_S[4*i+2]);\
	CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, B::ToEnum(), 3, a ^ m_S[4*i+3]);

			CRYPTOPP_KEYSTREAM_OUTPUT_SWITCH(SEAL_OUTPUT, 4*4);

			if (i & 1)
			{
				a += n3;
				b += n4;
				c ^= n3;
				d ^= n4;
			}
			else
			{
				a += n1;
				b += n2;        
				c ^= n1;
				d ^= n2;
			}
		}

		if (++m_insideCounter == m_iterationsPerCount)
		{
			++m_outsideCounter;
			m_insideCounter = 0;
		}
	}

	a = b = c = d = n1 = n2 = n3 = n4 = 0;
	p = q = 0;
}

template class SEAL_Policy<BigEndian>;
template class SEAL_Policy<LittleEndian>;

NAMESPACE_END
