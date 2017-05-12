// chacha.cpp - written and placed in the public domain by Jeffrey Walton.
//              Copyright assigned to the Crypto++ project.
//              Based on Wei Dai's Salsa20 and Bernstein's reference ChaCha
//              family implementation at http://cr.yp.to/chacha.html.

#include "pch.h"
#include "config.h"
#include "chacha.h"
#include "argnames.h"
#include "misc.h"
#include "cpu.h"

NAMESPACE_BEGIN(CryptoPP)

#define CHACHA_QUARTER_ROUND(a,b,c,d) \
    a += b; d ^= a; d = rotlFixed<word32>(d,16); \
    c += d; b ^= c; b = rotlFixed<word32>(b,12); \
    a += b; d ^= a; d = rotlFixed<word32>(d, 8); \
    c += d; b ^= c; b = rotlFixed<word32>(b, 7);

#if CRYPTOPP_DEBUG && !defined(CRYPTOPP_DOXYGEN_PROCESSING)
void ChaCha_TestInstantiations()
{
	 ChaCha8::Encryption x1;
	ChaCha12::Encryption x2;
	ChaCha20::Encryption x3;
}
#endif

template <unsigned int R>
void ChaCha_Policy<R>::CipherSetKey(const NameValuePairs &params, const byte *key, size_t length)
{
	CRYPTOPP_UNUSED(params);
	CRYPTOPP_ASSERT(length == 16 || length == 32);

	// "expand 16-byte k" or "expand 32-byte k"
	m_state[0] = 0x61707865;
	m_state[1] = (length == 16) ? 0x3120646e : 0x3320646e;
	m_state[2] = (length == 16) ? 0x79622d36 : 0x79622d32;
	m_state[3] = 0x6b206574;

	GetBlock<word32, LittleEndian> get1(key);
	get1(m_state[4])(m_state[5])(m_state[6])(m_state[7]);

	GetBlock<word32, LittleEndian> get2(key + ((length == 32) ? 16 : 0));
	get2(m_state[8])(m_state[9])(m_state[10])(m_state[11]);
}

template <unsigned int R>
void ChaCha_Policy<R>::CipherResynchronize(byte *keystreamBuffer, const byte *IV, size_t length)
{
	CRYPTOPP_UNUSED(keystreamBuffer), CRYPTOPP_UNUSED(length);
	CRYPTOPP_ASSERT(length==8);

	GetBlock<word32, LittleEndian> get(IV);
	m_state[12] = m_state[13] = 0;
	get(m_state[14])(m_state[15]);
}

template<unsigned int R>
void ChaCha_Policy<R>::SeekToIteration(lword iterationCount)
{
	CRYPTOPP_UNUSED(iterationCount);
	throw NotImplemented(std::string(ChaCha_Info<R>::StaticAlgorithmName()) + ":  SeekToIteration is not yet implemented");

	// TODO: these were Salsa20, and Wei re-arranged the state array for SSE2 operations.
	// If we can generate some out-of-band test vectors, then test and implement. Also
	//  see the test vectors in salsa.txt and the use of Seek test argument.
	// m_state[8] = (word32)iterationCount;
	// m_state[5] = (word32)SafeRightShift<32>(iterationCount);
}

template<unsigned int R>
unsigned int ChaCha_Policy<R>::GetAlignment() const
{
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE && 0
	if (HasSSE2())
		return 16;
	else
#endif
		return GetAlignmentOf<word32>();
}

template<unsigned int R>
unsigned int ChaCha_Policy<R>::GetOptimalBlockSize() const
{
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE && 0
	if (HasSSE2())
		return 4*BYTES_PER_ITERATION;
	else
#endif
		return BYTES_PER_ITERATION;
}

template<unsigned int R>
void ChaCha_Policy<R>::OperateKeystream(KeystreamOperation operation, byte *output, const byte *input, size_t iterationCount)
{
	word32 x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;

	while (iterationCount--)
	{
		x0 = m_state[0];	x1 = m_state[1];	x2 = m_state[2];	x3 = m_state[3];
		x4 = m_state[4];	x5 = m_state[5];	x6 = m_state[6];	x7 = m_state[7];
		x8 = m_state[8];	x9 = m_state[9];	x10 = m_state[10];	x11 = m_state[11];
		x12 = m_state[12];	x13 = m_state[13];	x14 = m_state[14];	x15 = m_state[15];

		for (int i = static_cast<int>(ROUNDS); i > 0; i -= 2)
		{
			CHACHA_QUARTER_ROUND(x0, x4,  x8, x12);
			CHACHA_QUARTER_ROUND(x1, x5,  x9, x13);
			CHACHA_QUARTER_ROUND(x2, x6, x10, x14);
			CHACHA_QUARTER_ROUND(x3, x7, x11, x15);

			CHACHA_QUARTER_ROUND(x0, x5, x10, x15);
			CHACHA_QUARTER_ROUND(x1, x6, x11, x12);
			CHACHA_QUARTER_ROUND(x2, x7,  x8, x13);
			CHACHA_QUARTER_ROUND(x3, x4,  x9, x14);
		}

		#undef CHACHA_OUTPUT
		#define CHACHA_OUTPUT(x){\
			CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 0, x0 + m_state[0]);\
			CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 1, x1 + m_state[1]);\
			CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 2, x2 + m_state[2]);\
			CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 3, x3 + m_state[3]);\
			CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 4, x4 + m_state[4]);\
			CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 5, x5 + m_state[5]);\
			CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 6, x6 + m_state[6]);\
			CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 7, x7 + m_state[7]);\
			CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 8, x8 + m_state[8]);\
			CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 9, x9 + m_state[9]);\
			CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 10, x10 + m_state[10]);\
			CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 11, x11 + m_state[11]);\
			CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 12, x12 + m_state[12]);\
			CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 13, x13 + m_state[13]);\
			CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 14, x14 + m_state[14]);\
			CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 15, x15 + m_state[15]);}

#ifndef CRYPTOPP_DOXYGEN_PROCESSING
		CRYPTOPP_KEYSTREAM_OUTPUT_SWITCH(CHACHA_OUTPUT, BYTES_PER_ITERATION);
#endif

		++m_state[12];
		m_state[13] += static_cast<word32>(m_state[12] == 0);
	}
}

template class ChaCha_Policy<8>;
template class ChaCha_Policy<12>;
template class ChaCha_Policy<20>;

NAMESPACE_END

