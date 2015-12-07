// integer.cpp - written and placed in the public domain by Wei Dai
// contains public domain code contributed by Alister Lee and Leonard Janke

#include "pch.h"
#include "config.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4100)
#endif

#if CRYPTOPP_GCC_DIAGNOSTIC_AVAILABLE
# pragma GCC diagnostic ignored "-Wunused"
# pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

#ifndef CRYPTOPP_IMPORTS

#include "integer.h"
#include "secblock.h"
#include "modarith.h"
#include "nbtheory.h"
#include "smartptr.h"
#include "algparam.h"
#include "filters.h"
#include "asn.h"
#include "oids.h"
#include "words.h"
#include "pubkey.h"		// for P1363_KDF2
#include "sha.h"
#include "cpu.h"
#include "misc.h"

#include <iostream>

#if _MSC_VER >= 1400
	#include <intrin.h>
#endif

#ifdef __DECCXX
	#include <c_asm.h>
#endif

#ifdef CRYPTOPP_MSVC6_NO_PP
	#pragma message("You do not seem to have the Visual C++ Processor Pack installed, so use of SSE2 instructions will be disabled.")
#endif

// "Inline assembly operands don't work with .intel_syntax",
//   http://llvm.org/bugs/show_bug.cgi?id=24232
#if CRYPTOPP_BOOL_X32 || defined(CRYPTOPP_DISABLE_INTEL_ASM)
# undef CRYPTOPP_X86_ASM_AVAILABLE
# undef CRYPTOPP_X32_ASM_AVAILABLE
# undef CRYPTOPP_X64_ASM_AVAILABLE
# undef CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
# undef CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE
# define CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE 0
# define CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE 0
#else
# define CRYPTOPP_INTEGER_SSE2 (CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE && CRYPTOPP_BOOL_X86)
#endif

NAMESPACE_BEGIN(CryptoPP)
	
// Debian QEMU/ARMEL issue in MultiplyTop; see http://github.com/weidai11/cryptopp/issues/31.
#if __ARMEL__ && (CRYPTOPP_GCC_VERSION >= 50200) && (CRYPTOPP_GCC_VERSION < 50300) && __OPTIMIZE__
# define WORKAROUND_ARMEL_BUG 1
#endif

// Debian QEMU/ARM64 issue in Integer or ModularArithmetic; see http://github.com/weidai11/cryptopp/issues/61.
#if (__aarch64__ || __AARCH64EL__) && (CRYPTOPP_GCC_VERSION >= 50200) && (CRYPTOPP_GCC_VERSION < 50300)
# define WORKAROUND_ARM64_BUG 1
#endif

#if WORKAROUND_ARMEL_BUG
# pragma GCC push_options
# pragma GCC optimize("O1")
#endif
	
#if WORKAROUND_ARM64_BUG
# pragma GCC push_options
# pragma GCC optimize("no-devirtualize")
#endif

bool AssignIntToInteger(const std::type_info &valueType, void *pInteger, const void *pInt)
{
	if (valueType != typeid(Integer))
		return false;
	*reinterpret_cast<Integer *>(pInteger) = *reinterpret_cast<const int *>(pInt);
	return true;
}

inline static int Compare(const word *A, const word *B, size_t N)
{
	while (N--)
		if (A[N] > B[N])
			return 1;
		else if (A[N] < B[N])
			return -1;

	return 0;
}

inline static int Increment(word *A, size_t N, word B=1)
{
	assert(N);
	word t = A[0];
	A[0] = t+B;
	if (A[0] >= t)
		return 0;
	for (unsigned i=1; i<N; i++)
		if (++A[i])
			return 0;
	return 1;
}

inline static int Decrement(word *A, size_t N, word B=1)
{
	assert(N);
	word t = A[0];
	A[0] = t-B;
	if (A[0] <= t)
		return 0;
	for (unsigned i=1; i<N; i++)
		if (A[i]--)
			return 0;
	return 1;
}

static void TwosComplement(word *A, size_t N)
{
	Decrement(A, N);
	for (unsigned i=0; i<N; i++)
		A[i] = ~A[i];
}

static word AtomicInverseModPower2(word A)
{
	assert(A%2==1);

	word R=A%8;

	for (unsigned i=3; i<WORD_BITS; i*=2)
		R = R*(2-R*A);

	assert(R*A==1);
	return R;
}

// ********************************************************

#if !defined(CRYPTOPP_NATIVE_DWORD_AVAILABLE) || (defined(__x86_64__) && defined(CRYPTOPP_WORD128_AVAILABLE))
	#define Declare2Words(x)			word x##0, x##1;
	#define AssignWord(a, b)			a##0 = b; a##1 = 0;
	#define Add2WordsBy1(a, b, c)		a##0 = b##0 + c; a##1 = b##1 + (a##0 < c);
	#define LowWord(a)					a##0
	#define HighWord(a)					a##1
	#ifdef _MSC_VER
		#define MultiplyWordsLoHi(p0, p1, a, b)		p0 = _umul128(a, b, &p1);
		#ifndef __INTEL_COMPILER
			#define Double3Words(c, d)		d##1 = __shiftleft128(d##0, d##1, 1); d##0 = __shiftleft128(c, d##0, 1); c *= 2;
		#endif
	#elif defined(__DECCXX)
		#define MultiplyWordsLoHi(p0, p1, a, b)		p0 = a*b; p1 = asm("umulh %a0, %a1, %v0", a, b);
	#elif defined(__x86_64__)
		#if defined(__SUNPRO_CC) && __SUNPRO_CC < 0x5100
			// Sun Studio's gcc-style inline assembly is heavily bugged as of version 5.9 Patch 124864-09 2008/12/16, but this one works
			#define MultiplyWordsLoHi(p0, p1, a, b)		asm ("mulq %3" : "=a"(p0), "=d"(p1) : "a"(a), "r"(b) : "cc");
		#else
			#define MultiplyWordsLoHi(p0, p1, a, b)		asm ("mulq %3" : "=a"(p0), "=d"(p1) : "a"(a), "g"(b) : "cc");
			#define MulAcc(c, d, a, b)		asm ("mulq %6; addq %3, %0; adcq %4, %1; adcq $0, %2;" : "+r"(c), "+r"(d##0), "+r"(d##1), "=a"(p0), "=d"(p1) : "a"(a), "g"(b) : "cc");
			#define Double3Words(c, d)		asm ("addq %0, %0; adcq %1, %1; adcq %2, %2;" : "+r"(c), "+r"(d##0), "+r"(d##1) : : "cc");
			#define Acc2WordsBy1(a, b)		asm ("addq %2, %0; adcq $0, %1;" : "+r"(a##0), "+r"(a##1) : "r"(b) : "cc");
			#define Acc2WordsBy2(a, b)		asm ("addq %2, %0; adcq %3, %1;" : "+r"(a##0), "+r"(a##1) : "r"(b##0), "r"(b##1) : "cc");
			#define Acc3WordsBy2(c, d, e)	asm ("addq %5, %0; adcq %6, %1; adcq $0, %2;" : "+r"(c), "=r"(e##0), "=r"(e##1) : "1"(d##0), "2"(d##1), "r"(e##0), "r"(e##1) : "cc");
		#endif
	#endif
	#define MultiplyWords(p, a, b)		MultiplyWordsLoHi(p##0, p##1, a, b)
	#ifndef Double3Words
		#define Double3Words(c, d)		d##1 = 2*d##1 + (d##0>>(WORD_BITS-1)); d##0 = 2*d##0 + (c>>(WORD_BITS-1)); c *= 2;
	#endif
	#ifndef Acc2WordsBy2
		#define Acc2WordsBy2(a, b)		a##0 += b##0; a##1 += a##0 < b##0; a##1 += b##1;
	#endif
	#define AddWithCarry(u, a, b)		{word t = a+b; u##0 = t + u##1; u##1 = (t<a) + (u##0<t);}
	#define SubtractWithBorrow(u, a, b)	{word t = a-b; u##0 = t - u##1; u##1 = (t>a) + (u##0>t);}
	#define GetCarry(u)					u##1
	#define GetBorrow(u)				u##1
#else
	#define Declare2Words(x)			dword x;
	#if _MSC_VER >= 1400 && !defined(__INTEL_COMPILER)
		#define MultiplyWords(p, a, b)		p = __emulu(a, b);
	#else
		#define MultiplyWords(p, a, b)		p = (dword)a*b;
	#endif
	#define AssignWord(a, b)			a = b;
	#define Add2WordsBy1(a, b, c)		a = b + c;
	#define Acc2WordsBy2(a, b)			a += b;
	#define LowWord(a)					word(a)
	#define HighWord(a)					word(a>>WORD_BITS)
	#define Double3Words(c, d)			d = 2*d + (c>>(WORD_BITS-1)); c *= 2;
	#define AddWithCarry(u, a, b)		u = dword(a) + b + GetCarry(u);
	#define SubtractWithBorrow(u, a, b)	u = dword(a) - b - GetBorrow(u);
	#define GetCarry(u)					HighWord(u)
	#define GetBorrow(u)				word(u>>(WORD_BITS*2-1))
#endif
#ifndef MulAcc
	#define MulAcc(c, d, a, b)			MultiplyWords(p, a, b); Acc2WordsBy1(p, c); c = LowWord(p); Acc2WordsBy1(d, HighWord(p));
#endif
#ifndef Acc2WordsBy1
	#define Acc2WordsBy1(a, b)			Add2WordsBy1(a, a, b)
#endif
#ifndef Acc3WordsBy2
	#define Acc3WordsBy2(c, d, e)		Acc2WordsBy1(e, c); c = LowWord(e); Add2WordsBy1(e, d, HighWord(e));
#endif

class DWord
{
public:
	// Converity finding on default ctor. We've isntrumented the code,
	//   and cannot uncover a case where it affects a result.
#if (defined(__COVERITY__) || !defined(NDEBUG)) && defined(CRYPTOPP_NATIVE_DWORD_AVAILABLE)
	// Repeating pattern of 1010 for debug builds to break things...
	DWord() : m_whole(0) {memset(&m_whole, 0xa, sizeof(m_whole));}
#elif (defined(__COVERITY__) || !defined(NDEBUG)) && !defined(CRYPTOPP_NATIVE_DWORD_AVAILABLE)
	// Repeating pattern of 1010 for debug builds to break things...
	DWord() : m_halfs() {memset(&m_halfs, 0xa, sizeof(m_halfs));}
#else
	DWord() {}
#endif

#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
	explicit DWord(word low) : m_whole(low) {}
#else
	explicit DWord(word low)
	{
		m_halfs.low = low;
		m_halfs.high = 0;
	}
#endif

	DWord(word low, word high)
	{
		m_halfs.low = low;
		m_halfs.high = high;
	}

	static DWord Multiply(word a, word b)
	{
		DWord r;
		#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
			r.m_whole = (dword)a * b;
		#elif defined(MultiplyWordsLoHi)
			MultiplyWordsLoHi(r.m_halfs.low, r.m_halfs.high, a, b);
		#else
			assert(0);
		#endif
		return r;
	}

	static DWord MultiplyAndAdd(word a, word b, word c)
	{
		DWord r = Multiply(a, b);
		return r += c;
	}

	DWord & operator+=(word a)
	{
		#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
			m_whole = m_whole + a;
		#else
			m_halfs.low += a;
			m_halfs.high += (m_halfs.low < a);
		#endif
		return *this;
	}

	DWord operator+(word a)
	{
		DWord r;
		#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
			r.m_whole = m_whole + a;
		#else
			r.m_halfs.low = m_halfs.low + a;
			r.m_halfs.high = m_halfs.high + (r.m_halfs.low < a);
		#endif
		return r;
	}

	DWord operator-(DWord a)
	{
		DWord r;
		#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
			r.m_whole = m_whole - a.m_whole;
		#else
			r.m_halfs.low = m_halfs.low - a.m_halfs.low;
			r.m_halfs.high = m_halfs.high - a.m_halfs.high - (r.m_halfs.low > m_halfs.low);
		#endif
		return r;
	}

	DWord operator-(word a)
	{
		DWord r;
		#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
			r.m_whole = m_whole - a;
		#else
			r.m_halfs.low = m_halfs.low - a;
			r.m_halfs.high = m_halfs.high - (r.m_halfs.low > m_halfs.low);
		#endif
		return r;
	}

	// returns quotient, which must fit in a word
	word operator/(word divisor);

	word operator%(word a);

	bool operator!() const
	{
	#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
		return !m_whole;
	#else
		return !m_halfs.high && !m_halfs.low;
	#endif
	}

	word GetLowHalf() const {return m_halfs.low;}
	word GetHighHalf() const {return m_halfs.high;}
	word GetHighHalfAsBorrow() const {return 0-m_halfs.high;}

private:
	union
	{
	#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
		dword m_whole;
	#endif
		struct
		{
		#ifdef IS_LITTLE_ENDIAN
			word low;
			word high;
		#else
			word high;
			word low;
		#endif
		} m_halfs;
	};
};

class Word
{
public:
	// Converity finding on default ctor. We've isntrumented the code,
	//   and cannot uncover a case where it affects a result.
#if defined(__COVERITY__)
	Word() : m_whole(0) {}
#elif !defined(NDEBUG)
	// Repeating pattern of 1010 for debug builds to break things...
	Word() : m_whole(0) {memset(&m_whole, 0xa, sizeof(m_whole));}
#else
	Word() {}
#endif

	Word(word value) : m_whole(value) {}
	Word(hword low, hword high) : m_whole(low | (word(high) << (WORD_BITS/2))) {}

	static Word Multiply(hword a, hword b)
	{
		Word r;
		r.m_whole = (word)a * b;
		return r;
	}

	Word operator-(Word a)
	{
		Word r;
		r.m_whole = m_whole - a.m_whole;
		return r;
	}

	Word operator-(hword a)
	{
		Word r;
		r.m_whole = m_whole - a;
		return r;
	}

	// returns quotient, which must fit in a word
	hword operator/(hword divisor)
	{
		return hword(m_whole / divisor);
	}

	bool operator!() const
	{
		return !m_whole;
	}

	word GetWhole() const {return m_whole;}
	hword GetLowHalf() const {return hword(m_whole);}
	hword GetHighHalf() const {return hword(m_whole>>(WORD_BITS/2));}
	hword GetHighHalfAsBorrow() const {return 0-hword(m_whole>>(WORD_BITS/2));}
	
private:
	word m_whole;
};

// do a 3 word by 2 word divide, returns quotient and leaves remainder in A
template <class S, class D>
S DivideThreeWordsByTwo(S *A, S B0, S B1, D *dummy=NULL)
{
	CRYPTOPP_UNUSED(dummy);

	// assert {A[2],A[1]} < {B1,B0}, so quotient can fit in a S
	assert(A[2] < B1 || (A[2]==B1 && A[1] < B0));

	// estimate the quotient: do a 2 S by 1 S divide
	S Q;
	if (S(B1+1) == 0)
		Q = A[2];
	else if (B1 > 0)
		Q = D(A[1], A[2]) / S(B1+1);
	else
		Q = D(A[0], A[1]) / B0;

	// now subtract Q*B from A
	D p = D::Multiply(B0, Q);
	D u = (D) A[0] - p.GetLowHalf();
	A[0] = u.GetLowHalf();
	u = (D) A[1] - p.GetHighHalf() - u.GetHighHalfAsBorrow() - D::Multiply(B1, Q);
	A[1] = u.GetLowHalf();
	A[2] += u.GetHighHalf();

	// Q <= actual quotient, so fix it
	while (A[2] || A[1] > B1 || (A[1]==B1 && A[0]>=B0))
	{
		u = (D) A[0] - B0;
		A[0] = u.GetLowHalf();
		u = (D) A[1] - B1 - u.GetHighHalfAsBorrow();
		A[1] = u.GetLowHalf();
		A[2] += u.GetHighHalf();
		Q++;
		assert(Q);	// shouldn't overflow
	}

	return Q;
}

// do a 4 word by 2 word divide, returns 2 word quotient in Q0 and Q1
template <class S, class D>
inline D DivideFourWordsByTwo(S *T, const D &Al, const D &Ah, const D &B)
{
	if (!B) // if divisor is 0, we assume divisor==2**(2*WORD_BITS)
		return D(Ah.GetLowHalf(), Ah.GetHighHalf());
	else
	{
		S Q[2];
		T[0] = Al.GetLowHalf();
		T[1] = Al.GetHighHalf(); 
		T[2] = Ah.GetLowHalf();
		T[3] = Ah.GetHighHalf();
		Q[1] = DivideThreeWordsByTwo<S, D>(T+1, B.GetLowHalf(), B.GetHighHalf());
		Q[0] = DivideThreeWordsByTwo<S, D>(T, B.GetLowHalf(), B.GetHighHalf());
		return D(Q[0], Q[1]);
	}
}

// returns quotient, which must fit in a word
inline word DWord::operator/(word a)
{
	#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
		return word(m_whole / a);
	#else
		hword r[4];
		return DivideFourWordsByTwo<hword, Word>(r, m_halfs.low, m_halfs.high, a).GetWhole();
	#endif
}

inline word DWord::operator%(word a)
{
	#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
		return word(m_whole % a);
	#else
		if (a < (word(1) << (WORD_BITS/2)))
		{
			hword h = hword(a);
			word r = m_halfs.high % h;
			r = ((m_halfs.low >> (WORD_BITS/2)) + (r << (WORD_BITS/2))) % h;
			return hword((hword(m_halfs.low) + (r << (WORD_BITS/2))) % h);
		}
		else
		{
			hword r[4];
			DivideFourWordsByTwo<hword, Word>(r, m_halfs.low, m_halfs.high, a);
			return Word(r[0], r[1]).GetWhole();
		}
	#endif
}

// ********************************************************

// Use some tricks to share assembly code between MSVC and GCC
#if defined(__GNUC__)
	#define AddPrologue \
		int result;	\
		__asm__ __volatile__ \
		( \
			INTEL_NOPREFIX
	#define AddEpilogue \
			".att_syntax prefix;" \
					: "=a" (result)\
					: "d" (C), "a" (A), "D" (B), "c" (N) \
					: "%esi", "memory", "cc" \
		);\
		return result;
	#define MulPrologue \
		__asm__ __volatile__ \
		( \
			".intel_syntax noprefix;" \
			AS1(	push	ebx) \
			AS2(	mov		ebx, edx)
	#define MulEpilogue \
			AS1(	pop		ebx) \
			".att_syntax prefix;" \
			: \
			: "d" (s_maskLow16), "c" (C), "a" (A), "D" (B) \
			: "%esi", "memory", "cc" \
		);
	#define SquPrologue		MulPrologue
	#define SquEpilogue	\
			AS1(	pop		ebx) \
			".att_syntax prefix;" \
			: \
			: "d" (s_maskLow16), "c" (C), "a" (A) \
			: "%esi", "%edi", "memory", "cc" \
		);
	#define TopPrologue		MulPrologue
	#define TopEpilogue	\
			AS1(	pop		ebx) \
			".att_syntax prefix;" \
			: \
			: "d" (s_maskLow16), "c" (C), "a" (A), "D" (B), "S" (L) \
			: "memory", "cc" \
		);
#else
	#define AddPrologue \
		__asm	push edi \
		__asm	push esi \
		__asm	mov		eax, [esp+12] \
		__asm	mov		edi, [esp+16]
	#define AddEpilogue \
		__asm	pop esi \
		__asm	pop edi \
		__asm	ret 8
#if _MSC_VER < 1300
	#define SaveEBX		__asm push ebx
	#define RestoreEBX	__asm pop ebx
#else
	#define SaveEBX
	#define RestoreEBX
#endif
	#define SquPrologue					\
		AS2(	mov		eax, A)			\
		AS2(	mov		ecx, C)			\
		SaveEBX							\
		AS2(	lea		ebx, s_maskLow16)
	#define MulPrologue					\
		AS2(	mov		eax, A)			\
		AS2(	mov		edi, B)			\
		AS2(	mov		ecx, C)			\
		SaveEBX							\
		AS2(	lea		ebx, s_maskLow16)
	#define TopPrologue					\
		AS2(	mov		eax, A)			\
		AS2(	mov		edi, B)			\
		AS2(	mov		ecx, C)			\
		AS2(	mov		esi, L)			\
		SaveEBX							\
		AS2(	lea		ebx, s_maskLow16)
	#define SquEpilogue		RestoreEBX
	#define MulEpilogue		RestoreEBX
	#define TopEpilogue		RestoreEBX
#endif

#ifdef CRYPTOPP_X64_MASM_AVAILABLE
extern "C" {
int Baseline_Add(size_t N, word *C, const word *A, const word *B);
int Baseline_Sub(size_t N, word *C, const word *A, const word *B);
}
#elif defined(CRYPTOPP_X64_ASM_AVAILABLE) && defined(__GNUC__) && defined(CRYPTOPP_WORD128_AVAILABLE)
int Baseline_Add(size_t N, word *C, const word *A, const word *B)
{
	word result;
	__asm__ __volatile__
	(
	INTEL_NOPREFIX
	AS1(	neg		%1)
	ASJ(	jz,		1, f)
	AS2(	mov		%0,[%3+8*%1])
	AS2(	add		%0,[%4+8*%1])
	AS2(	mov		[%2+8*%1],%0)
	ASL(0)
	AS2(	mov		%0,[%3+8*%1+8])
	AS2(	adc		%0,[%4+8*%1+8])
	AS2(	mov		[%2+8*%1+8],%0)
	AS2(	lea		%1,[%1+2])
	ASJ(	jrcxz,	1, f)
	AS2(	mov		%0,[%3+8*%1])
	AS2(	adc		%0,[%4+8*%1])
	AS2(	mov		[%2+8*%1],%0)
	ASJ(	jmp,	0, b)
	ASL(1)
	AS2(	mov		%0, 0)
	AS2(	adc		%0, %0)
	ATT_NOPREFIX
	: "=&r" (result), "+c" (N)
	: "r" (C+N), "r" (A+N), "r" (B+N)
	: "memory", "cc"
	);
	return (int)result;
}

