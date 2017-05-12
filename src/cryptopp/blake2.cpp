// blake2.cpp - written and placed in the public domain by Jeffrey Walton and Zooko
//              Wilcox-O'Hearn. Copyright assigned to the Crypto++ project.
//              Based on Aumasson, Neves, Wilcox-O'Hearn and Winnerlein's reference BLAKE2
//              implementation at http://github.com/BLAKE2/BLAKE2.

#include "pch.h"
#include "config.h"
#include "cryptlib.h"
#include "argnames.h"
#include "algparam.h"
#include "blake2.h"
#include "cpu.h"

NAMESPACE_BEGIN(CryptoPP)

// Uncomment for benchmarking C++ against SSE2 or NEON
// #undef CRYPTOPP_BOOL_SSE4_INTRINSICS_AVAILABLE
// #undef CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE

// Apple Clang 6.0/Clang 3.5 does not have SSSE3 intrinsics
//   http://llvm.org/bugs/show_bug.cgi?id=20213
#if (defined(CRYPTOPP_APPLE_CLANG_VERSION) && (CRYPTOPP_APPLE_CLANG_VERSION <= 60000)) || (defined(CRYPTOPP_LLVM_CLANG_VERSION) && (CRYPTOPP_LLVM_CLANG_VERSION <= 30500))
# undef CRYPTOPP_BOOL_SSE4_INTRINSICS_AVAILABLE
#endif

// Sun Studio 12.3 and earlier lack SSE2's _mm_set_epi64x. Win32 lacks _mm_set_epi64x (Win64 supplies it except for VS2008).
// Also see http://stackoverflow.com/a/38547909/608639
#if CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE && ((__SUNPRO_CC >= 0x5100 && __SUNPRO_CC < 0x5130) || (_MSC_VER >= 1200 && _MSC_VER < 1600) || (defined(_M_IX86) && _MSC_VER >= 1600))
inline __m128i _mm_set_epi64x(const word64 a, const word64 b)
{
	const word64 t[2] = {b,a}; __m128i r;
	memcpy(&r, &t, sizeof(r));
	return r;
}
#endif

// C/C++ implementation
static void BLAKE2_CXX_Compress32(const byte* input, BLAKE2_State<word32, false>& state);
static void BLAKE2_CXX_Compress64(const byte* input, BLAKE2_State<word64, true>& state);

// Also see http://github.com/weidai11/cryptopp/issues/247 for singling out SunCC 5.12
#if CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE
static void BLAKE2_SSE2_Compress32(const byte* input, BLAKE2_State<word32, false>& state);
# if (__SUNPRO_CC != 0x5120)
static void BLAKE2_SSE2_Compress64(const byte* input, BLAKE2_State<word64, true>& state);
# endif
#endif

#if CRYPTOPP_BOOL_SSE4_INTRINSICS_AVAILABLE
static void BLAKE2_SSE4_Compress32(const byte* input, BLAKE2_State<word32, false>& state);
static void BLAKE2_SSE4_Compress64(const byte* input, BLAKE2_State<word64, true>& state);
#endif

#if CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE
static void BLAKE2_NEON_Compress32(const byte* input, BLAKE2_State<word32, false>& state);
static void BLAKE2_NEON_Compress64(const byte* input, BLAKE2_State<word64, true>& state);
#endif

#ifndef CRYPTOPP_DOXYGEN_PROCESSING

// IV and Sigma are a better fit as part of BLAKE2_Base, but that places
//   the constants out of reach for the NEON, SSE2 and SSE4 implementations.
template<bool T_64bit>
struct CRYPTOPP_NO_VTABLE BLAKE2_IV {};

//! \brief BLAKE2s initialization vector specialization
template<>
struct CRYPTOPP_NO_VTABLE BLAKE2_IV<false>
{
	CRYPTOPP_CONSTANT(IVSIZE = 8)
	// Always align for NEON and SSE
	CRYPTOPP_ALIGN_DATA(16) static const word32 iv[8];
};

CRYPTOPP_ALIGN_DATA(16)
const word32 BLAKE2_IV<false>::iv[8] = {
	0x6A09E667UL, 0xBB67AE85UL, 0x3C6EF372UL, 0xA54FF53AUL,
	0x510E527FUL, 0x9B05688CUL, 0x1F83D9ABUL, 0x5BE0CD19UL
};

#define BLAKE2S_IV(n) BLAKE2_IV<false>::iv[n]

template<>
struct CRYPTOPP_NO_VTABLE BLAKE2_IV<true>
{
	CRYPTOPP_CONSTANT(IVSIZE = 8)
	// Always align for NEON and SSE
	CRYPTOPP_ALIGN_DATA(16) static const word64 iv[8];
};

CRYPTOPP_ALIGN_DATA(16)
const word64 BLAKE2_IV<true>::iv[8] = {
	W64LIT(0x6a09e667f3bcc908), W64LIT(0xbb67ae8584caa73b),
	W64LIT(0x3c6ef372fe94f82b), W64LIT(0xa54ff53a5f1d36f1),
	W64LIT(0x510e527fade682d1), W64LIT(0x9b05688c2b3e6c1f),
	W64LIT(0x1f83d9abfb41bd6b), W64LIT(0x5be0cd19137e2179)
};

#define BLAKE2B_IV(n) BLAKE2_IV<true>::iv[n]

// IV and Sigma are a better fit as part of BLAKE2_Base, but that places
//   the constants out of reach for the NEON, SSE2 and SSE4 implementations.
template<bool T_64bit>
struct CRYPTOPP_NO_VTABLE BLAKE2_Sigma {};

template<>
struct CRYPTOPP_NO_VTABLE BLAKE2_Sigma<false>
{
	// Always align for NEON and SSE
	CRYPTOPP_ALIGN_DATA(16) static const byte sigma[10][16];
};

