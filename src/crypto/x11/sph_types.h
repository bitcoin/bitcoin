/* $Id: sph_types.h 260 2011-07-21 01:02:38Z tp $ */
/**
 * Basic type definitions.
 *
 * This header file defines the generic integer types that will be used
 * for the implementation of hash functions; it also contains helper
 * functions which encode and decode multi-byte integer values, using
 * either little-endian or big-endian conventions.
 *
 * This file contains a compile-time test on the size of a byte
 * (the <code>unsigned char</code> C type). If bytes are not octets,
 * i.e. if they do not have a size of exactly 8 bits, then compilation
 * is aborted. Architectures where bytes are not octets are relatively
 * rare, even in the embedded devices market. We forbid non-octet bytes
 * because there is no clear convention on how octet streams are encoded
 * on such systems.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2007-2010  Projet RNRT SAPHIR
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @file     sph_types.h
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#ifndef SPH_TYPES_H__
#define SPH_TYPES_H__

#include <limits.h>

/*
 * All our I/O functions are defined over octet streams. We do not know
 * how to handle input data if bytes are not octets.
 */
#if CHAR_BIT != 8
#error This code requires 8-bit bytes
#endif

/*
 * We want to define the types "sph_u32" and "sph_u64" which hold
 * unsigned values of at least, respectively, 32 and 64 bits. These
 * tests should select appropriate types for most platforms. The
 * macro "SPH_64" is defined if the 64-bit is supported.
 */

#undef SPH_64
#undef SPH_64_TRUE

#if defined __STDC__ && __STDC_VERSION__ >= 199901L

/*
 * On C99 implementations, we can use <stdint.h> to get an exact 64-bit
 * type, if any, or otherwise use a wider type (which must exist, for
 * C99 conformance).
 */

#include <stdint.h>

#ifdef UINT32_MAX
typedef uint32_t sph_u32;
typedef int32_t sph_s32;
#else
typedef uint_fast32_t sph_u32;
typedef int_fast32_t sph_s32;
#endif
#if !SPH_NO_64
#ifdef UINT64_MAX
typedef uint64_t sph_u64;
typedef int64_t sph_s64;
#else
typedef uint_fast64_t sph_u64;
typedef int_fast64_t sph_s64;
#endif
#endif

#define SPH_C32(x)    ((sph_u32)(x))
#if !SPH_NO_64
#define SPH_C64(x)    ((sph_u64)(x))
#define SPH_64  1
#endif

#else

/*
 * On non-C99 systems, we use "unsigned int" if it is wide enough,
 * "unsigned long" otherwise. This supports all "reasonable" architectures.
 * We have to be cautious: pre-C99 preprocessors handle constants
 * differently in '#if' expressions. Hence the shifts to test UINT_MAX.
 */

#if ((UINT_MAX >> 11) >> 11) >= 0x3FF

typedef unsigned int sph_u32;
typedef int sph_s32;

#define SPH_C32(x)    ((sph_u32)(x ## U))

#else

typedef unsigned long sph_u32;
typedef long sph_s32;

#define SPH_C32(x)    ((sph_u32)(x ## UL))

#endif

#if !SPH_NO_64

/*
 * We want a 64-bit type. We use "unsigned long" if it is wide enough (as
 * is common on 64-bit architectures such as AMD64, Alpha or Sparcv9),
 * "unsigned long long" otherwise, if available. We use ULLONG_MAX to
 * test whether "unsigned long long" is available; we also know that
 * gcc features this type, even if the libc header do not know it.
 */

#if ((ULONG_MAX >> 31) >> 31) >= 3

typedef unsigned long sph_u64;
typedef long sph_s64;

#define SPH_C64(x)    ((sph_u64)(x ## UL))

#define SPH_64  1

#elif ((ULLONG_MAX >> 31) >> 31) >= 3 || defined __GNUC__

typedef unsigned long long sph_u64;
typedef long long sph_s64;

#define SPH_C64(x)    ((sph_u64)(x ## ULL))

#define SPH_64  1

#else

/*
 * No 64-bit type...
 */

#endif

#endif

#endif

/*
 * If the "unsigned long" type has length 64 bits or more, then this is
 * a "true" 64-bit architectures. This is also true with Visual C on
 * amd64, even though the "long" type is limited to 32 bits.
 */
#if SPH_64 && (((ULONG_MAX >> 31) >> 31) >= 3 || defined _M_X64)
#define SPH_64_TRUE   1
#endif

/*
 * Implementation note: some processors have specific opcodes to perform
 * a rotation. Recent versions of gcc recognize the expression above and
 * use the relevant opcodes, when appropriate.
 */

#define SPH_T32(x)    ((x) & SPH_C32(0xFFFFFFFF))
#define SPH_ROTL32(x, n)   SPH_T32(((x) << (n)) | ((x) >> (32 - (n))))
#define SPH_ROTR32(x, n)   SPH_ROTL32(x, (32 - (n)))

#if SPH_64

