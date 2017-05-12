// cpu.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "config.h"

#ifndef EXCEPTION_EXECUTE_HANDLER
# define EXCEPTION_EXECUTE_HANDLER 1
#endif

#ifndef CRYPTOPP_IMPORTS

#include "cpu.h"
#include "misc.h"
#include <algorithm>

#ifndef CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY
#include <signal.h>
#include <setjmp.h>
#endif

#if CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE
#include <emmintrin.h>
#endif

NAMESPACE_BEGIN(CryptoPP)

#ifndef CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY
extern "C" {
    typedef void (*SigHandler)(int);
};
#endif  // Not CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY

#ifdef CRYPTOPP_CPUID_AVAILABLE

#if _MSC_VER >= 1400 && CRYPTOPP_BOOL_X64

bool CpuId(word32 input, word32 output[4])
{
	__cpuid((int *)output, input);
	return true;
}

#else

#ifndef CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY
extern "C"
{
	static jmp_buf s_jmpNoCPUID;
	static void SigIllHandlerCPUID(int)
	{
		longjmp(s_jmpNoCPUID, 1);
	}

	static jmp_buf s_jmpNoSSE2;
	static void SigIllHandlerSSE2(int)
	{
		longjmp(s_jmpNoSSE2, 1);
	}
}
#endif

bool CpuId(word32 input, word32 output[4])
{
#if defined(CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY)
    __try
	{
		__asm
		{
			mov eax, input
			mov ecx, 0
			cpuid
			mov edi, output
			mov [edi], eax
			mov [edi+4], ebx
			mov [edi+8], ecx
			mov [edi+12], edx
		}
	}
	// GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

	// function 0 returns the highest basic function understood in EAX
	if(input == 0)
		return !!output[0];

	return true;
#else
	// longjmp and clobber warnings. Volatile is required.
	// http://github.com/weidai11/cryptopp/issues/24 and http://stackoverflow.com/q/7721854
	volatile bool result = true;

	volatile SigHandler oldHandler = signal(SIGILL, SigIllHandlerCPUID);
	if (oldHandler == SIG_ERR)
		return false;

# ifndef __MINGW32__
	volatile sigset_t oldMask;
	if (sigprocmask(0, NULL, (sigset_t*)&oldMask))
		return false;
# endif

	if (setjmp(s_jmpNoCPUID))
		result = false;
	else
	{
		asm volatile
		(
			// save ebx in case -fPIC is being used
			// TODO: this might need an early clobber on EDI.
# if CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64
			"pushq %%rbx; cpuid; mov %%ebx, %%edi; popq %%rbx"
# else
			"push %%ebx; cpuid; mov %%ebx, %%edi; pop %%ebx"
# endif
			: "=a" (output[0]), "=D" (output[1]), "=c" (output[2]), "=d" (output[3])
			: "a" (input), "c" (0)
		);
	}

# ifndef __MINGW32__
	sigprocmask(SIG_SETMASK, (sigset_t*)&oldMask, NULL);
# endif

	signal(SIGILL, oldHandler);
	return result;
#endif
}

#endif

static bool TrySSE2()
{
#if CRYPTOPP_BOOL_X64
	return true;
#elif defined(CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY)
    __try
	{
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
		AS2(por xmm0, xmm0)        // executing SSE2 instruction
#elif CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE
		__m128i x = _mm_setzero_si128();
		return _mm_cvtsi128_si32(x) == 0;
#endif
	}
	// GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
	return true;
#else
	// longjmp and clobber warnings. Volatile is required.
	// http://github.com/weidai11/cryptopp/issues/24 and http://stackoverflow.com/q/7721854
	volatile bool result = true;

	volatile SigHandler oldHandler = signal(SIGILL, SigIllHandlerSSE2);
	if (oldHandler == SIG_ERR)
		return false;

# ifndef __MINGW32__
	volatile sigset_t oldMask;
	if (sigprocmask(0, NULL, (sigset_t*)&oldMask))
		return false;
# endif

	if (setjmp(s_jmpNoSSE2))
		result = false;
	else
	{
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
		__asm __volatile ("por %xmm0, %xmm0");
#elif CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE
		__m128i x = _mm_setzero_si128();
		result = _mm_cvtsi128_si32(x) == 0;
#endif
	}

# ifndef __MINGW32__
	sigprocmask(SIG_SETMASK, (sigset_t*)&oldMask, NULL);
# endif

	signal(SIGILL, oldHandler);
	return result;
#endif
}

