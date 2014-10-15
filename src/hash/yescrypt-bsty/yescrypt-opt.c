/*-
 * Copyright 2009 Colin Percival
 * Copyright 2013,2014 Alexander Peslyak
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file was originally written by Colin Percival as part of the Tarsnap
 * online backup system.
 */

#ifdef __i386__
#warning "This implementation does not use SIMD, and thus it runs a lot slower than the SIMD-enabled implementation. Enable at least SSE2 in the C compiler and use yescrypt-best.c instead unless you're building this SIMD-less implementation on purpose (portability to older CPUs or testing)."
#elif defined(__x86_64__)
#warning "This implementation does not use SIMD, and thus it runs a lot slower than the SIMD-enabled implementation. Use yescrypt-best.c instead unless you're building this SIMD-less implementation on purpose (for testing only)."
#endif

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include "sha256_Y.h"
#include "sysendian.h"

#include "yescrypt-platform.c"

static inline void
blkcpy(uint64_t * dest, const uint64_t * src, size_t count)
{
	do {
		*dest++ = *src++; *dest++ = *src++;
		*dest++ = *src++; *dest++ = *src++;
	} while (count -= 4);
}

static inline void
blkxor(uint64_t * dest, const uint64_t * src, size_t count)
{
	do {
		*dest++ ^= *src++; *dest++ ^= *src++;
		*dest++ ^= *src++; *dest++ ^= *src++;
	} while (count -= 4);
}

typedef union {
	uint32_t w[16];
	uint64_t d[8];
} salsa20_blk_t;

static inline void
salsa20_simd_shuffle(const salsa20_blk_t * Bin, salsa20_blk_t * Bout)
{
#define COMBINE(out, in1, in2) \
	Bout->d[out] = Bin->w[in1 * 2] | ((uint64_t)Bin->w[in2 * 2 + 1] << 32);
	COMBINE(0, 0, 2)
	COMBINE(1, 5, 7)
	COMBINE(2, 2, 4)
	COMBINE(3, 7, 1)
	COMBINE(4, 4, 6)
	COMBINE(5, 1, 3)
	COMBINE(6, 6, 0)
	COMBINE(7, 3, 5)
#undef COMBINE
}

static inline void
salsa20_simd_unshuffle(const salsa20_blk_t * Bin, salsa20_blk_t * Bout)
{
#define COMBINE(out, in1, in2) \
	Bout->w[out * 2] = Bin->d[in1]; \
	Bout->w[out * 2 + 1] = Bin->d[in2] >> 32;
	COMBINE(0, 0, 6)
	COMBINE(1, 5, 3)
	COMBINE(2, 2, 0)
	COMBINE(3, 7, 5)
	COMBINE(4, 4, 2)
	COMBINE(5, 1, 7)
	COMBINE(6, 6, 4)
	COMBINE(7, 3, 1)
#undef COMBINE
}

/**
 * salsa20_8(B):
 * Apply the salsa20/8 core to the provided block.
 */
