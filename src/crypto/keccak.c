/* $Id: keccak.c 259 2011-07-19 22:11:27Z tp $ */
/*
 * Keccak implementation.
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
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#include <stddef.h>
#include <string.h>

#include "sph_keccak.h"

#ifdef __cplusplus
extern "C"{
#endif

/*
 * Parameters:
 *
 *  SPH_KECCAK_64          use a 64-bit type
 *  SPH_KECCAK_UNROLL      number of loops to unroll (0/undef for full unroll)
 *  SPH_KECCAK_INTERLEAVE  use bit-interleaving (32-bit type only)
 *  SPH_KECCAK_NOCOPY      do not copy the state into local variables
 *
 * If there is no usable 64-bit type, the code automatically switches
 * back to the 32-bit implementation.
 *
 * Some tests on an Intel Core2 Q6600 (both 64-bit and 32-bit, 32 kB L1
 * code cache), a PowerPC (G3, 32 kB L1 code cache), an ARM920T core
 * (16 kB L1 code cache), and a small MIPS-compatible CPU (Broadcom BCM3302,
 * 8 kB L1 code cache), seem to show that the following are optimal:
 *
 * -- x86, 64-bit: use the 64-bit implementation, unroll 8 rounds,
 * do not copy the state; unrolling 2, 6 or all rounds also provides
 * near-optimal performance.
 * -- x86, 32-bit: use the 32-bit implementation, unroll 6 rounds,
 * interleave, do not copy the state. Unrolling 1, 2, 4 or 8 rounds
 * also provides near-optimal performance.
 * -- PowerPC: use the 64-bit implementation, unroll 8 rounds,
 * copy the state. Unrolling 4 or 6 rounds is near-optimal.
 * -- ARM: use the 64-bit implementation, unroll 2 or 4 rounds,
 * copy the state.
 * -- MIPS: use the 64-bit implementation, unroll 2 rounds, copy
 * the state. Unrolling only 1 round is also near-optimal.
 *
 * Also, interleaving does not always yield actual improvements when
 * using a 32-bit implementation; in particular when the architecture
 * does not offer a native rotation opcode (interleaving replaces one
 * 64-bit rotation with two 32-bit rotations, which is a gain only if
 * there is a native 32-bit rotation opcode and not a native 64-bit
 * rotation opcode; also, interleaving implies a small overhead when
 * processing input words).
 *
 * To sum up:
 * -- when possible, use the 64-bit code
 * -- exception: on 32-bit x86, use 32-bit code
 * -- when using 32-bit code, use interleaving
 * -- copy the state, except on x86
 * -- unroll 8 rounds on "big" machine, 2 rounds on "small" machines
 */

#if SPH_SMALL_FOOTPRINT && !defined SPH_SMALL_FOOTPRINT_KECCAK
#define SPH_SMALL_FOOTPRINT_KECCAK   1
#endif

/*
 * By default, we select the 64-bit implementation if a 64-bit type
 * is available, unless a 32-bit x86 is detected.
 */
#if !defined SPH_KECCAK_64 && SPH_64 \
	&& !(defined __i386__ || SPH_I386_GCC || SPH_I386_MSVC)
#define SPH_KECCAK_64   1
#endif

/*
 * If using a 32-bit implementation, we prefer to interleave.
 */
#if !SPH_KECCAK_64 && !defined SPH_KECCAK_INTERLEAVE
#define SPH_KECCAK_INTERLEAVE   1
#endif

/*
 * Unroll 8 rounds on big systems, 2 rounds on small systems.
 */
#ifndef SPH_KECCAK_UNROLL
#if SPH_SMALL_FOOTPRINT_KECCAK
#define SPH_KECCAK_UNROLL   2
#else
#define SPH_KECCAK_UNROLL   8
#endif
#endif

/*
 * We do not want to copy the state to local variables on x86 (32-bit
 * and 64-bit alike).
 */
#ifndef SPH_KECCAK_NOCOPY
#if defined __i386__ || defined __x86_64 || SPH_I386_MSVC || SPH_I386_GCC
#define SPH_KECCAK_NOCOPY   1
#else
#define SPH_KECCAK_NOCOPY   0
#endif
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4146)
#endif

#if SPH_KECCAK_64

static const sph_u64 RC[] = {
	SPH_C64(0x0000000000000001), SPH_C64(0x0000000000008082),
	SPH_C64(0x800000000000808A), SPH_C64(0x8000000080008000),
	SPH_C64(0x000000000000808B), SPH_C64(0x0000000080000001),
	SPH_C64(0x8000000080008081), SPH_C64(0x8000000000008009),
	SPH_C64(0x000000000000008A), SPH_C64(0x0000000000000088),
	SPH_C64(0x0000000080008009), SPH_C64(0x000000008000000A),
	SPH_C64(0x000000008000808B), SPH_C64(0x800000000000008B),
	SPH_C64(0x8000000000008089), SPH_C64(0x8000000000008003),
	SPH_C64(0x8000000000008002), SPH_C64(0x8000000000000080),
	SPH_C64(0x000000000000800A), SPH_C64(0x800000008000000A),
	SPH_C64(0x8000000080008081), SPH_C64(0x8000000000008080),
	SPH_C64(0x0000000080000001), SPH_C64(0x8000000080008008)
};

#if SPH_KECCAK_NOCOPY

#define a00   (kc->u.wide[ 0])
#define a10   (kc->u.wide[ 1])
#define a20   (kc->u.wide[ 2])
#define a30   (kc->u.wide[ 3])
#define a40   (kc->u.wide[ 4])
#define a01   (kc->u.wide[ 5])
#define a11   (kc->u.wide[ 6])
#define a21   (kc->u.wide[ 7])
#define a31   (kc->u.wide[ 8])
#define a41   (kc->u.wide[ 9])
#define a02   (kc->u.wide[10])
#define a12   (kc->u.wide[11])
#define a22   (kc->u.wide[12])
#define a32   (kc->u.wide[13])
#define a42   (kc->u.wide[14])
#define a03   (kc->u.wide[15])
#define a13   (kc->u.wide[16])
#define a23   (kc->u.wide[17])
#define a33   (kc->u.wide[18])
#define a43   (kc->u.wide[19])
#define a04   (kc->u.wide[20])
#define a14   (kc->u.wide[21])
#define a24   (kc->u.wide[22])
#define a34   (kc->u.wide[23])
#define a44   (kc->u.wide[24])

#define DECL_STATE
#define READ_STATE(sc)
#define WRITE_STATE(sc)

#define INPUT_BUF(size)   do { \
		size_t j; \
		for (j = 0; j < (size); j += 8) { \
			kc->u.wide[j >> 3] ^= sph_dec64le_aligned(buf + j); \
		} \
	} while (0)

#define INPUT_BUF144   INPUT_BUF(144)
#define INPUT_BUF136   INPUT_BUF(136)
#define INPUT_BUF104   INPUT_BUF(104)
#define INPUT_BUF72    INPUT_BUF(72)

#else

#define DECL_STATE \
	sph_u64 a00, a01, a02, a03, a04; \
	sph_u64 a10, a11, a12, a13, a14; \
	sph_u64 a20, a21, a22, a23, a24; \
	sph_u64 a30, a31, a32, a33, a34; \
	sph_u64 a40, a41, a42, a43, a44;

#define READ_STATE(state)   do { \
		a00 = (state)->u.wide[ 0]; \
		a10 = (state)->u.wide[ 1]; \
		a20 = (state)->u.wide[ 2]; \
		a30 = (state)->u.wide[ 3]; \
		a40 = (state)->u.wide[ 4]; \
		a01 = (state)->u.wide[ 5]; \
		a11 = (state)->u.wide[ 6]; \
		a21 = (state)->u.wide[ 7]; \
		a31 = (state)->u.wide[ 8]; \
		a41 = (state)->u.wide[ 9]; \
		a02 = (state)->u.wide[10]; \
		a12 = (state)->u.wide[11]; \
		a22 = (state)->u.wide[12]; \
		a32 = (state)->u.wide[13]; \
		a42 = (state)->u.wide[14]; \
		a03 = (state)->u.wide[15]; \
		a13 = (state)->u.wide[16]; \
		a23 = (state)->u.wide[17]; \
		a33 = (state)->u.wide[18]; \
		a43 = (state)->u.wide[19]; \
		a04 = (state)->u.wide[20]; \
		a14 = (state)->u.wide[21]; \
		a24 = (state)->u.wide[22]; \
		a34 = (state)->u.wide[23]; \
		a44 = (state)->u.wide[24]; \
	} while (0)

#define WRITE_STATE(state)   do { \
		(state)->u.wide[ 0] = a00; \
		(state)->u.wide[ 1] = a10; \
		(state)->u.wide[ 2] = a20; \
		(state)->u.wide[ 3] = a30; \
		(state)->u.wide[ 4] = a40; \
		(state)->u.wide[ 5] = a01; \
		(state)->u.wide[ 6] = a11; \
		(state)->u.wide[ 7] = a21; \
		(state)->u.wide[ 8] = a31; \
		(state)->u.wide[ 9] = a41; \
		(state)->u.wide[10] = a02; \
		(state)->u.wide[11] = a12; \
		(state)->u.wide[12] = a22; \
		(state)->u.wide[13] = a32; \
		(state)->u.wide[14] = a42; \
		(state)->u.wide[15] = a03; \
		(state)->u.wide[16] = a13; \
		(state)->u.wide[17] = a23; \
		(state)->u.wide[18] = a33; \
		(state)->u.wide[19] = a43; \
		(state)->u.wide[20] = a04; \
		(state)->u.wide[21] = a14; \
		(state)->u.wide[22] = a24; \
		(state)->u.wide[23] = a34; \
		(state)->u.wide[24] = a44; \
	} while (0)

