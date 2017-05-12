// tiger.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "config.h"

#include "tiger.h"
#include "misc.h"
#include "cpu.h"

#if defined(CRYPTOPP_DISABLE_TIGER_ASM)
# undef CRYPTOPP_X86_ASM_AVAILABLE
# undef CRYPTOPP_X32_ASM_AVAILABLE
# undef CRYPTOPP_X64_ASM_AVAILABLE
# undef CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
#endif

NAMESPACE_BEGIN(CryptoPP)

void Tiger::InitState(HashWordType *state)
{
	state[0] = W64LIT(0x0123456789ABCDEF);
	state[1] = W64LIT(0xFEDCBA9876543210);
	state[2] = W64LIT(0xF096A5B4C3B2E187);
}

void Tiger::TruncatedFinal(byte *hash, size_t size)
{
	ThrowIfInvalidTruncatedSize(size);

	PadLastBlock(56, 0x01);
	CorrectEndianess(m_data, m_data, 56);

	m_data[7] = GetBitCountLo();

	Transform(m_state, m_data);
	CorrectEndianess(m_state, m_state, DigestSize());
	memcpy(hash, m_state, size);

	Restart();		// reinit for next use
}

void Tiger::Transform (word64 *digest, const word64 *X)
{
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE && (CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32)
	if (HasSSE2())
	{
#ifdef __GNUC__
		__asm__ __volatile__
		(
		INTEL_NOPREFIX
		AS_PUSH_IF86(bx)
#else
	#if _MSC_VER < 1300
		const word64 *t = table;
		AS2(	mov		edx, t)
	#else
		AS2(	lea		edx, [table])
	#endif
		AS2(	mov		eax, digest)
		AS2(	mov		esi, X)
#endif
		AS2(	movq	mm0, [eax])
		AS2(	movq	mm1, [eax+1*8])
		AS2(	movq	mm5, mm1)
		AS2(	movq	mm2, [eax+2*8])
		AS2(	movq	mm7, [edx+4*2048+0*8])
		AS2(	movq	mm6, [edx+4*2048+1*8])
		AS2(	mov		ecx, esp)
		AS2(	and		esp, 0xfffffff0)
		AS2(	sub		esp, 8*8)
		AS_PUSH_IF86(cx)

#define SSE2_round(a,b,c,x,mul) \
		AS2(	pxor	c, [x])\
		AS2(	movd	ecx, c)\
		AS2(	movzx	edi, cl)\
		AS2(	movq	mm3, [edx+0*2048+edi*8])\
		AS2(	movzx	edi, ch)\
		AS2(	movq	mm4, [edx+3*2048+edi*8])\
		AS2(	shr		ecx, 16)\
		AS2(	movzx	edi, cl)\
		AS2(	pxor	mm3, [edx+1*2048+edi*8])\
		AS2(	movzx	edi, ch)\
		AS2(	pxor	mm4, [edx+2*2048+edi*8])\
		AS3(	pextrw	ecx, c, 2)\
		AS2(	movzx	edi, cl)\
		AS2(	pxor	mm3, [edx+2*2048+edi*8])\
		AS2(	movzx	edi, ch)\
		AS2(	pxor	mm4, [edx+1*2048+edi*8])\
		AS3(	pextrw	ecx, c, 3)\
		AS2(	movzx	edi, cl)\
		AS2(	pxor	mm3, [edx+3*2048+edi*8])\
		AS2(	psubq	a, mm3)\
		AS2(	movzx	edi, ch)\
		AS2(	pxor	mm4, [edx+0*2048+edi*8])\
		AS2(	paddq	b, mm4)\
		SSE2_mul_##mul(b)

#define SSE2_mul_5(b)	\
		AS2(	movq	mm3, b)\
		AS2(	psllq	b, 2)\
		AS2(	paddq	b, mm3)

#define SSE2_mul_7(b)	\
		AS2(	movq	mm3, b)\
		AS2(	psllq	b, 3)\
		AS2(	psubq	b, mm3)

#define SSE2_mul_9(b)	\
		AS2(	movq	mm3, b)\
		AS2(	psllq	b, 3)\
		AS2(	paddq	b, mm3)

#define label2_5 1
#define label2_7 2
#define label2_9 3

#define SSE2_pass(A,B,C,mul,X)	\
		AS2(	xor		ebx, ebx)\
		ASL(mul)\
		SSE2_round(A,B,C,X+0*8+ebx,mul)\
		SSE2_round(B,C,A,X+1*8+ebx,mul)\
		AS2(	cmp		ebx, 6*8)\
		ASJ(	je,		label2_##mul, f)\
		SSE2_round(C,A,B,X+2*8+ebx,mul)\
		AS2(	add		ebx, 3*8)\
		ASJ(	jmp,	mul, b)\
		ASL(label2_##mul)

#define SSE2_key_schedule(Y,X) \
		AS2(	movq	mm3, [X+7*8])\
		AS2(	pxor	mm3, mm6)\
		AS2(	movq	mm4, [X+0*8])\
		AS2(	psubq	mm4, mm3)\
		AS2(	movq	[Y+0*8], mm4)\
		AS2(	pxor	mm4, [X+1*8])\
		AS2(	movq	mm3, mm4)\
		AS2(	movq	[Y+1*8], mm4)\
		AS2(	paddq	mm4, [X+2*8])\
		AS2(	pxor	mm3, mm7)\
		AS2(	psllq	mm3, 19)\
		AS2(	movq	[Y+2*8], mm4)\
		AS2(	pxor	mm3, mm4)\
		AS2(	movq	mm4, [X+3*8])\
		AS2(	psubq	mm4, mm3)\
		AS2(	movq	[Y+3*8], mm4)\
		AS2(	pxor	mm4, [X+4*8])\
		AS2(	movq	mm3, mm4)\
		AS2(	movq	[Y+4*8], mm4)\
		AS2(	paddq	mm4, [X+5*8])\
		AS2(	pxor	mm3, mm7)\
		AS2(	psrlq	mm3, 23)\
		AS2(	movq	[Y+5*8], mm4)\
		AS2(	pxor	mm3, mm4)\
		AS2(	movq	mm4, [X+6*8])\
		AS2(	psubq	mm4, mm3)\
		AS2(	movq	[Y+6*8], mm4)\
		AS2(	pxor	mm4, [X+7*8])\
		AS2(	movq	mm3, mm4)\
		AS2(	movq	[Y+7*8], mm4)\
		AS2(	paddq	mm4, [Y+0*8])\
		AS2(	pxor	mm3, mm7)\
		AS2(	psllq	mm3, 19)\
		AS2(	movq	[Y+0*8], mm4)\
		AS2(	pxor	mm3, mm4)\
		AS2(	movq	mm4, [Y+1*8])\
		AS2(	psubq	mm4, mm3)\
		AS2(	movq	[Y+1*8], mm4)\
		AS2(	pxor	mm4, [Y+2*8])\
		AS2(	movq	mm3, mm4)\
		AS2(	movq	[Y+2*8], mm4)\
		AS2(	paddq	mm4, [Y+3*8])\
		AS2(	pxor	mm3, mm7)\
		AS2(	psrlq	mm3, 23)\
		AS2(	movq	[Y+3*8], mm4)\
		AS2(	pxor	mm3, mm4)\
		AS2(	movq	mm4, [Y+4*8])\
		AS2(	psubq	mm4, mm3)\
		AS2(	movq	[Y+4*8], mm4)\
		AS2(	pxor	mm4, [Y+5*8])\
		AS2(	movq	[Y+5*8], mm4)\
		AS2(	paddq	mm4, [Y+6*8])\
		AS2(	movq	[Y+6*8], mm4)\
		AS2(	pxor	mm4, [edx+4*2048+2*8])\
		AS2(	movq	mm3, [Y+7*8])\
		AS2(	psubq	mm3, mm4)\
		AS2(	movq	[Y+7*8], mm3)

#if CRYPTOPP_BOOL_X32
		SSE2_pass(mm0, mm1, mm2, 5, esi)
		SSE2_key_schedule(esp+8, esi)
		SSE2_pass(mm2, mm0, mm1, 7, esp+8)
		SSE2_key_schedule(esp+8, esp+8)
		SSE2_pass(mm1, mm2, mm0, 9, esp+8)
#else
		SSE2_pass(mm0, mm1, mm2, 5, esi)
		SSE2_key_schedule(esp+4, esi)
		SSE2_pass(mm2, mm0, mm1, 7, esp+4)
		SSE2_key_schedule(esp+4, esp+4)
		SSE2_pass(mm1, mm2, mm0, 9, esp+4)
#endif

		AS2(	pxor	mm0, [eax+0*8])
		AS2(	movq	[eax+0*8], mm0)
		AS2(	psubq	mm1, mm5)
		AS2(	movq	[eax+1*8], mm1)
		AS2(	paddq	mm2, [eax+2*8])
		AS2(	movq	[eax+2*8], mm2)

		AS_POP_IF86(sp)
		AS1(	emms)

#ifdef __GNUC__
		AS_POP_IF86(bx)
		ATT_PREFIX
			:
			: "a" (digest), "S" (X), "d" (table)
			: "%ecx", "%edi", "memory", "cc"
		);
#endif
	}
	else
#endif
	{
		word64 a = digest[0];
		word64 b = digest[1];
		word64 c = digest[2];
		word64 Y[8];

#define t1 (table)
#define t2 (table+256)
#define t3 (table+256*2)
#define t4 (table+256*3)

#define round(a,b,c,x,mul) \
	c ^= x; \
	a -= t1[GETBYTE(c,0)] ^ t2[GETBYTE(c,2)] ^ t3[GETBYTE(c,4)] ^ t4[GETBYTE(c,6)]; \
	b += t4[GETBYTE(c,1)] ^ t3[GETBYTE(c,3)] ^ t2[GETBYTE(c,5)] ^ t1[GETBYTE(c,7)]; \
	b *= mul

#define pass(a,b,c,mul,X) {\
	int i=0;\
	while (true)\
	{\
		round(a,b,c,X[i+0],mul); \
		round(b,c,a,X[i+1],mul); \
		if (i==6)\
			break;\
		round(c,a,b,X[i+2],mul); \
		i+=3;\
	}}

#define key_schedule(Y,X) \
	Y[0] = X[0] - (X[7]^W64LIT(0xA5A5A5A5A5A5A5A5)); \
	Y[1] = X[1] ^ Y[0]; \
	Y[2] = X[2] + Y[1]; \
	Y[3] = X[3] - (Y[2] ^ ((~Y[1])<<19)); \
	Y[4] = X[4] ^ Y[3]; \
	Y[5] = X[5] + Y[4]; \
	Y[6] = X[6] - (Y[5] ^ ((~Y[4])>>23)); \
	Y[7] = X[7] ^ Y[6]; \
	Y[0] += Y[7]; \
	Y[1] -= Y[0] ^ ((~Y[7])<<19); \
	Y[2] ^= Y[1]; \
	Y[3] += Y[2]; \
	Y[4] -= Y[3] ^ ((~Y[2])>>23); \
	Y[5] ^= Y[4]; \
	Y[6] += Y[5]; \
	Y[7] -= Y[6] ^ W64LIT(0x0123456789ABCDEF)

		pass(a,b,c,5,X);
		key_schedule(Y,X);
		pass(c,a,b,7,Y);
		key_schedule(Y,Y);
		pass(b,c,a,9,Y);

		digest[0] = a ^ digest[0];
		digest[1] = b - digest[1];
		digest[2] = c + digest[2];
	}
}

NAMESPACE_END