bool CRYPTOPP_SECTION_INIT g_x86DetectionDone = false;
bool CRYPTOPP_SECTION_INIT g_hasMMX = false, CRYPTOPP_SECTION_INIT g_hasISSE = false, CRYPTOPP_SECTION_INIT g_hasSSE2 = false, CRYPTOPP_SECTION_INIT g_hasSSSE3 = false;
bool CRYPTOPP_SECTION_INIT g_hasSSE4 = false, CRYPTOPP_SECTION_INIT g_hasAESNI = false, CRYPTOPP_SECTION_INIT g_hasCLMUL = false, CRYPTOPP_SECTION_INIT g_isP4 = false;
bool CRYPTOPP_SECTION_INIT g_hasRDRAND = false, CRYPTOPP_SECTION_INIT g_hasRDSEED = false;
bool CRYPTOPP_SECTION_INIT g_hasPadlockRNG = false, CRYPTOPP_SECTION_INIT g_hasPadlockACE = false, CRYPTOPP_SECTION_INIT g_hasPadlockACE2 = false;
bool CRYPTOPP_SECTION_INIT g_hasPadlockPHE = false, CRYPTOPP_SECTION_INIT g_hasPadlockPMM = false;
word32 CRYPTOPP_SECTION_INIT g_cacheLineSize = CRYPTOPP_L1_CACHE_LINE_SIZE;

static inline bool IsIntel(const word32 output[4])
{
	// This is the "GenuineIntel" string
	return (output[1] /*EBX*/ == 0x756e6547) &&
		(output[2] /*ECX*/ == 0x6c65746e) &&
		(output[3] /*EDX*/ == 0x49656e69);
}

static inline bool IsAMD(const word32 output[4])
{
	// This is the "AuthenticAMD" string. Some early K5's can return "AMDisbetter!"
	return (output[1] /*EBX*/ == 0x68747541) &&
		(output[2] /*ECX*/ == 0x444D4163) &&
		(output[3] /*EDX*/ == 0x69746E65);
}

static inline bool IsVIA(const word32 output[4])
{
	// This is the "CentaurHauls" string. Some non-PadLock's can return "VIA VIA VIA "
	return (output[1] /*EBX*/ == 0x746e6543) &&
		(output[2] /*ECX*/ == 0x736c7561) &&
		(output[3] /*EDX*/ == 0x48727561);
}