#define INPUT_BUF144   do { \
		a00 ^= sph_dec64le_aligned(buf +   0); \
		a10 ^= sph_dec64le_aligned(buf +   8); \
		a20 ^= sph_dec64le_aligned(buf +  16); \
		a30 ^= sph_dec64le_aligned(buf +  24); \
		a40 ^= sph_dec64le_aligned(buf +  32); \
		a01 ^= sph_dec64le_aligned(buf +  40); \
		a11 ^= sph_dec64le_aligned(buf +  48); \
		a21 ^= sph_dec64le_aligned(buf +  56); \
		a31 ^= sph_dec64le_aligned(buf +  64); \
		a41 ^= sph_dec64le_aligned(buf +  72); \
		a02 ^= sph_dec64le_aligned(buf +  80); \
		a12 ^= sph_dec64le_aligned(buf +  88); \
		a22 ^= sph_dec64le_aligned(buf +  96); \
		a32 ^= sph_dec64le_aligned(buf + 104); \
		a42 ^= sph_dec64le_aligned(buf + 112); \
		a03 ^= sph_dec64le_aligned(buf + 120); \
		a13 ^= sph_dec64le_aligned(buf + 128); \
		a23 ^= sph_dec64le_aligned(buf + 136); \
	} while (0)

#define INPUT_BUF136   do { \
		a00 ^= sph_dec64le_aligned(buf +   0); \
		a10 ^= sph_dec64le_aligned(buf +   8); \
		a20 ^= sph_dec64le_aligned(buf +  16); \
		a30 ^= sph_dec64le_aligned(buf +  24); \
		a40 ^= sph_dec64le_aligned(buf +  32); \
		a01 ^= sph_dec64le_aligned(buf +  40); \
		a11 ^= sph_dec64le_aligned(buf +  48); \
		a21 ^= sph_dec64le_aligned(buf +  56); \
		a31 ^= sph_dec64le_aligned(buf +  64); \
		a41 ^= sph_dec64le_aligned(buf +  72); \
		a02 ^= sph_dec64le_aligned(buf +  80); \
		a12 ^= sph_dec64le_aligned(buf +  88); \
		a22 ^= sph_dec64le_aligned(buf +  96); \
		a32 ^= sph_dec64le_aligned(buf + 104); \
		a42 ^= sph_dec64le_aligned(buf + 112); \
		a03 ^= sph_dec64le_aligned(buf + 120); \
		a13 ^= sph_dec64le_aligned(buf + 128); \
	} while (0)

#define INPUT_BUF104   do { \
		a00 ^= sph_dec64le_aligned(buf +   0); \
		a10 ^= sph_dec64le_aligned(buf +   8); \
		a20 ^= sph_dec64le_aligned(buf +  16); \
		a30 ^= sph_dec64le_aligned(buf +  24); \
		a40 ^= sph_dec64le_aligned(buf +  32); \
		a01 ^= sph_dec64le_aligned(buf +  40); \
		a11 ^= sph_dec64le_aligned(buf +  48); \
		a21 ^= sph_dec64le_aligned(buf +  56); \
		a31 ^= sph_dec64le_aligned(buf +  64); \
		a41 ^= sph_dec64le_aligned(buf +  72); \
		a02 ^= sph_dec64le_aligned(buf +  80); \
		a12 ^= sph_dec64le_aligned(buf +  88); \
		a22 ^= sph_dec64le_aligned(buf +  96); \
	} while (0)

#define INPUT_BUF72   do { \
		a00 ^= sph_dec64le_aligned(buf +   0); \
		a10 ^= sph_dec64le_aligned(buf +   8); \
		a20 ^= sph_dec64le_aligned(buf +  16); \
		a30 ^= sph_dec64le_aligned(buf +  24); \
		a40 ^= sph_dec64le_aligned(buf +  32); \
		a01 ^= sph_dec64le_aligned(buf +  40); \
		a11 ^= sph_dec64le_aligned(buf +  48); \
		a21 ^= sph_dec64le_aligned(buf +  56); \
		a31 ^= sph_dec64le_aligned(buf +  64); \
	} while (0)

#define INPUT_BUF(lim)   do { \
		a00 ^= sph_dec64le_aligned(buf +   0); \
		a10 ^= sph_dec64le_aligned(buf +   8); \
		a20 ^= sph_dec64le_aligned(buf +  16); \
		a30 ^= sph_dec64le_aligned(buf +  24); \
		a40 ^= sph_dec64le_aligned(buf +  32); \
		a01 ^= sph_dec64le_aligned(buf +  40); \
		a11 ^= sph_dec64le_aligned(buf +  48); \
		a21 ^= sph_dec64le_aligned(buf +  56); \
		a31 ^= sph_dec64le_aligned(buf +  64); \
		if ((lim) == 72) \
			break; \
		a41 ^= sph_dec64le_aligned(buf +  72); \
		a02 ^= sph_dec64le_aligned(buf +  80); \
		a12 ^= sph_dec64le_aligned(buf +  88); \
		a22 ^= sph_dec64le_aligned(buf +  96); \
		if ((lim) == 104) \
			break; \
		a32 ^= sph_dec64le_aligned(buf + 104); \
		a42 ^= sph_dec64le_aligned(buf + 112); \
		a03 ^= sph_dec64le_aligned(buf + 120); \
		a13 ^= sph_dec64le_aligned(buf + 128); \
		if ((lim) == 136) \
			break; \
		a23 ^= sph_dec64le_aligned(buf + 136); \
	} while (0)

#endif

#define DECL64(x)        sph_u64 x
#define MOV64(d, s)      (d = s)
#define XOR64(d, a, b)   (d = a ^ b)
#define AND64(d, a, b)   (d = a & b)
#define OR64(d, a, b)    (d = a | b)
#define NOT64(d, s)      (d = SPH_T64(~s))
#define ROL64(d, v, n)   (d = SPH_ROTL64(v, n))
#define XOR64_IOTA       XOR64

#else

static const struct {
	sph_u32 high, low;
} RC[] = {
#if SPH_KECCAK_INTERLEAVE
	{ SPH_C32(0x00000000), SPH_C32(0x00000001) },
	{ SPH_C32(0x00000089), SPH_C32(0x00000000) },
	{ SPH_C32(0x8000008B), SPH_C32(0x00000000) },
	{ SPH_C32(0x80008080), SPH_C32(0x00000000) },
	{ SPH_C32(0x0000008B), SPH_C32(0x00000001) },
	{ SPH_C32(0x00008000), SPH_C32(0x00000001) },
	{ SPH_C32(0x80008088), SPH_C32(0x00000001) },
	{ SPH_C32(0x80000082), SPH_C32(0x00000001) },
	{ SPH_C32(0x0000000B), SPH_C32(0x00000000) },
	{ SPH_C32(0x0000000A), SPH_C32(0x00000000) },
	{ SPH_C32(0x00008082), SPH_C32(0x00000001) },
	{ SPH_C32(0x00008003), SPH_C32(0x00000000) },
	{ SPH_C32(0x0000808B), SPH_C32(0x00000001) },
	{ SPH_C32(0x8000000B), SPH_C32(0x00000001) },
	{ SPH_C32(0x8000008A), SPH_C32(0x00000001) },
	{ SPH_C32(0x80000081), SPH_C32(0x00000001) },
	{ SPH_C32(0x80000081), SPH_C32(0x00000000) },
	{ SPH_C32(0x80000008), SPH_C32(0x00000000) },
	{ SPH_C32(0x00000083), SPH_C32(0x00000000) },
	{ SPH_C32(0x80008003), SPH_C32(0x00000000) },
	{ SPH_C32(0x80008088), SPH_C32(0x00000001) },
	{ SPH_C32(0x80000088), SPH_C32(0x00000000) },
	{ SPH_C32(0x00008000), SPH_C32(0x00000001) },
	{ SPH_C32(0x80008082), SPH_C32(0x00000000) }
#else
	{ SPH_C32(0x00000000), SPH_C32(0x00000001) },
	{ SPH_C32(0x00000000), SPH_C32(0x00008082) },
	{ SPH_C32(0x80000000), SPH_C32(0x0000808A) },
	{ SPH_C32(0x80000000), SPH_C32(0x80008000) },
	{ SPH_C32(0x00000000), SPH_C32(0x0000808B) },
	{ SPH_C32(0x00000000), SPH_C32(0x80000001) },
	{ SPH_C32(0x80000000), SPH_C32(0x80008081) },
	{ SPH_C32(0x80000000), SPH_C32(0x00008009) },
	{ SPH_C32(0x00000000), SPH_C32(0x0000008A) },
	{ SPH_C32(0x00000000), SPH_C32(0x00000088) },
	{ SPH_C32(0x00000000), SPH_C32(0x80008009) },
	{ SPH_C32(0x00000000), SPH_C32(0x8000000A) },
	{ SPH_C32(0x00000000), SPH_C32(0x8000808B) },
	{ SPH_C32(0x80000000), SPH_C32(0x0000008B) },
	{ SPH_C32(0x80000000), SPH_C32(0x00008089) },
	{ SPH_C32(0x80000000), SPH_C32(0x00008003) },
	{ SPH_C32(0x80000000), SPH_C32(0x00008002) },
	{ SPH_C32(0x80000000), SPH_C32(0x00000080) },
	{ SPH_C32(0x00000000), SPH_C32(0x0000800A) },
	{ SPH_C32(0x80000000), SPH_C32(0x8000000A) },
	{ SPH_C32(0x80000000), SPH_C32(0x80008081) },
	{ SPH_C32(0x80000000), SPH_C32(0x00008080) },
	{ SPH_C32(0x00000000), SPH_C32(0x80000001) },
	{ SPH_C32(0x80000000), SPH_C32(0x80008008) }
#endif
};

#if SPH_KECCAK_INTERLEAVE

#define INTERLEAVE(xl, xh)   do { \
		sph_u32 l, h, t; \
		l = (xl); h = (xh); \
		t = (l ^ (l >> 1)) & SPH_C32(0x22222222); l ^= t ^ (t << 1); \
		t = (h ^ (h >> 1)) & SPH_C32(0x22222222); h ^= t ^ (t << 1); \
		t = (l ^ (l >> 2)) & SPH_C32(0x0C0C0C0C); l ^= t ^ (t << 2); \
		t = (h ^ (h >> 2)) & SPH_C32(0x0C0C0C0C); h ^= t ^ (t << 2); \
		t = (l ^ (l >> 4)) & SPH_C32(0x00F000F0); l ^= t ^ (t << 4); \
		t = (h ^ (h >> 4)) & SPH_C32(0x00F000F0); h ^= t ^ (t << 4); \
		t = (l ^ (l >> 8)) & SPH_C32(0x0000FF00); l ^= t ^ (t << 8); \
		t = (h ^ (h >> 8)) & SPH_C32(0x0000FF00); h ^= t ^ (t << 8); \
		t = (l ^ SPH_T32(h << 16)) & SPH_C32(0xFFFF0000); \
		l ^= t; h ^= t >> 16; \
		(xl) = l; (xh) = h; \
	} while (0)

