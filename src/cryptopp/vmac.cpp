// vmac.cpp - written and placed in the public domain by Wei Dai
// based on Ted Krovetz's public domain vmac.c and draft-krovetz-vmac-01.txt

#include "pch.h"
#include "config.h"

#include "vmac.h"
#include "cpu.h"
#include "argnames.h"
#include "secblock.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4731)
#endif

NAMESPACE_BEGIN(CryptoPP)

#if defined(_MSC_VER) && !CRYPTOPP_BOOL_SLOW_WORD64
#include <intrin.h>
#endif

#define VMAC_BOOL_WORD128 (defined(CRYPTOPP_WORD128_AVAILABLE) && !defined(CRYPTOPP_X64_ASM_AVAILABLE))
#ifdef __BORLANDC__
#define const	// Turbo C++ 2006 workaround
#endif
static const word64 p64   = W64LIT(0xfffffffffffffeff);  /* 2^64 - 257 prime  */
static const word64 m62   = W64LIT(0x3fffffffffffffff);  /* 62-bit mask       */
static const word64 m63   = W64LIT(0x7fffffffffffffff);  /* 63-bit mask       */
static const word64 m64   = W64LIT(0xffffffffffffffff);  /* 64-bit mask       */
static const word64 mpoly = W64LIT(0x1fffffff1fffffff);  /* Poly key mask     */
#ifdef __BORLANDC__
#undef const
#endif
#if VMAC_BOOL_WORD128
#ifdef __powerpc__
// workaround GCC Bug 31690: ICE with const __uint128_t and C++ front-end
#define m126				((word128(m62)<<64)|m64)
#else
static const word128 m126 = (word128(m62)<<64)|m64;		 /* 126-bit mask      */
#endif
#endif

void VMAC_Base::UncheckedSetKey(const byte *userKey, unsigned int keylength, const NameValuePairs &params)
{
	int digestLength = params.GetIntValueWithDefault(Name::DigestSize(), DefaultDigestSize());
	if (digestLength != 8 && digestLength != 16)
		throw InvalidArgument("VMAC: DigestSize must be 8 or 16");
	m_is128 = digestLength == 16;

	m_L1KeyLength = params.GetIntValueWithDefault(Name::L1KeyLength(), 128);
	if (m_L1KeyLength <= 0 || m_L1KeyLength % 128 != 0)
		throw InvalidArgument("VMAC: L1KeyLength must be a positive multiple of 128");

	AllocateBlocks();

	BlockCipher &cipher = AccessCipher();
	cipher.SetKey(userKey, keylength, params);
	const unsigned int blockSize = cipher.BlockSize();
	const unsigned int blockSizeInWords = blockSize / sizeof(word64);
	SecBlock<word64> out(blockSizeInWords);
	SecByteBlock in;
	in.CleanNew(blockSize);
	size_t i;

	/* Fill nh key */
	in[0] = 0x80; 
	cipher.AdvancedProcessBlocks(in, NULL, (byte *)m_nhKey(), m_nhKeySize()*sizeof(word64), cipher.BT_InBlockIsCounter);
	ConditionalByteReverse<word64>(BIG_ENDIAN_ORDER, m_nhKey(), m_nhKey(), m_nhKeySize()*sizeof(word64));

	/* Fill poly key */
	in[0] = 0xC0;
	in[15] = 0;
	for (i = 0; i <= (size_t)m_is128; i++)
	{
		cipher.ProcessBlock(in, out.BytePtr());
		m_polyState()[i*4+2] = GetWord<word64>(true, BIG_ENDIAN_ORDER, out.BytePtr()) & mpoly;
		m_polyState()[i*4+3]  = GetWord<word64>(true, BIG_ENDIAN_ORDER, out.BytePtr()+8) & mpoly;
		in[15]++;
	}

	/* Fill ip key */
	in[0] = 0xE0;
	in[15] = 0;
	word64 *l3Key = m_l3Key();
	for (i = 0; i <= (size_t)m_is128; i++)
		do
		{
			cipher.ProcessBlock(in, out.BytePtr());
			l3Key[i*2+0] = GetWord<word64>(true, BIG_ENDIAN_ORDER, out.BytePtr());
			l3Key[i*2+1] = GetWord<word64>(true, BIG_ENDIAN_ORDER, out.BytePtr()+8);
			in[15]++;
		} while ((l3Key[i*2+0] >= p64) || (l3Key[i*2+1] >= p64));

	m_padCached = false;
	size_t nonceLength;
	const byte *nonce = GetIVAndThrowIfInvalid(params, nonceLength);
	Resynchronize(nonce, (int)nonceLength);
}

void VMAC_Base::GetNextIV(RandomNumberGenerator &rng, byte *IV)
{
	SimpleKeyingInterface::GetNextIV(rng, IV);
	IV[0] &= 0x7f;
}

