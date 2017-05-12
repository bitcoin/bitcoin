// panama.cpp - written and placed in the public domain by Wei Dai

// use "cl /EP /P /DCRYPTOPP_GENERATE_X64_MASM panama.cpp" to generate MASM code

#include "pch.h"

#ifndef CRYPTOPP_GENERATE_X64_MASM

#include "panama.h"
#include "secblock.h"
#include "misc.h"
#include "cpu.h"

NAMESPACE_BEGIN(CryptoPP)

#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4731)
#endif

template <class B>
void Panama<B>::Reset()
{
	memset(m_state, 0, m_state.SizeInBytes());
#if CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE && !defined(CRYPTOPP_DISABLE_PANAMA_ASM)
	m_state[17] = HasSSSE3();
#endif
}

#endif	// #ifndef CRYPTOPP_GENERATE_X64_MASM

#ifdef CRYPTOPP_X64_MASM_AVAILABLE
extern "C" {
void Panama_SSE2_Pull(size_t count, word32 *state, word32 *z, const word32 *y);
}
#elif CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE && !defined(CRYPTOPP_DISABLE_PANAMA_ASM)

#ifdef CRYPTOPP_GENERATE_X64_MASM
	Panama_SSE2_Pull	PROC FRAME
	rex_push_reg rdi
	alloc_stack(2*16)
	save_xmm128 xmm6, 0h
	save_xmm128 xmm7, 10h
	.endprolog
