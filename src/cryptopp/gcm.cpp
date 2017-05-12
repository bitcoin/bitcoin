// gcm.cpp - written and placed in the public domain by Wei Dai

// use "cl /EP /P /DCRYPTOPP_GENERATE_X64_MASM gcm.cpp" to generate MASM code

#include "pch.h"
#include "config.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4189)
#endif

#ifndef CRYPTOPP_IMPORTS
#ifndef CRYPTOPP_GENERATE_X64_MASM

// Clang 3.3 integrated assembler crash on Linux.
#if (defined(CRYPTOPP_LLVM_CLANG_VERSION) && (CRYPTOPP_LLVM_CLANG_VERSION < 30400))
# undef CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
#endif

// SunCC 5.13 and below crash with AES-NI/CLMUL and C++{03|11}. Disable one or the other.
//   Also see http://github.com/weidai11/cryptopp/issues/226
#if defined(__SUNPRO_CC) && (__SUNPRO_CC <= 0x513)
# undef CRYPTOPP_BOOL_AESNI_INTRINSICS_AVAILABLE
#endif

#include "gcm.h"
#include "cpu.h"

NAMESPACE_BEGIN(CryptoPP)

// Different assemblers accept different mnemonics: 'movd eax, xmm0' vs 'movd rax, xmm0' vs 'mov eax, xmm0' vs 'mov rax, xmm0'
#if (CRYPTOPP_LLVM_CLANG_VERSION >= 30600) || (CRYPTOPP_APPLE_CLANG_VERSION >= 70000) || defined(CRYPTOPP_CLANG_INTEGRATED_ASSEMBLER)
// 'movd eax, xmm0' only. REG_WORD() macro not used.
# define USE_MOVD_REG32 1
#elif (defined(CRYPTOPP_LLVM_CLANG_VERSION) || defined(CRYPTOPP_APPLE_CLANG_VERSION)) && defined(CRYPTOPP_X64_ASM_AVAILABLE)
// 'movd eax, xmm0' or 'movd rax, xmm0'. REG_WORD() macro supplies REG32 or REG64.
# define USE_MOVD_REG32_OR_REG64 1
#elif defined(__GNUC__) || defined(_MSC_VER)
// 'movd eax, xmm0' or 'movd rax, xmm0'. REG_WORD() macro supplies REG32 or REG64.
# define USE_MOVD_REG32_OR_REG64 1
#else
// 'mov eax, xmm0' or 'mov rax, xmm0'. REG_WORD() macro supplies REG32 or REG64.
# define USE_MOV_REG32_OR_REG64 1
#endif

word16 GCM_Base::s_reductionTable[256];
volatile bool GCM_Base::s_reductionTableInitialized = false;

void GCM_Base::GCTR::IncrementCounterBy256()
{
	IncrementCounterByOne(m_counterArray+BlockSize()-4, 3);
}

#if 0
// preserved for testing
void gcm_gf_mult(const unsigned char *a, const unsigned char *b, unsigned char *c)
{
	word64 Z0=0, Z1=0, V0, V1;

	typedef BlockGetAndPut<word64, BigEndian> Block;
	Block::Get(a)(V0)(V1);

	for (int i=0; i<16; i++)
	{
		for (int j=0x80; j!=0; j>>=1)
		{
			int x = b[i] & j;
			Z0 ^= x ? V0 : 0;
			Z1 ^= x ? V1 : 0;
			x = (int)V1 & 1;
			V1 = (V1>>1) | (V0<<63);
			V0 = (V0>>1) ^ (x ? W64LIT(0xe1) << 56 : 0);
		}
	}
	Block::Put(NULL, c)(Z0)(Z1);
}

__m128i _mm_clmulepi64_si128(const __m128i &a, const __m128i &b, int i)
{
	word64 A[1] = {ByteReverse(((word64*)&a)[i&1])};
	word64 B[1] = {ByteReverse(((word64*)&b)[i>>4])};

	PolynomialMod2 pa((byte *)A, 8);
	PolynomialMod2 pb((byte *)B, 8);
	PolynomialMod2 c = pa*pb;

	__m128i output;
	for (int i=0; i<16; i++)
		((byte *)&output)[i] = c.GetByte(i);
	return output;
}
#endif

#if CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE || CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
inline static void SSE2_Xor16(byte *a, const byte *b, const byte *c)
{
// SunCC 5.14 crash (bewildering since asserts are not in effect in release builds)
//   Also see http://github.com/weidai11/cryptopp/issues/226 and http://github.com/weidai11/cryptopp/issues/284
# if __SUNPRO_CC
	*(__m128i *)(void *)a = _mm_xor_si128(*(__m128i *)(void *)b, *(__m128i *)(void *)c);
# elif CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE
	CRYPTOPP_ASSERT(IsAlignedOn(a,GetAlignmentOf<__m128i>()));
	CRYPTOPP_ASSERT(IsAlignedOn(b,GetAlignmentOf<__m128i>()));
	CRYPTOPP_ASSERT(IsAlignedOn(c,GetAlignmentOf<__m128i>()));
	*(__m128i *)(void *)a = _mm_xor_si128(*(__m128i *)(void *)b, *(__m128i *)(void *)c);
# else
	asm ("movdqa %1, %%xmm0; pxor %2, %%xmm0; movdqa %%xmm0, %0;" : "=m" (a[0]) : "m"(b[0]), "m"(c[0]));
# endif
}
#endif

#if CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE
inline static void NEON_Xor16(byte *a, const byte *b, const byte *c)
{
	CRYPTOPP_ASSERT(IsAlignedOn(a,GetAlignmentOf<uint64x2_t>()));
	CRYPTOPP_ASSERT(IsAlignedOn(b,GetAlignmentOf<uint64x2_t>()));
	CRYPTOPP_ASSERT(IsAlignedOn(c,GetAlignmentOf<uint64x2_t>()));
	*(uint64x2_t*)a = veorq_u64(*(uint64x2_t*)b, *(uint64x2_t*)c);
}
#endif

inline static void Xor16(byte *a, const byte *b, const byte *c)
{
	CRYPTOPP_ASSERT(IsAlignedOn(a,GetAlignmentOf<word64>()));
	CRYPTOPP_ASSERT(IsAlignedOn(b,GetAlignmentOf<word64>()));
	CRYPTOPP_ASSERT(IsAlignedOn(c,GetAlignmentOf<word64>()));
	((word64 *)(void *)a)[0] = ((word64 *)(void *)b)[0] ^ ((word64 *)(void *)c)[0];
	((word64 *)(void *)a)[1] = ((word64 *)(void *)b)[1] ^ ((word64 *)(void *)c)[1];
}