int Baseline_Sub(size_t N, word *C, const word *A, const word *B)
{
	word result;
	__asm__ __volatile__
	(
	INTEL_NOPREFIX
	AS1(	neg		%1)
	ASJ(	jz,		1, f)
	AS2(	mov		%0,[%3+8*%1])
	AS2(	sub		%0,[%4+8*%1])
	AS2(	mov		[%2+8*%1],%0)
	ASL(0)
	AS2(	mov		%0,[%3+8*%1+8])
	AS2(	sbb		%0,[%4+8*%1+8])
	AS2(	mov		[%2+8*%1+8],%0)
	AS2(	lea		%1,[%1+2])
	ASJ(	jrcxz,	1, f)
	AS2(	mov		%0,[%3+8*%1])
	AS2(	sbb		%0,[%4+8*%1])
	AS2(	mov		[%2+8*%1],%0)
	ASJ(	jmp,	0, b)
	ASL(1)
	AS2(	mov		%0, 0)
	AS2(	adc		%0, %0)
	ATT_NOPREFIX
	: "=&r" (result), "+c" (N)
	: "r" (C+N), "r" (A+N), "r" (B+N)
	: "memory", "cc"
	);
	return (int)result;
}
#elif defined(CRYPTOPP_X86_ASM_AVAILABLE) && CRYPTOPP_BOOL_X86
CRYPTOPP_NAKED int CRYPTOPP_FASTCALL Baseline_Add(size_t N, word *C, const word *A, const word *B)
{
	AddPrologue

	// now: eax = A, edi = B, edx = C, ecx = N
	AS2(	lea		eax, [eax+4*ecx])
	AS2(	lea		edi, [edi+4*ecx])
	AS2(	lea		edx, [edx+4*ecx])

	AS1(	neg		ecx)				// ecx is negative index
	AS2(	test	ecx, 2)				// this clears carry flag
	ASJ(	jz,		0, f)
	AS2(	sub		ecx, 2)
	ASJ(	jmp,	1, f)

	ASL(0)
	ASJ(	jecxz,	2, f)				// loop until ecx overflows and becomes zero
	AS2(	mov		esi,[eax+4*ecx])
	AS2(	adc		esi,[edi+4*ecx])
	AS2(	mov		[edx+4*ecx],esi)
	AS2(	mov		esi,[eax+4*ecx+4])
	AS2(	adc		esi,[edi+4*ecx+4])
	AS2(	mov		[edx+4*ecx+4],esi)
	ASL(1)
	AS2(	mov		esi,[eax+4*ecx+8])
	AS2(	adc		esi,[edi+4*ecx+8])
	AS2(	mov		[edx+4*ecx+8],esi)
	AS2(	mov		esi,[eax+4*ecx+12])
	AS2(	adc		esi,[edi+4*ecx+12])
	AS2(	mov		[edx+4*ecx+12],esi)

	AS2(	lea		ecx,[ecx+4])		// advance index, avoid inc which causes slowdown on Intel Core 2
	ASJ(	jmp,	0, b)

	ASL(2)
	AS2(	mov		eax, 0)
	AS1(	setc	al)					// store carry into eax (return result register)

	AddEpilogue
}

CRYPTOPP_NAKED int CRYPTOPP_FASTCALL Baseline_Sub(size_t N, word *C, const word *A, const word *B)
{
	AddPrologue

	// now: eax = A, edi = B, edx = C, ecx = N
	AS2(	lea		eax, [eax+4*ecx])
	AS2(	lea		edi, [edi+4*ecx])
	AS2(	lea		edx, [edx+4*ecx])

	AS1(	neg		ecx)				// ecx is negative index
	AS2(	test	ecx, 2)				// this clears carry flag
	ASJ(	jz,		0, f)
	AS2(	sub		ecx, 2)
	ASJ(	jmp,	1, f)

	ASL(0)
	ASJ(	jecxz,	2, f)				// loop until ecx overflows and becomes zero
	AS2(	mov		esi,[eax+4*ecx])
	AS2(	sbb		esi,[edi+4*ecx])
	AS2(	mov		[edx+4*ecx],esi)
	AS2(	mov		esi,[eax+4*ecx+4])
	AS2(	sbb		esi,[edi+4*ecx+4])
	AS2(	mov		[edx+4*ecx+4],esi)
	ASL(1)
	AS2(	mov		esi,[eax+4*ecx+8])
	AS2(	sbb		esi,[edi+4*ecx+8])
	AS2(	mov		[edx+4*ecx+8],esi)
	AS2(	mov		esi,[eax+4*ecx+12])
	AS2(	sbb		esi,[edi+4*ecx+12])
	AS2(	mov		[edx+4*ecx+12],esi)

	AS2(	lea		ecx,[ecx+4])		// advance index, avoid inc which causes slowdown on Intel Core 2
	ASJ(	jmp,	0, b)

	ASL(2)
	AS2(	mov		eax, 0)
	AS1(	setc	al)					// store carry into eax (return result register)

	AddEpilogue
}

#if CRYPTOPP_INTEGER_SSE2
CRYPTOPP_NAKED int CRYPTOPP_FASTCALL SSE2_Add(size_t N, word *C, const word *A, const word *B)
{
	AddPrologue

	// now: eax = A, edi = B, edx = C, ecx = N
	AS2(	lea		eax, [eax+4*ecx])
	AS2(	lea		edi, [edi+4*ecx])
	AS2(	lea		edx, [edx+4*ecx])

	AS1(	neg		ecx)				// ecx is negative index
	AS2(	pxor    mm2, mm2)
	ASJ(	jz,		2, f)
	AS2(	test	ecx, 2)				// this clears carry flag
	ASJ(	jz,		0, f)
	AS2(	sub		ecx, 2)
	ASJ(	jmp,	1, f)

	ASL(0)
	AS2(	movd     mm0, DWORD PTR [eax+4*ecx])
	AS2(	movd     mm1, DWORD PTR [edi+4*ecx])
	AS2(	paddq    mm0, mm1)
	AS2(	paddq	 mm2, mm0)
	AS2(	movd	 DWORD PTR [edx+4*ecx], mm2)
	AS2(	psrlq    mm2, 32)

	AS2(	movd     mm0, DWORD PTR [eax+4*ecx+4])
	AS2(	movd     mm1, DWORD PTR [edi+4*ecx+4])
	AS2(	paddq    mm0, mm1)
	AS2(	paddq	 mm2, mm0)
	AS2(	movd	 DWORD PTR [edx+4*ecx+4], mm2)
	AS2(	psrlq    mm2, 32)

	ASL(1)
	AS2(	movd     mm0, DWORD PTR [eax+4*ecx+8])
	AS2(	movd     mm1, DWORD PTR [edi+4*ecx+8])
	AS2(	paddq    mm0, mm1)
	AS2(	paddq	 mm2, mm0)
	AS2(	movd	 DWORD PTR [edx+4*ecx+8], mm2)
	AS2(	psrlq    mm2, 32)

	AS2(	movd     mm0, DWORD PTR [eax+4*ecx+12])
	AS2(	movd     mm1, DWORD PTR [edi+4*ecx+12])
	AS2(	paddq    mm0, mm1)
	AS2(	paddq	 mm2, mm0)
	AS2(	movd	 DWORD PTR [edx+4*ecx+12], mm2)
	AS2(	psrlq    mm2, 32)

	AS2(	add		ecx, 4)
	ASJ(	jnz,	0, b)

	ASL(2)
	AS2(	movd	eax, mm2)
	AS1(	emms)

	AddEpilogue
}
CRYPTOPP_NAKED int CRYPTOPP_FASTCALL SSE2_Sub(size_t N, word *C, const word *A, const word *B)
{
	AddPrologue

	// now: eax = A, edi = B, edx = C, ecx = N
	AS2(	lea		eax, [eax+4*ecx])
	AS2(	lea		edi, [edi+4*ecx])
	AS2(	lea		edx, [edx+4*ecx])

	AS1(	neg		ecx)				// ecx is negative index
	AS2(	pxor    mm2, mm2)
	ASJ(	jz,		2, f)
	AS2(	test	ecx, 2)				// this clears carry flag
	ASJ(	jz,		0, f)
	AS2(	sub		ecx, 2)
	ASJ(	jmp,	1, f)

	ASL(0)
	AS2(	movd     mm0, DWORD PTR [eax+4*ecx])
	AS2(	movd     mm1, DWORD PTR [edi+4*ecx])
	AS2(	psubq    mm0, mm1)
	AS2(	psubq	 mm0, mm2)
	AS2(	movd	 DWORD PTR [edx+4*ecx], mm0)
	AS2(	psrlq    mm0, 63)

	AS2(	movd     mm2, DWORD PTR [eax+4*ecx+4])
	AS2(	movd     mm1, DWORD PTR [edi+4*ecx+4])
	AS2(	psubq    mm2, mm1)
	AS2(	psubq	 mm2, mm0)
	AS2(	movd	 DWORD PTR [edx+4*ecx+4], mm2)
	AS2(	psrlq    mm2, 63)

	ASL(1)
	AS2(	movd     mm0, DWORD PTR [eax+4*ecx+8])
	AS2(	movd     mm1, DWORD PTR [edi+4*ecx+8])
	AS2(	psubq    mm0, mm1)
	AS2(	psubq	 mm0, mm2)
	AS2(	movd	 DWORD PTR [edx+4*ecx+8], mm0)
	AS2(	psrlq    mm0, 63)

	AS2(	movd     mm2, DWORD PTR [eax+4*ecx+12])
	AS2(	movd     mm1, DWORD PTR [edi+4*ecx+12])
	AS2(	psubq    mm2, mm1)
	AS2(	psubq	 mm2, mm0)
	AS2(	movd	 DWORD PTR [edx+4*ecx+12], mm2)
	AS2(	psrlq    mm2, 63)

	AS2(	add		ecx, 4)
	ASJ(	jnz,	0, b)

	ASL(2)
	AS2(	movd	eax, mm2)
	AS1(	emms)

	AddEpilogue
}
#endif	// #if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
#else
int CRYPTOPP_FASTCALL Baseline_Add(size_t N, word *C, const word *A, const word *B)
{
	assert (N%2 == 0);

	Declare2Words(u);
	AssignWord(u, 0);
	for (size_t i=0; i<N; i+=2)
	{
		AddWithCarry(u, A[i], B[i]);
		C[i] = LowWord(u);
		AddWithCarry(u, A[i+1], B[i+1]);
		C[i+1] = LowWord(u);
	}
	return int(GetCarry(u));
}

int CRYPTOPP_FASTCALL Baseline_Sub(size_t N, word *C, const word *A, const word *B)
{
	assert (N%2 == 0);

	Declare2Words(u);
	AssignWord(u, 0);
	for (size_t i=0; i<N; i+=2)
	{
		SubtractWithBorrow(u, A[i], B[i]);
		C[i] = LowWord(u);
		SubtractWithBorrow(u, A[i+1], B[i+1]);
		C[i+1] = LowWord(u);
	}
	return int(GetBorrow(u));
}
#endif

static word LinearMultiply(word *C, const word *A, word B, size_t N)
{
	word carry=0;
	for(unsigned i=0; i<N; i++)
	{
		Declare2Words(p);
		MultiplyWords(p, A[i], B);
		Acc2WordsBy1(p, carry);
		C[i] = LowWord(p);
		carry = HighWord(p);
	}
	return carry;
}

#ifndef CRYPTOPP_DOXYGEN_PROCESSING

#define Mul_2 \
	Mul_Begin(2) \
	Mul_SaveAcc(0, 0, 1) Mul_Acc(1, 0) \
	Mul_End(1, 1)

#define Mul_4 \
	Mul_Begin(4) \
	Mul_SaveAcc(0, 0, 1) Mul_Acc(1, 0) \
	Mul_SaveAcc(1, 0, 2) Mul_Acc(1, 1) Mul_Acc(2, 0)  \
	Mul_SaveAcc(2, 0, 3) Mul_Acc(1, 2) Mul_Acc(2, 1) Mul_Acc(3, 0)  \
	Mul_SaveAcc(3, 1, 3) Mul_Acc(2, 2) Mul_Acc(3, 1)  \
	Mul_SaveAcc(4, 2, 3) Mul_Acc(3, 2) \
	Mul_End(5, 3)

#define Mul_8 \
	Mul_Begin(8) \
	Mul_SaveAcc(0, 0, 1) Mul_Acc(1, 0) \
	Mul_SaveAcc(1, 0, 2) Mul_Acc(1, 1) Mul_Acc(2, 0)  \
	Mul_SaveAcc(2, 0, 3) Mul_Acc(1, 2) Mul_Acc(2, 1) Mul_Acc(3, 0)  \
	Mul_SaveAcc(3, 0, 4) Mul_Acc(1, 3) Mul_Acc(2, 2) Mul_Acc(3, 1) Mul_Acc(4, 0) \
	Mul_SaveAcc(4, 0, 5) Mul_Acc(1, 4) Mul_Acc(2, 3) Mul_Acc(3, 2) Mul_Acc(4, 1) Mul_Acc(5, 0) \
	Mul_SaveAcc(5, 0, 6) Mul_Acc(1, 5) Mul_Acc(2, 4) Mul_Acc(3, 3) Mul_Acc(4, 2) Mul_Acc(5, 1) Mul_Acc(6, 0) \
	Mul_SaveAcc(6, 0, 7) Mul_Acc(1, 6) Mul_Acc(2, 5) Mul_Acc(3, 4) Mul_Acc(4, 3) Mul_Acc(5, 2) Mul_Acc(6, 1) Mul_Acc(7, 0) \
	Mul_SaveAcc(7, 1, 7) Mul_Acc(2, 6) Mul_Acc(3, 5) Mul_Acc(4, 4) Mul_Acc(5, 3) Mul_Acc(6, 2) Mul_Acc(7, 1) \
	Mul_SaveAcc(8, 2, 7) Mul_Acc(3, 6) Mul_Acc(4, 5) Mul_Acc(5, 4) Mul_Acc(6, 3) Mul_Acc(7, 2) \
	Mul_SaveAcc(9, 3, 7) Mul_Acc(4, 6) Mul_Acc(5, 5) Mul_Acc(6, 4) Mul_Acc(7, 3) \
	Mul_SaveAcc(10, 4, 7) Mul_Acc(5, 6) Mul_Acc(6, 5) Mul_Acc(7, 4) \
	Mul_SaveAcc(11, 5, 7) Mul_Acc(6, 6) Mul_Acc(7, 5) \
	Mul_SaveAcc(12, 6, 7) Mul_Acc(7, 6) \
	Mul_End(13, 7)

#define Mul_16 \
	Mul_Begin(16) \
	Mul_SaveAcc(0, 0, 1) Mul_Acc(1, 0) \
	Mul_SaveAcc(1, 0, 2) Mul_Acc(1, 1) Mul_Acc(2, 0) \
	Mul_SaveAcc(2, 0, 3) Mul_Acc(1, 2) Mul_Acc(2, 1) Mul_Acc(3, 0) \
	Mul_SaveAcc(3, 0, 4) Mul_Acc(1, 3) Mul_Acc(2, 2) Mul_Acc(3, 1) Mul_Acc(4, 0) \
	Mul_SaveAcc(4, 0, 5) Mul_Acc(1, 4) Mul_Acc(2, 3) Mul_Acc(3, 2) Mul_Acc(4, 1) Mul_Acc(5, 0) \
	Mul_SaveAcc(5, 0, 6) Mul_Acc(1, 5) Mul_Acc(2, 4) Mul_Acc(3, 3) Mul_Acc(4, 2) Mul_Acc(5, 1) Mul_Acc(6, 0) \
	Mul_SaveAcc(6, 0, 7) Mul_Acc(1, 6) Mul_Acc(2, 5) Mul_Acc(3, 4) Mul_Acc(4, 3) Mul_Acc(5, 2) Mul_Acc(6, 1) Mul_Acc(7, 0) \
	Mul_SaveAcc(7, 0, 8) Mul_Acc(1, 7) Mul_Acc(2, 6) Mul_Acc(3, 5) Mul_Acc(4, 4) Mul_Acc(5, 3) Mul_Acc(6, 2) Mul_Acc(7, 1) Mul_Acc(8, 0) \
	Mul_SaveAcc(8, 0, 9) Mul_Acc(1, 8) Mul_Acc(2, 7) Mul_Acc(3, 6) Mul_Acc(4, 5) Mul_Acc(5, 4) Mul_Acc(6, 3) Mul_Acc(7, 2) Mul_Acc(8, 1) Mul_Acc(9, 0) \
	Mul_SaveAcc(9, 0, 10) Mul_Acc(1, 9) Mul_Acc(2, 8) Mul_Acc(3, 7) Mul_Acc(4, 6) Mul_Acc(5, 5) Mul_Acc(6, 4) Mul_Acc(7, 3) Mul_Acc(8, 2) Mul_Acc(9, 1) Mul_Acc(10, 0) \
	Mul_SaveAcc(10, 0, 11) Mul_Acc(1, 10) Mul_Acc(2, 9) Mul_Acc(3, 8) Mul_Acc(4, 7) Mul_Acc(5, 6) Mul_Acc(6, 5) Mul_Acc(7, 4) Mul_Acc(8, 3) Mul_Acc(9, 2) Mul_Acc(10, 1) Mul_Acc(11, 0) \
	Mul_SaveAcc(11, 0, 12) Mul_Acc(1, 11) Mul_Acc(2, 10) Mul_Acc(3, 9) Mul_Acc(4, 8) Mul_Acc(5, 7) Mul_Acc(6, 6) Mul_Acc(7, 5) Mul_Acc(8, 4) Mul_Acc(9, 3) Mul_Acc(10, 2) Mul_Acc(11, 1) Mul_Acc(12, 0) \
	Mul_SaveAcc(12, 0, 13) Mul_Acc(1, 12) Mul_Acc(2, 11) Mul_Acc(3, 10) Mul_Acc(4, 9) Mul_Acc(5, 8) Mul_Acc(6, 7) Mul_Acc(7, 6) Mul_Acc(8, 5) Mul_Acc(9, 4) Mul_Acc(10, 3) Mul_Acc(11, 2) Mul_Acc(12, 1) Mul_Acc(13, 0) \
	Mul_SaveAcc(13, 0, 14) Mul_Acc(1, 13) Mul_Acc(2, 12) Mul_Acc(3, 11) Mul_Acc(4, 10) Mul_Acc(5, 9) Mul_Acc(6, 8) Mul_Acc(7, 7) Mul_Acc(8, 6) Mul_Acc(9, 5) Mul_Acc(10, 4) Mul_Acc(11, 3) Mul_Acc(12, 2) Mul_Acc(13, 1) Mul_Acc(14, 0) \
	Mul_SaveAcc(14, 0, 15) Mul_Acc(1, 14) Mul_Acc(2, 13) Mul_Acc(3, 12) Mul_Acc(4, 11) Mul_Acc(5, 10) Mul_Acc(6, 9) Mul_Acc(7, 8) Mul_Acc(8, 7) Mul_Acc(9, 6) Mul_Acc(10, 5) Mul_Acc(11, 4) Mul_Acc(12, 3) Mul_Acc(13, 2) Mul_Acc(14, 1) Mul_Acc(15, 0) \
	Mul_SaveAcc(15, 1, 15) Mul_Acc(2, 14) Mul_Acc(3, 13) Mul_Acc(4, 12) Mul_Acc(5, 11) Mul_Acc(6, 10) Mul_Acc(7, 9) Mul_Acc(8, 8) Mul_Acc(9, 7) Mul_Acc(10, 6) Mul_Acc(11, 5) Mul_Acc(12, 4) Mul_Acc(13, 3) Mul_Acc(14, 2) Mul_Acc(15, 1) \
	Mul_SaveAcc(16, 2, 15) Mul_Acc(3, 14) Mul_Acc(4, 13) Mul_Acc(5, 12) Mul_Acc(6, 11) Mul_Acc(7, 10) Mul_Acc(8, 9) Mul_Acc(9, 8) Mul_Acc(10, 7) Mul_Acc(11, 6) Mul_Acc(12, 5) Mul_Acc(13, 4) Mul_Acc(14, 3) Mul_Acc(15, 2) \
	Mul_SaveAcc(17, 3, 15) Mul_Acc(4, 14) Mul_Acc(5, 13) Mul_Acc(6, 12) Mul_Acc(7, 11) Mul_Acc(8, 10) Mul_Acc(9, 9) Mul_Acc(10, 8) Mul_Acc(11, 7) Mul_Acc(12, 6) Mul_Acc(13, 5) Mul_Acc(14, 4) Mul_Acc(15, 3) \
	Mul_SaveAcc(18, 4, 15) Mul_Acc(5, 14) Mul_Acc(6, 13) Mul_Acc(7, 12) Mul_Acc(8, 11) Mul_Acc(9, 10) Mul_Acc(10, 9) Mul_Acc(11, 8) Mul_Acc(12, 7) Mul_Acc(13, 6) Mul_Acc(14, 5) Mul_Acc(15, 4) \
	Mul_SaveAcc(19, 5, 15) Mul_Acc(6, 14) Mul_Acc(7, 13) Mul_Acc(8, 12) Mul_Acc(9, 11) Mul_Acc(10, 10) Mul_Acc(11, 9) Mul_Acc(12, 8) Mul_Acc(13, 7) Mul_Acc(14, 6) Mul_Acc(15, 5) \
	Mul_SaveAcc(20, 6, 15) Mul_Acc(7, 14) Mul_Acc(8, 13) Mul_Acc(9, 12) Mul_Acc(10, 11) Mul_Acc(11, 10) Mul_Acc(12, 9) Mul_Acc(13, 8) Mul_Acc(14, 7) Mul_Acc(15, 6) \
	Mul_SaveAcc(21, 7, 15) Mul_Acc(8, 14) Mul_Acc(9, 13) Mul_Acc(10, 12) Mul_Acc(11, 11) Mul_Acc(12, 10) Mul_Acc(13, 9) Mul_Acc(14, 8) Mul_Acc(15, 7) \
	Mul_SaveAcc(22, 8, 15) Mul_Acc(9, 14) Mul_Acc(10, 13) Mul_Acc(11, 12) Mul_Acc(12, 11) Mul_Acc(13, 10) Mul_Acc(14, 9) Mul_Acc(15, 8) \
	Mul_SaveAcc(23, 9, 15) Mul_Acc(10, 14) Mul_Acc(11, 13) Mul_Acc(12, 12) Mul_Acc(13, 11) Mul_Acc(14, 10) Mul_Acc(15, 9) \
	Mul_SaveAcc(24, 10, 15) Mul_Acc(11, 14) Mul_Acc(12, 13) Mul_Acc(13, 12) Mul_Acc(14, 11) Mul_Acc(15, 10) \
	Mul_SaveAcc(25, 11, 15) Mul_Acc(12, 14) Mul_Acc(13, 13) Mul_Acc(14, 12) Mul_Acc(15, 11) \
	Mul_SaveAcc(26, 12, 15) Mul_Acc(13, 14) Mul_Acc(14, 13) Mul_Acc(15, 12) \
	Mul_SaveAcc(27, 13, 15) Mul_Acc(14, 14) Mul_Acc(15, 13) \
	Mul_SaveAcc(28, 14, 15) Mul_Acc(15, 14) \
	Mul_End(29, 15)