void VMAC_Base::Resynchronize(const byte *nonce, int len)
{
	size_t length = ThrowIfInvalidIVLength(len);
	size_t s = IVSize();
	byte *storedNonce = m_nonce();

	if (m_is128)
	{
		memset(storedNonce, 0, s-length);
		memcpy(storedNonce+s-length, nonce, length);
		AccessCipher().ProcessBlock(storedNonce, m_pad());
	}
	else
	{
		if (m_padCached && (storedNonce[s-1] | 1) == (nonce[length-1] | 1))
		{
			m_padCached = VerifyBufsEqual(storedNonce+s-length, nonce, length-1);
			for (size_t i=0; m_padCached && i<s-length; i++)
				m_padCached = (storedNonce[i] == 0);
		}
		if (!m_padCached)
		{
			memset(storedNonce, 0, s-length);
			memcpy(storedNonce+s-length, nonce, length-1);
			storedNonce[s-1] = nonce[length-1] & 0xfe;
			AccessCipher().ProcessBlock(storedNonce, m_pad());
			m_padCached = true;
		}
		storedNonce[s-1] = nonce[length-1];
	}
	m_isFirstBlock = true;
	Restart();
}

void VMAC_Base::HashEndianCorrectedBlock(const word64 *data)
{
	CRYPTOPP_UNUSED(data);
	assert(false);
	throw NotImplemented("VMAC: HashEndianCorrectedBlock is not implemented");
}

unsigned int VMAC_Base::OptimalDataAlignment() const
{
	return 
#if (CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE || defined(CRYPTOPP_X64_MASM_AVAILABLE)) && !defined(CRYPTOPP_DISABLE_VMAC_ASM)
		HasSSE2() ? 16 : 
#endif
		GetCipher().OptimalDataAlignment();
}