#if CRYPTOPP_BOOL_AESNI_INTRINSICS_AVAILABLE
CRYPTOPP_ALIGN_DATA(16)
static const word64 s_clmulConstants64[] = {
	W64LIT(0xe100000000000000), W64LIT(0xc200000000000000),
	W64LIT(0x08090a0b0c0d0e0f), W64LIT(0x0001020304050607),
	W64LIT(0x0001020304050607), W64LIT(0x08090a0b0c0d0e0f)};

static const __m128i *s_clmulConstants = (const __m128i *)(const void *)s_clmulConstants64;
static const unsigned int s_clmulTableSizeInBlocks = 8;

inline __m128i CLMUL_Reduce(__m128i c0, __m128i c1, __m128i c2, const __m128i &r)
{
	/*
	The polynomial to be reduced is c0 * x^128 + c1 * x^64 + c2. c0t below refers to the most
	significant half of c0 as a polynomial, which, due to GCM's bit reflection, are in the
	rightmost bit positions, and the lowest byte addresses.

	c1 ^= c0t * 0xc200000000000000
	c2t ^= c0t
	t = shift (c1t ^ c0b) left 1 bit
	c2 ^= t * 0xe100000000000000
	c2t ^= c1b
	shift c2 left 1 bit and xor in lowest bit of c1t
	*/
#if 0	// MSVC 2010 workaround: see http://connect.microsoft.com/VisualStudio/feedback/details/575301
	c2 = _mm_xor_si128(c2, _mm_move_epi64(c0));
#else
	c1 = _mm_xor_si128(c1, _mm_slli_si128(c0, 8));
#endif
	c1 = _mm_xor_si128(c1, _mm_clmulepi64_si128(c0, r, 0x10));
	c0 = _mm_srli_si128(c0, 8);
	c0 = _mm_xor_si128(c0, c1);
	c0 = _mm_slli_epi64(c0, 1);
	c0 = _mm_clmulepi64_si128(c0, r, 0);
	c2 = _mm_xor_si128(c2, c0);
	c2 = _mm_xor_si128(c2, _mm_srli_si128(c1, 8));
	c1 = _mm_unpacklo_epi64(c1, c2);
	c1 = _mm_srli_epi64(c1, 63);
	c2 = _mm_slli_epi64(c2, 1);
	return _mm_xor_si128(c2, c1);
}

inline __m128i CLMUL_GF_Mul(const __m128i &x, const __m128i &h, const __m128i &r)
{
	const __m128i c0 = _mm_clmulepi64_si128(x,h,0);
	const __m128i c1 = _mm_xor_si128(_mm_clmulepi64_si128(x,h,1), _mm_clmulepi64_si128(x,h,0x10));
	const __m128i c2 = _mm_clmulepi64_si128(x,h,0x11);

	return CLMUL_Reduce(c0, c1, c2, r);
}
#endif

#if CRYPTOPP_BOOL_ARM_CRYPTO_INTRINSICS_AVAILABLE

CRYPTOPP_ALIGN_DATA(16)
static const word64 s_clmulConstants64[] = {
	W64LIT(0xe100000000000000), W64LIT(0xc200000000000000),  // Used for ARM and x86; polynomial coefficients
	W64LIT(0x08090a0b0c0d0e0f), W64LIT(0x0001020304050607),  // Unused for ARM; used for x86 _mm_shuffle_epi8
	W64LIT(0x0001020304050607), W64LIT(0x08090a0b0c0d0e0f)   // Unused for ARM; used for x86 _mm_shuffle_epi8
};

static const uint64x2_t *s_clmulConstants = (const uint64x2_t *)s_clmulConstants64;
static const unsigned int s_clmulTableSizeInBlocks = 8;

inline uint64x2_t PMULL_Reduce(uint64x2_t c0, uint64x2_t c1, uint64x2_t c2, const uint64x2_t &r)
{
	// See comments fo CLMUL_Reduce

	c1 = veorq_u64(c1, (uint64x2_t)vextq_u8(vdupq_n_u8(0), (uint8x16_t)c0, 8));
	c1 = veorq_u64(c1, (uint64x2_t)vmull_p64(vgetq_lane_u64(c0, 0), vgetq_lane_u64(r, 1)));
	c0 = (uint64x2_t)vextq_u8((uint8x16_t)c0, vdupq_n_u8(0), 8);
	c0 = veorq_u64(c0, c1);
	c0 = vshlq_n_u64(c0, 1);
	c0 = (uint64x2_t)vmull_p64(vgetq_lane_u64(c0, 0), vgetq_lane_u64(r, 0));
	c2 = veorq_u64(c2, c0);
	c2 = veorq_u64(c2, (uint64x2_t)vextq_u8((uint8x16_t)c1, vdupq_n_u8(0), 8));
	c1 = vcombine_u64(vget_low_u64(c1), vget_low_u64(c2));
	c1 = vshrq_n_u64(c1, 63);
	c2 = vshlq_n_u64(c2, 1);

	return veorq_u64(c2, c1);
}

inline uint64x2_t PMULL_GF_Mul(const uint64x2_t &x, const uint64x2_t &h, const uint64x2_t &r)
{
	const uint64x2_t c0 = (uint64x2_t)vmull_p64(vgetq_lane_u64(x, 0), vgetq_lane_u64(h, 0));
	const uint64x2_t c1 = veorq_u64((uint64x2_t)vmull_p64(vgetq_lane_u64(x, 1), vgetq_lane_u64(h,0)),
								(uint64x2_t)vmull_p64(vgetq_lane_u64(x, 0), vgetq_lane_u64(h, 1)));
	const uint64x2_t c2 = (uint64x2_t)vmull_high_p64((poly64x2_t)x, (poly64x2_t)h);

	return PMULL_Reduce(c0, c1, c2, r);
}
#endif