static void
salsa20_8(uint64_t B[8])
{
	size_t i;
	salsa20_blk_t X;
#define x X.w

	salsa20_simd_unshuffle((const salsa20_blk_t *)B, &X);

	for (i = 0; i < 8; i += 2) {
#define R(a,b) (((a) << (b)) | ((a) >> (32 - (b))))
		/* Operate on columns */
		x[ 4] ^= R(x[ 0]+x[12], 7);  x[ 8] ^= R(x[ 4]+x[ 0], 9);
		x[12] ^= R(x[ 8]+x[ 4],13);  x[ 0] ^= R(x[12]+x[ 8],18);

		x[ 9] ^= R(x[ 5]+x[ 1], 7);  x[13] ^= R(x[ 9]+x[ 5], 9);
		x[ 1] ^= R(x[13]+x[ 9],13);  x[ 5] ^= R(x[ 1]+x[13],18);

		x[14] ^= R(x[10]+x[ 6], 7);  x[ 2] ^= R(x[14]+x[10], 9);
		x[ 6] ^= R(x[ 2]+x[14],13);  x[10] ^= R(x[ 6]+x[ 2],18);

		x[ 3] ^= R(x[15]+x[11], 7);  x[ 7] ^= R(x[ 3]+x[15], 9);
		x[11] ^= R(x[ 7]+x[ 3],13);  x[15] ^= R(x[11]+x[ 7],18);

		/* Operate on rows */
		x[ 1] ^= R(x[ 0]+x[ 3], 7);  x[ 2] ^= R(x[ 1]+x[ 0], 9);
		x[ 3] ^= R(x[ 2]+x[ 1],13);  x[ 0] ^= R(x[ 3]+x[ 2],18);

		x[ 6] ^= R(x[ 5]+x[ 4], 7);  x[ 7] ^= R(x[ 6]+x[ 5], 9);
		x[ 4] ^= R(x[ 7]+x[ 6],13);  x[ 5] ^= R(x[ 4]+x[ 7],18);

		x[11] ^= R(x[10]+x[ 9], 7);  x[ 8] ^= R(x[11]+x[10], 9);
		x[ 9] ^= R(x[ 8]+x[11],13);  x[10] ^= R(x[ 9]+x[ 8],18);

		x[12] ^= R(x[15]+x[14], 7);  x[13] ^= R(x[12]+x[15], 9);
		x[14] ^= R(x[13]+x[12],13);  x[15] ^= R(x[14]+x[13],18);
#undef R
	}
#undef x

	{
		salsa20_blk_t Y;
		salsa20_simd_shuffle(&X, &Y);
		for (i = 0; i < 16; i += 4) {
			((salsa20_blk_t *)B)->w[i] += Y.w[i];
			((salsa20_blk_t *)B)->w[i + 1] += Y.w[i + 1];
			((salsa20_blk_t *)B)->w[i + 2] += Y.w[i + 2];
			((salsa20_blk_t *)B)->w[i + 3] += Y.w[i + 3];
		}
	}
}

/**
 * blockmix_salsa8(Bin, Bout, X, r):
 * Compute Bout = BlockMix_{salsa20/8, r}(Bin).  The input Bin must be 128r
 * bytes in length; the output Bout must also be the same size.  The
 * temporary space X must be 64 bytes.
 */
static void
blockmix_salsa8(const uint64_t * Bin, uint64_t * Bout, uint64_t * X, size_t r)
{
	size_t i;

	/* 1: X <-- B_{2r - 1} */
	blkcpy(X, &Bin[(2 * r - 1) * 8], 8);

	/* 2: for i = 0 to 2r - 1 do */
	for (i = 0; i < 2 * r; i += 2) {
		/* 3: X <-- H(X \xor B_i) */
		blkxor(X, &Bin[i * 8], 8);
		salsa20_8(X);

		/* 4: Y_i <-- X */
		/* 6: B' <-- (Y_0, Y_2 ... Y_{2r-2}, Y_1, Y_3 ... Y_{2r-1}) */
		blkcpy(&Bout[i * 4], X, 8);

		/* 3: X <-- H(X \xor B_i) */
		blkxor(X, &Bin[i * 8 + 8], 8);
		salsa20_8(X);

		/* 4: Y_i <-- X */
		/* 6: B' <-- (Y_0, Y_2 ... Y_{2r-2}, Y_1, Y_3 ... Y_{2r-1}) */
		blkcpy(&Bout[i * 4 + r * 8], X, 8);
	}
}

/* These are tunable */
#define S_BITS 8
#define S_SIMD 2
#define S_P 4
#define S_ROUNDS 6

/* Number of S-boxes.  Not tunable, hard-coded in a few places. */
#define S_N 2

/* Derived values.  Not tunable on their own. */
#define S_SIZE1 (1 << S_BITS)
#define S_MASK ((S_SIZE1 - 1) * S_SIMD * 8)
#define S_MASK2 (((uint64_t)S_MASK << 32) | S_MASK)
#define S_SIZE_ALL (S_N * S_SIZE1 * S_SIMD)
#define S_P_SIZE (S_P * S_SIMD)
#define S_MIN_R ((S_P * S_SIMD + 15) / 16)

/**
 * pwxform(B):
 * Transform the provided block using the provided S-boxes.
 */