#define SPH_T64(x)    ((x) & SPH_C64(0xFFFFFFFFFFFFFFFF))
#define SPH_ROTL64(x, n)   SPH_T64(((x) << (n)) | ((x) >> (64 - (n))))
#define SPH_ROTR64(x, n)   SPH_ROTL64(x, (64 - (n)))

#endif

/*
 * Define SPH_INLINE to be an "inline" qualifier, if available. We define
 * some small macro-like functions which benefit greatly from being inlined.
 */
#if (defined __STDC__ && __STDC_VERSION__ >= 199901L) || defined __GNUC__
#define SPH_INLINE inline
#elif defined _MSC_VER
#define SPH_INLINE __inline
#else
#define SPH_INLINE
#endif

/*
 * We define some macros which qualify the architecture. These macros
 * may be explicit set externally (e.g. as compiler parameters). The
 * code below sets those macros if they are not already defined.
 *
 * Most macros are boolean, thus evaluate to either zero or non-zero.
 * The SPH_UPTR macro is special, in that it evaluates to a C type,
 * or is not defined.
 *
 * SPH_UPTR             if defined: unsigned type to cast pointers into
 *
 * SPH_UNALIGNED        non-zero if unaligned accesses are efficient
 * SPH_LITTLE_ENDIAN    non-zero if architecture is known to be little-endian
 * SPH_BIG_ENDIAN       non-zero if architecture is known to be big-endian
 * SPH_LITTLE_FAST      non-zero if little-endian decoding is fast
 * SPH_BIG_FAST         non-zero if big-endian decoding is fast
 *
 * If SPH_UPTR is defined, then encoding and decoding of 32-bit and 64-bit
 * values will try to be "smart". Either SPH_LITTLE_ENDIAN or SPH_BIG_ENDIAN
 * _must_ be non-zero in those situations. The 32-bit and 64-bit types
 * _must_ also have an exact width.
 *
 * SPH_SPARCV9_GCC_32   UltraSPARC-compatible with gcc, 32-bit mode
 * SPH_SPARCV9_GCC_64   UltraSPARC-compatible with gcc, 64-bit mode
 * SPH_SPARCV9_GCC      UltraSPARC-compatible with gcc
 * SPH_I386_GCC         x86-compatible (32-bit) with gcc
 * SPH_I386_MSVC        x86-compatible (32-bit) with Microsoft Visual C
 * SPH_AMD64_GCC        x86-compatible (64-bit) with gcc
 * SPH_AMD64_MSVC       x86-compatible (64-bit) with Microsoft Visual C
 * SPH_PPC32_GCC        PowerPC, 32-bit, with gcc
 * SPH_PPC64_GCC        PowerPC, 64-bit, with gcc
 *
 * TODO: enhance automatic detection, for more architectures and compilers.
 * Endianness is the most important. SPH_UNALIGNED and SPH_UPTR help with
 * some very fast functions (e.g. MD4) when using unaligned input data.
 * The CPU-specific-with-GCC macros are useful only for inline assembly,
 * normally restrained to this header file.
 */

/*
 * 32-bit x86, aka "i386 compatible".
 */
#if defined __i386__ || defined _M_IX86

#define SPH_DETECT_UNALIGNED         1
#define SPH_DETECT_LITTLE_ENDIAN     1
#define SPH_DETECT_UPTR              sph_u32
#ifdef __GNUC__
#define SPH_DETECT_I386_GCC          1
#endif
#ifdef _MSC_VER
#define SPH_DETECT_I386_MSVC         1
#endif

/*
 * 64-bit x86, hereafter known as "amd64".
 */
#elif defined __x86_64 || defined _M_X64

#define SPH_DETECT_UNALIGNED         1
#define SPH_DETECT_LITTLE_ENDIAN     1
#define SPH_DETECT_UPTR              sph_u64
#ifdef __GNUC__
#define SPH_DETECT_AMD64_GCC         1
#endif
#ifdef _MSC_VER
#define SPH_DETECT_AMD64_MSVC        1
#endif

/*
 * 64-bit Sparc architecture (implies v9).
 */
#elif ((defined __sparc__ || defined __sparc) && defined __arch64__) \
	|| defined __sparcv9

#define SPH_DETECT_BIG_ENDIAN        1
#define SPH_DETECT_UPTR              sph_u64
#ifdef __GNUC__
#define SPH_DETECT_SPARCV9_GCC_64    1
#define SPH_DETECT_LITTLE_FAST       1
#endif

/*
 * 32-bit Sparc.
 */
#elif (defined __sparc__ || defined __sparc) \
	&& !(defined __sparcv9 || defined __arch64__)

#define SPH_DETECT_BIG_ENDIAN        1
#define SPH_DETECT_UPTR              sph_u32
#if defined __GNUC__ && defined __sparc_v9__
#define SPH_DETECT_SPARCV9_GCC_32    1
#define SPH_DETECT_LITTLE_FAST       1
#endif

/*
 * ARM, little-endian.
 */
#elif defined __arm__ && __ARMEL__

#define SPH_DETECT_LITTLE_ENDIAN     1

/*
 * MIPS, little-endian.
 */
