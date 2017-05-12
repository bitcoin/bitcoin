// salsa.cpp - written and placed in the public domain by Wei Dai

// use "cl /EP /P /DCRYPTOPP_GENERATE_X64_MASM salsa.cpp" to generate MASM code

#include "pch.h"
#include "config.h"

#ifndef CRYPTOPP_GENERATE_X64_MASM

#include "salsa.h"
#include "argnames.h"
#include "misc.h"
#include "cpu.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4702 4740)
#endif

// TODO: work around GCC 4.8+ issue with SSE2 ASM until the exact details are known
//   and fix is released. Duplicate with "valgrind ./cryptest.exe tv salsa"
// Clang due to "Inline assembly operands don't work with .intel_syntax"
//   https://llvm.org/bugs/show_bug.cgi?id=24232
#if defined(CRYPTOPP_DISABLE_SALSA_ASM)
# undef CRYPTOPP_X86_ASM_AVAILABLE
# undef CRYPTOPP_X32_ASM_AVAILABLE
# undef CRYPTOPP_X64_ASM_AVAILABLE
# undef CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
# undef CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE
# define CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE 0
# define CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE 0
#endif

NAMESPACE_BEGIN(CryptoPP)

#if CRYPTOPP_DEBUG && !defined(CRYPTOPP_DOXYGEN_PROCESSING)
void Salsa20_TestInstantiations()
{
	Salsa20::Encryption x1;
	XSalsa20::Encryption x2;
}
#endif

void Salsa20_Policy::CipherSetKey(const NameValuePairs &params, const byte *key, size_t length)
{
	m_rounds = params.GetIntValueWithDefault(Name::Rounds(), 20);

	if (!(m_rounds == 8 || m_rounds == 12 || m_rounds == 20))
		throw InvalidRounds(Salsa20::StaticAlgorithmName(), m_rounds);

	// m_state is reordered for SSE2
	GetBlock<word32, LittleEndian> get1(key);
	get1(m_state[13])(m_state[10])(m_state[7])(m_state[4]);
	GetBlock<word32, LittleEndian> get2(key + length - 16);
	get2(m_state[15])(m_state[12])(m_state[9])(m_state[6]);

	// "expand 16-byte k" or "expand 32-byte k"
	m_state[0] = 0x61707865;
	m_state[1] = (length == 16) ? 0x3120646e : 0x3320646e;
	m_state[2] = (length == 16) ? 0x79622d36 : 0x79622d32;
	m_state[3] = 0x6b206574;
}

void Salsa20_Policy::CipherResynchronize(byte *keystreamBuffer, const byte *IV, size_t length)
{
	CRYPTOPP_UNUSED(keystreamBuffer), CRYPTOPP_UNUSED(length);
	CRYPTOPP_ASSERT(length==8);

	GetBlock<word32, LittleEndian> get(IV);
	get(m_state[14])(m_state[11]);
	m_state[8] = m_state[5] = 0;
}

void Salsa20_Policy::SeekToIteration(lword iterationCount)
{
	m_state[8] = (word32)iterationCount;
	m_state[5] = (word32)SafeRightShift<32>(iterationCount);
}

#if (CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64) && !defined(CRYPTOPP_DISABLE_SALSA_ASM)
unsigned int Salsa20_Policy::GetAlignment() const
{
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
	if (HasSSE2())
		return 16;
	else
#endif
		return GetAlignmentOf<word32>();
}

unsigned int Salsa20_Policy::GetOptimalBlockSize() const
{
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
	if (HasSSE2())
		return 4*BYTES_PER_ITERATION;
	else
#endif
		return BYTES_PER_ITERATION;
}
#endif

#ifdef CRYPTOPP_X64_MASM_AVAILABLE
extern "C" {
void Salsa20_OperateKeystream(byte *output, const byte *input, size_t iterationCount, int rounds, void *state);
}
#endif

#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4731)	// frame pointer register 'ebp' modified by inline assembly code
#endif