#define Squ_2 \
	Squ_Begin(2) \
	Squ_End(2)

#define Squ_4 \
	Squ_Begin(4) \
	Squ_SaveAcc(1, 0, 2) Squ_Diag(1) \
	Squ_SaveAcc(2, 0, 3) Squ_Acc(1, 2) Squ_NonDiag \
	Squ_SaveAcc(3, 1, 3) Squ_Diag(2) \
	Squ_SaveAcc(4, 2, 3) Squ_NonDiag \
	Squ_End(4)

#define Squ_8 \
	Squ_Begin(8) \
	Squ_SaveAcc(1, 0, 2) Squ_Diag(1) \
	Squ_SaveAcc(2, 0, 3) Squ_Acc(1, 2) Squ_NonDiag \
	Squ_SaveAcc(3, 0, 4) Squ_Acc(1, 3) Squ_Diag(2) \
	Squ_SaveAcc(4, 0, 5) Squ_Acc(1, 4) Squ_Acc(2, 3) Squ_NonDiag \
	Squ_SaveAcc(5, 0, 6) Squ_Acc(1, 5) Squ_Acc(2, 4) Squ_Diag(3) \
	Squ_SaveAcc(6, 0, 7) Squ_Acc(1, 6) Squ_Acc(2, 5) Squ_Acc(3, 4) Squ_NonDiag \
	Squ_SaveAcc(7, 1, 7) Squ_Acc(2, 6) Squ_Acc(3, 5) Squ_Diag(4) \
	Squ_SaveAcc(8, 2, 7) Squ_Acc(3, 6) Squ_Acc(4, 5)  Squ_NonDiag \
	Squ_SaveAcc(9, 3, 7) Squ_Acc(4, 6) Squ_Diag(5) \
	Squ_SaveAcc(10, 4, 7) Squ_Acc(5, 6) Squ_NonDiag \
	Squ_SaveAcc(11, 5, 7) Squ_Diag(6) \
	Squ_SaveAcc(12, 6, 7) Squ_NonDiag \
	Squ_End(8)

#define Squ_16 \
	Squ_Begin(16) \
	Squ_SaveAcc(1, 0, 2) Squ_Diag(1) \
	Squ_SaveAcc(2, 0, 3) Squ_Acc(1, 2) Squ_NonDiag \
	Squ_SaveAcc(3, 0, 4) Squ_Acc(1, 3) Squ_Diag(2) \
	Squ_SaveAcc(4, 0, 5) Squ_Acc(1, 4) Squ_Acc(2, 3) Squ_NonDiag \
	Squ_SaveAcc(5, 0, 6) Squ_Acc(1, 5) Squ_Acc(2, 4) Squ_Diag(3) \
	Squ_SaveAcc(6, 0, 7) Squ_Acc(1, 6) Squ_Acc(2, 5) Squ_Acc(3, 4) Squ_NonDiag \
	Squ_SaveAcc(7, 0, 8) Squ_Acc(1, 7) Squ_Acc(2, 6) Squ_Acc(3, 5) Squ_Diag(4) \
	Squ_SaveAcc(8, 0, 9) Squ_Acc(1, 8) Squ_Acc(2, 7) Squ_Acc(3, 6) Squ_Acc(4, 5) Squ_NonDiag \
	Squ_SaveAcc(9, 0, 10) Squ_Acc(1, 9) Squ_Acc(2, 8) Squ_Acc(3, 7) Squ_Acc(4, 6) Squ_Diag(5) \
	Squ_SaveAcc(10, 0, 11) Squ_Acc(1, 10) Squ_Acc(2, 9) Squ_Acc(3, 8) Squ_Acc(4, 7) Squ_Acc(5, 6) Squ_NonDiag \
	Squ_SaveAcc(11, 0, 12) Squ_Acc(1, 11) Squ_Acc(2, 10) Squ_Acc(3, 9) Squ_Acc(4, 8) Squ_Acc(5, 7) Squ_Diag(6) \
	Squ_SaveAcc(12, 0, 13) Squ_Acc(1, 12) Squ_Acc(2, 11) Squ_Acc(3, 10) Squ_Acc(4, 9) Squ_Acc(5, 8) Squ_Acc(6, 7) Squ_NonDiag \
	Squ_SaveAcc(13, 0, 14) Squ_Acc(1, 13) Squ_Acc(2, 12) Squ_Acc(3, 11) Squ_Acc(4, 10) Squ_Acc(5, 9) Squ_Acc(6, 8) Squ_Diag(7) \
	Squ_SaveAcc(14, 0, 15) Squ_Acc(1, 14) Squ_Acc(2, 13) Squ_Acc(3, 12) Squ_Acc(4, 11) Squ_Acc(5, 10) Squ_Acc(6, 9) Squ_Acc(7, 8) Squ_NonDiag \
	Squ_SaveAcc(15, 1, 15) Squ_Acc(2, 14) Squ_Acc(3, 13) Squ_Acc(4, 12) Squ_Acc(5, 11) Squ_Acc(6, 10) Squ_Acc(7, 9) Squ_Diag(8) \
	Squ_SaveAcc(16, 2, 15) Squ_Acc(3, 14) Squ_Acc(4, 13) Squ_Acc(5, 12) Squ_Acc(6, 11) Squ_Acc(7, 10) Squ_Acc(8, 9) Squ_NonDiag \
	Squ_SaveAcc(17, 3, 15) Squ_Acc(4, 14) Squ_Acc(5, 13) Squ_Acc(6, 12) Squ_Acc(7, 11) Squ_Acc(8, 10) Squ_Diag(9) \
	Squ_SaveAcc(18, 4, 15) Squ_Acc(5, 14) Squ_Acc(6, 13) Squ_Acc(7, 12) Squ_Acc(8, 11) Squ_Acc(9, 10) Squ_NonDiag \
	Squ_SaveAcc(19, 5, 15) Squ_Acc(6, 14) Squ_Acc(7, 13) Squ_Acc(8, 12) Squ_Acc(9, 11) Squ_Diag(10) \
	Squ_SaveAcc(20, 6, 15) Squ_Acc(7, 14) Squ_Acc(8, 13) Squ_Acc(9, 12) Squ_Acc(10, 11) Squ_NonDiag \
	Squ_SaveAcc(21, 7, 15) Squ_Acc(8, 14) Squ_Acc(9, 13) Squ_Acc(10, 12) Squ_Diag(11) \
	Squ_SaveAcc(22, 8, 15) Squ_Acc(9, 14) Squ_Acc(10, 13) Squ_Acc(11, 12) Squ_NonDiag \
	Squ_SaveAcc(23, 9, 15) Squ_Acc(10, 14) Squ_Acc(11, 13) Squ_Diag(12) \
	Squ_SaveAcc(24, 10, 15) Squ_Acc(11, 14) Squ_Acc(12, 13) Squ_NonDiag \
	Squ_SaveAcc(25, 11, 15) Squ_Acc(12, 14) Squ_Diag(13) \
	Squ_SaveAcc(26, 12, 15) Squ_Acc(13, 14) Squ_NonDiag \
	Squ_SaveAcc(27, 13, 15) Squ_Diag(14) \
	Squ_SaveAcc(28, 14, 15) Squ_NonDiag \
	Squ_End(16)

#define Bot_2 \
	Mul_Begin(2) \
	Bot_SaveAcc(0, 0, 1) Bot_Acc(1, 0) \
	Bot_End(2)

#define Bot_4 \
	Mul_Begin(4) \
	Mul_SaveAcc(0, 0, 1) Mul_Acc(1, 0) \
	Mul_SaveAcc(1, 2, 0) Mul_Acc(1, 1) Mul_Acc(0, 2)  \
	Bot_SaveAcc(2, 0, 3) Bot_Acc(1, 2) Bot_Acc(2, 1) Bot_Acc(3, 0)  \
	Bot_End(4)

#define Bot_8 \
	Mul_Begin(8) \
	Mul_SaveAcc(0, 0, 1) Mul_Acc(1, 0) \
	Mul_SaveAcc(1, 0, 2) Mul_Acc(1, 1) Mul_Acc(2, 0)  \
	Mul_SaveAcc(2, 0, 3) Mul_Acc(1, 2) Mul_Acc(2, 1) Mul_Acc(3, 0)  \
	Mul_SaveAcc(3, 0, 4) Mul_Acc(1, 3) Mul_Acc(2, 2) Mul_Acc(3, 1) Mul_Acc(4, 0) \
	Mul_SaveAcc(4, 0, 5) Mul_Acc(1, 4) Mul_Acc(2, 3) Mul_Acc(3, 2) Mul_Acc(4, 1) Mul_Acc(5, 0) \
	Mul_SaveAcc(5, 0, 6) Mul_Acc(1, 5) Mul_Acc(2, 4) Mul_Acc(3, 3) Mul_Acc(4, 2) Mul_Acc(5, 1) Mul_Acc(6, 0) \
	Bot_SaveAcc(6, 0, 7) Bot_Acc(1, 6) Bot_Acc(2, 5) Bot_Acc(3, 4) Bot_Acc(4, 3) Bot_Acc(5, 2) Bot_Acc(6, 1) Bot_Acc(7, 0) \
	Bot_End(8)

#define Bot_16 \
	Mul_Begin(16) \
	Mul_SaveAcc(0, 0, 1) Mul_Acc(1, 0) \
	Mul_SaveAcc(1, 0, 2) Mul_Acc(1, 1) Mul_Acc(2, 0) \
	Mul_SaveAcc(2, 0, 3) Mul_Acc(1, 2) Mul_Acc(2, 1) Mul_Acc(3, 0) \
	Mul_SaveAcc(3, 0, 4) Mul_Acc(1, 3) Mul_Acc(2, 2) Mul_Acc(3, 1) Mul_Acc(4, 0) \
	Mul_SaveAcc(4, 0, 5) Mul_Acc(1, 4) Mul_Acc(2, 3) Mul_Acc(3, 2) Mul_Acc(4, 1) Mul_Acc(5, 0) \
	Mul_SaveAcc(5, 0, 6) Mul_Acc(1, 5) Mul_Acc(2, 4) Mul_Acc(3, 3) Mul_Acc(4, 2) Mul_Acc(5, 1) Mul_Acc(6, 0) \
	Mul_SaveAcc(6, 0, 7) Mul_Acc(1, 6) Mul_Acc(2, 5) Mul_Acc(3, 4) Mul_Acc(4, 3) Mul_Acc(5, 2) Mul_Acc(6, 1) Mul_Acc(7, 0) \
	Mul_SaveAcc(7, 0, 8) Mul_Acc(1, 7) Mul_Acc(2, 6) Mul_Acc(3, 5) Mul_Acc(4, 4) Mul_Acc(5, 3) Mul_Acc(6, 2) Mul_Acc(7, 1) Mul_Acc(8, 0) \
	Mul_SaveAcc(8, 0, 9) Mul_Acc(1, 8) Mul_Acc(2, 7) Mul_Acc(3, 6) Mul_Acc(4, 5) Mul_Acc(5, 4) Mul_Acc(6, 3) Mul_Acc(7, 2) Mul_Acc(8, 1) Mul_Acc(9, 0) \
	Mul_SaveAcc(9, 0, 10) Mul_Acc(1, 9) Mul_Acc(2, 8) Mul_Acc(3, 7) Mul_Acc(4, 6) Mul_Acc(5, 5) Mul_Acc(6, 4) Mul_Acc(7, 3) Mul_Acc(8, 2) Mul_Acc(9, 1) Mul_Acc(10, 0) \
	Mul_SaveAcc(10, 0, 11) Mul_Acc(1, 10) Mul_Acc(2, 9) Mul_Acc(3, 8) Mul_Acc(4, 7) Mul_Acc(5, 6) Mul_Acc(6, 5) Mul_Acc(7, 4) Mul_Acc(8, 3) Mul_Acc(9, 2) Mul_Acc(10, 1) Mul_Acc(11, 0) \
	Mul_SaveAcc(11, 0, 12) Mul_Acc(1, 11) Mul_Acc(2, 10) Mul_Acc(3, 9) Mul_Acc(4, 8) Mul_Acc(5, 7) Mul_Acc(6, 6) Mul_Acc(7, 5) Mul_Acc(8, 4) Mul_Acc(9, 3) Mul_Acc(10, 2) Mul_Acc(11, 1) Mul_Acc(12, 0) \
	Mul_SaveAcc(12, 0, 13) Mul_Acc(1, 12) Mul_Acc(2, 11) Mul_Acc(3, 10) Mul_Acc(4, 9) Mul_Acc(5, 8) Mul_Acc(6, 7) Mul_Acc(7, 6) Mul_Acc(8, 5) Mul_Acc(9, 4) Mul_Acc(10, 3) Mul_Acc(11, 2) Mul_Acc(12, 1) Mul_Acc(13, 0) \
	Mul_SaveAcc(13, 0, 14) Mul_Acc(1, 13) Mul_Acc(2, 12) Mul_Acc(3, 11) Mul_Acc(4, 10) Mul_Acc(5, 9) Mul_Acc(6, 8) Mul_Acc(7, 7) Mul_Acc(8, 6) Mul_Acc(9, 5) Mul_Acc(10, 4) Mul_Acc(11, 3) Mul_Acc(12, 2) Mul_Acc(13, 1) Mul_Acc(14, 0) \
	Bot_SaveAcc(14, 0, 15) Bot_Acc(1, 14) Bot_Acc(2, 13) Bot_Acc(3, 12) Bot_Acc(4, 11) Bot_Acc(5, 10) Bot_Acc(6, 9) Bot_Acc(7, 8) Bot_Acc(8, 7) Bot_Acc(9, 6) Bot_Acc(10, 5) Bot_Acc(11, 4) Bot_Acc(12, 3) Bot_Acc(13, 2) Bot_Acc(14, 1) Bot_Acc(15, 0) \
	Bot_End(16)
	
#endif

#if 0
#define Mul_Begin(n)				\
	Declare2Words(p)				\
	Declare2Words(c)				\
	Declare2Words(d)				\
	MultiplyWords(p, A[0], B[0])	\
	AssignWord(c, LowWord(p))		\
	AssignWord(d, HighWord(p))

#define Mul_Acc(i, j)				\
	MultiplyWords(p, A[i], B[j])	\
	Acc2WordsBy1(c, LowWord(p))		\
	Acc2WordsBy1(d, HighWord(p))

#define Mul_SaveAcc(k, i, j) 		\
	R[k] = LowWord(c);				\
	Add2WordsBy1(c, d, HighWord(c))	\
	MultiplyWords(p, A[i], B[j])	\
	AssignWord(d, HighWord(p))		\
	Acc2WordsBy1(c, LowWord(p))

#define Mul_End(n)					\
	R[2*n-3] = LowWord(c);			\
	Acc2WordsBy1(d, HighWord(c))	\
	MultiplyWords(p, A[n-1], B[n-1])\
	Acc2WordsBy2(d, p)				\
	R[2*n-2] = LowWord(d);			\
	R[2*n-1] = HighWord(d);

#define Bot_SaveAcc(k, i, j)		\
	R[k] = LowWord(c);				\
	word e = LowWord(d) + HighWord(c);	\
	e += A[i] * B[j];

#define Bot_Acc(i, j)	\
	e += A[i] * B[j];

#define Bot_End(n)		\
	R[n-1] = e;
#else
#define Mul_Begin(n)				\
	Declare2Words(p)				\
	word c;	\
	Declare2Words(d)				\
	MultiplyWords(p, A[0], B[0])	\
	c = LowWord(p);		\
	AssignWord(d, HighWord(p))

#define Mul_Acc(i, j)				\
	MulAcc(c, d, A[i], B[j])

#define Mul_SaveAcc(k, i, j) 		\
	R[k] = c;				\
	c = LowWord(d);	\
	AssignWord(d, HighWord(d))	\
	MulAcc(c, d, A[i], B[j])

#define Mul_End(k, i)					\
	R[k] = c;			\
	MultiplyWords(p, A[i], B[i])	\
	Acc2WordsBy2(p, d)				\
	R[k+1] = LowWord(p);			\
	R[k+2] = HighWord(p);

#define Bot_SaveAcc(k, i, j)		\
	R[k] = c;				\
	c = LowWord(d);	\
	c += A[i] * B[j];

#define Bot_Acc(i, j)	\
	c += A[i] * B[j];

#define Bot_End(n)		\
	R[n-1] = c;
#endif

#define Squ_Begin(n)				\
	Declare2Words(p)				\
	word c;				\
	Declare2Words(d)				\
	Declare2Words(e)				\
	MultiplyWords(p, A[0], A[0])	\
	R[0] = LowWord(p);				\
	AssignWord(e, HighWord(p))		\
	MultiplyWords(p, A[0], A[1])	\
	c = LowWord(p);		\
	AssignWord(d, HighWord(p))		\
	Squ_NonDiag						\

#define Squ_NonDiag				\
	Double3Words(c, d)

#define Squ_SaveAcc(k, i, j) 		\
	Acc3WordsBy2(c, d, e)			\
	R[k] = c;				\
	MultiplyWords(p, A[i], A[j])	\
	c = LowWord(p);		\
	AssignWord(d, HighWord(p))		\

#define Squ_Acc(i, j)				\
	MulAcc(c, d, A[i], A[j])

#define Squ_Diag(i)					\
	Squ_NonDiag						\
	MulAcc(c, d, A[i], A[i])

#define Squ_End(n)					\
	Acc3WordsBy2(c, d, e)			\
	R[2*n-3] = c;			\
	MultiplyWords(p, A[n-1], A[n-1])\
	Acc2WordsBy2(p, e)				\
	R[2*n-2] = LowWord(p);			\
	R[2*n-1] = HighWord(p);


void Baseline_Multiply2(word *R, const word *A, const word *B)
{
	Mul_2
}

void Baseline_Multiply4(word *R, const word *A, const word *B)
{
	Mul_4
}

void Baseline_Multiply8(word *R, const word *A, const word *B)
{
	Mul_8
}

void Baseline_Square2(word *R, const word *A)
{
	Squ_2
}

void Baseline_Square4(word *R, const word *A)
{
	Squ_4
}

void Baseline_Square8(word *R, const word *A)
{
	Squ_8
}

void Baseline_MultiplyBottom2(word *R, const word *A, const word *B)
{
	Bot_2
}

void Baseline_MultiplyBottom4(word *R, const word *A, const word *B)
{
	Bot_4
}

void Baseline_MultiplyBottom8(word *R, const word *A, const word *B)
{
	Bot_8
}

#define Top_Begin(n)				\
	Declare2Words(p)				\
	word c;	\
	Declare2Words(d)				\
	MultiplyWords(p, A[0], B[n-2]);\
	AssignWord(d, HighWord(p));

#define Top_Acc(i, j)	\
	MultiplyWords(p, A[i], B[j]);\
	Acc2WordsBy1(d, HighWord(p));

#define Top_SaveAcc0(i, j) 		\
	c = LowWord(d);	\
	AssignWord(d, HighWord(d))	\
	MulAcc(c, d, A[i], B[j])

#define Top_SaveAcc1(i, j) 		\
	c = L<c; \
	Acc2WordsBy1(d, c);	\
	c = LowWord(d);	\
	AssignWord(d, HighWord(d))	\
	MulAcc(c, d, A[i], B[j])

void Baseline_MultiplyTop2(word *R, const word *A, const word *B, word L)
{
	CRYPTOPP_UNUSED(L);
	word T[4];
	Baseline_Multiply2(T, A, B);
	R[0] = T[2];
	R[1] = T[3];
}

void Baseline_MultiplyTop4(word *R, const word *A, const word *B, word L)
{
	Top_Begin(4)
	Top_Acc(1, 1) Top_Acc(2, 0)  \
	Top_SaveAcc0(0, 3) Mul_Acc(1, 2) Mul_Acc(2, 1) Mul_Acc(3, 0)  \
	Top_SaveAcc1(1, 3) Mul_Acc(2, 2) Mul_Acc(3, 1)  \
	Mul_SaveAcc(0, 2, 3) Mul_Acc(3, 2) \
	Mul_End(1, 3)
}