static void
block_pwxform(uint64_t * B, const uint64_t * S)
{
	uint64_t (*X)[S_SIMD] = (uint64_t (*)[S_SIMD])B;
	const uint8_t *S0 = (const uint8_t *)S;
	const uint8_t *S1 = (const uint8_t *)(S + S_SIZE1 * S_SIMD);
	size_t i, j;
#if S_SIMD > 2
	size_t k;
#endif

	for (j = 0; j < S_P; j++) {
		uint64_t *Xj = X[j];
		uint64_t x0 = Xj[0];
#if S_SIMD > 1
		uint64_t x1 = Xj[1];
#endif

		for (i = 0; i < S_ROUNDS; i++) {
			uint64_t x = x0 & S_MASK2;
			const uint64_t *p0, *p1;

			p0 = (const uint64_t *)(S0 + (uint32_t)x);
			p1 = (const uint64_t *)(S1 + (x >> 32));

			x0 = (uint64_t)(x0 >> 32) * (uint32_t)x0;
			x0 += p0[0];
			x0 ^= p1[0];

#if S_SIMD > 1
			x1 = (uint64_t)(x1 >> 32) * (uint32_t)x1;
			x1 += p0[1];
			x1 ^= p1[1];
#endif

#if S_SIMD > 2
			for (k = 2; k < S_SIMD; k++) {
				x = Xj[k];

				x = (uint64_t)(x >> 32) * (uint32_t)x;
				x += p0[k];
				x ^= p1[k];

				Xj[k] = x;
			}
#endif
		}

		Xj[0] = x0;
#if S_SIMD > 1
		Xj[1] = x1;
#endif
	}
}

/**
 * blockmix_pwxform(Bin, Bout, S, r):
 * Compute Bout = BlockMix_pwxform{salsa20/8, S, r}(Bin).  The input Bin must
 * be 128r bytes in length; the output Bout must also be the same size.
 *
 * S lacks const qualifier to match blockmix_salsa8()'s prototype, which we
 * need to refer to both functions via the same function pointers.
 */
static void
blockmix_pwxform(const uint64_t * Bin, uint64_t * Bout, uint64_t * S, size_t r)
{
	size_t r1, r2, i;

	/* Convert 128-byte blocks to (S_P_SIZE * 64-bit) blocks */
	r1 = r * 128 / (S_P_SIZE * 8);

	/* X <-- B_{r1 - 1} */
	blkcpy(Bout, &Bin[(r1 - 1) * S_P_SIZE], S_P_SIZE);

	/* X <-- X \xor B_i */
	blkxor(Bout, Bin, S_P_SIZE);

	/* X <-- H'(X) */
	/* B'_i <-- X */
	block_pwxform(Bout, S);

	/* for i = 0 to r1 - 1 do */
	for (i = 1; i < r1; i++) {
		/* X <-- X \xor B_i */
		blkcpy(&Bout[i * S_P_SIZE], &Bout[(i - 1) * S_P_SIZE],
		    S_P_SIZE);
		blkxor(&Bout[i * S_P_SIZE], &Bin[i * S_P_SIZE], S_P_SIZE);

		/* X <-- H'(X) */
		/* B'_i <-- X */
		block_pwxform(&Bout[i * S_P_SIZE], S);
	}

	/* Handle partial blocks */
	if (i * S_P_SIZE < r * 16)
		blkcpy(&Bout[i * S_P_SIZE], &Bin[i * S_P_SIZE],
		    r * 16 - i * S_P_SIZE);

	i = (r1 - 1) * S_P_SIZE / 8;
	/* Convert 128-byte blocks to 64-byte blocks */
	r2 = r * 2;

	/* B'_i <-- H(B'_i) */
	salsa20_8(&Bout[i * 8]);
	i++;

	for (; i < r2; i++) {
		/* B'_i <-- H(B'_i \xor B'_{i-1}) */
		blkxor(&Bout[i * 8], &Bout[(i - 1) * 8], 8);
		salsa20_8(&Bout[i * 8]);
	}
}

/**
 * integerify(B, r):
 * Return the result of parsing B_{2r-1} as a little-endian integer.
 */
static inline uint64_t
integerify(const uint64_t * B, size_t r)
{
/*
 * Our 64-bit words are in host byte order, and word 6 holds the second 32-bit
 * word of B_{2r-1} due to SIMD shuffling.  The 64-bit value we return is also
 * in host byte order, as it should be.
 */
	const uint64_t * X = &B[(2 * r - 1) * 8];
	uint32_t lo = X[0];
	uint32_t hi = X[6] >> 32;
	return ((uint64_t)hi << 32) + lo;
}

/**
 * smix1(B, r, N, flags, V, NROM, shared, XY, S):
 * Compute first loop of B = SMix_r(B, N).  The input B must be 128r bytes in
 * length; the temporary storage V must be 128rN bytes in length; the temporary
 * storage XY must be 256r + 64 bytes in length.  The value N must be even and
 * no smaller than 2.
 */
