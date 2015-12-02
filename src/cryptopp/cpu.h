// cpu.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile cpu.h
//! \brief Classes, functions, intrinsics and features for X86, X32 nd X64 assembly

#ifndef CRYPTOPP_CPU_H
#define CRYPTOPP_CPU_H

#include "config.h"

#ifdef CRYPTOPP_GENERATE_X64_MASM

#define CRYPTOPP_X86_ASM_AVAILABLE
#define CRYPTOPP_BOOL_X64 1
#define CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE 1
#define NAMESPACE_END

#else

# if CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE
#  include <emmintrin.h>
# endif

#if CRYPTOPP_BOOL_AESNI_INTRINSICS_AVAILABLE
#if !defined(__GNUC__) || defined(__SSSE3__) || defined(__INTEL_COMPILER)
#include <tmmintrin.h>
#else
NAMESPACE_BEGIN(CryptoPP)
__inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_shuffle_epi8 (__m128i a, __m128i b)
{
	asm ("pshufb %1, %0" : "+x"(a) : "xm"(b));
  	return a;
}
NAMESPACE_END
#endif // tmmintrin.h
#if !defined(__GNUC__) || defined(__SSE4_1__) || defined(__INTEL_COMPILER)
#include <smmintrin.h>
#else
NAMESPACE_BEGIN(CryptoPP)
__inline int __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_extract_epi32 (__m128i a, const int i)
{
	int r;
	asm ("pextrd %2, %1, %0" : "=rm"(r) : "x"(a), "i"(i));
  	return r;
}
__inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_insert_epi32 (__m128i a, int b, const int i)
{
	asm ("pinsrd %2, %1, %0" : "+x"(a) : "rm"(b), "i"(i));
  	return a;
}
NAMESPACE_END
#endif // smmintrin.h
#if !defined(__GNUC__) || (defined(__AES__) && defined(__PCLMUL__)) || defined(__INTEL_COMPILER)
#include <wmmintrin.h>
#else
NAMESPACE_BEGIN(CryptoPP)
__inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_clmulepi64_si128 (__m128i a, __m128i b, const int i)
{
	asm ("pclmulqdq %2, %1, %0" : "+x"(a) : "xm"(b), "i"(i));
  	return a;
}
__inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_aeskeygenassist_si128 (__m128i a, const int i)
{
	__m128i r;
	asm ("aeskeygenassist %2, %1, %0" : "=x"(r) : "xm"(a), "i"(i));
  	return r;
}
__inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_aesimc_si128 (__m128i a)
{
	__m128i r;
	asm ("aesimc %1, %0" : "=x"(r) : "xm"(a));
  	return r;
}
__inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_aesenc_si128 (__m128i a, __m128i b)
{
	asm ("aesenc %1, %0" : "+x"(a) : "xm"(b));
  	return a;
}
__inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_aesenclast_si128 (__m128i a, __m128i b)
{
	asm ("aesenclast %1, %0" : "+x"(a) : "xm"(b));
  	return a;
}
__inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_aesdec_si128 (__m128i a, __m128i b)
{
	asm ("aesdec %1, %0" : "+x"(a) : "xm"(b));
  	return a;
}
__inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_aesdeclast_si128 (__m128i a, __m128i b)
{
	asm ("aesdeclast %1, %0" : "+x"(a) : "xm"(b));
  	return a;
}
NAMESPACE_END
#endif // wmmintrin.h
#endif // CRYPTOPP_BOOL_AESNI_INTRINSICS_AVAILABLE

NAMESPACE_BEGIN(CryptoPP)

#if CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64

#define CRYPTOPP_CPUID_AVAILABLE
	
// these should not be used directly
extern CRYPTOPP_DLL bool g_x86DetectionDone;
extern CRYPTOPP_DLL bool g_hasMMX;
extern CRYPTOPP_DLL bool g_hasISSE;
extern CRYPTOPP_DLL bool g_hasSSE2;
extern CRYPTOPP_DLL bool g_hasSSSE3;
extern CRYPTOPP_DLL bool g_hasAESNI;
extern CRYPTOPP_DLL bool g_hasCLMUL;
extern CRYPTOPP_DLL bool g_isP4;
extern CRYPTOPP_DLL bool g_hasRDRAND;
extern CRYPTOPP_DLL bool g_hasRDSEED;
extern CRYPTOPP_DLL word32 g_cacheLineSize;

CRYPTOPP_DLL void CRYPTOPP_API DetectX86Features();
CRYPTOPP_DLL bool CRYPTOPP_API CpuId(word32 input, word32 output[4]);

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

inline bool HasSSSE3()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasSSSE3;
}

inline bool HasAESNI()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasAESNI;
}

inline bool HasCLMUL()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasCLMUL;
}

inline bool IsP4()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_isP4;
}

inline bool HasRDRAND()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasRDRAND;
}

inline bool HasRDSEED()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_hasRDSEED;
}

inline int GetCacheLineSize()
{
	if (!g_x86DetectionDone)
		DetectX86Features();
	return g_cacheLineSize;
}

#else

inline int GetCacheLineSize()
{
	return CRYPTOPP_L1_CACHE_LINE_SIZE;
}

#endif

#endif

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

#if defined(CRYPTOPP_CLANG_VERSION) || defined(CRYPTOPP_APPLE_CLANG_VERSION)
	#define NEW_LINE "\n"
	#define INTEL_PREFIX ".intel_syntax;"
	#define INTEL_NOPREFIX ".intel_syntax;"
	#define ATT_PREFIX ".att_syntax;"
	#define ATT_NOPREFIX ".att_syntax;"
#else
	#define NEW_LINE
	#define INTEL_PREFIX ".intel_syntax prefix;"
	#define INTEL_NOPREFIX ".intel_syntax noprefix;"
	#define ATT_PREFIX ".att_syntax prefix;"
	#define ATT_NOPREFIX ".att_syntax noprefix;"
#endif

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

NAMESPACE_END

#endif