#if (CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE && (CRYPTOPP_BOOL_X86 || (CRYPTOPP_BOOL_X32 && !defined(CRYPTOPP_DISABLE_VMAC_ASM))))
#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4731)	// frame pointer register 'ebp' modified by inline assembly code
#endif
void
#ifdef __GNUC__
__attribute__ ((noinline))		// Intel Compiler 9.1 workaround
#endif
VMAC_Base::VHASH_Update_SSE2(const word64 *data, size_t blocksRemainingInWord64, int tagPart)
{
	const word64 *nhK = m_nhKey();
	word64 *polyS = m_polyState();
	word32 L1KeyLength = m_L1KeyLength;

	CRYPTOPP_UNUSED(data); CRYPTOPP_UNUSED(tagPart); CRYPTOPP_UNUSED(L1KeyLength);
	CRYPTOPP_UNUSED(blocksRemainingInWord64);

#ifdef __GNUC__
	word32 temp;
	__asm__ __volatile__
	(
	AS2(	mov		%%ebx, %0)
	AS2(	mov		%1, %%ebx)
	INTEL_NOPREFIX
#else
	#if _MSC_VER < 1300 || defined(__INTEL_COMPILER)
	char isFirstBlock = m_isFirstBlock;
	AS2(	mov		ebx, [L1KeyLength])
	AS2(	mov		dl, [isFirstBlock])
	#else
	AS2(	mov		ecx, this)
	AS2(	mov		ebx, [ecx+m_L1KeyLength])
	AS2(	mov		dl, [ecx+m_isFirstBlock])
	#endif
	AS2(	mov		eax, tagPart)
	AS2(	shl		eax, 4)
	AS2(	mov		edi, nhK)
	AS2(	add		edi, eax)
	AS2(	add		eax, eax)
	AS2(	add		eax, polyS)

	AS2(	mov		esi, data)
	AS2(	mov		ecx, blocksRemainingInWord64)
#endif

	AS2(	shr		ebx, 3)
#if CRYPTOPP_BOOL_X32
	AS_PUSH_IF86(	bp)
	AS2(	sub		esp, 24)
#else
	AS_PUSH_IF86(	bp)
	AS2(	sub		esp, 12)
#endif
	ASL(4)
	AS2(	mov		ebp, ebx)
	AS2(	cmp		ecx, ebx)
	AS2(	cmovl	ebp, ecx)
	AS2(	sub		ecx, ebp)
	AS2(	lea		ebp, [edi+8*ebp])	// end of nhK
	AS2(	movq	mm6, [esi])
	AS2(	paddq	mm6, [edi])
	AS2(	movq	mm5, [esi+8])
	AS2(	paddq	mm5, [edi+8])
	AS2(	add		esi, 16)
	AS2(	add		edi, 16)
	AS2(	movq	mm4, mm6)
	ASS(	pshufw	mm2, mm6, 1, 0, 3, 2)
	AS2(	pmuludq	mm6, mm5)
	ASS(	pshufw	mm3, mm5, 1, 0, 3, 2)
	AS2(	pmuludq	mm5, mm2)
	AS2(	pmuludq	mm2, mm3)
	AS2(	pmuludq	mm3, mm4)
	AS2(	pxor	mm7, mm7)
	AS2(	movd	[esp], mm6)
	AS2(	psrlq	mm6, 32)
#if CRYPTOPP_BOOL_X32
	AS2(	movd	[esp+8], mm5)
#else
	AS2(	movd	[esp+4], mm5)
#endif
	AS2(	psrlq	mm5, 32)
	AS2(	cmp		edi, ebp)
	ASJ(	je,		1, f)
	ASL(0)
	AS2(	movq	mm0, [esi])
	AS2(	paddq	mm0, [edi])
	AS2(	movq	mm1, [esi+8])
	AS2(	paddq	mm1, [edi+8])
	AS2(	add		esi, 16)
	AS2(	add		edi, 16)
	AS2(	movq	mm4, mm0)
	AS2(	paddq	mm5, mm2)
	ASS(	pshufw	mm2, mm0, 1, 0, 3, 2)
	AS2(	pmuludq	mm0, mm1)
#if CRYPTOPP_BOOL_X32
	AS2(	movd	[esp+16], mm3)
#else
	AS2(	movd	[esp+8], mm3)
#endif
	AS2(	psrlq	mm3, 32)
	AS2(	paddq	mm5, mm3)
	ASS(	pshufw	mm3, mm1, 1, 0, 3, 2)
	AS2(	pmuludq	mm1, mm2)
	AS2(	pmuludq	mm2, mm3)
	AS2(	pmuludq	mm3, mm4)
	AS2(	movd	mm4, [esp])
	AS2(	paddq	mm7, mm4)
#if CRYPTOPP_BOOL_X32
	AS2(	movd	mm4, [esp+8])
	AS2(	paddq	mm6, mm4)
	AS2(	movd	mm4, [esp+16])
#else
	AS2(	movd	mm4, [esp+4])
	AS2(	paddq	mm6, mm4)
	AS2(	movd	mm4, [esp+8])
#endif
	AS2(	paddq	mm6, mm4)
	AS2(	movd	[esp], mm0)
	AS2(	psrlq	mm0, 32)
	AS2(	paddq	mm6, mm0)
#if CRYPTOPP_BOOL_X32
	AS2(	movd	[esp+8], mm1)
#else
	AS2(	movd	[esp+4], mm1)
#endif
	AS2(	psrlq	mm1, 32)
	AS2(	paddq	mm5, mm1)
	AS2(	cmp		edi, ebp)
	ASJ(	jne,	0, b)
	ASL(1)
	AS2(	paddq	mm5, mm2)
#if CRYPTOPP_BOOL_X32
	AS2(	movd	[esp+16], mm3)
#else
	AS2(	movd	[esp+8], mm3)
#endif
	AS2(	psrlq	mm3, 32)
	AS2(	paddq	mm5, mm3)
	AS2(	movd	mm4, [esp])
	AS2(	paddq	mm7, mm4)
#if CRYPTOPP_BOOL_X32
	AS2(	movd	mm4, [esp+8])
	AS2(	paddq	mm6, mm4)
	AS2(	movd	mm4, [esp+16])
#else
	AS2(	movd	mm4, [esp+4])
	AS2(	paddq	mm6, mm4)
	AS2(	movd	mm4, [esp+8])
#endif
	AS2(	paddq	mm6, mm4)
	AS2(	lea		ebp, [8*ebx])
	AS2(	sub		edi, ebp)		// reset edi to start of nhK

	AS2(	movd	[esp], mm7)
	AS2(	psrlq	mm7, 32)
	AS2(	paddq	mm6, mm7)
#if CRYPTOPP_BOOL_X32
	AS2(	movd	[esp+8], mm6)
#else
	AS2(	movd	[esp+4], mm6)
#endif
	AS2(	psrlq	mm6, 32)
	AS2(	paddq	mm5, mm6)
	AS2(	psllq	mm5, 2)
	AS2(	psrlq	mm5, 2)

#define a0 [eax+2*4]
#define a1 [eax+3*4]
#define a2 [eax+0*4]
#define a3 [eax+1*4]
#define k0 [eax+2*8+2*4]
#define k1 [eax+2*8+3*4]
#define k2 [eax+2*8+0*4]
#define k3 [eax+2*8+1*4]
	AS2(	test	dl, dl)
	ASJ(	jz,		2, f)
	AS2(	movd	mm1, k0)
	AS2(	movd	mm0, [esp])
	AS2(	paddq	mm0, mm1)
	AS2(	movd	a0, mm0)
	AS2(	psrlq	mm0, 32)
	AS2(	movd	mm1, k1)
#if CRYPTOPP_BOOL_X32
	AS2(	movd	mm2, [esp+8])
#else
	AS2(	movd	mm2, [esp+4])
#endif
	AS2(	paddq	mm1, mm2)
	AS2(	paddq	mm0, mm1)
	AS2(	movd	a1, mm0)
	AS2(	psrlq	mm0, 32)
	AS2(	paddq	mm5, k2)
	AS2(	paddq	mm0, mm5)
	AS2(	movq	a2, mm0)
	AS2(	xor		edx, edx)
	ASJ(	jmp,	3, f)
	ASL(2)
	AS2(	movd	mm0, a3)
	AS2(	movq	mm4, mm0)
	AS2(	pmuludq	mm0, k3)		// a3*k3
	AS2(	movd	mm1, a0)
	AS2(	pmuludq	mm1, k2)		// a0*k2
	AS2(	movd	mm2, a1)
	AS2(	movd	mm6, k1)
	AS2(	pmuludq	mm2, mm6)		// a1*k1
	AS2(	movd	mm3, a2)
	AS2(	psllq	mm0, 1)
	AS2(	paddq	mm0, mm5)
	AS2(	movq	mm5, mm3)
	AS2(	movd	mm7, k0)
	AS2(	pmuludq	mm3, mm7)		// a2*k0
	AS2(	pmuludq	mm4, mm7)		// a3*k0
	AS2(	pmuludq	mm5, mm6)		// a2*k1
	AS2(	paddq	mm0, mm1)
	AS2(	movd	mm1, a1)
	AS2(	paddq	mm4, mm5)
	AS2(	movq	mm5, mm1)
	AS2(	pmuludq	mm1, k2)		// a1*k2
	AS2(	paddq	mm0, mm2)
	AS2(	movd	mm2, a0)
	AS2(	paddq	mm0, mm3)
	AS2(	movq	mm3, mm2)
	AS2(	pmuludq	mm2, k3)		// a0*k3
	AS2(	pmuludq	mm3, mm7)		// a0*k0
#if CRYPTOPP_BOOL_X32
	AS2(	movd	[esp+16], mm0)
#else
	AS2(	movd	[esp+8], mm0)
#endif
	AS2(	psrlq	mm0, 32)
	AS2(	pmuludq	mm7, mm5)		// a1*k0
	AS2(	pmuludq	mm5, k3)		// a1*k3
	AS2(	paddq	mm0, mm1)
	AS2(	movd	mm1, a2)
	AS2(	pmuludq	mm1, k2)		// a2*k2
	AS2(	paddq	mm0, mm2)
	AS2(	paddq	mm0, mm4)
	AS2(	movq	mm4, mm0)
	AS2(	movd	mm2, a3)
	AS2(	pmuludq	mm2, mm6)		// a3*k1
	AS2(	pmuludq	mm6, a0)		// a0*k1
	AS2(	psrlq	mm0, 31)
	AS2(	paddq	mm0, mm3)
	AS2(	movd	mm3, [esp])
	AS2(	paddq	mm0, mm3)
	AS2(	movd	mm3, a2)
	AS2(	pmuludq	mm3, k3)		// a2*k3
	AS2(	paddq	mm5, mm1)
	AS2(	movd	mm1, a3)
	AS2(	pmuludq	mm1, k2)		// a3*k2
	AS2(	paddq	mm5, mm2)
#if CRYPTOPP_BOOL_X32
	AS2(	movd	mm2, [esp+8])
#else
	AS2(	movd	mm2, [esp+4])
#endif
	AS2(	psllq	mm5, 1)
	AS2(	paddq	mm0, mm5)
	AS2(	psllq	mm4, 33)
	AS2(	movd	a0, mm0)
	AS2(	psrlq	mm0, 32)
	AS2(	paddq	mm6, mm7)
#if CRYPTOPP_BOOL_X32
	AS2(	movd	mm7, [esp+16])
#else
	AS2(	movd	mm7, [esp+8])
#endif
	AS2(	paddq	mm0, mm6)
	AS2(	paddq	mm0, mm2)
	AS2(	paddq	mm3, mm1)
	AS2(	psllq	mm3, 1)
	AS2(	paddq	mm0, mm3)
	AS2(	psrlq	mm4, 1)
	AS2(	movd	a1, mm0)
	AS2(	psrlq	mm0, 32)
	AS2(	por		mm4, mm7)
	AS2(	paddq	mm0, mm4)
	AS2(	movq	a2, mm0)
#undef a0
#undef a1
#undef a2
#undef a3
#undef k0
#undef k1
#undef k2
#undef k3

	ASL(3)
	AS2(	test	ecx, ecx)
	ASJ(	jnz,	4, b)
#if CRYPTOPP_BOOL_X32
	AS2(	add		esp, 24)
#else
	AS2(	add		esp, 12)
#endif
	AS_POP_IF86(	bp)
	AS1(	emms)
#ifdef __GNUC__
	ATT_PREFIX
	AS2(	mov	%0, %%ebx)
		: "=m" (temp)
		: "m" (L1KeyLength), "c" (blocksRemainingInWord64), "S" (data), "D" (nhK+tagPart*2), "d" (m_isFirstBlock), "a" (polyS+tagPart*4)
		: "memory", "cc"
	);
#endif
}
#endif