#else
void CRYPTOPP_NOINLINE Panama_SSE2_Pull(size_t count, word32 *state, word32 *z, const word32 *y)
{
#if defined(CRYPTOPP_GNU_STYLE_INLINE_ASSEMBLY)
	asm __volatile__
	(
	INTEL_NOPREFIX
	AS_PUSH_IF86(	bx)
#else
	AS2(	mov		AS_REG_1, count)
	AS2(	mov		AS_REG_2, state)
	AS2(	mov		AS_REG_3, z)
	AS2(	mov		AS_REG_4, y)
#endif
#endif	// #ifdef CRYPTOPP_GENERATE_X64_MASM

#if CRYPTOPP_BOOL_X32
	#define REG_loopEnd			r8d
#elif CRYPTOPP_BOOL_X86
	#define REG_loopEnd			[esp]
#elif defined(CRYPTOPP_GENERATE_X64_MASM)
	#define REG_loopEnd			rdi
#else
	#define REG_loopEnd			r8
#endif

	AS2(	shl		AS_REG_1, 5)
	ASJ(	jz,		5, f)
	AS2(	mov		AS_REG_6d, [AS_REG_2+4*17])
	AS2(	add		AS_REG_1, AS_REG_6)

	#if CRYPTOPP_BOOL_X64
		AS2(	mov		REG_loopEnd, AS_REG_1)
	#else
		AS_PUSH_IF86(	bp)
		// AS1(	push	AS_REG_1) // AS_REG_1 is defined as ecx uner X86 and X32 (see cpu.h)
		AS_PUSH_IF86(	cx)
	#endif

	AS2(	movdqa	xmm0, XMMWORD_PTR [AS_REG_2+0*16])
	AS2(	movdqa	xmm1, XMMWORD_PTR [AS_REG_2+1*16])
	AS2(	movdqa	xmm2, XMMWORD_PTR [AS_REG_2+2*16])
	AS2(	movdqa	xmm3, XMMWORD_PTR [AS_REG_2+3*16])
	AS2(	mov		eax, dword ptr [AS_REG_2+4*16])

	ASL(4)
	// gamma and pi
#if CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE
	AS2(	test	AS_REG_6, 1)
	ASJ(	jnz,	6, f)
#endif
	AS2(	movdqa	xmm6, xmm2)
	AS2(	movss	xmm6, xmm3)
	ASS(	pshufd	xmm5, xmm6, 0, 3, 2, 1)
	AS2(	movd	xmm6, eax)
	AS2(	movdqa	xmm7, xmm3)
	AS2(	movss	xmm7, xmm6)
	ASS(	pshufd	xmm6, xmm7, 0, 3, 2, 1)
#if CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE
	ASJ(	jmp,	7, f)
	ASL(6)
	AS2(	movdqa	xmm5, xmm3)
	AS3(	palignr	xmm5, xmm2, 4)
	AS2(	movd	xmm6, eax)
	AS3(	palignr	xmm6, xmm3, 4)
	ASL(7)
#endif

	AS2(	movd	AS_REG_1d, xmm2)
	AS1(	not		AS_REG_1d)
	AS2(	movd	AS_REG_7d, xmm3)
	AS2(	or		AS_REG_1d, AS_REG_7d)
	AS2(	xor		eax, AS_REG_1d)

#define SSE2_Index(i) ASM_MOD(((i)*13+16), 17)

#define pi(i)	\
	AS2(	movd	AS_REG_1d, xmm7)\
	AS2(	rol		AS_REG_1d, ASM_MOD((ASM_MOD(5*i,17)*(ASM_MOD(5*i,17)+1)/2), 32))\
	AS2(	mov		[AS_REG_2+SSE2_Index(ASM_MOD(5*(i), 17))*4], AS_REG_1d)

#define pi4(x, y, z, a, b, c, d)	\
	AS2(	pcmpeqb	xmm7, xmm7)\
	AS2(	pxor	xmm7, x)\
	AS2(	por		xmm7, y)\
	AS2(	pxor	xmm7, z)\
	pi(a)\
	ASS(	pshuflw	xmm7, xmm7, 1, 0, 3, 2)\
	pi(b)\
	AS2(	punpckhqdq	xmm7, xmm7)\
	pi(c)\
	ASS(	pshuflw	xmm7, xmm7, 1, 0, 3, 2)\
	pi(d)

	pi4(xmm1, xmm2, xmm3, 1, 5, 9, 13)
	pi4(xmm0, xmm1, xmm2, 2, 6, 10, 14)
	pi4(xmm6, xmm0, xmm1, 3, 7, 11, 15)
	pi4(xmm5, xmm6, xmm0, 4, 8, 12, 16)

	// output keystream and update buffer here to hide partial memory stalls between pi and theta
	AS2(	movdqa	xmm4, xmm3)
	AS2(	punpcklqdq	xmm3, xmm2)		// 1 5 2 6
	AS2(	punpckhdq	xmm4, xmm2)		// 9 10 13 14
	AS2(	movdqa	xmm2, xmm1)
	AS2(	punpcklqdq	xmm1, xmm0)		// 3 7 4 8
	AS2(	punpckhdq	xmm2, xmm0)		// 11 12 15 16

	// keystream
	AS2(	test	AS_REG_3, AS_REG_3)
	ASJ(	jz,		0, f)
	AS2(	movdqa	xmm6, xmm4)
	AS2(	punpcklqdq	xmm4, xmm2)
	AS2(	punpckhqdq	xmm6, xmm2)
	AS2(	test	AS_REG_4, 15)
	ASJ(	jnz,	2, f)
	AS2(	test	AS_REG_4, AS_REG_4)
	ASJ(	jz,		1, f)
	AS2(	pxor	xmm4, [AS_REG_4])
	AS2(	pxor	xmm6, [AS_REG_4+16])
	AS2(	add		AS_REG_4, 32)
	ASJ(	jmp,	1, f)
	ASL(2)
	AS2(	movdqu	xmm0, [AS_REG_4])
	AS2(	movdqu	xmm2, [AS_REG_4+16])
	AS2(	pxor	xmm4, xmm0)
	AS2(	pxor	xmm6, xmm2)
	AS2(	add		AS_REG_4, 32)
	ASL(1)
	AS2(	test	AS_REG_3, 15)
	ASJ(	jnz,	3, f)
	AS2(	movdqa	XMMWORD_PTR [AS_REG_3], xmm4)
	AS2(	movdqa	XMMWORD_PTR [AS_REG_3+16], xmm6)
	AS2(	add		AS_REG_3, 32)
	ASJ(	jmp,	0, f)
	ASL(3)
	AS2(	movdqu	XMMWORD_PTR [AS_REG_3], xmm4)
	AS2(	movdqu	XMMWORD_PTR [AS_REG_3+16], xmm6)
	AS2(	add		AS_REG_3, 32)
	ASL(0)

	// buffer update
	AS2(	lea		AS_REG_1, [AS_REG_6 + 32])
	AS2(	and		AS_REG_1, 31*32)
	AS2(	lea		AS_REG_7, [AS_REG_6 + (32-24)*32])
	AS2(	and		AS_REG_7, 31*32)

	AS2(	movdqa	xmm0, XMMWORD_PTR [AS_REG_2+20*4+AS_REG_1+0*8])
	AS2(	pxor	xmm3, xmm0)
	ASS(	pshufd	xmm0, xmm0, 2, 3, 0, 1)
	AS2(	movdqa	XMMWORD_PTR [AS_REG_2+20*4+AS_REG_1+0*8], xmm3)
	AS2(	pxor	xmm0, XMMWORD_PTR [AS_REG_2+20*4+AS_REG_7+2*8])
	AS2(	movdqa	XMMWORD_PTR [AS_REG_2+20*4+AS_REG_7+2*8], xmm0)

	AS2(	movdqa	xmm4, XMMWORD_PTR [AS_REG_2+20*4+AS_REG_1+2*8])
	AS2(	pxor	xmm1, xmm4)
	AS2(	movdqa	XMMWORD_PTR [AS_REG_2+20*4+AS_REG_1+2*8], xmm1)
	AS2(	pxor	xmm4, XMMWORD_PTR [AS_REG_2+20*4+AS_REG_7+0*8])
	AS2(	movdqa	XMMWORD_PTR [AS_REG_2+20*4+AS_REG_7+0*8], xmm4)

	// theta
	AS2(	movdqa	xmm3, XMMWORD_PTR [AS_REG_2+3*16])
	AS2(	movdqa	xmm2, XMMWORD_PTR [AS_REG_2+2*16])
	AS2(	movdqa	xmm1, XMMWORD_PTR [AS_REG_2+1*16])
	AS2(	movdqa	xmm0, XMMWORD_PTR [AS_REG_2+0*16])

#if CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE
	AS2(	test	AS_REG_6, 1)
	ASJ(	jnz,	8, f)
#endif
	AS2(	movd	xmm6, eax)
	AS2(	movdqa	xmm7, xmm3)
	AS2(	movss	xmm7, xmm6)
	AS2(	movdqa	xmm6, xmm2)
	AS2(	movss	xmm6, xmm3)
	AS2(	movdqa	xmm5, xmm1)
	AS2(	movss	xmm5, xmm2)
	AS2(	movdqa	xmm4, xmm0)
	AS2(	movss	xmm4, xmm1)
	ASS(	pshufd	xmm7, xmm7, 0, 3, 2, 1)
	ASS(	pshufd	xmm6, xmm6, 0, 3, 2, 1)
	ASS(	pshufd	xmm5, xmm5, 0, 3, 2, 1)
	ASS(	pshufd	xmm4, xmm4, 0, 3, 2, 1)
#if CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE
	ASJ(	jmp,	9, f)
	ASL(8)
	AS2(	movd	xmm7, eax)
	AS3(	palignr	xmm7, xmm3, 4)
	AS2(	movq	xmm6, xmm3)
	AS3(	palignr	xmm6, xmm2, 4)
	AS2(	movq	xmm5, xmm2)
	AS3(	palignr	xmm5, xmm1, 4)
	AS2(	movq	xmm4, xmm1)
	AS3(	palignr	xmm4, xmm0, 4)
	ASL(9)
#endif

	AS2(	xor		eax, 1)
	AS2(	movd	AS_REG_1d, xmm0)
	AS2(	xor		eax, AS_REG_1d)
	AS2(	movd	AS_REG_1d, xmm3)
	AS2(	xor		eax, AS_REG_1d)

	AS2(	pxor	xmm3, xmm2)
	AS2(	pxor	xmm2, xmm1)
	AS2(	pxor	xmm1, xmm0)
	AS2(	pxor	xmm0, xmm7)
	AS2(	pxor	xmm3, xmm7)
	AS2(	pxor	xmm2, xmm6)
	AS2(	pxor	xmm1, xmm5)
	AS2(	pxor	xmm0, xmm4)

	// sigma
	AS2(	lea		AS_REG_1, [AS_REG_6 + (32-4)*32])
	AS2(	and		AS_REG_1, 31*32)
	AS2(	lea		AS_REG_7, [AS_REG_6 + 16*32])
	AS2(	and		AS_REG_7, 31*32)

	AS2(	movdqa	xmm4, XMMWORD_PTR [AS_REG_2+20*4+AS_REG_1+0*16])
	AS2(	movdqa	xmm5, XMMWORD_PTR [AS_REG_2+20*4+AS_REG_7+0*16])
	AS2(	movdqa	xmm6, xmm4)
	AS2(	punpcklqdq	xmm4, xmm5)
	AS2(	punpckhqdq	xmm6, xmm5)
	AS2(	pxor	xmm3, xmm4)
	AS2(	pxor	xmm2, xmm6)

	AS2(	movdqa	xmm4, XMMWORD_PTR [AS_REG_2+20*4+AS_REG_1+1*16])
	AS2(	movdqa	xmm5, XMMWORD_PTR [AS_REG_2+20*4+AS_REG_7+1*16])
	AS2(	movdqa	xmm6, xmm4)
	AS2(	punpcklqdq	xmm4, xmm5)
	AS2(	punpckhqdq	xmm6, xmm5)
	AS2(	pxor	xmm1, xmm4)
	AS2(	pxor	xmm0, xmm6)

	// loop
	AS2(	add		AS_REG_6, 32)
	AS2(	cmp		AS_REG_6, REG_loopEnd)
	ASJ(	jne,	4, b)

	// save state
	AS2(	mov		[AS_REG_2+4*16], eax)
	AS2(	movdqa	XMMWORD_PTR [AS_REG_2+3*16], xmm3)
	AS2(	movdqa	XMMWORD_PTR [AS_REG_2+2*16], xmm2)
	AS2(	movdqa	XMMWORD_PTR [AS_REG_2+1*16], xmm1)
	AS2(	movdqa	XMMWORD_PTR [AS_REG_2+0*16], xmm0)

	#if CRYPTOPP_BOOL_X32
		AS2(	add		esp, 8)
		AS_POP_IF86(	bp)
	#elif CRYPTOPP_BOOL_X86
		AS2(	add		esp, 4)
		AS_POP_IF86(	bp)
	#endif
	ASL(5)

#if defined(CRYPTOPP_GNU_STYLE_INLINE_ASSEMBLY)
		AS_POP_IF86(	bx)
		ATT_PREFIX
			:
	#if CRYPTOPP_BOOL_X64
			: "D" (count), "S" (state), "d" (z), "c" (y)
			: "%r8", "%r9", "r10", "%eax", "memory", "cc", "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7"
	#else
			: "c" (count), "d" (state), "S" (z), "D" (y)
			: "%eax", "memory", "cc"
	#endif
	);
#endif

#ifdef CRYPTOPP_GENERATE_X64_MASM
	movdqa	xmm6, [rsp + 0h]
	movdqa	xmm7, [rsp + 10h]
	add rsp, 2*16
	pop	rdi
	ret
	Panama_SSE2_Pull ENDP
#else
}
#endif
#endif	// #ifdef CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE

#ifndef CRYPTOPP_GENERATE_X64_MASM

template <class B>
void Panama<B>::Iterate(size_t count, const word32 *p, byte *output, const byte *input, KeystreamOperation operation)
{
	CRYPTOPP_ASSERT(IsAlignedOn(m_state,GetAlignmentOf<word32>()));
	word32 bstart = m_state[17];
	word32 *const aPtr = m_state;
	word32 cPtr[17];

#define bPtr ((byte *)(aPtr+20))

// reorder the state for SSE2
// a and c: 4 8 12 16 | 3 7 11 15 | 2 6 10 14 | 1 5 9 13 | 0
//			xmm0		xmm1		xmm2		xmm3		eax
#define a(i) aPtr[((i)*13+16) % 17]		// 13 is inverse of 4 mod 17
#define c(i) cPtr[((i)*13+16) % 17]
// b: 0 4 | 1 5 | 2 6 | 3 7
#define b(i, j) b##i[(j)*2%8 + (j)/4]

// buffer update
#define US(i) {word32 t=b(0,i); b(0,i)=ConditionalByteReverse(B::ToEnum(), p[i])^t; b(25,(i+6)%8)^=t;}
#define UL(i) {word32 t=b(0,i); b(0,i)=a(i+1)^t; b(25,(i+6)%8)^=t;}
// gamma and pi
#define GP(i) c(5*i%17) = rotlFixed(a(i) ^ (a((i+1)%17) | ~a((i+2)%17)), ((5*i%17)*((5*i%17)+1)/2)%32)
// theta and sigma
#define T(i,x) a(i) = c(i) ^ c((i+1)%17) ^ c((i+4)%17) ^ x
#define TS1S(i) T(i+1, ConditionalByteReverse(B::ToEnum(), p[i]))
#define TS1L(i) T(i+1, b(4,i))
#define TS2(i) T(i+9, b(16,i))

	while (count--)
	{
		if (output)
		{
#define PANAMA_OUTPUT(x)	\
	CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, B::ToEnum(), 0, a(0+9));\
	CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, B::ToEnum(), 1, a(1+9));\
	CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, B::ToEnum(), 2, a(2+9));\
	CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, B::ToEnum(), 3, a(3+9));\
	CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, B::ToEnum(), 4, a(4+9));\
	CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, B::ToEnum(), 5, a(5+9));\
	CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, B::ToEnum(), 6, a(6+9));\
	CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, B::ToEnum(), 7, a(7+9));

			typedef word32 WordType;
			CRYPTOPP_KEYSTREAM_OUTPUT_SWITCH(PANAMA_OUTPUT, 4*8);
		}

		word32 *const b16 = (word32 *)(void *)(bPtr+((bstart+16*32) & 31*32));
		word32 *const b4 = (word32 *)(void *)(bPtr+((bstart+(32-4)*32) & 31*32));
       	bstart += 32;
		word32 *const b0 = (word32 *)(void *)(bPtr+((bstart) & 31*32));
		word32 *const b25 = (word32 *)(void *)(bPtr+((bstart+(32-25)*32) & 31*32));

		if (p)
		{
			US(0); US(1); US(2); US(3); US(4); US(5); US(6); US(7);
		}
		else
		{
			UL(0); UL(1); UL(2); UL(3); UL(4); UL(5); UL(6); UL(7);
		}

		GP(0);
		GP(1);
		GP(2);
		GP(3);
		GP(4);
		GP(5);
		GP(6);
		GP(7);
		GP(8);
		GP(9);
		GP(10);
		GP(11);
		GP(12);
		GP(13);
		GP(14);
		GP(15);
		GP(16);

		T(0,1);

		if (p)
		{
			TS1S(0); TS1S(1); TS1S(2); TS1S(3); TS1S(4); TS1S(5); TS1S(6); TS1S(7);
			p += 8;
		}
		else
		{
			TS1L(0); TS1L(1); TS1L(2); TS1L(3); TS1L(4); TS1L(5); TS1L(6); TS1L(7);
		}

		TS2(0); TS2(1); TS2(2); TS2(3); TS2(4); TS2(5); TS2(6); TS2(7);
	}
	m_state[17] = bstart;
}