void Salsa20_Policy::OperateKeystream(KeystreamOperation operation, byte *output, const byte *input, size_t iterationCount)
{
#endif	// #ifdef CRYPTOPP_GENERATE_X64_MASM

#ifdef CRYPTOPP_X64_MASM_AVAILABLE
	Salsa20_OperateKeystream(output, input, iterationCount, m_rounds, m_state.data());
	return;
#endif

#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
#ifdef CRYPTOPP_GENERATE_X64_MASM
		ALIGN   8
	Salsa20_OperateKeystream	PROC FRAME
		mov		r10, [rsp + 5*8]			; state
		alloc_stack(10*16 + 32*16 + 8)
		save_xmm128 xmm6, 0200h
		save_xmm128 xmm7, 0210h
		save_xmm128 xmm8, 0220h
		save_xmm128 xmm9, 0230h
		save_xmm128 xmm10, 0240h
		save_xmm128 xmm11, 0250h
		save_xmm128 xmm12, 0260h
		save_xmm128 xmm13, 0270h
		save_xmm128 xmm14, 0280h
		save_xmm128 xmm15, 0290h
		.endprolog

	#define REG_output			rcx
	#define REG_input			rdx
	#define REG_iterationCount	r8
	#define REG_state			r10
	#define REG_rounds			e9d
	#define REG_roundsLeft		eax
	#define REG_temp32			r11d
	#define REG_temp			r11
	#define SSE2_WORKSPACE		rsp
#else
	if (HasSSE2())
	{
	#if CRYPTOPP_BOOL_X64
		#define REG_output			%1
		#define REG_input			%0
		#define REG_iterationCount	%2
		#define REG_state			%4		/* constant */
		#define REG_rounds			%3		/* constant */
		#define REG_roundsLeft		eax
		#define REG_temp32			edx
		#define REG_temp			rdx
		#define SSE2_WORKSPACE		%5		/* constant */

		CRYPTOPP_ALIGN_DATA(16) byte workspace[16*32];
	#else
		#define REG_output			edi
		#define REG_input			eax
		#define REG_iterationCount	ecx
		#define REG_state			esi
		#define REG_rounds			edx
		#define REG_roundsLeft		ebx
		#define REG_temp32			ebp
		#define REG_temp			ebp
		#define SSE2_WORKSPACE		esp + WORD_SZ
	#endif

	#ifdef __GNUC__
		__asm__ __volatile__
		(
			INTEL_NOPREFIX
			AS_PUSH_IF86(	bx)
	#else
		void *s = m_state.data();
		word32 r = m_rounds;

		AS2(	mov		REG_iterationCount, iterationCount)
		AS2(	mov		REG_input, input)
		AS2(	mov		REG_output, output)
		AS2(	mov		REG_state, s)
		AS2(	mov		REG_rounds, r)
	#endif
#endif	// #ifndef CRYPTOPP_GENERATE_X64_MASM

		AS_PUSH_IF86(	bp)
		AS2(	cmp		REG_iterationCount, 4)
		ASJ(	jl,		5, f)

#if CRYPTOPP_BOOL_X86
		AS2(	mov		ebx, esp)
		AS2(	and		esp, -16)
		AS2(	sub		esp, 32*16)
		AS1(	push	ebx)
#endif

#define SSE2_EXPAND_S(i, j)		\
	ASS(	pshufd	xmm4, xmm##i, j, j, j, j)	\
	AS2(	movdqa	[SSE2_WORKSPACE + (i*4+j)*16 + 256], xmm4)

		AS2(	movdqa	xmm0, [REG_state + 0*16])
		AS2(	movdqa	xmm1, [REG_state + 1*16])
		AS2(	movdqa	xmm2, [REG_state + 2*16])
		AS2(	movdqa	xmm3, [REG_state + 3*16])
		SSE2_EXPAND_S(0, 0)
		SSE2_EXPAND_S(0, 1)
		SSE2_EXPAND_S(0, 2)
		SSE2_EXPAND_S(0, 3)
		SSE2_EXPAND_S(1, 0)
		SSE2_EXPAND_S(1, 2)
		SSE2_EXPAND_S(1, 3)
		SSE2_EXPAND_S(2, 1)
		SSE2_EXPAND_S(2, 2)
		SSE2_EXPAND_S(2, 3)
		SSE2_EXPAND_S(3, 0)
		SSE2_EXPAND_S(3, 1)
		SSE2_EXPAND_S(3, 2)
		SSE2_EXPAND_S(3, 3)

#define SSE2_EXPAND_S85(i)		\
		AS2(	mov		dword ptr [SSE2_WORKSPACE + 8*16 + i*4 + 256], REG_roundsLeft)	\
		AS2(	mov		dword ptr [SSE2_WORKSPACE + 5*16 + i*4 + 256], REG_temp32)	\
		AS2(	add		REG_roundsLeft, 1)	\
		AS2(	adc		REG_temp32, 0)

		ASL(1)
		AS2(	mov		REG_roundsLeft, dword ptr [REG_state + 8*4])
		AS2(	mov		REG_temp32, dword ptr [REG_state + 5*4])
		SSE2_EXPAND_S85(0)
		SSE2_EXPAND_S85(1)
		SSE2_EXPAND_S85(2)
		SSE2_EXPAND_S85(3)
		AS2(	mov		dword ptr [REG_state + 8*4], REG_roundsLeft)
		AS2(	mov		dword ptr [REG_state + 5*4], REG_temp32)

#define SSE2_QUARTER_ROUND(a, b, d, i)		\
	AS2(	movdqa	xmm4, xmm##d)			\
	AS2(	paddd	xmm4, xmm##a)			\
	AS2(	movdqa	xmm5, xmm4)				\
	AS2(	pslld	xmm4, i)				\
	AS2(	psrld	xmm5, 32-i)				\
	AS2(	pxor	xmm##b, xmm4)			\
	AS2(	pxor	xmm##b, xmm5)

#define L01(A,B,C,D,a,b,c,d,i)		AS2(	movdqa	xmm##A, [SSE2_WORKSPACE + d*16 + i*256])	/* y3 */
#define L02(A,B,C,D,a,b,c,d,i)		AS2(	movdqa	xmm##C, [SSE2_WORKSPACE + a*16 + i*256])	/* y0 */
#define L03(A,B,C,D,a,b,c,d,i)		AS2(	paddd	xmm##A, xmm##C)		/* y0+y3 */
#define L04(A,B,C,D,a,b,c,d,i)		AS2(	movdqa	xmm##B, xmm##A)
#define L05(A,B,C,D,a,b,c,d,i)		AS2(	pslld	xmm##A, 7)
#define L06(A,B,C,D,a,b,c,d,i)		AS2(	psrld	xmm##B, 32-7)
#define L07(A,B,C,D,a,b,c,d,i)		AS2(	pxor	xmm##A, [SSE2_WORKSPACE + b*16 + i*256])
#define L08(A,B,C,D,a,b,c,d,i)		AS2(	pxor	xmm##A, xmm##B)		/* z1 */
#define L09(A,B,C,D,a,b,c,d,i)		AS2(	movdqa	[SSE2_WORKSPACE + b*16], xmm##A)
#define L10(A,B,C,D,a,b,c,d,i)		AS2(	movdqa	xmm##B, xmm##A)
#define L11(A,B,C,D,a,b,c,d,i)		AS2(	paddd	xmm##A, xmm##C)		/* z1+y0 */
#define L12(A,B,C,D,a,b,c,d,i)		AS2(	movdqa	xmm##D, xmm##A)
#define L13(A,B,C,D,a,b,c,d,i)		AS2(	pslld	xmm##A, 9)
#define L14(A,B,C,D,a,b,c,d,i)		AS2(	psrld	xmm##D, 32-9)
#define L15(A,B,C,D,a,b,c,d,i)		AS2(	pxor	xmm##A, [SSE2_WORKSPACE + c*16 + i*256])
#define L16(A,B,C,D,a,b,c,d,i)		AS2(	pxor	xmm##A, xmm##D)		/* z2 */
#define L17(A,B,C,D,a,b,c,d,i)		AS2(	movdqa	[SSE2_WORKSPACE + c*16], xmm##A)
#define L18(A,B,C,D,a,b,c,d,i)		AS2(	movdqa	xmm##D, xmm##A)
#define L19(A,B,C,D,a,b,c,d,i)		AS2(	paddd	xmm##A, xmm##B)		/* z2+z1 */
#define L20(A,B,C,D,a,b,c,d,i)		AS2(	movdqa	xmm##B, xmm##A)
#define L21(A,B,C,D,a,b,c,d,i)		AS2(	pslld	xmm##A, 13)
#define L22(A,B,C,D,a,b,c,d,i)		AS2(	psrld	xmm##B, 32-13)
#define L23(A,B,C,D,a,b,c,d,i)		AS2(	pxor	xmm##A, [SSE2_WORKSPACE + d*16 + i*256])
#define L24(A,B,C,D,a,b,c,d,i)		AS2(	pxor	xmm##A, xmm##B)		/* z3 */
#define L25(A,B,C,D,a,b,c,d,i)		AS2(	movdqa	[SSE2_WORKSPACE + d*16], xmm##A)
#define L26(A,B,C,D,a,b,c,d,i)		AS2(	paddd	xmm##A, xmm##D)		/* z3+z2 */
#define L27(A,B,C,D,a,b,c,d,i)		AS2(	movdqa	xmm##D, xmm##A)
#define L28(A,B,C,D,a,b,c,d,i)		AS2(	pslld	xmm##A, 18)
#define L29(A,B,C,D,a,b,c,d,i)		AS2(	psrld	xmm##D, 32-18)
#define L30(A,B,C,D,a,b,c,d,i)		AS2(	pxor	xmm##A, xmm##C)		/* xor y0 */
#define L31(A,B,C,D,a,b,c,d,i)		AS2(	pxor	xmm##A, xmm##D)		/* z0 */
#define L32(A,B,C,D,a,b,c,d,i)		AS2(	movdqa	[SSE2_WORKSPACE + a*16], xmm##A)

#define SSE2_QUARTER_ROUND_X8(i, a, b, c, d, e, f, g, h)	\
	L01(0,1,2,3, a,b,c,d, i)	L01(4,5,6,7, e,f,g,h, i)	\
	L02(0,1,2,3, a,b,c,d, i)	L02(4,5,6,7, e,f,g,h, i)	\
	L03(0,1,2,3, a,b,c,d, i)	L03(4,5,6,7, e,f,g,h, i)	\
	L04(0,1,2,3, a,b,c,d, i)	L04(4,5,6,7, e,f,g,h, i)	\
	L05(0,1,2,3, a,b,c,d, i)	L05(4,5,6,7, e,f,g,h, i)	\
	L06(0,1,2,3, a,b,c,d, i)	L06(4,5,6,7, e,f,g,h, i)	\
	L07(0,1,2,3, a,b,c,d, i)	L07(4,5,6,7, e,f,g,h, i)	\
	L08(0,1,2,3, a,b,c,d, i)	L08(4,5,6,7, e,f,g,h, i)	\
	L09(0,1,2,3, a,b,c,d, i)	L09(4,5,6,7, e,f,g,h, i)	\
	L10(0,1,2,3, a,b,c,d, i)	L10(4,5,6,7, e,f,g,h, i)	\
	L11(0,1,2,3, a,b,c,d, i)	L11(4,5,6,7, e,f,g,h, i)	\
	L12(0,1,2,3, a,b,c,d, i)	L12(4,5,6,7, e,f,g,h, i)	\
	L13(0,1,2,3, a,b,c,d, i)	L13(4,5,6,7, e,f,g,h, i)	\
	L14(0,1,2,3, a,b,c,d, i)	L14(4,5,6,7, e,f,g,h, i)	\
	L15(0,1,2,3, a,b,c,d, i)	L15(4,5,6,7, e,f,g,h, i)	\
	L16(0,1,2,3, a,b,c,d, i)	L16(4,5,6,7, e,f,g,h, i)	\
	L17(0,1,2,3, a,b,c,d, i)	L17(4,5,6,7, e,f,g,h, i)	\
	L18(0,1,2,3, a,b,c,d, i)	L18(4,5,6,7, e,f,g,h, i)	\
	L19(0,1,2,3, a,b,c,d, i)	L19(4,5,6,7, e,f,g,h, i)	\
	L20(0,1,2,3, a,b,c,d, i)	L20(4,5,6,7, e,f,g,h, i)	\
	L21(0,1,2,3, a,b,c,d, i)	L21(4,5,6,7, e,f,g,h, i)	\
	L22(0,1,2,3, a,b,c,d, i)	L22(4,5,6,7, e,f,g,h, i)	\
	L23(0,1,2,3, a,b,c,d, i)	L23(4,5,6,7, e,f,g,h, i)	\
	L24(0,1,2,3, a,b,c,d, i)	L24(4,5,6,7, e,f,g,h, i)	\
	L25(0,1,2,3, a,b,c,d, i)	L25(4,5,6,7, e,f,g,h, i)	\
	L26(0,1,2,3, a,b,c,d, i)	L26(4,5,6,7, e,f,g,h, i)	\
	L27(0,1,2,3, a,b,c,d, i)	L27(4,5,6,7, e,f,g,h, i)	\
	L28(0,1,2,3, a,b,c,d, i)	L28(4,5,6,7, e,f,g,h, i)	\
	L29(0,1,2,3, a,b,c,d, i)	L29(4,5,6,7, e,f,g,h, i)	\
	L30(0,1,2,3, a,b,c,d, i)	L30(4,5,6,7, e,f,g,h, i)	\
	L31(0,1,2,3, a,b,c,d, i)	L31(4,5,6,7, e,f,g,h, i)	\
	L32(0,1,2,3, a,b,c,d, i)	L32(4,5,6,7, e,f,g,h, i)

#define SSE2_QUARTER_ROUND_X16(i, a, b, c, d, e, f, g, h, A, B, C, D, E, F, G, H)	\
	L01(0,1,2,3, a,b,c,d, i)	L01(4,5,6,7, e,f,g,h, i)	L01(8,9,10,11, A,B,C,D, i)	L01(12,13,14,15, E,F,G,H, i)	\
	L02(0,1,2,3, a,b,c,d, i)	L02(4,5,6,7, e,f,g,h, i)	L02(8,9,10,11, A,B,C,D, i)	L02(12,13,14,15, E,F,G,H, i)	\
	L03(0,1,2,3, a,b,c,d, i)	L03(4,5,6,7, e,f,g,h, i)	L03(8,9,10,11, A,B,C,D, i)	L03(12,13,14,15, E,F,G,H, i)	\
	L04(0,1,2,3, a,b,c,d, i)	L04(4,5,6,7, e,f,g,h, i)	L04(8,9,10,11, A,B,C,D, i)	L04(12,13,14,15, E,F,G,H, i)	\
	L05(0,1,2,3, a,b,c,d, i)	L05(4,5,6,7, e,f,g,h, i)	L05(8,9,10,11, A,B,C,D, i)	L05(12,13,14,15, E,F,G,H, i)	\
	L06(0,1,2,3, a,b,c,d, i)	L06(4,5,6,7, e,f,g,h, i)	L06(8,9,10,11, A,B,C,D, i)	L06(12,13,14,15, E,F,G,H, i)	\
	L07(0,1,2,3, a,b,c,d, i)	L07(4,5,6,7, e,f,g,h, i)	L07(8,9,10,11, A,B,C,D, i)	L07(12,13,14,15, E,F,G,H, i)	\
	L08(0,1,2,3, a,b,c,d, i)	L08(4,5,6,7, e,f,g,h, i)	L08(8,9,10,11, A,B,C,D, i)	L08(12,13,14,15, E,F,G,H, i)	\
	L09(0,1,2,3, a,b,c,d, i)	L09(4,5,6,7, e,f,g,h, i)	L09(8,9,10,11, A,B,C,D, i)	L09(12,13,14,15, E,F,G,H, i)	\
	L10(0,1,2,3, a,b,c,d, i)	L10(4,5,6,7, e,f,g,h, i)	L10(8,9,10,11, A,B,C,D, i)	L10(12,13,14,15, E,F,G,H, i)	\
	L11(0,1,2,3, a,b,c,d, i)	L11(4,5,6,7, e,f,g,h, i)	L11(8,9,10,11, A,B,C,D, i)	L11(12,13,14,15, E,F,G,H, i)	\
	L12(0,1,2,3, a,b,c,d, i)	L12(4,5,6,7, e,f,g,h, i)	L12(8,9,10,11, A,B,C,D, i)	L12(12,13,14,15, E,F,G,H, i)	\
	L13(0,1,2,3, a,b,c,d, i)	L13(4,5,6,7, e,f,g,h, i)	L13(8,9,10,11, A,B,C,D, i)	L13(12,13,14,15, E,F,G,H, i)	\
	L14(0,1,2,3, a,b,c,d, i)	L14(4,5,6,7, e,f,g,h, i)	L14(8,9,10,11, A,B,C,D, i)	L14(12,13,14,15, E,F,G,H, i)	\
	L15(0,1,2,3, a,b,c,d, i)	L15(4,5,6,7, e,f,g,h, i)	L15(8,9,10,11, A,B,C,D, i)	L15(12,13,14,15, E,F,G,H, i)	\
	L16(0,1,2,3, a,b,c,d, i)	L16(4,5,6,7, e,f,g,h, i)	L16(8,9,10,11, A,B,C,D, i)	L16(12,13,14,15, E,F,G,H, i)	\
	L17(0,1,2,3, a,b,c,d, i)	L17(4,5,6,7, e,f,g,h, i)	L17(8,9,10,11, A,B,C,D, i)	L17(12,13,14,15, E,F,G,H, i)	\
	L18(0,1,2,3, a,b,c,d, i)	L18(4,5,6,7, e,f,g,h, i)	L18(8,9,10,11, A,B,C,D, i)	L18(12,13,14,15, E,F,G,H, i)	\
	L19(0,1,2,3, a,b,c,d, i)	L19(4,5,6,7, e,f,g,h, i)	L19(8,9,10,11, A,B,C,D, i)	L19(12,13,14,15, E,F,G,H, i)	\
	L20(0,1,2,3, a,b,c,d, i)	L20(4,5,6,7, e,f,g,h, i)	L20(8,9,10,11, A,B,C,D, i)	L20(12,13,14,15, E,F,G,H, i)	\
	L21(0,1,2,3, a,b,c,d, i)	L21(4,5,6,7, e,f,g,h, i)	L21(8,9,10,11, A,B,C,D, i)	L21(12,13,14,15, E,F,G,H, i)	\
	L22(0,1,2,3, a,b,c,d, i)	L22(4,5,6,7, e,f,g,h, i)	L22(8,9,10,11, A,B,C,D, i)	L22(12,13,14,15, E,F,G,H, i)	\
	L23(0,1,2,3, a,b,c,d, i)	L23(4,5,6,7, e,f,g,h, i)	L23(8,9,10,11, A,B,C,D, i)	L23(12,13,14,15, E,F,G,H, i)	\
	L24(0,1,2,3, a,b,c,d, i)	L24(4,5,6,7, e,f,g,h, i)	L24(8,9,10,11, A,B,C,D, i)	L24(12,13,14,15, E,F,G,H, i)	\
	L25(0,1,2,3, a,b,c,d, i)	L25(4,5,6,7, e,f,g,h, i)	L25(8,9,10,11, A,B,C,D, i)	L25(12,13,14,15, E,F,G,H, i)	\
	L26(0,1,2,3, a,b,c,d, i)	L26(4,5,6,7, e,f,g,h, i)	L26(8,9,10,11, A,B,C,D, i)	L26(12,13,14,15, E,F,G,H, i)	\
	L27(0,1,2,3, a,b,c,d, i)	L27(4,5,6,7, e,f,g,h, i)	L27(8,9,10,11, A,B,C,D, i)	L27(12,13,14,15, E,F,G,H, i)	\
	L28(0,1,2,3, a,b,c,d, i)	L28(4,5,6,7, e,f,g,h, i)	L28(8,9,10,11, A,B,C,D, i)	L28(12,13,14,15, E,F,G,H, i)	\
	L29(0,1,2,3, a,b,c,d, i)	L29(4,5,6,7, e,f,g,h, i)	L29(8,9,10,11, A,B,C,D, i)	L29(12,13,14,15, E,F,G,H, i)	\
	L30(0,1,2,3, a,b,c,d, i)	L30(4,5,6,7, e,f,g,h, i)	L30(8,9,10,11, A,B,C,D, i)	L30(12,13,14,15, E,F,G,H, i)	\
	L31(0,1,2,3, a,b,c,d, i)	L31(4,5,6,7, e,f,g,h, i)	L31(8,9,10,11, A,B,C,D, i)	L31(12,13,14,15, E,F,G,H, i)	\
	L32(0,1,2,3, a,b,c,d, i)	L32(4,5,6,7, e,f,g,h, i)	L32(8,9,10,11, A,B,C,D, i)	L32(12,13,14,15, E,F,G,H, i)

#if CRYPTOPP_BOOL_X64
		SSE2_QUARTER_ROUND_X16(1, 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15)
#else
		SSE2_QUARTER_ROUND_X8(1, 2, 6, 10, 14, 3, 7, 11, 15)
		SSE2_QUARTER_ROUND_X8(1, 0, 4, 8, 12, 1, 5, 9, 13)
#endif
		AS2(	mov		REG_roundsLeft, REG_rounds)
		ASJ(	jmp,	2, f)

		ASL(SSE2_Salsa_Output)
		AS2(	movdqa		xmm0, xmm4)
		AS2(	punpckldq	xmm4, xmm5)
		AS2(	movdqa		xmm1, xmm6)
		AS2(	punpckldq	xmm6, xmm7)
		AS2(	movdqa		xmm2, xmm4)
		AS2(	punpcklqdq	xmm4, xmm6)	// e
		AS2(	punpckhqdq	xmm2, xmm6)	// f
		AS2(	punpckhdq	xmm0, xmm5)
		AS2(	punpckhdq	xmm1, xmm7)
		AS2(	movdqa		xmm6, xmm0)
		AS2(	punpcklqdq	xmm0, xmm1)	// g
		AS2(	punpckhqdq	xmm6, xmm1)	// h
		AS_XMM_OUTPUT4(SSE2_Salsa_Output_A, REG_input, REG_output, 4, 2, 0, 6, 1, 0, 4, 8, 12, 1)
		AS1(	ret)

		ASL(6)
#if CRYPTOPP_BOOL_X64
		SSE2_QUARTER_ROUND_X16(0, 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15)
		ASL(2)
		SSE2_QUARTER_ROUND_X16(0, 0, 13, 10, 7, 1, 14, 11, 4, 2, 15, 8, 5, 3, 12, 9, 6)
#else
		SSE2_QUARTER_ROUND_X8(0, 2, 6, 10, 14, 3, 7, 11, 15)
		SSE2_QUARTER_ROUND_X8(0, 0, 4, 8, 12, 1, 5, 9, 13)
		ASL(2)
		SSE2_QUARTER_ROUND_X8(0, 2, 15, 8, 5, 3, 12, 9, 6)
		SSE2_QUARTER_ROUND_X8(0, 0, 13, 10, 7, 1, 14, 11, 4)
#endif
		AS2(	sub		REG_roundsLeft, 2)
		ASJ(	jnz,	6, b)

#define SSE2_OUTPUT_4(a, b, c, d)	\
	AS2(	movdqa		xmm4, [SSE2_WORKSPACE + a*16 + 256])\
	AS2(	paddd		xmm4, [SSE2_WORKSPACE + a*16])\
	AS2(	movdqa		xmm5, [SSE2_WORKSPACE + b*16 + 256])\
	AS2(	paddd		xmm5, [SSE2_WORKSPACE + b*16])\
	AS2(	movdqa		xmm6, [SSE2_WORKSPACE + c*16 + 256])\
	AS2(	paddd		xmm6, [SSE2_WORKSPACE + c*16])\
	AS2(	movdqa		xmm7, [SSE2_WORKSPACE + d*16 + 256])\
	AS2(	paddd		xmm7, [SSE2_WORKSPACE + d*16])\
	ASC(	call,		SSE2_Salsa_Output)

		SSE2_OUTPUT_4(0, 13, 10, 7)
		SSE2_OUTPUT_4(4, 1, 14, 11)
		SSE2_OUTPUT_4(8, 5, 2, 15)
		SSE2_OUTPUT_4(12, 9, 6, 3)
		AS2(	test	REG_input, REG_input)
		ASJ(	jz,		9, f)
		AS2(	add		REG_input, 12*16)
		ASL(9)
		AS2(	add		REG_output, 12*16)
		AS2(	sub		REG_iterationCount, 4)
		AS2(	cmp		REG_iterationCount, 4)
		ASJ(	jge,	1, b)
		AS_POP_IF86(	sp)

		ASL(5)
		AS2(	sub		REG_iterationCount, 1)
		ASJ(	jl,		4, f)
		AS2(	movdqa	xmm0, [REG_state + 0*16])
		AS2(	movdqa	xmm1, [REG_state + 1*16])
		AS2(	movdqa	xmm2, [REG_state + 2*16])
		AS2(	movdqa	xmm3, [REG_state + 3*16])
		AS2(	mov		REG_roundsLeft, REG_rounds)

		ASL(0)
		SSE2_QUARTER_ROUND(0, 1, 3, 7)
		SSE2_QUARTER_ROUND(1, 2, 0, 9)
		SSE2_QUARTER_ROUND(2, 3, 1, 13)
		SSE2_QUARTER_ROUND(3, 0, 2, 18)
		ASS(	pshufd	xmm1, xmm1, 2, 1, 0, 3)
		ASS(	pshufd	xmm2, xmm2, 1, 0, 3, 2)
		ASS(	pshufd	xmm3, xmm3, 0, 3, 2, 1)
		SSE2_QUARTER_ROUND(0, 3, 1, 7)
		SSE2_QUARTER_ROUND(3, 2, 0, 9)
		SSE2_QUARTER_ROUND(2, 1, 3, 13)
		SSE2_QUARTER_ROUND(1, 0, 2, 18)
		ASS(	pshufd	xmm1, xmm1, 0, 3, 2, 1)
		ASS(	pshufd	xmm2, xmm2, 1, 0, 3, 2)
		ASS(	pshufd	xmm3, xmm3, 2, 1, 0, 3)
		AS2(	sub		REG_roundsLeft, 2)
		ASJ(	jnz,	0, b)

		AS2(	paddd	xmm0, [REG_state + 0*16])
		AS2(	paddd	xmm1, [REG_state + 1*16])
		AS2(	paddd	xmm2, [REG_state + 2*16])
		AS2(	paddd	xmm3, [REG_state + 3*16])

		AS2(	add		dword ptr [REG_state + 8*4], 1)
		AS2(	adc		dword ptr [REG_state + 5*4], 0)

		AS2(	pcmpeqb	xmm6, xmm6)			// all ones
		AS2(	psrlq	xmm6, 32)			// lo32 mask
		ASS(	pshufd	xmm7, xmm6, 0, 1, 2, 3)		// hi32 mask
		AS2(	movdqa	xmm4, xmm0)
		AS2(	movdqa	xmm5, xmm3)
		AS2(	pand	xmm0, xmm7)
		AS2(	pand	xmm4, xmm6)
		AS2(	pand	xmm3, xmm6)
		AS2(	pand	xmm5, xmm7)
		AS2(	por		xmm4, xmm5)			// 0,13,2,15
		AS2(	movdqa	xmm5, xmm1)
		AS2(	pand	xmm1, xmm7)
		AS2(	pand	xmm5, xmm6)
		AS2(	por		xmm0, xmm5)			// 4,1,6,3
		AS2(	pand	xmm6, xmm2)
		AS2(	pand	xmm2, xmm7)
		AS2(	por		xmm1, xmm6)			// 8,5,10,7
		AS2(	por		xmm2, xmm3)			// 12,9,14,11

		AS2(	movdqa	xmm5, xmm4)
		AS2(	movdqa	xmm6, xmm0)
		AS3(	shufpd	xmm4, xmm1, 2)		// 0,13,10,7
		AS3(	shufpd	xmm0, xmm2, 2)		// 4,1,14,11
		AS3(	shufpd	xmm1, xmm5, 2)		// 8,5,2,15
		AS3(	shufpd	xmm2, xmm6, 2)		// 12,9,6,3

		// output keystream
		AS_XMM_OUTPUT4(SSE2_Salsa_Output_B, REG_input, REG_output, 4, 0, 1, 2, 3, 0, 1, 2, 3, 4)
		ASJ(	jmp,	5, b)
		ASL(4)

		AS_POP_IF86(	bp)
#ifdef __GNUC__
		AS_POP_IF86(	bx)
		ATT_PREFIX
	#if CRYPTOPP_BOOL_X64
			: "+r" (input), "+r" (output), "+r" (iterationCount)
			: "r" (m_rounds), "r" (m_state.m_ptr), "r" (workspace)
			: "%eax", "%rdx", "memory", "cc", "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7", "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14", "%xmm15"
	#else
			: "+a" (input), "+D" (output), "+c" (iterationCount)
			: "d" (m_rounds), "S" (m_state.m_ptr)
			: "memory", "cc"
	#endif
		);
#endif
#ifdef CRYPTOPP_GENERATE_X64_MASM
	movdqa	xmm6, [rsp + 0200h]
	movdqa	xmm7, [rsp + 0210h]
	movdqa	xmm8, [rsp + 0220h]
	movdqa	xmm9, [rsp + 0230h]
	movdqa	xmm10, [rsp + 0240h]
	movdqa	xmm11, [rsp + 0250h]
	movdqa	xmm12, [rsp + 0260h]
	movdqa	xmm13, [rsp + 0270h]
	movdqa	xmm14, [rsp + 0280h]
	movdqa	xmm15, [rsp + 0290h]
	add		rsp, 10*16 + 32*16 + 8
	ret
Salsa20_OperateKeystream ENDP
#else
	}
	else
#endif
#endif
#ifndef CRYPTOPP_GENERATE_X64_MASM
	{
		word32 x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;

		while (iterationCount--)
		{
			x0 = m_state[0];	x1 = m_state[1];	x2 = m_state[2];	x3 = m_state[3];
			x4 = m_state[4];	x5 = m_state[5];	x6 = m_state[6];	x7 = m_state[7];
			x8 = m_state[8];	x9 = m_state[9];	x10 = m_state[10];	x11 = m_state[11];
			x12 = m_state[12];	x13 = m_state[13];	x14 = m_state[14];	x15 = m_state[15];

			for (int i=m_rounds; i>0; i-=2)
			{
				#define QUARTER_ROUND(a, b, c, d)	\
					b = b ^ rotlFixed(a + d, 7);	\
					c = c ^ rotlFixed(b + a, 9);	\
					d = d ^ rotlFixed(c + b, 13);	\
					a = a ^ rotlFixed(d + c, 18);

				QUARTER_ROUND(x0, x4, x8, x12)
				QUARTER_ROUND(x1, x5, x9, x13)
				QUARTER_ROUND(x2, x6, x10, x14)
				QUARTER_ROUND(x3, x7, x11, x15)

				QUARTER_ROUND(x0, x13, x10, x7)
				QUARTER_ROUND(x1, x14, x11, x4)
				QUARTER_ROUND(x2, x15, x8, x5)
				QUARTER_ROUND(x3, x12, x9, x6)
			}

			#define SALSA_OUTPUT(x)	{\
				CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 0, x0 + m_state[0]);\
				CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 1, x13 + m_state[13]);\
				CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 2, x10 + m_state[10]);\
				CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 3, x7 + m_state[7]);\
				CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 4, x4 + m_state[4]);\
				CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 5, x1 + m_state[1]);\
				CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 6, x14 + m_state[14]);\
				CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 7, x11 + m_state[11]);\
				CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 8, x8 + m_state[8]);\
				CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 9, x5 + m_state[5]);\
				CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 10, x2 + m_state[2]);\
				CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 11, x15 + m_state[15]);\
				CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 12, x12 + m_state[12]);\
				CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 13, x9 + m_state[9]);\
				CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 14, x6 + m_state[6]);\
				CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 15, x3 + m_state[3]);}

