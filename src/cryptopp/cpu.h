// cpu.h - written and placed in the public domain by Wei Dai

//! \file cpu.h
//! \brief Functions for CPU features and intrinsics
//! \details The functions are used in X86/X32/X64 and NEON code paths

#ifndef CRYPTOPP_CPU_H
#define CRYPTOPP_CPU_H

#include "config.h"

// ARM32/ARM64 Headers
#if (CRYPTOPP_BOOL_ARM32 || CRYPTOPP_BOOL_ARM64)
# if defined(__GNUC__)
#  include <stdint.h>
# endif
# if CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE || defined(__ARM_NEON)
#  include <arm_neon.h>
# endif
# if (CRYPTOPP_BOOL_ARM_CRYPTO_INTRINSICS_AVAILABLE || CRYPTOPP_BOOL_ARM_CRC32_INTRINSICS_AVAILABLE) || defined(__ARM_ACLE)
#  include <arm_acle.h>
# endif
#endif  // ARM32 and ARM64 Headers

// X86/X64/X32 Headers
#if CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64

// GCC X86 super-include
#if (CRYPTOPP_GCC_VERSION >= 40800)
#  include <x86intrin.h>
#endif
#if (CRYPTOPP_MSC_VERSION >= 1400)
#  include <intrin.h>
#endif

// Baseline include
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE || CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE
#  include <emmintrin.h>    // __m64, __m128i, _mm_set_epi64x
#endif
#if CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE
#  include <tmmintrin.h>    // _mm_shuffle_pi8, _mm_shuffle_epi8
#endif // tmmintrin.h
#if CRYPTOPP_BOOL_SSE4_INTRINSICS_AVAILABLE
#  include <smmintrin.h>    // _mm_blend_epi16
#  include <nmmintrin.h>    // _mm_crc32_u{8|16|32}
#endif // smmintrin.h
#if CRYPTOPP_BOOL_AESNI_INTRINSICS_AVAILABLE
#  include <wmmintrin.h>    // aesenc, aesdec, etc
#endif // wmmintrin.h
#if CRYPTOPP_BOOL_AVX_INTRINSICS_AVAILABLE
#  include <immintrin.h>    // RDRAND, RDSEED and AVX
#endif
#if CRYPTOPP_BOOL_AVX2_INTRINSICS_AVAILABLE
#  include <zmmintrin.h>    // AVX 512-bit extensions
#endif
#endif  // X86/X64/X32 Headers

// Applies to both X86/X32/X64 and ARM32/ARM64. And we've got MIPS devices on the way.
#if defined(_MSC_VER) || defined(__BORLANDC__)
# define CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY
#else
# define CRYPTOPP_GNU_STYLE_INLINE_ASSEMBLY
#endif

// Applies to both X86/X32/X64 and ARM32/ARM64
#if defined(CRYPTOPP_LLVM_CLANG_VERSION) || defined(CRYPTOPP_APPLE_CLANG_VERSION) || defined(CRYPTOPP_CLANG_INTEGRATED_ASSEMBLER)
	#define NEW_LINE "\n"
	#define INTEL_PREFIX ".intel_syntax;"
	#define INTEL_NOPREFIX ".intel_syntax;"
	#define ATT_PREFIX ".att_syntax;"
	#define ATT_NOPREFIX ".att_syntax;"
#elif defined(__GNUC__)
	#define NEW_LINE
	#define INTEL_PREFIX ".intel_syntax prefix;"
	#define INTEL_NOPREFIX ".intel_syntax noprefix;"
	#define ATT_PREFIX ".att_syntax prefix;"
	#define ATT_NOPREFIX ".att_syntax noprefix;"
#else
	#define NEW_LINE
	#define INTEL_PREFIX
	#define INTEL_NOPREFIX
	#define ATT_PREFIX
	#define ATT_NOPREFIX
#endif

#ifdef CRYPTOPP_GENERATE_X64_MASM

#define CRYPTOPP_X86_ASM_AVAILABLE
#define CRYPTOPP_BOOL_X64 1
#define CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE 1
#define NAMESPACE_END

#else

NAMESPACE_BEGIN(CryptoPP)