#define UNINTERLEAVE(xl, xh)   do { \
		sph_u32 l, h, t; \
		l = (xl); h = (xh); \
		t = (l ^ SPH_T32(h << 16)) & SPH_C32(0xFFFF0000); \
		l ^= t; h ^= t >> 16; \
		t = (l ^ (l >> 8)) & SPH_C32(0x0000FF00); l ^= t ^ (t << 8); \
		t = (h ^ (h >> 8)) & SPH_C32(0x0000FF00); h ^= t ^ (t << 8); \
		t = (l ^ (l >> 4)) & SPH_C32(0x00F000F0); l ^= t ^ (t << 4); \
		t = (h ^ (h >> 4)) & SPH_C32(0x00F000F0); h ^= t ^ (t << 4); \
		t = (l ^ (l >> 2)) & SPH_C32(0x0C0C0C0C); l ^= t ^ (t << 2); \
		t = (h ^ (h >> 2)) & SPH_C32(0x0C0C0C0C); h ^= t ^ (t << 2); \
		t = (l ^ (l >> 1)) & SPH_C32(0x22222222); l ^= t ^ (t << 1); \
		t = (h ^ (h >> 1)) & SPH_C32(0x22222222); h ^= t ^ (t << 1); \
		(xl) = l; (xh) = h; \
	} while (0)

#else

#define INTERLEAVE(l, h)
#define UNINTERLEAVE(l, h)

#endif

#if SPH_KECCAK_NOCOPY

#define a00l   (kc->u.narrow[2 *  0 + 0])
#define a00h   (kc->u.narrow[2 *  0 + 1])
#define a10l   (kc->u.narrow[2 *  1 + 0])
#define a10h   (kc->u.narrow[2 *  1 + 1])
#define a20l   (kc->u.narrow[2 *  2 + 0])
#define a20h   (kc->u.narrow[2 *  2 + 1])
#define a30l   (kc->u.narrow[2 *  3 + 0])
#define a30h   (kc->u.narrow[2 *  3 + 1])
#define a40l   (kc->u.narrow[2 *  4 + 0])
#define a40h   (kc->u.narrow[2 *  4 + 1])
#define a01l   (kc->u.narrow[2 *  5 + 0])
#define a01h   (kc->u.narrow[2 *  5 + 1])
#define a11l   (kc->u.narrow[2 *  6 + 0])
#define a11h   (kc->u.narrow[2 *  6 + 1])
#define a21l   (kc->u.narrow[2 *  7 + 0])
#define a21h   (kc->u.narrow[2 *  7 + 1])
#define a31l   (kc->u.narrow[2 *  8 + 0])
#define a31h   (kc->u.narrow[2 *  8 + 1])
#define a41l   (kc->u.narrow[2 *  9 + 0])
#define a41h   (kc->u.narrow[2 *  9 + 1])
#define a02l   (kc->u.narrow[2 * 10 + 0])
#define a02h   (kc->u.narrow[2 * 10 + 1])
#define a12l   (kc->u.narrow[2 * 11 + 0])
#define a12h   (kc->u.narrow[2 * 11 + 1])
#define a22l   (kc->u.narrow[2 * 12 + 0])
#define a22h   (kc->u.narrow[2 * 12 + 1])
#define a32l   (kc->u.narrow[2 * 13 + 0])
#define a32h   (kc->u.narrow[2 * 13 + 1])
#define a42l   (kc->u.narrow[2 * 14 + 0])
#define a42h   (kc->u.narrow[2 * 14 + 1])
#define a03l   (kc->u.narrow[2 * 15 + 0])
#define a03h   (kc->u.narrow[2 * 15 + 1])
#define a13l   (kc->u.narrow[2 * 16 + 0])
#define a13h   (kc->u.narrow[2 * 16 + 1])
#define a23l   (kc->u.narrow[2 * 17 + 0])
#define a23h   (kc->u.narrow[2 * 17 + 1])
#define a33l   (kc->u.narrow[2 * 18 + 0])
#define a33h   (kc->u.narrow[2 * 18 + 1])
#define a43l   (kc->u.narrow[2 * 19 + 0])
#define a43h   (kc->u.narrow[2 * 19 + 1])
#define a04l   (kc->u.narrow[2 * 20 + 0])
#define a04h   (kc->u.narrow[2 * 20 + 1])
#define a14l   (kc->u.narrow[2 * 21 + 0])
#define a14h   (kc->u.narrow[2 * 21 + 1])
#define a24l   (kc->u.narrow[2 * 22 + 0])
#define a24h   (kc->u.narrow[2 * 22 + 1])
#define a34l   (kc->u.narrow[2 * 23 + 0])
#define a34h   (kc->u.narrow[2 * 23 + 1])
#define a44l   (kc->u.narrow[2 * 24 + 0])
#define a44h   (kc->u.narrow[2 * 24 + 1])

#define DECL_STATE
#define READ_STATE(state)
#define WRITE_STATE(state)

#define INPUT_BUF(size)   do { \
		size_t j; \
		for (j = 0; j < (size); j += 8) { \
			sph_u32 tl, th; \
			tl = sph_dec32le_aligned(buf + j + 0); \
			th = sph_dec32le_aligned(buf + j + 4); \
			INTERLEAVE(tl, th); \
			kc->u.narrow[(j >> 2) + 0] ^= tl; \
			kc->u.narrow[(j >> 2) + 1] ^= th; \
		} \
	} while (0)

#define INPUT_BUF144   INPUT_BUF(144)
#define INPUT_BUF136   INPUT_BUF(136)
#define INPUT_BUF104   INPUT_BUF(104)
#define INPUT_BUF72    INPUT_BUF(72)

#else

#define DECL_STATE \
	sph_u32 a00l, a00h, a01l, a01h, a02l, a02h, a03l, a03h, a04l, a04h; \
	sph_u32 a10l, a10h, a11l, a11h, a12l, a12h, a13l, a13h, a14l, a14h; \
	sph_u32 a20l, a20h, a21l, a21h, a22l, a22h, a23l, a23h, a24l, a24h; \
	sph_u32 a30l, a30h, a31l, a31h, a32l, a32h, a33l, a33h, a34l, a34h; \
	sph_u32 a40l, a40h, a41l, a41h, a42l, a42h, a43l, a43h, a44l, a44h;

#define READ_STATE(state)   do { \
		a00l = (state)->u.narrow[2 *  0 + 0]; \
		a00h = (state)->u.narrow[2 *  0 + 1]; \
		a10l = (state)->u.narrow[2 *  1 + 0]; \
		a10h = (state)->u.narrow[2 *  1 + 1]; \
		a20l = (state)->u.narrow[2 *  2 + 0]; \
		a20h = (state)->u.narrow[2 *  2 + 1]; \
		a30l = (state)->u.narrow[2 *  3 + 0]; \
		a30h = (state)->u.narrow[2 *  3 + 1]; \
		a40l = (state)->u.narrow[2 *  4 + 0]; \
		a40h = (state)->u.narrow[2 *  4 + 1]; \
		a01l = (state)->u.narrow[2 *  5 + 0]; \
		a01h = (state)->u.narrow[2 *  5 + 1]; \
		a11l = (state)->u.narrow[2 *  6 + 0]; \
		a11h = (state)->u.narrow[2 *  6 + 1]; \
		a21l = (state)->u.narrow[2 *  7 + 0]; \
		a21h = (state)->u.narrow[2 *  7 + 1]; \
		a31l = (state)->u.narrow[2 *  8 + 0]; \
		a31h = (state)->u.narrow[2 *  8 + 1]; \
		a41l = (state)->u.narrow[2 *  9 + 0]; \
		a41h = (state)->u.narrow[2 *  9 + 1]; \
		a02l = (state)->u.narrow[2 * 10 + 0]; \
		a02h = (state)->u.narrow[2 * 10 + 1]; \
		a12l = (state)->u.narrow[2 * 11 + 0]; \
		a12h = (state)->u.narrow[2 * 11 + 1]; \
		a22l = (state)->u.narrow[2 * 12 + 0]; \
		a22h = (state)->u.narrow[2 * 12 + 1]; \
		a32l = (state)->u.narrow[2 * 13 + 0]; \
		a32h = (state)->u.narrow[2 * 13 + 1]; \
		a42l = (state)->u.narrow[2 * 14 + 0]; \
		a42h = (state)->u.narrow[2 * 14 + 1]; \
		a03l = (state)->u.narrow[2 * 15 + 0]; \
		a03h = (state)->u.narrow[2 * 15 + 1]; \
		a13l = (state)->u.narrow[2 * 16 + 0]; \
		a13h = (state)->u.narrow[2 * 16 + 1]; \
		a23l = (state)->u.narrow[2 * 17 + 0]; \
		a23h = (state)->u.narrow[2 * 17 + 1]; \
		a33l = (state)->u.narrow[2 * 18 + 0]; \
		a33h = (state)->u.narrow[2 * 18 + 1]; \
		a43l = (state)->u.narrow[2 * 19 + 0]; \
		a43h = (state)->u.narrow[2 * 19 + 1]; \
		a04l = (state)->u.narrow[2 * 20 + 0]; \
		a04h = (state)->u.narrow[2 * 20 + 1]; \
		a14l = (state)->u.narrow[2 * 21 + 0]; \
		a14h = (state)->u.narrow[2 * 21 + 1]; \
		a24l = (state)->u.narrow[2 * 22 + 0]; \
		a24h = (state)->u.narrow[2 * 22 + 1]; \
		a34l = (state)->u.narrow[2 * 23 + 0]; \
		a34h = (state)->u.narrow[2 * 23 + 1]; \
		a44l = (state)->u.narrow[2 * 24 + 0]; \
		a44h = (state)->u.narrow[2 * 24 + 1]; \
	} while (0)