#elif MIPSEL || _MIPSEL || __MIPSEL || __MIPSEL__

#define SPH_DETECT_LITTLE_ENDIAN     1

/*
 * MIPS, big-endian.
 */
#elif MIPSEB || _MIPSEB || __MIPSEB || __MIPSEB__

#define SPH_DETECT_BIG_ENDIAN        1

/*
 * PowerPC.
 */
#elif defined __powerpc__ || defined __POWERPC__ || defined __ppc__ \
	|| defined _ARCH_PPC

/*
 * Note: we do not declare cross-endian access to be "fast": even if
 * using inline assembly, implementation should still assume that
 * keeping the decoded word in a temporary is faster than decoding
 * it again.
 */
#if defined __GNUC__
#if SPH_64_TRUE
#define SPH_DETECT_PPC64_GCC         1
#else
#define SPH_DETECT_PPC32_GCC         1
#endif
#endif

#if defined __BIG_ENDIAN__ || defined _BIG_ENDIAN
#define SPH_DETECT_BIG_ENDIAN        1
#elif defined __LITTLE_ENDIAN__ || defined _LITTLE_ENDIAN
#define SPH_DETECT_LITTLE_ENDIAN     1
#endif

/*
 * Itanium, 64-bit.
 */
#elif defined __ia64 || defined __ia64__ \
	|| defined __itanium__ || defined _M_IA64

#if defined __BIG_ENDIAN__ || defined _BIG_ENDIAN
#define SPH_DETECT_BIG_ENDIAN        1
#else
#define SPH_DETECT_LITTLE_ENDIAN     1
#endif
#if defined __LP64__ || defined _LP64
#define SPH_DETECT_UPTR              sph_u64
#else
#define SPH_DETECT_UPTR              sph_u32
#endif

#endif

#if defined SPH_DETECT_SPARCV9_GCC_32 || defined SPH_DETECT_SPARCV9_GCC_64
#define SPH_DETECT_SPARCV9_GCC       1
#endif

#if defined SPH_DETECT_UNALIGNED && !defined SPH_UNALIGNED
#define SPH_UNALIGNED         SPH_DETECT_UNALIGNED
#endif
#if defined SPH_DETECT_UPTR && !defined SPH_UPTR
#define SPH_UPTR              SPH_DETECT_UPTR
#endif
#if defined SPH_DETECT_LITTLE_ENDIAN && !defined SPH_LITTLE_ENDIAN
#define SPH_LITTLE_ENDIAN     SPH_DETECT_LITTLE_ENDIAN
#endif
#if defined SPH_DETECT_BIG_ENDIAN && !defined SPH_BIG_ENDIAN
#define SPH_BIG_ENDIAN        SPH_DETECT_BIG_ENDIAN
#endif
#if defined SPH_DETECT_LITTLE_FAST && !defined SPH_LITTLE_FAST
#define SPH_LITTLE_FAST       SPH_DETECT_LITTLE_FAST
#endif
#if defined SPH_DETECT_BIG_FAST && !defined SPH_BIG_FAST
#define SPH_BIG_FAST    SPH_DETECT_BIG_FAST
#endif
#if defined SPH_DETECT_SPARCV9_GCC_32 && !defined SPH_SPARCV9_GCC_32
#define SPH_SPARCV9_GCC_32    SPH_DETECT_SPARCV9_GCC_32
#endif
#if defined SPH_DETECT_SPARCV9_GCC_64 && !defined SPH_SPARCV9_GCC_64
#define SPH_SPARCV9_GCC_64    SPH_DETECT_SPARCV9_GCC_64
#endif
#if defined SPH_DETECT_SPARCV9_GCC && !defined SPH_SPARCV9_GCC
#define SPH_SPARCV9_GCC       SPH_DETECT_SPARCV9_GCC
#endif
#if defined SPH_DETECT_I386_GCC && !defined SPH_I386_GCC
#define SPH_I386_GCC          SPH_DETECT_I386_GCC
#endif
#if defined SPH_DETECT_I386_MSVC && !defined SPH_I386_MSVC
#define SPH_I386_MSVC         SPH_DETECT_I386_MSVC
#endif
#if defined SPH_DETECT_AMD64_GCC && !defined SPH_AMD64_GCC
#define SPH_AMD64_GCC         SPH_DETECT_AMD64_GCC
#endif
#if defined SPH_DETECT_AMD64_MSVC && !defined SPH_AMD64_MSVC
#define SPH_AMD64_MSVC        SPH_DETECT_AMD64_MSVC
#endif
#if defined SPH_DETECT_PPC32_GCC && !defined SPH_PPC32_GCC
#define SPH_PPC32_GCC         SPH_DETECT_PPC32_GCC
#endif
#if defined SPH_DETECT_PPC64_GCC && !defined SPH_PPC64_GCC
#define SPH_PPC64_GCC         SPH_DETECT_PPC64_GCC
#endif

#if SPH_LITTLE_ENDIAN && !defined SPH_LITTLE_FAST
#define SPH_LITTLE_FAST              1
#endif
#if SPH_BIG_ENDIAN && !defined SPH_BIG_FAST
#define SPH_BIG_FAST                 1
#endif