static void
smix1(uint64_t * B, size_t r, uint64_t N, yescrypt_flags_t flags,
    uint64_t * V, uint64_t NROM, const yescrypt_shared_t * shared,
    uint64_t * XY, uint64_t * S)
{
	void (*blockmix)(const uint64_t *, uint64_t *, uint64_t *, size_t) =
	    (S ? blockmix_pwxform : blockmix_salsa8);
	const uint64_t * VROM = shared->shared1.aligned;
	uint32_t VROM_mask = shared->mask1;
	size_t s = 16 * r;
	uint64_t * X = V;
	uint64_t * Y = &XY[s];
	uint64_t * Z = S ? S : &XY[2 * s];
	uint64_t n, i, j;
	size_t k;

	/* 1: X <-- B */
	/* 3: V_i <-- X */
	for (i = 0; i < 2 * r; i++) {
		const salsa20_blk_t *src = (const salsa20_blk_t *)&B[i * 8];
		salsa20_blk_t *tmp = (salsa20_blk_t *)Y;
		salsa20_blk_t *dst = (salsa20_blk_t *)&X[i * 8];
		for (k = 0; k < 16; k++)
			tmp->w[k] = le32dec(&src->w[k]);
		salsa20_simd_shuffle(tmp, dst);
	}

	/* 4: X <-- H(X) */
	/* 3: V_i <-- X */
	blockmix(X, Y, Z, r);
	blkcpy(&V[s], Y, s);

	X = XY;

	if (NROM && (VROM_mask & 1)) {
		if ((1 & VROM_mask) == 1) {
			/* j <-- Integerify(X) mod NROM */
			j = integerify(Y, r) & (NROM - 1);

			/* X <-- H(X \xor VROM_j) */
			blkxor(Y, &VROM[j * s], s);
		}

		blockmix(Y, X, Z, r);

		/* 2: for i = 0 to N - 1 do */
		for (n = 1, i = 2; i < N; i += 2) {
			/* 3: V_i <-- X */
			blkcpy(&V[i * s], X, s);

			if ((i & (i - 1)) == 0)
				n <<= 1;

			/* j <-- Wrap(Integerify(X), i) */
			j = integerify(X, r) & (n - 1);
			j += i - n;

			/* X <-- X \xor V_j */
			blkxor(X, &V[j * s], s);

			/* 4: X <-- H(X) */
			blockmix(X, Y, Z, r);

			/* 3: V_i <-- X */
			blkcpy(&V[(i + 1) * s], Y, s);

			j = integerify(Y, r);
			if (((i + 1) & VROM_mask) == 1) {
				/* j <-- Integerify(X) mod NROM */
				j &= NROM - 1;

				/* X <-- H(X \xor VROM_j) */
				blkxor(Y, &VROM[j * s], s);
			} else {
				/* j <-- Wrap(Integerify(X), i) */
				j &= n - 1;
				j += i + 1 - n;

				/* X <-- H(X \xor V_j) */
				blkxor(Y, &V[j * s], s);
			}

			blockmix(Y, X, Z, r);
		}
	} else {
		yescrypt_flags_t rw = flags & YESCRYPT_RW;

		/* 4: X <-- H(X) */
		blockmix(Y, X, Z, r);

		/* 2: for i = 0 to N - 1 do */
		for (n = 1, i = 2; i < N; i += 2) {
			/* 3: V_i <-- X */
			blkcpy(&V[i * s], X, s);

			if (rw) {
				if ((i & (i - 1)) == 0)
					n <<= 1;

				/* j <-- Wrap(Integerify(X), i) */
				j = integerify(X, r) & (n - 1);
				j += i - n;

				/* X <-- X \xor V_j */
				blkxor(X, &V[j * s], s);
			}

			/* 4: X <-- H(X) */
			blockmix(X, Y, Z, r);

			/* 3: V_i <-- X */
			blkcpy(&V[(i + 1) * s], Y, s);

			if (rw) {
				/* j <-- Wrap(Integerify(X), i) */
				j = integerify(Y, r) & (n - 1);
				j += (i + 1) - n;

				/* X <-- X \xor V_j */
				blkxor(Y, &V[j * s], s);
			}

			/* 4: X <-- H(X) */
			blockmix(Y, X, Z, r);
		}
	}

	/* B' <-- X */
	for (i = 0; i < 2 * r; i++) {
		const salsa20_blk_t *src = (const salsa20_blk_t *)&X[i * 8];
		salsa20_blk_t *tmp = (salsa20_blk_t *)Y;
		salsa20_blk_t *dst = (salsa20_blk_t *)&B[i * 8];
		for (k = 0; k < 16; k++)
			le32enc(&tmp->w[k], src->w[k]);
		salsa20_simd_unshuffle(tmp, dst);
	}
}