#if CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64 || CRYPTOPP_DOXYGEN_PROCESSING

#define CRYPTOPP_CPUID_AVAILABLE

// Hide from Doxygen
#ifndef CRYPTOPP_DOXYGEN_PROCESSING
// These should not be used directly
extern CRYPTOPP_DLL bool g_x86DetectionDone;
extern CRYPTOPP_DLL bool g_hasMMX;
extern CRYPTOPP_DLL bool g_hasISSE;
extern CRYPTOPP_DLL bool g_hasSSE2;
extern CRYPTOPP_DLL bool g_hasSSSE3;
extern CRYPTOPP_DLL bool g_hasSSE4;
extern CRYPTOPP_DLL bool g_hasAESNI;
extern CRYPTOPP_DLL bool g_hasCLMUL;
extern CRYPTOPP_DLL bool g_isP4;
extern CRYPTOPP_DLL bool g_hasRDRAND;
extern CRYPTOPP_DLL bool g_hasRDSEED;
extern CRYPTOPP_DLL bool g_hasPadlockRNG;
extern CRYPTOPP_DLL bool g_hasPadlockACE;
extern CRYPTOPP_DLL bool g_hasPadlockACE2;
extern CRYPTOPP_DLL bool g_hasPadlockPHE;
extern CRYPTOPP_DLL bool g_hasPadlockPMM;
extern CRYPTOPP_DLL word32 g_cacheLineSize;

CRYPTOPP_DLL void CRYPTOPP_API DetectX86Features();
CRYPTOPP_DLL bool CRYPTOPP_API CpuId(word32 input, word32 output[4]);
#endif // CRYPTOPP_DOXYGEN_PROCESSING

//! \brief Determines MMX availability
//! \returns true if MMX is determined to be available, false otherwise
//! \details MMX, SSE and SSE2 are core processor features for x86_64, and
//!   the function always returns true for the platform.
inline bool HasMMX()
{
#if CRYPTOPP_BOOL_X64
	return true;
#else
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasMMX;
#endif
}

//! \brief Determines SSE availability
//! \returns true if SSE is determined to be available, false otherwise
//! \details MMX, SSE and SSE2 are core processor features for x86_64, and
//!   the function always returns true for the platform.
inline bool HasISSE()
{
#if CRYPTOPP_BOOL_X64
	return true;
#else
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasISSE;
#endif
}

//! \brief Determines SSE2 availability
//! \returns true if SSE2 is determined to be available, false otherwise
//! \details MMX, SSE and SSE2 are core processor features for x86_64, and
//!   the function always returns true for the platform.
inline bool HasSSE2()
{
#if CRYPTOPP_BOOL_X64
	return true;
#else
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasSSE2;
#endif
}

//! \brief Determines SSSE3 availability
//! \returns true if SSSE3 is determined to be available, false otherwise
//! \details HasSSSE3() is a runtime check performed using CPUID
//! \note Some Clang compilers incorrectly omit SSSE3 even though its native to the processor.
inline bool HasSSSE3()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasSSSE3;
}

//! \brief Determines SSE4 availability
//! \returns true if SSE4.1 and SSE4.2 are determined to be available, false otherwise
//! \details HasSSE4() is a runtime check performed using CPUID which requires both SSE4.1 and SSE4.2
inline bool HasSSE4()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasSSE4;
}

//! \brief Determines AES-NI availability
//! \returns true if AES-NI is determined to be available, false otherwise
//! \details HasAESNI() is a runtime check performed using CPUID
inline bool HasAESNI()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasAESNI;
}

//! \brief Determines Carryless Multiply availability
//! \returns true if pclmulqdq is determined to be available, false otherwise
//! \details HasCLMUL() is a runtime check performed using CPUID
inline bool HasCLMUL()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasCLMUL;
}

//! \brief Determines if the CPU is an Intel P4
//! \returns true if the CPU is a P4, false otherwise
//! \details IsP4() is a runtime check performed using CPUID
inline bool IsP4()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_isP4;
}

//! \brief Determines RDRAND availability
//! \returns true if RDRAND is determined to be available, false otherwise
//! \details HasRDRAND() is a runtime check performed using CPUID
inline bool HasRDRAND()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasRDRAND;
}