namespace Weak {
template <class B>
size_t PanamaHash<B>::HashMultipleBlocks(const word32 *input, size_t length)
{
	this->Iterate(length / this->BLOCKSIZE, input);
	return length % this->BLOCKSIZE;
}

template <class B>
void PanamaHash<B>::TruncatedFinal(byte *hash, size_t size)
{
	this->ThrowIfInvalidTruncatedSize(size);

	this->PadLastBlock(this->BLOCKSIZE, 0x01);

	HashEndianCorrectedBlock(this->m_data);

	this->Iterate(32);	// pull

	FixedSizeSecBlock<word32, 8> buf;
	this->Iterate(1, NULL, buf.BytePtr(), NULL);

	memcpy(hash, buf, size);

	this->Restart();		// reinit for next use
}
}

template <class B>
void PanamaCipherPolicy<B>::CipherSetKey(const NameValuePairs &params, const byte *key, size_t length)
{
	CRYPTOPP_UNUSED(params); CRYPTOPP_UNUSED(length);
	CRYPTOPP_ASSERT(length==32);
	memcpy(m_key, key, 32);
}

template <class B>
void PanamaCipherPolicy<B>::CipherResynchronize(byte *keystreamBuffer, const byte *iv, size_t length)
{
	CRYPTOPP_UNUSED(keystreamBuffer); CRYPTOPP_UNUSED(iv); CRYPTOPP_UNUSED(length);
	CRYPTOPP_ASSERT(IsAlignedOn(iv,GetAlignmentOf<word32>()));
	CRYPTOPP_ASSERT(length==32);

	this->Reset();
	this->Iterate(1, m_key);
	if (iv && IsAligned<word32>(iv))
		this->Iterate(1, (const word32 *)(void *)iv);
	else
	{
		FixedSizeSecBlock<word32, 8> buf;
		if (iv)
			memcpy(buf, iv, 32);
		else
			memset(buf, 0, 32);
		this->Iterate(1, buf);
	}

#if (CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE || defined(CRYPTOPP_X64_MASM_AVAILABLE)) && !defined(CRYPTOPP_DISABLE_PANAMA_ASM)
	if (B::ToEnum() == LITTLE_ENDIAN_ORDER && HasSSE2() && !IsP4())		// SSE2 code is slower on P4 Prescott
		Panama_SSE2_Pull(32, this->m_state, NULL, NULL);
	else
#endif
		this->Iterate(32);
}