#define WRITE_STATE(state)   do { \
		(state)->u.narrow[2 *  0 + 0] = a00l; \
		(state)->u.narrow[2 *  0 + 1] = a00h; \
		(state)->u.narrow[2 *  1 + 0] = a10l; \
		(state)->u.narrow[2 *  1 + 1] = a10h; \
		(state)->u.narrow[2 *  2 + 0] = a20l; \
		(state)->u.narrow[2 *  2 + 1] = a20h; \
		(state)->u.narrow[2 *  3 + 0] = a30l; \
		(state)->u.narrow[2 *  3 + 1] = a30h; \
		(state)->u.narrow[2 *  4 + 0] = a40l; \
		(state)->u.narrow[2 *  4 + 1] = a40h; \
		(state)->u.narrow[2 *  5 + 0] = a01l; \
		(state)->u.narrow[2 *  5 + 1] = a01h; \
		(state)->u.narrow[2 *  6 + 0] = a11l; \
		(state)->u.narrow[2 *  6 + 1] = a11h; \
		(state)->u.narrow[2 *  7 + 0] = a21l; \
		(state)->u.narrow[2 *  7 + 1] = a21h; \
		(state)->u.narrow[2 *  8 + 0] = a31l; \
		(state)->u.narrow[2 *  8 + 1] = a31h; \
		(state)->u.narrow[2 *  9 + 0] = a41l; \
		(state)->u.narrow[2 *  9 + 1] = a41h; \
		(state)->u.narrow[2 * 10 + 0] = a02l; \
		(state)->u.narrow[2 * 10 + 1] = a02h; \
		(state)->u.narrow[2 * 11 + 0] = a12l; \
		(state)->u.narrow[2 * 11 + 1] = a12h; \
		(state)->u.narrow[2 * 12 + 0] = a22l; \
		(state)->u.narrow[2 * 12 + 1] = a22h; \
		(state)->u.narrow[2 * 13 + 0] = a32l; \
		(state)->u.narrow[2 * 13 + 1] = a32h; \
		(state)->u.narrow[2 * 14 + 0] = a42l; \
		(state)->u.narrow[2 * 14 + 1] = a42h; \
		(state)->u.narrow[2 * 15 + 0] = a03l; \
		(state)->u.narrow[2 * 15 + 1] = a03h; \
		(state)->u.narrow[2 * 16 + 0] = a13l; \
		(state)->u.narrow[2 * 16 + 1] = a13h; \
		(state)->u.narrow[2 * 17 + 0] = a23l; \
		(state)->u.narrow[2 * 17 + 1] = a23h; \
		(state)->u.narrow[2 * 18 + 0] = a33l; \
		(state)->u.narrow[2 * 18 + 1] = a33h; \
		(state)->u.narrow[2 * 19 + 0] = a43l; \
		(state)->u.narrow[2 * 19 + 1] = a43h; \
		(state)->u.narrow[2 * 20 + 0] = a04l; \
		(state)->u.narrow[2 * 20 + 1] = a04h; \
		(state)->u.narrow[2 * 21 + 0] = a14l; \
		(state)->u.narrow[2 * 21 + 1] = a14h; \
		(state)->u.narrow[2 * 22 + 0] = a24l; \
		(state)->u.narrow[2 * 22 + 1] = a24h; \
		(state)->u.narrow[2 * 23 + 0] = a34l; \
		(state)->u.narrow[2 * 23 + 1] = a34h; \
		(state)->u.narrow[2 * 24 + 0] = a44l; \
		(state)->u.narrow[2 * 24 + 1] = a44h; \
	} while (0)

#define READ64(d, off)   do { \
		sph_u32 tl, th; \
		tl = sph_dec32le_aligned(buf + (off)); \
		th = sph_dec32le_aligned(buf + (off) + 4); \
		INTERLEAVE(tl, th); \
		d ## l ^= tl; \
		d ## h ^= th; \
	} while (0)

#define INPUT_BUF144   do { \
		READ64(a00,   0); \
		READ64(a10,   8); \
		READ64(a20,  16); \
		READ64(a30,  24); \
		READ64(a40,  32); \
		READ64(a01,  40); \
		READ64(a11,  48); \
		READ64(a21,  56); \
		READ64(a31,  64); \
		READ64(a41,  72); \
		READ64(a02,  80); \
		READ64(a12,  88); \
		READ64(a22,  96); \
		READ64(a32, 104); \
		READ64(a42, 112); \
		READ64(a03, 120); \
		READ64(a13, 128); \
		READ64(a23, 136); \
	} while (0)

#define INPUT_BUF136   do { \
		READ64(a00,   0); \
		READ64(a10,   8); \
		READ64(a20,  16); \
		READ64(a30,  24); \
		READ64(a40,  32); \
		READ64(a01,  40); \
		READ64(a11,  48); \
		READ64(a21,  56); \
		READ64(a31,  64); \
		READ64(a41,  72); \
		READ64(a02,  80); \
		READ64(a12,  88); \
		READ64(a22,  96); \
		READ64(a32, 104); \
		READ64(a42, 112); \
		READ64(a03, 120); \
		READ64(a13, 128); \
	} while (0)

#define INPUT_BUF104   do { \
		READ64(a00,   0); \
		READ64(a10,   8); \
		READ64(a20,  16); \
		READ64(a30,  24); \
		READ64(a40,  32); \
		READ64(a01,  40); \
		READ64(a11,  48); \
		READ64(a21,  56); \
		READ64(a31,  64); \
		READ64(a41,  72); \
		READ64(a02,  80); \
		READ64(a12,  88); \
		READ64(a22,  96); \
	} while (0)

#define INPUT_BUF72   do { \
		READ64(a00,   0); \
		READ64(a10,   8); \
		READ64(a20,  16); \
		READ64(a30,  24); \
		READ64(a40,  32); \
		READ64(a01,  40); \
		READ64(a11,  48); \
		READ64(a21,  56); \
		READ64(a31,  64); \
	} while (0)

#define INPUT_BUF(lim)   do { \
		READ64(a00,   0); \
		READ64(a10,   8); \
		READ64(a20,  16); \
		READ64(a30,  24); \
		READ64(a40,  32); \
		READ64(a01,  40); \
		READ64(a11,  48); \
		READ64(a21,  56); \
		READ64(a31,  64); \
		if ((lim) == 72) \
			break; \
		READ64(a41,  72); \
		READ64(a02,  80); \
		READ64(a12,  88); \
		READ64(a22,  96); \
		if ((lim) == 104) \
			break; \
		READ64(a32, 104); \
		READ64(a42, 112); \
		READ64(a03, 120); \
		READ64(a13, 128); \
		if ((lim) == 136) \
			break; \
		READ64(a23, 136); \
	} while (0)

#endif

#define DECL64(x)        sph_u64 x ## l, x ## h
#define MOV64(d, s)      (d ## l = s ## l, d ## h = s ## h)
#define XOR64(d, a, b)   (d ## l = a ## l ^ b ## l, d ## h = a ## h ^ b ## h)
#define AND64(d, a, b)   (d ## l = a ## l & b ## l, d ## h = a ## h & b ## h)
#define OR64(d, a, b)    (d ## l = a ## l | b ## l, d ## h = a ## h | b ## h)
#define NOT64(d, s)      (d ## l = SPH_T32(~s ## l), d ## h = SPH_T32(~s ## h))
#define ROL64(d, v, n)   ROL64_ ## n(d, v)

#if SPH_KECCAK_INTERLEAVE

#define ROL64_odd1(d, v)   do { \
		sph_u32 tmp; \
		tmp = v ## l; \
		d ## l = SPH_T32(v ## h << 1) | (v ## h >> 31); \
		d ## h = tmp; \
	} while (0)

#define ROL64_odd63(d, v)   do { \
		sph_u32 tmp; \
		tmp = SPH_T32(v ## l << 31) | (v ## l >> 1); \
		d ## l = v ## h; \
		d ## h = tmp; \
	} while (0)

#define ROL64_odd(d, v, n)   do { \
		sph_u32 tmp; \
		tmp = SPH_T32(v ## l << (n - 1)) | (v ## l >> (33 - n)); \
		d ## l = SPH_T32(v ## h << n) | (v ## h >> (32 - n)); \
		d ## h = tmp; \
	} while (0)

#define ROL64_even(d, v, n)   do { \
		d ## l = SPH_T32(v ## l << n) | (v ## l >> (32 - n)); \
		d ## h = SPH_T32(v ## h << n) | (v ## h >> (32 - n)); \
	} while (0)

#define ROL64_0(d, v)
#define ROL64_1(d, v)    ROL64_odd1(d, v)
#define ROL64_2(d, v)    ROL64_even(d, v,  1)
#define ROL64_3(d, v)    ROL64_odd( d, v,  2)
#define ROL64_4(d, v)    ROL64_even(d, v,  2)
#define ROL64_5(d, v)    ROL64_odd( d, v,  3)
#define ROL64_6(d, v)    ROL64_even(d, v,  3)
#define ROL64_7(d, v)    ROL64_odd( d, v,  4)
#define ROL64_8(d, v)    ROL64_even(d, v,  4)
#define ROL64_9(d, v)    ROL64_odd( d, v,  5)
#define ROL64_10(d, v)   ROL64_even(d, v,  5)
#define ROL64_11(d, v)   ROL64_odd( d, v,  6)
#define ROL64_12(d, v)   ROL64_even(d, v,  6)
#define ROL64_13(d, v)   ROL64_odd( d, v,  7)
#define ROL64_14(d, v)   ROL64_even(d, v,  7)
#define ROL64_15(d, v)   ROL64_odd( d, v,  8)
#define ROL64_16(d, v)   ROL64_even(d, v,  8)
#define ROL64_17(d, v)   ROL64_odd( d, v,  9)
#define ROL64_18(d, v)   ROL64_even(d, v,  9)
#define ROL64_19(d, v)   ROL64_odd( d, v, 10)
#define ROL64_20(d, v)   ROL64_even(d, v, 10)
#define ROL64_21(d, v)   ROL64_odd( d, v, 11)
#define ROL64_22(d, v)   ROL64_even(d, v, 11)
#define ROL64_23(d, v)   ROL64_odd( d, v, 12)
#define ROL64_24(d, v)   ROL64_even(d, v, 12)
#define ROL64_25(d, v)   ROL64_odd( d, v, 13)
#define ROL64_26(d, v)   ROL64_even(d, v, 13)
#define ROL64_27(d, v)   ROL64_odd( d, v, 14)
#define ROL64_28(d, v)   ROL64_even(d, v, 14)
#define ROL64_29(d, v)   ROL64_odd( d, v, 15)
#define ROL64_30(d, v)   ROL64_even(d, v, 15)
#define ROL64_31(d, v)   ROL64_odd( d, v, 16)
#define ROL64_32(d, v)   ROL64_even(d, v, 16)
#define ROL64_33(d, v)   ROL64_odd( d, v, 17)
#define ROL64_34(d, v)   ROL64_even(d, v, 17)
#define ROL64_35(d, v)   ROL64_odd( d, v, 18)
#define ROL64_36(d, v)   ROL64_even(d, v, 18)
#define ROL64_37(d, v)   ROL64_odd( d, v, 19)
#define ROL64_38(d, v)   ROL64_even(d, v, 19)
#define ROL64_39(d, v)   ROL64_odd( d, v, 20)
#define ROL64_40(d, v)   ROL64_even(d, v, 20)
#define ROL64_41(d, v)   ROL64_odd( d, v, 21)
#define ROL64_42(d, v)   ROL64_even(d, v, 21)
#define ROL64_43(d, v)   ROL64_odd( d, v, 22)
#define ROL64_44(d, v)   ROL64_even(d, v, 22)
#define ROL64_45(d, v)   ROL64_odd( d, v, 23)
#define ROL64_46(d, v)   ROL64_even(d, v, 23)
#define ROL64_47(d, v)   ROL64_odd( d, v, 24)
#define ROL64_48(d, v)   ROL64_even(d, v, 24)
#define ROL64_49(d, v)   ROL64_odd( d, v, 25)
#define ROL64_50(d, v)   ROL64_even(d, v, 25)
#define ROL64_51(d, v)   ROL64_odd( d, v, 26)
#define ROL64_52(d, v)   ROL64_even(d, v, 26)
#define ROL64_53(d, v)   ROL64_odd( d, v, 27)
#define ROL64_54(d, v)   ROL64_even(d, v, 27)
#define ROL64_55(d, v)   ROL64_odd( d, v, 28)
#define ROL64_56(d, v)   ROL64_even(d, v, 28)
#define ROL64_57(d, v)   ROL64_odd( d, v, 29)
#define ROL64_58(d, v)   ROL64_even(d, v, 29)
#define ROL64_59(d, v)   ROL64_odd( d, v, 30)
#define ROL64_60(d, v)   ROL64_even(d, v, 30)
#define ROL64_61(d, v)   ROL64_odd( d, v, 31)
#define ROL64_62(d, v)   ROL64_even(d, v, 31)
#define ROL64_63(d, v)   ROL64_odd63(d, v)

