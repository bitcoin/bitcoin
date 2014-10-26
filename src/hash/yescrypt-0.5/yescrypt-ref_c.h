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
 *
 * This is the reference implementation.  Its purpose is to provide a simple
 * human- and machine-readable specification that implementations intended
 * for actual use should be tested against.  It is deliberately mostly not
 * optimized, and it is not meant to be used in production.  Instead, use
 * yescrypt-best.c or one of the source files included from there.
 */

#warning "This reference implementation is deliberately mostly not optimized. Use yescrypt-best.c instead unless you're testing (against) the reference implementation on purpose."

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include "sha256.h"
#include "sysendian.h"

#include "yescrypt.h"

static void
blkcpy(uint32_t * dest, const uint32_t * src, size_t count)
{
	do {
		*dest++ = *src++;
	} while (--count);
}

static void
blkxor(uint32_t * dest, const uint32_t * src, size_t count)
{
	do {
		*dest++ ^= *src++;
	} while (--count);
}

/**
 * salsa20_8(B):
 * Apply the salsa20/8 core to the provided block.
 */
static void
salsa20_8(uint32_t B[16])
{
	uint32_t x[16];
	size_t i;

	/* Mimic SIMD shuffling */
	for (i = 0; i < 16; i++)
		x[i * 5 % 16] = B[i];

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

	/* Mimic SIMD shuffling */
	for (i = 0; i < 16; i++)
		B[i] += x[i * 5 % 16];
}

/**
 * blockmix_salsa8(B, Y, r):
 * Compute B = BlockMix_{salsa20/8, r}(B).  The input B must be 128r bytes in
 * length; the temporary space Y must also be the same size.
 */