#if defined SPH_UPTR && !(SPH_LITTLE_ENDIAN || SPH_BIG_ENDIAN)
#error SPH_UPTR defined, but endianness is not known.
#endif

#if SPH_I386_GCC && !SPH_NO_ASM

/*
 * On x86 32-bit, with gcc, we use the bswapl opcode to byte-swap 32-bit
 * values.
 */

static SPH_INLINE sph_u32
sph_bswap32(sph_u32 x)
{
	__asm__ __volatile__ ("bswapl %0" : "=r" (x) : "0" (x));
	return x;
}

#if SPH_64

static SPH_INLINE sph_u64
sph_bswap64(sph_u64 x)
{
	return ((sph_u64)sph_bswap32((sph_u32)x) << 32)
		| (sph_u64)sph_bswap32((sph_u32)(x >> 32));
}

#endif

#elif SPH_AMD64_GCC && !SPH_NO_ASM

/*
 * On x86 64-bit, with gcc, we use the bswapl opcode to byte-swap 32-bit
 * and 64-bit values.
 */

static SPH_INLINE sph_u32
sph_bswap32(sph_u32 x)
{
	__asm__ __volatile__ ("bswapl %0" : "=r" (x) : "0" (x));
	return x;
}

#if SPH_64

static SPH_INLINE sph_u64
sph_bswap64(sph_u64 x)
{
	__asm__ __volatile__ ("bswapq %0" : "=r" (x) : "0" (x));
	return x;
}

#endif

#else

static SPH_INLINE sph_u32
sph_bswap32(sph_u32 x)
{
	x = SPH_T32((x << 16) | (x >> 16));
	x = ((x & SPH_C32(0xFF00FF00)) >> 8)
		| ((x & SPH_C32(0x00FF00FF)) << 8);
	return x;
}

#if SPH_64

/**
 * Byte-swap a 64-bit value.
 *
 * @param x   the input value
 * @return  the byte-swapped value
 */
static SPH_INLINE sph_u64
sph_bswap64(sph_u64 x)
{
	x = SPH_T64((x << 32) | (x >> 32));
	x = ((x & SPH_C64(0xFFFF0000FFFF0000)) >> 16)
		| ((x & SPH_C64(0x0000FFFF0000FFFF)) << 16);
	x = ((x & SPH_C64(0xFF00FF00FF00FF00)) >> 8)
		| ((x & SPH_C64(0x00FF00FF00FF00FF)) << 8);
	return x;
}

#endif

#endif

#if SPH_SPARCV9_GCC && !SPH_NO_ASM

/*
 * On UltraSPARC systems, native ordering is big-endian, but it is
 * possible to perform little-endian read accesses by specifying the
 * address space 0x88 (ASI_PRIMARY_LITTLE). Basically, either we use
 * the opcode "lda [%reg]0x88,%dst", where %reg is the register which
 * contains the source address and %dst is the destination register,
 * or we use "lda [%reg+imm]%asi,%dst", which uses the %asi register
 * to get the address space name. The latter format is better since it
 * combines an addition and the actual access in a single opcode; but
 * it requires the setting (and subsequent resetting) of %asi, which is
 * slow. Some operations (i.e. MD5 compression function) combine many
 * successive little-endian read accesses, which may share the same
 * %asi setting. The macros below contain the appropriate inline
 * assembly.
 */

#define SPH_SPARCV9_SET_ASI   \
	sph_u32 sph_sparcv9_asi; \
	__asm__ __volatile__ ( \
		"rd %%asi,%0\n\twr %%g0,0x88,%%asi" : "=r" (sph_sparcv9_asi));

#define SPH_SPARCV9_RESET_ASI  \
	__asm__ __volatile__ ("wr %%g0,%0,%%asi" : : "r" (sph_sparcv9_asi));

#define SPH_SPARCV9_DEC32LE(base, idx)   ({ \
		sph_u32 sph_sparcv9_tmp; \
		__asm__ __volatile__ ("lda [%1+" #idx "*4]%%asi,%0" \
			: "=r" (sph_sparcv9_tmp) : "r" (base)); \
		sph_sparcv9_tmp; \
	})

#endif

static SPH_INLINE void
sph_enc16be(void *dst, unsigned val)
{
	((unsigned char *)dst)[0] = (val >> 8);
	((unsigned char *)dst)[1] = val;
}

static SPH_INLINE unsigned
sph_dec16be(const void *src)
{
	return ((unsigned)(((const unsigned char *)src)[0]) << 8)
		| (unsigned)(((const unsigned char *)src)[1]);
}

static SPH_INLINE void
sph_enc16le(void *dst, unsigned val)
{
	((unsigned char *)dst)[0] = val;
	((unsigned char *)dst)[1] = val >> 8;
}

static SPH_INLINE unsigned
sph_dec16le(const void *src)
{
	return (unsigned)(((const unsigned char *)src)[0])
		| ((unsigned)(((const unsigned char *)src)[1]) << 8);
}