#else

#define ROL64_small(d, v, n)   do { \
		sph_u32 tmp; \
		tmp = SPH_T32(v ## l << n) | (v ## h >> (32 - n)); \
		d ## h = SPH_T32(v ## h << n) | (v ## l >> (32 - n)); \
		d ## l = tmp; \
	} while (0)

#define ROL64_0(d, v)    0
#define ROL64_1(d, v)    ROL64_small(d, v, 1)
#define ROL64_2(d, v)    ROL64_small(d, v, 2)
#define ROL64_3(d, v)    ROL64_small(d, v, 3)
#define ROL64_4(d, v)    ROL64_small(d, v, 4)
#define ROL64_5(d, v)    ROL64_small(d, v, 5)
#define ROL64_6(d, v)    ROL64_small(d, v, 6)
#define ROL64_7(d, v)    ROL64_small(d, v, 7)
#define ROL64_8(d, v)    ROL64_small(d, v, 8)
#define ROL64_9(d, v)    ROL64_small(d, v, 9)
#define ROL64_10(d, v)   ROL64_small(d, v, 10)
#define ROL64_11(d, v)   ROL64_small(d, v, 11)
#define ROL64_12(d, v)   ROL64_small(d, v, 12)
#define ROL64_13(d, v)   ROL64_small(d, v, 13)
#define ROL64_14(d, v)   ROL64_small(d, v, 14)
#define ROL64_15(d, v)   ROL64_small(d, v, 15)
#define ROL64_16(d, v)   ROL64_small(d, v, 16)
#define ROL64_17(d, v)   ROL64_small(d, v, 17)
#define ROL64_18(d, v)   ROL64_small(d, v, 18)
#define ROL64_19(d, v)   ROL64_small(d, v, 19)
#define ROL64_20(d, v)   ROL64_small(d, v, 20)
#define ROL64_21(d, v)   ROL64_small(d, v, 21)
#define ROL64_22(d, v)   ROL64_small(d, v, 22)
#define ROL64_23(d, v)   ROL64_small(d, v, 23)
#define ROL64_24(d, v)   ROL64_small(d, v, 24)
#define ROL64_25(d, v)   ROL64_small(d, v, 25)
#define ROL64_26(d, v)   ROL64_small(d, v, 26)
#define ROL64_27(d, v)   ROL64_small(d, v, 27)
#define ROL64_28(d, v)   ROL64_small(d, v, 28)
#define ROL64_29(d, v)   ROL64_small(d, v, 29)
#define ROL64_30(d, v)   ROL64_small(d, v, 30)
#define ROL64_31(d, v)   ROL64_small(d, v, 31)

#define ROL64_32(d, v)   do { \
		sph_u32 tmp; \
		tmp = v ## l; \
		d ## l = v ## h; \
		d ## h = tmp; \
	} while (0)

#define ROL64_big(d, v, n)   do { \
		sph_u32 trl, trh; \
		ROL64_small(tr, v, n); \
		d ## h = trl; \
		d ## l = trh; \
	} while (0)

#define ROL64_33(d, v)   ROL64_big(d, v, 1)
#define ROL64_34(d, v)   ROL64_big(d, v, 2)
#define ROL64_35(d, v)   ROL64_big(d, v, 3)
#define ROL64_36(d, v)   ROL64_big(d, v, 4)
#define ROL64_37(d, v)   ROL64_big(d, v, 5)
#define ROL64_38(d, v)   ROL64_big(d, v, 6)
#define ROL64_39(d, v)   ROL64_big(d, v, 7)
#define ROL64_40(d, v)   ROL64_big(d, v, 8)
#define ROL64_41(d, v)   ROL64_big(d, v, 9)
#define ROL64_42(d, v)   ROL64_big(d, v, 10)
#define ROL64_43(d, v)   ROL64_big(d, v, 11)
#define ROL64_44(d, v)   ROL64_big(d, v, 12)
#define ROL64_45(d, v)   ROL64_big(d, v, 13)
#define ROL64_46(d, v)   ROL64_big(d, v, 14)
#define ROL64_47(d, v)   ROL64_big(d, v, 15)
#define ROL64_48(d, v)   ROL64_big(d, v, 16)
#define ROL64_49(d, v)   ROL64_big(d, v, 17)
#define ROL64_50(d, v)   ROL64_big(d, v, 18)
#define ROL64_51(d, v)   ROL64_big(d, v, 19)
#define ROL64_52(d, v)   ROL64_big(d, v, 20)
#define ROL64_53(d, v)   ROL64_big(d, v, 21)
#define ROL64_54(d, v)   ROL64_big(d, v, 22)
#define ROL64_55(d, v)   ROL64_big(d, v, 23)
#define ROL64_56(d, v)   ROL64_big(d, v, 24)
#define ROL64_57(d, v)   ROL64_big(d, v, 25)
#define ROL64_58(d, v)   ROL64_big(d, v, 26)
#define ROL64_59(d, v)   ROL64_big(d, v, 27)
#define ROL64_60(d, v)   ROL64_big(d, v, 28)
#define ROL64_61(d, v)   ROL64_big(d, v, 29)
#define ROL64_62(d, v)   ROL64_big(d, v, 30)
#define ROL64_63(d, v)   ROL64_big(d, v, 31)

#endif

#define XOR64_IOTA(d, s, k) \
	(d ## l = s ## l ^ k.low, d ## h = s ## h ^ k.high)

#endif

#define TH_ELT(t, c0, c1, c2, c3, c4, d0, d1, d2, d3, d4)   do { \
		DECL64(tt0); \
		DECL64(tt1); \
		DECL64(tt2); \
		DECL64(tt3); \
		XOR64(tt0, d0, d1); \
		XOR64(tt1, d2, d3); \
		XOR64(tt0, tt0, d4); \
		XOR64(tt0, tt0, tt1); \
		ROL64(tt0, tt0, 1); \
		XOR64(tt2, c0, c1); \
		XOR64(tt3, c2, c3); \
		XOR64(tt0, tt0, c4); \
		XOR64(tt2, tt2, tt3); \
		XOR64(t, tt0, tt2); \
	} while (0)

#define THETA(b00, b01, b02, b03, b04, b10, b11, b12, b13, b14, \
	b20, b21, b22, b23, b24, b30, b31, b32, b33, b34, \
	b40, b41, b42, b43, b44) \
	do { \
		DECL64(t0); \
		DECL64(t1); \
		DECL64(t2); \
		DECL64(t3); \
		DECL64(t4); \
		TH_ELT(t0, b40, b41, b42, b43, b44, b10, b11, b12, b13, b14); \
		TH_ELT(t1, b00, b01, b02, b03, b04, b20, b21, b22, b23, b24); \
		TH_ELT(t2, b10, b11, b12, b13, b14, b30, b31, b32, b33, b34); \
		TH_ELT(t3, b20, b21, b22, b23, b24, b40, b41, b42, b43, b44); \
		TH_ELT(t4, b30, b31, b32, b33, b34, b00, b01, b02, b03, b04); \
		XOR64(b00, b00, t0); \
		XOR64(b01, b01, t0); \
		XOR64(b02, b02, t0); \
		XOR64(b03, b03, t0); \
		XOR64(b04, b04, t0); \
		XOR64(b10, b10, t1); \
		XOR64(b11, b11, t1); \
		XOR64(b12, b12, t1); \
		XOR64(b13, b13, t1); \
		XOR64(b14, b14, t1); \
		XOR64(b20, b20, t2); \
		XOR64(b21, b21, t2); \
		XOR64(b22, b22, t2); \
		XOR64(b23, b23, t2); \
		XOR64(b24, b24, t2); \
		XOR64(b30, b30, t3); \
		XOR64(b31, b31, t3); \
		XOR64(b32, b32, t3); \
		XOR64(b33, b33, t3); \
		XOR64(b34, b34, t3); \
		XOR64(b40, b40, t4); \
		XOR64(b41, b41, t4); \
		XOR64(b42, b42, t4); \
		XOR64(b43, b43, t4); \
		XOR64(b44, b44, t4); \
	} while (0)

