/*-
 * Copyright 2009 Colin Percival
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

#include <sys/types.h>
#ifndef _WIN32
#include <sys/mman.h>
#endif
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sha256.h"
#include "sysendian.h"

#include "libscrypt.h"

static void blkcpy(void *, void *, size_t);
static void blkxor(void *, void *, size_t);
static void salsa20_8(uint32_t[16]);
static void blockmix_salsa8(uint32_t *, uint32_t *, uint32_t *, size_t);
static uint64_t integerify(void *, size_t);
static void smix(uint8_t *, size_t, uint64_t, uint32_t *, uint32_t *);

static void
blkcpy(void * dest, void * src, size_t len)
{
	size_t * D = dest;
	size_t * S = src;
	size_t L = len / sizeof(size_t);
	size_t i;

	for (i = 0; i < L; i++)
		D[i] = S[i];
}

static void
blkxor(void * dest, void * src, size_t len)
{
	size_t * D = dest;
	size_t * S = src;
	size_t L = len / sizeof(size_t);
	size_t i;

	for (i = 0; i < L; i++)
		D[i] ^= S[i];
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

	blkcpy(x, B, 64);
	for (i = 0; i < 8; i += 2) {
#define R(a,b) (((a) << (b)) | ((a) >> (32 - (b))))
		/* Operate on columns. */
		x[ 4] ^= R(x[ 0]+x[12], 7);  x[ 8] ^= R(x[ 4]+x[ 0], 9);
		x[12] ^= R(x[ 8]+x[ 4],13);  x[ 0] ^= R(x[12]+x[ 8],18);

		x[ 9] ^= R(x[ 5]+x[ 1], 7);  x[13] ^= R(x[ 9]+x[ 5], 9);
		x[ 1] ^= R(x[13]+x[ 9],13);  x[ 5] ^= R(x[ 1]+x[13],18);

		x[14] ^= R(x[10]+x[ 6], 7);  x[ 2] ^= R(x[14]+x[10], 9);
		x[ 6] ^= R(x[ 2]+x[14],13);  x[10] ^= R(x[ 6]+x[ 2],18);

		x[ 3] ^= R(x[15]+x[11], 7);  x[ 7] ^= R(x[ 3]+x[15], 9);
		x[11] ^= R(x[ 7]+x[ 3],13);  x[15] ^= R(x[11]+x[ 7],18);

		/* Operate on rows. */
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
	for (i = 0; i < 16; i++)
		B[i] += x[i];
}

/**
 * blockmix_salsa8(Bin, Bout, X, r):
 * Compute Bout = BlockMix_{salsa20/8, r}(Bin).  The input Bin must be 128r
 * bytes in length; the output Bout must also be the same size.  The
 * temporary space X must be 64 bytes.
 */
static void
blockmix_salsa8(uint32_t * Bin, uint32_t * Bout, uint32_t * X, size_t r)
{
	size_t i;

	/* 1: X <-- B_{2r - 1} */
	blkcpy(X, &Bin[(2 * r - 1) * 16], 64);

	/* 2: for i = 0 to 2r - 1 do */
	for (i = 0; i < 2 * r; i += 2) {
		/* 3: X <-- H(X \xor B_i) */
		blkxor(X, &Bin[i * 16], 64);
		salsa20_8(X);

		/* 4: Y_i <-- X */
		/* 6: B' <-- (Y_0, Y_2 ... Y_{2r-2}, Y_1, Y_3 ... Y_{2r-1}) */
		blkcpy(&Bout[i * 8], X, 64);

		/* 3: X <-- H(X \xor B_i) */
		blkxor(X, &Bin[i * 16 + 16], 64);
		salsa20_8(X);

		/* 4: Y_i <-- X */
		/* 6: B' <-- (Y_0, Y_2 ... Y_{2r-2}, Y_1, Y_3 ... Y_{2r-1}) */
		blkcpy(&Bout[i * 8 + r * 16], X, 64);
	}
}

/**
 * integerify(B, r):
 * Return the result of parsing B_{2r-1} as a little-endian integer.
 */
static uint64_t
integerify(void * B, size_t r)
{
	uint32_t * X = (void *)((uintptr_t)(B) + (2 * r - 1) * 64);

	return (((uint64_t)(X[1]) << 32) + X[0]);
}

/**
 * smix(B, r, N, V, XY):
 * Compute B = SMix_r(B, N).  The input B must be 128r bytes in length;
 * the temporary storage V must be 128rN bytes in length; the temporary
 * storage XY must be 256r + 64 bytes in length.  The value N must be a
 * power of 2 greater than 1.  The arrays B, V, and XY must be aligned to a
 * multiple of 64 bytes.
 */
