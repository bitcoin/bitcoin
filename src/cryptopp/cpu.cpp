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

#ifdef CRYPTOPP_CPUID_AVAILABLE

#if _MSC_VER >= 1400 && CRYPTOPP_BOOL_X64

bool CpuId(word32 input, word32 output[4])
{
	__cpuid((int *)output, input);
	return true;
}

#else

#ifndef CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY
extern "C" {
typedef void (*SigHandler)(int);

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
	// http://github.com/weidai11/cryptopp/issues/24
	// http://stackoverflow.com/q/7721854
	volatile bool result = true;

	SigHandler oldHandler = signal(SIGILL, SigIllHandlerCPUID);
	if (oldHandler == SIG_ERR)
		result = false;

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
	// http://github.com/weidai11/cryptopp/issues/24
	// http://stackoverflow.com/q/7721854
	volatile bool result = true;

	SigHandler oldHandler = signal(SIGILL, SigIllHandlerSSE2);
	if (oldHandler == SIG_ERR)
		return false;

	if (setjmp(s_jmpNoSSE2))
		result = true;
	else
	{
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
		__asm __volatile ("por %xmm0, %xmm0");
#elif CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE
		__m128i x = _mm_setzero_si128();
		result = _mm_cvtsi128_si32(x) == 0;
#endif
	}

	signal(SIGILL, oldHandler);
	return result;
#endif
}

bool g_x86DetectionDone = false;
bool g_hasMMX = false, g_hasISSE = false, g_hasSSE2 = false, g_hasSSSE3 = false, g_hasAESNI = false, g_hasCLMUL = false, g_isP4 = false, g_hasRDRAND = false, g_hasRDSEED = false;
word32 g_cacheLineSize = CRYPTOPP_L1_CACHE_LINE_SIZE;

// MacPorts/GCC does not provide constructor(priority). Apple/GCC and Fink/GCC do provide it.
#define HAVE_GCC_CONSTRUCTOR1 (__GNUC__ && (CRYPTOPP_INIT_PRIORITY > 0) && ((CRYPTOPP_GCC_VERSION >= 40300) || (CRYPTOPP_CLANG_VERSION >= 20900) || (_INTEL_COMPILER >= 300)) && !(MACPORTS_GCC_COMPILER > 0))
#define HAVE_GCC_CONSTRUCTOR0 (__GNUC__ && (CRYPTOPP_INIT_PRIORITY > 0) && !(MACPORTS_GCC_COMPILER > 0))

static inline bool IsIntel(const word32 output[4])
{
	// This is the "GenuineIntel" string
	return (output[1] /*EBX*/ == 0x756e6547) &&
		(output[2] /*ECX*/ == 0x6c65746e) &&
		(output[3] /*EDX*/ == 0x49656e69);
}

static inline bool IsAMD(const word32 output[4])
{
	// This is the "AuthenticAMD" string
	return (output[1] /*EBX*/ == 0x68747541) &&
		(output[2] /*ECX*/ == 0x69746E65) &&
		(output[3] /*EDX*/ == 0x444D4163);
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

	static const unsigned int RDRAND_FLAG = (1 << 30);
	static const unsigned int RDSEED_FLAG = (1 << 18);
	if (IsIntel(cpuid))
	{
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
		CpuId(0x80000005, cpuid);
		g_cacheLineSize = GETBYTE(cpuid[2], 0);
		g_hasRDRAND = !!(cpuid[2] /*ECX*/ & RDRAND_FLAG);
	}

	if (!g_cacheLineSize)
		g_cacheLineSize = CRYPTOPP_L1_CACHE_LINE_SIZE;

	*((volatile bool*)&g_x86DetectionDone) = true;
}

#endif

NAMESPACE_END

#endif