#if VMAC_BOOL_WORD128
	#define DeclareNH(a) word128 a=0
	#define MUL64(rh,rl,i1,i2) {word128 p = word128(i1)*(i2); rh = word64(p>>64); rl = word64(p);}
	#define AccumulateNH(a, b, c) a += word128(b)*(c)
	#define Multiply128(r, i1, i2) r = word128(word64(i1)) * word64(i2)
#else
	#if _MSC_VER >= 1400 && !defined(__INTEL_COMPILER)
		#define MUL32(a, b) __emulu(word32(a), word32(b))
	#else
		#define MUL32(a, b) ((word64)((word32)(a)) * (word32)(b))
	#endif
	#if defined(CRYPTOPP_X64_ASM_AVAILABLE)
		#define DeclareNH(a)			word64 a##0=0, a##1=0
		#define MUL64(rh,rl,i1,i2)		asm ("mulq %3" : "=a"(rl), "=d"(rh) : "a"(i1), "g"(i2) : "cc");
		#define AccumulateNH(a, b, c)	asm ("mulq %3; addq %%rax, %0; adcq %%rdx, %1" : "+r"(a##0), "+r"(a##1) : "a"(b), "g"(c) : "%rdx", "cc");
		#define ADD128(rh,rl,ih,il)     asm ("addq %3, %1; adcq %2, %0" : "+r"(rh),"+r"(rl) : "r"(ih),"r"(il) : "cc");
	#elif defined(_MSC_VER) && !CRYPTOPP_BOOL_SLOW_WORD64
		#define DeclareNH(a) word64 a##0=0, a##1=0
		#define MUL64(rh,rl,i1,i2)   (rl) = _umul128(i1,i2,&(rh));
		#define AccumulateNH(a, b, c)	{\
			word64 ph, pl;\
			pl = _umul128(b,c,&ph);\
			a##0 += pl;\
			a##1 += ph + (a##0 < pl);}
	#else
		#define VMAC_BOOL_32BIT 1
		#define DeclareNH(a) word64 a##0=0, a##1=0, a##2=0
		#define MUL64(rh,rl,i1,i2)                                               \
			{   word64 _i1 = (i1), _i2 = (i2);                                 \
				word64 m1= MUL32(_i1,_i2>>32);                                 \
				word64 m2= MUL32(_i1>>32,_i2);                                 \
				rh         = MUL32(_i1>>32,_i2>>32);                             \
				rl         = MUL32(_i1,_i2);                                     \
				ADD128(rh,rl,(m1 >> 32),(m1 << 32));                             \
				ADD128(rh,rl,(m2 >> 32),(m2 << 32));                             \
			}
		#define AccumulateNH(a, b, c)	{\
			word64 p = MUL32(b, c);\
			a##1 += word32((p)>>32);\
			a##0 += word32(p);\
			p = MUL32((b)>>32, c);\
			a##2 += word32((p)>>32);\
			a##1 += word32(p);\
			p = MUL32((b)>>32, (c)>>32);\
			a##2 += p;\
			p = MUL32(b, (c)>>32);\
			a##1 += word32(p);\
			a##2 += word32(p>>32);}
	#endif