/**
 * Encode a 32-bit value into the provided buffer (big endian convention).
 *
 * @param dst   the destination buffer
 * @param val   the 32-bit value to encode
 */
static SPH_INLINE void
sph_enc32be(void *dst, sph_u32 val)
{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_LITTLE_ENDIAN
	val = sph_bswap32(val);
#endif
	*(sph_u32 *)dst = val;
#else
	if (((SPH_UPTR)dst & 3) == 0) {
#if SPH_LITTLE_ENDIAN
		val = sph_bswap32(val);
#endif
		*(sph_u32 *)dst = val;
	} else {
		((unsigned char *)dst)[0] = (val >> 24);
		((unsigned char *)dst)[1] = (val >> 16);
		((unsigned char *)dst)[2] = (val >> 8);
		((unsigned char *)dst)[3] = val;
	}
#endif
#else
	((unsigned char *)dst)[0] = (val >> 24);
	((unsigned char *)dst)[1] = (val >> 16);
	((unsigned char *)dst)[2] = (val >> 8);
	((unsigned char *)dst)[3] = val;
#endif
}

/**
 * Encode a 32-bit value into the provided buffer (big endian convention).
 * The destination buffer must be properly aligned.
 *
 * @param dst   the destination buffer (32-bit aligned)
 * @param val   the value to encode
 */
static SPH_INLINE void
sph_enc32be_aligned(void *dst, sph_u32 val)
{
#if SPH_LITTLE_ENDIAN
	*(sph_u32 *)dst = sph_bswap32(val);
#elif SPH_BIG_ENDIAN
	*(sph_u32 *)dst = val;
#else
	((unsigned char *)dst)[0] = (val >> 24);
	((unsigned char *)dst)[1] = (val >> 16);
	((unsigned char *)dst)[2] = (val >> 8);
	((unsigned char *)dst)[3] = val;
#endif
}

/**
 * Decode a 32-bit value from the provided buffer (big endian convention).
 *
 * @param src   the source buffer
 * @return  the decoded value
 */
static SPH_INLINE sph_u32
sph_dec32be(const void *src)
{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_LITTLE_ENDIAN
	return sph_bswap32(*(const sph_u32 *)src);
#else
	return *(const sph_u32 *)src;
#endif
#else
	if (((SPH_UPTR)src & 3) == 0) {
#if SPH_LITTLE_ENDIAN
		return sph_bswap32(*(const sph_u32 *)src);
#else
		return *(const sph_u32 *)src;
#endif
	} else {
		return ((sph_u32)(((const unsigned char *)src)[0]) << 24)
			| ((sph_u32)(((const unsigned char *)src)[1]) << 16)
			| ((sph_u32)(((const unsigned char *)src)[2]) << 8)
			| (sph_u32)(((const unsigned char *)src)[3]);
	}
#endif
#else
	return ((sph_u32)(((const unsigned char *)src)[0]) << 24)
		| ((sph_u32)(((const unsigned char *)src)[1]) << 16)
		| ((sph_u32)(((const unsigned char *)src)[2]) << 8)
		| (sph_u32)(((const unsigned char *)src)[3]);
#endif
}

/**
 * Decode a 32-bit value from the provided buffer (big endian convention).
 * The source buffer must be properly aligned.
 *
 * @param src   the source buffer (32-bit aligned)
 * @return  the decoded value
 */
static SPH_INLINE sph_u32
sph_dec32be_aligned(const void *src)
{
#if SPH_LITTLE_ENDIAN
	return sph_bswap32(*(const sph_u32 *)src);
#elif SPH_BIG_ENDIAN
	return *(const sph_u32 *)src;
#else
	return ((sph_u32)(((const unsigned char *)src)[0]) << 24)
		| ((sph_u32)(((const unsigned char *)src)[1]) << 16)
		| ((sph_u32)(((const unsigned char *)src)[2]) << 8)
		| (sph_u32)(((const unsigned char *)src)[3]);
#endif
}

/**
 * Encode a 32-bit value into the provided buffer (little endian convention).
 *
 * @param dst   the destination buffer
 * @param val   the 32-bit value to encode
 */
static SPH_INLINE void
sph_enc32le(void *dst, sph_u32 val)
{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_BIG_ENDIAN
	val = sph_bswap32(val);
#endif
	*(sph_u32 *)dst = val;
#else
	if (((SPH_UPTR)dst & 3) == 0) {
#if SPH_BIG_ENDIAN
		val = sph_bswap32(val);
#endif
		*(sph_u32 *)dst = val;
	} else {
		((unsigned char *)dst)[0] = val;
		((unsigned char *)dst)[1] = (val >> 8);
		((unsigned char *)dst)[2] = (val >> 16);
		((unsigned char *)dst)[3] = (val >> 24);
	}
#endif
#else
	((unsigned char *)dst)[0] = val;
	((unsigned char *)dst)[1] = (val >> 8);
	((unsigned char *)dst)[2] = (val >> 16);
	((unsigned char *)dst)[3] = (val >> 24);
#endif
}