/**
 * smix2(B, r, N, Nloop, flags, V, NROM, shared, XY, S):
 * Compute second loop of B = SMix_r(B, N).  The input B must be 128r bytes in
 * length; the temporary storage V must be 128rN bytes in length; the temporary
 * storage XY must be 256r + 64 bytes in length.  The value N must be a
 * power of 2 greater than 1.  The value Nloop must be even.
 */
static void
smix2(uint64_t * B, size_t r, uint64_t N, uint64_t Nloop,
    yescrypt_flags_t flags,
    uint64_t * V, uint64_t NROM, const yescrypt_shared_t * shared,
    uint64_t * XY, uint64_t * S)
{
	void (*blockmix)(const uint64_t *, uint64_t *, uint64_t *, size_t) =
	    (S ? blockmix_pwxform : blockmix_salsa8);
	const uint64_t * VROM = shared->shared1.aligned;
	uint32_t VROM_mask = shared->mask1 | 1;
	size_t s = 16 * r;
	yescrypt_flags_t rw = flags & YESCRYPT_RW;
	uint64_t * X = XY;
	uint64_t * Y = &XY[s];
	uint64_t * Z = S ? S : &XY[2 * s];
	uint64_t i, j;
	size_t k;

	if (Nloop == 0)
		return;

	/* X <-- B' */
	for (i = 0; i < 2 * r; i++) {
		const salsa20_blk_t *src = (const salsa20_blk_t *)&B[i * 8];
		salsa20_blk_t *tmp = (salsa20_blk_t *)Y;
		salsa20_blk_t *dst = (salsa20_blk_t *)&X[i * 8];
		for (k = 0; k < 16; k++)
			tmp->w[k] = le32dec(&src->w[k]);
		salsa20_simd_shuffle(tmp, dst);
	}

	if (NROM) {
		/* 6: for i = 0 to N - 1 do */
		for (i = 0; i < Nloop; i += 2) {
			/* 7: j <-- Integerify(X) mod N */
			j = integerify(X, r) & (N - 1);

			/* 8: X <-- H(X \xor V_j) */
			blkxor(X, &V[j * s], s);
			/* V_j <-- Xprev \xor V_j */
			if (rw)
				blkcpy(&V[j * s], X, s);
			blockmix(X, Y, Z, r);

			j = integerify(Y, r);
			if (((i + 1) & VROM_mask) == 1) {
				/* j <-- Integerify(X) mod NROM */
				j &= NROM - 1;

				/* X <-- H(X \xor VROM_j) */
				blkxor(Y, &VROM[j * s], s);
			} else {
				/* 7: j <-- Integerify(X) mod N */
				j &= N - 1;

				/* 8: X <-- H(X \xor V_j) */
				blkxor(Y, &V[j * s], s);
				/* V_j <-- Xprev \xor V_j */
				if (rw)
					blkcpy(&V[j * s], Y, s);
			}

			blockmix(Y, X, Z, r);
		}
	} else {
		/* 6: for i = 0 to N - 1 do */
		i = Nloop / 2;
		do {
			/* 7: j <-- Integerify(X) mod N */
			j = integerify(X, r) & (N - 1);

			/* 8: X <-- H(X \xor V_j) */
			blkxor(X, &V[j * s], s);
			/* V_j <-- Xprev \xor V_j */
			if (rw)
				blkcpy(&V[j * s], X, s);
			blockmix(X, Y, Z, r);

			/* 7: j <-- Integerify(X) mod N */
			j = integerify(Y, r) & (N - 1);

			/* 8: X <-- H(X \xor V_j) */
			blkxor(Y, &V[j * s], s);
			/* V_j <-- Xprev \xor V_j */
			if (rw)
				blkcpy(&V[j * s], Y, s);
			blockmix(Y, X, Z, r);
		} while (--i);
	}

	/* 10: B' <-- X */
	for (i = 0; i < 2 * r; i++) {
		const salsa20_blk_t *src = (const salsa20_blk_t *)&X[i * 8];
		salsa20_blk_t *tmp = (salsa20_blk_t *)Y;
		salsa20_blk_t *dst = (salsa20_blk_t *)&B[i * 8];
		for (k = 0; k < 16; k++)
			le32enc(&tmp->w[k], src->w[k]);
		salsa20_simd_unshuffle(tmp, dst);
	}
}