#endif
#ifndef VMAC_BOOL_32BIT
	#define VMAC_BOOL_32BIT 0
#endif
#ifndef ADD128
	#define ADD128(rh,rl,ih,il)                                          \
		{   word64 _il = (il);                                         \
			(rl) += (_il);                                               \
			(rh) += (ih) + ((rl) < (_il));                               \
		}
#endif

#if !(defined(_MSC_VER) && _MSC_VER < 1300)
template <bool T_128BitTag>
#endif
void VMAC_Base::VHASH_Update_Template(const word64 *data, size_t blocksRemainingInWord64)
{
	#define INNER_LOOP_ITERATION(j)	{\
		word64 d0 = ConditionalByteReverse(LITTLE_ENDIAN_ORDER, data[i+2*j+0]);\
		word64 d1 = ConditionalByteReverse(LITTLE_ENDIAN_ORDER, data[i+2*j+1]);\
		AccumulateNH(nhA, d0+nhK[i+2*j+0], d1+nhK[i+2*j+1]);\
		if (T_128BitTag)\
			AccumulateNH(nhB, d0+nhK[i+2*j+2], d1+nhK[i+2*j+3]);\
		}

#if (defined(_MSC_VER) && _MSC_VER < 1300)
	bool T_128BitTag = m_is128;