CRYPTOPP_ALIGN_DATA(16)
const byte BLAKE2_Sigma<false>::sigma[10][16] = {
	{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
	{ 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 },
	{ 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 },
	{  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 },
	{  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 },
	{  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 },
	{ 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 },
	{ 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 },
	{  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 },
	{ 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13 , 0 },
};

//! \brief BLAKE2b sigma table specialization
template<>
struct CRYPTOPP_NO_VTABLE BLAKE2_Sigma<true>
{
	// Always align for NEON and SSE
	CRYPTOPP_ALIGN_DATA(16) static const byte sigma[12][16];
};

CRYPTOPP_ALIGN_DATA(16)
const byte BLAKE2_Sigma<true>::sigma[12][16] = {
	{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
	{ 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 },
	{ 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 },
	{  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 },
	{  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 },
	{  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 },
	{ 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 },
	{ 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 },
	{  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 },
	{ 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13 , 0 },
	{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
	{ 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 }
};

typedef void (*pfnCompress32)(const byte*, BLAKE2_State<word32, false>&);
typedef void (*pfnCompress64)(const byte*, BLAKE2_State<word64, true>&);

pfnCompress64 InitializeCompress64Fn()
{
#if CRYPTOPP_BOOL_SSE4_INTRINSICS_AVAILABLE
	if (HasSSE4())
		return &BLAKE2_SSE4_Compress64;
	else
#endif
#if CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE
# if (__SUNPRO_CC != 0x5120)
	if (HasSSE2())
		return &BLAKE2_SSE2_Compress64;
	else
# endif
#endif
#if CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE
	if (HasNEON())
		return &BLAKE2_NEON_Compress64;
	else
#endif
	return &BLAKE2_CXX_Compress64;
}

pfnCompress32 InitializeCompress32Fn()
{
#if CRYPTOPP_BOOL_SSE4_INTRINSICS_AVAILABLE
	if (HasSSE4())
		return &BLAKE2_SSE4_Compress32;
	else
#endif
#if CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE
	if (HasSSE2())
		return &BLAKE2_SSE2_Compress32;
	else
#endif
#if CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE
	if (HasNEON())
		return &BLAKE2_NEON_Compress32;
	else
#endif
	return &BLAKE2_CXX_Compress32;
}

#endif // CRYPTOPP_DOXYGEN_PROCESSING

BLAKE2_ParameterBlock<false>::BLAKE2_ParameterBlock(size_t digestLen, size_t keyLen,
		const byte* saltStr, size_t saltLen,
		const byte* personalizationStr, size_t personalizationLen)
{
	// Avoid Coverity finding SIZEOF_MISMATCH/suspicious_sizeof
	digestLength = (byte)digestLen;
	keyLength = (byte)keyLen;
	fanout = depth = 1;
	nodeDepth = innerLength = 0;

	memset(leafLength, 0x00, COUNTOF(leafLength));
	memset(nodeOffset, 0x00, COUNTOF(nodeOffset));

	if (saltStr && saltLen)
	{
		memcpy_s(salt, COUNTOF(salt), saltStr, saltLen);
		const size_t rem = COUNTOF(salt) - saltLen;
		const size_t off = COUNTOF(salt) - rem;
		if (rem)
			memset(salt+off, 0x00, rem);
	}
	else
	{
		memset(salt, 0x00, COUNTOF(salt));
	}

	if (personalizationStr && personalizationLen)
	{
		memcpy_s(personalization, COUNTOF(personalization), personalizationStr, personalizationLen);
		const size_t rem = COUNTOF(personalization) - personalizationLen;
		const size_t off = COUNTOF(personalization) - rem;
		if (rem)
			memset(personalization+off, 0x00, rem);
	}
	else
	{
		memset(personalization, 0x00, COUNTOF(personalization));
	}
}

BLAKE2_ParameterBlock<true>::BLAKE2_ParameterBlock(size_t digestLen, size_t keyLen,
		const byte* saltStr, size_t saltLen,
		const byte* personalizationStr, size_t personalizationLen)
{
	// Avoid Coverity finding SIZEOF_MISMATCH/suspicious_sizeof
	digestLength = (byte)digestLen;
	keyLength = (byte)keyLen;
	fanout = depth = 1;
	nodeDepth = innerLength = 0;

	memset(rfu, 0x00, COUNTOF(rfu));
	memset(leafLength, 0x00, COUNTOF(leafLength));
	memset(nodeOffset, 0x00, COUNTOF(nodeOffset));

	if (saltStr && saltLen)
	{
		memcpy_s(salt, COUNTOF(salt), saltStr, saltLen);
		const size_t rem = COUNTOF(salt) - saltLen;
		const size_t off = COUNTOF(salt) - rem;
		if (rem)
			memset(salt+off, 0x00, rem);
	}
	else
	{
		memset(salt, 0x00, COUNTOF(salt));
	}

	if (personalizationStr && personalizationLen)
	{
		memcpy_s(personalization, COUNTOF(personalization), personalizationStr, personalizationLen);
		const size_t rem = COUNTOF(personalization) - personalizationLen;
		const size_t off = COUNTOF(personalization) - rem;
		if (rem)
			memset(personalization+off, 0x00, rem);
	}
	else
	{
		memset(personalization, 0x00, COUNTOF(personalization));
	}
}

template <class W, bool T_64bit>
void BLAKE2_Base<W, T_64bit>::UncheckedSetKey(const byte *key, unsigned int length, const CryptoPP::NameValuePairs& params)
{
	if (key && length)
	{
		AlignedSecByteBlock temp(BLOCKSIZE);
		memcpy_s(temp, BLOCKSIZE, key, length);

		const size_t rem = BLOCKSIZE - length;
		if (rem)
			memset(temp+length, 0x00, rem);

		m_key.swap(temp);
	}
	else
	{
		m_key.resize(0);
	}

#if defined(__COVERITY__)
	// Avoid Coverity finding SIZEOF_MISMATCH/suspicious_sizeof
	ParameterBlock& block = *m_block.data();
	memset(m_block.data(), 0x00, sizeof(ParameterBlock));
#else
	// Set Head bytes; Tail bytes are set below
	ParameterBlock& block = *m_block.data();
	memset(m_block.data(), 0x00, T_64bit ? 32 : 16);
#endif

	block.keyLength = (byte)length;
	block.digestLength = (byte)params.GetIntValueWithDefault(Name::DigestSize(), DIGESTSIZE);
	block.fanout = block.depth = 1;

	ConstByteArrayParameter t;
	if (params.GetValue(Name::Salt(), t) && t.begin() && t.size())
	{
		memcpy_s(block.salt, COUNTOF(block.salt), t.begin(), t.size());
		const size_t rem = COUNTOF(block.salt) - t.size();
		const size_t off = COUNTOF(block.salt) - rem;
		if (rem)
			memset(block.salt+off, 0x00, rem);
	}
	else
	{
		memset(block.salt, 0x00, COUNTOF(block.salt));
	}

	if (params.GetValue(Name::Personalization(), t) && t.begin() && t.size())
	{
		memcpy_s(block.personalization, COUNTOF(block.personalization), t.begin(), t.size());
		const size_t rem = COUNTOF(block.personalization) - t.size();
		const size_t off = COUNTOF(block.personalization) - rem;
		if (rem)
			memset(block.personalization+off, 0x00, rem);
	}
	else
	{
		memset(block.personalization, 0x00, COUNTOF(block.personalization));
	}
}

template <class W, bool T_64bit>
BLAKE2_Base<W, T_64bit>::BLAKE2_Base() : m_state(1), m_block(1), m_digestSize(DIGESTSIZE), m_treeMode(false)
{
	UncheckedSetKey(NULL, 0, g_nullNameValuePairs);
	Restart();
}

template <class W, bool T_64bit>
BLAKE2_Base<W, T_64bit>::BLAKE2_Base(bool treeMode, unsigned int digestSize) : m_state(1), m_block(1), m_digestSize(digestSize), m_treeMode(treeMode)
{
	CRYPTOPP_ASSERT(digestSize <= DIGESTSIZE);

	UncheckedSetKey(NULL, 0, g_nullNameValuePairs);
	Restart();
}

template <class W, bool T_64bit>
BLAKE2_Base<W, T_64bit>::BLAKE2_Base(const byte *key, size_t keyLength, const byte* salt, size_t saltLength,
	const byte* personalization, size_t personalizationLength, bool treeMode, unsigned int digestSize)
	: m_state(1), m_block(1), m_digestSize(digestSize), m_treeMode(treeMode)
{
	CRYPTOPP_ASSERT(keyLength <= MAX_KEYLENGTH);
	CRYPTOPP_ASSERT(digestSize <= DIGESTSIZE);
	CRYPTOPP_ASSERT(saltLength <= SALTSIZE);
	CRYPTOPP_ASSERT(personalizationLength <= PERSONALIZATIONSIZE);

	UncheckedSetKey(key, static_cast<unsigned int>(keyLength), MakeParameters(Name::DigestSize(),(int)digestSize)(Name::TreeMode(),treeMode, false)
		(Name::Salt(), ConstByteArrayParameter(salt, saltLength))(Name::Personalization(), ConstByteArrayParameter(personalization, personalizationLength)));
	Restart();
}

template <class W, bool T_64bit>
void BLAKE2_Base<W, T_64bit>::Restart()
{
	static const W zero[2] = {0,0};
	Restart(*m_block.data(), zero);
}

template <class W, bool T_64bit>
void BLAKE2_Base<W, T_64bit>::Restart(const BLAKE2_ParameterBlock<T_64bit>& block, const W counter[2])
{
	// We take a parameter block as a parameter to allow customized state.
	// Avoid the copy of the parameter block when we are passing our own block.
	if (&block != m_block.data())
	{
		memcpy_s(m_block.data(), sizeof(ParameterBlock), &block, sizeof(ParameterBlock));
		m_block.data()->digestLength = (byte)m_digestSize;
		m_block.data()->keyLength = (byte)m_key.size();
	}

	State& state = *m_state.data();
	state.t[0] = state.t[1] = 0, state.f[0] = state.f[1] = 0, state.length = 0;

	if (counter != NULL)
	{
		state.t[0] = counter[0];
		state.t[1] = counter[1];
	}

	PutBlock<W, LittleEndian, true> put(m_block.data(), &state.h[0]);
	put(BLAKE2_IV<T_64bit>::iv[0])(BLAKE2_IV<T_64bit>::iv[1])(BLAKE2_IV<T_64bit>::iv[2])(BLAKE2_IV<T_64bit>::iv[3]);
	put(BLAKE2_IV<T_64bit>::iv[4])(BLAKE2_IV<T_64bit>::iv[5])(BLAKE2_IV<T_64bit>::iv[6])(BLAKE2_IV<T_64bit>::iv[7]);

	// When BLAKE2 is keyed, the input stream is simply {key||message}. Key it
	// during Restart to avoid FirstPut and friends. Key size == 0 means no key.
	if (m_key.size())
		Update(m_key, m_key.size());
}

template <class W, bool T_64bit>
void BLAKE2_Base<W, T_64bit>::Update(const byte *input, size_t length)
{
	State& state = *m_state.data();
	if (state.length + length > BLOCKSIZE)
	{
		// Complete current block
		const size_t fill = BLOCKSIZE - state.length;
		memcpy_s(&state.buffer[state.length], fill, input, fill);

		IncrementCounter();
		Compress(state.buffer);
		state.length = 0;

		length -= fill, input += fill;

		// Compress in-place to avoid copies
		while (length > BLOCKSIZE)
		{
			IncrementCounter();
			Compress(input);
			length -= BLOCKSIZE, input += BLOCKSIZE;
		}
	}

	// Copy tail bytes
	if (input && length)
	{
		CRYPTOPP_ASSERT(length <= BLOCKSIZE - state.length);
		memcpy_s(&state.buffer[state.length], length, input, length);
		state.length += static_cast<unsigned int>(length);
	}
}

template <class W, bool T_64bit>
void BLAKE2_Base<W, T_64bit>::TruncatedFinal(byte *hash, size_t size)
{
	this->ThrowIfInvalidTruncatedSize(size);

	// Set last block unconditionally
	State& state = *m_state.data();
	state.f[0] = static_cast<W>(-1);

	// Set last node if tree mode
	if (m_treeMode)
		state.f[1] = static_cast<W>(-1);

	// Increment counter for tail bytes only
	IncrementCounter(state.length);

	memset(state.buffer + state.length, 0x00, BLOCKSIZE - state.length);
	Compress(state.buffer);

	// Copy to caller buffer
	memcpy_s(hash, size, &state.h[0], size);

	Restart();
}

template <class W, bool T_64bit>
void BLAKE2_Base<W, T_64bit>::IncrementCounter(size_t count)
{
	State& state = *m_state.data();
	state.t[0] += static_cast<W>(count);
	state.t[1] += !!(state.t[0] < count);
}

template <>
void BLAKE2_Base<word64, true>::Compress(const byte *input)
{
	// Selects the most advanced implmentation at runtime
	static const pfnCompress64 s_pfn = InitializeCompress64Fn();
	s_pfn(input, *m_state.data());
}

template <>
void BLAKE2_Base<word32, false>::Compress(const byte *input)
{
	// Selects the most advanced implmentation at runtime
	static const pfnCompress32 s_pfn = InitializeCompress32Fn();
	s_pfn(input, *m_state.data());
}

void BLAKE2_CXX_Compress64(const byte* input, BLAKE2_State<word64, true>& state)
{
	#undef BLAKE2_G
	#undef BLAKE2_ROUND

	#define BLAKE2_G(r,i,a,b,c,d) \
	  do { \
	    a = a + b + m[BLAKE2_Sigma<true>::sigma[r][2*i+0]]; \
	    d = rotrVariable<word64>(d ^ a, 32); \
	    c = c + d; \
	    b = rotrVariable<word64>(b ^ c, 24); \
	    a = a + b + m[BLAKE2_Sigma<true>::sigma[r][2*i+1]]; \
	    d = rotrVariable<word64>(d ^ a, 16); \
	    c = c + d; \
	    b = rotrVariable<word64>(b ^ c, 63); \
	  } while(0)

	#define BLAKE2_ROUND(r)  \
	  do { \
	    BLAKE2_G(r,0,v[ 0],v[ 4],v[ 8],v[12]); \
	    BLAKE2_G(r,1,v[ 1],v[ 5],v[ 9],v[13]); \
	    BLAKE2_G(r,2,v[ 2],v[ 6],v[10],v[14]); \
	    BLAKE2_G(r,3,v[ 3],v[ 7],v[11],v[15]); \
	    BLAKE2_G(r,4,v[ 0],v[ 5],v[10],v[15]); \
	    BLAKE2_G(r,5,v[ 1],v[ 6],v[11],v[12]); \
	    BLAKE2_G(r,6,v[ 2],v[ 7],v[ 8],v[13]); \
	    BLAKE2_G(r,7,v[ 3],v[ 4],v[ 9],v[14]); \
	  } while(0)

	word64 m[16], v[16];

	GetBlock<word64, LittleEndian, true> get1(input);
	get1(m[0])(m[1])(m[2])(m[3])(m[4])(m[5])(m[6])(m[7])(m[8])(m[9])(m[10])(m[11])(m[12])(m[13])(m[14])(m[15]);

	GetBlock<word64, LittleEndian, true> get2(&state.h[0]);
	get2(v[0])(v[1])(v[2])(v[3])(v[4])(v[5])(v[6])(v[7]);

	v[ 8] = BLAKE2B_IV(0);
	v[ 9] = BLAKE2B_IV(1);
	v[10] = BLAKE2B_IV(2);
	v[11] = BLAKE2B_IV(3);
	v[12] = state.t[0] ^ BLAKE2B_IV(4);
	v[13] = state.t[1] ^ BLAKE2B_IV(5);
	v[14] = state.f[0] ^ BLAKE2B_IV(6);
	v[15] = state.f[1] ^ BLAKE2B_IV(7);

	BLAKE2_ROUND( 0 );
	BLAKE2_ROUND( 1 );
	BLAKE2_ROUND( 2 );
	BLAKE2_ROUND( 3 );
	BLAKE2_ROUND( 4 );
	BLAKE2_ROUND( 5 );
	BLAKE2_ROUND( 6 );
	BLAKE2_ROUND( 7 );
	BLAKE2_ROUND( 8 );
	BLAKE2_ROUND( 9 );
	BLAKE2_ROUND( 10 );
	BLAKE2_ROUND( 11 );

	for(unsigned int i = 0; i < 8; ++i)
		state.h[i] = state.h[i] ^ ConditionalByteReverse(LittleEndian::ToEnum(), v[i] ^ v[i + 8]);
}

void BLAKE2_CXX_Compress32(const byte* input, BLAKE2_State<word32, false>& state)
{
	#undef BLAKE2_G
	#undef BLAKE2_ROUND

	#define BLAKE2_G(r,i,a,b,c,d) \
	  do { \
	    a = a + b + m[BLAKE2_Sigma<false>::sigma[r][2*i+0]]; \
	    d = rotrVariable<word32>(d ^ a, 16); \
	    c = c + d; \
	    b = rotrVariable<word32>(b ^ c, 12); \
	    a = a + b + m[BLAKE2_Sigma<false>::sigma[r][2*i+1]]; \
	    d = rotrVariable<word32>(d ^ a, 8); \
	    c = c + d; \
	    b = rotrVariable<word32>(b ^ c, 7); \
	  } while(0)

	#define BLAKE2_ROUND(r)  \
	  do { \
	    BLAKE2_G(r,0,v[ 0],v[ 4],v[ 8],v[12]); \
	    BLAKE2_G(r,1,v[ 1],v[ 5],v[ 9],v[13]); \
	    BLAKE2_G(r,2,v[ 2],v[ 6],v[10],v[14]); \
	    BLAKE2_G(r,3,v[ 3],v[ 7],v[11],v[15]); \
	    BLAKE2_G(r,4,v[ 0],v[ 5],v[10],v[15]); \
	    BLAKE2_G(r,5,v[ 1],v[ 6],v[11],v[12]); \
	    BLAKE2_G(r,6,v[ 2],v[ 7],v[ 8],v[13]); \
	    BLAKE2_G(r,7,v[ 3],v[ 4],v[ 9],v[14]); \
	  } while(0)

	word32 m[16], v[16];

	GetBlock<word32, LittleEndian, true> get1(input);
	get1(m[0])(m[1])(m[2])(m[3])(m[4])(m[5])(m[6])(m[7])(m[8])(m[9])(m[10])(m[11])(m[12])(m[13])(m[14])(m[15]);

	GetBlock<word32, LittleEndian, true> get2(&state.h[0]);
	get2(v[0])(v[1])(v[2])(v[3])(v[4])(v[5])(v[6])(v[7]);

	v[ 8] = BLAKE2S_IV(0);
	v[ 9] = BLAKE2S_IV(1);
	v[10] = BLAKE2S_IV(2);
	v[11] = BLAKE2S_IV(3);
	v[12] = state.t[0] ^ BLAKE2S_IV(4);
	v[13] = state.t[1] ^ BLAKE2S_IV(5);
	v[14] = state.f[0] ^ BLAKE2S_IV(6);
	v[15] = state.f[1] ^ BLAKE2S_IV(7);

	BLAKE2_ROUND( 0 );
	BLAKE2_ROUND( 1 );
	BLAKE2_ROUND( 2 );
	BLAKE2_ROUND( 3 );
	BLAKE2_ROUND( 4 );
	BLAKE2_ROUND( 5 );
	BLAKE2_ROUND( 6 );
	BLAKE2_ROUND( 7 );
	BLAKE2_ROUND( 8 );
	BLAKE2_ROUND( 9 );

	for(unsigned int i = 0; i < 8; ++i)
		state.h[i] = state.h[i] ^ ConditionalByteReverse(LittleEndian::ToEnum(), v[i] ^ v[i + 8]);
}

#if CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE
static void BLAKE2_SSE2_Compress32(const byte* input, BLAKE2_State<word32, false>& state)
{
  word32 m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15;
  GetBlock<word32, LittleEndian, true> get(input);
  get(m0)(m1)(m2)(m3)(m4)(m5)(m6)(m7)(m8)(m9)(m10)(m11)(m12)(m13)(m14)(m15);

  __m128i row1,row2,row3,row4;
  __m128i buf1,buf2,buf3,buf4;
  __m128i ff0,ff1;

  row1 = ff0 = _mm_loadu_si128((const __m128i*)(const void*)(&state.h[0]));
  row2 = ff1 = _mm_loadu_si128((const __m128i*)(const void*)(&state.h[4]));
  row3 = _mm_setr_epi32(BLAKE2S_IV(0),BLAKE2S_IV(1),BLAKE2S_IV(2),BLAKE2S_IV(3));
  row4 = _mm_xor_si128(_mm_setr_epi32(BLAKE2S_IV(4),BLAKE2S_IV(5),BLAKE2S_IV(6),BLAKE2S_IV(7)),_mm_loadu_si128((const __m128i*)(const void*)(&state.t[0])));
  buf1 = _mm_set_epi32(m6,m4,m2,m0);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf1),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf2 = _mm_set_epi32(m7,m5,m3,m1);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf2),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(0,3,2,1));

  buf3 = _mm_set_epi32(m14,m12,m10,m8);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf3),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf4 = _mm_set_epi32(m15,m13,m11,m9);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf4),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(2,1,0,3));

  buf1 = _mm_set_epi32(m13,m9,m4,m14);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf1),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf2 = _mm_set_epi32(m6,m15,m8,m10);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf2),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(0,3,2,1));

  buf3 = _mm_set_epi32(m5,m11,m0,m1);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf3),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf4 = _mm_set_epi32(m3,m7,m2,m12);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf4),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(2,1,0,3));

  buf1 = _mm_set_epi32(m15,m5,m12,m11);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf1),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf2 = _mm_set_epi32(m13,m2,m0,m8);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf2),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(0,3,2,1));

  buf3 = _mm_set_epi32(m9,m7,m3,m10);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf3),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf4 = _mm_set_epi32(m4,m1,m6,m14);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf4),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(2,1,0,3));

  buf1 = _mm_set_epi32(m11,m13,m3,m7);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf1),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf2 = _mm_set_epi32(m14,m12,m1,m9);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf2),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(0,3,2,1));

  buf3 = _mm_set_epi32(m15,m4,m5,m2);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf3),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf4 = _mm_set_epi32(m8,m0,m10,m6);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf4),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(2,1,0,3));

  buf1 = _mm_set_epi32(m10,m2,m5,m9);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf1),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf2 = _mm_set_epi32(m15,m4,m7,m0);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf2),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(0,3,2,1));

  buf3 = _mm_set_epi32(m3,m6,m11,m14);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf3),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf4 = _mm_set_epi32(m13,m8,m12,m1);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf4),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(2,1,0,3));

  buf1 = _mm_set_epi32(m8,m0,m6,m2);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf1),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf2 = _mm_set_epi32(m3,m11,m10,m12);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf2),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(0,3,2,1));

  buf3 = _mm_set_epi32(m1,m15,m7,m4);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf3),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf4 = _mm_set_epi32(m9,m14,m5,m13);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf4),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(2,1,0,3));

  buf1 = _mm_set_epi32(m4,m14,m1,m12);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf1),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf2 = _mm_set_epi32(m10,m13,m15,m5);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf2),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(0,3,2,1));

  buf3 = _mm_set_epi32(m8,m9,m6,m0);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf3),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf4 = _mm_set_epi32(m11,m2,m3,m7);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf4),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(2,1,0,3));

  buf1 = _mm_set_epi32(m3,m12,m7,m13);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf1),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf2 = _mm_set_epi32(m9,m1,m14,m11);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf2),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(0,3,2,1));

  buf3 = _mm_set_epi32(m2,m8,m15,m5);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf3),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf4 = _mm_set_epi32(m10,m6,m4,m0);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf4),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(2,1,0,3));

  buf1 = _mm_set_epi32(m0,m11,m14,m6);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf1),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf2 = _mm_set_epi32(m8,m3,m9,m15);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf2),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(0,3,2,1));

  buf3 = _mm_set_epi32(m10,m1,m13,m12);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf3),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf4 = _mm_set_epi32(m5,m4,m7,m2);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf4),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(2,1,0,3));

  buf1 = _mm_set_epi32(m1,m7,m8,m10);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf1),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf2 = _mm_set_epi32(m5,m6,m4,m2);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf2),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(0,3,2,1));

  buf3 = _mm_set_epi32(m13,m3,m9,m15);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf3),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,16),_mm_slli_epi32(row4,16));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,12),_mm_slli_epi32(row2,20));

  buf4 = _mm_set_epi32(m0,m12,m14,m11);
  row1 = _mm_add_epi32(_mm_add_epi32(row1,buf4),row2);
  row4 = _mm_xor_si128(row4,row1);
  row4 = _mm_xor_si128(_mm_srli_epi32(row4,8),_mm_slli_epi32(row4,24));
  row3 = _mm_add_epi32(row3,row4);
  row2 = _mm_xor_si128(row2,row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2,7),_mm_slli_epi32(row2,25));

  row4 = _mm_shuffle_epi32(row4,_MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3,_MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2,_MM_SHUFFLE(2,1,0,3));

  _mm_storeu_si128((__m128i *)(void*)(&state.h[0]),_mm_xor_si128(ff0,_mm_xor_si128(row1,row3)));
  _mm_storeu_si128((__m128i *)(void*)(&state.h[4]),_mm_xor_si128(ff1,_mm_xor_si128(row2,row4)));
}