//! \brief Determines RDSEED availability
//! \returns true if RDSEED is determined to be available, false otherwise
//! \details HasRDSEED() is a runtime check performed using CPUID
inline bool HasRDSEED()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasRDSEED;
}

//! \brief Determines Padlock RNG availability
//! \returns true if VIA Padlock RNG is determined to be available, false otherwise
//! \details HasPadlockRNG() is a runtime check performed using CPUID
inline bool HasPadlockRNG()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasPadlockRNG;
}

//! \brief Determines Padlock ACE availability
//! \returns true if VIA Padlock ACE is determined to be available, false otherwise
//! \details HasPadlockACE() is a runtime check performed using CPUID
inline bool HasPadlockACE()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasPadlockACE;
}

//! \brief Determines Padlock ACE2 availability
//! \returns true if VIA Padlock ACE2 is determined to be available, false otherwise
//! \details HasPadlockACE2() is a runtime check performed using CPUID
inline bool HasPadlockACE2()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasPadlockACE2;
}

//! \brief Determines Padlock PHE availability
//! \returns true if VIA Padlock PHE is determined to be available, false otherwise
//! \details HasPadlockPHE() is a runtime check performed using CPUID
inline bool HasPadlockPHE()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasPadlockPHE;
}

//! \brief Determines Padlock PMM availability
//! \returns true if VIA Padlock PMM is determined to be available, false otherwise
//! \details HasPadlockPMM() is a runtime check performed using CPUID
inline bool HasPadlockPMM()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasPadlockPMM;
}

//! \brief Provides the cache line size
//! \returns lower bound on the size of a cache line in bytes, if available
//! \details GetCacheLineSize() returns the lower bound on the size of a cache line, if it
//!   is available. If the value is not available at runtime, then 32 is returned for a 32-bit
//!   processor and 64 is returned for a 64-bit processor.
//! \details x86/x32/x64 uses CPUID to determine the value and its usually accurate. The ARM
//!   processor equivalent is a privileged instruction, so a compile time value is returned.
inline int GetCacheLineSize()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_cacheLineSize;
}

#elif (CRYPTOPP_BOOL_ARM32 || CRYPTOPP_BOOL_ARM64)

extern bool g_ArmDetectionDone;
extern bool g_hasNEON, g_hasPMULL, g_hasCRC32, g_hasAES, g_hasSHA1, g_hasSHA2;
void CRYPTOPP_API DetectArmFeatures();

//! \brief Determine if an ARM processor has Advanced SIMD available
//! \returns true if the hardware is capable of Advanced SIMD at runtime, false otherwise.
//! \details Advanced SIMD instructions are available under Aarch64 (ARM-64) and Aarch32 (ARM-32).
//! \details Runtime support requires compile time support. When compiling with GCC, you may
//!   need to compile with <tt>-mfpu=neon</tt> (32-bit) or <tt>-march=armv8-a</tt>
//!   (64-bit). Also see ARM's <tt>__ARM_NEON</tt> preprocessor macro.
inline bool HasNEON()
{
	if (!g_ArmDetectionDone)
		DetectArmFeatures();
	return g_hasNEON;
}

//! \brief Determine if an ARM processor provides Polynomial Multiplication (long)
//! \returns true if the hardware is capable of polynomial multiplications at runtime, false otherwise.
//! \details The multiplication instructions are available under Aarch64 (ARM-64) and Aarch32 (ARM-32).
//! \details Runtime support requires compile time support. When compiling with GCC, you may
//!   need to compile with <tt>-march=armv8-a+crypto</tt>; while Apple requires
//!   <tt>-arch arm64</tt>. Also see ARM's <tt>__ARM_FEATURE_CRYPTO</tt> preprocessor macro.
inline bool HasPMULL()
{
	if (!g_ArmDetectionDone)
		DetectArmFeatures();
	return g_hasPMULL;
}