#endif
	size_t L1KeyLengthInWord64 = m_L1KeyLength / 8;
	size_t innerLoopEnd = L1KeyLengthInWord64;
	const word64 *nhK = m_nhKey();
	word64 *polyS = m_polyState();
	bool isFirstBlock = true;
	size_t i;

	#if !VMAC_BOOL_32BIT
		#if VMAC_BOOL_WORD128
			word128 a1=0, a2=0;
		#else
			word64 ah1=0, al1=0, ah2=0, al2=0;
		#endif
		word64 kh1, kl1, kh2, kl2;
		kh1=(polyS+0*4+2)[0]; kl1=(polyS+0*4+2)[1];
		if (T_128BitTag)
		{
			kh2=(polyS+1*4+2)[0]; kl2=(polyS+1*4+2)[1];
		}
	#endif

	do
	{
		DeclareNH(nhA);
		DeclareNH(nhB);

		i = 0;
		if (blocksRemainingInWord64 < L1KeyLengthInWord64)
		{
			if (blocksRemainingInWord64 % 8)
			{
				innerLoopEnd = blocksRemainingInWord64 % 8;
				for (; i<innerLoopEnd; i+=2)
					INNER_LOOP_ITERATION(0);
			}
			innerLoopEnd = blocksRemainingInWord64;
		}
		for (; i<innerLoopEnd; i+=8)
		{
			INNER_LOOP_ITERATION(0);
			INNER_LOOP_ITERATION(1);
			INNER_LOOP_ITERATION(2);
			INNER_LOOP_ITERATION(3);
		}
		blocksRemainingInWord64 -= innerLoopEnd;
		data += innerLoopEnd;

		#if VMAC_BOOL_32BIT
			word32 nh0[2],  nh1[2];
			word64 nh2[2];

			nh0[0] = word32(nhA0);
			nhA1 += (nhA0 >> 32);
			nh1[0] = word32(nhA1);
			nh2[0] = (nhA2 + (nhA1 >> 32)) & m62;

			if (T_128BitTag)
			{
				nh0[1] = word32(nhB0);
				nhB1 += (nhB0 >> 32);
				nh1[1] = word32(nhB1);
				nh2[1] = (nhB2 + (nhB1 >> 32)) & m62;
			}

			#define a0 (((word32 *)(polyS+i*4))[2+NativeByteOrder::ToEnum()])
			#define a1 (*(((word32 *)(polyS+i*4))+3-NativeByteOrder::ToEnum()))		// workaround for GCC 3.2
			#define a2 (((word32 *)(polyS+i*4))[0+NativeByteOrder::ToEnum()])
			#define a3 (*(((word32 *)(polyS+i*4))+1-NativeByteOrder::ToEnum()))
			#define aHi ((polyS+i*4)[0])
			#define k0 (((word32 *)(polyS+i*4+2))[2+NativeByteOrder::ToEnum()])
			#define k1 (*(((word32 *)(polyS+i*4+2))+3-NativeByteOrder::ToEnum()))
			#define k2 (((word32 *)(polyS+i*4+2))[0+NativeByteOrder::ToEnum()])
			#define k3 (*(((word32 *)(polyS+i*4+2))+1-NativeByteOrder::ToEnum()))
			#define kHi ((polyS+i*4+2)[0])

			if (isFirstBlock)
			{
				isFirstBlock = false;
				if (m_isFirstBlock)
				{
					m_isFirstBlock = false;
					for (i=0; i<=(size_t)T_128BitTag; i++)
					{
						word64 t = (word64)nh0[i] + k0;
						a0 = (word32)t;
						t = (t >> 32) + nh1[i] + k1;
						a1 = (word32)t;
						aHi = (t >> 32) + nh2[i] + kHi;
					}
					continue;
				}
			}
			for (i=0; i<=(size_t)T_128BitTag; i++)
			{
				word64 p, t;
				word32 t2;

				p = MUL32(a3, 2*k3);
				p += nh2[i];
				p += MUL32(a0, k2);
				p += MUL32(a1, k1);
				p += MUL32(a2, k0);
				t2 = (word32)p;
				p >>= 32;
				p += MUL32(a0, k3);
				p += MUL32(a1, k2);
				p += MUL32(a2, k1);
				p += MUL32(a3, k0);
				t = (word64(word32(p) & 0x7fffffff) << 32) | t2;
				p >>= 31;
				p += nh0[i];
				p += MUL32(a0, k0);
				p += MUL32(a1, 2*k3);
				p += MUL32(a2, 2*k2);
				p += MUL32(a3, 2*k1);
				t2 = (word32)p;
				p >>= 32;
				p += nh1[i];
				p += MUL32(a0, k1);
				p += MUL32(a1, k0);
				p += MUL32(a2, 2*k3);
				p += MUL32(a3, 2*k2);
				a0 = t2;
				a1 = (word32)p;
				aHi = (p >> 32) + t;
			}

			#undef a0
			#undef a1
			#undef a2
			#undef a3
			#undef aHi
			#undef k0
			#undef k1
			#undef k2
			#undef k3		
			#undef kHi
		#else		// #if VMAC_BOOL_32BIT
			if (isFirstBlock)
			{
				isFirstBlock = false;
				if (m_isFirstBlock)
				{
					m_isFirstBlock = false;
					#if VMAC_BOOL_WORD128
						#define first_poly_step(a, kh, kl, m)	a = (m & m126) + ((word128(kh) << 64) | kl)

						first_poly_step(a1, kh1, kl1, nhA);
						if (T_128BitTag)
							first_poly_step(a2, kh2, kl2, nhB);
					#else
						#define first_poly_step(ah, al, kh, kl, mh, ml)		{\
							mh &= m62;\
							ADD128(mh, ml, kh, kl);	\
							ah = mh; al = ml;}

						first_poly_step(ah1, al1, kh1, kl1, nhA1, nhA0);
						if (T_128BitTag)
							first_poly_step(ah2, al2, kh2, kl2, nhB1, nhB0);
					#endif
					continue;
				}
				else
				{
					#if VMAC_BOOL_WORD128
						a1 = (word128((polyS+0*4)[0]) << 64) | (polyS+0*4)[1];
					#else
						ah1=(polyS+0*4)[0]; al1=(polyS+0*4)[1];
					#endif
					if (T_128BitTag)
					{
						#if VMAC_BOOL_WORD128
							a2 = (word128((polyS+1*4)[0]) << 64) | (polyS+1*4)[1];
						#else
							ah2=(polyS+1*4)[0]; al2=(polyS+1*4)[1];
						#endif
					}
				}
			}

			#if VMAC_BOOL_WORD128
				#define poly_step(a, kh, kl, m)	\
				{   word128 t1, t2, t3, t4;\
					Multiply128(t2, a>>64, kl);\
					Multiply128(t3, a, kh);\
					Multiply128(t1, a, kl);\
					Multiply128(t4, a>>64, 2*kh);\
					t2 += t3;\
					t4 += t1;\
					t2 += t4>>64;\
					a = (word128(word64(t2)&m63) << 64) | word64(t4);\
					t2 *= 2;\
					a += m & m126;\
					a += t2>>64;}

				poly_step(a1, kh1, kl1, nhA);
				if (T_128BitTag)
					poly_step(a2, kh2, kl2, nhB);
			#else
				#define poly_step(ah, al, kh, kl, mh, ml)					\
				{   word64 t1h, t1l, t2h, t2l, t3h, t3l, z=0;				\
					/* compute ab*cd, put bd into result registers */       \
					MUL64(t2h,t2l,ah,kl);                                   \
					MUL64(t3h,t3l,al,kh);                                   \
					MUL64(t1h,t1l,ah,2*kh);                                 \
					MUL64(ah,al,al,kl);                                     \
					/* add together ad + bc */                              \
					ADD128(t2h,t2l,t3h,t3l);                                \
					/* add 2 * ac to result */                              \
					ADD128(ah,al,t1h,t1l);                                  \
					/* now (ah,al), (t2l,2*t2h) need summing */             \
					/* first add the high registers, carrying into t2h */   \
					ADD128(t2h,ah,z,t2l);                                   \
					/* double t2h and add top bit of ah */                  \
					t2h += t2h + (ah >> 63);                                \
					ah &= m63;                                              \
					/* now add the low registers */                         \
					mh &= m62;												\
					ADD128(ah,al,mh,ml);                                    \
					ADD128(ah,al,z,t2h);                                    \
				}

				poly_step(ah1, al1, kh1, kl1, nhA1, nhA0);
				if (T_128BitTag)
					poly_step(ah2, al2, kh2, kl2, nhB1, nhB0);
			#endif
		#endif		// #if VMAC_BOOL_32BIT
	} while (blocksRemainingInWord64);

	#if VMAC_BOOL_WORD128
		(polyS+0*4)[0]=word64(a1>>64); (polyS+0*4)[1]=word64(a1);
		if (T_128BitTag)
		{
			(polyS+1*4)[0]=word64(a2>>64); (polyS+1*4)[1]=word64(a2);
		}
	#elif !VMAC_BOOL_32BIT
		(polyS+0*4)[0]=ah1; (polyS+0*4)[1]=al1;
		if (T_128BitTag)
		{
			(polyS+1*4)[0]=ah2; (polyS+1*4)[1]=al2;
		}
	#endif
}