# if (__SUNPRO_CC != 0x5120)
static void BLAKE2_SSE2_Compress64(const byte* input, BLAKE2_State<word64, true>& state)
{
  word64 m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15;
  GetBlock<word64, LittleEndian, true> get(input);
  get(m0)(m1)(m2)(m3)(m4)(m5)(m6)(m7)(m8)(m9)(m10)(m11)(m12)(m13)(m14)(m15);

  __m128i row1l, row1h, row2l, row2h;
  __m128i row3l, row3h, row4l, row4h;
  __m128i b0, b1, t0, t1;

  row1l = _mm_loadu_si128((const __m128i*)(const void*)(&state.h[0]));
  row1h = _mm_loadu_si128((const __m128i*)(const void*)(&state.h[2]));
  row2l = _mm_loadu_si128((const __m128i*)(const void*)(&state.h[4]));
  row2h = _mm_loadu_si128((const __m128i*)(const void*)(&state.h[6]));
  row3l = _mm_loadu_si128((const __m128i*)(const void*)(&BLAKE2B_IV(0)));
  row3h = _mm_loadu_si128((const __m128i*)(const void*)(&BLAKE2B_IV(2)));
  row4l = _mm_xor_si128(_mm_loadu_si128((const __m128i*)(const void*)(&BLAKE2B_IV(4))), _mm_loadu_si128((const __m128i*)(const void*)(&state.t[0])));
  row4h = _mm_xor_si128(_mm_loadu_si128((const __m128i*)(const void*)(&BLAKE2B_IV(6))), _mm_loadu_si128((const __m128i*)(const void*)(&state.f[0])));

  b0 = _mm_set_epi64x(m2, m0);
  b1 = _mm_set_epi64x(m6, m4);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l, 40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h, 40));

  b0 = _mm_set_epi64x(m3, m1);
  b1 = _mm_set_epi64x(m7, m5);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t0, t0));
  row4h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row4h, row4h));
  row2l = _mm_unpackhi_epi64(row2l, _mm_unpacklo_epi64(row2h, row2h));
  row2h = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(t1, t1));

  b0 = _mm_set_epi64x(m10, m8);
  b1 = _mm_set_epi64x(m14, m12);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m11, m9);
  b1 = _mm_set_epi64x(m15, m13);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(row2l, row2l));
  row2h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row2h, row2h));
  row4l = _mm_unpackhi_epi64(row4l, _mm_unpacklo_epi64(row4h, row4h));
  row4h = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t1, t1));

  b0 = _mm_set_epi64x(m4, m14);
  b1 = _mm_set_epi64x(m13, m9);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m8, m10);
  b1 = _mm_set_epi64x(m6, m15);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t0, t0));
  row4h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row4h, row4h));
  row2l = _mm_unpackhi_epi64(row2l, _mm_unpacklo_epi64(row2h, row2h));
  row2h = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(t1, t1));
  b0 = _mm_set_epi64x(m0, m1);
  b1 = _mm_set_epi64x(m5, m11);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m2, m12);
  b1 = _mm_set_epi64x(m3, m7);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(row2l, row2l));
  row2h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row2h, row2h));
  row4l = _mm_unpackhi_epi64(row4l, _mm_unpacklo_epi64(row4h, row4h));
  row4h = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t1, t1));

  b0 = _mm_set_epi64x(m12, m11);
  b1 = _mm_set_epi64x(m15, m5);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m0, m8);
  b1 = _mm_set_epi64x(m13, m2);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t0, t0));
  row4h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row4h, row4h));
  row2l = _mm_unpackhi_epi64(row2l, _mm_unpacklo_epi64(row2h, row2h));
  row2h = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(t1, t1));
  b0 = _mm_set_epi64x(m3, m10);
  b1 = _mm_set_epi64x(m9, m7);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m6, m14);
  b1 = _mm_set_epi64x(m4, m1);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(row2l, row2l));
  row2h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row2h, row2h));
  row4l = _mm_unpackhi_epi64(row4l, _mm_unpacklo_epi64(row4h, row4h));
  row4h = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t1, t1));

  b0 = _mm_set_epi64x(m3, m7);
  b1 = _mm_set_epi64x(m11, m13);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m1, m9);
  b1 = _mm_set_epi64x(m14, m12);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t0, t0));
  row4h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row4h, row4h));
  row2l = _mm_unpackhi_epi64(row2l, _mm_unpacklo_epi64(row2h, row2h));
  row2h = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(t1, t1));
  b0 = _mm_set_epi64x(m5, m2);
  b1 = _mm_set_epi64x(m15, m4);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m10, m6);
  b1 = _mm_set_epi64x(m8, m0);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(row2l, row2l));
  row2h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row2h, row2h));
  row4l = _mm_unpackhi_epi64(row4l, _mm_unpacklo_epi64(row4h, row4h));
  row4h = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t1, t1));

  b0 = _mm_set_epi64x(m5, m9);
  b1 = _mm_set_epi64x(m10, m2);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m7, m0);
  b1 = _mm_set_epi64x(m15, m4);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t0, t0));
  row4h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row4h, row4h));
  row2l = _mm_unpackhi_epi64(row2l, _mm_unpacklo_epi64(row2h, row2h));
  row2h = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(t1, t1));
  b0 = _mm_set_epi64x(m11, m14);
  b1 = _mm_set_epi64x(m3, m6);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));


  b0 = _mm_set_epi64x(m12, m1);
  b1 = _mm_set_epi64x(m13, m8);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(row2l, row2l));
  row2h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row2h, row2h));
  row4l = _mm_unpackhi_epi64(row4l, _mm_unpacklo_epi64(row4h, row4h));
  row4h = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t1, t1));

  b0 = _mm_set_epi64x(m6, m2);
  b1 = _mm_set_epi64x(m8, m0);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m10, m12);
  b1 = _mm_set_epi64x(m3, m11);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t0, t0));
  row4h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row4h, row4h));
  row2l = _mm_unpackhi_epi64(row2l, _mm_unpacklo_epi64(row2h, row2h));
  row2h = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(t1, t1));
  b0 = _mm_set_epi64x(m7, m4);
  b1 = _mm_set_epi64x(m1, m15);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m5, m13);
  b1 = _mm_set_epi64x(m9, m14);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(row2l, row2l));
  row2h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row2h, row2h));
  row4l = _mm_unpackhi_epi64(row4l, _mm_unpacklo_epi64(row4h, row4h));
  row4h = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t1, t1));

  b0 = _mm_set_epi64x(m1, m12);
  b1 = _mm_set_epi64x(m4, m14);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m15, m5);
  b1 = _mm_set_epi64x(m10, m13);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t0, t0));
  row4h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row4h, row4h));
  row2l = _mm_unpackhi_epi64(row2l, _mm_unpacklo_epi64(row2h, row2h));
  row2h = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(t1, t1));
  b0 = _mm_set_epi64x(m6, m0);
  b1 = _mm_set_epi64x(m8, m9);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m3, m7);
  b1 = _mm_set_epi64x(m11, m2);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(row2l, row2l));
  row2h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row2h, row2h));
  row4l = _mm_unpackhi_epi64(row4l, _mm_unpacklo_epi64(row4h, row4h));
  row4h = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t1, t1));

  b0 = _mm_set_epi64x(m7, m13);
  b1 = _mm_set_epi64x(m3, m12);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m14, m11);
  b1 = _mm_set_epi64x(m9, m1);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t0, t0));
  row4h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row4h, row4h));
  row2l = _mm_unpackhi_epi64(row2l, _mm_unpacklo_epi64(row2h, row2h));
  row2h = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(t1, t1));
  b0 = _mm_set_epi64x(m15, m5);
  b1 = _mm_set_epi64x(m2, m8);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m4, m0);
  b1 = _mm_set_epi64x(m10, m6);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(row2l, row2l));
  row2h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row2h, row2h));
  row4l = _mm_unpackhi_epi64(row4l, _mm_unpacklo_epi64(row4h, row4h));
  row4h = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t1, t1));

  b0 = _mm_set_epi64x(m14, m6);
  b1 = _mm_set_epi64x(m0, m11);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m9, m15);
  b1 = _mm_set_epi64x(m8, m3);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t0, t0));
  row4h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row4h, row4h));
  row2l = _mm_unpackhi_epi64(row2l, _mm_unpacklo_epi64(row2h, row2h));
  row2h = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(t1, t1));
  b0 = _mm_set_epi64x(m13, m12);
  b1 = _mm_set_epi64x(m10, m1);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m7, m2);
  b1 = _mm_set_epi64x(m5, m4);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(row2l, row2l));
  row2h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row2h, row2h));
  row4l = _mm_unpackhi_epi64(row4l, _mm_unpacklo_epi64(row4h, row4h));
  row4h = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t1, t1));

  b0 = _mm_set_epi64x(m8, m10);
  b1 = _mm_set_epi64x(m1, m7);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m4, m2);
  b1 = _mm_set_epi64x(m5, m6);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t0, t0));
  row4h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row4h, row4h));
  row2l = _mm_unpackhi_epi64(row2l, _mm_unpacklo_epi64(row2h, row2h));
  row2h = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(t1, t1));
  b0 = _mm_set_epi64x(m9, m15);
  b1 = _mm_set_epi64x(m13, m3);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m14, m11);
  b1 = _mm_set_epi64x(m0, m12);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(row2l, row2l));
  row2h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row2h, row2h));
  row4l = _mm_unpackhi_epi64(row4l, _mm_unpacklo_epi64(row4h, row4h));
  row4h = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t1, t1));

  b0 = _mm_set_epi64x(m2, m0);
  b1 = _mm_set_epi64x(m6, m4);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m3, m1);
  b1 = _mm_set_epi64x(m7, m5);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t0, t0));
  row4h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row4h, row4h));
  row2l = _mm_unpackhi_epi64(row2l, _mm_unpacklo_epi64(row2h, row2h));
  row2h = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(t1, t1));

  b0 = _mm_set_epi64x(m10, m8);
  b1 = _mm_set_epi64x(m14, m12);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m11, m9);
  b1 = _mm_set_epi64x(m15, m13);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(row2l, row2l));
  row2h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row2h, row2h));
  row4l = _mm_unpackhi_epi64(row4l, _mm_unpacklo_epi64(row4h, row4h));
  row4h = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t1, t1));

  b0 = _mm_set_epi64x(m4, m14);
  b1 = _mm_set_epi64x(m13, m9);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m8, m10);
  b1 = _mm_set_epi64x(m6, m15);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t0, t0));
  row4h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row4h, row4h));
  row2l = _mm_unpackhi_epi64(row2l, _mm_unpacklo_epi64(row2h, row2h));
  row2h = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(t1, t1));
  b0 = _mm_set_epi64x(m0, m1);
  b1 = _mm_set_epi64x(m5, m11);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,32),_mm_slli_epi64(row4l,32));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,32),_mm_slli_epi64(row4h,32));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,24),_mm_slli_epi64(row2l,40));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,24),_mm_slli_epi64(row2h,40));

  b0 = _mm_set_epi64x(m2, m12);
  b1 = _mm_set_epi64x(m3, m7);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_xor_si128(_mm_srli_epi64(row4l,16),_mm_slli_epi64(row4l,48));
  row4h = _mm_xor_si128(_mm_srli_epi64(row4h,16),_mm_slli_epi64(row4h,48));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l,63),_mm_slli_epi64(row2l,1));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h,63),_mm_slli_epi64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(row2l, row2l));
  row2h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row2h, row2h));
  row4l = _mm_unpackhi_epi64(row4l, _mm_unpacklo_epi64(row4h, row4h));
  row4h = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t1, t1));

  row1l = _mm_xor_si128(row3l, row1l);
  row1h = _mm_xor_si128(row3h, row1h);
  _mm_storeu_si128((__m128i *)(void*)(&state.h[0]), _mm_xor_si128(_mm_loadu_si128((const __m128i*)(const void*)(&state.h[0])), row1l));
  _mm_storeu_si128((__m128i *)(void*)(&state.h[2]), _mm_xor_si128(_mm_loadu_si128((const __m128i*)(const void*)(&state.h[2])), row1h));

  row2l = _mm_xor_si128(row4l, row2l);
  row2h = _mm_xor_si128(row4h, row2h);
  _mm_storeu_si128((__m128i *)(void*)(&state.h[4]), _mm_xor_si128(_mm_loadu_si128((const __m128i*)(const void*)(&state.h[4])), row2l));
  _mm_storeu_si128((__m128i *)(void*)(&state.h[6]), _mm_xor_si128(_mm_loadu_si128((const __m128i*)(const void*)(&state.h[6])), row2h));
}
# endif // (__SUNPRO_CC != 0x5120)
#endif  // CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE

#if CRYPTOPP_BOOL_SSE4_INTRINSICS_AVAILABLE
static void BLAKE2_SSE4_Compress32(const byte* input, BLAKE2_State<word32, false>& state)
{
  __m128i row1, row2, row3, row4;
  __m128i buf1, buf2, buf3, buf4;

  __m128i t0, t1, t2;
  __m128i ff0, ff1;

  const __m128i r8 = _mm_set_epi8(12, 15, 14, 13, 8, 11, 10, 9, 4, 7, 6, 5, 0, 3, 2, 1);
  const __m128i r16 = _mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);

  const __m128i m0 = _mm_loadu_si128((const __m128i*)(const void*)(input + 00));
  const __m128i m1 = _mm_loadu_si128((const __m128i*)(const void*)(input + 16));
  const __m128i m2 = _mm_loadu_si128((const __m128i*)(const void*)(input + 32));
  const __m128i m3 = _mm_loadu_si128((const __m128i*)(const void*)(input + 48));

  row1 = ff0 = _mm_loadu_si128((const __m128i*)(const void*)(&state.h[0]));
  row2 = ff1 = _mm_loadu_si128((const __m128i*)(const void*)(&state.h[4]));
  row3 = _mm_setr_epi32(BLAKE2S_IV(0), BLAKE2S_IV(1), BLAKE2S_IV(2), BLAKE2S_IV(3));
  row4 = _mm_xor_si128(_mm_setr_epi32(BLAKE2S_IV(4), BLAKE2S_IV(5), BLAKE2S_IV(6), BLAKE2S_IV(7)), _mm_loadu_si128((const __m128i*)(const void*)(&state.t[0])));
  buf1 = _mm_castps_si128((_mm_shuffle_ps(_mm_castsi128_ps((m0)), _mm_castsi128_ps((m1)), _MM_SHUFFLE(2,0,2,0))));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf1), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  buf2 = _mm_castps_si128((_mm_shuffle_ps(_mm_castsi128_ps((m0)), _mm_castsi128_ps((m1)), _MM_SHUFFLE(3,1,3,1))));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf2), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(0,3,2,1));

  buf3 = _mm_castps_si128((_mm_shuffle_ps(_mm_castsi128_ps((m2)), _mm_castsi128_ps((m3)), _MM_SHUFFLE(2,0,2,0))));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf3), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  buf4 = _mm_castps_si128((_mm_shuffle_ps(_mm_castsi128_ps((m2)), _mm_castsi128_ps((m3)), _MM_SHUFFLE(3,1,3,1))));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf4), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(2,1,0,3));

  t0 = _mm_blend_epi16(m1, m2, 0x0C);
  t1 = _mm_slli_si128(m3, 4);
  t2 = _mm_blend_epi16(t0, t1, 0xF0);
  buf1 = _mm_shuffle_epi32(t2, _MM_SHUFFLE(2,1,0,3));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf1), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_shuffle_epi32(m2,_MM_SHUFFLE(0,0,2,0));
  t1 = _mm_blend_epi16(m1,m3,0xC0);
  t2 = _mm_blend_epi16(t0, t1, 0xF0);
  buf2 = _mm_shuffle_epi32(t2, _MM_SHUFFLE(2,3,0,1));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf2), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(0,3,2,1));

  t0 = _mm_slli_si128(m1, 4);
  t1 = _mm_blend_epi16(m2, t0, 0x30);
  t2 = _mm_blend_epi16(m0, t1, 0xF0);
  buf3 = _mm_shuffle_epi32(t2, _MM_SHUFFLE(2,3,0,1));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf3), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_unpackhi_epi32(m0,m1);
  t1 = _mm_slli_si128(m3, 4);
  t2 = _mm_blend_epi16(t0, t1, 0x0C);
  buf4 = _mm_shuffle_epi32(t2, _MM_SHUFFLE(2,3,0,1));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf4), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(2,1,0,3));

  t0 = _mm_unpackhi_epi32(m2,m3);
  t1 = _mm_blend_epi16(m3,m1,0x0C);
  t2 = _mm_blend_epi16(t0, t1, 0x0F);
  buf1 = _mm_shuffle_epi32(t2, _MM_SHUFFLE(3,1,0,2));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf1), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_unpacklo_epi32(m2,m0);
  t1 = _mm_blend_epi16(t0, m0, 0xF0);
  t2 = _mm_slli_si128(m3, 8);
  buf2 = _mm_blend_epi16(t1, t2, 0xC0);

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf2), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(0,3,2,1));

  t0 = _mm_blend_epi16(m0, m2, 0x3C);
  t1 = _mm_srli_si128(m1, 12);
  t2 = _mm_blend_epi16(t0,t1,0x03);
  buf3 = _mm_shuffle_epi32(t2, _MM_SHUFFLE(1,0,3,2));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf3), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_slli_si128(m3, 4);
  t1 = _mm_blend_epi16(m0, m1, 0x33);
  t2 = _mm_blend_epi16(t1, t0, 0xC0);
  buf4 = _mm_shuffle_epi32(t2, _MM_SHUFFLE(0,1,2,3));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf4), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(2,1,0,3));

  t0 = _mm_unpackhi_epi32(m0,m1);
  t1 = _mm_unpackhi_epi32(t0, m2);
  t2 = _mm_blend_epi16(t1, m3, 0x0C);
  buf1 = _mm_shuffle_epi32(t2, _MM_SHUFFLE(3,1,0,2));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf1), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_slli_si128(m2, 8);
  t1 = _mm_blend_epi16(m3,m0,0x0C);
  t2 = _mm_blend_epi16(t1, t0, 0xC0);
  buf2 = _mm_shuffle_epi32(t2, _MM_SHUFFLE(2,0,1,3));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf2), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(0,3,2,1));

  t0 = _mm_blend_epi16(m0,m1,0x0F);
  t1 = _mm_blend_epi16(t0, m3, 0xC0);
  buf3 = _mm_shuffle_epi32(t1, _MM_SHUFFLE(3,0,1,2));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf3), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_unpacklo_epi32(m0,m2);
  t1 = _mm_unpackhi_epi32(m1,m2);
  buf4 = _mm_unpacklo_epi64(t1,t0);

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf4), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(2,1,0,3));

  t0 = _mm_unpacklo_epi64(m1,m2);
  t1 = _mm_unpackhi_epi64(m0,m2);
  t2 = _mm_blend_epi16(t0,t1,0x33);
  buf1 = _mm_shuffle_epi32(t2, _MM_SHUFFLE(2,0,1,3));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf1), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_unpackhi_epi64(m1,m3);
  t1 = _mm_unpacklo_epi64(m0,m1);
  buf2 = _mm_blend_epi16(t0,t1,0x33);

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf2), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(0,3,2,1));

  t0 = _mm_unpackhi_epi64(m3,m1);
  t1 = _mm_unpackhi_epi64(m2,m0);
  buf3 = _mm_blend_epi16(t1,t0,0x33);

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf3), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_blend_epi16(m0,m2,0x03);
  t1 = _mm_slli_si128(t0, 8);
  t2 = _mm_blend_epi16(t1,m3,0x0F);
  buf4 = _mm_shuffle_epi32(t2, _MM_SHUFFLE(1,2,0,3));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf4), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(2,1,0,3));

  t0 = _mm_unpackhi_epi32(m0,m1);
  t1 = _mm_unpacklo_epi32(m0,m2);
  buf1 = _mm_unpacklo_epi64(t0,t1);

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf1), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_srli_si128(m2, 4);
  t1 = _mm_blend_epi16(m0,m3,0x03);
  buf2 = _mm_blend_epi16(t1,t0,0x3C);

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf2), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(0,3,2,1));

  t0 = _mm_blend_epi16(m1,m0,0x0C);
  t1 = _mm_srli_si128(m3, 4);
  t2 = _mm_blend_epi16(t0,t1,0x30);
  buf3 = _mm_shuffle_epi32(t2, _MM_SHUFFLE(1,2,3,0));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf3), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_unpacklo_epi64(m1,m2);
  t1= _mm_shuffle_epi32(m3, _MM_SHUFFLE(0,2,0,1));
  buf4 = _mm_blend_epi16(t0,t1,0x33);

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf4), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(2,1,0,3));

  t0 = _mm_slli_si128(m1, 12);
  t1 = _mm_blend_epi16(m0,m3,0x33);
  buf1 = _mm_blend_epi16(t1,t0,0xC0);

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf1), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_blend_epi16(m3,m2,0x30);
  t1 = _mm_srli_si128(m1, 4);
  t2 = _mm_blend_epi16(t0,t1,0x03);
  buf2 = _mm_shuffle_epi32(t2, _MM_SHUFFLE(2,1,3,0));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf2), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(0,3,2,1));

  t0 = _mm_unpacklo_epi64(m0,m2);
  t1 = _mm_srli_si128(m1, 4);
  buf3 = _mm_shuffle_epi32(_mm_blend_epi16(t0,t1,0x0C), _MM_SHUFFLE(2,3,1,0));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf3), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_unpackhi_epi32(m1,m2);
  t1 = _mm_unpackhi_epi64(m0,t0);
  buf4 = _mm_shuffle_epi32(t1, _MM_SHUFFLE(3,0,1,2));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf4), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(2,1,0,3));

  t0 = _mm_unpackhi_epi32(m0,m1);
  t1 = _mm_blend_epi16(t0,m3,0x0F);
  buf1 = _mm_shuffle_epi32(t1,_MM_SHUFFLE(2,0,3,1));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf1), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_blend_epi16(m2,m3,0x30);
  t1 = _mm_srli_si128(m0,4);
  t2 = _mm_blend_epi16(t0,t1,0x03);
  buf2 = _mm_shuffle_epi32(t2, _MM_SHUFFLE(1,0,2,3));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf2), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(0,3,2,1));

  t0 = _mm_unpackhi_epi64(m0,m3);
  t1 = _mm_unpacklo_epi64(m1,m2);
  t2 = _mm_blend_epi16(t0,t1,0x3C);
  buf3 = _mm_shuffle_epi32(t2,_MM_SHUFFLE(0,2,3,1));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf3), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_unpacklo_epi32(m0,m1);
  t1 = _mm_unpackhi_epi32(m1,m2);
  buf4 = _mm_unpacklo_epi64(t0,t1);

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf4), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(2,1,0,3));

  t0 = _mm_unpackhi_epi32(m1,m3);
  t1 = _mm_unpacklo_epi64(t0,m0);
  t2 = _mm_blend_epi16(t1,m2,0xC0);
  buf1 = _mm_shufflehi_epi16(t2,_MM_SHUFFLE(1,0,3,2));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf1), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_unpackhi_epi32(m0,m3);
  t1 = _mm_blend_epi16(m2,t0,0xF0);
  buf2 = _mm_shuffle_epi32(t1,_MM_SHUFFLE(0,2,1,3));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf2), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(0,3,2,1));

  t0 = _mm_blend_epi16(m2,m0,0x0C);
  t1 = _mm_slli_si128(t0,4);
  buf3 = _mm_blend_epi16(t1,m3,0x0F);

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf3), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_blend_epi16(m1,m0,0x30);
  buf4 = _mm_shuffle_epi32(t0,_MM_SHUFFLE(1,0,3,2));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf4), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(2,1,0,3));

  t0 = _mm_blend_epi16(m0,m2,0x03);
  t1 = _mm_blend_epi16(m1,m2,0x30);
  t2 = _mm_blend_epi16(t1,t0,0x0F);
  buf1 = _mm_shuffle_epi32(t2,_MM_SHUFFLE(1,3,0,2));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf1), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_slli_si128(m0,4);
  t1 = _mm_blend_epi16(m1,t0,0xC0);
  buf2 = _mm_shuffle_epi32(t1,_MM_SHUFFLE(1,2,0,3));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf2), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(2,1,0,3));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(0,3,2,1));

  t0 = _mm_unpackhi_epi32(m0,m3);
  t1 = _mm_unpacklo_epi32(m2,m3);
  t2 = _mm_unpackhi_epi64(t0,t1);
  buf3 = _mm_shuffle_epi32(t2,_MM_SHUFFLE(3,0,2,1));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf3), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r16);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 12),_mm_slli_epi32(row2, 20));

  t0 = _mm_blend_epi16(m3,m2,0xC0);
  t1 = _mm_unpacklo_epi32(m0,m3);
  t2 = _mm_blend_epi16(t0,t1,0x0F);
  buf4 = _mm_shuffle_epi32(t2,_MM_SHUFFLE(0,1,2,3));

  row1 = _mm_add_epi32(_mm_add_epi32(row1, buf4), row2);
  row4 = _mm_xor_si128(row4, row1);
  row4 = _mm_shuffle_epi8(row4,r8);
  row3 = _mm_add_epi32(row3, row4);
  row2 = _mm_xor_si128(row2, row3);
  row2 = _mm_xor_si128(_mm_srli_epi32(row2, 7),_mm_slli_epi32(row2, 25));

  row4 = _mm_shuffle_epi32(row4, _MM_SHUFFLE(0,3,2,1));
  row3 = _mm_shuffle_epi32(row3, _MM_SHUFFLE(1,0,3,2));
  row2 = _mm_shuffle_epi32(row2, _MM_SHUFFLE(2,1,0,3));

  _mm_storeu_si128((__m128i *)(void*)(&state.h[0]), _mm_xor_si128(ff0, _mm_xor_si128(row1, row3)));
  _mm_storeu_si128((__m128i *)(void*)(&state.h[4]), _mm_xor_si128(ff1, _mm_xor_si128(row2, row4)));
}