#define RHO(b00, b01, b02, b03, b04, b10, b11, b12, b13, b14, \
	b20, b21, b22, b23, b24, b30, b31, b32, b33, b34, \
	b40, b41, b42, b43, b44) \
	do { \
		/* ROL64(b00, b00,  0); */ \
		ROL64(b01, b01, 36); \
		ROL64(b02, b02,  3); \
		ROL64(b03, b03, 41); \
		ROL64(b04, b04, 18); \
		ROL64(b10, b10,  1); \
		ROL64(b11, b11, 44); \
		ROL64(b12, b12, 10); \
		ROL64(b13, b13, 45); \
		ROL64(b14, b14,  2); \
		ROL64(b20, b20, 62); \
		ROL64(b21, b21,  6); \
		ROL64(b22, b22, 43); \
		ROL64(b23, b23, 15); \
		ROL64(b24, b24, 61); \
		ROL64(b30, b30, 28); \
		ROL64(b31, b31, 55); \
		ROL64(b32, b32, 25); \
		ROL64(b33, b33, 21); \
		ROL64(b34, b34, 56); \
		ROL64(b40, b40, 27); \
		ROL64(b41, b41, 20); \
		ROL64(b42, b42, 39); \
		ROL64(b43, b43,  8); \
		ROL64(b44, b44, 14); \
	} while (0)

/*
 * The KHI macro integrates the "lane complement" optimization. On input,
 * some words are complemented:
 *    a00 a01 a02 a04 a13 a20 a21 a22 a30 a33 a34 a43
 * On output, the following words are complemented:
 *    a04 a10 a20 a22 a23 a31
 *
 * The (implicit) permutation and the theta expansion will bring back
 * the input mask for the next round.
 */

#define KHI_XO(d, a, b, c)   do { \
		DECL64(kt); \
		OR64(kt, b, c); \
		XOR64(d, a, kt); \
	} while (0)

#define KHI_XA(d, a, b, c)   do { \
		DECL64(kt); \
		AND64(kt, b, c); \
		XOR64(d, a, kt); \
	} while (0)

#define KHI(b00, b01, b02, b03, b04, b10, b11, b12, b13, b14, \
	b20, b21, b22, b23, b24, b30, b31, b32, b33, b34, \
	b40, b41, b42, b43, b44) \
	do { \
		DECL64(c0); \
		DECL64(c1); \
		DECL64(c2); \
		DECL64(c3); \
		DECL64(c4); \
		DECL64(bnn); \
		NOT64(bnn, b20); \
		KHI_XO(c0, b00, b10, b20); \
		KHI_XO(c1, b10, bnn, b30); \
		KHI_XA(c2, b20, b30, b40); \
		KHI_XO(c3, b30, b40, b00); \
		KHI_XA(c4, b40, b00, b10); \
		MOV64(b00, c0); \
		MOV64(b10, c1); \
		MOV64(b20, c2); \
		MOV64(b30, c3); \
		MOV64(b40, c4); \
		NOT64(bnn, b41); \
		KHI_XO(c0, b01, b11, b21); \
		KHI_XA(c1, b11, b21, b31); \
		KHI_XO(c2, b21, b31, bnn); \
		KHI_XO(c3, b31, b41, b01); \
		KHI_XA(c4, b41, b01, b11); \
		MOV64(b01, c0); \
		MOV64(b11, c1); \
		MOV64(b21, c2); \
		MOV64(b31, c3); \
		MOV64(b41, c4); \
		NOT64(bnn, b32); \
		KHI_XO(c0, b02, b12, b22); \
		KHI_XA(c1, b12, b22, b32); \
		KHI_XA(c2, b22, bnn, b42); \
		KHI_XO(c3, bnn, b42, b02); \
		KHI_XA(c4, b42, b02, b12); \
		MOV64(b02, c0); \
		MOV64(b12, c1); \
		MOV64(b22, c2); \
		MOV64(b32, c3); \
		MOV64(b42, c4); \
		NOT64(bnn, b33); \
		KHI_XA(c0, b03, b13, b23); \
		KHI_XO(c1, b13, b23, b33); \
		KHI_XO(c2, b23, bnn, b43); \
		KHI_XA(c3, bnn, b43, b03); \
		KHI_XO(c4, b43, b03, b13); \
		MOV64(b03, c0); \
		MOV64(b13, c1); \
		MOV64(b23, c2); \
		MOV64(b33, c3); \
		MOV64(b43, c4); \
		NOT64(bnn, b14); \
		KHI_XA(c0, b04, bnn, b24); \
		KHI_XO(c1, bnn, b24, b34); \
		KHI_XA(c2, b24, b34, b44); \
		KHI_XO(c3, b34, b44, b04); \
		KHI_XA(c4, b44, b04, b14); \
		MOV64(b04, c0); \
		MOV64(b14, c1); \
		MOV64(b24, c2); \
		MOV64(b34, c3); \
		MOV64(b44, c4); \
	} while (0)

#define IOTA(r)   XOR64_IOTA(a00, a00, r)

#define P0    a00, a01, a02, a03, a04, a10, a11, a12, a13, a14, a20, a21, \
              a22, a23, a24, a30, a31, a32, a33, a34, a40, a41, a42, a43, a44
#define P1    a00, a30, a10, a40, a20, a11, a41, a21, a01, a31, a22, a02, \
              a32, a12, a42, a33, a13, a43, a23, a03, a44, a24, a04, a34, a14
#define P2    a00, a33, a11, a44, a22, a41, a24, a02, a30, a13, a32, a10, \
              a43, a21, a04, a23, a01, a34, a12, a40, a14, a42, a20, a03, a31
#define P3    a00, a23, a41, a14, a32, a24, a42, a10, a33, a01, a43, a11, \
              a34, a02, a20, a12, a30, a03, a21, a44, a31, a04, a22, a40, a13
#define P4    a00, a12, a24, a31, a43, a42, a04, a11, a23, a30, a34, a41, \
              a03, a10, a22, a21, a33, a40, a02, a14, a13, a20, a32, a44, a01
#define P5    a00, a21, a42, a13, a34, a04, a20, a41, a12, a33, a03, a24, \
              a40, a11, a32, a02, a23, a44, a10, a31, a01, a22, a43, a14, a30
#define P6    a00, a02, a04, a01, a03, a20, a22, a24, a21, a23, a40, a42, \
              a44, a41, a43, a10, a12, a14, a11, a13, a30, a32, a34, a31, a33
#define P7    a00, a10, a20, a30, a40, a22, a32, a42, a02, a12, a44, a04, \
              a14, a24, a34, a11, a21, a31, a41, a01, a33, a43, a03, a13, a23
#define P8    a00, a11, a22, a33, a44, a32, a43, a04, a10, a21, a14, a20, \
              a31, a42, a03, a41, a02, a13, a24, a30, a23, a34, a40, a01, a12
#define P9    a00, a41, a32, a23, a14, a43, a34, a20, a11, a02, a31, a22, \
              a13, a04, a40, a24, a10, a01, a42, a33, a12, a03, a44, a30, a21
#define P10   a00, a24, a43, a12, a31, a34, a03, a22, a41, a10, a13, a32, \
              a01, a20, a44, a42, a11, a30, a04, a23, a21, a40, a14, a33, a02
#define P11   a00, a42, a34, a21, a13, a03, a40, a32, a24, a11, a01, a43, \
              a30, a22, a14, a04, a41, a33, a20, a12, a02, a44, a31, a23, a10
#define P12   a00, a04, a03, a02, a01, a40, a44, a43, a42, a41, a30, a34, \
              a33, a32, a31, a20, a24, a23, a22, a21, a10, a14, a13, a12, a11
#define P13   a00, a20, a40, a10, a30, a44, a14, a34, a04, a24, a33, a03, \
              a23, a43, a13, a22, a42, a12, a32, a02, a11, a31, a01, a21, a41
#define P14   a00, a22, a44, a11, a33, a14, a31, a03, a20, a42, a23, a40, \
              a12, a34, a01, a32, a04, a21, a43, a10, a41, a13, a30, a02, a24
#define P15   a00, a32, a14, a41, a23, a31, a13, a40, a22, a04, a12, a44, \
              a21, a03, a30, a43, a20, a02, a34, a11, a24, a01, a33, a10, a42
#define P16   a00, a43, a31, a24, a12, a13, a01, a44, a32, a20, a21, a14, \
              a02, a40, a33, a34, a22, a10, a03, a41, a42, a30, a23, a11, a04
#define P17   a00, a34, a13, a42, a21, a01, a30, a14, a43, a22, a02, a31, \
              a10, a44, a23, a03, a32, a11, a40, a24, a04, a33, a12, a41, a20
#define P18   a00, a03, a01, a04, a02, a30, a33, a31, a34, a32, a10, a13, \
              a11, a14, a12, a40, a43, a41, a44, a42, a20, a23, a21, a24, a22
#define P19   a00, a40, a30, a20, a10, a33, a23, a13, a03, a43, a11, a01, \
              a41, a31, a21, a44, a34, a24, a14, a04, a22, a12, a02, a42, a32
#define P20   a00, a44, a33, a22, a11, a23, a12, a01, a40, a34, a41, a30, \
              a24, a13, a02, a14, a03, a42, a31, a20, a32, a21, a10, a04, a43
#define P21   a00, a14, a23, a32, a41, a12, a21, a30, a44, a03, a24, a33, \
              a42, a01, a10, a31, a40, a04, a13, a22, a43, a02, a11, a20, a34
#define P22   a00, a31, a12, a43, a24, a21, a02, a33, a14, a40, a42, a23, \
              a04, a30, a11, a13, a44, a20, a01, a32, a34, a10, a41, a22, a03
#define P23   a00, a13, a21, a34, a42, a02, a10, a23, a31, a44, a04, a12, \
              a20, a33, a41, a01, a14, a22, a30, a43, a03, a11, a24, a32, a40

#define P1_TO_P0   do { \
		DECL64(t); \
		MOV64(t, a01); \
		MOV64(a01, a30); \
		MOV64(a30, a33); \
		MOV64(a33, a23); \
		MOV64(a23, a12); \
		MOV64(a12, a21); \
		MOV64(a21, a02); \
		MOV64(a02, a10); \
		MOV64(a10, a11); \
		MOV64(a11, a41); \
		MOV64(a41, a24); \
		MOV64(a24, a42); \
		MOV64(a42, a04); \
		MOV64(a04, a20); \
		MOV64(a20, a22); \
		MOV64(a22, a32); \
		MOV64(a32, a43); \
		MOV64(a43, a34); \
		MOV64(a34, a03); \
		MOV64(a03, a40); \
		MOV64(a40, a44); \
		MOV64(a44, a14); \
		MOV64(a14, a31); \
		MOV64(a31, a13); \
		MOV64(a13, t); \
	} while (0)

