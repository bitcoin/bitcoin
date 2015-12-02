// 3way.cpp - modifed by Wei Dai from Joan Daemen's 3way.c
// The original code and all modifications are in the public domain.

#include "pch.h"
#include "3way.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

#if !defined(NDEBUG) && !defined(CRYPTOPP_DOXYGEN_PROCESSING)
void ThreeWay_TestInstantiations()
{
	ThreeWay::Encryption x1;
	ThreeWay::Decryption x2;
}
#endif

static const word32 START_E = 0x0b0b; // round constant of first encryption round
static const word32 START_D = 0xb1b1; // round constant of first decryption round
#ifdef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
static const word32 RC_MODULUS = 0x11011;
#endif

static inline word32 reverseBits(word32 a)
{
	a = ((a & 0xAAAAAAAA) >> 1) | ((a & 0x55555555) << 1);
	a = ((a & 0xCCCCCCCC) >> 2) | ((a & 0x33333333) << 2);
	return ((a & 0xF0F0F0F0) >> 4) | ((a & 0x0F0F0F0F) << 4);
}

#define mu(a0, a1, a2)				\
{									\
	a1 = reverseBits(a1);			\
	word32 t = reverseBits(a0);		\
	a0 = reverseBits(a2);			\
	a2 = t;							\
}

#define pi_gamma_pi(a0, a1, a2)		\
{									\
	word32 b0, b2;					\
	b2 = rotlFixed(a2, 1U);				\
	b0 = rotlFixed(a0, 22U);				\
	a0 = rotlFixed(b0 ^ (a1|(~b2)), 1U);	\
	a2 = rotlFixed(b2 ^ (b0|(~a1)), 22U);\
	a1 ^= (b2|(~b0));				\
}

// thanks to Paulo Barreto for this optimized theta()
#define theta(a0, a1, a2)									\
{ 															\
	word32 b0, b1, c; 										\
	c = a0 ^ a1 ^ a2; 										\
	c = rotlFixed(c, 16U) ^ rotlFixed(c, 8U);				\
	b0 = (a0 << 24) ^ (a2 >> 8) ^ (a1 << 8) ^ (a0 >> 24); 	\
	b1 = (a1 << 24) ^ (a0 >> 8) ^ (a2 << 8) ^ (a1 >> 24); 	\
	a0 ^= c ^ b0; 											\
	a1 ^= c ^ b1; 											\
	a2 ^= c ^ (b0 >> 16) ^ (b1 << 16); 						\
}															

#define rho(a0, a1, a2)			\
{								\
	theta(a0, a1, a2);			\
	pi_gamma_pi(a0, a1, a2);	\
}											

void ThreeWay::Base::UncheckedSetKey(const byte *uk, unsigned int length, const NameValuePairs &params)
{
	AssertValidKeyLength(length);

	m_rounds = GetRoundsAndThrowIfInvalid(params, this);

	for (unsigned int i=0; i<3; i++)
		m_k[i] = (word32)uk[4*i+3] | ((word32)uk[4*i+2]<<8) | ((word32)uk[4*i+1]<<16) | ((word32)uk[4*i]<<24);

	if (!IsForwardTransformation())
	{
		theta(m_k[0], m_k[1], m_k[2]);
		mu(m_k[0], m_k[1], m_k[2]);
		m_k[0] = ByteReverse(m_k[0]);
		m_k[1] = ByteReverse(m_k[1]);
		m_k[2] = ByteReverse(m_k[2]);
	}
}

void ThreeWay::Enc::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	typedef BlockGetAndPut<word32, BigEndian> Block;

	word32 a0, a1, a2;
	Block::Get(inBlock)(a0)(a1)(a2);

	word32 rc = START_E;

	for(unsigned i=0; i<m_rounds; i++)
	{
		a0 ^= m_k[0] ^ (rc<<16);
		a1 ^= m_k[1];
		a2 ^= m_k[2] ^ rc;
		rho(a0, a1, a2);

		rc <<= 1;
		if (rc&0x10000) rc ^= 0x11011;
	}
	a0 ^= m_k[0] ^ (rc<<16);
	a1 ^= m_k[1];
	a2 ^= m_k[2] ^ rc;
	theta(a0, a1, a2);

	Block::Put(xorBlock, outBlock)(a0)(a1)(a2);
}

void ThreeWay::Dec::ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const
{
	typedef BlockGetAndPut<word32, LittleEndian> Block;

	word32 a0, a1, a2;
	Block::Get(inBlock)(a0)(a1)(a2);

	word32 rc = START_D;

	mu(a0, a1, a2);
	for(unsigned i=0; i<m_rounds; i++)
	{
		a0 ^= m_k[0] ^ (rc<<16);
		a1 ^= m_k[1];
		a2 ^= m_k[2] ^ rc;
		rho(a0, a1, a2);

		rc <<= 1;
		if (rc&0x10000) rc ^= 0x11011;
	}
	a0 ^= m_k[0] ^ (rc<<16);
	a1 ^= m_k[1];
	a2 ^= m_k[2] ^ rc;
	theta(a0, a1, a2);
	mu(a0, a1, a2);

	Block::Put(xorBlock, outBlock)(a0)(a1)(a2);
}

NAMESPACE_END