inline void VMAC_Base::VHASH_Update(const word64 *data, size_t blocksRemainingInWord64)
{
#if (CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE && (CRYPTOPP_BOOL_X86 || (CRYPTOPP_BOOL_X32 && !defined(CRYPTOPP_DISABLE_VMAC_ASM))))
	if (HasSSE2())
	{
		VHASH_Update_SSE2(data, blocksRemainingInWord64, 0);
		if (m_is128)
			VHASH_Update_SSE2(data, blocksRemainingInWord64, 1);
		m_isFirstBlock = false;
	}
	else
#endif
	{
#if defined(_MSC_VER) && _MSC_VER < 1300
		VHASH_Update_Template(data, blocksRemainingInWord64);
#else
		if (m_is128)
			VHASH_Update_Template<true>(data, blocksRemainingInWord64);
		else
			VHASH_Update_Template<false>(data, blocksRemainingInWord64);
#endif
	}
}

size_t VMAC_Base::HashMultipleBlocks(const word64 *data, size_t length)
{
	size_t remaining = ModPowerOf2(length, m_L1KeyLength);
	VHASH_Update(data, (length-remaining)/8);
	return remaining;
}

static word64 L3Hash(const word64 *input, const word64 *l3Key, size_t len)
{
    word64 rh, rl, t, z=0;
	word64 p1 = input[0], p2 = input[1];
	word64 k1 = l3Key[0], k2 = l3Key[1];

    /* fully reduce (p1,p2)+(len,0) mod p127 */
    t = p1 >> 63;
    p1 &= m63;
    ADD128(p1, p2, len, t);
    /* At this point, (p1,p2) is at most 2^127+(len<<64) */
    t = (p1 > m63) + ((p1 == m63) & (p2 == m64));
    ADD128(p1, p2, z, t);
    p1 &= m63;

    /* compute (p1,p2)/(2^64-2^32) and (p1,p2)%(2^64-2^32) */
    t = p1 + (p2 >> 32);
    t += (t >> 32);
    t += (word32)t > 0xfffffffeU;
    p1 += (t >> 32);
    p2 += (p1 << 32);

    /* compute (p1+k1)%p64 and (p2+k2)%p64 */
    p1 += k1;
    p1 += (0 - (p1 < k1)) & 257;
    p2 += k2;
    p2 += (0 - (p2 < k2)) & 257;

    /* compute (p1+k1)*(p2+k2)%p64 */
    MUL64(rh, rl, p1, p2);
    t = rh >> 56;
    ADD128(t, rl, z, rh);
    rh <<= 8;
    ADD128(t, rl, z, rh);
    t += t << 8;
    rl += t;
    rl += (0 - (rl < t)) & 257;
    rl += (0 - (rl > p64-1)) & 257;
    return rl;
}