/**
 * p2floor(x):
 * Largest power of 2 not greater than argument.
 */
static uint64_t
p2floor(uint64_t x)
{
	uint64_t y;
	while ((y = x & (x - 1)))
		x = y;
	return x;
}

/**
 * smix(B, r, N, p, t, flags, V, NROM, shared, XY, S):
 * Compute B = SMix_r(B, N).  The input B must be 128rp bytes in length; the
 * temporary storage V must be 128rN bytes in length; the temporary storage
 * XY must be 256r+64 or (256r+64)*p bytes in length (the larger size is
 * required with OpenMP-enabled builds).  The value N must be a power of 2
 * greater than 1.
 */
static void
smix(uint64_t * B, size_t r, uint64_t N, uint32_t p, uint32_t t,
    yescrypt_flags_t flags,
    uint64_t * V, uint64_t NROM, const yescrypt_shared_t * shared,
    uint64_t * XY, uint64_t * S)
{
	size_t s = 16 * r;
	uint64_t Nchunk = N / p, Nloop_all, Nloop_rw;
	uint32_t i;

	Nloop_all = Nchunk;
	if (flags & YESCRYPT_RW) {
		if (t <= 1) {
			if (t)
				Nloop_all *= 2; /* 2/3 */
			Nloop_all = (Nloop_all + 2) / 3; /* 1/3, round up */
		} else {
			Nloop_all *= t - 1;
		}
	} else if (t) {
		if (t == 1)
			Nloop_all += (Nloop_all + 1) / 2; /* 1.5, round up */
		Nloop_all *= t;
	}

	Nloop_rw = 0;
	if (flags & __YESCRYPT_INIT_SHARED)
		Nloop_rw = Nloop_all;
	else if (flags & YESCRYPT_RW)
		Nloop_rw = Nloop_all / p;

	Nchunk &= ~(uint64_t)1; /* round down to even */
	Nloop_all++; Nloop_all &= ~(uint64_t)1; /* round up to even */
	Nloop_rw &= ~(uint64_t)1; /* round down to even */

#ifdef _OPENMP
#pragma omp parallel if (p > 1) default(none) private(i) shared(B, r, N, p, flags, V, NROM, shared, XY, S, s, Nchunk, Nloop_all, Nloop_rw)
	{
#pragma omp for
#endif
	for (i = 0; i < p; i++) {
		uint64_t Vchunk = i * Nchunk;
		uint64_t * Bp = &B[i * s];
		uint64_t * Vp = &V[Vchunk * s];
#ifdef _OPENMP
		uint64_t * XYp = &XY[i * (2 * s + 8)];
#else
		uint64_t * XYp = XY;
#endif
		uint64_t Np = (i < p - 1) ? Nchunk : (N - Vchunk);
		uint64_t * Sp = S ? &S[i * S_SIZE_ALL] : S;
		if (Sp)
			smix1(Bp, 1, S_SIZE_ALL / 16,
			    flags & ~YESCRYPT_PWXFORM,
			    Sp, NROM, shared, XYp, NULL);
		if (!(flags & __YESCRYPT_INIT_SHARED_2))
			smix1(Bp, r, Np, flags, Vp, NROM, shared, XYp, Sp);
		smix2(Bp, r, p2floor(Np), Nloop_rw, flags, Vp,
		    NROM, shared, XYp, Sp);
	}

	if (Nloop_all > Nloop_rw) {
#ifdef _OPENMP
#pragma omp for
#endif
		for (i = 0; i < p; i++) {
			uint64_t * Bp = &B[i * s];
#ifdef _OPENMP
			uint64_t * XYp = &XY[i * (2 * s + 8)];
#else
			uint64_t * XYp = XY;
#endif
			uint64_t * Sp = S ? &S[i * S_SIZE_ALL] : S;
			smix2(Bp, r, N, Nloop_all - Nloop_rw,
			    flags & ~YESCRYPT_RW, V, NROM, shared, XYp, Sp);
		}
	}
#ifdef _OPENMP
	}
#endif
}

/**
 * yescrypt_kdf(shared, local, passwd, passwdlen, salt, saltlen,
 *     N, r, p, t, flags, buf, buflen):
 * Compute scrypt(passwd[0 .. passwdlen - 1], salt[0 .. saltlen - 1], N, r,
 * p, buflen), or a revision of scrypt as requested by flags and shared, and
 * write the result into buf.  The parameters r, p, and buflen must satisfy
 * r * p < 2^30 and buflen <= (2^32 - 1) * 32.  The parameter N must be a power
 * of 2 greater than 1.
 *
 * t controls computation time while not affecting peak memory usage.  shared
 * and flags may request special modes as described in yescrypt.h.  local is
 * the thread-local data structure, allowing to preserve and reuse a memory
 * allocation across calls, thereby reducing its overhead.
 *
 * Return 0 on success; or -1 on error.
 */