//! \brief Determine if an ARM processor has CRC32 available
//! \returns true if the hardware is capable of CRC32 at runtime, false otherwise.
//! \details CRC32 instructions provide access to the processor's CRC32 and CRC32-C intructions.
//!   They are provided by ARM C Language Extensions 2.0 (ACLE 2.0) and available under Aarch64
//!   (ARM-64) and Aarch32 (ARM-32) running on Aarch64 (i.e., an AArch32 execution environment).
//! \details Runtime support requires compile time support. When compiling with GCC, you may
//!   need to compile with <tt>-march=armv8-a+crc</tt>; while Apple requires
//!   <tt>-arch arm64</tt>. Also see ARM's <tt>__ARM_FEATURE_CRC32</tt> preprocessor macro.
inline bool HasCRC32()
{
	if (!g_ArmDetectionDone)
		DetectArmFeatures();
	return g_hasCRC32;
}

//! \brief Determine if an ARM processor has AES available
//! \returns true if the hardware is capable of AES at runtime, false otherwise.
//! \details AES is part of the Crypto extensions from ARM C Language Extensions 2.0 (ACLE 2.0)
//!   and available under Aarch64 (ARM-64) and Aarch32 (ARM-32) running on Aarch64 (i.e., an
//!   AArch32 execution environment).
//! \details Runtime support requires compile time support. When compiling with GCC, you may
//!   need to compile with <tt>-march=armv8-a+crypto</tt>; while Apple requires
//!   <tt>-arch arm64</tt>. Also see ARM's <tt>__ARM_FEATURE_CRYPTO</tt> preprocessor macro.
inline bool HasAES()
{
	if (!g_ArmDetectionDone)
		DetectArmFeatures();
	return g_hasAES;
}

//! \brief Determine if an ARM processor has SHA1 available
//! \returns true if the hardware is capable of SHA1 at runtime, false otherwise.
//! \details SHA1 is part of the Crypto extensions from ARM C Language Extensions 2.0 (ACLE 2.0)
//!   and available under Aarch64 (ARM-64) and Aarch32 (ARM-32) running on Aarch64 (i.e., an
//!   AArch32 execution environment).
//! \details Runtime support requires compile time support. When compiling with GCC, you may
//!   need to compile with <tt>-march=armv8-a+crypto</tt>; while Apple requires
//!   <tt>-arch arm64</tt>. Also see ARM's <tt>__ARM_FEATURE_CRYPTO</tt> preprocessor macro.
inline bool HasSHA1()
{
	if (!g_ArmDetectionDone)
		DetectArmFeatures();
	return g_hasSHA1;
}

//! \brief Determine if an ARM processor has SHA2 available
//! \returns true if the hardware is capable of SHA2 at runtime, false otherwise.
//! \details SHA2 is part of the Crypto extensions from ARM C Language Extensions 2.0 (ACLE 2.0)
//!   and available under Aarch64 (ARM-64) and Aarch32 (ARM-32) running on Aarch64 (i.e., an
//!   AArch32 execution environment).
//! \details Runtime support requires compile time support. When compiling with GCC, you may
//!   need to compile with <tt>-march=armv8-a+crypto</tt>; while Apple requires
//!   <tt>-arch arm64</tt>. Also see ARM's <tt>__ARM_FEATURE_CRYPTO</tt> preprocessor macro.
inline bool HasSHA2()
{
	if (!g_ArmDetectionDone)
		DetectArmFeatures();
	return g_hasSHA2;
}

//! \brief Provides the cache line size at runtime
//! \returns true if the hardware is capable of CRC32 at runtime, false otherwise.
//! \details GetCacheLineSize() provides is an estimate using CRYPTOPP_L1_CACHE_LINE_SIZE.
//!   The runtime instructions to query the processor are privileged.
inline int GetCacheLineSize()
{
	return CRYPTOPP_L1_CACHE_LINE_SIZE;
}

#else

inline int GetCacheLineSize()
{
	return CRYPTOPP_L1_CACHE_LINE_SIZE;
}

#endif  // X86/X32/X64 and ARM

#endif

#if CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64

#ifdef CRYPTOPP_GENERATE_X64_MASM
	#define AS1(x) x*newline*
	#define AS2(x, y) x, y*newline*
	#define AS3(x, y, z) x, y, z*newline*
	#define ASS(x, y, a, b, c, d) x, y, a*64+b*16+c*4+d*newline*
	#define ASL(x) label##x:*newline*
	#define ASJ(x, y, z) x label##y*newline*
	#define ASC(x, y) x label##y*newline*
	#define AS_HEX(y) 0##y##h