/**
 * Encode a 32-bit value into the provided buffer (little endian convention).
 * The destination buffer must be properly aligned.
 *
 * @param dst   the destination buffer (32-bit aligned)
 * @param val   the value to encode
 */
static SPH_INLINE void
sph_enc32le_aligned(void *dst, sph_u32 val)
{
#if SPH_LITTLE_ENDIAN
	*(sph_u32 *)dst = val;
#elif SPH_BIG_ENDIAN
	*(sph_u32 *)dst = sph_bswap32(val);
#else
	((unsigned char *)dst)[0] = val;
	((unsigned char *)dst)[1] = (val >> 8);
	((unsigned char *)dst)[2] = (val >> 16);
	((unsigned char *)dst)[3] = (val >> 24);
#endif
}

/**
 * Decode a 32-bit value from the provided buffer (little endian convention).
 *
 * @param src   the source buffer
 * @return  the decoded value
 */
static SPH_INLINE sph_u32
sph_dec32le(const void *src)
{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_BIG_ENDIAN
	return sph_bswap32(*(const sph_u32 *)src);
#else
	return *(const sph_u32 *)src;
#endif
#else
	if (((SPH_UPTR)src & 3) == 0) {
#if SPH_BIG_ENDIAN
#if SPH_SPARCV9_GCC && !SPH_NO_ASM
		sph_u32 tmp;

		/*
		 * "__volatile__" is needed here because without it,
		 * gcc-3.4.3 miscompiles the code and performs the
		 * access before the test on the address, thus triggering
		 * a bus error...
		 */
		__asm__ __volatile__ (
			"lda [%1]0x88,%0" : "=r" (tmp) : "r" (src));
		return tmp;
#else
		return sph_bswap32(*(const sph_u32 *)src);
#endif
#else
		return *(const sph_u32 *)src;
#endif
	} else {
		return (sph_u32)(((const unsigned char *)src)[0])
			| ((sph_u32)(((const unsigned char *)src)[1]) << 8)
			| ((sph_u32)(((const unsigned char *)src)[2]) << 16)
			| ((sph_u32)(((const unsigned char *)src)[3]) << 24);
	}
#endif
#else
	return (sph_u32)(((const unsigned char *)src)[0])
		| ((sph_u32)(((const unsigned char *)src)[1]) << 8)
		| ((sph_u32)(((const unsigned char *)src)[2]) << 16)
		| ((sph_u32)(((const unsigned char *)src)[3]) << 24);
#endif
}

/**
 * Decode a 32-bit value from the provided buffer (little endian convention).
 * The source buffer must be properly aligned.
 *
 * @param src   the source buffer (32-bit aligned)
 * @return  the decoded value
 */
static SPH_INLINE sph_u32
sph_dec32le_aligned(const void *src)
{
#if SPH_LITTLE_ENDIAN
	return *(const sph_u32 *)src;
#elif SPH_BIG_ENDIAN
#if SPH_SPARCV9_GCC && !SPH_NO_ASM
	sph_u32 tmp;

	__asm__ __volatile__ ("lda [%1]0x88,%0" : "=r" (tmp) : "r" (src));
	return tmp;
#else
	return sph_bswap32(*(const sph_u32 *)src);
#endif
#else
	return (sph_u32)(((const unsigned char *)src)[0])
		| ((sph_u32)(((const unsigned char *)src)[1]) << 8)
		| ((sph_u32)(((const unsigned char *)src)[2]) << 16)
		| ((sph_u32)(((const unsigned char *)src)[3]) << 24);
#endif
}

#if SPH_64

/**
 * Encode a 64-bit value into the provided buffer (big endian convention).
 *
 * @param dst   the destination buffer
 * @param val   the 64-bit value to encode
 */
static SPH_INLINE void
sph_enc64be(void *dst, sph_u64 val)
{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_LITTLE_ENDIAN
	val = sph_bswap64(val);
#endif
	*(sph_u64 *)dst = val;
#else
	if (((SPH_UPTR)dst & 7) == 0) {
#if SPH_LITTLE_ENDIAN
		val = sph_bswap64(val);
#endif
		*(sph_u64 *)dst = val;
	} else {
		((unsigned char *)dst)[0] = (val >> 56);
		((unsigned char *)dst)[1] = (val >> 48);
		((unsigned char *)dst)[2] = (val >> 40);
		((unsigned char *)dst)[3] = (val >> 32);
		((unsigned char *)dst)[4] = (val >> 24);
		((unsigned char *)dst)[5] = (val >> 16);
		((unsigned char *)dst)[6] = (val >> 8);
		((unsigned char *)dst)[7] = val;
	}
#endif
#else
	((unsigned char *)dst)[0] = (val >> 56);
	((unsigned char *)dst)[1] = (val >> 48);
	((unsigned char *)dst)[2] = (val >> 40);
	((unsigned char *)dst)[3] = (val >> 32);
	((unsigned char *)dst)[4] = (val >> 24);
	((unsigned char *)dst)[5] = (val >> 16);
	((unsigned char *)dst)[6] = (val >> 8);
	((unsigned char *)dst)[7] = val;
#endif
}