int
yescrypt_kdf(const yescrypt_shared_t * shared, yescrypt_local_t * local,
    const uint8_t * passwd, size_t passwdlen,
    const uint8_t * salt, size_t saltlen,
    uint64_t N, uint32_t r, uint32_t p, uint32_t t, yescrypt_flags_t flags,
    uint8_t * buf, size_t buflen)
{
	yescrypt_region_t tmp;
	uint64_t NROM;
	size_t B_size, V_size, XY_size, need;
	uint64_t * B, * V, * XY, * S;
	uint64_t sha256[4];

	/*
	 * YESCRYPT_PARALLEL_SMIX is a no-op at p = 1 for its intended purpose,
	 * so don't let it have side-effects.  Without this adjustment, it'd
	 * enable the SHA-256 password pre-hashing and output post-hashing,
	 * because any deviation from classic scrypt implies those.
	 */
	if (p == 1)
		flags &= ~YESCRYPT_PARALLEL_SMIX;

	/* Sanity-check parameters */
	if (flags & ~YESCRYPT_KNOWN_FLAGS) {
		errno = EINVAL;
		return -1;
	}
#if SIZE_MAX > UINT32_MAX
	if (buflen > (((uint64_t)(1) << 32) - 1) * 32) {
		errno = EFBIG;
		return -1;
	}
#endif
	if ((uint64_t)(r) * (uint64_t)(p) >= (1 << 30)) {
		errno = EFBIG;
		return -1;
	}
	if (((N & (N - 1)) != 0) || (N <= 1) || (r < 1) || (p < 1)) {
		errno = EINVAL;
		return -1;
	}
	if ((flags & YESCRYPT_PARALLEL_SMIX) && (N / p <= 1)) {
		errno = EINVAL;
		return -1;
	}
#if S_MIN_R > 1
	if ((flags & YESCRYPT_PWXFORM) && (r < S_MIN_R)) {
		errno = EINVAL;
		return -1;
	}
#endif
	if ((p > SIZE_MAX / ((size_t)256 * r + 64)) ||
#if SIZE_MAX / 256 <= UINT32_MAX
	    (r > SIZE_MAX / 256) ||
#endif
	    (N > SIZE_MAX / 128 / r)) {
		errno = ENOMEM;
		return -1;
	}
	if (N > UINT64_MAX / ((uint64_t)t + 1)) {
		errno = EFBIG;
		return -1;
	}
#ifdef _OPENMP
	if (!(flags & YESCRYPT_PARALLEL_SMIX) &&
	    (N > SIZE_MAX / 128 / (r * p))) {
		errno = ENOMEM;
		return -1;
	}
#endif
	if ((flags & YESCRYPT_PWXFORM) &&
#ifndef _OPENMP
	    (flags & YESCRYPT_PARALLEL_SMIX) &&
#endif
	    p > SIZE_MAX / (S_SIZE_ALL * sizeof(*S))) {
		errno = ENOMEM;
		return -1;
	}

	NROM = 0;
	if (shared->shared1.aligned) {
		NROM = shared->shared1.aligned_size / ((size_t)128 * r);
		if (((NROM & (NROM - 1)) != 0) || (NROM <= 1) ||
		    !(flags & YESCRYPT_RW)) {
			errno = EINVAL;
			return -1;
		}
	}

	/* Allocate memory */
	V = NULL;
	V_size = (size_t)128 * r * N;
#ifdef _OPENMP
	if (!(flags & YESCRYPT_PARALLEL_SMIX))
		V_size *= p;
#endif
	need = V_size;
	if (flags & __YESCRYPT_INIT_SHARED) {
		if (local->aligned_size < need) {
			if (local->base || local->aligned ||
			    local->base_size || local->aligned_size) {
				errno = EINVAL;
				return -1;
			}
			if (!alloc_region(local, need))
				return -1;
		}
		V = (uint64_t *)local->aligned;
		need = 0;
	}
	B_size = (size_t)128 * r * p;
	need += B_size;
	if (need < B_size) {
		errno = ENOMEM;
		return -1;
	}
	XY_size = (size_t)256 * r + 64;
#ifdef _OPENMP
	XY_size *= p;
#endif
	need += XY_size;
	if (need < XY_size) {
		errno = ENOMEM;
		return -1;
	}
	if (flags & YESCRYPT_PWXFORM) {
		size_t S_size = S_SIZE_ALL * sizeof(*S);
#ifdef _OPENMP
		S_size *= p;
#else
		if (flags & YESCRYPT_PARALLEL_SMIX)
			S_size *= p;
#endif
		need += S_size;
		if (need < S_size) {
			errno = ENOMEM;
			return -1;
		}
	}
	if (flags & __YESCRYPT_INIT_SHARED) {
		if (!alloc_region(&tmp, need))
			return -1;
		B = (uint64_t *)tmp.aligned;
		XY = (uint64_t *)((uint8_t *)B + B_size);
	} else {
		init_region(&tmp);
		if (local->aligned_size < need) {
			if (free_region(local))
				return -1;
			if (!alloc_region(local, need))
				return -1;
		}
		B = (uint64_t *)local->aligned;
		V = (uint64_t *)((uint8_t *)B + B_size);
		XY = (uint64_t *)((uint8_t *)V + V_size);
	}
	S = NULL;
	if (flags & YESCRYPT_PWXFORM)
		S = (uint64_t *)((uint8_t *)XY + XY_size);

	if (t || flags) {
		SHA256_CTX_Y ctx;
		SHA256_Init_Y(&ctx);
		SHA256_Update_Y(&ctx, passwd, passwdlen);
		SHA256_Final_Y((uint8_t *)sha256, &ctx);
		passwd = (uint8_t *)sha256;
		passwdlen = sizeof(sha256);
	}

	/* 1: (B_0 ... B_{p-1}) <-- PBKDF2(P, S, 1, p * MFLen) */
	PBKDF2_SHA256(passwd, passwdlen, salt, saltlen, 1,
	    (uint8_t *)B, B_size);

	if (t || flags)
		blkcpy(sha256, B, sizeof(sha256) / sizeof(sha256[0]));

	if (p == 1 || (flags & YESCRYPT_PARALLEL_SMIX)) {
		smix(B, r, N, p, t, flags, V, NROM, shared, XY, S);
	} else {
		uint32_t i;

		/* 2: for i = 0 to p - 1 do */
#ifdef _OPENMP
#pragma omp parallel for default(none) private(i) shared(B, r, N, p, t, flags, V, NROM, shared, XY, S)
#endif
		for (i = 0; i < p; i++) {
			/* 3: B_i <-- MF(B_i, N) */
#ifdef _OPENMP
			smix(&B[(size_t)16 * r * i], r, N, 1, t, flags,
			    &V[(size_t)16 * r * i * N],
			    NROM, shared,
			    &XY[((size_t)32 * r + 8) * i],
			    S ? &S[S_SIZE_ALL * i] : S);
#else
			smix(&B[(size_t)16 * r * i], r, N, 1, t, flags, V,
			    NROM, shared, XY, S);
#endif
		}
	}

	/* 5: DK <-- PBKDF2(P, B, 1, dkLen) */
	PBKDF2_SHA256(passwd, passwdlen, (uint8_t *)B, B_size, 1, buf, buflen);

	/*
	 * Except when computing classic scrypt, allow all computation so far
	 * to be performed on the client.  The final steps below match those of
	 * SCRAM (RFC 5802), so that an extension of SCRAM (with the steps so
	 * far in place of SCRAM's use of PBKDF2 and with SHA-256 in place of
	 * SCRAM's use of SHA-1) would be usable with yescrypt hashes.
	 */
	if ((t || flags) && buflen == sizeof(sha256)) {
		/* Compute ClientKey */
		{
			HMAC_SHA256_CTX_Y ctx;
			HMAC_SHA256_Init_Y(&ctx, buf, buflen);
			HMAC_SHA256_Update_Y(&ctx, salt, saltlen);
			HMAC_SHA256_Final_Y((uint8_t *)sha256, &ctx);
		}
		/* Compute StoredKey */
		{
			SHA256_CTX_Y ctx;
			SHA256_Init_Y(&ctx);
			SHA256_Update_Y(&ctx, (uint8_t *)sha256, sizeof(sha256));
			SHA256_Final_Y(buf, &ctx);
		}
	}

	if (free_region(&tmp))
		return -1;

	/* Success! */
	return 0;
}