#elif defined(_MSC_VER) || defined(__BORLANDC__)
	#define CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY
	#define AS1(x) __asm {x}
	#define AS2(x, y) __asm {x, y}
	#define AS3(x, y, z) __asm {x, y, z}
	#define ASS(x, y, a, b, c, d) __asm {x, y, (a)*64+(b)*16+(c)*4+(d)}
	#define ASL(x) __asm {label##x:}
	#define ASJ(x, y, z) __asm {x label##y}
	#define ASC(x, y) __asm {x label##y}
	#define CRYPTOPP_NAKED __declspec(naked)
	#define AS_HEX(y) 0x##y
#else
	#define CRYPTOPP_GNU_STYLE_INLINE_ASSEMBLY

	// define these in two steps to allow arguments to be expanded
	#define GNU_AS1(x) #x ";" NEW_LINE
	#define GNU_AS2(x, y) #x ", " #y ";" NEW_LINE
	#define GNU_AS3(x, y, z) #x ", " #y ", " #z ";" NEW_LINE
	#define GNU_ASL(x) "\n" #x ":" NEW_LINE
	#define GNU_ASJ(x, y, z) #x " " #y #z ";" NEW_LINE
	#define AS1(x) GNU_AS1(x)
	#define AS2(x, y) GNU_AS2(x, y)
	#define AS3(x, y, z) GNU_AS3(x, y, z)
	#define ASS(x, y, a, b, c, d) #x ", " #y ", " #a "*64+" #b "*16+" #c "*4+" #d ";"
	#define ASL(x) GNU_ASL(x)
	#define ASJ(x, y, z) GNU_ASJ(x, y, z)
	#define ASC(x, y) #x " " #y ";"
	#define CRYPTOPP_NAKED
	#define AS_HEX(y) 0x##y
#endif

#define IF0(y)
#define IF1(y) y

#ifdef CRYPTOPP_GENERATE_X64_MASM
#define ASM_MOD(x, y) ((x) MOD (y))
#define XMMWORD_PTR XMMWORD PTR
#else
// GNU assembler doesn't seem to have mod operator
#define ASM_MOD(x, y) ((x)-((x)/(y))*(y))
// GAS 2.15 doesn't support XMMWORD PTR. it seems necessary only for MASM
#define XMMWORD_PTR
#endif

#if CRYPTOPP_BOOL_X86
	#define AS_REG_1 ecx
	#define AS_REG_2 edx
	#define AS_REG_3 esi
	#define AS_REG_4 edi
	#define AS_REG_5 eax
	#define AS_REG_6 ebx
	#define AS_REG_7 ebp
	#define AS_REG_1d ecx
	#define AS_REG_2d edx
	#define AS_REG_3d esi
	#define AS_REG_4d edi
	#define AS_REG_5d eax
	#define AS_REG_6d ebx
	#define AS_REG_7d ebp
	#define WORD_SZ 4
	#define WORD_REG(x)	e##x
	#define WORD_PTR DWORD PTR
	#define AS_PUSH_IF86(x) AS1(push e##x)
	#define AS_POP_IF86(x) AS1(pop e##x)
	#define AS_JCXZ jecxz
#elif CRYPTOPP_BOOL_X32
	#define AS_REG_1 ecx
	#define AS_REG_2 edx
	#define AS_REG_3 r8d
	#define AS_REG_4 r9d
	#define AS_REG_5 eax
	#define AS_REG_6 r10d
	#define AS_REG_7 r11d
	#define AS_REG_1d ecx
	#define AS_REG_2d edx
	#define AS_REG_3d r8d
	#define AS_REG_4d r9d
	#define AS_REG_5d eax
	#define AS_REG_6d r10d
	#define AS_REG_7d r11d
	#define WORD_SZ 4
	#define WORD_REG(x)	e##x
	#define WORD_PTR DWORD PTR
	#define AS_PUSH_IF86(x) AS1(push r##x)
	#define AS_POP_IF86(x) AS1(pop r##x)
	#define AS_JCXZ jecxz