void GCM_Base::SetKeyWithoutResync(const byte *userKey, size_t keylength, const NameValuePairs &params)
{
	BlockCipher &blockCipher = AccessBlockCipher();
	blockCipher.SetKey(userKey, keylength, params);

	if (blockCipher.BlockSize() != REQUIRED_BLOCKSIZE)
		throw InvalidArgument(AlgorithmName() + ": block size of underlying block cipher is not 16");

	int tableSize, i, j, k;

#if CRYPTOPP_BOOL_AESNI_INTRINSICS_AVAILABLE
	if (HasCLMUL())
	{
		// Avoid "parameter not used" error and suppress Coverity finding
		(void)params.GetIntValue(Name::TableSize(), tableSize);
		tableSize = s_clmulTableSizeInBlocks * REQUIRED_BLOCKSIZE;
	}
	else
#elif CRYPTOPP_BOOL_ARM_CRYPTO_INTRINSICS_AVAILABLE
	if (HasPMULL())
	{
		// Avoid "parameter not used" error and suppress Coverity finding
		(void)params.GetIntValue(Name::TableSize(), tableSize);
		tableSize = s_clmulTableSizeInBlocks * REQUIRED_BLOCKSIZE;
	}
	else
#endif
	{
		if (params.GetIntValue(Name::TableSize(), tableSize))
			tableSize = (tableSize >= 64*1024) ? 64*1024 : 2*1024;
		else
			tableSize = (GetTablesOption() == GCM_64K_Tables) ? 64*1024 : 2*1024;

#if defined(_MSC_VER) && (_MSC_VER >= 1300 && _MSC_VER < 1400)
		// VC 2003 workaround: compiler generates bad code for 64K tables
		tableSize = 2*1024;
#endif
	}

	m_buffer.resize(3*REQUIRED_BLOCKSIZE + tableSize);
	byte *table = MulTable();
	byte *hashKey = HashKey();
	memset(hashKey, 0, REQUIRED_BLOCKSIZE);
	blockCipher.ProcessBlock(hashKey);

#if CRYPTOPP_BOOL_AESNI_INTRINSICS_AVAILABLE
	if (HasCLMUL())
	{
		const __m128i r = s_clmulConstants[0];
		__m128i h0 = _mm_shuffle_epi8(_mm_load_si128((__m128i *)(void *)hashKey), s_clmulConstants[1]);
		__m128i h = h0;

		for (i=0; i<tableSize; i+=32)
		{
			__m128i h1 = CLMUL_GF_Mul(h, h0, r);
			_mm_storel_epi64((__m128i *)(void *)(table+i), h);
			_mm_storeu_si128((__m128i *)(void *)(table+i+16), h1);
			_mm_storeu_si128((__m128i *)(void *)(table+i+8), h);
			_mm_storel_epi64((__m128i *)(void *)(table+i+8), h1);
			h = CLMUL_GF_Mul(h1, h0, r);
		}

		return;
	}
#elif CRYPTOPP_BOOL_ARM_CRYPTO_INTRINSICS_AVAILABLE
	if (HasPMULL())
	{
		const uint64x2_t r = s_clmulConstants[0];
		const uint64x2_t t = vld1q_u64((uint64_t *)hashKey);
		const uint64x2_t h0 = (uint64x2_t)vrev64q_u8((uint8x16_t)vcombine_u64(vget_high_u64(t), vget_low_u64(t)));

		uint64x2_t h = h0;
		for (i=0; i<tableSize-32; i+=32)
		{
			const uint64x2_t h1 = PMULL_GF_Mul(h, h0, r);
			vst1_u64((uint64_t *)(table+i), vget_low_u64(h));
			vst1q_u64((uint64_t *)(table+i+16), h1);
			vst1q_u64((uint64_t *)(table+i+8), h);
			vst1_u64((uint64_t *)(table+i+8), vget_low_u64(h1));
			h = PMULL_GF_Mul(h1, h0, r);
		}

		const uint64x2_t h1 = PMULL_GF_Mul(h, h0, r);
		vst1_u64((uint64_t *)(table+i), vget_low_u64(h));
		vst1q_u64((uint64_t *)(table+i+16), h1);
		vst1q_u64((uint64_t *)(table+i+8), h);
		vst1_u64((uint64_t *)(table+i+8), vget_low_u64(h1));

		return;
	}
#endif

	word64 V0, V1;
	typedef BlockGetAndPut<word64, BigEndian> Block;
	Block::Get(hashKey)(V0)(V1);

	if (tableSize == 64*1024)
	{
		for (i=0; i<128; i++)
		{
			k = i%8;
			Block::Put(NULL, table+(i/8)*256*16+(size_t(1)<<(11-k)))(V0)(V1);

			int x = (int)V1 & 1;
			V1 = (V1>>1) | (V0<<63);
			V0 = (V0>>1) ^ (x ? W64LIT(0xe1) << 56 : 0);
		}

		for (i=0; i<16; i++)
		{
			memset(table+i*256*16, 0, 16);
#if CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE || CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
			if (HasSSE2())
				for (j=2; j<=0x80; j*=2)
					for (k=1; k<j; k++)
						SSE2_Xor16(table+i*256*16+(j+k)*16, table+i*256*16+j*16, table+i*256*16+k*16);
			else
#elif CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE
			if (HasNEON())
				for (j=2; j<=0x80; j*=2)
					for (k=1; k<j; k++)
						NEON_Xor16(table+i*256*16+(j+k)*16, table+i*256*16+j*16, table+i*256*16+k*16);
			else
#endif
				for (j=2; j<=0x80; j*=2)
					for (k=1; k<j; k++)
						Xor16(table+i*256*16+(j+k)*16, table+i*256*16+j*16, table+i*256*16+k*16);
		}
	}
	else
	{
		if (!s_reductionTableInitialized)
		{
			s_reductionTable[0] = 0;
			word16 x = 0x01c2;
			s_reductionTable[1] = ByteReverse(x);
			for (unsigned int ii=2; ii<=0x80; ii*=2)
			{
				x <<= 1;
				s_reductionTable[ii] = ByteReverse(x);
				for (unsigned int jj=1; jj<ii; jj++)
					s_reductionTable[ii+jj] = s_reductionTable[ii] ^ s_reductionTable[jj];
			}
			s_reductionTableInitialized = true;
		}

		for (i=0; i<128-24; i++)
		{
			k = i%32;
			if (k < 4)
				Block::Put(NULL, table+1024+(i/32)*256+(size_t(1)<<(7-k)))(V0)(V1);
			else if (k < 8)
				Block::Put(NULL, table+(i/32)*256+(size_t(1)<<(11-k)))(V0)(V1);

			int x = (int)V1 & 1;
			V1 = (V1>>1) | (V0<<63);
			V0 = (V0>>1) ^ (x ? W64LIT(0xe1) << 56 : 0);
		}

		for (i=0; i<4; i++)
		{
			memset(table+i*256, 0, 16);
			memset(table+1024+i*256, 0, 16);
#if CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE || CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
			if (HasSSE2())
				for (j=2; j<=8; j*=2)
					for (k=1; k<j; k++)
					{
						SSE2_Xor16(table+i*256+(j+k)*16, table+i*256+j*16, table+i*256+k*16);
						SSE2_Xor16(table+1024+i*256+(j+k)*16, table+1024+i*256+j*16, table+1024+i*256+k*16);
					}
			else
#elif CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE
			if (HasNEON())
				for (j=2; j<=8; j*=2)
					for (k=1; k<j; k++)
					{
						NEON_Xor16(table+i*256+(j+k)*16, table+i*256+j*16, table+i*256+k*16);
						NEON_Xor16(table+1024+i*256+(j+k)*16, table+1024+i*256+j*16, table+1024+i*256+k*16);
					}
			else
#endif
				for (j=2; j<=8; j*=2)
					for (k=1; k<j; k++)
					{
						Xor16(table+i*256+(j+k)*16, table+i*256+j*16, table+i*256+k*16);
						Xor16(table+1024+i*256+(j+k)*16, table+1024+i*256+j*16, table+1024+i*256+k*16);
					}
		}
	}
}