static void BLAKE2_SSE4_Compress64(const byte* input, BLAKE2_State<word64, true>& state)
{
  __m128i row1l, row1h;
  __m128i row2l, row2h;
  __m128i row3l, row3h;
  __m128i row4l, row4h;
  __m128i b0, b1, t0, t1;

  const __m128i r16 = _mm_setr_epi8(2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9);
  const __m128i r24 = _mm_setr_epi8(3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10);

  const __m128i m0 = _mm_loadu_si128((const __m128i*)(const void*)(input + 00));
  const __m128i m1 = _mm_loadu_si128((const __m128i*)(const void*)(input + 16));
  const __m128i m2 = _mm_loadu_si128((const __m128i*)(const void*)(input + 32));
  const __m128i m3 = _mm_loadu_si128((const __m128i*)(const void*)(input + 48));
  const __m128i m4 = _mm_loadu_si128((const __m128i*)(const void*)(input + 64));
  const __m128i m5 = _mm_loadu_si128((const __m128i*)(const void*)(input + 80));
  const __m128i m6 = _mm_loadu_si128((const __m128i*)(const void*)(input + 96));
  const __m128i m7 = _mm_loadu_si128((const __m128i*)(const void*)(input + 112));

  row1l = _mm_loadu_si128((const __m128i*)(const void*)(&state.h[0]));
  row1h = _mm_loadu_si128((const __m128i*)(const void*)(&state.h[2]));
  row2l = _mm_loadu_si128((const __m128i*)(const void*)(&state.h[4]));
  row2h = _mm_loadu_si128((const __m128i*)(const void*)(&state.h[6]));
  row3l = _mm_loadu_si128((const __m128i*)(const void*)(&BLAKE2B_IV(0)));
  row3h = _mm_loadu_si128((const __m128i*)(const void*)(&BLAKE2B_IV(2)));
  row4l = _mm_xor_si128(_mm_loadu_si128((const __m128i*)(const void*)(&BLAKE2B_IV(4))), _mm_loadu_si128((const __m128i*)(const void*)(&state.t[0])));
  row4h = _mm_xor_si128(_mm_loadu_si128((const __m128i*)(const void*)(&BLAKE2B_IV(6))), _mm_loadu_si128((const __m128i*)(const void*)(&state.f[0])));

  b0 = _mm_unpacklo_epi64(m0, m1);
  b1 = _mm_unpacklo_epi64(m2, m3);
  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpackhi_epi64(m0, m1);
  b1 = _mm_unpackhi_epi64(m2, m3);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2h, row2l, 8);
  t1 = _mm_alignr_epi8(row2l, row2h, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4h, row4l, 8);
  t1 = _mm_alignr_epi8(row4l, row4h, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_unpacklo_epi64(m4, m5);
  b1 = _mm_unpacklo_epi64(m6, m7);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpackhi_epi64(m4, m5);
  b1 = _mm_unpackhi_epi64(m6, m7);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2l, row2h, 8);
  t1 = _mm_alignr_epi8(row2h, row2l, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4l, row4h, 8);
  t1 = _mm_alignr_epi8(row4h, row4l, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_unpacklo_epi64(m7, m2);
  b1 = _mm_unpackhi_epi64(m4, m6);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpacklo_epi64(m5, m4);
  b1 = _mm_alignr_epi8(m3, m7, 8);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2h, row2l, 8);
  t1 = _mm_alignr_epi8(row2l, row2h, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4h, row4l, 8);
  t1 = _mm_alignr_epi8(row4l, row4h, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_shuffle_epi32(m0, _MM_SHUFFLE(1,0,3,2));
  b1 = _mm_unpackhi_epi64(m5, m2);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpacklo_epi64(m6, m1);
  b1 = _mm_unpackhi_epi64(m3, m1);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2l, row2h, 8);
  t1 = _mm_alignr_epi8(row2h, row2l, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4l, row4h, 8);
  t1 = _mm_alignr_epi8(row4h, row4l, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_alignr_epi8(m6, m5, 8);
  b1 = _mm_unpackhi_epi64(m2, m7);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpacklo_epi64(m4, m0);
  b1 = _mm_blend_epi16(m1, m6, 0xF0);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2h, row2l, 8);
  t1 = _mm_alignr_epi8(row2l, row2h, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4h, row4l, 8);
  t1 = _mm_alignr_epi8(row4l, row4h, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_blend_epi16(m5, m1, 0xF0);
  b1 = _mm_unpackhi_epi64(m3, m4);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpacklo_epi64(m7, m3);
  b1 = _mm_alignr_epi8(m2, m0, 8);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2l, row2h, 8);
  t1 = _mm_alignr_epi8(row2h, row2l, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4l, row4h, 8);
  t1 = _mm_alignr_epi8(row4h, row4l, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_unpackhi_epi64(m3, m1);
  b1 = _mm_unpackhi_epi64(m6, m5);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpackhi_epi64(m4, m0);
  b1 = _mm_unpacklo_epi64(m6, m7);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2h, row2l, 8);
  t1 = _mm_alignr_epi8(row2l, row2h, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4h, row4l, 8);
  t1 = _mm_alignr_epi8(row4l, row4h, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_blend_epi16(m1, m2, 0xF0);
  b1 = _mm_blend_epi16(m2, m7, 0xF0);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpacklo_epi64(m3, m5);
  b1 = _mm_unpacklo_epi64(m0, m4);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2l, row2h, 8);
  t1 = _mm_alignr_epi8(row2h, row2l, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4l, row4h, 8);
  t1 = _mm_alignr_epi8(row4h, row4l, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_unpackhi_epi64(m4, m2);
  b1 = _mm_unpacklo_epi64(m1, m5);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_blend_epi16(m0, m3, 0xF0);
  b1 = _mm_blend_epi16(m2, m7, 0xF0);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2h, row2l, 8);
  t1 = _mm_alignr_epi8(row2l, row2h, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4h, row4l, 8);
  t1 = _mm_alignr_epi8(row4l, row4h, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_blend_epi16(m7, m5, 0xF0);
  b1 = _mm_blend_epi16(m3, m1, 0xF0);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_alignr_epi8(m6, m0, 8);
  b1 = _mm_blend_epi16(m4, m6, 0xF0);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2l, row2h, 8);
  t1 = _mm_alignr_epi8(row2h, row2l, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4l, row4h, 8);
  t1 = _mm_alignr_epi8(row4h, row4l, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_unpacklo_epi64(m1, m3);
  b1 = _mm_unpacklo_epi64(m0, m4);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpacklo_epi64(m6, m5);
  b1 = _mm_unpackhi_epi64(m5, m1);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2h, row2l, 8);
  t1 = _mm_alignr_epi8(row2l, row2h, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4h, row4l, 8);
  t1 = _mm_alignr_epi8(row4l, row4h, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_blend_epi16(m2, m3, 0xF0);
  b1 = _mm_unpackhi_epi64(m7, m0);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpackhi_epi64(m6, m2);
  b1 = _mm_blend_epi16(m7, m4, 0xF0);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2l, row2h, 8);
  t1 = _mm_alignr_epi8(row2h, row2l, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4l, row4h, 8);
  t1 = _mm_alignr_epi8(row4h, row4l, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_blend_epi16(m6, m0, 0xF0);
  b1 = _mm_unpacklo_epi64(m7, m2);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpackhi_epi64(m2, m7);
  b1 = _mm_alignr_epi8(m5, m6, 8);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2h, row2l, 8);
  t1 = _mm_alignr_epi8(row2l, row2h, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4h, row4l, 8);
  t1 = _mm_alignr_epi8(row4l, row4h, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_unpacklo_epi64(m0, m3);
  b1 = _mm_shuffle_epi32(m4, _MM_SHUFFLE(1,0,3,2));

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpackhi_epi64(m3, m1);
  b1 = _mm_blend_epi16(m1, m5, 0xF0);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2l, row2h, 8);
  t1 = _mm_alignr_epi8(row2h, row2l, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4l, row4h, 8);
  t1 = _mm_alignr_epi8(row4h, row4l, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_unpackhi_epi64(m6, m3);
  b1 = _mm_blend_epi16(m6, m1, 0xF0);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_alignr_epi8(m7, m5, 8);
  b1 = _mm_unpackhi_epi64(m0, m4);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2h, row2l, 8);
  t1 = _mm_alignr_epi8(row2l, row2h, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4h, row4l, 8);
  t1 = _mm_alignr_epi8(row4l, row4h, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_unpackhi_epi64(m2, m7);
  b1 = _mm_unpacklo_epi64(m4, m1);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpacklo_epi64(m0, m2);
  b1 = _mm_unpacklo_epi64(m3, m5);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2l, row2h, 8);
  t1 = _mm_alignr_epi8(row2h, row2l, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4l, row4h, 8);
  t1 = _mm_alignr_epi8(row4h, row4l, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_unpacklo_epi64(m3, m7);
  b1 = _mm_alignr_epi8(m0, m5, 8);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpackhi_epi64(m7, m4);
  b1 = _mm_alignr_epi8(m4, m1, 8);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2h, row2l, 8);
  t1 = _mm_alignr_epi8(row2l, row2h, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4h, row4l, 8);
  t1 = _mm_alignr_epi8(row4l, row4h, 8);
  row4l = t1, row4h = t0;

  b0 = m6;
  b1 = _mm_alignr_epi8(m5, m0, 8);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_blend_epi16(m1, m3, 0xF0);
  b1 = m2;

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2l, row2h, 8);
  t1 = _mm_alignr_epi8(row2h, row2l, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4l, row4h, 8);
  t1 = _mm_alignr_epi8(row4h, row4l, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_unpacklo_epi64(m5, m4);
  b1 = _mm_unpackhi_epi64(m3, m0);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpacklo_epi64(m1, m2);
  b1 = _mm_blend_epi16(m3, m2, 0xF0);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2h, row2l, 8);
  t1 = _mm_alignr_epi8(row2l, row2h, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4h, row4l, 8);
  t1 = _mm_alignr_epi8(row4l, row4h, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_unpackhi_epi64(m7, m4);
  b1 = _mm_unpackhi_epi64(m1, m6);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_alignr_epi8(m7, m5, 8);
  b1 = _mm_unpacklo_epi64(m6, m0);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2l, row2h, 8);
  t1 = _mm_alignr_epi8(row2h, row2l, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4l, row4h, 8);
  t1 = _mm_alignr_epi8(row4h, row4l, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_unpacklo_epi64(m0, m1);
  b1 = _mm_unpacklo_epi64(m2, m3);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpackhi_epi64(m0, m1);
  b1 = _mm_unpackhi_epi64(m2, m3);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2h, row2l, 8);
  t1 = _mm_alignr_epi8(row2l, row2h, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4h, row4l, 8);
  t1 = _mm_alignr_epi8(row4l, row4h, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_unpacklo_epi64(m4, m5);
  b1 = _mm_unpacklo_epi64(m6, m7);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpackhi_epi64(m4, m5);
  b1 = _mm_unpackhi_epi64(m6, m7);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2l, row2h, 8);
  t1 = _mm_alignr_epi8(row2h, row2l, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4l, row4h, 8);
  t1 = _mm_alignr_epi8(row4h, row4l, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_unpacklo_epi64(m7, m2);
  b1 = _mm_unpackhi_epi64(m4, m6);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpacklo_epi64(m5, m4);
  b1 = _mm_alignr_epi8(m3, m7, 8);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2h, row2l, 8);
  t1 = _mm_alignr_epi8(row2l, row2h, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4h, row4l, 8);
  t1 = _mm_alignr_epi8(row4l, row4h, 8);
  row4l = t1, row4h = t0;

  b0 = _mm_shuffle_epi32(m0, _MM_SHUFFLE(1,0,3,2));
  b1 = _mm_unpackhi_epi64(m5, m2);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi32(row4l, _MM_SHUFFLE(2,3,0,1));
  row4h = _mm_shuffle_epi32(row4h, _MM_SHUFFLE(2,3,0,1));
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_shuffle_epi8(row2l, r24);
  row2h = _mm_shuffle_epi8(row2h, r24);

  b0 = _mm_unpacklo_epi64(m6, m1);
  b1 = _mm_unpackhi_epi64(m3, m1);

  row1l = _mm_add_epi64(_mm_add_epi64(row1l, b0), row2l);
  row1h = _mm_add_epi64(_mm_add_epi64(row1h, b1), row2h);
  row4l = _mm_xor_si128(row4l, row1l);
  row4h = _mm_xor_si128(row4h, row1h);
  row4l = _mm_shuffle_epi8(row4l, r16);
  row4h = _mm_shuffle_epi8(row4h, r16);
  row3l = _mm_add_epi64(row3l, row4l);
  row3h = _mm_add_epi64(row3h, row4h);
  row2l = _mm_xor_si128(row2l, row3l);
  row2h = _mm_xor_si128(row2h, row3h);
  row2l = _mm_xor_si128(_mm_srli_epi64(row2l, 63), _mm_add_epi64(row2l, row2l));
  row2h = _mm_xor_si128(_mm_srli_epi64(row2h, 63), _mm_add_epi64(row2h, row2h));

  t0 = _mm_alignr_epi8(row2l, row2h, 8);
  t1 = _mm_alignr_epi8(row2h, row2l, 8);
  row2l = t0, row2h = t1, t0 = row3l, row3l = row3h, row3h = t0;
  t0 = _mm_alignr_epi8(row4l, row4h, 8);
  t1 = _mm_alignr_epi8(row4h, row4l, 8);
  row4l = t1, row4h = t0;

  row1l = _mm_xor_si128(row3l, row1l);
  row1h = _mm_xor_si128(row3h, row1h);
  _mm_storeu_si128((__m128i *)(void*)(&state.h[0]), _mm_xor_si128(_mm_loadu_si128((const __m128i*)(const void*)(&state.h[0])), row1l));
  _mm_storeu_si128((__m128i *)(void*)(&state.h[2]), _mm_xor_si128(_mm_loadu_si128((const __m128i*)(const void*)(&state.h[2])), row1h));

  row2l = _mm_xor_si128(row4l, row2l);
  row2h = _mm_xor_si128(row4h, row2h);
  _mm_storeu_si128((__m128i *)(void*)(&state.h[4]), _mm_xor_si128(_mm_loadu_si128((const __m128i*)(const void*)(&state.h[4])), row2l));
  _mm_storeu_si128((__m128i *)(void*)(&state.h[6]), _mm_xor_si128(_mm_loadu_si128((const __m128i*)(const void*)(&state.h[6])), row2h));
}
#endif  // CRYPTOPP_BOOL_SSE4_INTRINSICS_AVAILABLE