#if HAVE_GCC_CONSTRUCTOR1
void __attribute__ ((constructor (CRYPTOPP_INIT_PRIORITY + 50))) DetectX86Features()
#elif HAVE_GCC_CONSTRUCTOR0
void __attribute__ ((constructor)) DetectX86Features()
#else
void DetectX86Features()
#endif
{
	word32 cpuid[4], cpuid1[4];
	if (!CpuId(0, cpuid))
		return;
	if (!CpuId(1, cpuid1))
		return;

	g_hasMMX = (cpuid1[3] & (1 << 23)) != 0;
	if ((cpuid1[3] & (1 << 26)) != 0)
		g_hasSSE2 = TrySSE2();
	g_hasSSSE3 = g_hasSSE2 && (cpuid1[2] & (1<<9));
	g_hasSSE4 = g_hasSSE2 && ((cpuid1[2] & (1<<19)) && (cpuid1[2] & (1<<20)));
	g_hasAESNI = g_hasSSE2 && (cpuid1[2] & (1<<25));
	g_hasCLMUL = g_hasSSE2 && (cpuid1[2] & (1<<1));

	if ((cpuid1[3] & (1 << 25)) != 0)
		g_hasISSE = true;
	else
	{
		word32 cpuid2[4];
		CpuId(0x080000000, cpuid2);
		if (cpuid2[0] >= 0x080000001)
		{
			CpuId(0x080000001, cpuid2);
			g_hasISSE = (cpuid2[3] & (1 << 22)) != 0;
		}
	}

	if (IsIntel(cpuid))
	{
		static const unsigned int RDRAND_FLAG = (1 << 30);
		static const unsigned int RDSEED_FLAG = (1 << 18);

		g_isP4 = ((cpuid1[0] >> 8) & 0xf) == 0xf;
		g_cacheLineSize = 8 * GETBYTE(cpuid1[1], 1);
		g_hasRDRAND = !!(cpuid1[2] /*ECX*/ & RDRAND_FLAG);

		if (cpuid[0] /*EAX*/ >= 7)
		{
			word32 cpuid3[4];
			if (CpuId(7, cpuid3))
				g_hasRDSEED = !!(cpuid3[1] /*EBX*/ & RDSEED_FLAG);
		}
	}
	else if (IsAMD(cpuid))
	{
		static const unsigned int RDRAND_FLAG = (1 << 30);

		CpuId(0x01, cpuid);
		g_hasRDRAND = !!(cpuid[2] /*ECX*/ & RDRAND_FLAG);

		CpuId(0x80000005, cpuid);
		g_cacheLineSize = GETBYTE(cpuid[2], 0);
	}
	else if (IsVIA(cpuid))
	{
		static const unsigned int  RNG_FLAGS = (0x3 << 2);
		static const unsigned int  ACE_FLAGS = (0x3 << 6);
		static const unsigned int ACE2_FLAGS = (0x3 << 8);
		static const unsigned int  PHE_FLAGS = (0x3 << 10);
		static const unsigned int  PMM_FLAGS = (0x3 << 12);

		CpuId(0xC0000000, cpuid);
		if (cpuid[0] >= 0xC0000001)
		{
			// Extended features available
			CpuId(0xC0000001, cpuid);
			g_hasPadlockRNG  = !!(cpuid[3] /*EDX*/ & RNG_FLAGS);
			g_hasPadlockACE  = !!(cpuid[3] /*EDX*/ & ACE_FLAGS);
			g_hasPadlockACE2 = !!(cpuid[3] /*EDX*/ & ACE2_FLAGS);
			g_hasPadlockPHE  = !!(cpuid[3] /*EDX*/ & PHE_FLAGS);
			g_hasPadlockPMM  = !!(cpuid[3] /*EDX*/ & PMM_FLAGS);
		}
	}

	if (!g_cacheLineSize)
		g_cacheLineSize = CRYPTOPP_L1_CACHE_LINE_SIZE;

	*((volatile bool*)&g_x86DetectionDone) = true;
}

#elif (CRYPTOPP_BOOL_ARM32 || CRYPTOPP_BOOL_ARM64)

// The ARM equivalent of CPUID probing is reading a MSR. The code requires Exception Level 1 (EL1) and above, but user space runs at EL0.
//   Attempting to run the code results in a SIGILL and termination.
//
//     #if defined(__arm64__) || defined(__aarch64__)
//	     word64 caps = 0;  // Read ID_AA64ISAR0_EL1
//	     __asm __volatile("mrs %0, " "id_aa64isar0_el1" : "=r" (caps));
//     #elif defined(__arm__) || defined(__aarch32__)
//	     word32 caps = 0;  // Read ID_ISAR5_EL1
//	     __asm __volatile("mrs %0, " "id_isar5_el1" : "=r" (caps));
//     #endif
//
// The following does not work well either. Its appears to be missing constants, and it does not detect Aarch32 execution environments on Aarch64
// http://community.arm.com/groups/android-community/blog/2014/10/10/runtime-detection-of-cpu-features-on-an-armv8-a-cpu
//
bool CRYPTOPP_SECTION_INIT g_ArmDetectionDone = false;
bool CRYPTOPP_SECTION_INIT g_hasNEON = false, CRYPTOPP_SECTION_INIT g_hasPMULL = false, CRYPTOPP_SECTION_INIT g_hasCRC32 = false;
bool CRYPTOPP_SECTION_INIT g_hasAES = false, CRYPTOPP_SECTION_INIT g_hasSHA1 = false, CRYPTOPP_SECTION_INIT g_hasSHA2 = false;
word32 CRYPTOPP_SECTION_INIT g_cacheLineSize = CRYPTOPP_L1_CACHE_LINE_SIZE;