inline void GCM_Base::ReverseHashBufferIfNeeded()
{
#if CRYPTOPP_BOOL_AESNI_INTRINSICS_AVAILABLE
	if (HasCLMUL())
	{
		__m128i &x = *(__m128i *)(void *)HashBuffer();
		x = _mm_shuffle_epi8(x, s_clmulConstants[1]);
	}
#elif CRYPTOPP_BOOL_ARM_CRYPTO_INTRINSICS_AVAILABLE
	if (HasPMULL())
	{
		if (GetNativeByteOrder() != BIG_ENDIAN_ORDER)
		{
			const uint8x16_t x = vrev64q_u8(vld1q_u8(HashBuffer()));
			vst1q_u8(HashBuffer(), (uint8x16_t)vcombine_u64(vget_high_u64((uint64x2_t)x), vget_low_u64((uint64x2_t)x)));
		}
	}
#endif
}

void GCM_Base::Resync(const byte *iv, size_t len)
{
	BlockCipher &cipher = AccessBlockCipher();
	byte *hashBuffer = HashBuffer();

	if (len == 12)
	{
		memcpy(hashBuffer, iv, len);
		memset(hashBuffer+len, 0, 3);
		hashBuffer[len+3] = 1;
	}
	else
	{
		size_t origLen = len;
		memset(hashBuffer, 0, HASH_BLOCKSIZE);

		if (len >= HASH_BLOCKSIZE)
		{
			len = GCM_Base::AuthenticateBlocks(iv, len);
			iv += (origLen - len);
		}

		if (len > 0)
		{
			memcpy(m_buffer, iv, len);
			memset(m_buffer+len, 0, HASH_BLOCKSIZE-len);
			GCM_Base::AuthenticateBlocks(m_buffer, HASH_BLOCKSIZE);
		}

		PutBlock<word64, BigEndian, true>(NULL, m_buffer)(0)(origLen*8);
		GCM_Base::AuthenticateBlocks(m_buffer, HASH_BLOCKSIZE);

		ReverseHashBufferIfNeeded();
	}

	if (m_state >= State_IVSet)
		m_ctr.Resynchronize(hashBuffer, REQUIRED_BLOCKSIZE);
	else
		m_ctr.SetCipherWithIV(cipher, hashBuffer);

	m_ctr.Seek(HASH_BLOCKSIZE);

	memset(hashBuffer, 0, HASH_BLOCKSIZE);
}

unsigned int GCM_Base::OptimalDataAlignment() const
{
	return
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE || defined(CRYPTOPP_X64_MASM_AVAILABLE)
		HasSSE2() ? 16 :
#elif CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE
		HasNEON() ? 16 :
#endif
		GetBlockCipher().OptimalDataAlignment();
}

#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4731)	// frame pointer register 'ebp' modified by inline assembly code
#endif

#endif	// #ifndef CRYPTOPP_GENERATE_X64_MASM

#ifdef CRYPTOPP_X64_MASM_AVAILABLE
extern "C" {
void GCM_AuthenticateBlocks_2K(const byte *data, size_t blocks, word64 *hashBuffer, const word16 *reductionTable);
void GCM_AuthenticateBlocks_64K(const byte *data, size_t blocks, word64 *hashBuffer);
}
#endif

#ifndef CRYPTOPP_GENERATE_X64_MASM

size_t GCM_Base::AuthenticateBlocks(const byte *data, size_t len)
{
#if CRYPTOPP_BOOL_AESNI_INTRINSICS_AVAILABLE
	if (HasCLMUL())
	{
		const __m128i *table = (const __m128i *)(const void *)MulTable();
		__m128i x = _mm_load_si128((__m128i *)(void *)HashBuffer());
		const __m128i r = s_clmulConstants[0], mask1 = s_clmulConstants[1], mask2 = s_clmulConstants[2];

		while (len >= 16)
		{
			size_t s = UnsignedMin(len/16, s_clmulTableSizeInBlocks), i=0;
			__m128i d, d2 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i *)(const void *)(data+(s-1)*16)), mask2);;
			__m128i c0 = _mm_setzero_si128();
			__m128i c1 = _mm_setzero_si128();
			__m128i c2 = _mm_setzero_si128();

			while (true)
			{
				__m128i h0 = _mm_load_si128(table+i);
				__m128i h1 = _mm_load_si128(table+i+1);
				__m128i h2 = _mm_xor_si128(h0, h1);

				if (++i == s)
				{
					d = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i *)(const void *)data), mask1);
					d = _mm_xor_si128(d, x);
					c0 = _mm_xor_si128(c0, _mm_clmulepi64_si128(d, h0, 0));
					c2 = _mm_xor_si128(c2, _mm_clmulepi64_si128(d, h1, 1));
					d = _mm_xor_si128(d, _mm_shuffle_epi32(d, _MM_SHUFFLE(1, 0, 3, 2)));
					c1 = _mm_xor_si128(c1, _mm_clmulepi64_si128(d, h2, 0));
					break;
				}

				d = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i *)(const void *)(data+(s-i)*16-8)), mask2);
				c0 = _mm_xor_si128(c0, _mm_clmulepi64_si128(d2, h0, 1));
				c2 = _mm_xor_si128(c2, _mm_clmulepi64_si128(d, h1, 1));
				d2 = _mm_xor_si128(d2, d);
				c1 = _mm_xor_si128(c1, _mm_clmulepi64_si128(d2, h2, 1));

				if (++i == s)
				{
					d = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i *)(const void *)data), mask1);
					d = _mm_xor_si128(d, x);
					c0 = _mm_xor_si128(c0, _mm_clmulepi64_si128(d, h0, 0x10));
					c2 = _mm_xor_si128(c2, _mm_clmulepi64_si128(d, h1, 0x11));
					d = _mm_xor_si128(d, _mm_shuffle_epi32(d, _MM_SHUFFLE(1, 0, 3, 2)));
					c1 = _mm_xor_si128(c1, _mm_clmulepi64_si128(d, h2, 0x10));
					break;
				}

				d2 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i *)(const void *)(data+(s-i)*16-8)), mask1);
				c0 = _mm_xor_si128(c0, _mm_clmulepi64_si128(d, h0, 0x10));
				c2 = _mm_xor_si128(c2, _mm_clmulepi64_si128(d2, h1, 0x10));
				d = _mm_xor_si128(d, d2);
				c1 = _mm_xor_si128(c1, _mm_clmulepi64_si128(d, h2, 0x10));
			}
			data += s*16;
			len -= s*16;

			c1 = _mm_xor_si128(_mm_xor_si128(c1, c0), c2);
			x = CLMUL_Reduce(c0, c1, c2, r);
		}

		_mm_store_si128((__m128i *)(void *)HashBuffer(), x);
		return len;
	}