/**
 * Encode a 64-bit value into the provided buffer (big endian convention).
 * The destination buffer must be properly aligned.
 *
 * @param dst   the destination buffer (64-bit aligned)
 * @param val   the value to encode
 */
static SPH_INLINE void
sph_enc64be_aligned(void *dst, sph_u64 val)
{
#if SPH_LITTLE_ENDIAN
	*(sph_u64 *)dst = sph_bswap64(val);
#elif SPH_BIG_ENDIAN
	*(sph_u64 *)dst = val;
#else
	((unsigned char *)dst)[0] = (val >> 56);
	((unsigned char *)dst)[1] = (val >> 48);
	((unsigned char *)dst)[2] = (val >> 40);
	((unsigned char *)dst)[3] = (val >> 32);
	((unsigned char *)dst)[4] = (val >> 24);
	((unsigned char *)dst)[5] = (val >> 16);
	((unsigned char *)dst)[6] = (val >> 8);
	((unsigned char *)dst)[7] = val;
#endif
}

/**
 * Decode a 64-bit value from the provided buffer (big endian convention).
 *
 * @param src   the source buffer
 * @return  the decoded value
 */
static SPH_INLINE sph_u64
sph_dec64be(const void *src)
{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_LITTLE_ENDIAN
	return sph_bswap64(*(const sph_u64 *)src);
#else
	return *(const sph_u64 *)src;
#endif
#else
	if (((SPH_UPTR)src & 7) == 0) {
#if SPH_LITTLE_ENDIAN
		return sph_bswap64(*(const sph_u64 *)src);
#else
		return *(const sph_u64 *)src;
#endif
	} else {
		return ((sph_u64)(((const unsigned char *)src)[0]) << 56)
			| ((sph_u64)(((const unsigned char *)src)[1]) << 48)
			| ((sph_u64)(((const unsigned char *)src)[2]) << 40)
			| ((sph_u64)(((const unsigned char *)src)[3]) << 32)
			| ((sph_u64)(((const unsigned char *)src)[4]) << 24)
			| ((sph_u64)(((const unsigned char *)src)[5]) << 16)
			| ((sph_u64)(((const unsigned char *)src)[6]) << 8)
			| (sph_u64)(((const unsigned char *)src)[7]);
	}
#endif
#else
	return ((sph_u64)(((const unsigned char *)src)[0]) << 56)
		| ((sph_u64)(((const unsigned char *)src)[1]) << 48)
		| ((sph_u64)(((const unsigned char *)src)[2]) << 40)
		| ((sph_u64)(((const unsigned char *)src)[3]) << 32)
		| ((sph_u64)(((const unsigned char *)src)[4]) << 24)
		| ((sph_u64)(((const unsigned char *)src)[5]) << 16)
		| ((sph_u64)(((const unsigned char *)src)[6]) << 8)
		| (sph_u64)(((const unsigned char *)src)[7]);
#endif
}

/**
 * Decode a 64-bit value from the provided buffer (big endian convention).
 * The source buffer must be properly aligned.
 *
 * @param src   the source buffer (64-bit aligned)
 * @return  the decoded value
 */
static SPH_INLINE sph_u64
sph_dec64be_aligned(const void *src)
{
#if SPH_LITTLE_ENDIAN
	return sph_bswap64(*(const sph_u64 *)src);
#elif SPH_BIG_ENDIAN
	return *(const sph_u64 *)src;
#else
	return ((sph_u64)(((const unsigned char *)src)[0]) << 56)
		| ((sph_u64)(((const unsigned char *)src)[1]) << 48)
		| ((sph_u64)(((const unsigned char *)src)[2]) << 40)
		| ((sph_u64)(((const unsigned char *)src)[3]) << 32)
		| ((sph_u64)(((const unsigned char *)src)[4]) << 24)
		| ((sph_u64)(((const unsigned char *)src)[5]) << 16)
		| ((sph_u64)(((const unsigned char *)src)[6]) << 8)
		| (sph_u64)(((const unsigned char *)src)[7]);
#endif
}

/**
 * Encode a 64-bit value into the provided buffer (little endian convention).
 *
 * @param dst   the destination buffer
 * @param val   the 64-bit value to encode
 */
static SPH_INLINE void
sph_enc64le(void *dst, sph_u64 val)
{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_BIG_ENDIAN
	val = sph_bswap64(val);
#endif
	*(sph_u64 *)dst = val;
#else
	if (((SPH_UPTR)dst & 7) == 0) {
#if SPH_BIG_ENDIAN
		val = sph_bswap64(val);
#endif
		*(sph_u64 *)dst = val;
	} else {
		((unsigned char *)dst)[0] = val;
		((unsigned char *)dst)[1] = (val >> 8);
		((unsigned char *)dst)[2] = (val >> 16);
		((unsigned char *)dst)[3] = (val >> 24);
		((unsigned char *)dst)[4] = (val >> 32);
		((unsigned char *)dst)[5] = (val >> 40);
		((unsigned char *)dst)[6] = (val >> 48);
		((unsigned char *)dst)[7] = (val >> 56);
	}
#endif
#else
	((unsigned char *)dst)[0] = val;
	((unsigned char *)dst)[1] = (val >> 8);
	((unsigned char *)dst)[2] = (val >> 16);
	((unsigned char *)dst)[3] = (val >> 24);
	((unsigned char *)dst)[4] = (val >> 32);
	((unsigned char *)dst)[5] = (val >> 40);
	((unsigned char *)dst)[6] = (val >> 48);
	((unsigned char *)dst)[7] = (val >> 56);
#endif
}