#ifndef CRYPTOPP_DOXYGEN_PROCESSING
			CRYPTOPP_KEYSTREAM_OUTPUT_SWITCH(SALSA_OUTPUT, BYTES_PER_ITERATION);
#endif

			if (++m_state[8] == 0)
				++m_state[5];
		}
	}
}	// see comment above if an internal compiler error occurs here

void XSalsa20_Policy::CipherSetKey(const NameValuePairs &params, const byte *key, size_t length)
{
	m_rounds = params.GetIntValueWithDefault(Name::Rounds(), 20);

	if (!(m_rounds == 8 || m_rounds == 12 || m_rounds == 20))
		throw InvalidRounds(XSalsa20::StaticAlgorithmName(), m_rounds);

	GetUserKey(LITTLE_ENDIAN_ORDER, m_key.begin(), m_key.size(), key, length);
	if (length == 16)
		memcpy(m_key.begin()+4, m_key.begin(), 16);

	// "expand 32-byte k"
	m_state[0] = 0x61707865;
	m_state[1] = 0x3320646e;
	m_state[2] = 0x79622d32;
	m_state[3] = 0x6b206574;
}

void XSalsa20_Policy::CipherResynchronize(byte *keystreamBuffer, const byte *IV, size_t length)
{
	CRYPTOPP_UNUSED(keystreamBuffer), CRYPTOPP_UNUSED(length);
	CRYPTOPP_ASSERT(length==24);

	word32 x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;

	GetBlock<word32, LittleEndian> get(IV);
	get(x14)(x11)(x8)(x5)(m_state[14])(m_state[11]);

	x13 = m_key[0];		x10 = m_key[1];		x7 = m_key[2];		x4 = m_key[3];
	x15 = m_key[4];		x12 = m_key[5];		x9 = m_key[6];		x6 = m_key[7];
	x0 = m_state[0];	x1 = m_state[1];	x2 = m_state[2];	x3 = m_state[3];

	for (int i=m_rounds; i>0; i-=2)
	{
		QUARTER_ROUND(x0, x4, x8, x12)
		QUARTER_ROUND(x1, x5, x9, x13)
		QUARTER_ROUND(x2, x6, x10, x14)
		QUARTER_ROUND(x3, x7, x11, x15)

		QUARTER_ROUND(x0, x13, x10, x7)
		QUARTER_ROUND(x1, x14, x11, x4)
		QUARTER_ROUND(x2, x15, x8, x5)
		QUARTER_ROUND(x3, x12, x9, x6)
	}

	m_state[13] = x0;	m_state[10] = x1;	m_state[7] = x2;	m_state[4] = x3;
	m_state[15] = x14;	m_state[12] = x11;	m_state[9] = x8;	m_state[6] = x5;
	m_state[8] = m_state[5] = 0;
}

NAMESPACE_END

#endif // #ifndef CRYPTOPP_GENERATE_X64_MASM