template <class B>
unsigned int PanamaCipherPolicy<B>::GetAlignment() const
{
#if (CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE || defined(CRYPTOPP_X64_MASM_AVAILABLE)) && !defined(CRYPTOPP_DISABLE_PANAMA_ASM)
	if (B::ToEnum() == LITTLE_ENDIAN_ORDER && HasSSE2())
		return 16;
	else
#endif
		return 1;
}

template <class B>
void PanamaCipherPolicy<B>::OperateKeystream(KeystreamOperation operation, byte *output, const byte *input, size_t iterationCount)
{
#if (CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE || defined(CRYPTOPP_X64_MASM_AVAILABLE)) && !defined(CRYPTOPP_DISABLE_PANAMA_ASM)
	// No need for alignment CRYPTOPP_ASSERT. Panama_SSE2_Pull is ASM, and its not bound by C alignment requirements.
	if (B::ToEnum() == LITTLE_ENDIAN_ORDER && HasSSE2())
		Panama_SSE2_Pull(iterationCount, this->m_state, (word32 *)(void *)output, (const word32 *)(void *)input);
	else
#endif
		this->Iterate(iterationCount, NULL, output, input, operation);
}

template class Panama<BigEndian>;
template class Panama<LittleEndian>;

template class Weak::PanamaHash<BigEndian>;
template class Weak::PanamaHash<LittleEndian>;

template class PanamaCipherPolicy<BigEndian>;
template class PanamaCipherPolicy<LittleEndian>;

NAMESPACE_END

#endif	// #ifndef CRYPTOPP_GENERATE_X64_MASM