#if CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE

// Reverse words for ARM (use arguments to _mm_set_epi32 without reversing them).
#define vld1q_u32_rev(x, a,b,c,d) d[1]=c[0],d[2]=b[0],d[3]=a[0]; x = vld1q_u32(d);

// Keep things straight due to swapping. For a 128-bit vector, H64 denotes
//   the high 64-bit vector, and L64 denotes the low 64-bit vector. The
//   vectors are the same as returned by vget_high_u64 and vget_low_u64.
static const int LANE_H64 = 1;
static const int LANE_L64 = 0;

static void BLAKE2_NEON_Compress32(const byte* input, BLAKE2_State<word32, false>& state)
{
  //CRYPTOPP_ASSERT(IsAlignedOn(input,GetAlignmentOf<uint8_t*>()));
  CRYPTOPP_ASSERT(IsAlignedOn(&state.h[0],GetAlignmentOf<uint32x4_t>()));
  CRYPTOPP_ASSERT(IsAlignedOn(&state.h[4],GetAlignmentOf<uint32x4_t>()));
  CRYPTOPP_ASSERT(IsAlignedOn(&state.t[0],GetAlignmentOf<uint32x4_t>()));

  CRYPTOPP_ALIGN_DATA(16) uint32_t m0[4], m1[4], m2[4], m3[4], m4[4], m5[4], m6[4], m7[4];
  CRYPTOPP_ALIGN_DATA(16) uint32_t m8[4], m9[4], m10[4], m11[4], m12[4], m13[4], m14[4], m15[4];

  GetBlock<word32, LittleEndian, true> get(input);
  get(m0[0])(m1[0])(m2[0])(m3[0])(m4[0])(m5[0])(m6[0])(m7[0])(m8[0])(m9[0])(m10[0])(m11[0])(m12[0])(m13[0])(m14[0])(m15[0]);

  uint32x4_t row1,row2,row3,row4;
  uint32x4_t buf1,buf2,buf3,buf4;
  uint32x4_t ff0,ff1;

  row1 = ff0 = vld1q_u32((const uint32_t*)&state.h[0]);
  row2 = ff1 = vld1q_u32((const uint32_t*)&state.h[4]);
  row3 = vld1q_u32((const uint32_t*)&BLAKE2S_IV(0));
  row4 = veorq_u32(vld1q_u32((const uint32_t*)&BLAKE2S_IV(4)), vld1q_u32((const uint32_t*)&state.t[0]));

  // buf1 = vld1q_u32(m6,m4,m2,m0);
  vld1q_u32_rev(buf1, m6,m4,m2,m0);

  row1 = vaddq_u32(vaddq_u32(row1,buf1),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf2 = vld1q_u32(m7,m5,m3,m1);
  vld1q_u32_rev(buf2, m7,m5,m3,m1);

  row1 = vaddq_u32(vaddq_u32(row1,buf2),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,3);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,1);

  // buf3 = vld1q_u32(m14,m12,m10,m8);
  vld1q_u32_rev(buf3, m14,m12,m10,m8);

  row1 = vaddq_u32(vaddq_u32(row1,buf3),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf4 = vld1q_u32(m15,m13,m11,m9);
  vld1q_u32_rev(buf4, m15,m13,m11,m9);

  row1 = vaddq_u32(vaddq_u32(row1,buf4),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,1);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,3);

  // buf1 = vld1q_u32(m13,m9,m4,m14);
  vld1q_u32_rev(buf1, m13,m9,m4,m14);

  row1 = vaddq_u32(vaddq_u32(row1,buf1),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf2 = vld1q_u32(m6,m15,m8,m10);
  vld1q_u32_rev(buf2, m6,m15,m8,m10);

  row1 = vaddq_u32(vaddq_u32(row1,buf2),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,3);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,1);

  // buf3 = vld1q_u32(m5,m11,m0,m1);
  vld1q_u32_rev(buf3, m5,m11,m0,m1);

  row1 = vaddq_u32(vaddq_u32(row1,buf3),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf4 = vld1q_u32(m3,m7,m2,m12);
  vld1q_u32_rev(buf4, m3,m7,m2,m12);

  row1 = vaddq_u32(vaddq_u32(row1,buf4),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,1);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,3);

  // buf1 = vld1q_u32(m15,m5,m12,m11);
  vld1q_u32_rev(buf1, m15,m5,m12,m11);

  row1 = vaddq_u32(vaddq_u32(row1,buf1),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf2 = vld1q_u32(m13,m2,m0,m8);
  vld1q_u32_rev(buf2, m13,m2,m0,m8);

  row1 = vaddq_u32(vaddq_u32(row1,buf2),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,3);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,1);

  // buf3 = vld1q_u32(m9,m7,m3,m10);
  vld1q_u32_rev(buf3, m9,m7,m3,m10);

  row1 = vaddq_u32(vaddq_u32(row1,buf3),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf4 = vld1q_u32(m4,m1,m6,m14);
  vld1q_u32_rev(buf4, m4,m1,m6,m14);

  row1 = vaddq_u32(vaddq_u32(row1,buf4),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,1);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,3);

  // buf1 = vld1q_u32(m11,m13,m3,m7);
  vld1q_u32_rev(buf1, m11,m13,m3,m7);

  row1 = vaddq_u32(vaddq_u32(row1,buf1),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf2 = vld1q_u32(m14,m12,m1,m9);
  vld1q_u32_rev(buf2, m14,m12,m1,m9);

  row1 = vaddq_u32(vaddq_u32(row1,buf2),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,3);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,1);

  // buf3 = vld1q_u32(m15,m4,m5,m2);
  vld1q_u32_rev(buf3, m15,m4,m5,m2);

  row1 = vaddq_u32(vaddq_u32(row1,buf3),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf4 = vld1q_u32(m8,m0,m10,m6);
  vld1q_u32_rev(buf4, m8,m0,m10,m6);

  row1 = vaddq_u32(vaddq_u32(row1,buf4),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,1);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,3);

  // buf1 = vld1q_u32(m10,m2,m5,m9);
  vld1q_u32_rev(buf1, m10,m2,m5,m9);

  row1 = vaddq_u32(vaddq_u32(row1,buf1),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf2 = vld1q_u32(m15,m4,m7,m0);
  vld1q_u32_rev(buf2, m15,m4,m7,m0);

  row1 = vaddq_u32(vaddq_u32(row1,buf2),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,3);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,1);

  // buf3 = vld1q_u32(m3,m6,m11,m14);
  vld1q_u32_rev(buf3, m3,m6,m11,m14);

  row1 = vaddq_u32(vaddq_u32(row1,buf3),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf4 = vld1q_u32(m13,m8,m12,m1);
  vld1q_u32_rev(buf4, m13,m8,m12,m1);

  row1 = vaddq_u32(vaddq_u32(row1,buf4),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,1);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,3);

  // buf1 = vld1q_u32(m8,m0,m6,m2);
  vld1q_u32_rev(buf1, m8,m0,m6,m2);

  row1 = vaddq_u32(vaddq_u32(row1,buf1),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf2 = vld1q_u32(m3,m11,m10,m12);
  vld1q_u32_rev(buf2, m3,m11,m10,m12);

  row1 = vaddq_u32(vaddq_u32(row1,buf2),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,3);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,1);

  // buf3 = vld1q_u32(m1,m15,m7,m4);
  vld1q_u32_rev(buf3, m1,m15,m7,m4);

  row1 = vaddq_u32(vaddq_u32(row1,buf3),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf4 = vld1q_u32(m9,m14,m5,m13);
  vld1q_u32_rev(buf4, m9,m14,m5,m13);

  row1 = vaddq_u32(vaddq_u32(row1,buf4),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,1);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,3);

  // buf1 = vld1q_u32(m4,m14,m1,m12);
  vld1q_u32_rev(buf1, m4,m14,m1,m12);

  row1 = vaddq_u32(vaddq_u32(row1,buf1),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf2 = vld1q_u32(m10,m13,m15,m5);
  vld1q_u32_rev(buf2, m10,m13,m15,m5);

  row1 = vaddq_u32(vaddq_u32(row1,buf2),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,3);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,1);

  // buf3 = vld1q_u32(m8,m9,m6,m0);
  vld1q_u32_rev(buf3, m8,m9,m6,m0);

  row1 = vaddq_u32(vaddq_u32(row1,buf3),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf4 = vld1q_u32(m11,m2,m3,m7);
  vld1q_u32_rev(buf4, m11,m2,m3,m7);

  row1 = vaddq_u32(vaddq_u32(row1,buf4),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,1);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,3);

  // buf1 = vld1q_u32(m3,m12,m7,m13);
  vld1q_u32_rev(buf1, m3,m12,m7,m13);

  row1 = vaddq_u32(vaddq_u32(row1,buf1),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf2 = vld1q_u32(m9,m1,m14,m11);
  vld1q_u32_rev(buf2, m9,m1,m14,m11);

  row1 = vaddq_u32(vaddq_u32(row1,buf2),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,3);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,1);

  // buf3 = vld1q_u32(m2,m8,m15,m5);
  vld1q_u32_rev(buf3, m2,m8,m15,m5);

  row1 = vaddq_u32(vaddq_u32(row1,buf3),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf4 = vld1q_u32(m10,m6,m4,m0);
  vld1q_u32_rev(buf4, m10,m6,m4,m0);

  row1 = vaddq_u32(vaddq_u32(row1,buf4),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,1);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,3);

  // buf1 = vld1q_u32(m0,m11,m14,m6);
  vld1q_u32_rev(buf1, m0,m11,m14,m6);

  row1 = vaddq_u32(vaddq_u32(row1,buf1),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf2 = vld1q_u32(m8,m3,m9,m15);
  vld1q_u32_rev(buf2, m8,m3,m9,m15);

  row1 = vaddq_u32(vaddq_u32(row1,buf2),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,3);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,1);

  // buf3 = vld1q_u32(m10,m1,m13,m12);
  vld1q_u32_rev(buf3, m10,m1,m13,m12);

  row1 = vaddq_u32(vaddq_u32(row1,buf3),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf4 = vld1q_u32(m5,m4,m7,m2);
  vld1q_u32_rev(buf4, m5,m4,m7,m2);

  row1 = vaddq_u32(vaddq_u32(row1,buf4),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,1);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,3);

  // buf1 = vld1q_u32(m1,m7,m8,m10);
  vld1q_u32_rev(buf1, m1,m7,m8,m10);

  row1 = vaddq_u32(vaddq_u32(row1,buf1),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf2 = vld1q_u32(m5,m6,m4,m2);
  vld1q_u32_rev(buf2, m5,m6,m4,m2);

  row1 = vaddq_u32(vaddq_u32(row1,buf2),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,3);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,1);

  // buf3 = vld1q_u32(m13,m3,m9,m15);
  vld1q_u32_rev(buf3, m13,m3,m9,m15);

  row1 = vaddq_u32(vaddq_u32(row1,buf3),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,16),vshlq_n_u32(row4,16));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,12),vshlq_n_u32(row2,20));

  // buf4 = vld1q_u32(m0,m12,m14,m11);
  vld1q_u32_rev(buf4, m0,m12,m14,m11);

  row1 = vaddq_u32(vaddq_u32(row1,buf4),row2);
  row4 = veorq_u32(row4,row1);
  row4 = veorq_u32(vshrq_n_u32(row4,8),vshlq_n_u32(row4,24));
  row3 = vaddq_u32(row3,row4);
  row2 = veorq_u32(row2,row3);
  row2 = veorq_u32(vshrq_n_u32(row2,7),vshlq_n_u32(row2,25));

  row4 = vextq_u32(row4,row4,1);
  row3 = vcombine_u32(vget_high_u32(row3),vget_low_u32(row3));
  row2 = vextq_u32(row2,row2,3);

  vst1q_u32((uint32_t*)&state.h[0],veorq_u32(ff0,veorq_u32(row1,row3)));
  vst1q_u32((uint32_t*)&state.h[4],veorq_u32(ff1,veorq_u32(row2,row4)));
}