#define P2_TO_P0   do { \
		DECL64(t); \
		MOV64(t, a01); \
		MOV64(a01, a33); \
		MOV64(a33, a12); \
		MOV64(a12, a02); \
		MOV64(a02, a11); \
		MOV64(a11, a24); \
		MOV64(a24, a04); \
		MOV64(a04, a22); \
		MOV64(a22, a43); \
		MOV64(a43, a03); \
		MOV64(a03, a44); \
		MOV64(a44, a31); \
		MOV64(a31, t); \
		MOV64(t, a10); \
		MOV64(a10, a41); \
		MOV64(a41, a42); \
		MOV64(a42, a20); \
		MOV64(a20, a32); \
		MOV64(a32, a34); \
		MOV64(a34, a40); \
		MOV64(a40, a14); \
		MOV64(a14, a13); \
		MOV64(a13, a30); \
		MOV64(a30, a23); \
		MOV64(a23, a21); \
		MOV64(a21, t); \
	} while (0)

#define P4_TO_P0   do { \
		DECL64(t); \
		MOV64(t, a01); \
		MOV64(a01, a12); \
		MOV64(a12, a11); \
		MOV64(a11, a04); \
		MOV64(a04, a43); \
		MOV64(a43, a44); \
		MOV64(a44, t); \
		MOV64(t, a02); \
		MOV64(a02, a24); \
		MOV64(a24, a22); \
		MOV64(a22, a03); \
		MOV64(a03, a31); \
		MOV64(a31, a33); \
		MOV64(a33, t); \
		MOV64(t, a10); \
		MOV64(a10, a42); \
		MOV64(a42, a32); \
		MOV64(a32, a40); \
		MOV64(a40, a13); \
		MOV64(a13, a23); \
		MOV64(a23, t); \
		MOV64(t, a14); \
		MOV64(a14, a30); \
		MOV64(a30, a21); \
		MOV64(a21, a41); \
		MOV64(a41, a20); \
		MOV64(a20, a34); \
		MOV64(a34, t); \
	} while (0)

#define P6_TO_P0   do { \
		DECL64(t); \
		MOV64(t, a01); \
		MOV64(a01, a02); \
		MOV64(a02, a04); \
		MOV64(a04, a03); \
		MOV64(a03, t); \
		MOV64(t, a10); \
		MOV64(a10, a20); \
		MOV64(a20, a40); \
		MOV64(a40, a30); \
		MOV64(a30, t); \
		MOV64(t, a11); \
		MOV64(a11, a22); \
		MOV64(a22, a44); \
		MOV64(a44, a33); \
		MOV64(a33, t); \
		MOV64(t, a12); \
		MOV64(a12, a24); \
		MOV64(a24, a43); \
		MOV64(a43, a31); \
		MOV64(a31, t); \
		MOV64(t, a13); \
		MOV64(a13, a21); \
		MOV64(a21, a42); \
		MOV64(a42, a34); \
		MOV64(a34, t); \
		MOV64(t, a14); \
		MOV64(a14, a23); \
		MOV64(a23, a41); \
		MOV64(a41, a32); \
		MOV64(a32, t); \
	} while (0)

#define P8_TO_P0   do { \
		DECL64(t); \
		MOV64(t, a01); \
		MOV64(a01, a11); \
		MOV64(a11, a43); \
		MOV64(a43, t); \
		MOV64(t, a02); \
		MOV64(a02, a22); \
		MOV64(a22, a31); \
		MOV64(a31, t); \
		MOV64(t, a03); \
		MOV64(a03, a33); \
		MOV64(a33, a24); \
		MOV64(a24, t); \
		MOV64(t, a04); \
		MOV64(a04, a44); \
		MOV64(a44, a12); \
		MOV64(a12, t); \
		MOV64(t, a10); \
		MOV64(a10, a32); \
		MOV64(a32, a13); \
		MOV64(a13, t); \
		MOV64(t, a14); \
		MOV64(a14, a21); \
		MOV64(a21, a20); \
		MOV64(a20, t); \
		MOV64(t, a23); \
		MOV64(a23, a42); \
		MOV64(a42, a40); \
		MOV64(a40, t); \
		MOV64(t, a30); \
		MOV64(a30, a41); \
		MOV64(a41, a34); \
		MOV64(a34, t); \
	} while (0)

#define P12_TO_P0   do { \
		DECL64(t); \
		MOV64(t, a01); \
		MOV64(a01, a04); \
		MOV64(a04, t); \
		MOV64(t, a02); \
		MOV64(a02, a03); \
		MOV64(a03, t); \
		MOV64(t, a10); \
		MOV64(a10, a40); \
		MOV64(a40, t); \
		MOV64(t, a11); \
		MOV64(a11, a44); \
		MOV64(a44, t); \
		MOV64(t, a12); \
		MOV64(a12, a43); \
		MOV64(a43, t); \
		MOV64(t, a13); \
		MOV64(a13, a42); \
		MOV64(a42, t); \
		MOV64(t, a14); \
		MOV64(a14, a41); \
		MOV64(a41, t); \
		MOV64(t, a20); \
		MOV64(a20, a30); \
		MOV64(a30, t); \
		MOV64(t, a21); \
		MOV64(a21, a34); \
		MOV64(a34, t); \
		MOV64(t, a22); \
		MOV64(a22, a33); \
		MOV64(a33, t); \
		MOV64(t, a23); \
		MOV64(a23, a32); \
		MOV64(a32, t); \
		MOV64(t, a24); \
		MOV64(a24, a31); \
		MOV64(a31, t); \
	} while (0)

#define LPAR   (
#define RPAR   )

#define KF_ELT(r, s, k)   do { \
		THETA LPAR P ## r RPAR; \
		RHO LPAR P ## r RPAR; \
		KHI LPAR P ## s RPAR; \
		IOTA(k); \
	} while (0)

#define DO(x)   x

#define KECCAK_F_1600   DO(KECCAK_F_1600_)

#if SPH_KECCAK_UNROLL == 1

#define KECCAK_F_1600_   do { \
		int j; \
		for (j = 0; j < 24; j ++) { \
			KF_ELT( 0,  1, RC[j + 0]); \
			P1_TO_P0; \
		} \
	} while (0)

#elif SPH_KECCAK_UNROLL == 2

#define KECCAK_F_1600_   do { \
		int j; \
		for (j = 0; j < 24; j += 2) { \
			KF_ELT( 0,  1, RC[j + 0]); \
			KF_ELT( 1,  2, RC[j + 1]); \
			P2_TO_P0; \
		} \
	} while (0)

#elif SPH_KECCAK_UNROLL == 4

#define KECCAK_F_1600_   do { \
		int j; \
		for (j = 0; j < 24; j += 4) { \
			KF_ELT( 0,  1, RC[j + 0]); \
			KF_ELT( 1,  2, RC[j + 1]); \
			KF_ELT( 2,  3, RC[j + 2]); \
			KF_ELT( 3,  4, RC[j + 3]); \
			P4_TO_P0; \
		} \
	} while (0)

#elif SPH_KECCAK_UNROLL == 6

#define KECCAK_F_1600_   do { \
		int j; \
		for (j = 0; j < 24; j += 6) { \
			KF_ELT( 0,  1, RC[j + 0]); \
			KF_ELT( 1,  2, RC[j + 1]); \
			KF_ELT( 2,  3, RC[j + 2]); \
			KF_ELT( 3,  4, RC[j + 3]); \
			KF_ELT( 4,  5, RC[j + 4]); \
			KF_ELT( 5,  6, RC[j + 5]); \
			P6_TO_P0; \
		} \
	} while (0)

#elif SPH_KECCAK_UNROLL == 8

#define KECCAK_F_1600_   do { \
		int j; \
		for (j = 0; j < 24; j += 8) { \
			KF_ELT( 0,  1, RC[j + 0]); \
			KF_ELT( 1,  2, RC[j + 1]); \
			KF_ELT( 2,  3, RC[j + 2]); \
			KF_ELT( 3,  4, RC[j + 3]); \
			KF_ELT( 4,  5, RC[j + 4]); \
			KF_ELT( 5,  6, RC[j + 5]); \
			KF_ELT( 6,  7, RC[j + 6]); \
			KF_ELT( 7,  8, RC[j + 7]); \
			P8_TO_P0; \
		} \
	} while (0)

#elif SPH_KECCAK_UNROLL == 12

#define KECCAK_F_1600_   do { \
		int j; \
		for (j = 0; j < 24; j += 12) { \
			KF_ELT( 0,  1, RC[j +  0]); \
			KF_ELT( 1,  2, RC[j +  1]); \
			KF_ELT( 2,  3, RC[j +  2]); \
			KF_ELT( 3,  4, RC[j +  3]); \
			KF_ELT( 4,  5, RC[j +  4]); \
			KF_ELT( 5,  6, RC[j +  5]); \
			KF_ELT( 6,  7, RC[j +  6]); \
			KF_ELT( 7,  8, RC[j +  7]); \
			KF_ELT( 8,  9, RC[j +  8]); \
			KF_ELT( 9, 10, RC[j +  9]); \
			KF_ELT(10, 11, RC[j + 10]); \
			KF_ELT(11, 12, RC[j + 11]); \
			P12_TO_P0; \
		} \
	} while (0)

#elif SPH_KECCAK_UNROLL == 0

#define KECCAK_F_1600_   do { \
		KF_ELT( 0,  1, RC[ 0]); \
		KF_ELT( 1,  2, RC[ 1]); \
		KF_ELT( 2,  3, RC[ 2]); \
		KF_ELT( 3,  4, RC[ 3]); \
		KF_ELT( 4,  5, RC[ 4]); \
		KF_ELT( 5,  6, RC[ 5]); \
		KF_ELT( 6,  7, RC[ 6]); \
		KF_ELT( 7,  8, RC[ 7]); \
		KF_ELT( 8,  9, RC[ 8]); \
		KF_ELT( 9, 10, RC[ 9]); \
		KF_ELT(10, 11, RC[10]); \
		KF_ELT(11, 12, RC[11]); \
		KF_ELT(12, 13, RC[12]); \
		KF_ELT(13, 14, RC[13]); \
		KF_ELT(14, 15, RC[14]); \
		KF_ELT(15, 16, RC[15]); \
		KF_ELT(16, 17, RC[16]); \
		KF_ELT(17, 18, RC[17]); \
		KF_ELT(18, 19, RC[18]); \
		KF_ELT(19, 20, RC[19]); \
		KF_ELT(20, 21, RC[20]); \
		KF_ELT(21, 22, RC[21]); \
		KF_ELT(22, 23, RC[22]); \
		KF_ELT(23,  0, RC[23]); \
	} while (0)