void VMAC_Base::TruncatedFinal(byte *mac, size_t size)
{
	size_t len = ModPowerOf2(GetBitCountLo()/8, m_L1KeyLength);

	if (len)
	{
		memset(m_data()+len, 0, (0-len)%16);
		VHASH_Update(DataBuf(), ((len+15)/16)*2);
		len *= 8;	// convert to bits
	}
	else if (m_isFirstBlock)
	{
		// special case for empty string
		m_polyState()[0] = m_polyState()[2];
		m_polyState()[1] = m_polyState()[3];
		if (m_is128)
		{
			m_polyState()[4] = m_polyState()[6];
			m_polyState()[5] = m_polyState()[7];
		}
	}

	if (m_is128)
	{
		word64 t[2];
		t[0] = L3Hash(m_polyState(), m_l3Key(), len) + GetWord<word64>(true, BIG_ENDIAN_ORDER, m_pad());
		t[1] = L3Hash(m_polyState()+4, m_l3Key()+2, len) + GetWord<word64>(true, BIG_ENDIAN_ORDER, m_pad()+8);
		if (size == 16)
		{
			PutWord(false, BIG_ENDIAN_ORDER, mac, t[0]);
			PutWord(false, BIG_ENDIAN_ORDER, mac+8, t[1]);
		}
		else
		{
			t[0] = ConditionalByteReverse(BIG_ENDIAN_ORDER, t[0]);
			t[1] = ConditionalByteReverse(BIG_ENDIAN_ORDER, t[1]);
			memcpy(mac, t, size);
		}
	}
	else
	{
		word64 t = L3Hash(m_polyState(), m_l3Key(), len);
		t += GetWord<word64>(true, BIG_ENDIAN_ORDER, m_pad() + (m_nonce()[IVSize()-1]&1) * 8);
		if (size == 8)
			PutWord(false, BIG_ENDIAN_ORDER, mac, t);
		else
		{
			t = ConditionalByteReverse(BIG_ENDIAN_ORDER, t);
			memcpy(mac, &t, size);
		}
	}
}

NAMESPACE_END