static void
blockmix_salsa8(uint32_t * B, uint32_t * Y, size_t r)
{
	uint32_t X[16];
	size_t i;

	/* 1: X <-- B_{2r - 1} */
	blkcpy(X, &B[(2 * r - 1) * 16], 16);

	/* 2: for i = 0 to 2r - 1 do */
	for (i = 0; i < 2 * r; i++) {
		/* 3: X <-- H(X \xor B_i) */
		blkxor(X, &B[i * 16], 16);
		salsa20_8(X);

		/* 4: Y_i <-- X */
		blkcpy(&Y[i * 16], X, 16);
	}

	/* 6: B' <-- (Y_0, Y_2 ... Y_{2r-2}, Y_1, Y_3 ... Y_{2r-1}) */
	for (i = 0; i < r; i++)
		blkcpy(&B[i * 16], &Y[(i * 2) * 16], 16);
	for (i = 0; i < r; i++)
		blkcpy(&B[(i + r) * 16], &Y[(i * 2 + 1) * 16], 16);
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
#define S_SIZE_ALL (S_N * S_SIZE1 * S_SIMD * 2)
#define S_P_SIZE (S_P * S_SIMD * 2)
#define S_MIN_R ((S_P * S_SIMD + 15) / 16)

/**
 * pwxform(B):
 * Transform the provided block using the provided S-boxes.
 */
static void
block_pwxform(uint32_t * B, const uint32_t * S)
{
	uint32_t (*X)[S_SIMD][2] = (uint32_t (*)[S_SIMD][2])B;
	const uint32_t (*S0)[2] = (const uint32_t (*)[2])S;
	const uint32_t (*S1)[2] = S0 + S_SIZE1 * S_SIMD;
	size_t i, j, k;

	for (i = 0; i < S_ROUNDS; i++) {
		for (j = 0; j < S_P; j++) {
			uint32_t xl = X[j][0][0];
			uint32_t xh = X[j][0][1];
			const uint32_t (*p0)[2], (*p1)[2];

			p0 = S0 + (xl & S_MASK) / sizeof(*S0);
			p1 = S1 + (xh & S_MASK) / sizeof(*S1);

			for (k = 0; k < S_SIMD; k++) {
				uint64_t x, s0, s1;

				s0 = ((uint64_t)p0[k][1] << 32) + p0[k][0];
				s1 = ((uint64_t)p1[k][1] << 32) + p1[k][0];

				xl = X[j][k][0];
				xh = X[j][k][1];

				x = (uint64_t)xh * xl;
				x += s0;
				x ^= s1;

				X[j][k][0] = x;
				X[j][k][1] = x >> 32;
			}
		}
	}
}

/**
 * blockmix_pwxform(B, Y, S, r):
 * Compute B = BlockMix_pwxform{salsa20/8, S, r}(B).  The input B must be 128r
 * bytes in length; the temporary space Y must be at least S_P_SIZE*4 bytes.
 */
static void
blockmix_pwxform(uint32_t * B, uint32_t * Y, const uint32_t * S, size_t r)
{
	size_t r1, r2, i;

	/* Convert 128-byte blocks to (S_P_SIZE * 32-bit) blocks */
	r1 = r * 128 / (S_P_SIZE * 4);

	/* X <-- B_{r1 - 1} */
	blkcpy(Y, &B[(r1 - 1) * S_P_SIZE], S_P_SIZE);

	/* for i = 0 to r1 - 1 do */
	for (i = 0; i < r1; i++) {
		/* X <-- X \xor B_i */
		blkxor(Y, &B[i * S_P_SIZE], S_P_SIZE);

		/* X <-- H'(X) */
		block_pwxform(Y, S);

		/* B'_i <-- X */
		blkcpy(&B[i * S_P_SIZE], Y, S_P_SIZE);
	}

	i = (r1 - 1) * S_P_SIZE / 16;
	/* Convert 128-byte blocks to 64-byte blocks */
	r2 = r * 2;

	/* B_i <-- H(B_i) */
	salsa20_8(&B[i * 16]);
	i++;

	for (; i < r2; i++) {
		/* B_i <-- H(B_i \xor B_{i-1}) */
		blkxor(&B[i * 16], &B[(i - 1) * 16], 16);
		salsa20_8(&B[i * 16]);
	}
}

/**
 * integerify(B, r):
 * Return the result of parsing B_{2r-1} as a little-endian integer.
 */
static uint64_t
integerify(const uint32_t * B, size_t r)
{
/*
 * Our 32-bit words are in host byte order, and word 13 is the second word of
 * B_{2r-1} due to SIMD shuffling.  The 64-bit value we return is also in host
 * byte order, as it should be.
 */
	const uint32_t * X = &B[(2 * r - 1) * 16];
	return ((uint64_t)X[13] << 32) + X[0];
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
 * wrap(x, i):
 * Wrap x to the range 0 to i-1.
 */
static uint64_t
wrap(uint64_t x, uint64_t i)
{
	uint64_t n = p2floor(i);
	return (x & (n - 1)) + (i - n);
}

/**
 * smix1(B, r, N, flags, V, NROM, shared, XY, S):
 * Compute first loop of B = SMix_r(B, N).  The input B must be 128r bytes in
 * length; the temporary storage V must be 128rN bytes in length; the temporary
 * storage XY must be 256r bytes in length.
 */
static void
smix1(uint32_t * B, size_t r, uint64_t N, yescrypt_flags_t flags,
    uint32_t * V, uint64_t NROM, const yescrypt_shared_t * shared,
    uint32_t * XY, uint32_t * S)
{
	const uint32_t * VROM = shared->shared1.aligned;
	uint32_t VROM_mask = NROM ? shared->mask1 : 0;
	size_t s = 32 * r;
	uint32_t * X = XY;
	uint32_t * Y = &XY[s];
	uint64_t i, j;
	size_t k;

	/* 1: X <-- B */
	for (k = 0; k < 2 * r; k++)
		for (i = 0; i < 16; i++)
			X[k * 16 + i] = le32dec(&B[k * 16 + (i * 5 % 16)]);

	/* 2: for i = 0 to N - 1 do */
	for (i = 0; i < N; i++) {
		/* 3: V_i <-- X */
		blkcpy(&V[i * s], X, s);

		if ((i & VROM_mask) == 1) {
			/* j <-- Integerify(X) mod NROM */
			j = integerify(X, r) & (NROM - 1);

			/* X <-- H(X \xor VROM_j) */
			blkxor(X, &VROM[j * s], s);
		} else if ((flags & YESCRYPT_RW) && i > 1) {
			/* j <-- Wrap(Integerify(X), i) */
			j = wrap(integerify(X, r), i);

			/* X <-- X \xor V_j */
			blkxor(X, &V[j * s], s);
		}

		/* 4: X <-- H(X) */
		if (S)
			blockmix_pwxform(X, Y, S, r);
		else
			blockmix_salsa8(X, Y, r);
	}

	/* B' <-- X */
	for (k = 0; k < 2 * r; k++)
		for (i = 0; i < 16; i++)
			le32enc(&B[k * 16 + (i * 5 % 16)], X[k * 16 + i]);
}

/**
 * smix2(B, r, N, Nloop, flags, V, NROM, shared, XY, S):
 * Compute second loop of B = SMix_r(B, N).  The input B must be 128r bytes in
 * length; the temporary storage V must be 128rN bytes in length; the temporary
 * storage XY must be 256r bytes in length.  The value N must be a power of 2
 * greater than 1.
 */
static void
smix2(uint32_t * B, size_t r, uint64_t N, uint64_t Nloop,
    yescrypt_flags_t flags, uint32_t * V, uint64_t NROM,
    const yescrypt_shared_t * shared, uint32_t * XY, uint32_t * S)
{
	const uint32_t * VROM = shared->shared1.aligned;
	uint32_t VROM_mask = NROM ? (shared->mask1 | 1) : 0;
	size_t s = 32 * r;
	uint32_t * X = XY;
	uint32_t * Y = &XY[s];
	uint64_t i, j;
	size_t k;

	/* X <-- B */
	for (k = 0; k < 2 * r; k++)
		for (i = 0; i < 16; i++)
			X[k * 16 + i] = le32dec(&B[k * 16 + (i * 5 % 16)]);

	/* 6: for i = 0 to N - 1 do */
	for (i = 0; i < Nloop; i++) {
		if ((i & VROM_mask) == 1) {
			/* j <-- Integerify(X) mod NROM */
			j = integerify(X, r) & (NROM - 1);

			/* X <-- H(X \xor VROM_j) */
			blkxor(X, &VROM[j * s], s);
		} else {
			/* 7: j <-- Integerify(X) mod N */
			j = integerify(X, r) & (N - 1);

			/* 8.1: X <-- X \xor V_j */
			blkxor(X, &V[j * s], s);
			/* V_j <-- X */
			if (flags & YESCRYPT_RW)
				blkcpy(&V[j * s], X, s);
		}

		/* 8.2: X <-- H(X) */
		if (S)
			blockmix_pwxform(X, Y, S, r);
		else
			blockmix_salsa8(X, Y, r);
	}

	/* 10: B' <-- X */
	for (k = 0; k < 2 * r; k++)
		for (i = 0; i < 16; i++)
			le32enc(&B[k * 16 + (i * 5 % 16)], X[k * 16 + i]);
}

/**
 * smix(B, r, N, p, t, flags, V, NROM, shared, XY, S):
 * Compute B = SMix_r(B, N).  The input B must be 128rp bytes in length; the
 * temporary storage V must be 128rN bytes in length; the temporary storage
 * XY must be 256r bytes in length.  The value N must be a power of 2 greater
 * than 1.
 */
static void
smix(uint32_t * B, size_t r, uint64_t N, uint32_t p, uint32_t t,
    yescrypt_flags_t flags,
    uint32_t * V, uint64_t NROM, const yescrypt_shared_t * shared,
    uint32_t * XY, uint32_t * S)
{
	size_t s = 32 * r;
	uint64_t Vchunk = 0, Nchunk = N / p, Nloop_all, Nloop_rw;
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

	for (i = 0; i < p; i++) {
		uint32_t * Bp = &B[i * s];
		uint32_t * Vp = &V[Vchunk * s];
		uint64_t Np = (i < p - 1) ? Nchunk : (N - Vchunk);
		uint32_t * Sp = S ? &S[i * S_SIZE_ALL] : S;
		if (Sp)
			smix1(Bp, 1, S_SIZE_ALL / 32,
			    flags & ~YESCRYPT_PWXFORM,
			    Sp, NROM, shared, XY, NULL);
		if (!(flags & __YESCRYPT_INIT_SHARED_2))
			smix1(Bp, r, Np, flags, Vp, NROM, shared, XY, Sp);
		smix2(Bp, r, p2floor(Np), Nloop_rw, flags, Vp,
		    NROM, shared, XY, Sp);
		Vchunk += Nchunk;
	}

	for (i = 0; i < p; i++) {
		uint32_t * Bp = &B[i * s];
		uint32_t * Sp = S ? &S[i * S_SIZE_ALL] : NULL;
		smix2(Bp, r, N, Nloop_all - Nloop_rw, flags & ~YESCRYPT_RW,
		    V, NROM, shared, XY, Sp);
	}
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
 * the thread-local data structure, allowing optimized implementations to
 * preserve and reuse a memory allocation across calls, thereby reducing its
 * overhead (this reference implementation does not make that optimization).
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
	int retval = -1;
	uint64_t NROM;
	size_t B_size, V_size;
	uint32_t * B, * V, * XY, * S;
	uint32_t sha256[8];

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
	if ((r > SIZE_MAX / 128 / p) ||
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
	if (((flags & (YESCRYPT_PWXFORM | YESCRYPT_PARALLEL_SMIX)) ==
	    (YESCRYPT_PWXFORM | YESCRYPT_PARALLEL_SMIX)) &&
	    p > SIZE_MAX / (S_SIZE_ALL * sizeof(*S))) {
		errno = ENOMEM;
		return -1;
	}

	NROM = 0;
	if (shared->shared1.aligned) {
		NROM = shared->shared1.aligned_size / ((size_t)128 * r);
/*
 * This implementation could support NROM without YESCRYPT_RW as well, but we
 * currently don't want to make such support available so that it can be safely
 * excluded from optimized implementations (where it'd require extra code).
 */
		if (((NROM & (NROM - 1)) != 0) || (NROM <= 1) ||
		    !(flags & YESCRYPT_RW)) {
			errno = EINVAL;
			return -1;
		}
	}

	/* Allocate memory */
	V_size = (size_t)128 * r * N;
	if (flags & __YESCRYPT_INIT_SHARED) {
		V = (uint32_t *)local->aligned;
		if (local->aligned_size < V_size) {
			if (local->base || local->aligned ||
			    local->base_size || local->aligned_size) {
				errno = EINVAL;
				return -1;
			}
			if ((V = malloc(V_size)) == NULL)
				return -1;
			local->base = local->aligned = V;
			local->base_size = local->aligned_size = V_size;
		}
	} else {
		if ((V = malloc(V_size)) == NULL)
			return -1;
	}
	B_size = (size_t)128 * r * p;
	if ((B = malloc(B_size)) == NULL)
		goto free_V;
	if ((XY = malloc((size_t)256 * r)) == NULL)
		goto free_B;
	S = NULL;
	if (flags & YESCRYPT_PWXFORM) {
		size_t S_size = S_SIZE_ALL * sizeof(*S);
		if (flags & YESCRYPT_PARALLEL_SMIX)
			S_size *= p;
		if ((S = malloc(S_size)) == NULL)
			goto free_XY;
	}

	if (t || flags) {
		SHA256_CTX ctx;
		SHA256_Init(&ctx);
		SHA256_Update(&ctx, passwd, passwdlen);
		SHA256_Final((uint8_t *)sha256, &ctx);
		passwd = (uint8_t *)sha256;
		passwdlen = sizeof(sha256);
	}

	/* 1: (B_0 ... B_{p-1}) <-- PBKDF2(P, S, 1, p * MFLen) */
	PBKDF2_SHA256(passwd, passwdlen, salt, saltlen, 1,
	    (uint8_t *)B, B_size);

	if (t || flags)
		blkcpy(sha256, B, sizeof(sha256) / sizeof(sha256[0]));

	if (flags & YESCRYPT_PARALLEL_SMIX) {
		smix(B, r, N, p, t, flags, V, NROM, shared, XY, S);
	} else {
		uint32_t i;

		/* 2: for i = 0 to p - 1 do */
		for (i = 0; i < p; i++) {
			/* 3: B_i <-- MF(B_i, N) */
			smix(&B[(size_t)32 * r * i], r, N, 1, t, flags, V,
			    NROM, shared, XY, S);
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
			HMAC_SHA256_CTX ctx;
			HMAC_SHA256_Init(&ctx, buf, buflen);
			HMAC_SHA256_Update(&ctx, "Client Key", 10);
			HMAC_SHA256_Final((uint8_t *)sha256, &ctx);
		}
		/* Compute StoredKey */
		{
			SHA256_CTX ctx;
			SHA256_Init(&ctx);
			SHA256_Update(&ctx, (uint8_t *)sha256, sizeof(sha256));
			SHA256_Final(buf, &ctx);
		}
	}

	/* Success! */
	retval = 0;

	/* Free memory */
	free(S);
free_XY:
	free(XY);
free_B:
	free(B);
free_V:
	if (!(flags & __YESCRYPT_INIT_SHARED))
		free(V);

	return retval;
}

int
yescrypt_init_shared(yescrypt_shared_t * shared,
    const uint8_t * param, size_t paramlen,
    uint64_t N, uint32_t r, uint32_t p,
    yescrypt_init_shared_flags_t flags, uint32_t mask,
    uint8_t * buf, size_t buflen)
{
	yescrypt_shared1_t * shared1 = &shared->shared1;
	yescrypt_shared_t dummy, half1, half2;
	uint8_t salt[32];

	if (flags & YESCRYPT_SHARED_PREALLOCATED) {
		if (!shared1->aligned || !shared1->aligned_size)
			return -1;
	} else {
		shared1->base = shared1->aligned = NULL;
		shared1->base_size = shared1->aligned_size = 0;
	}
	shared->mask1 = 1;
	if (!param && !paramlen && !N && !r && !p && !buf && !buflen)
		return 0;

	dummy.shared1.base = dummy.shared1.aligned = NULL;
	dummy.shared1.base_size = dummy.shared1.aligned_size = 0;
	dummy.mask1 = 1;
	if (yescrypt_kdf(&dummy, shared1,
	    param, paramlen, NULL, 0, N, r, p, 0,
	    YESCRYPT_RW | YESCRYPT_PARALLEL_SMIX | __YESCRYPT_INIT_SHARED_1,
	    salt, sizeof(salt)))
		goto out;

	half1 = half2 = *shared;
	half1.shared1.aligned_size /= 2;
	half2.shared1.aligned += half1.shared1.aligned_size;
	half2.shared1.aligned_size = half1.shared1.aligned_size;
	N /= 2;

	if (p > 1 && yescrypt_kdf(&half1, &half2.shared1,
	    param, paramlen, salt, sizeof(salt), N, r, p, 0,
	    YESCRYPT_RW | YESCRYPT_PARALLEL_SMIX | __YESCRYPT_INIT_SHARED_2,
	    salt, sizeof(salt)))
		goto out;

	if (yescrypt_kdf(&half2, &half1.shared1,
	    param, paramlen, salt, sizeof(salt), N, r, p, 0,
	    YESCRYPT_RW | YESCRYPT_PARALLEL_SMIX | __YESCRYPT_INIT_SHARED_1,
	    salt, sizeof(salt)))
		goto out;

	if (yescrypt_kdf(&half1, &half2.shared1,
	    param, paramlen, salt, sizeof(salt), N, r, p, 0,
	    YESCRYPT_RW | YESCRYPT_PARALLEL_SMIX | __YESCRYPT_INIT_SHARED_1,
	    buf, buflen))
		goto out;

	shared->mask1 = mask;

	return 0;

out:
	if (!(flags & YESCRYPT_SHARED_PREALLOCATED))
		free(shared1->base);
	return -1;
}

int
yescrypt_free_shared(yescrypt_shared_t * shared)
{
	free(shared->shared1.base);
	shared->shared1.base = shared->shared1.aligned = NULL;
	shared->shared1.base_size = shared->shared1.aligned_size = 0;
	return 0;
}

int
yescrypt_init_local(yescrypt_local_t * local)
{
/* The reference implementation doesn't use the local structure */
	local->base = local->aligned = NULL;
	local->base_size = local->aligned_size = 0;
	return 0;
}

int
yescrypt_free_local(yescrypt_local_t * local)
{
/* The reference implementation frees its memory in yescrypt_kdf() */
	return 0;
}