#elif CRYPTOPP_BOOL_ARM_CRYPTO_INTRINSICS_AVAILABLE
	if (HasPMULL())
	{
		const uint64x2_t *table = (const uint64x2_t *)MulTable();
		uint64x2_t x = vld1q_u64((const uint64_t*)HashBuffer());
		const uint64x2_t r = s_clmulConstants[0];

		while (len >= 16)
		{
			size_t s = UnsignedMin(len/16, s_clmulTableSizeInBlocks), i=0;
			uint64x2_t d, d2 = (uint64x2_t)vrev64q_u8((uint8x16_t)vld1q_u64((const uint64_t *)(data+(s-1)*16)));
			uint64x2_t c0 = vdupq_n_u64(0);
			uint64x2_t c1 = vdupq_n_u64(0);
			uint64x2_t c2 = vdupq_n_u64(0);

			while (true)
			{
				const uint64x2_t h0 = vld1q_u64((const uint64_t*)(table+i));
				const uint64x2_t h1 = vld1q_u64((const uint64_t*)(table+i+1));
				const uint64x2_t h2 = veorq_u64(h0, h1);

				if (++i == s)
				{
					const uint64x2_t t1 = vld1q_u64((const uint64_t *)data);
					 d = veorq_u64((uint64x2_t)vrev64q_u8((uint8x16_t)vcombine_u64(vget_high_u64(t1), vget_low_u64(t1))), x);
					c0 = veorq_u64(c0, (uint64x2_t)vmull_p64(vgetq_lane_u64(d, 0), vgetq_lane_u64(h0, 0)));
					c2 = veorq_u64(c2, (uint64x2_t)vmull_p64(vgetq_lane_u64(d, 1), vgetq_lane_u64(h1, 0)));
					 d = veorq_u64(d, (uint64x2_t)vcombine_u32(vget_high_u32((uint32x4_t)d), vget_low_u32((uint32x4_t)d)));
					c1 = veorq_u64(c1, (uint64x2_t)vmull_p64(vgetq_lane_u64(d, 0), vgetq_lane_u64(h2, 0)));

					break;
				}

				 d = (uint64x2_t)vrev64q_u8((uint8x16_t)vld1q_u64((const uint64_t *)(data+(s-i)*16-8)));
				c0 = veorq_u64(c0, (uint64x2_t)vmull_p64(vgetq_lane_u64(d2, 1), vgetq_lane_u64(h0, 0)));
				c2 = veorq_u64(c2, (uint64x2_t)vmull_p64(vgetq_lane_u64(d, 1), vgetq_lane_u64(h1, 0)));
				d2 = veorq_u64(d2, d);
				c1 = veorq_u64(c1, (uint64x2_t)vmull_p64(vgetq_lane_u64(d2, 1), vgetq_lane_u64(h2, 0)));

				if (++i == s)
				{

					const uint64x2_t t2 = vld1q_u64((const uint64_t *)data);
					 d = veorq_u64((uint64x2_t)vrev64q_u8((uint8x16_t)vcombine_u64(vget_high_u64(t2), vget_low_u64(t2))), x);
					c0 = veorq_u64(c0, (uint64x2_t)vmull_p64(vgetq_lane_u64(d, 0), vgetq_lane_u64(h0, 1)));
					c2 = veorq_u64(c2, (uint64x2_t)vmull_high_p64((poly64x2_t)d, (poly64x2_t)h1));
					 d = veorq_u64(d, (uint64x2_t)vcombine_u32(vget_high_u32((uint32x4_t)d), vget_low_u32((uint32x4_t)d)));
					c1 = veorq_u64(c1, (uint64x2_t)vmull_p64(vgetq_lane_u64(d, 0), vgetq_lane_u64(h2, 1)));

					break;
				}

				const uint64x2_t t3 = vld1q_u64((uint64_t *)(data+(s-i)*16-8));
				d2 = (uint64x2_t)vrev64q_u8((uint8x16_t)vcombine_u64(vget_high_u64(t3), vget_low_u64(t3)));
				c0 = veorq_u64(c0, (uint64x2_t)vmull_p64(vgetq_lane_u64(d, 0), vgetq_lane_u64(h0, 1)));
				c2 = veorq_u64(c2, (uint64x2_t)vmull_p64(vgetq_lane_u64(d2, 0), vgetq_lane_u64(h1, 1)));
				 d = veorq_u64(d, d2);
				c1 = veorq_u64(c1, (uint64x2_t)vmull_p64(vgetq_lane_u64(d, 0), vgetq_lane_u64(h2, 1)));
			}
			data += s*16;
			len -= s*16;

			c1 = veorq_u64(veorq_u64(c1, c0), c2);
			x = PMULL_Reduce(c0, c1, c2, r);
		}

		vst1q_u64((uint64_t *)HashBuffer(), x);
		return len;
}
#endif

	typedef BlockGetAndPut<word64, NativeByteOrder> Block;
	word64 *hashBuffer = (word64 *)(void *)HashBuffer();
	CRYPTOPP_ASSERT(IsAlignedOn(hashBuffer,GetAlignmentOf<word64>()));

	switch (2*(m_buffer.size()>=64*1024)
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE || defined(CRYPTOPP_X64_MASM_AVAILABLE)
		+ HasSSE2()
//#elif CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE
//		+ HasNEON()
#endif
		)
	{
	case 0:		// non-SSE2 and 2K tables
		{
		byte *table = MulTable();
		word64 x0 = hashBuffer[0], x1 = hashBuffer[1];

		do
		{
			word64 y0, y1, a0, a1, b0, b1, c0, c1, d0, d1;
			Block::Get(data)(y0)(y1);
			x0 ^= y0;
			x1 ^= y1;

			data += HASH_BLOCKSIZE;
			len -= HASH_BLOCKSIZE;

			#define READ_TABLE_WORD64_COMMON(a, b, c, d)	*(word64 *)(void *)(table+(a*1024)+(b*256)+c+d*8)

			#ifdef IS_LITTLE_ENDIAN
				#if CRYPTOPP_BOOL_SLOW_WORD64
					word32 z0 = (word32)x0;
					word32 z1 = (word32)(x0>>32);
					word32 z2 = (word32)x1;
					word32 z3 = (word32)(x1>>32);
					#define READ_TABLE_WORD64(a, b, c, d, e)	READ_TABLE_WORD64_COMMON((d%2), c, (d?(z##c>>((d?d-1:0)*4))&0xf0:(z##c&0xf)<<4), e)
				#else
					#define READ_TABLE_WORD64(a, b, c, d, e)	READ_TABLE_WORD64_COMMON((d%2), c, ((d+8*b)?(x##a>>(((d+8*b)?(d+8*b)-1:1)*4))&0xf0:(x##a&0xf)<<4), e)
				#endif
				#define GF_MOST_SIG_8BITS(a) (a##1 >> 7*8)
				#define GF_SHIFT_8(a) a##1 = (a##1 << 8) ^ (a##0 >> 7*8); a##0 <<= 8;
			#else
				#define READ_TABLE_WORD64(a, b, c, d, e)	READ_TABLE_WORD64_COMMON((1-d%2), c, ((15-d-8*b)?(x##a>>(((15-d-8*b)?(15-d-8*b)-1:0)*4))&0xf0:(x##a&0xf)<<4), e)
				#define GF_MOST_SIG_8BITS(a) (a##1 & 0xff)
				#define GF_SHIFT_8(a) a##1 = (a##1 >> 8) ^ (a##0 << 7*8); a##0 >>= 8;
			#endif

			#define GF_MUL_32BY128(op, a, b, c)											\
				a0 op READ_TABLE_WORD64(a, b, c, 0, 0) ^ READ_TABLE_WORD64(a, b, c, 1, 0);\
				a1 op READ_TABLE_WORD64(a, b, c, 0, 1) ^ READ_TABLE_WORD64(a, b, c, 1, 1);\
				b0 op READ_TABLE_WORD64(a, b, c, 2, 0) ^ READ_TABLE_WORD64(a, b, c, 3, 0);\
				b1 op READ_TABLE_WORD64(a, b, c, 2, 1) ^ READ_TABLE_WORD64(a, b, c, 3, 1);\
				c0 op READ_TABLE_WORD64(a, b, c, 4, 0) ^ READ_TABLE_WORD64(a, b, c, 5, 0);\
				c1 op READ_TABLE_WORD64(a, b, c, 4, 1) ^ READ_TABLE_WORD64(a, b, c, 5, 1);\
				d0 op READ_TABLE_WORD64(a, b, c, 6, 0) ^ READ_TABLE_WORD64(a, b, c, 7, 0);\
				d1 op READ_TABLE_WORD64(a, b, c, 6, 1) ^ READ_TABLE_WORD64(a, b, c, 7, 1);\

			GF_MUL_32BY128(=, 0, 0, 0)
			GF_MUL_32BY128(^=, 0, 1, 1)
			GF_MUL_32BY128(^=, 1, 0, 2)
			GF_MUL_32BY128(^=, 1, 1, 3)

			word32 r = (word32)s_reductionTable[GF_MOST_SIG_8BITS(d)] << 16;
			GF_SHIFT_8(d)
			c0 ^= d0; c1 ^= d1;
			r ^= (word32)s_reductionTable[GF_MOST_SIG_8BITS(c)] << 8;
			GF_SHIFT_8(c)
			b0 ^= c0; b1 ^= c1;
			r ^= s_reductionTable[GF_MOST_SIG_8BITS(b)];
			GF_SHIFT_8(b)
			a0 ^= b0; a1 ^= b1;
			a0 ^= ConditionalByteReverse<word64>(LITTLE_ENDIAN_ORDER, r);
			x0 = a0; x1 = a1;
		}
		while (len >= HASH_BLOCKSIZE);

		hashBuffer[0] = x0; hashBuffer[1] = x1;
		return len;
		}

	case 2:		// non-SSE2 and 64K tables
		{
		byte *table = MulTable();
		word64 x0 = hashBuffer[0], x1 = hashBuffer[1];

		do
		{
			word64 y0, y1, a0, a1;
			Block::Get(data)(y0)(y1);
			x0 ^= y0;
			x1 ^= y1;

			data += HASH_BLOCKSIZE;
			len -= HASH_BLOCKSIZE;

			#undef READ_TABLE_WORD64_COMMON
			#undef READ_TABLE_WORD64

			#define READ_TABLE_WORD64_COMMON(a, c, d)	*(word64 *)(void *)(table+(a)*256*16+(c)+(d)*8)

			#ifdef IS_LITTLE_ENDIAN
				#if CRYPTOPP_BOOL_SLOW_WORD64
					word32 z0 = (word32)x0;
					word32 z1 = (word32)(x0>>32);
					word32 z2 = (word32)x1;
					word32 z3 = (word32)(x1>>32);
					#define READ_TABLE_WORD64(b, c, d, e)	READ_TABLE_WORD64_COMMON(c*4+d, (d?(z##c>>((d?d:1)*8-4))&0xff0:(z##c&0xff)<<4), e)
				#else
					#define READ_TABLE_WORD64(b, c, d, e)	READ_TABLE_WORD64_COMMON(c*4+d, ((d+4*(c%2))?(x##b>>(((d+4*(c%2))?(d+4*(c%2)):1)*8-4))&0xff0:(x##b&0xff)<<4), e)
				#endif
			#else
				#define READ_TABLE_WORD64(b, c, d, e)	READ_TABLE_WORD64_COMMON(c*4+d, ((7-d-4*(c%2))?(x##b>>(((7-d-4*(c%2))?(7-d-4*(c%2)):1)*8-4))&0xff0:(x##b&0xff)<<4), e)
			#endif

			#define GF_MUL_8BY128(op, b, c, d)		\
				a0 op READ_TABLE_WORD64(b, c, d, 0);\
				a1 op READ_TABLE_WORD64(b, c, d, 1);\

			GF_MUL_8BY128(=, 0, 0, 0)
			GF_MUL_8BY128(^=, 0, 0, 1)
			GF_MUL_8BY128(^=, 0, 0, 2)
			GF_MUL_8BY128(^=, 0, 0, 3)
			GF_MUL_8BY128(^=, 0, 1, 0)
			GF_MUL_8BY128(^=, 0, 1, 1)
			GF_MUL_8BY128(^=, 0, 1, 2)
			GF_MUL_8BY128(^=, 0, 1, 3)
			GF_MUL_8BY128(^=, 1, 2, 0)
			GF_MUL_8BY128(^=, 1, 2, 1)
			GF_MUL_8BY128(^=, 1, 2, 2)
			GF_MUL_8BY128(^=, 1, 2, 3)
			GF_MUL_8BY128(^=, 1, 3, 0)
			GF_MUL_8BY128(^=, 1, 3, 1)
			GF_MUL_8BY128(^=, 1, 3, 2)
			GF_MUL_8BY128(^=, 1, 3, 3)

			x0 = a0; x1 = a1;
		}
		while (len >= HASH_BLOCKSIZE);

		hashBuffer[0] = x0; hashBuffer[1] = x1;
		return len;
		}
#endif	// #ifndef CRYPTOPP_GENERATE_X64_MASM

#ifdef CRYPTOPP_X64_MASM_AVAILABLE
	case 1:		// SSE2 and 2K tables
		GCM_AuthenticateBlocks_2K(data, len/16, hashBuffer, s_reductionTable);
		return len % 16;
	case 3:		// SSE2 and 64K tables
		GCM_AuthenticateBlocks_64K(data, len/16, hashBuffer);
		return len % 16;
#endif

#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
	case 1:		// SSE2 and 2K tables
		{
		#ifdef __GNUC__
			__asm__ __volatile__
			(
			INTEL_NOPREFIX
		#elif defined(CRYPTOPP_GENERATE_X64_MASM)
			ALIGN   8
			GCM_AuthenticateBlocks_2K	PROC FRAME
			rex_push_reg rsi
			push_reg rdi
			push_reg rbx
			.endprolog
			mov rsi, r8
			mov r11, r9
		#else
			AS2(	mov		WORD_REG(cx), data			)
			AS2(	mov		WORD_REG(dx), len			)
			AS2(	mov		WORD_REG(si), hashBuffer	)
			AS2(	shr		WORD_REG(dx), 4				)
		#endif

		#if CRYPTOPP_BOOL_X32
			AS1(push	rbx)
			AS1(push	rbp)
		#else
			AS_PUSH_IF86(	bx)
			AS_PUSH_IF86(	bp)
		#endif

		#ifdef __GNUC__
			AS2(	mov		AS_REG_7, WORD_REG(di))
		#elif CRYPTOPP_BOOL_X86
			AS2(	lea		AS_REG_7, s_reductionTable)
		#endif

		AS2(	movdqa	xmm0, [WORD_REG(si)]			)

		#define MUL_TABLE_0 WORD_REG(si) + 32
		#define MUL_TABLE_1 WORD_REG(si) + 32 + 1024
		#define RED_TABLE AS_REG_7

		ASL(0)
		AS2(	movdqu	xmm4, [WORD_REG(cx)]			)
		AS2(	pxor	xmm0, xmm4						)

		AS2(	movd	ebx, xmm0						)
		AS2(	mov		eax, AS_HEX(f0f0f0f0)			)
		AS2(	and		eax, ebx						)
		AS2(	shl		ebx, 4							)
		AS2(	and		ebx, AS_HEX(f0f0f0f0)			)
		AS2(	movzx	edi, ah							)
		AS2(	movdqa	xmm5, XMMWORD_PTR [MUL_TABLE_1 + WORD_REG(di)]	)
		AS2(	movzx	edi, al					)
		AS2(	movdqa	xmm4, XMMWORD_PTR [MUL_TABLE_1 + WORD_REG(di)]	)
		AS2(	shr		eax, 16							)
		AS2(	movzx	edi, ah					)
		AS2(	movdqa	xmm3, XMMWORD_PTR [MUL_TABLE_1 + WORD_REG(di)]	)
		AS2(	movzx	edi, al					)
		AS2(	movdqa	xmm2, XMMWORD_PTR [MUL_TABLE_1 + WORD_REG(di)]	)

		#define SSE2_MUL_32BITS(i)											\
			AS2(	psrldq	xmm0, 4											)\
			AS2(	movd	eax, xmm0										)\
			AS2(	and		eax, AS_HEX(f0f0f0f0)									)\
			AS2(	movzx	edi, bh											)\
			AS2(	pxor	xmm5, XMMWORD_PTR [MUL_TABLE_0 + (i-1)*256 + WORD_REG(di)]	)\
			AS2(	movzx	edi, bl											)\
			AS2(	pxor	xmm4, XMMWORD_PTR [MUL_TABLE_0 + (i-1)*256 + WORD_REG(di)]	)\
			AS2(	shr		ebx, 16											)\
			AS2(	movzx	edi, bh											)\
			AS2(	pxor	xmm3, XMMWORD_PTR [MUL_TABLE_0 + (i-1)*256 + WORD_REG(di)]	)\
			AS2(	movzx	edi, bl											)\
			AS2(	pxor	xmm2, XMMWORD_PTR [MUL_TABLE_0 + (i-1)*256 + WORD_REG(di)]	)\
			AS2(	movd	ebx, xmm0										)\
			AS2(	shl		ebx, 4											)\
			AS2(	and		ebx, AS_HEX(f0f0f0f0)									)\
			AS2(	movzx	edi, ah											)\
			AS2(	pxor	xmm5, XMMWORD_PTR [MUL_TABLE_1 + i*256 + WORD_REG(di)]		)\
			AS2(	movzx	edi, al											)\
			AS2(	pxor	xmm4, XMMWORD_PTR [MUL_TABLE_1 + i*256 + WORD_REG(di)]		)\
			AS2(	shr		eax, 16											)\
			AS2(	movzx	edi, ah											)\
			AS2(	pxor	xmm3, XMMWORD_PTR [MUL_TABLE_1 + i*256 + WORD_REG(di)]		)\
			AS2(	movzx	edi, al											)\
			AS2(	pxor	xmm2, XMMWORD_PTR [MUL_TABLE_1 + i*256 + WORD_REG(di)]		)\

		SSE2_MUL_32BITS(1)
		SSE2_MUL_32BITS(2)
		SSE2_MUL_32BITS(3)

		AS2(	movzx	edi, bh					)
		AS2(	pxor	xmm5, XMMWORD_PTR [MUL_TABLE_0 + 3*256 + WORD_REG(di)]	)
		AS2(	movzx	edi, bl					)
		AS2(	pxor	xmm4, XMMWORD_PTR [MUL_TABLE_0 + 3*256 + WORD_REG(di)]	)
		AS2(	shr		ebx, 16						)
		AS2(	movzx	edi, bh					)
		AS2(	pxor	xmm3, XMMWORD_PTR [MUL_TABLE_0 + 3*256 + WORD_REG(di)]	)
		AS2(	movzx	edi, bl					)
		AS2(	pxor	xmm2, XMMWORD_PTR [MUL_TABLE_0 + 3*256 + WORD_REG(di)]	)

		AS2(	movdqa	xmm0, xmm3						)
		AS2(	pslldq	xmm3, 1							)
		AS2(	pxor	xmm2, xmm3						)
		AS2(	movdqa	xmm1, xmm2						)
		AS2(	pslldq	xmm2, 1							)
		AS2(	pxor	xmm5, xmm2						)

		AS2(	psrldq	xmm0, 15						)
#if USE_MOVD_REG32
		AS2(	movd	edi, xmm0						)
#elif USE_MOV_REG32_OR_REG64
		AS2(	mov		WORD_REG(di), xmm0				)
#else	// GNU Assembler
		AS2(	movd	WORD_REG(di), xmm0				)
#endif
		AS2(	movzx	eax, WORD PTR [RED_TABLE + WORD_REG(di)*2]	)
		AS2(	shl		eax, 8							)

		AS2(	movdqa	xmm0, xmm5						)
		AS2(	pslldq	xmm5, 1							)
		AS2(	pxor	xmm4, xmm5						)

		AS2(	psrldq	xmm1, 15						)
#if USE_MOVD_REG32
		AS2(	movd	edi, xmm1						)
#elif USE_MOV_REG32_OR_REG64
		AS2(	mov		WORD_REG(di), xmm1				)
#else
		AS2(	movd	WORD_REG(di), xmm1				)
#endif
		AS2(	xor		ax, WORD PTR [RED_TABLE + WORD_REG(di)*2]	)
		AS2(	shl		eax, 8							)

		AS2(	psrldq	xmm0, 15						)
#if USE_MOVD_REG32
		AS2(	movd	edi, xmm0						)
#elif USE_MOV_REG32_OR_REG64
		AS2(	mov		WORD_REG(di), xmm0				)
#else
		AS2(	movd	WORD_REG(di), xmm0				)
#endif
		AS2(	xor		ax, WORD PTR [RED_TABLE + WORD_REG(di)*2]	)

		AS2(	movd	xmm0, eax						)
		AS2(	pxor	xmm0, xmm4						)

		AS2(	add		WORD_REG(cx), 16				)
		AS2(	sub		WORD_REG(dx), 1					)
		ATT_NOPREFIX
		ASJ(	jnz,	0, b							)
		INTEL_NOPREFIX
		AS2(	movdqa	[WORD_REG(si)], xmm0			)

		#if CRYPTOPP_BOOL_X32
			AS1(pop		rbp)
			AS1(pop		rbx)
		#else
			AS_POP_IF86(	bp)
			AS_POP_IF86(	bx)
		#endif

		#ifdef __GNUC__
				ATT_PREFIX
					:
					: "c" (data), "d" (len/16), "S" (hashBuffer), "D" (s_reductionTable)
					: "memory", "cc", "%eax"
			#if CRYPTOPP_BOOL_X64
					, "%ebx", "%r11"
			#endif
				);
		#elif defined(CRYPTOPP_GENERATE_X64_MASM)
			pop rbx
			pop rdi
			pop rsi
			ret
			GCM_AuthenticateBlocks_2K ENDP
		#endif

		return len%16;
		}
	case 3:		// SSE2 and 64K tables
		{
		#ifdef __GNUC__
			__asm__ __volatile__
			(
			INTEL_NOPREFIX
		#elif defined(CRYPTOPP_GENERATE_X64_MASM)
			ALIGN   8
			GCM_AuthenticateBlocks_64K	PROC FRAME
			rex_push_reg rsi
			push_reg rdi
			.endprolog
			mov rsi, r8
		#else
			AS2(	mov		WORD_REG(cx), data			)
			AS2(	mov		WORD_REG(dx), len			)
			AS2(	mov		WORD_REG(si), hashBuffer	)
			AS2(	shr		WORD_REG(dx), 4				)
		#endif

		AS2(	movdqa	xmm0, [WORD_REG(si)]				)

		#undef MUL_TABLE
		#define MUL_TABLE(i,j) WORD_REG(si) + 32 + (i*4+j)*256*16

		ASL(1)
		AS2(	movdqu	xmm1, [WORD_REG(cx)]				)
		AS2(	pxor	xmm1, xmm0						)
		AS2(	pxor	xmm0, xmm0						)

		#undef SSE2_MUL_32BITS
		#define SSE2_MUL_32BITS(i)								\
			AS2(	movd	eax, xmm1							)\
			AS2(	psrldq	xmm1, 4								)\
			AS2(	movzx	edi, al						)\
			AS2(	add		WORD_REG(di), WORD_REG(di)					)\
			AS2(	pxor	xmm0, [MUL_TABLE(i,0) + WORD_REG(di)*8]	)\
			AS2(	movzx	edi, ah						)\
			AS2(	add		WORD_REG(di), WORD_REG(di)					)\
			AS2(	pxor	xmm0, [MUL_TABLE(i,1) + WORD_REG(di)*8]	)\
			AS2(	shr		eax, 16								)\
			AS2(	movzx	edi, al						)\
			AS2(	add		WORD_REG(di), WORD_REG(di)					)\
			AS2(	pxor	xmm0, [MUL_TABLE(i,2) + WORD_REG(di)*8]	)\
			AS2(	movzx	edi, ah						)\
			AS2(	add		WORD_REG(di), WORD_REG(di)					)\
			AS2(	pxor	xmm0, [MUL_TABLE(i,3) + WORD_REG(di)*8]	)\

		SSE2_MUL_32BITS(0)
		SSE2_MUL_32BITS(1)
		SSE2_MUL_32BITS(2)
		SSE2_MUL_32BITS(3)

		AS2(	add	WORD_REG(cx), 16		)
		AS2(	sub	WORD_REG(dx), 1			)
		ATT_NOPREFIX
		ASJ(	jnz,	1, b				)
		INTEL_NOPREFIX
		AS2(	movdqa	[WORD_REG(si)], xmm0		)

		#ifdef __GNUC__
				ATT_PREFIX
					:
					: "c" (data), "d" (len/16), "S" (hashBuffer)
					: "memory", "cc", "%edi", "%eax"
				);
		#elif defined(CRYPTOPP_GENERATE_X64_MASM)
			pop rdi
			pop rsi
			ret
			GCM_AuthenticateBlocks_64K ENDP
		#endif

		return len%16;
		}
#endif
#ifndef CRYPTOPP_GENERATE_X64_MASM
	}

	return len%16;
}

void GCM_Base::AuthenticateLastHeaderBlock()
{
	if (m_bufferedDataLength > 0)
	{
		memset(m_buffer+m_bufferedDataLength, 0, HASH_BLOCKSIZE-m_bufferedDataLength);
		m_bufferedDataLength = 0;
		GCM_Base::AuthenticateBlocks(m_buffer, HASH_BLOCKSIZE);
	}
}

void GCM_Base::AuthenticateLastConfidentialBlock()
{
	GCM_Base::AuthenticateLastHeaderBlock();
	PutBlock<word64, BigEndian, true>(NULL, m_buffer)(m_totalHeaderLength*8)(m_totalMessageLength*8);
	GCM_Base::AuthenticateBlocks(m_buffer, HASH_BLOCKSIZE);
}

void GCM_Base::AuthenticateLastFooterBlock(byte *mac, size_t macSize)
{
	m_ctr.Seek(0);
	ReverseHashBufferIfNeeded();
	m_ctr.ProcessData(mac, HashBuffer(), macSize);
}

NAMESPACE_END

#endif	// #ifndef CRYPTOPP_GENERATE_X64_MASM
#endif