void Baseline_MultiplyTop8(word *R, const word *A, const word *B, word L)
{
	Top_Begin(8)
	Top_Acc(1, 5) Top_Acc(2, 4) Top_Acc(3, 3) Top_Acc(4, 2) Top_Acc(5, 1) Top_Acc(6, 0) \
	Top_SaveAcc0(0, 7) Mul_Acc(1, 6) Mul_Acc(2, 5) Mul_Acc(3, 4) Mul_Acc(4, 3) Mul_Acc(5, 2) Mul_Acc(6, 1) Mul_Acc(7, 0) \
	Top_SaveAcc1(1, 7) Mul_Acc(2, 6) Mul_Acc(3, 5) Mul_Acc(4, 4) Mul_Acc(5, 3) Mul_Acc(6, 2) Mul_Acc(7, 1) \
	Mul_SaveAcc(0, 2, 7) Mul_Acc(3, 6) Mul_Acc(4, 5) Mul_Acc(5, 4) Mul_Acc(6, 3) Mul_Acc(7, 2) \
	Mul_SaveAcc(1, 3, 7) Mul_Acc(4, 6) Mul_Acc(5, 5) Mul_Acc(6, 4) Mul_Acc(7, 3) \
	Mul_SaveAcc(2, 4, 7) Mul_Acc(5, 6) Mul_Acc(6, 5) Mul_Acc(7, 4) \
	Mul_SaveAcc(3, 5, 7) Mul_Acc(6, 6) Mul_Acc(7, 5) \
	Mul_SaveAcc(4, 6, 7) Mul_Acc(7, 6) \
	Mul_End(5, 7)
}

#if !CRYPTOPP_INTEGER_SSE2	// save memory by not compiling these functions when SSE2 is available
void Baseline_Multiply16(word *R, const word *A, const word *B)
{
	Mul_16
}

void Baseline_Square16(word *R, const word *A)
{
	Squ_16
}

void Baseline_MultiplyBottom16(word *R, const word *A, const word *B)
{
	Bot_16
}

void Baseline_MultiplyTop16(word *R, const word *A, const word *B, word L)
{
	Top_Begin(16)
	Top_Acc(1, 13) Top_Acc(2, 12) Top_Acc(3, 11) Top_Acc(4, 10) Top_Acc(5, 9) Top_Acc(6, 8) Top_Acc(7, 7) Top_Acc(8, 6) Top_Acc(9, 5) Top_Acc(10, 4) Top_Acc(11, 3) Top_Acc(12, 2) Top_Acc(13, 1) Top_Acc(14, 0) \
	Top_SaveAcc0(0, 15) Mul_Acc(1, 14) Mul_Acc(2, 13) Mul_Acc(3, 12) Mul_Acc(4, 11) Mul_Acc(5, 10) Mul_Acc(6, 9) Mul_Acc(7, 8) Mul_Acc(8, 7) Mul_Acc(9, 6) Mul_Acc(10, 5) Mul_Acc(11, 4) Mul_Acc(12, 3) Mul_Acc(13, 2) Mul_Acc(14, 1) Mul_Acc(15, 0) \
	Top_SaveAcc1(1, 15) Mul_Acc(2, 14) Mul_Acc(3, 13) Mul_Acc(4, 12) Mul_Acc(5, 11) Mul_Acc(6, 10) Mul_Acc(7, 9) Mul_Acc(8, 8) Mul_Acc(9, 7) Mul_Acc(10, 6) Mul_Acc(11, 5) Mul_Acc(12, 4) Mul_Acc(13, 3) Mul_Acc(14, 2) Mul_Acc(15, 1) \
	Mul_SaveAcc(0, 2, 15) Mul_Acc(3, 14) Mul_Acc(4, 13) Mul_Acc(5, 12) Mul_Acc(6, 11) Mul_Acc(7, 10) Mul_Acc(8, 9) Mul_Acc(9, 8) Mul_Acc(10, 7) Mul_Acc(11, 6) Mul_Acc(12, 5) Mul_Acc(13, 4) Mul_Acc(14, 3) Mul_Acc(15, 2) \
	Mul_SaveAcc(1, 3, 15) Mul_Acc(4, 14) Mul_Acc(5, 13) Mul_Acc(6, 12) Mul_Acc(7, 11) Mul_Acc(8, 10) Mul_Acc(9, 9) Mul_Acc(10, 8) Mul_Acc(11, 7) Mul_Acc(12, 6) Mul_Acc(13, 5) Mul_Acc(14, 4) Mul_Acc(15, 3) \
	Mul_SaveAcc(2, 4, 15) Mul_Acc(5, 14) Mul_Acc(6, 13) Mul_Acc(7, 12) Mul_Acc(8, 11) Mul_Acc(9, 10) Mul_Acc(10, 9) Mul_Acc(11, 8) Mul_Acc(12, 7) Mul_Acc(13, 6) Mul_Acc(14, 5) Mul_Acc(15, 4) \
	Mul_SaveAcc(3, 5, 15) Mul_Acc(6, 14) Mul_Acc(7, 13) Mul_Acc(8, 12) Mul_Acc(9, 11) Mul_Acc(10, 10) Mul_Acc(11, 9) Mul_Acc(12, 8) Mul_Acc(13, 7) Mul_Acc(14, 6) Mul_Acc(15, 5) \
	Mul_SaveAcc(4, 6, 15) Mul_Acc(7, 14) Mul_Acc(8, 13) Mul_Acc(9, 12) Mul_Acc(10, 11) Mul_Acc(11, 10) Mul_Acc(12, 9) Mul_Acc(13, 8) Mul_Acc(14, 7) Mul_Acc(15, 6) \
	Mul_SaveAcc(5, 7, 15) Mul_Acc(8, 14) Mul_Acc(9, 13) Mul_Acc(10, 12) Mul_Acc(11, 11) Mul_Acc(12, 10) Mul_Acc(13, 9) Mul_Acc(14, 8) Mul_Acc(15, 7) \
	Mul_SaveAcc(6, 8, 15) Mul_Acc(9, 14) Mul_Acc(10, 13) Mul_Acc(11, 12) Mul_Acc(12, 11) Mul_Acc(13, 10) Mul_Acc(14, 9) Mul_Acc(15, 8) \
	Mul_SaveAcc(7, 9, 15) Mul_Acc(10, 14) Mul_Acc(11, 13) Mul_Acc(12, 12) Mul_Acc(13, 11) Mul_Acc(14, 10) Mul_Acc(15, 9) \
	Mul_SaveAcc(8, 10, 15) Mul_Acc(11, 14) Mul_Acc(12, 13) Mul_Acc(13, 12) Mul_Acc(14, 11) Mul_Acc(15, 10) \
	Mul_SaveAcc(9, 11, 15) Mul_Acc(12, 14) Mul_Acc(13, 13) Mul_Acc(14, 12) Mul_Acc(15, 11) \
	Mul_SaveAcc(10, 12, 15) Mul_Acc(13, 14) Mul_Acc(14, 13) Mul_Acc(15, 12) \
	Mul_SaveAcc(11, 13, 15) Mul_Acc(14, 14) Mul_Acc(15, 13) \
	Mul_SaveAcc(12, 14, 15) Mul_Acc(15, 14) \
	Mul_End(13, 15)
}
#endif

// ********************************************************

#if CRYPTOPP_INTEGER_SSE2

CRYPTOPP_ALIGN_DATA(16) static const word32 s_maskLow16[4] CRYPTOPP_SECTION_ALIGN16 = {0xffff,0xffff,0xffff,0xffff};

#undef Mul_Begin
#undef Mul_Acc
#undef Top_Begin
#undef Top_Acc
#undef Squ_Acc
#undef Squ_NonDiag
#undef Squ_Diag
#undef Squ_SaveAcc
#undef Squ_Begin
#undef Mul_SaveAcc
#undef Bot_Acc
#undef Bot_SaveAcc
#undef Bot_End
#undef Squ_End
#undef Mul_End

#define SSE2_FinalSave(k)			\
	AS2(	psllq		xmm5, 16)	\
	AS2(	paddq		xmm4, xmm5)	\
	AS2(	movq		QWORD PTR [ecx+8*(k)], xmm4)

#define SSE2_SaveShift(k)			\
	AS2(	movq		xmm0, xmm6)	\
	AS2(	punpckhqdq	xmm6, xmm0)	\
	AS2(	movq		xmm1, xmm7)	\
	AS2(	punpckhqdq	xmm7, xmm1)	\
	AS2(	paddd		xmm6, xmm0)	\
	AS2(	pslldq		xmm6, 4)	\
	AS2(	paddd		xmm7, xmm1)	\
	AS2(	paddd		xmm4, xmm6)	\
	AS2(	pslldq		xmm7, 4)	\
	AS2(	movq		xmm6, xmm4)	\
	AS2(	paddd		xmm5, xmm7)	\
	AS2(	movq		xmm7, xmm5)	\
	AS2(	movd		DWORD PTR [ecx+8*(k)], xmm4)	\
	AS2(	psrlq		xmm6, 16)	\
	AS2(	paddq		xmm6, xmm7)	\
	AS2(	punpckhqdq	xmm4, xmm0)	\
	AS2(	punpckhqdq	xmm5, xmm0)	\
	AS2(	movq		QWORD PTR [ecx+8*(k)+2], xmm6)	\
	AS2(	psrlq		xmm6, 3*16)	\
	AS2(	paddd		xmm4, xmm6)	\

#define Squ_SSE2_SaveShift(k)			\
	AS2(	movq		xmm0, xmm6)	\
	AS2(	punpckhqdq	xmm6, xmm0)	\
	AS2(	movq		xmm1, xmm7)	\
	AS2(	punpckhqdq	xmm7, xmm1)	\
	AS2(	paddd		xmm6, xmm0)	\
	AS2(	pslldq		xmm6, 4)	\
	AS2(	paddd		xmm7, xmm1)	\
	AS2(	paddd		xmm4, xmm6)	\
	AS2(	pslldq		xmm7, 4)	\
	AS2(	movhlps		xmm6, xmm4)	\
	AS2(	movd		DWORD PTR [ecx+8*(k)], xmm4)	\
	AS2(	paddd		xmm5, xmm7)	\
	AS2(	movhps		QWORD PTR [esp+12], xmm5)\
	AS2(	psrlq		xmm4, 16)	\
	AS2(	paddq		xmm4, xmm5)	\
	AS2(	movq		QWORD PTR [ecx+8*(k)+2], xmm4)	\
	AS2(	psrlq		xmm4, 3*16)	\
	AS2(	paddd		xmm4, xmm6)	\
	AS2(	movq		QWORD PTR [esp+4], xmm4)\

#define SSE2_FirstMultiply(i)				\
	AS2(	movdqa		xmm7, [esi+(i)*16])\
	AS2(	movdqa		xmm5, [edi-(i)*16])\
	AS2(	pmuludq		xmm5, xmm7)		\
	AS2(	movdqa		xmm4, [ebx])\
	AS2(	movdqa		xmm6, xmm4)		\
	AS2(	pand		xmm4, xmm5)		\
	AS2(	psrld		xmm5, 16)		\
	AS2(	pmuludq		xmm7, [edx-(i)*16])\
	AS2(	pand		xmm6, xmm7)		\
	AS2(	psrld		xmm7, 16)

#define Squ_Begin(n)							\
	SquPrologue									\
	AS2(	mov		esi, esp)\
	AS2(	and		esp, 0xfffffff0)\
	AS2(	lea		edi, [esp-32*n])\
	AS2(	sub		esp, 32*n+16)\
	AS1(	push	esi)\
	AS2(	mov		esi, edi)					\
	AS2(	xor		edx, edx)					\
	ASL(1)										\
	ASS(	pshufd	xmm0, [eax+edx], 3,1,2,0)	\
	ASS(	pshufd	xmm1, [eax+edx], 2,0,3,1)	\
	AS2(	movdqa	[edi+2*edx], xmm0)		\
	AS2(	psrlq	xmm0, 32)					\
	AS2(	movdqa	[edi+2*edx+16], xmm0)	\
	AS2(	movdqa	[edi+16*n+2*edx], xmm1)		\
	AS2(	psrlq	xmm1, 32)					\
	AS2(	movdqa	[edi+16*n+2*edx+16], xmm1)	\
	AS2(	add		edx, 16)					\
	AS2(	cmp		edx, 8*(n))					\
	ASJ(	jne,	1, b)						\
	AS2(	lea		edx, [edi+16*n])\
	SSE2_FirstMultiply(0)							\