#else

#error Unimplemented unroll count for Keccak.

#endif

static void
keccak_init(sph_keccak_context *kc, unsigned out_size)
{
	int i;

#if SPH_KECCAK_64
	for (i = 0; i < 25; i ++)
		kc->u.wide[i] = 0;
	/*
	 * Initialization for the "lane complement".
	 */
	kc->u.wide[ 1] = SPH_C64(0xFFFFFFFFFFFFFFFF);
	kc->u.wide[ 2] = SPH_C64(0xFFFFFFFFFFFFFFFF);
	kc->u.wide[ 8] = SPH_C64(0xFFFFFFFFFFFFFFFF);
	kc->u.wide[12] = SPH_C64(0xFFFFFFFFFFFFFFFF);
	kc->u.wide[17] = SPH_C64(0xFFFFFFFFFFFFFFFF);
	kc->u.wide[20] = SPH_C64(0xFFFFFFFFFFFFFFFF);
#else

	for (i = 0; i < 50; i ++)
		kc->u.narrow[i] = 0;
	/*
	 * Initialization for the "lane complement".
	 * Note: since we set to all-one full 64-bit words,
	 * interleaving (if applicable) is a no-op.
	 */
	kc->u.narrow[ 2] = SPH_C32(0xFFFFFFFF);
	kc->u.narrow[ 3] = SPH_C32(0xFFFFFFFF);
	kc->u.narrow[ 4] = SPH_C32(0xFFFFFFFF);
	kc->u.narrow[ 5] = SPH_C32(0xFFFFFFFF);
	kc->u.narrow[16] = SPH_C32(0xFFFFFFFF);
	kc->u.narrow[17] = SPH_C32(0xFFFFFFFF);
	kc->u.narrow[24] = SPH_C32(0xFFFFFFFF);
	kc->u.narrow[25] = SPH_C32(0xFFFFFFFF);
	kc->u.narrow[34] = SPH_C32(0xFFFFFFFF);
	kc->u.narrow[35] = SPH_C32(0xFFFFFFFF);
	kc->u.narrow[40] = SPH_C32(0xFFFFFFFF);
	kc->u.narrow[41] = SPH_C32(0xFFFFFFFF);
#endif
	kc->ptr = 0;
	kc->lim = 200 - (out_size >> 2);
}

static void
keccak_core(sph_keccak_context *kc, const void *data, size_t len, size_t lim)
{
	unsigned char *buf;
	size_t ptr;
	DECL_STATE

	buf = kc->buf;
	ptr = kc->ptr;

	if (len < (lim - ptr)) {
		memcpy(buf + ptr, data, len);
		kc->ptr = ptr + len;
		return;
	}

	READ_STATE(kc);
	while (len > 0) {
		size_t clen;

		clen = (lim - ptr);
		if (clen > len)
			clen = len;
		memcpy(buf + ptr, data, clen);
		ptr += clen;
		data = (const unsigned char *)data + clen;
		len -= clen;
		if (ptr == lim) {
			INPUT_BUF(lim);
			KECCAK_F_1600;
			ptr = 0;
		}
	}
	WRITE_STATE(kc);
	kc->ptr = ptr;
}

#if SPH_KECCAK_64

#define DEFCLOSE(d, lim) \
	static void keccak_close ## d( \
		sph_keccak_context *kc, unsigned ub, unsigned n, void *dst) \
	{ \
		unsigned eb; \
		union { \
			unsigned char tmp[lim + 1]; \
			sph_u64 dummy;   /* for alignment */ \
		} u; \
		size_t j; \
 \
		eb = (0x100 | (ub & 0xFF)) >> (8 - n); \
		if (kc->ptr == (lim - 1)) { \
			if (n == 7) { \
				u.tmp[0] = eb; \
				memset(u.tmp + 1, 0, lim - 1); \
				u.tmp[lim] = 0x80; \
				j = 1 + lim; \
			} else { \
				u.tmp[0] = eb | 0x80; \
				j = 1; \
			} \
		} else { \
			j = lim - kc->ptr; \
			u.tmp[0] = eb; \
			memset(u.tmp + 1, 0, j - 2); \
			u.tmp[j - 1] = 0x80; \
		} \
		keccak_core(kc, u.tmp, j, lim); \
		/* Finalize the "lane complement" */ \
		kc->u.wide[ 1] = ~kc->u.wide[ 1]; \
		kc->u.wide[ 2] = ~kc->u.wide[ 2]; \
		kc->u.wide[ 8] = ~kc->u.wide[ 8]; \
		kc->u.wide[12] = ~kc->u.wide[12]; \
		kc->u.wide[17] = ~kc->u.wide[17]; \
		kc->u.wide[20] = ~kc->u.wide[20]; \
		for (j = 0; j < d; j += 8) \
			sph_enc64le_aligned(u.tmp + j, kc->u.wide[j >> 3]); \
		memcpy(dst, u.tmp, d); \
		keccak_init(kc, (unsigned)d << 3); \
	} \

#else

#define DEFCLOSE(d, lim) \
	static void keccak_close ## d( \
		sph_keccak_context *kc, unsigned ub, unsigned n, void *dst) \
	{ \
		unsigned eb; \
		union { \
			unsigned char tmp[lim + 1]; \
			sph_u64 dummy;   /* for alignment */ \
		} u; \
		size_t j; \
 \
		eb = (0x100 | (ub & 0xFF)) >> (8 - n); \
		if (kc->ptr == (lim - 1)) { \
			if (n == 7) { \
				u.tmp[0] = eb; \
				memset(u.tmp + 1, 0, lim - 1); \
				u.tmp[lim] = 0x80; \
				j = 1 + lim; \
			} else { \
				u.tmp[0] = eb | 0x80; \
				j = 1; \
			} \
		} else { \
			j = lim - kc->ptr; \
			u.tmp[0] = eb; \
			memset(u.tmp + 1, 0, j - 2); \
			u.tmp[j - 1] = 0x80; \
		} \
		keccak_core(kc, u.tmp, j, lim); \
		/* Finalize the "lane complement" */ \
		kc->u.narrow[ 2] = ~kc->u.narrow[ 2]; \
		kc->u.narrow[ 3] = ~kc->u.narrow[ 3]; \
		kc->u.narrow[ 4] = ~kc->u.narrow[ 4]; \
		kc->u.narrow[ 5] = ~kc->u.narrow[ 5]; \
		kc->u.narrow[16] = ~kc->u.narrow[16]; \
		kc->u.narrow[17] = ~kc->u.narrow[17]; \
		kc->u.narrow[24] = ~kc->u.narrow[24]; \
		kc->u.narrow[25] = ~kc->u.narrow[25]; \
		kc->u.narrow[34] = ~kc->u.narrow[34]; \
		kc->u.narrow[35] = ~kc->u.narrow[35]; \
		kc->u.narrow[40] = ~kc->u.narrow[40]; \
		kc->u.narrow[41] = ~kc->u.narrow[41]; \
		/* un-interleave */ \
		for (j = 0; j < 50; j += 2) \
			UNINTERLEAVE(kc->u.narrow[j], kc->u.narrow[j + 1]); \
		for (j = 0; j < d; j += 4) \
			sph_enc32le_aligned(u.tmp + j, kc->u.narrow[j >> 2]); \
		memcpy(dst, u.tmp, d); \
		keccak_init(kc, (unsigned)d << 3); \
	} \

#endif

DEFCLOSE(28, 144)
DEFCLOSE(32, 136)
DEFCLOSE(48, 104)
DEFCLOSE(64, 72)

/* see sph_keccak.h */
void
sph_keccak224_init(void *cc)
{
	keccak_init(cc, 224);
}

/* see sph_keccak.h */
void
sph_keccak224(void *cc, const void *data, size_t len)
{
	keccak_core(cc, data, len, 144);
}

/* see sph_keccak.h */
void
sph_keccak224_close(void *cc, void *dst)
{
	sph_keccak224_addbits_and_close(cc, 0, 0, dst);
}

/* see sph_keccak.h */
void
sph_keccak224_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	keccak_close28(cc, ub, n, dst);
}

/* see sph_keccak.h */
void
sph_keccak256_init(void *cc)
{
	keccak_init(cc, 256);
}

/* see sph_keccak.h */
void
sph_keccak256(void *cc, const void *data, size_t len)
{
	keccak_core(cc, data, len, 136);
}

/* see sph_keccak.h */
void
sph_keccak256_close(void *cc, void *dst)
{
	sph_keccak256_addbits_and_close(cc, 0, 0, dst);
}

/* see sph_keccak.h */
void
sph_keccak256_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	keccak_close32(cc, ub, n, dst);
}

/* see sph_keccak.h */
void
sph_keccak384_init(void *cc)
{
	keccak_init(cc, 384);
}

/* see sph_keccak.h */
void
sph_keccak384(void *cc, const void *data, size_t len)
{
	keccak_core(cc, data, len, 104);
}

/* see sph_keccak.h */
void
sph_keccak384_close(void *cc, void *dst)
{
	sph_keccak384_addbits_and_close(cc, 0, 0, dst);
}

/* see sph_keccak.h */
void
sph_keccak384_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	keccak_close48(cc, ub, n, dst);
}

/* see sph_keccak.h */
void
sph_keccak512_init(void *cc)
{
	keccak_init(cc, 512);
}

/* see sph_keccak.h */
void
sph_keccak512(void *cc, const void *data, size_t len)
{
	keccak_core(cc, data, len, 72);
}

/* see sph_keccak.h */
void
sph_keccak512_close(void *cc, void *dst)
{
	sph_keccak512_addbits_and_close(cc, 0, 0, dst);
}

/* see sph_keccak.h */
void
sph_keccak512_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	keccak_close64(cc, ub, n, dst);
}


#ifdef __cplusplus
}
#endif