#elif CRYPTOPP_BOOL_X64
	#ifdef CRYPTOPP_GENERATE_X64_MASM
		#define AS_REG_1 rcx
		#define AS_REG_2 rdx
		#define AS_REG_3 r8
		#define AS_REG_4 r9
		#define AS_REG_5 rax
		#define AS_REG_6 r10
		#define AS_REG_7 r11
		#define AS_REG_1d ecx
		#define AS_REG_2d edx
		#define AS_REG_3d r8d
		#define AS_REG_4d r9d
		#define AS_REG_5d eax
		#define AS_REG_6d r10d
		#define AS_REG_7d r11d
	#else
		#define AS_REG_1 rdi
		#define AS_REG_2 rsi
		#define AS_REG_3 rdx
		#define AS_REG_4 rcx
		#define AS_REG_5 r8
		#define AS_REG_6 r9
		#define AS_REG_7 r10
		#define AS_REG_1d edi
		#define AS_REG_2d esi
		#define AS_REG_3d edx
		#define AS_REG_4d ecx
		#define AS_REG_5d r8d
		#define AS_REG_6d r9d
		#define AS_REG_7d r10d
	#endif
	#define WORD_SZ 8
	#define WORD_REG(x)	r##x
	#define WORD_PTR QWORD PTR
	#define AS_PUSH_IF86(x)
	#define AS_POP_IF86(x)
	#define AS_JCXZ jrcxz
#endif

// helper macro for stream cipher output
#define AS_XMM_OUTPUT4(labelPrefix, inputPtr, outputPtr, x0, x1, x2, x3, t, p0, p1, p2, p3, increment)\
	AS2(	test	inputPtr, inputPtr)\
	ASC(	jz,		labelPrefix##3)\
	AS2(	test	inputPtr, 15)\
	ASC(	jnz,	labelPrefix##7)\
	AS2(	pxor	xmm##x0, [inputPtr+p0*16])\
	AS2(	pxor	xmm##x1, [inputPtr+p1*16])\
	AS2(	pxor	xmm##x2, [inputPtr+p2*16])\
	AS2(	pxor	xmm##x3, [inputPtr+p3*16])\
	AS2(	add		inputPtr, increment*16)\
	ASC(	jmp,	labelPrefix##3)\
	ASL(labelPrefix##7)\
	AS2(	movdqu	xmm##t, [inputPtr+p0*16])\
	AS2(	pxor	xmm##x0, xmm##t)\
	AS2(	movdqu	xmm##t, [inputPtr+p1*16])\
	AS2(	pxor	xmm##x1, xmm##t)\
	AS2(	movdqu	xmm##t, [inputPtr+p2*16])\
	AS2(	pxor	xmm##x2, xmm##t)\
	AS2(	movdqu	xmm##t, [inputPtr+p3*16])\
	AS2(	pxor	xmm##x3, xmm##t)\
	AS2(	add		inputPtr, increment*16)\
	ASL(labelPrefix##3)\
	AS2(	test	outputPtr, 15)\
	ASC(	jnz,	labelPrefix##8)\
	AS2(	movdqa	[outputPtr+p0*16], xmm##x0)\
	AS2(	movdqa	[outputPtr+p1*16], xmm##x1)\
	AS2(	movdqa	[outputPtr+p2*16], xmm##x2)\
	AS2(	movdqa	[outputPtr+p3*16], xmm##x3)\
	ASC(	jmp,	labelPrefix##9)\
	ASL(labelPrefix##8)\
	AS2(	movdqu	[outputPtr+p0*16], xmm##x0)\
	AS2(	movdqu	[outputPtr+p1*16], xmm##x1)\
	AS2(	movdqu	[outputPtr+p2*16], xmm##x2)\
	AS2(	movdqu	[outputPtr+p3*16], xmm##x3)\
	ASL(labelPrefix##9)\
	AS2(	add		outputPtr, increment*16)

#endif  //  X86/X32/X64

NAMESPACE_END

#endif  // CRYPTOPP_CPU_H