static void BLAKE2_NEON_Compress64(const byte* input, BLAKE2_State<word64, true>& state)
{
  //CRYPTOPP_ASSERT(IsAlignedOn(input,GetAlignmentOf<uint8_t*>()));
  CRYPTOPP_ASSERT(IsAlignedOn(&state.h[0],GetAlignmentOf<uint64x2_t>()));
  CRYPTOPP_ASSERT(IsAlignedOn(&state.h[4],GetAlignmentOf<uint64x2_t>()));
  CRYPTOPP_ASSERT(IsAlignedOn(&state.t[0],GetAlignmentOf<uint64x2_t>()));

  uint64x2_t m0m1,m2m3,m4m5,m6m7,m8m9,m10m11,m12m13,m14m15;

    m0m1 = vreinterpretq_u64_u8(vld1q_u8(input+  0));
    m2m3 = vreinterpretq_u64_u8(vld1q_u8(input+ 16));
    m4m5 = vreinterpretq_u64_u8(vld1q_u8(input+ 32));
    m6m7 = vreinterpretq_u64_u8(vld1q_u8(input+ 48));
    m8m9 = vreinterpretq_u64_u8(vld1q_u8(input+ 64));
  m10m11 = vreinterpretq_u64_u8(vld1q_u8(input+ 80));
  m12m13 = vreinterpretq_u64_u8(vld1q_u8(input+ 96));
  m14m15 = vreinterpretq_u64_u8(vld1q_u8(input+112));

  uint64x2_t row1l, row1h, row2l, row2h;
  uint64x2_t row3l, row3h, row4l, row4h;
  uint64x2_t b0 = {0,0}, b1 = {0,0}, t0, t1;

  row1l = vld1q_u64((const uint64_t *)&state.h[0]);
  row1h = vld1q_u64((const uint64_t *)&state.h[2]);
  row2l = vld1q_u64((const uint64_t *)&state.h[4]);
  row2h = vld1q_u64((const uint64_t *)&state.h[6]);
  row3l = vld1q_u64((const uint64_t *)&BLAKE2B_IV(0));
  row3h = vld1q_u64((const uint64_t *)&BLAKE2B_IV(2));
  row4l = veorq_u64(vld1q_u64((const uint64_t *)&BLAKE2B_IV(4)), vld1q_u64((const uint64_t*)&state.t[0]));
  row4h = veorq_u64(vld1q_u64((const uint64_t *)&BLAKE2B_IV(6)), vld1q_u64((const uint64_t*)&state.f[0]));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4h,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row4h,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_H64),row2l,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2l,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2h,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row2h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_L64),row2l,LANE_H64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2l,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2h,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row2h,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4l,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4h,LANE_L64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row4h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4h,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row4h,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_H64),row2l,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2l,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2h,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row2h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_L64),row2l,LANE_H64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2l,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2h,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row2h,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4l,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4h,LANE_L64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row4h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4h,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row4h,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_H64),row2l,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2l,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2h,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row2h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_L64),row2l,LANE_H64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2l,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2h,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row2h,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4l,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4h,LANE_L64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row4h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4h,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row4h,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_H64),row2l,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2l,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2h,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row2h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_L64),row2l,LANE_H64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2l,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2h,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row2h,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4l,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4h,LANE_L64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row4h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4h,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row4h,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_H64),row2l,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2l,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2h,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row2h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_L64),row2l,LANE_H64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2l,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2h,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row2h,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4l,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4h,LANE_L64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row4h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4h,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row4h,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_H64),row2l,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2l,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2h,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row2h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_L64),row2l,LANE_H64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2l,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2h,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row2h,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4l,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4h,LANE_L64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row4h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4h,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row4h,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_H64),row2l,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2l,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2h,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row2h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_L64),row2l,LANE_H64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2l,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2h,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row2h,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4l,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4h,LANE_L64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row4h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4h,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row4h,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_H64),row2l,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2l,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2h,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row2h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_L64),row2l,LANE_H64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2l,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2h,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row2h,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4l,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4h,LANE_L64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row4h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4h,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row4h,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_H64),row2l,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2l,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2h,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row2h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_L64),row2l,LANE_H64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2l,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2h,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row2h,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4l,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4h,LANE_L64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row4h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4h,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row4h,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_H64),row2l,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2l,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2h,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row2h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_L64),row2l,LANE_H64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2l,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2h,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row2h,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4l,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4h,LANE_L64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row4h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4h,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row4h,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_H64),row2l,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2l,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2h,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row2h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_L64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_H64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_L64),row2l,LANE_H64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2l,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2h,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row2h,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4l,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4h,LANE_L64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row4h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m8m9,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m14m15,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_L64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row4l, t1 = row2l, row4l = row3l, row3l = row3h, row3h = row4l;
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4h,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row4h,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_H64),row2l,LANE_L64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2l,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2h,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row2h,LANE_H64);

  b0 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_H64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m0m1,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m10m11,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m4m5,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,32),vshlq_n_u64(row4l,32));
  row4h = veorq_u64(vshrq_n_u64(row4h,32),vshlq_n_u64(row4h,32));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,24),vshlq_n_u64(row2l,40));
  row2h = veorq_u64(vshrq_n_u64(row2h,24),vshlq_n_u64(row2h,40));

  b0 = vsetq_lane_u64(vgetq_lane_u64(m12m13,LANE_L64),b0,LANE_L64);
  b0 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_L64),b0,LANE_H64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m6m7,LANE_H64),b1,LANE_L64);
  b1 = vsetq_lane_u64(vgetq_lane_u64(m2m3,LANE_H64),b1,LANE_H64);
  row1l = vaddq_u64(vaddq_u64(row1l, b0), row2l);
  row1h = vaddq_u64(vaddq_u64(row1h, b1), row2h);
  row4l = veorq_u64(row4l, row1l);
  row4h = veorq_u64(row4h, row1h);
  row4l = veorq_u64(vshrq_n_u64(row4l,16),vshlq_n_u64(row4l,48));
  row4h = veorq_u64(vshrq_n_u64(row4h,16),vshlq_n_u64(row4h,48));
  row3l = vaddq_u64(row3l, row4l);
  row3h = vaddq_u64(row3h, row4h);
  row2l = veorq_u64(row2l, row3l);
  row2h = veorq_u64(row2h, row3h);
  row2l = veorq_u64(vshrq_n_u64(row2l,63),vshlq_n_u64(row2l,1));
  row2h = veorq_u64(vshrq_n_u64(row2h,63),vshlq_n_u64(row2h,1));

  t0 = row3l, row3l = row3h, row3h = t0, t0 = row2l, t1 = row4l;
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2l,LANE_L64),row2l,LANE_H64);
  row2l = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_H64),row2l,LANE_L64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(row2h,LANE_L64),row2h,LANE_H64);
  row2h = vsetq_lane_u64(vgetq_lane_u64(t0,LANE_H64),row2h,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4l,LANE_H64),row4l,LANE_L64);
  row4l = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_L64),row4l,LANE_H64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(row4h,LANE_H64),row4h,LANE_L64);
  row4h = vsetq_lane_u64(vgetq_lane_u64(t1,LANE_L64),row4h,LANE_H64);

  row1l = veorq_u64(row3l, row1l);
  row1h = veorq_u64(row3h, row1h);
  vst1q_u64((uint64_t*)&state.h[0], veorq_u64(vld1q_u64((const uint64_t*)&state.h[0]), row1l));
  vst1q_u64((uint64_t*)&state.h[2], veorq_u64(vld1q_u64((const uint64_t*)&state.h[2]), row1h));

  row2l = veorq_u64(row4l, row2l);
  row2h = veorq_u64(row4h, row2h);
  vst1q_u64((uint64_t*)&state.h[4], veorq_u64(vld1q_u64((const uint64_t*)&state.h[4]), row2l));
  vst1q_u64((uint64_t*)&state.h[6], veorq_u64(vld1q_u64((const uint64_t*)&state.h[6]), row2h));
}
#endif  // CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE

template class BLAKE2_Base<word32, false>;
template class BLAKE2_Base<word64, true>;

NAMESPACE_END