#define Squ_Acc(i)								\
	ASL(LSqu##i)								\
	AS2(	movdqa		xmm1, [esi+(i)*16])	\
	AS2(	movdqa		xmm0, [edi-(i)*16])	\
	AS2(	movdqa		xmm2, [ebx])	\
	AS2(	pmuludq		xmm0, xmm1)				\
	AS2(	pmuludq		xmm1, [edx-(i)*16])	\
	AS2(	movdqa		xmm3, xmm2)			\
	AS2(	pand		xmm2, xmm0)			\
	AS2(	psrld		xmm0, 16)			\
	AS2(	paddd		xmm4, xmm2)			\
	AS2(	paddd		xmm5, xmm0)			\
	AS2(	pand		xmm3, xmm1)			\
	AS2(	psrld		xmm1, 16)			\
	AS2(	paddd		xmm6, xmm3)			\
	AS2(	paddd		xmm7, xmm1)		\

#define Squ_Acc1(i)		
#define Squ_Acc2(i)		ASC(call, LSqu##i)
#define Squ_Acc3(i)		Squ_Acc2(i)
#define Squ_Acc4(i)		Squ_Acc2(i)
#define Squ_Acc5(i)		Squ_Acc2(i)
#define Squ_Acc6(i)		Squ_Acc2(i)
#define Squ_Acc7(i)		Squ_Acc2(i)
#define Squ_Acc8(i)		Squ_Acc2(i)

#define SSE2_End(E, n)					\
	SSE2_SaveShift(2*(n)-3)			\
	AS2(	movdqa		xmm7, [esi+16])	\
	AS2(	movdqa		xmm0, [edi])	\
	AS2(	pmuludq		xmm0, xmm7)				\
	AS2(	movdqa		xmm2, [ebx])		\
	AS2(	pmuludq		xmm7, [edx])	\
	AS2(	movdqa		xmm6, xmm2)				\
	AS2(	pand		xmm2, xmm0)				\
	AS2(	psrld		xmm0, 16)				\
	AS2(	paddd		xmm4, xmm2)				\
	AS2(	paddd		xmm5, xmm0)				\
	AS2(	pand		xmm6, xmm7)				\
	AS2(	psrld		xmm7, 16)	\
	SSE2_SaveShift(2*(n)-2)			\
	SSE2_FinalSave(2*(n)-1)			\
	AS1(	pop		esp)\
	E

#define Squ_End(n)		SSE2_End(SquEpilogue, n)
#define Mul_End(n)		SSE2_End(MulEpilogue, n)
#define Top_End(n)		SSE2_End(TopEpilogue, n)

#define Squ_Column1(k, i)	\
	Squ_SSE2_SaveShift(k)					\
	AS2(	add			esi, 16)	\
	SSE2_FirstMultiply(1)\
	Squ_Acc##i(i)	\
	AS2(	paddd		xmm4, xmm4)		\
	AS2(	paddd		xmm5, xmm5)		\
	AS2(	movdqa		xmm3, [esi])				\
	AS2(	movq		xmm1, QWORD PTR [esi+8])	\
	AS2(	pmuludq		xmm1, xmm3)		\
	AS2(	pmuludq		xmm3, xmm3)		\
	AS2(	movdqa		xmm0, [ebx])\
	AS2(	movdqa		xmm2, xmm0)		\
	AS2(	pand		xmm0, xmm1)		\
	AS2(	psrld		xmm1, 16)		\
	AS2(	paddd		xmm6, xmm0)		\
	AS2(	paddd		xmm7, xmm1)		\
	AS2(	pand		xmm2, xmm3)		\
	AS2(	psrld		xmm3, 16)		\
	AS2(	paddd		xmm6, xmm6)		\
	AS2(	paddd		xmm7, xmm7)		\
	AS2(	paddd		xmm4, xmm2)		\
	AS2(	paddd		xmm5, xmm3)		\
	AS2(	movq		xmm0, QWORD PTR [esp+4])\
	AS2(	movq		xmm1, QWORD PTR [esp+12])\
	AS2(	paddd		xmm4, xmm0)\
	AS2(	paddd		xmm5, xmm1)\

#define Squ_Column0(k, i)	\
	Squ_SSE2_SaveShift(k)					\
	AS2(	add			edi, 16)	\
	AS2(	add			edx, 16)	\
	SSE2_FirstMultiply(1)\
	Squ_Acc##i(i)	\
	AS2(	paddd		xmm6, xmm6)		\
	AS2(	paddd		xmm7, xmm7)		\
	AS2(	paddd		xmm4, xmm4)		\
	AS2(	paddd		xmm5, xmm5)		\
	AS2(	movq		xmm0, QWORD PTR [esp+4])\
	AS2(	movq		xmm1, QWORD PTR [esp+12])\
	AS2(	paddd		xmm4, xmm0)\
	AS2(	paddd		xmm5, xmm1)\

#define SSE2_MulAdd45						\
	AS2(	movdqa		xmm7, [esi])	\
	AS2(	movdqa		xmm0, [edi])	\
	AS2(	pmuludq		xmm0, xmm7)				\
	AS2(	movdqa		xmm2, [ebx])		\
	AS2(	pmuludq		xmm7, [edx])	\
	AS2(	movdqa		xmm6, xmm2)				\
	AS2(	pand		xmm2, xmm0)				\
	AS2(	psrld		xmm0, 16)				\
	AS2(	paddd		xmm4, xmm2)				\
	AS2(	paddd		xmm5, xmm0)				\
	AS2(	pand		xmm6, xmm7)				\
	AS2(	psrld		xmm7, 16)

#define Mul_Begin(n)							\
	MulPrologue									\
	AS2(	mov		esi, esp)\
	AS2(	and		esp, 0xfffffff0)\
	AS2(	sub		esp, 48*n+16)\
	AS1(	push	esi)\
	AS2(	xor		edx, edx)					\
	ASL(1)										\
	ASS(	pshufd	xmm0, [eax+edx], 3,1,2,0)	\
	ASS(	pshufd	xmm1, [eax+edx], 2,0,3,1)	\
	ASS(	pshufd	xmm2, [edi+edx], 3,1,2,0)	\
	AS2(	movdqa	[esp+20+2*edx], xmm0)		\
	AS2(	psrlq	xmm0, 32)					\
	AS2(	movdqa	[esp+20+2*edx+16], xmm0)	\
	AS2(	movdqa	[esp+20+16*n+2*edx], xmm1)		\
	AS2(	psrlq	xmm1, 32)					\
	AS2(	movdqa	[esp+20+16*n+2*edx+16], xmm1)	\
	AS2(	movdqa	[esp+20+32*n+2*edx], xmm2)		\
	AS2(	psrlq	xmm2, 32)					\
	AS2(	movdqa	[esp+20+32*n+2*edx+16], xmm2)	\
	AS2(	add		edx, 16)					\
	AS2(	cmp		edx, 8*(n))					\
	ASJ(	jne,	1, b)						\
	AS2(	lea		edi, [esp+20])\
	AS2(	lea		edx, [esp+20+16*n])\
	AS2(	lea		esi, [esp+20+32*n])\
	SSE2_FirstMultiply(0)							\

#define Mul_Acc(i)								\
	ASL(LMul##i)										\
	AS2(	movdqa		xmm1, [esi+i/2*(1-(i-2*(i/2))*2)*16])	\
	AS2(	movdqa		xmm0, [edi-i/2*(1-(i-2*(i/2))*2)*16])	\
	AS2(	movdqa		xmm2, [ebx])	\
	AS2(	pmuludq		xmm0, xmm1)				\
	AS2(	pmuludq		xmm1, [edx-i/2*(1-(i-2*(i/2))*2)*16])	\
	AS2(	movdqa		xmm3, xmm2)			\
	AS2(	pand		xmm2, xmm0)			\
	AS2(	psrld		xmm0, 16)			\
	AS2(	paddd		xmm4, xmm2)			\
	AS2(	paddd		xmm5, xmm0)			\
	AS2(	pand		xmm3, xmm1)			\
	AS2(	psrld		xmm1, 16)			\
	AS2(	paddd		xmm6, xmm3)			\
	AS2(	paddd		xmm7, xmm1)		\

#define Mul_Acc1(i)		
#define Mul_Acc2(i)		ASC(call, LMul##i)
#define Mul_Acc3(i)		Mul_Acc2(i)
#define Mul_Acc4(i)		Mul_Acc2(i)
#define Mul_Acc5(i)		Mul_Acc2(i)
#define Mul_Acc6(i)		Mul_Acc2(i)
#define Mul_Acc7(i)		Mul_Acc2(i)
#define Mul_Acc8(i)		Mul_Acc2(i)
#define Mul_Acc9(i)		Mul_Acc2(i)
#define Mul_Acc10(i)	Mul_Acc2(i)
#define Mul_Acc11(i)	Mul_Acc2(i)
#define Mul_Acc12(i)	Mul_Acc2(i)
#define Mul_Acc13(i)	Mul_Acc2(i)
#define Mul_Acc14(i)	Mul_Acc2(i)
#define Mul_Acc15(i)	Mul_Acc2(i)
#define Mul_Acc16(i)	Mul_Acc2(i)

#define Mul_Column1(k, i)	\
	SSE2_SaveShift(k)					\
	AS2(	add			esi, 16)	\
	SSE2_MulAdd45\
	Mul_Acc##i(i)	\

#define Mul_Column0(k, i)	\
	SSE2_SaveShift(k)					\
	AS2(	add			edi, 16)	\
	AS2(	add			edx, 16)	\
	SSE2_MulAdd45\
	Mul_Acc##i(i)	\

#define Bot_Acc(i)							\
	AS2(	movdqa		xmm1, [esi+i/2*(1-(i-2*(i/2))*2)*16])	\
	AS2(	movdqa		xmm0, [edi-i/2*(1-(i-2*(i/2))*2)*16])	\
	AS2(	pmuludq		xmm0, xmm1)				\
	AS2(	pmuludq		xmm1, [edx-i/2*(1-(i-2*(i/2))*2)*16])		\
	AS2(	paddq		xmm4, xmm0)				\
	AS2(	paddd		xmm6, xmm1)

#define Bot_SaveAcc(k)					\
	SSE2_SaveShift(k)							\
	AS2(	add			edi, 16)	\
	AS2(	add			edx, 16)	\
	AS2(	movdqa		xmm6, [esi])	\
	AS2(	movdqa		xmm0, [edi])	\
	AS2(	pmuludq		xmm0, xmm6)				\
	AS2(	paddq		xmm4, xmm0)				\
	AS2(	psllq		xmm5, 16)				\
	AS2(	paddq		xmm4, xmm5)				\
	AS2(	pmuludq		xmm6, [edx])

#define Bot_End(n)							\
	AS2(	movhlps		xmm7, xmm6)			\
	AS2(	paddd		xmm6, xmm7)			\
	AS2(	psllq		xmm6, 32)			\
	AS2(	paddd		xmm4, xmm6)			\
	AS2(	movq		QWORD PTR [ecx+8*((n)-1)], xmm4)	\
	AS1(	pop		esp)\
	MulEpilogue

#define Top_Begin(n)							\
	TopPrologue									\
	AS2(	mov		edx, esp)\
	AS2(	and		esp, 0xfffffff0)\
	AS2(	sub		esp, 48*n+16)\
	AS1(	push	edx)\
	AS2(	xor		edx, edx)					\
	ASL(1)										\
	ASS(	pshufd	xmm0, [eax+edx], 3,1,2,0)	\
	ASS(	pshufd	xmm1, [eax+edx], 2,0,3,1)	\
	ASS(	pshufd	xmm2, [edi+edx], 3,1,2,0)	\
	AS2(	movdqa	[esp+20+2*edx], xmm0)		\
	AS2(	psrlq	xmm0, 32)					\
	AS2(	movdqa	[esp+20+2*edx+16], xmm0)	\
	AS2(	movdqa	[esp+20+16*n+2*edx], xmm1)		\
	AS2(	psrlq	xmm1, 32)					\
	AS2(	movdqa	[esp+20+16*n+2*edx+16], xmm1)	\
	AS2(	movdqa	[esp+20+32*n+2*edx], xmm2)		\
	AS2(	psrlq	xmm2, 32)					\
	AS2(	movdqa	[esp+20+32*n+2*edx+16], xmm2)	\
	AS2(	add		edx, 16)					\
	AS2(	cmp		edx, 8*(n))					\
	ASJ(	jne,	1, b)						\
	AS2(	mov		eax, esi)					\
	AS2(	lea		edi, [esp+20+00*n+16*(n/2-1)])\
	AS2(	lea		edx, [esp+20+16*n+16*(n/2-1)])\
	AS2(	lea		esi, [esp+20+32*n+16*(n/2-1)])\
	AS2(	pxor	xmm4, xmm4)\
	AS2(	pxor	xmm5, xmm5)

#define Top_Acc(i)							\
	AS2(	movq		xmm0, QWORD PTR [esi+i/2*(1-(i-2*(i/2))*2)*16+8])	\
	AS2(	pmuludq		xmm0, [edx-i/2*(1-(i-2*(i/2))*2)*16])	\
	AS2(	psrlq		xmm0, 48)				\
	AS2(	paddd		xmm5, xmm0)\

#define Top_Column0(i)	\
	AS2(	psllq		xmm5, 32)				\
	AS2(	add			edi, 16)	\
	AS2(	add			edx, 16)	\
	SSE2_MulAdd45\
	Mul_Acc##i(i)	\

#define Top_Column1(i)	\
	SSE2_SaveShift(0)					\
	AS2(	add			esi, 16)	\
	SSE2_MulAdd45\
	Mul_Acc##i(i)	\
	AS2(	shr			eax, 16)	\
	AS2(	movd		xmm0, eax)\
	AS2(	movd		xmm1, [ecx+4])\
	AS2(	psrld		xmm1, 16)\
	AS2(	pcmpgtd		xmm1, xmm0)\
	AS2(	psrld		xmm1, 31)\
	AS2(	paddd		xmm4, xmm1)\

void SSE2_Square4(word *C, const word *A)
{
	Squ_Begin(2)
	Squ_Column0(0, 1)
	Squ_End(2)
}

void SSE2_Square8(word *C, const word *A)
{
	Squ_Begin(4)
#ifndef __GNUC__
	ASJ(	jmp,	0, f)
	Squ_Acc(2)
	AS1(	ret) ASL(0)
#endif
	Squ_Column0(0, 1)
	Squ_Column1(1, 1)
	Squ_Column0(2, 2)
	Squ_Column1(3, 1)
	Squ_Column0(4, 1)
	Squ_End(4)
}

void SSE2_Square16(word *C, const word *A)
{
	Squ_Begin(8)
#ifndef __GNUC__
	ASJ(	jmp,	0, f)
	Squ_Acc(4) Squ_Acc(3) Squ_Acc(2)
	AS1(	ret) ASL(0)
#endif
	Squ_Column0(0, 1)
	Squ_Column1(1, 1)
	Squ_Column0(2, 2)
	Squ_Column1(3, 2)
	Squ_Column0(4, 3)
	Squ_Column1(5, 3)
	Squ_Column0(6, 4)
	Squ_Column1(7, 3)
	Squ_Column0(8, 3)
	Squ_Column1(9, 2)
	Squ_Column0(10, 2)
	Squ_Column1(11, 1)
	Squ_Column0(12, 1)
	Squ_End(8)
}

void SSE2_Square32(word *C, const word *A)
{
	Squ_Begin(16)
	ASJ(	jmp,	0, f)
	Squ_Acc(8) Squ_Acc(7) Squ_Acc(6) Squ_Acc(5) Squ_Acc(4) Squ_Acc(3) Squ_Acc(2)
	AS1(	ret) ASL(0)
	Squ_Column0(0, 1)
	Squ_Column1(1, 1)
	Squ_Column0(2, 2)
	Squ_Column1(3, 2)
	Squ_Column0(4, 3)
	Squ_Column1(5, 3)
	Squ_Column0(6, 4)
	Squ_Column1(7, 4)
	Squ_Column0(8, 5)
	Squ_Column1(9, 5)
	Squ_Column0(10, 6)
	Squ_Column1(11, 6)
	Squ_Column0(12, 7)
	Squ_Column1(13, 7)
	Squ_Column0(14, 8)
	Squ_Column1(15, 7)
	Squ_Column0(16, 7)
	Squ_Column1(17, 6)
	Squ_Column0(18, 6)
	Squ_Column1(19, 5)
	Squ_Column0(20, 5)
	Squ_Column1(21, 4)
	Squ_Column0(22, 4)
	Squ_Column1(23, 3)
	Squ_Column0(24, 3)
	Squ_Column1(25, 2)
	Squ_Column0(26, 2)
	Squ_Column1(27, 1)
	Squ_Column0(28, 1)
	Squ_End(16)
}

void SSE2_Multiply4(word *C, const word *A, const word *B)
{
	Mul_Begin(2)
#ifndef __GNUC__
	ASJ(	jmp,	0, f)
	Mul_Acc(2)
	AS1(	ret) ASL(0)
#endif
	Mul_Column0(0, 2)
	Mul_End(2)
}

void SSE2_Multiply8(word *C, const word *A, const word *B)
{
	Mul_Begin(4)
#ifndef __GNUC__
	ASJ(	jmp,	0, f)
	Mul_Acc(4) Mul_Acc(3) Mul_Acc(2)
	AS1(	ret) ASL(0)
#endif
	Mul_Column0(0, 2)
	Mul_Column1(1, 3)
	Mul_Column0(2, 4)
	Mul_Column1(3, 3)
	Mul_Column0(4, 2)
	Mul_End(4)
}

void SSE2_Multiply16(word *C, const word *A, const word *B)
{
	Mul_Begin(8)
#ifndef __GNUC__
	ASJ(	jmp,	0, f)
	Mul_Acc(8) Mul_Acc(7) Mul_Acc(6) Mul_Acc(5) Mul_Acc(4) Mul_Acc(3) Mul_Acc(2)
	AS1(	ret) ASL(0)
#endif
	Mul_Column0(0, 2)
	Mul_Column1(1, 3)
	Mul_Column0(2, 4)
	Mul_Column1(3, 5)
	Mul_Column0(4, 6)
	Mul_Column1(5, 7)
	Mul_Column0(6, 8)
	Mul_Column1(7, 7)
	Mul_Column0(8, 6)
	Mul_Column1(9, 5)
	Mul_Column0(10, 4)
	Mul_Column1(11, 3)
	Mul_Column0(12, 2)
	Mul_End(8)
}

void SSE2_Multiply32(word *C, const word *A, const word *B)
{
	Mul_Begin(16)
	ASJ(	jmp,	0, f)
	Mul_Acc(16) Mul_Acc(15) Mul_Acc(14) Mul_Acc(13) Mul_Acc(12) Mul_Acc(11) Mul_Acc(10) Mul_Acc(9) Mul_Acc(8) Mul_Acc(7) Mul_Acc(6) Mul_Acc(5) Mul_Acc(4) Mul_Acc(3) Mul_Acc(2)
	AS1(	ret) ASL(0)
	Mul_Column0(0, 2)
	Mul_Column1(1, 3)
	Mul_Column0(2, 4)
	Mul_Column1(3, 5)
	Mul_Column0(4, 6)
	Mul_Column1(5, 7)
	Mul_Column0(6, 8)
	Mul_Column1(7, 9)
	Mul_Column0(8, 10)
	Mul_Column1(9, 11)
	Mul_Column0(10, 12)
	Mul_Column1(11, 13)
	Mul_Column0(12, 14)
	Mul_Column1(13, 15)
	Mul_Column0(14, 16)
	Mul_Column1(15, 15)
	Mul_Column0(16, 14)
	Mul_Column1(17, 13)
	Mul_Column0(18, 12)
	Mul_Column1(19, 11)
	Mul_Column0(20, 10)
	Mul_Column1(21, 9)
	Mul_Column0(22, 8)
	Mul_Column1(23, 7)
	Mul_Column0(24, 6)
	Mul_Column1(25, 5)
	Mul_Column0(26, 4)
	Mul_Column1(27, 3)
	Mul_Column0(28, 2)
	Mul_End(16)
}

void SSE2_MultiplyBottom4(word *C, const word *A, const word *B)
{
	Mul_Begin(2)
	Bot_SaveAcc(0) Bot_Acc(2)
	Bot_End(2)
}

void SSE2_MultiplyBottom8(word *C, const word *A, const word *B)
{
	Mul_Begin(4)
#ifndef __GNUC__
	ASJ(	jmp,	0, f)
	Mul_Acc(3) Mul_Acc(2)
	AS1(	ret) ASL(0)
#endif
	Mul_Column0(0, 2)
	Mul_Column1(1, 3)
	Bot_SaveAcc(2) Bot_Acc(4) Bot_Acc(3) Bot_Acc(2)
	Bot_End(4)
}

void SSE2_MultiplyBottom16(word *C, const word *A, const word *B)
{
	Mul_Begin(8)
#ifndef __GNUC__
	ASJ(	jmp,	0, f)
	Mul_Acc(7) Mul_Acc(6) Mul_Acc(5) Mul_Acc(4) Mul_Acc(3) Mul_Acc(2)
	AS1(	ret) ASL(0)
#endif
	Mul_Column0(0, 2)
	Mul_Column1(1, 3)
	Mul_Column0(2, 4)
	Mul_Column1(3, 5)
	Mul_Column0(4, 6)
	Mul_Column1(5, 7)
	Bot_SaveAcc(6) Bot_Acc(8) Bot_Acc(7) Bot_Acc(6) Bot_Acc(5) Bot_Acc(4) Bot_Acc(3) Bot_Acc(2)
	Bot_End(8)
}

void SSE2_MultiplyBottom32(word *C, const word *A, const word *B)
{
	Mul_Begin(16)
#ifndef __GNUC__
	ASJ(	jmp,	0, f)
	Mul_Acc(15) Mul_Acc(14) Mul_Acc(13) Mul_Acc(12) Mul_Acc(11) Mul_Acc(10) Mul_Acc(9) Mul_Acc(8) Mul_Acc(7) Mul_Acc(6) Mul_Acc(5) Mul_Acc(4) Mul_Acc(3) Mul_Acc(2)
	AS1(	ret) ASL(0)
#endif
	Mul_Column0(0, 2)
	Mul_Column1(1, 3)
	Mul_Column0(2, 4)
	Mul_Column1(3, 5)
	Mul_Column0(4, 6)
	Mul_Column1(5, 7)
	Mul_Column0(6, 8)
	Mul_Column1(7, 9)
	Mul_Column0(8, 10)
	Mul_Column1(9, 11)
	Mul_Column0(10, 12)
	Mul_Column1(11, 13)
	Mul_Column0(12, 14)
	Mul_Column1(13, 15)
	Bot_SaveAcc(14) Bot_Acc(16) Bot_Acc(15) Bot_Acc(14) Bot_Acc(13) Bot_Acc(12) Bot_Acc(11) Bot_Acc(10) Bot_Acc(9) Bot_Acc(8) Bot_Acc(7) Bot_Acc(6) Bot_Acc(5) Bot_Acc(4) Bot_Acc(3) Bot_Acc(2)
	Bot_End(16)
}

void SSE2_MultiplyTop8(word *C, const word *A, const word *B, word L)
{
	Top_Begin(4)
	Top_Acc(3) Top_Acc(2) Top_Acc(1)
#ifndef __GNUC__
	ASJ(	jmp,	0, f)
	Mul_Acc(4) Mul_Acc(3) Mul_Acc(2)
	AS1(	ret) ASL(0)
#endif
	Top_Column0(4)
	Top_Column1(3)
	Mul_Column0(0, 2)
	Top_End(2)
}

void SSE2_MultiplyTop16(word *C, const word *A, const word *B, word L)
{
	Top_Begin(8)
	Top_Acc(7) Top_Acc(6) Top_Acc(5) Top_Acc(4) Top_Acc(3) Top_Acc(2) Top_Acc(1)
#ifndef __GNUC__
	ASJ(	jmp,	0, f)
	Mul_Acc(8) Mul_Acc(7) Mul_Acc(6) Mul_Acc(5) Mul_Acc(4) Mul_Acc(3) Mul_Acc(2)
	AS1(	ret) ASL(0)
#endif
	Top_Column0(8)
	Top_Column1(7)
	Mul_Column0(0, 6)
	Mul_Column1(1, 5)
	Mul_Column0(2, 4)
	Mul_Column1(3, 3)
	Mul_Column0(4, 2)
	Top_End(4)
}

void SSE2_MultiplyTop32(word *C, const word *A, const word *B, word L)
{
	Top_Begin(16)
	Top_Acc(15) Top_Acc(14) Top_Acc(13) Top_Acc(12) Top_Acc(11) Top_Acc(10) Top_Acc(9) Top_Acc(8) Top_Acc(7) Top_Acc(6) Top_Acc(5) Top_Acc(4) Top_Acc(3) Top_Acc(2) Top_Acc(1)
#ifndef __GNUC__
	ASJ(	jmp,	0, f)
	Mul_Acc(16) Mul_Acc(15) Mul_Acc(14) Mul_Acc(13) Mul_Acc(12) Mul_Acc(11) Mul_Acc(10) Mul_Acc(9) Mul_Acc(8) Mul_Acc(7) Mul_Acc(6) Mul_Acc(5) Mul_Acc(4) Mul_Acc(3) Mul_Acc(2)
	AS1(	ret) ASL(0)
#endif
	Top_Column0(16)
	Top_Column1(15)
	Mul_Column0(0, 14)
	Mul_Column1(1, 13)
	Mul_Column0(2, 12)
	Mul_Column1(3, 11)
	Mul_Column0(4, 10)
	Mul_Column1(5, 9)
	Mul_Column0(6, 8)
	Mul_Column1(7, 7)
	Mul_Column0(8, 6)
	Mul_Column1(9, 5)
	Mul_Column0(10, 4)
	Mul_Column1(11, 3)
	Mul_Column0(12, 2)
	Top_End(8)
}

#endif	// #if CRYPTOPP_INTEGER_SSE2

// ********************************************************

typedef int (CRYPTOPP_FASTCALL * PAdd)(size_t N, word *C, const word *A, const word *B);
typedef void (* PMul)(word *C, const word *A, const word *B);
typedef void (* PSqu)(word *C, const word *A);
typedef void (* PMulTop)(word *C, const word *A, const word *B, word L);

#if CRYPTOPP_INTEGER_SSE2
static PAdd s_pAdd = &Baseline_Add, s_pSub = &Baseline_Sub;
static size_t s_recursionLimit = 8;
#else
static const size_t s_recursionLimit = 16;
#endif

static PMul s_pMul[9], s_pBot[9];
static PSqu s_pSqu[9];
static PMulTop s_pTop[9];

static void SetFunctionPointers()
{
	s_pMul[0] = &Baseline_Multiply2;
	s_pBot[0] = &Baseline_MultiplyBottom2;
	s_pSqu[0] = &Baseline_Square2;
	s_pTop[0] = &Baseline_MultiplyTop2;
	s_pTop[1] = &Baseline_MultiplyTop4;

#if CRYPTOPP_INTEGER_SSE2
	if (HasSSE2())
	{
#if _MSC_VER != 1200 || defined(NDEBUG)
		if (IsP4())
		{
			s_pAdd = &SSE2_Add;
			s_pSub = &SSE2_Sub;
		}
#endif

		s_recursionLimit = 32;

		s_pMul[1] = &SSE2_Multiply4;
		s_pMul[2] = &SSE2_Multiply8;
		s_pMul[4] = &SSE2_Multiply16;
		s_pMul[8] = &SSE2_Multiply32;

		s_pBot[1] = &SSE2_MultiplyBottom4;
		s_pBot[2] = &SSE2_MultiplyBottom8;
		s_pBot[4] = &SSE2_MultiplyBottom16;
		s_pBot[8] = &SSE2_MultiplyBottom32;

		s_pSqu[1] = &SSE2_Square4;
		s_pSqu[2] = &SSE2_Square8;
		s_pSqu[4] = &SSE2_Square16;
		s_pSqu[8] = &SSE2_Square32;

		s_pTop[2] = &SSE2_MultiplyTop8;
		s_pTop[4] = &SSE2_MultiplyTop16;
		s_pTop[8] = &SSE2_MultiplyTop32;
	}
	else
#endif
	{
		s_pMul[1] = &Baseline_Multiply4;
		s_pMul[2] = &Baseline_Multiply8;

		s_pBot[1] = &Baseline_MultiplyBottom4;
		s_pBot[2] = &Baseline_MultiplyBottom8;

		s_pSqu[1] = &Baseline_Square4;
		s_pSqu[2] = &Baseline_Square8;

		s_pTop[2] = &Baseline_MultiplyTop8;

#if	!CRYPTOPP_INTEGER_SSE2
		s_pMul[4] = &Baseline_Multiply16;
		s_pBot[4] = &Baseline_MultiplyBottom16;
		s_pSqu[4] = &Baseline_Square16;
		s_pTop[4] = &Baseline_MultiplyTop16;
#endif
	}
}

inline int Add(word *C, const word *A, const word *B, size_t N)
{
#if CRYPTOPP_INTEGER_SSE2
	return s_pAdd(N, C, A, B);
#else
	return Baseline_Add(N, C, A, B);
#endif
}

inline int Subtract(word *C, const word *A, const word *B, size_t N)
{
#if CRYPTOPP_INTEGER_SSE2
	return s_pSub(N, C, A, B);
#else
	return Baseline_Sub(N, C, A, B);
#endif
}

// ********************************************************


#define A0		A
#define A1		(A+N2)
#define B0		B
#define B1		(B+N2)

#define T0		T
#define T1		(T+N2)
#define T2		(T+N)
#define T3		(T+N+N2)

#define R0		R
#define R1		(R+N2)
#define R2		(R+N)
#define R3		(R+N+N2)

// R[2*N] - result = A*B
// T[2*N] - temporary work space
// A[N] --- multiplier
// B[N] --- multiplicant

void RecursiveMultiply(word *R, word *T, const word *A, const word *B, size_t N)
{
	assert(N>=2 && N%2==0);

	if (N <= s_recursionLimit)
		s_pMul[N/4](R, A, B);
	else
	{
		const size_t N2 = N/2;

		size_t AN2 = Compare(A0, A1, N2) > 0 ?  0 : N2;
		Subtract(R0, A + AN2, A + (N2 ^ AN2), N2);

		size_t BN2 = Compare(B0, B1, N2) > 0 ?  0 : N2;
		Subtract(R1, B + BN2, B + (N2 ^ BN2), N2);

		RecursiveMultiply(R2, T2, A1, B1, N2);
		RecursiveMultiply(T0, T2, R0, R1, N2);
		RecursiveMultiply(R0, T2, A0, B0, N2);

		// now T[01] holds (A1-A0)*(B0-B1), R[01] holds A0*B0, R[23] holds A1*B1

		int c2 = Add(R2, R2, R1, N2);
		int c3 = c2;
		c2 += Add(R1, R2, R0, N2);
		c3 += Add(R2, R2, R3, N2);

		if (AN2 == BN2)
			c3 -= Subtract(R1, R1, T0, N);
		else
			c3 += Add(R1, R1, T0, N);

		c3 += Increment(R2, N2, c2);
		assert (c3 >= 0 && c3 <= 2);
		Increment(R3, N2, c3);
	}
}

// R[2*N] - result = A*A
// T[2*N] - temporary work space
// A[N] --- number to be squared

void RecursiveSquare(word *R, word *T, const word *A, size_t N)
{
	assert(N && N%2==0);

	if (N <= s_recursionLimit)
		s_pSqu[N/4](R, A);
	else
	{
		const size_t N2 = N/2;

		RecursiveSquare(R0, T2, A0, N2);
		RecursiveSquare(R2, T2, A1, N2);
		RecursiveMultiply(T0, T2, A0, A1, N2);

		int carry = Add(R1, R1, T0, N);
		carry += Add(R1, R1, T0, N);
		Increment(R3, N2, carry);
	}
}

// R[N] - bottom half of A*B
// T[3*N/2] - temporary work space
// A[N] - multiplier
// B[N] - multiplicant

void RecursiveMultiplyBottom(word *R, word *T, const word *A, const word *B, size_t N)
{
	assert(N>=2 && N%2==0);

	if (N <= s_recursionLimit)
		s_pBot[N/4](R, A, B);
	else
	{
		const size_t N2 = N/2;

		RecursiveMultiply(R, T, A0, B0, N2);
		RecursiveMultiplyBottom(T0, T1, A1, B0, N2);
		Add(R1, R1, T0, N2);
		RecursiveMultiplyBottom(T0, T1, A0, B1, N2);
		Add(R1, R1, T0, N2);
	}
}

// R[N] --- upper half of A*B
// T[2*N] - temporary work space
// L[N] --- lower half of A*B
// A[N] --- multiplier
// B[N] --- multiplicant

void MultiplyTop(word *R, word *T, const word *L, const word *A, const word *B, size_t N)
{
	assert(N>=2 && N%2==0);

	if (N <= s_recursionLimit)
		s_pTop[N/4](R, A, B, L[N-1]);
	else
	{
		const size_t N2 = N/2;

		size_t AN2 = Compare(A0, A1, N2) > 0 ?  0 : N2;
		Subtract(R0, A + AN2, A + (N2 ^ AN2), N2);

		size_t BN2 = Compare(B0, B1, N2) > 0 ?  0 : N2;
		Subtract(R1, B + BN2, B + (N2 ^ BN2), N2);

		RecursiveMultiply(T0, T2, R0, R1, N2);
		RecursiveMultiply(R0, T2, A1, B1, N2);

		// now T[01] holds (A1-A0)*(B0-B1) = A1*B0+A0*B1-A1*B1-A0*B0, R[01] holds A1*B1

		int t, c3;
		int c2 = Subtract(T2, L+N2, L, N2);

		if (AN2 == BN2)
		{
			c2 -= Add(T2, T2, T0, N2);
			t = (Compare(T2, R0, N2) == -1);
			c3 = t - Subtract(T2, T2, T1, N2);
		}
		else
		{
			c2 += Subtract(T2, T2, T0, N2);
			t = (Compare(T2, R0, N2) == -1);
			c3 = t + Add(T2, T2, T1, N2);
		}

		c2 += t;
		if (c2 >= 0)
			c3 += Increment(T2, N2, c2);
		else
			c3 -= Decrement(T2, N2, -c2);
		c3 += Add(R0, T2, R1, N2);

		assert (c3 >= 0 && c3 <= 2);
		Increment(R1, N2, c3);
	}
}

inline void Multiply(word *R, word *T, const word *A, const word *B, size_t N)
{
	RecursiveMultiply(R, T, A, B, N);
}

inline void Square(word *R, word *T, const word *A, size_t N)
{
	RecursiveSquare(R, T, A, N);
}

inline void MultiplyBottom(word *R, word *T, const word *A, const word *B, size_t N)
{
	RecursiveMultiplyBottom(R, T, A, B, N);
}

// R[NA+NB] - result = A*B
// T[NA+NB] - temporary work space
// A[NA] ---- multiplier
// B[NB] ---- multiplicant

void AsymmetricMultiply(word *R, word *T, const word *A, size_t NA, const word *B, size_t NB)
{
	if (NA == NB)
	{
		if (A == B)
			Square(R, T, A, NA);
		else
			Multiply(R, T, A, B, NA);

		return;
	}

	if (NA > NB)
	{
		std::swap(A, B);
		std::swap(NA, NB);
	}

	assert(NB % NA == 0);

	if (NA==2 && !A[1])
	{
		switch (A[0])
		{
		case 0:
			SetWords(R, 0, NB+2);
			return;
		case 1:
			CopyWords(R, B, NB);
			R[NB] = R[NB+1] = 0;
			return;
		default:
			R[NB] = LinearMultiply(R, B, A[0], NB);
			R[NB+1] = 0;
			return;
		}
	}

	size_t i;
	if ((NB/NA)%2 == 0)
	{
		Multiply(R, T, A, B, NA);
		CopyWords(T+2*NA, R+NA, NA);

		for (i=2*NA; i<NB; i+=2*NA)
			Multiply(T+NA+i, T, A, B+i, NA);
		for (i=NA; i<NB; i+=2*NA)
			Multiply(R+i, T, A, B+i, NA);
	}
	else
	{
		for (i=0; i<NB; i+=2*NA)
			Multiply(R+i, T, A, B+i, NA);
		for (i=NA; i<NB; i+=2*NA)
			Multiply(T+NA+i, T, A, B+i, NA);
	}

	if (Add(R+NA, R+NA, T+2*NA, NB-NA))
		Increment(R+NB, NA);
}

// R[N] ----- result = A inverse mod 2**(WORD_BITS*N)
// T[3*N/2] - temporary work space
// A[N] ----- an odd number as input

void RecursiveInverseModPower2(word *R, word *T, const word *A, size_t N)
{
	if (N==2)
	{
		T[0] = AtomicInverseModPower2(A[0]);
		T[1] = 0;
		s_pBot[0](T+2, T, A);
		TwosComplement(T+2, 2);
		Increment(T+2, 2, 2);
		s_pBot[0](R, T, T+2);
	}
	else
	{
		const size_t N2 = N/2;
		RecursiveInverseModPower2(R0, T0, A0, N2);
		T0[0] = 1;
		SetWords(T0+1, 0, N2-1);
		MultiplyTop(R1, T1, T0, R0, A0, N2);
		MultiplyBottom(T0, T1, R0, A1, N2);
		Add(T0, R1, T0, N2);
		TwosComplement(T0, N2);
		MultiplyBottom(R1, T1, R0, T0, N2);
	}
}

// R[N] --- result = X/(2**(WORD_BITS*N)) mod M
// T[3*N] - temporary work space
// X[2*N] - number to be reduced
// M[N] --- modulus
// U[N] --- multiplicative inverse of M mod 2**(WORD_BITS*N)

void MontgomeryReduce(word *R, word *T, word *X, const word *M, const word *U, size_t N)
{
#if 1
	MultiplyBottom(R, T, X, U, N);
	MultiplyTop(T, T+N, X, R, M, N);
	word borrow = Subtract(T, X+N, T, N);
	// defend against timing attack by doing this Add even when not needed
	word carry = Add(T+N, T, M, N);
	assert(carry | !borrow);
	CRYPTOPP_UNUSED(carry), CRYPTOPP_UNUSED(borrow);
	CopyWords(R, T + ((0-borrow) & N), N);
#elif 0
	const word u = 0-U[0];
	Declare2Words(p)
	for (size_t i=0; i<N; i++)
	{
		const word t = u * X[i];
		word c = 0;
		for (size_t j=0; j<N; j+=2)
		{
			MultiplyWords(p, t, M[j]);
			Acc2WordsBy1(p, X[i+j]);
			Acc2WordsBy1(p, c);
			X[i+j] = LowWord(p);
			c = HighWord(p);
			MultiplyWords(p, t, M[j+1]);
			Acc2WordsBy1(p, X[i+j+1]);
			Acc2WordsBy1(p, c);
			X[i+j+1] = LowWord(p);
			c = HighWord(p);
		}

		if (Increment(X+N+i, N-i, c))
			while (!Subtract(X+N, X+N, M, N)) {}
	}

	memcpy(R, X+N, N*WORD_SIZE);
#else
	__m64 u = _mm_cvtsi32_si64(0-U[0]), p;
	for (size_t i=0; i<N; i++)
	{
		__m64 t = _mm_cvtsi32_si64(X[i]);
		t = _mm_mul_su32(t, u);
		__m64 c = _mm_setzero_si64();
		for (size_t j=0; j<N; j+=2)
		{
			p = _mm_mul_su32(t, _mm_cvtsi32_si64(M[j]));
			p = _mm_add_si64(p, _mm_cvtsi32_si64(X[i+j]));
			c = _mm_add_si64(c, p);
			X[i+j] = _mm_cvtsi64_si32(c);
			c = _mm_srli_si64(c, 32);
			p = _mm_mul_su32(t, _mm_cvtsi32_si64(M[j+1]));
			p = _mm_add_si64(p, _mm_cvtsi32_si64(X[i+j+1]));
			c = _mm_add_si64(c, p);
			X[i+j+1] = _mm_cvtsi64_si32(c);
			c = _mm_srli_si64(c, 32);
		}

		if (Increment(X+N+i, N-i, _mm_cvtsi64_si32(c)))
			while (!Subtract(X+N, X+N, M, N)) {}
	}

	memcpy(R, X+N, N*WORD_SIZE);
	_mm_empty();
#endif
}

// R[N] --- result = X/(2**(WORD_BITS*N/2)) mod M
// T[2*N] - temporary work space
// X[2*N] - number to be reduced
// M[N] --- modulus
// U[N/2] - multiplicative inverse of M mod 2**(WORD_BITS*N/2)
// V[N] --- 2**(WORD_BITS*3*N/2) mod M

void HalfMontgomeryReduce(word *R, word *T, const word *X, const word *M, const word *U, const word *V, size_t N)
{
	assert(N%2==0 && N>=4);

#define M0		M
#define M1		(M+N2)
#define V0		V
#define V1		(V+N2)

#define X0		X
#define X1		(X+N2)
#define X2		(X+N)
#define X3		(X+N+N2)

	const size_t N2 = N/2;
	Multiply(T0, T2, V0, X3, N2);
	int c2 = Add(T0, T0, X0, N);
	MultiplyBottom(T3, T2, T0, U, N2);
	MultiplyTop(T2, R, T0, T3, M0, N2);
	c2 -= Subtract(T2, T1, T2, N2);
	Multiply(T0, R, T3, M1, N2);
	c2 -= Subtract(T0, T2, T0, N2);
	int c3 = -(int)Subtract(T1, X2, T1, N2);
	Multiply(R0, T2, V1, X3, N2);
	c3 += Add(R, R, T, N);

	if (c2>0)
		c3 += Increment(R1, N2);
	else if (c2<0)
		c3 -= Decrement(R1, N2, -c2);

	assert(c3>=-1 && c3<=1);
	if (c3>0)
		Subtract(R, R, M, N);
	else if (c3<0)
		Add(R, R, M, N);

#undef M0
#undef M1
#undef V0
#undef V1

#undef X0
#undef X1
#undef X2
#undef X3
}

#undef A0
#undef A1
#undef B0
#undef B1

#undef T0
#undef T1
#undef T2
#undef T3

#undef R0
#undef R1
#undef R2
#undef R3

/*
// do a 3 word by 2 word divide, returns quotient and leaves remainder in A
static word SubatomicDivide(word *A, word B0, word B1)
{
	// assert {A[2],A[1]} < {B1,B0}, so quotient can fit in a word
	assert(A[2] < B1 || (A[2]==B1 && A[1] < B0));

	// estimate the quotient: do a 2 word by 1 word divide
	word Q;
	if (B1+1 == 0)
		Q = A[2];
	else
		Q = DWord(A[1], A[2]).DividedBy(B1+1);

	// now subtract Q*B from A
	DWord p = DWord::Multiply(B0, Q);
	DWord u = (DWord) A[0] - p.GetLowHalf();
	A[0] = u.GetLowHalf();
	u = (DWord) A[1] - p.GetHighHalf() - u.GetHighHalfAsBorrow() - DWord::Multiply(B1, Q);
	A[1] = u.GetLowHalf();
	A[2] += u.GetHighHalf();

	// Q <= actual quotient, so fix it
	while (A[2] || A[1] > B1 || (A[1]==B1 && A[0]>=B0))
	{
		u = (DWord) A[0] - B0;
		A[0] = u.GetLowHalf();
		u = (DWord) A[1] - B1 - u.GetHighHalfAsBorrow();
		A[1] = u.GetLowHalf();
		A[2] += u.GetHighHalf();
		Q++;
		assert(Q);	// shouldn't overflow
	}

	return Q;
}

// do a 4 word by 2 word divide, returns 2 word quotient in Q0 and Q1
static inline void AtomicDivide(word *Q, const word *A, const word *B)
{
	if (!B[0] && !B[1]) // if divisor is 0, we assume divisor==2**(2*WORD_BITS)
	{
		Q[0] = A[2];
		Q[1] = A[3];
	}
	else
	{
		word T[4];
		T[0] = A[0]; T[1] = A[1]; T[2] = A[2]; T[3] = A[3];
		Q[1] = SubatomicDivide(T+1, B[0], B[1]);
		Q[0] = SubatomicDivide(T, B[0], B[1]);

#ifndef NDEBUG
		// multiply quotient and divisor and add remainder, make sure it equals dividend
		assert(!T[2] && !T[3] && (T[1] < B[1] || (T[1]==B[1] && T[0]<B[0])));
		word P[4];
		LowLevel::Multiply2(P, Q, B);
		Add(P, P, T, 4);
		assert(memcmp(P, A, 4*WORD_SIZE)==0);
#endif
	}
}
*/

static inline void AtomicDivide(word *Q, const word *A, const word *B)
{
	word T[4];
	DWord q = DivideFourWordsByTwo<word, DWord>(T, DWord(A[0], A[1]), DWord(A[2], A[3]), DWord(B[0], B[1]));
	Q[0] = q.GetLowHalf();
	Q[1] = q.GetHighHalf();

#ifndef NDEBUG
	if (B[0] || B[1])
	{
		// multiply quotient and divisor and add remainder, make sure it equals dividend
		assert(!T[2] && !T[3] && (T[1] < B[1] || (T[1]==B[1] && T[0]<B[0])));
		word P[4];
		s_pMul[0](P, Q, B);
		Add(P, P, T, 4);
		assert(memcmp(P, A, 4*WORD_SIZE)==0);
	}
#endif
}

// for use by Divide(), corrects the underestimated quotient {Q1,Q0}
static void CorrectQuotientEstimate(word *R, word *T, word *Q, const word *B, size_t N)
{
	assert(N && N%2==0);

	AsymmetricMultiply(T, T+N+2, Q, 2, B, N);

	word borrow = Subtract(R, R, T, N+2);
	assert(!borrow && !R[N+1]);
	CRYPTOPP_UNUSED(borrow);

	while (R[N] || Compare(R, B, N) >= 0)
	{
		R[N] -= Subtract(R, R, B, N);
		Q[1] += (++Q[0]==0);
		assert(Q[0] || Q[1]); // no overflow
	}
}

// R[NB] -------- remainder = A%B
// Q[NA-NB+2] --- quotient	= A/B
// T[NA+3*(NB+2)] - temp work space
// A[NA] -------- dividend
// B[NB] -------- divisor

void Divide(word *R, word *Q, word *T, const word *A, size_t NA, const word *B, size_t NB)
{
	assert(NA && NB && NA%2==0 && NB%2==0);
	assert(B[NB-1] || B[NB-2]);
	assert(NB <= NA);

	// set up temporary work space
	word *const TA=T;
	word *const TB=T+NA+2;
	word *const TP=T+NA+2+NB;

	// copy B into TB and normalize it so that TB has highest bit set to 1
	unsigned shiftWords = (B[NB-1]==0);
	TB[0] = TB[NB-1] = 0;
	CopyWords(TB+shiftWords, B, NB-shiftWords);
	unsigned shiftBits = WORD_BITS - BitPrecision(TB[NB-1]);
	assert(shiftBits < WORD_BITS);
	ShiftWordsLeftByBits(TB, NB, shiftBits);

	// copy A into TA and normalize it
	TA[0] = TA[NA] = TA[NA+1] = 0;
	CopyWords(TA+shiftWords, A, NA);
	ShiftWordsLeftByBits(TA, NA+2, shiftBits);

	if (TA[NA+1]==0 && TA[NA] <= 1)
	{
		Q[NA-NB+1] = Q[NA-NB] = 0;
		while (TA[NA] || Compare(TA+NA-NB, TB, NB) >= 0)
		{
			TA[NA] -= Subtract(TA+NA-NB, TA+NA-NB, TB, NB);
			++Q[NA-NB];
		}
	}
	else
	{
		NA+=2;
		assert(Compare(TA+NA-NB, TB, NB) < 0);
	}

	word BT[2];
	BT[0] = TB[NB-2] + 1;
	BT[1] = TB[NB-1] + (BT[0]==0);

	// start reducing TA mod TB, 2 words at a time
	for (size_t i=NA-2; i>=NB; i-=2)
	{
		AtomicDivide(Q+i-NB, TA+i-2, BT);
		CorrectQuotientEstimate(TA+i-NB, TP, Q+i-NB, TB, NB);
	}

	// copy TA into R, and denormalize it
	CopyWords(R, TA+shiftWords, NB);
	ShiftWordsRightByBits(R, NB, shiftBits);
}

static inline size_t EvenWordCount(const word *X, size_t N)
{
	while (N && X[N-2]==0 && X[N-1]==0)
		N-=2;
	return N;
}

// return k
// R[N] --- result = A^(-1) * 2^k mod M
// T[4*N] - temporary work space
// A[NA] -- number to take inverse of
// M[N] --- modulus

unsigned int AlmostInverse(word *R, word *T, const word *A, size_t NA, const word *M, size_t N)
{
	assert(NA<=N && N && N%2==0);

	word *b = T;
	word *c = T+N;
	word *f = T+2*N;
	word *g = T+3*N;
	size_t bcLen=2, fgLen=EvenWordCount(M, N);
	unsigned int k=0;
	bool s=false;

	SetWords(T, 0, 3*N);
	b[0]=1;
	CopyWords(f, A, NA);
	CopyWords(g, M, N);

	while (1)
	{
		word t=f[0];
		while (!t)
		{
			if (EvenWordCount(f, fgLen)==0)
			{
				SetWords(R, 0, N);
				return 0;
			}

			ShiftWordsRightByWords(f, fgLen, 1);
			bcLen += 2 * (c[bcLen-1] != 0);
			assert(bcLen <= N);
			ShiftWordsLeftByWords(c, bcLen, 1);
			k+=WORD_BITS;
			t=f[0];
		}

		// t must be non-0; otherwise undefined.
		unsigned int i = TrailingZeros(t);
		t >>= i;
		k += i;

		if (t==1 && f[1]==0 && EvenWordCount(f+2, fgLen-2)==0)
		{
			if (s)
				Subtract(R, M, b, N);
			else
				CopyWords(R, b, N);
			return k;
		}

		ShiftWordsRightByBits(f, fgLen, i);
		t = ShiftWordsLeftByBits(c, bcLen, i);
		c[bcLen] += t;
		bcLen += 2 * (t!=0);
		assert(bcLen <= N);

		bool swap = Compare(f, g, fgLen)==-1;
		ConditionalSwapPointers(swap, f, g);
		ConditionalSwapPointers(swap, b, c);
		s ^= swap;

		fgLen -= 2 * !(f[fgLen-2] | f[fgLen-1]);

		Subtract(f, f, g, fgLen);
		t = Add(b, b, c, bcLen);
		b[bcLen] += t;
		bcLen += 2*t;
		assert(bcLen <= N);
	}
}

// R[N] - result = A/(2^k) mod M
// A[N] - input
// M[N] - modulus

void DivideByPower2Mod(word *R, const word *A, size_t k, const word *M, size_t N)
{
	CopyWords(R, A, N);

	while (k--)
	{
		if (R[0]%2==0)
			ShiftWordsRightByBits(R, N, 1);
		else
		{
			word carry = Add(R, R, M, N);
			ShiftWordsRightByBits(R, N, 1);
			R[N-1] += carry<<(WORD_BITS-1);
		}
	}
}

// R[N] - result = A*(2^k) mod M
// A[N] - input
// M[N] - modulus

void MultiplyByPower2Mod(word *R, const word *A, size_t k, const word *M, size_t N)
{
	CopyWords(R, A, N);

	while (k--)
		if (ShiftWordsLeftByBits(R, N, 1) || Compare(R, M, N)>=0)
			Subtract(R, R, M, N);
}

// ******************************************************************

InitializeInteger::InitializeInteger()
{
	if (!g_pAssignIntToInteger)
	{
		SetFunctionPointers();
		g_pAssignIntToInteger = AssignIntToInteger;
	}
}

static const unsigned int RoundupSizeTable[] = {2, 2, 2, 4, 4, 8, 8, 8, 8};

static inline size_t RoundupSize(size_t n)
{
	if (n<=8)
		return RoundupSizeTable[n];
	else if (n<=16)
		return 16;
	else if (n<=32)
		return 32;
	else if (n<=64)
		return 64;
	else return size_t(1) << BitPrecision(n-1);
}

Integer::Integer()
	: reg(2), sign(POSITIVE)
{
	reg[0] = reg[1] = 0;
}

Integer::Integer(const Integer& t)
	: reg(RoundupSize(t.WordCount())), sign(t.sign)
{
	CopyWords(reg, t.reg, reg.size());
}

Integer::Integer(Sign s, lword value)
	: reg(2), sign(s)
{
	reg[0] = word(value);
	reg[1] = word(SafeRightShift<WORD_BITS>(value));
}

Integer::Integer(signed long value)
	: reg(2)
{
	if (value >= 0)
		sign = POSITIVE;
	else
	{
		sign = NEGATIVE;
		value = -value;
	}
	reg[0] = word(value);
	reg[1] = word(SafeRightShift<WORD_BITS>((unsigned long)value));
}

Integer::Integer(Sign s, word high, word low)
	: reg(2), sign(s)
{
	reg[0] = low;
	reg[1] = high;
}

bool Integer::IsConvertableToLong() const
{
	if (ByteCount() > sizeof(long))
		return false;

	unsigned long value = (unsigned long)reg[0];
	value += SafeLeftShift<WORD_BITS, unsigned long>((unsigned long)reg[1]);

	if (sign==POSITIVE)
		return (signed long)value >= 0;
	else
		return -(signed long)value < 0;
}

signed long Integer::ConvertToLong() const
{
	assert(IsConvertableToLong());

	unsigned long value = (unsigned long)reg[0];
	value += SafeLeftShift<WORD_BITS, unsigned long>((unsigned long)reg[1]);
	return sign==POSITIVE ? value : -(signed long)value;
}

Integer::Integer(BufferedTransformation &encodedInteger, size_t byteCount, Signedness s)
{
	Decode(encodedInteger, byteCount, s);
}

Integer::Integer(const byte *encodedInteger, size_t byteCount, Signedness s)
{
	Decode(encodedInteger, byteCount, s);
}

Integer::Integer(BufferedTransformation &bt)
{
	BERDecode(bt);
}

Integer::Integer(RandomNumberGenerator &rng, size_t bitcount)
{
	Randomize(rng, bitcount);
}

Integer::Integer(RandomNumberGenerator &rng, const Integer &min, const Integer &max, RandomNumberType rnType, const Integer &equiv, const Integer &mod)
{
	if (!Randomize(rng, min, max, rnType, equiv, mod))
		throw Integer::RandomNumberNotFound();
}

Integer Integer::Power2(size_t e)
{
	Integer r((word)0, BitsToWords(e+1));
	r.SetBit(e);
	return r;
}

template <long i>
struct NewInteger
{
	Integer * operator()() const
	{
		return new Integer(i);
	}
};

const Integer &Integer::Zero()
{
	return Singleton<Integer>().Ref();
}

const Integer &Integer::One()
{
	return Singleton<Integer, NewInteger<1> >().Ref();
}

const Integer &Integer::Two()
{
	return Singleton<Integer, NewInteger<2> >().Ref();
}

bool Integer::operator!() const
{
	return IsNegative() ? false : (reg[0]==0 && WordCount()==0);
}

Integer& Integer::operator=(const Integer& t)
{
	if (this != &t)
	{
		if (reg.size() != t.reg.size() || t.reg[t.reg.size()/2] == 0)
			reg.New(RoundupSize(t.WordCount()));
		CopyWords(reg, t.reg, reg.size());
		sign = t.sign;
	}
	return *this;
}

bool Integer::GetBit(size_t n) const
{
	if (n/WORD_BITS >= reg.size())
		return 0;
	else
		return bool((reg[n/WORD_BITS] >> (n % WORD_BITS)) & 1);
}

void Integer::SetBit(size_t n, bool value)
{
	if (value)
	{
		reg.CleanGrow(RoundupSize(BitsToWords(n+1)));
		reg[n/WORD_BITS] |= (word(1) << (n%WORD_BITS));
	}
	else
	{
		if (n/WORD_BITS < reg.size())
			reg[n/WORD_BITS] &= ~(word(1) << (n%WORD_BITS));
	}
}

byte Integer::GetByte(size_t n) const
{
	if (n/WORD_SIZE >= reg.size())
		return 0;
	else
		return byte(reg[n/WORD_SIZE] >> ((n%WORD_SIZE)*8));
}

void Integer::SetByte(size_t n, byte value)
{
	reg.CleanGrow(RoundupSize(BytesToWords(n+1)));
	reg[n/WORD_SIZE] &= ~(word(0xff) << 8*(n%WORD_SIZE));
	reg[n/WORD_SIZE] |= (word(value) << 8*(n%WORD_SIZE));
}

lword Integer::GetBits(size_t i, size_t n) const
{
	lword v = 0;
	assert(n <= sizeof(v)*8);
	for (unsigned int j=0; j<n; j++)
		v |= lword(GetBit(i+j)) << j;
	return v;
}

Integer Integer::operator-() const
{
	Integer result(*this);
	result.Negate();
	return result;
}

Integer Integer::AbsoluteValue() const
{
	Integer result(*this);
	result.sign = POSITIVE;
	return result;
}

void Integer::swap(Integer &a)
{
	reg.swap(a.reg);
	std::swap(sign, a.sign);
}

Integer::Integer(word value, size_t length)
	: reg(RoundupSize(length)), sign(POSITIVE)
{
	reg[0] = value;
	SetWords(reg+1, 0, reg.size()-1);
}

template <class T>
static Integer StringToInteger(const T *str)
{
	int radix;
	// GCC workaround
	// std::char_traits<wchar_t>::length() not defined in GCC 3.2 and STLport 4.5.3
	unsigned int length;
	for (length = 0; str[length] != 0; length++) {}

	Integer v;

	if (length == 0)
		return v;

	switch (str[length-1])
	{
	case 'h':
	case 'H':
		radix=16;
		break;
	case 'o':
	case 'O':
		radix=8;
		break;
	case 'b':
	case 'B':
		radix=2;
		break;
	default:
		radix=10;
	}

	if (length > 2 && str[0] == '0' && str[1] == 'x')
		radix = 16;

	for (unsigned i=0; i<length; i++)
	{
		int digit;

		if (str[i] >= '0' && str[i] <= '9')
			digit = str[i] - '0';
		else if (str[i] >= 'A' && str[i] <= 'F')
			digit = str[i] - 'A' + 10;
		else if (str[i] >= 'a' && str[i] <= 'f')
			digit = str[i] - 'a' + 10;
		else
			digit = radix;

		if (digit < radix)
		{
			v *= radix;
			v += digit;
		}
	}

	if (str[0] == '-')
		v.Negate();

	return v;
}

Integer::Integer(const char *str)
	: reg(2), sign(POSITIVE)
{
	*this = StringToInteger(str);
}

Integer::Integer(const wchar_t *str)
	: reg(2), sign(POSITIVE)
{
	*this = StringToInteger(str);
}

unsigned int Integer::WordCount() const
{
	return (unsigned int)CountWords(reg, reg.size());
}

unsigned int Integer::ByteCount() const
{
	unsigned wordCount = WordCount();
	if (wordCount)
		return (wordCount-1)*WORD_SIZE + BytePrecision(reg[wordCount-1]);
	else
		return 0;
}

unsigned int Integer::BitCount() const
{
	unsigned wordCount = WordCount();
	if (wordCount)
		return (wordCount-1)*WORD_BITS + BitPrecision(reg[wordCount-1]);
	else
		return 0;
}

void Integer::Decode(const byte *input, size_t inputLen, Signedness s)
{
	StringStore store(input, inputLen);
	Decode(store, inputLen, s);
}

void Integer::Decode(BufferedTransformation &bt, size_t inputLen, Signedness s)
{
	assert(bt.MaxRetrievable() >= inputLen);

	byte b;
	bt.Peek(b);
	sign = ((s==SIGNED) && (b & 0x80)) ? NEGATIVE : POSITIVE;

	while (inputLen>0 && (sign==POSITIVE ? b==0 : b==0xff))
	{
		bt.Skip(1);
		inputLen--;
		bt.Peek(b);
	}

	// The call to CleanNew is optimized away above -O0/-Og.
	const size_t size = RoundupSize(BytesToWords(inputLen));
	reg.CleanNew(size);

	assert(reg.SizeInBytes() >= inputLen);
	for (size_t i=inputLen; i > 0; i--)
	{
		bt.Get(b);
		reg[(i-1)/WORD_SIZE] |= word(b) << ((i-1)%WORD_SIZE)*8;
	}

	if (sign == NEGATIVE)
	{
		for (size_t i=inputLen; i<reg.size()*WORD_SIZE; i++)
			reg[i/WORD_SIZE] |= word(0xff) << (i%WORD_SIZE)*8;
		TwosComplement(reg, reg.size());
	}
}

size_t Integer::MinEncodedSize(Signedness signedness) const
{
	unsigned int outputLen = STDMAX(1U, ByteCount());
	if (signedness == UNSIGNED)
		return outputLen;
	if (NotNegative() && (GetByte(outputLen-1) & 0x80))
		outputLen++;
	if (IsNegative() && *this < -Power2(outputLen*8-1))
		outputLen++;
	return outputLen;
}

void Integer::Encode(byte *output, size_t outputLen, Signedness signedness) const
{
	assert(output && outputLen);
	ArraySink sink(output, outputLen);
	Encode(sink, outputLen, signedness);
}

void Integer::Encode(BufferedTransformation &bt, size_t outputLen, Signedness signedness) const
{
	if (signedness == UNSIGNED || NotNegative())
	{
		for (size_t i=outputLen; i > 0; i--)
			bt.Put(GetByte(i-1));
	}
	else
	{
		// take two's complement of *this
		Integer temp = Integer::Power2(8*STDMAX((size_t)ByteCount(), outputLen)) + *this;
		temp.Encode(bt, outputLen, UNSIGNED);
	}
}

void Integer::DEREncode(BufferedTransformation &bt) const
{
	DERGeneralEncoder enc(bt, INTEGER);
	Encode(enc, MinEncodedSize(SIGNED), SIGNED);
	enc.MessageEnd();
}

void Integer::BERDecode(const byte *input, size_t len)
{
	StringStore store(input, len);
	BERDecode(store);
}

void Integer::BERDecode(BufferedTransformation &bt)
{
	BERGeneralDecoder dec(bt, INTEGER);
	if (!dec.IsDefiniteLength() || dec.MaxRetrievable() < dec.RemainingLength())
		BERDecodeError();
	Decode(dec, (size_t)dec.RemainingLength(), SIGNED);
	dec.MessageEnd();
}

void Integer::DEREncodeAsOctetString(BufferedTransformation &bt, size_t length) const
{
	DERGeneralEncoder enc(bt, OCTET_STRING);
	Encode(enc, length);
	enc.MessageEnd();
}

void Integer::BERDecodeAsOctetString(BufferedTransformation &bt, size_t length)
{
	BERGeneralDecoder dec(bt, OCTET_STRING);
	if (!dec.IsDefiniteLength() || dec.RemainingLength() != length)
		BERDecodeError();
	Decode(dec, length);
	dec.MessageEnd();
}

size_t Integer::OpenPGPEncode(byte *output, size_t len) const
{
	ArraySink sink(output, len);
	return OpenPGPEncode(sink);
}

size_t Integer::OpenPGPEncode(BufferedTransformation &bt) const
{
	word16 bitCount = word16(BitCount());
	bt.PutWord16(bitCount);
	size_t byteCount = BitsToBytes(bitCount);
	Encode(bt, byteCount);
	return 2 + byteCount;
}

void Integer::OpenPGPDecode(const byte *input, size_t len)
{
	StringStore store(input, len);
	OpenPGPDecode(store);
}

void Integer::OpenPGPDecode(BufferedTransformation &bt)
{
	word16 bitCount;
	if (bt.GetWord16(bitCount) != 2 || bt.MaxRetrievable() < BitsToBytes(bitCount))
		throw OpenPGPDecodeErr();
	Decode(bt, BitsToBytes(bitCount));
}

void Integer::Randomize(RandomNumberGenerator &rng, size_t nbits)
{
	const size_t nbytes = nbits/8 + 1;
	SecByteBlock buf(nbytes);
	rng.GenerateBlock(buf, nbytes);
	if (nbytes)
		buf[0] = (byte)Crop(buf[0], nbits % 8);
	Decode(buf, nbytes, UNSIGNED);
}

void Integer::Randomize(RandomNumberGenerator &rng, const Integer &min, const Integer &max)
{
	if (min > max)
		throw InvalidArgument("Integer: Min must be no greater than Max");

	Integer range = max - min;
	const unsigned int nbits = range.BitCount();

	do
	{
		Randomize(rng, nbits);
	}
	while (*this > range);

	*this += min;
}

bool Integer::Randomize(RandomNumberGenerator &rng, const Integer &min, const Integer &max, RandomNumberType rnType, const Integer &equiv, const Integer &mod)
{
	return GenerateRandomNoThrow(rng, MakeParameters("Min", min)("Max", max)("RandomNumberType", rnType)("EquivalentTo", equiv)("Mod", mod));
}

class KDF2_RNG : public RandomNumberGenerator
{
public:
	KDF2_RNG(const byte *seed, size_t seedSize)
		: m_counter(0), m_counterAndSeed(seedSize + 4)
	{
		memcpy(m_counterAndSeed + 4, seed, seedSize);
	}

	void GenerateBlock(byte *output, size_t size)
	{
		PutWord(false, BIG_ENDIAN_ORDER, m_counterAndSeed, m_counter);
		++m_counter;
		P1363_KDF2<SHA1>::DeriveKey(output, size, m_counterAndSeed, m_counterAndSeed.size(), NULL, 0);
	}

private:
	word32 m_counter;
	SecByteBlock m_counterAndSeed;
};

bool Integer::GenerateRandomNoThrow(RandomNumberGenerator &i_rng, const NameValuePairs &params)
{
	Integer min = params.GetValueWithDefault("Min", Integer::Zero());
	Integer max;
	if (!params.GetValue("Max", max))
	{
		int bitLength;
		if (params.GetIntValue("BitLength", bitLength))
			max = Integer::Power2(bitLength);
		else
			throw InvalidArgument("Integer: missing Max argument");
	}
	if (min > max)
		throw InvalidArgument("Integer: Min must be no greater than Max");

	Integer equiv = params.GetValueWithDefault("EquivalentTo", Integer::Zero());
	Integer mod = params.GetValueWithDefault("Mod", Integer::One());

	if (equiv.IsNegative() || equiv >= mod)
		throw InvalidArgument("Integer: invalid EquivalentTo and/or Mod argument");

	Integer::RandomNumberType rnType = params.GetValueWithDefault("RandomNumberType", Integer::ANY);

	member_ptr<KDF2_RNG> kdf2Rng;
	ConstByteArrayParameter seed;
	if (params.GetValue(Name::Seed(), seed))
	{
		ByteQueue bq;
		DERSequenceEncoder seq(bq);
		min.DEREncode(seq);
		max.DEREncode(seq);
		equiv.DEREncode(seq);
		mod.DEREncode(seq);
		DEREncodeUnsigned(seq, rnType);
		DEREncodeOctetString(seq, seed.begin(), seed.size());
		seq.MessageEnd();

		SecByteBlock finalSeed((size_t)bq.MaxRetrievable());
		bq.Get(finalSeed, finalSeed.size());
		kdf2Rng.reset(new KDF2_RNG(finalSeed.begin(), finalSeed.size()));
	}
	RandomNumberGenerator &rng = kdf2Rng.get() ? (RandomNumberGenerator &)*kdf2Rng : i_rng;

	switch (rnType)
	{
		case ANY:
			if (mod == One())
				Randomize(rng, min, max);
			else
			{
				Integer min1 = min + (equiv-min)%mod;
				if (max < min1)
					return false;
				Randomize(rng, Zero(), (max - min1) / mod);
				*this *= mod;
				*this += min1;
			}
			return true;

		case PRIME:
		{
			const PrimeSelector *pSelector = params.GetValueWithDefault(Name::PointerToPrimeSelector(), (const PrimeSelector *)NULL);

			int i;
			i = 0;
			while (1)
			{
				if (++i==16)
				{
					// check if there are any suitable primes in [min, max]
					Integer first = min;
					if (FirstPrime(first, max, equiv, mod, pSelector))
					{
						// if there is only one suitable prime, we're done
						*this = first;
						if (!FirstPrime(first, max, equiv, mod, pSelector))
							return true;
					}
					else
						return false;
				}

				Randomize(rng, min, max);
				if (FirstPrime(*this, STDMIN(*this+mod*PrimeSearchInterval(max), max), equiv, mod, pSelector))
					return true;
			}
		}

		default:
			throw InvalidArgument("Integer: invalid RandomNumberType argument");
	}
}

std::istream& operator>>(std::istream& in, Integer &a)
{
	char c;
	unsigned int length = 0;
	SecBlock<char> str(length + 16);

	std::ws(in);

	do
	{
		in.read(&c, 1);
		str[length++] = c;
		if (length >= str.size())
			str.Grow(length + 16);
	}
	while (in && (c=='-' || c=='x' || (c>='0' && c<='9') || (c>='a' && c<='f') || (c>='A' && c<='F') || c=='h' || c=='H' || c=='o' || c=='O' || c==',' || c=='.'));

	if (in.gcount())
		in.putback(c);
	str[length-1] = '\0';
	a = Integer(str);

	return in;
}

std::ostream& operator<<(std::ostream& out, const Integer &a)
{
	// Get relevant conversion specifications from ostream.
	const long f = out.flags() & std::ios::basefield; // Get base digits.
	int base, block;
	char suffix;
	switch(f)
	{
	case std::ios::oct :
		base = 8;
		block = 8;
		suffix = 'o';
		break;
	case std::ios::hex :
		base = 16;
		block = 4;
		suffix = 'h';
		break;
	default :
		base = 10;
		block = 3;
		suffix = '.';
	}

	Integer temp1=a, temp2;
    
	if (a.IsNegative())
	{
		out << '-';
		temp1.Negate();
	}

	if (!a)
		out << '0';

	static const char upper[]="0123456789ABCDEF";
	static const char lower[]="0123456789abcdef";

	const char* vec = (out.flags() & std::ios::uppercase) ? upper : lower;
	unsigned int i=0;
	SecBlock<char> s(a.BitCount() / (SaturatingSubtract1(BitPrecision(base),1U)) + 1);

	while (!!temp1)
	{
		word digit;
		Integer::Divide(digit, temp2, temp1, base);
		s[i++]=vec[digit];
		temp1.swap(temp2);
	}

	while (i--)
	{
		out << s[i];
//		if (i && !(i%block))
//			out << ",";
	}

	return out << suffix;
}

Integer& Integer::operator++()
{
	if (NotNegative())
	{
		if (Increment(reg, reg.size()))
		{
			reg.CleanGrow(2*reg.size());
			reg[reg.size()/2]=1;
		}
	}
	else
	{
		word borrow = Decrement(reg, reg.size());
		assert(!borrow);
		CRYPTOPP_UNUSED(borrow);

		if (WordCount()==0)
			*this = Zero();
	}
	return *this;
}

Integer& Integer::operator--()
{
	if (IsNegative())
	{
		if (Increment(reg, reg.size()))
		{
			reg.CleanGrow(2*reg.size());
			reg[reg.size()/2]=1;
		}
	}
	else
	{
		if (Decrement(reg, reg.size()))
			*this = -One();
	}
	return *this;
}

void PositiveAdd(Integer &sum, const Integer &a, const Integer& b)
{
	int carry;
	if (a.reg.size() == b.reg.size())
		carry = Add(sum.reg, a.reg, b.reg, a.reg.size());
	else if (a.reg.size() > b.reg.size())
	{
		carry = Add(sum.reg, a.reg, b.reg, b.reg.size());
		CopyWords(sum.reg+b.reg.size(), a.reg+b.reg.size(), a.reg.size()-b.reg.size());
		carry = Increment(sum.reg+b.reg.size(), a.reg.size()-b.reg.size(), carry);
	}
	else
	{
		carry = Add(sum.reg, a.reg, b.reg, a.reg.size());
		CopyWords(sum.reg+a.reg.size(), b.reg+a.reg.size(), b.reg.size()-a.reg.size());
		carry = Increment(sum.reg+a.reg.size(), b.reg.size()-a.reg.size(), carry);
	}

	if (carry)
	{
		sum.reg.CleanGrow(2*sum.reg.size());
		sum.reg[sum.reg.size()/2] = 1;
	}
	sum.sign = Integer::POSITIVE;
}

void PositiveSubtract(Integer &diff, const Integer &a, const Integer& b)
{
	unsigned aSize = a.WordCount();
	aSize += aSize%2;
	unsigned bSize = b.WordCount();
	bSize += bSize%2;

	if (aSize == bSize)
	{
		if (Compare(a.reg, b.reg, aSize) >= 0)
		{
			Subtract(diff.reg, a.reg, b.reg, aSize);
			diff.sign = Integer::POSITIVE;
		}
		else
		{
			Subtract(diff.reg, b.reg, a.reg, aSize);
			diff.sign = Integer::NEGATIVE;
		}
	}
	else if (aSize > bSize)
	{
		word borrow = Subtract(diff.reg, a.reg, b.reg, bSize);
		CopyWords(diff.reg+bSize, a.reg+bSize, aSize-bSize);
		borrow = Decrement(diff.reg+bSize, aSize-bSize, borrow);
		assert(!borrow);
		diff.sign = Integer::POSITIVE;
	}
	else
	{
		word borrow = Subtract(diff.reg, b.reg, a.reg, aSize);
		CopyWords(diff.reg+aSize, b.reg+aSize, bSize-aSize);
		borrow = Decrement(diff.reg+aSize, bSize-aSize, borrow);
		assert(!borrow);
		diff.sign = Integer::NEGATIVE;
	}
}

// MSVC .NET 2003 workaround
template <class T> inline const T& STDMAX2(const T& a, const T& b)
{
	return a < b ? b : a;
}

Integer Integer::Plus(const Integer& b) const
{
	Integer sum((word)0, STDMAX2(reg.size(), b.reg.size()));
	if (NotNegative())
	{
		if (b.NotNegative())
			PositiveAdd(sum, *this, b);
		else
			PositiveSubtract(sum, *this, b);
	}
	else
	{
		if (b.NotNegative())
			PositiveSubtract(sum, b, *this);
		else
		{
			PositiveAdd(sum, *this, b);
			sum.sign = Integer::NEGATIVE;
		}
	}
	return sum;
}

Integer& Integer::operator+=(const Integer& t)
{
	reg.CleanGrow(t.reg.size());
	if (NotNegative())
	{
		if (t.NotNegative())
			PositiveAdd(*this, *this, t);
		else
			PositiveSubtract(*this, *this, t);
	}
	else
	{
		if (t.NotNegative())
			PositiveSubtract(*this, t, *this);
		else
		{
			PositiveAdd(*this, *this, t);
			sign = Integer::NEGATIVE;
		}
	}
	return *this;
}

Integer Integer::Minus(const Integer& b) const
{
	Integer diff((word)0, STDMAX2(reg.size(), b.reg.size()));
	if (NotNegative())
	{
		if (b.NotNegative())
			PositiveSubtract(diff, *this, b);
		else
			PositiveAdd(diff, *this, b);
	}
	else
	{
		if (b.NotNegative())
		{
			PositiveAdd(diff, *this, b);
			diff.sign = Integer::NEGATIVE;
		}
		else
			PositiveSubtract(diff, b, *this);
	}
	return diff;
}

Integer& Integer::operator-=(const Integer& t)
{
	reg.CleanGrow(t.reg.size());
	if (NotNegative())
	{
		if (t.NotNegative())
			PositiveSubtract(*this, *this, t);
		else
			PositiveAdd(*this, *this, t);
	}
	else
	{
		if (t.NotNegative())
		{
			PositiveAdd(*this, *this, t);
			sign = Integer::NEGATIVE;
		}
		else
			PositiveSubtract(*this, t, *this);
	}
	return *this;
}

Integer& Integer::operator<<=(size_t n)
{
	const size_t wordCount = WordCount();
	const size_t shiftWords = n / WORD_BITS;
	const unsigned int shiftBits = (unsigned int)(n % WORD_BITS);

	reg.CleanGrow(RoundupSize(wordCount+BitsToWords(n)));
	ShiftWordsLeftByWords(reg, wordCount + shiftWords, shiftWords);
	ShiftWordsLeftByBits(reg+shiftWords, wordCount+BitsToWords(shiftBits), shiftBits);
	return *this;
}

Integer& Integer::operator>>=(size_t n)
{
	const size_t wordCount = WordCount();
	const size_t shiftWords = n / WORD_BITS;
	const unsigned int shiftBits = (unsigned int)(n % WORD_BITS);

	ShiftWordsRightByWords(reg, wordCount, shiftWords);
	if (wordCount > shiftWords)
		ShiftWordsRightByBits(reg, wordCount-shiftWords, shiftBits);
	if (IsNegative() && WordCount()==0)   // avoid -0
		*this = Zero();
	return *this;
}

void PositiveMultiply(Integer &product, const Integer &a, const Integer &b)
{
	size_t aSize = RoundupSize(a.WordCount());
	size_t bSize = RoundupSize(b.WordCount());

	product.reg.CleanNew(RoundupSize(aSize+bSize));
	product.sign = Integer::POSITIVE;

	IntegerSecBlock workspace(aSize + bSize);
	AsymmetricMultiply(product.reg, workspace, a.reg, aSize, b.reg, bSize);
}

void Multiply(Integer &product, const Integer &a, const Integer &b)
{
	PositiveMultiply(product, a, b);

	if (a.NotNegative() != b.NotNegative())
		product.Negate();
}

Integer Integer::Times(const Integer &b) const
{
	Integer product;
	Multiply(product, *this, b);
	return product;
}

/*
void PositiveDivide(Integer &remainder, Integer &quotient,
				   const Integer &dividend, const Integer &divisor)
{
	remainder.reg.CleanNew(divisor.reg.size());
	remainder.sign = Integer::POSITIVE;
	quotient.reg.New(0);
	quotient.sign = Integer::POSITIVE;
	unsigned i=dividend.BitCount();
	while (i--)
	{
		word overflow = ShiftWordsLeftByBits(remainder.reg, remainder.reg.size(), 1);
		remainder.reg[0] |= dividend[i];
		if (overflow || remainder >= divisor)
		{
			Subtract(remainder.reg, remainder.reg, divisor.reg, remainder.reg.size());
			quotient.SetBit(i);
		}
	}
}
*/

void PositiveDivide(Integer &remainder, Integer &quotient,
				   const Integer &a, const Integer &b)
{
	unsigned aSize = a.WordCount();
	unsigned bSize = b.WordCount();

	if (!bSize)
		throw Integer::DivideByZero();

	if (aSize < bSize)
	{
		remainder = a;
		remainder.sign = Integer::POSITIVE;
		quotient = Integer::Zero();
		return;
	}

	aSize += aSize%2;	// round up to next even number
	bSize += bSize%2;

	remainder.reg.CleanNew(RoundupSize(bSize));
	remainder.sign = Integer::POSITIVE;
	quotient.reg.CleanNew(RoundupSize(aSize-bSize+2));
	quotient.sign = Integer::POSITIVE;

	IntegerSecBlock T(aSize+3*(bSize+2));
	Divide(remainder.reg, quotient.reg, T, a.reg, aSize, b.reg, bSize);
}

void Integer::Divide(Integer &remainder, Integer &quotient, const Integer &dividend, const Integer &divisor)
{
	PositiveDivide(remainder, quotient, dividend, divisor);

	if (dividend.IsNegative())
	{
		quotient.Negate();
		if (remainder.NotZero())
		{
			--quotient;
			remainder = divisor.AbsoluteValue() - remainder;
		}
	}

	if (divisor.IsNegative())
		quotient.Negate();
}

void Integer::DivideByPowerOf2(Integer &r, Integer &q, const Integer &a, unsigned int n)
{
	q = a;
	q >>= n;

	const size_t wordCount = BitsToWords(n);
	if (wordCount <= a.WordCount())
	{
		r.reg.resize(RoundupSize(wordCount));
		CopyWords(r.reg, a.reg, wordCount);
		SetWords(r.reg+wordCount, 0, r.reg.size()-wordCount);
		if (n % WORD_BITS != 0)
			r.reg[wordCount-1] %= (word(1) << (n % WORD_BITS));
	}
	else
	{
		r.reg.resize(RoundupSize(a.WordCount()));
		CopyWords(r.reg, a.reg, r.reg.size());
	}
	r.sign = POSITIVE;

	if (a.IsNegative() && r.NotZero())
	{
		--q;
		r = Power2(n) - r;
	}
}

Integer Integer::DividedBy(const Integer &b) const
{
	Integer remainder, quotient;
	Integer::Divide(remainder, quotient, *this, b);
	return quotient;
}

Integer Integer::Modulo(const Integer &b) const
{
	Integer remainder, quotient;
	Integer::Divide(remainder, quotient, *this, b);
	return remainder;
}

void Integer::Divide(word &remainder, Integer &quotient, const Integer &dividend, word divisor)
{
	if (!divisor)
		throw Integer::DivideByZero();

	assert(divisor);

	if ((divisor & (divisor-1)) == 0)	// divisor is a power of 2
	{
		quotient = dividend >> (BitPrecision(divisor)-1);
		remainder = dividend.reg[0] & (divisor-1);
		return;
	}

	unsigned int i = dividend.WordCount();
	quotient.reg.CleanNew(RoundupSize(i));
	remainder = 0;
	while (i--)
	{
		quotient.reg[i] = DWord(dividend.reg[i], remainder) / divisor;
		remainder = DWord(dividend.reg[i], remainder) % divisor;
	}

	if (dividend.NotNegative())
		quotient.sign = POSITIVE;
	else
	{
		quotient.sign = NEGATIVE;
		if (remainder)
		{
			--quotient;
			remainder = divisor - remainder;
		}
	}
}

Integer Integer::DividedBy(word b) const
{
	word remainder;
	Integer quotient;
	Integer::Divide(remainder, quotient, *this, b);
	return quotient;
}

word Integer::Modulo(word divisor) const
{
	if (!divisor)
		throw Integer::DivideByZero();

	assert(divisor);

	word remainder;

	if ((divisor & (divisor-1)) == 0)	// divisor is a power of 2
		remainder = reg[0] & (divisor-1);
	else
	{
		unsigned int i = WordCount();

		if (divisor <= 5)
		{
			DWord sum(0, 0);
			while (i--)
				sum += reg[i];
			remainder = sum % divisor;
		}
		else
		{
			remainder = 0;
			while (i--)
				remainder = DWord(reg[i], remainder) % divisor;
		}
	}

	if (IsNegative() && remainder)
		remainder = divisor - remainder;

	return remainder;
}

void Integer::Negate()
{
	if (!!(*this))	// don't flip sign if *this==0
		sign = Sign(1-sign);
}

int Integer::PositiveCompare(const Integer& t) const
{
	unsigned size = WordCount(), tSize = t.WordCount();

	if (size == tSize)
		return CryptoPP::Compare(reg, t.reg, size);
	else
		return size > tSize ? 1 : -1;
}

int Integer::Compare(const Integer& t) const
{
	if (NotNegative())
	{
		if (t.NotNegative())
			return PositiveCompare(t);
		else
			return 1;
	}
	else
	{
		if (t.NotNegative())
			return -1;
		else
			return -PositiveCompare(t);
	}
}

Integer Integer::SquareRoot() const
{
	if (!IsPositive())
		return Zero();

	// overestimate square root
	Integer x, y = Power2((BitCount()+1)/2);
	assert(y*y >= *this);

	do
	{
		x = y;
		y = (x + *this/x) >> 1;
	} while (y<x);

	return x;
}

bool Integer::IsSquare() const
{
	Integer r = SquareRoot();
	return *this == r.Squared();
}

bool Integer::IsUnit() const
{
	return (WordCount() == 1) && (reg[0] == 1);
}

Integer Integer::MultiplicativeInverse() const
{
	return IsUnit() ? *this : Zero();
}

Integer a_times_b_mod_c(const Integer &x, const Integer& y, const Integer& m)
{
	return x*y%m;
}

Integer a_exp_b_mod_c(const Integer &x, const Integer& e, const Integer& m)
{
	ModularArithmetic mr(m);
	return mr.Exponentiate(x, e);
}

Integer Integer::Gcd(const Integer &a, const Integer &b)
{
	return EuclideanDomainOf<Integer>().Gcd(a, b);
}

Integer Integer::InverseMod(const Integer &m) const
{
	assert(m.NotNegative());

	if (IsNegative())
		return Modulo(m).InverseMod(m);

	if (m.IsEven())
	{
		if (!m || IsEven())
			return Zero();	// no inverse
		if (*this == One())
			return One();

		Integer u = m.Modulo(*this).InverseMod(*this);
		return !u ? Zero() : (m*(*this-u)+1)/(*this);
	}

	SecBlock<word> T(m.reg.size() * 4);
	Integer r((word)0, m.reg.size());
	unsigned k = AlmostInverse(r.reg, T, reg, reg.size(), m.reg, m.reg.size());
	DivideByPower2Mod(r.reg, r.reg, k, m.reg, m.reg.size());
	return r;
}

word Integer::InverseMod(word mod) const
{
	word g0 = mod, g1 = *this % mod;
	word v0 = 0, v1 = 1;
	word y;

	while (g1)
	{
		if (g1 == 1)
			return v1;
		y = g0 / g1;
		g0 = g0 % g1;
		v0 += y * v1;

		if (!g0)
			break;
		if (g0 == 1)
			return mod-v0;
		y = g1 / g0;
		g1 = g1 % g0;
		v1 += y * v0;
	}
	return 0;
}

// ********************************************************

ModularArithmetic::ModularArithmetic(BufferedTransformation &bt)
{
	BERSequenceDecoder seq(bt);
	OID oid(seq);
	if (oid != ASN1::prime_field())
		BERDecodeError();
	m_modulus.BERDecode(seq);
	seq.MessageEnd();
	m_result.reg.resize(m_modulus.reg.size());
}

void ModularArithmetic::DEREncode(BufferedTransformation &bt) const
{
	DERSequenceEncoder seq(bt);
	ASN1::prime_field().DEREncode(seq);
	m_modulus.DEREncode(seq);
	seq.MessageEnd();
}

void ModularArithmetic::DEREncodeElement(BufferedTransformation &out, const Element &a) const
{
	a.DEREncodeAsOctetString(out, MaxElementByteLength());
}

void ModularArithmetic::BERDecodeElement(BufferedTransformation &in, Element &a) const
{
	a.BERDecodeAsOctetString(in, MaxElementByteLength());
}

const Integer& ModularArithmetic::Half(const Integer &a) const
{
	if (a.reg.size()==m_modulus.reg.size())
	{
		CryptoPP::DivideByPower2Mod(m_result.reg.begin(), a.reg, 1, m_modulus.reg, a.reg.size());
		return m_result;
	}
	else
		return m_result1 = (a.IsEven() ? (a >> 1) : ((a+m_modulus) >> 1));
}

const Integer& ModularArithmetic::Add(const Integer &a, const Integer &b) const
{
	if (a.reg.size()==m_modulus.reg.size() && b.reg.size()==m_modulus.reg.size())
	{
		if (CryptoPP::Add(m_result.reg.begin(), a.reg, b.reg, a.reg.size())
			|| Compare(m_result.reg, m_modulus.reg, a.reg.size()) >= 0)
		{
			CryptoPP::Subtract(m_result.reg.begin(), m_result.reg, m_modulus.reg, a.reg.size());
		}
		return m_result;
	}
	else
	{
		m_result1 = a+b;
		if (m_result1 >= m_modulus)
			m_result1 -= m_modulus;
		return m_result1;
	}
}

Integer& ModularArithmetic::Accumulate(Integer &a, const Integer &b) const
{
	if (a.reg.size()==m_modulus.reg.size() && b.reg.size()==m_modulus.reg.size())
	{
		if (CryptoPP::Add(a.reg, a.reg, b.reg, a.reg.size())
			|| Compare(a.reg, m_modulus.reg, a.reg.size()) >= 0)
		{
			CryptoPP::Subtract(a.reg, a.reg, m_modulus.reg, a.reg.size());
		}
	}
	else
	{
		a+=b;
		if (a>=m_modulus)
			a-=m_modulus;
	}

	return a;
}

const Integer& ModularArithmetic::Subtract(const Integer &a, const Integer &b) const
{
	if (a.reg.size()==m_modulus.reg.size() && b.reg.size()==m_modulus.reg.size())
	{
		if (CryptoPP::Subtract(m_result.reg.begin(), a.reg, b.reg, a.reg.size()))
			CryptoPP::Add(m_result.reg.begin(), m_result.reg, m_modulus.reg, a.reg.size());
		return m_result;
	}
	else
	{
		m_result1 = a-b;
		if (m_result1.IsNegative())
			m_result1 += m_modulus;
		return m_result1;
	}
}

Integer& ModularArithmetic::Reduce(Integer &a, const Integer &b) const
{
	if (a.reg.size()==m_modulus.reg.size() && b.reg.size()==m_modulus.reg.size())
	{
		if (CryptoPP::Subtract(a.reg, a.reg, b.reg, a.reg.size()))
			CryptoPP::Add(a.reg, a.reg, m_modulus.reg, a.reg.size());
	}
	else
	{
		a-=b;
		if (a.IsNegative())
			a+=m_modulus;
	}

	return a;
}

const Integer& ModularArithmetic::Inverse(const Integer &a) const
{
	if (!a)
		return a;

	CopyWords(m_result.reg.begin(), m_modulus.reg, m_modulus.reg.size());
	if (CryptoPP::Subtract(m_result.reg.begin(), m_result.reg, a.reg, a.reg.size()))
		Decrement(m_result.reg.begin()+a.reg.size(), m_modulus.reg.size()-a.reg.size());

	return m_result;
}

Integer ModularArithmetic::CascadeExponentiate(const Integer &x, const Integer &e1, const Integer &y, const Integer &e2) const
{
	if (m_modulus.IsOdd())
	{
		MontgomeryRepresentation dr(m_modulus);
		return dr.ConvertOut(dr.CascadeExponentiate(dr.ConvertIn(x), e1, dr.ConvertIn(y), e2));
	}
	else
		return AbstractRing<Integer>::CascadeExponentiate(x, e1, y, e2);
}

void ModularArithmetic::SimultaneousExponentiate(Integer *results, const Integer &base, const Integer *exponents, unsigned int exponentsCount) const
{
	if (m_modulus.IsOdd())
	{
		MontgomeryRepresentation dr(m_modulus);
		dr.SimultaneousExponentiate(results, dr.ConvertIn(base), exponents, exponentsCount);
		for (unsigned int i=0; i<exponentsCount; i++)
			results[i] = dr.ConvertOut(results[i]);
	}
	else
		AbstractRing<Integer>::SimultaneousExponentiate(results, base, exponents, exponentsCount);
}

MontgomeryRepresentation::MontgomeryRepresentation(const Integer &m)	// modulus must be odd
	: ModularArithmetic(m),
	  m_u((word)0, m_modulus.reg.size()),
	  m_workspace(5*m_modulus.reg.size())
{
	if (!m_modulus.IsOdd())
		throw InvalidArgument("MontgomeryRepresentation: Montgomery representation requires an odd modulus");

	RecursiveInverseModPower2(m_u.reg, m_workspace, m_modulus.reg, m_modulus.reg.size());
}

const Integer& MontgomeryRepresentation::Multiply(const Integer &a, const Integer &b) const
{
	word *const T = m_workspace.begin();
	word *const R = m_result.reg.begin();
	const size_t N = m_modulus.reg.size();
	assert(a.reg.size()<=N && b.reg.size()<=N);

	AsymmetricMultiply(T, T+2*N, a.reg, a.reg.size(), b.reg, b.reg.size());
	SetWords(T+a.reg.size()+b.reg.size(), 0, 2*N-a.reg.size()-b.reg.size());
	MontgomeryReduce(R, T+2*N, T, m_modulus.reg, m_u.reg, N);
	return m_result;
}

const Integer& MontgomeryRepresentation::Square(const Integer &a) const
{
	word *const T = m_workspace.begin();
	word *const R = m_result.reg.begin();
	const size_t N = m_modulus.reg.size();
	assert(a.reg.size()<=N);

	CryptoPP::Square(T, T+2*N, a.reg, a.reg.size());
	SetWords(T+2*a.reg.size(), 0, 2*N-2*a.reg.size());
	MontgomeryReduce(R, T+2*N, T, m_modulus.reg, m_u.reg, N);
	return m_result;
}

Integer MontgomeryRepresentation::ConvertOut(const Integer &a) const
{
	word *const T = m_workspace.begin();
	word *const R = m_result.reg.begin();
	const size_t N = m_modulus.reg.size();
	assert(a.reg.size()<=N);

	CopyWords(T, a.reg, a.reg.size());
	SetWords(T+a.reg.size(), 0, 2*N-a.reg.size());
	MontgomeryReduce(R, T+2*N, T, m_modulus.reg, m_u.reg, N);
	return m_result;
}

const Integer& MontgomeryRepresentation::MultiplicativeInverse(const Integer &a) const
{
//	  return (EuclideanMultiplicativeInverse(a, modulus)<<(2*WORD_BITS*modulus.reg.size()))%modulus;
	word *const T = m_workspace.begin();
	word *const R = m_result.reg.begin();
	const size_t N = m_modulus.reg.size();
	assert(a.reg.size()<=N);

	CopyWords(T, a.reg, a.reg.size());
	SetWords(T+a.reg.size(), 0, 2*N-a.reg.size());
	MontgomeryReduce(R, T+2*N, T, m_modulus.reg, m_u.reg, N);
	unsigned k = AlmostInverse(R, T, R, N, m_modulus.reg, N);

//	cout << "k=" << k << " N*32=" << 32*N << endl;

	if (k>N*WORD_BITS)
		DivideByPower2Mod(R, R, k-N*WORD_BITS, m_modulus.reg, N);
	else
		MultiplyByPower2Mod(R, R, N*WORD_BITS-k, m_modulus.reg, N);

	return m_result;
}

// Specialization declared in misc.h to allow us to print integers
//  with additional control options, like arbirary bases and uppercase.
template <> CRYPTOPP_DLL
std::string IntToString<Integer>(Integer value, unsigned int base)
{
	// Hack... set the high bit for uppercase. Set the next bit fo a suffix.
	static const unsigned int BIT_32 = (1U << 31);
	const bool UPPER = !!(base & BIT_32);
	static const unsigned int BIT_31 = (1U << 30);
	const bool BASE = !!(base & BIT_31);

	const char CH = UPPER ? 'A' : 'a';
	base &= ~(BIT_32|BIT_31);
	assert(base >= 2 && base <= 32);

	if (value == 0)
		return "0";

	bool negative = false, zero = false;
	if (value.IsNegative())
	{
		negative = true;
		value.Negate();
	}

	if (!value)
		zero = true;

	SecBlock<char> s(value.BitCount() / (SaturatingSubtract1(BitPrecision(base),1U)) + 1);
	Integer temp;

	unsigned int i=0;
	while (!!value)
	{
		word digit;
		Integer::Divide(digit, temp, value, word(base));
		s[i++]=char((digit < 10 ? '0' : (CH - 10)) + digit);
		value.swap(temp);
	}

	std::string result;
	result.reserve(i+2);
	
	if (negative)
		result += '-';

	if (zero)
		result += '0';

	while (i--)
		result += s[i];

	if (BASE)
	{
		if (base == 10)
			result += '.';
		else if (base == 16)
			result += 'h';
		else if (base == 8)
			result += 'o';
		else if (base == 2)
			result += 'b';
	}

	return result;
}

// Specialization declared in misc.h to avoid Coverity findings.
template <> CRYPTOPP_DLL
std::string IntToString<unsigned long long>(unsigned long long value, unsigned int base)
{
	// Hack... set the high bit for uppercase.
	static const unsigned int HIGH_BIT = (1U << 31);
	const char CH = !!(base & HIGH_BIT) ? 'A' : 'a';
	base &= ~HIGH_BIT;
	
	assert(base >= 2);
	if (value == 0)
		return "0";

	std::string result;
	while (value > 0)
	{
		unsigned long long digit = value % base;
		result = char((digit < 10 ? '0' : (CH - 10)) + digit) + result;
		value /= base;
	}
	return result;
}

NAMESPACE_END
	
#if WORKAROUND_ARMEL_BUG
# pragma GCC pop_options
#endif
	
#if WORKAROUND_ARM64_BUG
# pragma GCC pop_options
#endif

#endif