/**
 * Encode a 64-bit value into the provided buffer (little endian convention).
 * The destination buffer must be properly aligned.
 *
 * @param dst   the destination buffer (64-bit aligned)
 * @param val   the value to encode
 */
static SPH_INLINE void
sph_enc64le_aligned(void *dst, sph_u64 val)
{
#if SPH_LITTLE_ENDIAN
	*(sph_u64 *)dst = val;
#elif SPH_BIG_ENDIAN
	*(sph_u64 *)dst = sph_bswap64(val);
#else
	((unsigned char *)dst)[0] = val;
	((unsigned char *)dst)[1] = (val >> 8);
	((unsigned char *)dst)[2] = (val >> 16);
	((unsigned char *)dst)[3] = (val >> 24);
	((unsigned char *)dst)[4] = (val >> 32);
	((unsigned char *)dst)[5] = (val >> 40);
	((unsigned char *)dst)[6] = (val >> 48);
	((unsigned char *)dst)[7] = (val >> 56);
#endif
}

/**
 * Decode a 64-bit value from the provided buffer (little endian convention).
 *
 * @param src   the source buffer
 * @return  the decoded value
 */
static SPH_INLINE sph_u64
sph_dec64le(const void *src)
{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_BIG_ENDIAN
	return sph_bswap64(*(const sph_u64 *)src);
#else
	return *(const sph_u64 *)src;
#endif
#else
	if (((SPH_UPTR)src & 7) == 0) {
#if SPH_BIG_ENDIAN
#if SPH_SPARCV9_GCC_64 && !SPH_NO_ASM
		sph_u64 tmp;

		__asm__ __volatile__ (
			"ldxa [%1]0x88,%0" : "=r" (tmp) : "r" (src));
		return tmp;
#else
		return sph_bswap64(*(const sph_u64 *)src);
#endif
#else
		return *(const sph_u64 *)src;
#endif
	} else {
		return (sph_u64)(((const unsigned char *)src)[0])
			| ((sph_u64)(((const unsigned char *)src)[1]) << 8)
			| ((sph_u64)(((const unsigned char *)src)[2]) << 16)
			| ((sph_u64)(((const unsigned char *)src)[3]) << 24)
			| ((sph_u64)(((const unsigned char *)src)[4]) << 32)
			| ((sph_u64)(((const unsigned char *)src)[5]) << 40)
			| ((sph_u64)(((const unsigned char *)src)[6]) << 48)
			| ((sph_u64)(((const unsigned char *)src)[7]) << 56);
	}
#endif
#else
	return (sph_u64)(((const unsigned char *)src)[0])
		| ((sph_u64)(((const unsigned char *)src)[1]) << 8)
		| ((sph_u64)(((const unsigned char *)src)[2]) << 16)
		| ((sph_u64)(((const unsigned char *)src)[3]) << 24)
		| ((sph_u64)(((const unsigned char *)src)[4]) << 32)
		| ((sph_u64)(((const unsigned char *)src)[5]) << 40)
		| ((sph_u64)(((const unsigned char *)src)[6]) << 48)
		| ((sph_u64)(((const unsigned char *)src)[7]) << 56);
#endif
}

/**
 * Decode a 64-bit value from the provided buffer (little endian convention).
 * The source buffer must be properly aligned.
 *
 * @param src   the source buffer (64-bit aligned)
 * @return  the decoded value
 */
static SPH_INLINE sph_u64
sph_dec64le_aligned(const void *src)
{
#if SPH_LITTLE_ENDIAN
	return *(const sph_u64 *)src;
#elif SPH_BIG_ENDIAN
#if SPH_SPARCV9_GCC_64 && !SPH_NO_ASM
	sph_u64 tmp;

	__asm__ __volatile__ ("ldxa [%1]0x88,%0" : "=r" (tmp) : "r" (src));
	return tmp;
#else
	return sph_bswap64(*(const sph_u64 *)src);
#endif
#else
	return (sph_u64)(((const unsigned char *)src)[0])
		| ((sph_u64)(((const unsigned char *)src)[1]) << 8)
		| ((sph_u64)(((const unsigned char *)src)[2]) << 16)
		| ((sph_u64)(((const unsigned char *)src)[3]) << 24)
		| ((sph_u64)(((const unsigned char *)src)[4]) << 32)
		| ((sph_u64)(((const unsigned char *)src)[5]) << 40)
		| ((sph_u64)(((const unsigned char *)src)[6]) << 48)
		| ((sph_u64)(((const unsigned char *)src)[7]) << 56);
#endif
}

#endif

#endif