#ifndef CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY
extern "C"
{
	static jmp_buf s_jmpNoNEON;
	static void SigIllHandlerNEON(int)
	{
		longjmp(s_jmpNoNEON, 1);
	}

	static jmp_buf s_jmpNoPMULL;
	static void SigIllHandlerPMULL(int)
	{
		longjmp(s_jmpNoPMULL, 1);
	}

	static jmp_buf s_jmpNoCRC32;
	static void SigIllHandlerCRC32(int)
	{
		longjmp(s_jmpNoCRC32, 1);
	}

	static jmp_buf s_jmpNoAES;
	static void SigIllHandlerAES(int)
	{
		longjmp(s_jmpNoAES, 1);
	}

	static jmp_buf s_jmpNoSHA1;
	static void SigIllHandlerSHA1(int)
	{
		longjmp(s_jmpNoSHA1, 1);
	}

	static jmp_buf s_jmpNoSHA2;
	static void SigIllHandlerSHA2(int)
	{
		longjmp(s_jmpNoSHA2, 1);
	}
};
#endif  // Not CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY

static bool TryNEON()
{
#if (CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE)
# if defined(CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY)
	volatile bool result = true;
	__try
	{
		uint32_t v1[4] = {1,1,1,1};
		uint32x4_t x1 = vld1q_u32(v1);
		uint64_t v2[2] = {1,1};
		uint64x2_t x2 = vld1q_u64(v2);

		uint32x4_t x3 = vdupq_n_u32(2);
		x3 = vsetq_lane_u32(vgetq_lane_u32(x1,0),x3,0);
		x3 = vsetq_lane_u32(vgetq_lane_u32(x1,3),x3,3);
		uint64x2_t x4 = vdupq_n_u64(2);
		x4 = vsetq_lane_u64(vgetq_lane_u64(x2,0),x4,0);
		x4 = vsetq_lane_u64(vgetq_lane_u64(x2,1),x4,1);

		result = !!(vgetq_lane_u32(x3,0) | vgetq_lane_u64(x4,1));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
	return result;
# else
	// longjmp and clobber warnings. Volatile is required.
	// http://github.com/weidai11/cryptopp/issues/24 and http://stackoverflow.com/q/7721854
	volatile bool result = true;

	volatile SigHandler oldHandler = signal(SIGILL, SigIllHandlerNEON);
	if (oldHandler == SIG_ERR)
		return false;

	volatile sigset_t oldMask;
	if (sigprocmask(0, NULL, (sigset_t*)&oldMask))
		return false;

	if (setjmp(s_jmpNoNEON))
		result = false;
	else
	{
		uint32_t v1[4] = {1,1,1,1};
		uint32x4_t x1 = vld1q_u32(v1);
		uint64_t v2[2] = {1,1};
		uint64x2_t x2 = vld1q_u64(v2);

		uint32x4_t x3 = {0,0,0,0};
		x3 = vsetq_lane_u32(vgetq_lane_u32(x1,0),x3,0);
		x3 = vsetq_lane_u32(vgetq_lane_u32(x1,3),x3,3);
		uint64x2_t x4 = {0,0};
		x4 = vsetq_lane_u64(vgetq_lane_u64(x2,0),x4,0);
		x4 = vsetq_lane_u64(vgetq_lane_u64(x2,1),x4,1);

		// Hack... GCC optimizes away the code and returns true
		result = !!(vgetq_lane_u32(x3,0) | vgetq_lane_u64(x4,1));
	}

	sigprocmask(SIG_SETMASK, (sigset_t*)&oldMask, NULL);
	signal(SIGILL, oldHandler);
	return result;
# endif
#else
	return false;
#endif  // CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE
}

static bool TryPMULL()
{
#if (CRYPTOPP_BOOL_ARM_CRYPTO_INTRINSICS_AVAILABLE)
# if defined(CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY)
	volatile bool result = true;
	__try
	{
		const poly64_t a1={2}, b1={3};
		const poly64x2_t a2={4,5}, b2={6,7};
		const poly64x2_t a3={0x8080808080808080,0xa0a0a0a0a0a0a0a0}, b3={0xc0c0c0c0c0c0c0c0, 0xe0e0e0e0e0e0e0e0};

		const poly128_t r1 = vmull_p64(a1, b1);
		const poly128_t r2 = vmull_high_p64(a2, b2);
		const poly128_t r3 = vmull_high_p64(a3, b3);

		// Also see https://github.com/weidai11/cryptopp/issues/233.
		const uint64x2_t& t1 = vreinterpretq_u64_p128(r1);  // {6,0}
		const uint64x2_t& t2 = vreinterpretq_u64_p128(r2);  // {24,0}
		const uint64x2_t& t3 = vreinterpretq_u64_p128(r3);  // {bignum,bignum}

		result = !!(vgetq_lane_u64(t1,0) == 0x06 && vgetq_lane_u64(t1,1) == 0x00 && vgetq_lane_u64(t2,0) == 0x1b &&
			vgetq_lane_u64(t2,1) == 0x00 &&	vgetq_lane_u64(t3,0) == 0x6c006c006c006c00 && vgetq_lane_u64(t3,1) == 0x6c006c006c006c00);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
	return result;
# else
	// longjmp and clobber warnings. Volatile is required.
	// http://github.com/weidai11/cryptopp/issues/24 and http://stackoverflow.com/q/7721854
	volatile bool result = true;

	volatile SigHandler oldHandler = signal(SIGILL, SigIllHandlerPMULL);
	if (oldHandler == SIG_ERR)
		return false;

	volatile sigset_t oldMask;
	if (sigprocmask(0, NULL, (sigset_t*)&oldMask))
		return false;

	if (setjmp(s_jmpNoPMULL))
		result = false;
	else
	{
		const poly64_t a1={2}, b1={3};
		const poly64x2_t a2={4,5}, b2={6,7};
		const poly64x2_t a3={0x8080808080808080,0xa0a0a0a0a0a0a0a0}, b3={0xc0c0c0c0c0c0c0c0, 0xe0e0e0e0e0e0e0e0};

		const poly128_t r1 = vmull_p64(a1, b1);
		const poly128_t r2 = vmull_high_p64(a2, b2);
		const poly128_t r3 = vmull_high_p64(a3, b3);

		// Linaro is missing vreinterpretq_u64_p128. Also see https://github.com/weidai11/cryptopp/issues/233.
		const uint64x2_t& t1 = (uint64x2_t)(r1);  // {6,0}
		const uint64x2_t& t2 = (uint64x2_t)(r2);  // {24,0}
		const uint64x2_t& t3 = (uint64x2_t)(r3);  // {bignum,bignum}

		result = !!(vgetq_lane_u64(t1,0) == 0x06 && vgetq_lane_u64(t1,1) == 0x00 && vgetq_lane_u64(t2,0) == 0x1b &&
			vgetq_lane_u64(t2,1) == 0x00 &&	vgetq_lane_u64(t3,0) == 0x6c006c006c006c00 && vgetq_lane_u64(t3,1) == 0x6c006c006c006c00);
	}

	sigprocmask(SIG_SETMASK, (sigset_t*)&oldMask, NULL);
	signal(SIGILL, oldHandler);
	return result;
# endif
#else
	return false;
#endif  // CRYPTOPP_BOOL_ARM_CRYPTO_INTRINSICS_AVAILABLE
}

static bool TryCRC32()
{
#if (CRYPTOPP_BOOL_ARM_CRC32_INTRINSICS_AVAILABLE)
# if defined(CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY)
	volatile bool result = true;
	__try
	{
		word32 w=0, x=1; word16 y=2; byte z=3;
		w = __crc32cw(w,x);
		w = __crc32ch(w,y);
		w = __crc32cb(w,z);

		result = !!w;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
	return result;
# else
	// longjmp and clobber warnings. Volatile is required.
	// http://github.com/weidai11/cryptopp/issues/24 and http://stackoverflow.com/q/7721854
	volatile bool result = true;

	volatile SigHandler oldHandler = signal(SIGILL, SigIllHandlerCRC32);
	if (oldHandler == SIG_ERR)
		return false;

	volatile sigset_t oldMask;
	if (sigprocmask(0, NULL, (sigset_t*)&oldMask))
		return false;

	if (setjmp(s_jmpNoCRC32))
		result = false;
	else
	{
		word32 w=0, x=1; word16 y=2; byte z=3;
		w = __crc32cw(w,x);
		w = __crc32ch(w,y);
		w = __crc32cb(w,z);

		// Hack... GCC optimizes away the code and returns true
		result = !!w;
	}

	sigprocmask(SIG_SETMASK, (sigset_t*)&oldMask, NULL);
	signal(SIGILL, oldHandler);
	return result;
# endif
#else
	return false;
#endif  // CRYPTOPP_BOOL_ARM_CRC32_INTRINSICS_AVAILABLE
}

static bool TryAES()
{
#if (CRYPTOPP_BOOL_ARM_CRYPTO_INTRINSICS_AVAILABLE)
# if defined(CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY)
	volatile bool result = true;
	__try
	{
		// AES encrypt and decrypt
		uint8x16_t data = vdupq_n_u8(0), key = vdupq_n_u8(0);
		uint8x16_t r1 = vaeseq_u8(data, key);
		uint8x16_t r2 = vaesdq_u8(data, key);

		result = !!(vgetq_lane_u8(r1,0) | vgetq_lane_u8(r2,7));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
	return result;
# else
	// longjmp and clobber warnings. Volatile is required.
	// http://github.com/weidai11/cryptopp/issues/24 and http://stackoverflow.com/q/7721854
	volatile bool result = true;

	volatile SigHandler oldHandler = signal(SIGILL, SigIllHandlerAES);
	if (oldHandler == SIG_ERR)
		return false;

	volatile sigset_t oldMask;
	if (sigprocmask(0, NULL, (sigset_t*)&oldMask))
		return false;

	if (setjmp(s_jmpNoAES))
		result = false;
	else
	{
		uint8x16_t data = vdupq_n_u8(0), key = vdupq_n_u8(0);
		uint8x16_t r1 = vaeseq_u8(data, key);
		uint8x16_t r2 = vaesdq_u8(data, key);

		// Hack... GCC optimizes away the code and returns true
		result = !!(vgetq_lane_u8(r1,0) | vgetq_lane_u8(r2,7));
	}

	sigprocmask(SIG_SETMASK, (sigset_t*)&oldMask, NULL);
	signal(SIGILL, oldHandler);
	return result;
# endif
#else
	return false;
#endif  // CRYPTOPP_BOOL_ARM_CRYPTO_INTRINSICS_AVAILABLE
}

static bool TrySHA1()
{
#if (CRYPTOPP_BOOL_ARM_CRYPTO_INTRINSICS_AVAILABLE)
# if defined(CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY)
	volatile bool result = true;
	__try
	{
		uint32x4_t data1 = {1,2,3,4}, data2 = {5,6,7,8}, data3 = {9,10,11,12};

		uint32x4_t r1 = vsha1cq_u32 (data1, 0, data2);
		uint32x4_t r2 = vsha1mq_u32 (data1, 0, data2);
		uint32x4_t r3 = vsha1pq_u32 (data1, 0, data2);
		uint32x4_t r4 = vsha1su0q_u32 (data1, data2, data3);
		uint32x4_t r5 = vsha1su1q_u32 (data1, data2);

		result = !!(vgetq_lane_u32(r1,0) | vgetq_lane_u32(r2,1) | vgetq_lane_u32(r3,2) | vgetq_lane_u32(r4,3) | vgetq_lane_u32(r5,0));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
	return result;
# else
	// longjmp and clobber warnings. Volatile is required.
	// http://github.com/weidai11/cryptopp/issues/24 and http://stackoverflow.com/q/7721854
	volatile bool result = true;

	volatile SigHandler oldHandler = signal(SIGILL, SigIllHandlerSHA1);
	if (oldHandler == SIG_ERR)
		return false;

	volatile sigset_t oldMask;
	if (sigprocmask(0, NULL, (sigset_t*)&oldMask))
		return false;

	if (setjmp(s_jmpNoSHA1))
		result = false;
	else
	{
		uint32x4_t data1 = {1,2,3,4}, data2 = {5,6,7,8}, data3 = {9,10,11,12};

		uint32x4_t r1 = vsha1cq_u32 (data1, 0, data2);
		uint32x4_t r2 = vsha1mq_u32 (data1, 0, data2);
		uint32x4_t r3 = vsha1pq_u32 (data1, 0, data2);
		uint32x4_t r4 = vsha1su0q_u32 (data1, data2, data3);
		uint32x4_t r5 = vsha1su1q_u32 (data1, data2);

		// Hack... GCC optimizes away the code and returns true
		result = !!(vgetq_lane_u32(r1,0) | vgetq_lane_u32(r2,1) | vgetq_lane_u32(r3,2) | vgetq_lane_u32(r4,3) | vgetq_lane_u32(r5,0));
	}

	sigprocmask(SIG_SETMASK, (sigset_t*)&oldMask, NULL);
	signal(SIGILL, oldHandler);
	return result;
# endif
#else
	return false;
#endif  // CRYPTOPP_BOOL_ARM_CRYPTO_INTRINSICS_AVAILABLE
}

static bool TrySHA2()
{
#if (CRYPTOPP_BOOL_ARM_CRYPTO_INTRINSICS_AVAILABLE)
# if defined(CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY)
	volatile bool result = true;
	__try
	{
		uint32x4_t data1 = {1,2,3,4}, data2 = {5,6,7,8}, data3 = {9,10,11,12};

		uint32x4_t r1 = vsha256hq_u32 (data1, data2, data3);
		uint32x4_t r2 = vsha256h2q_u32 (data1, data2, data3);
		uint32x4_t r3 = vsha256su0q_u32 (data1, data2);
		uint32x4_t r4 = vsha256su1q_u32 (data1, data2, data3);

		result = !!(vgetq_lane_u32(r1,0) | vgetq_lane_u32(r2,1) | vgetq_lane_u32(r3,2) | vgetq_lane_u32(r4,3));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
	return result;
# else
	// longjmp and clobber warnings. Volatile is required.
	// http://github.com/weidai11/cryptopp/issues/24 and http://stackoverflow.com/q/7721854
	volatile bool result = true;

	volatile SigHandler oldHandler = signal(SIGILL, SigIllHandlerSHA2);
	if (oldHandler == SIG_ERR)
		return false;

	volatile sigset_t oldMask;
	if (sigprocmask(0, NULL, (sigset_t*)&oldMask))
		return false;

	if (setjmp(s_jmpNoSHA2))
		result = false;
	else
	{
		uint32x4_t data1 = {1,2,3,4}, data2 = {5,6,7,8}, data3 = {9,10,11,12};

		uint32x4_t r1 = vsha256hq_u32 (data1, data2, data3);
		uint32x4_t r2 = vsha256h2q_u32 (data1, data2, data3);
		uint32x4_t r3 = vsha256su0q_u32 (data1, data2);
		uint32x4_t r4 = vsha256su1q_u32 (data1, data2, data3);

		// Hack... GCC optimizes away the code and returns true
		result = !!(vgetq_lane_u32(r1,0) | vgetq_lane_u32(r2,1) | vgetq_lane_u32(r3,2) | vgetq_lane_u32(r4,3));
	}

	sigprocmask(SIG_SETMASK, (sigset_t*)&oldMask, NULL);
	signal(SIGILL, oldHandler);
	return result;
# endif
#else
	return false;
#endif  // CRYPTOPP_BOOL_ARM_CRYPTO_INTRINSICS_AVAILABLE
}

#if HAVE_GCC_CONSTRUCTOR1
void __attribute__ ((constructor (CRYPTOPP_INIT_PRIORITY + 50))) DetectArmFeatures()
#elif HAVE_GCC_CONSTRUCTOR0
void __attribute__ ((constructor)) DetectArmFeatures()
#else
void DetectArmFeatures()
#endif
{
	g_hasNEON = TryNEON();
	g_hasPMULL = TryPMULL();
	g_hasCRC32 = TryCRC32();
	g_hasAES = TryAES();
	g_hasSHA1 = TrySHA1();
	g_hasSHA2 = TrySHA2();

	*((volatile bool*)&g_ArmDetectionDone) = true;
}

#endif

NAMESPACE_END

#endif