static void
smix(uint8_t * B, size_t r, uint64_t N, uint32_t * V, uint32_t * XY)
{
	uint32_t * X = XY;
	uint32_t * Y = &XY[32 * r];
	uint32_t * Z = &XY[64 * r];
	uint64_t i;
	uint64_t j;
	size_t k;

	/* 1: X <-- B */
	for (k = 0; k < 32 * r; k++)
		X[k] = le32dec(&B[4 * k]);

	/* 2: for i = 0 to N - 1 do */
	for (i = 0; i < N; i += 2) {
		/* 3: V_i <-- X */
		blkcpy(&V[i * (32 * r)], X, 128 * r);

		/* 4: X <-- H(X) */
		blockmix_salsa8(X, Y, Z, r);

		/* 3: V_i <-- X */
		blkcpy(&V[(i + 1) * (32 * r)], Y, 128 * r);

		/* 4: X <-- H(X) */
		blockmix_salsa8(Y, X, Z, r);
	}

	/* 6: for i = 0 to N - 1 do */
	for (i = 0; i < N; i += 2) {
		/* 7: j <-- Integerify(X) mod N */
		j = integerify(X, r) & (N - 1);

		/* 8: X <-- H(X \xor V_j) */
		blkxor(X, &V[j * (32 * r)], 128 * r);
		blockmix_salsa8(X, Y, Z, r);

		/* 7: j <-- Integerify(X) mod N */
		j = integerify(Y, r) & (N - 1);

		/* 8: X <-- H(X \xor V_j) */
		blkxor(Y, &V[j * (32 * r)], 128 * r);
		blockmix_salsa8(Y, X, Z, r);
	}

	/* 10: B' <-- X */
	for (k = 0; k < 32 * r; k++)
		le32enc(&B[4 * k], X[k]);
}

/**
 * crypto_scrypt(passwd, passwdlen, salt, saltlen, N, r, p, buf, buflen):
 * Compute scrypt(passwd[0 .. passwdlen - 1], salt[0 .. saltlen - 1], N, r,
 * p, buflen) and write the result into buf.  The parameters r, p, and buflen
 * must satisfy r * p < 2^30 and buflen <= (2^32 - 1) * 32.  The parameter N
 * must be a power of 2 greater than 1.
 *
 * Return 0 on success; or -1 on error
 */
int
libscrypt_scrypt(const uint8_t * passwd, size_t passwdlen,
    const uint8_t * salt, size_t saltlen, uint64_t N, uint32_t r, uint32_t p,
    uint8_t * buf, size_t buflen)
{
	void * B0, * V0, * XY0;
	uint8_t * B;
	uint32_t * V;
	uint32_t * XY;
	uint32_t i;

	/* Sanity-check parameters. */
#if SIZE_MAX > UINT32_MAX
	if (buflen > (((uint64_t)(1) << 32) - 1) * 32) {
		errno = EFBIG;
		goto err0;
	}
#endif
	if ((uint64_t)(r) * (uint64_t)(p) >= (1 << 30)) {
		errno = EFBIG;
		goto err0;
	}
	if (r == 0 || p == 0) {
		errno = EINVAL;
		goto err0;
	}
	if (((N & (N - 1)) != 0) || (N < 2)) {
		errno = EINVAL;
		goto err0;
	}
	if ((r > SIZE_MAX / 128 / p) ||
#if SIZE_MAX / 256 <= UINT32_MAX
	    (r > SIZE_MAX / 256) ||
#endif
	    (N > SIZE_MAX / 128 / r)) {
		errno = ENOMEM;
		goto err0;
	}

	/* Allocate memory. */
#ifdef HAVE_POSIX_MEMALIGN
	if ((errno = posix_memalign(&B0, 64, 128 * r * p)) != 0)
		goto err0;
	B = (uint8_t *)(B0);
	if ((errno = posix_memalign(&XY0, 64, 256 * r + 64)) != 0)
		goto err1;
	XY = (uint32_t *)(XY0);
#ifndef MAP_ANON
	if ((errno = posix_memalign(&V0, 64, 128 * r * N)) != 0)
		goto err2;
	V = (uint32_t *)(V0);
#endif
#else
	if ((B0 = malloc(128 * r * p + 63)) == NULL)
		goto err0;
	B = (uint8_t *)(((uintptr_t)(B0) + 63) & ~ (uintptr_t)(63));
	if ((XY0 = malloc(256 * r + 64 + 63)) == NULL)
		goto err1;
	XY = (uint32_t *)(((uintptr_t)(XY0) + 63) & ~ (uintptr_t)(63));
#ifndef MAP_ANON
	if ((V0 = malloc(128 * r * N + 63)) == NULL)
		goto err2;
	V = (uint32_t *)(((uintptr_t)(V0) + 63) & ~ (uintptr_t)(63));
#endif
#endif
#ifdef MAP_ANON
	if ((V0 = mmap(NULL, 128 * r * N, PROT_READ | PROT_WRITE,
#ifdef MAP_NOCORE
	    MAP_ANON | MAP_PRIVATE | MAP_NOCORE,
#else
	    MAP_ANON | MAP_PRIVATE,
#endif
	    -1, 0)) == MAP_FAILED)
		goto err2;
	V = (uint32_t *)(V0);
#endif

	/* 1: (B_0 ... B_{p-1}) <-- PBKDF2(P, S, 1, p * MFLen) */
	libscrypt_PBKDF2_SHA256(passwd, passwdlen, salt, saltlen, 1, B, p * 128 * r);

	/* 2: for i = 0 to p - 1 do */
	for (i = 0; i < p; i++) {
		/* 3: B_i <-- MF(B_i, N) */
		smix(&B[i * 128 * r], r, N, V, XY);
	}

	/* 5: DK <-- PBKDF2(P, B, 1, dkLen) */
	libscrypt_PBKDF2_SHA256(passwd, passwdlen, B, p * 128 * r, 1, buf, buflen);

	/* Free memory. */
#ifdef MAP_ANON
	if (munmap(V0, 128 * r * N))
		goto err2;
#else
	free(V0);
#endif
	free(XY0);
	free(B0);

	/* Success! */
	return (0);

err2:
	free(XY0);
err1:
	free(B0);
err0:
	/* Failure! */
	return (-1);
}
