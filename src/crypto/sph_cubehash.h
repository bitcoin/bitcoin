/* $Id: sph_cubehash.h 180 2010-05-08 02:29:25Z tp $ */
/**
 * CubeHash interface. CubeHash is a family of functions which differ by
 * their output size; this implementation defines CubeHash for output
 * sizes 224, 256, 384 and 512 bits, with the "standard parameters"
 * (CubeHash16/32 with the CubeHash specification notations).
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
 * @file     sph_cubehash.h
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#ifndef SPH_CUBEHASH_H__
#define SPH_CUBEHASH_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <stddef.h>
#include <crypto/sph_types.h>

/**
 * Output size (in bits) for CubeHash-224.
 */
#define SPH_SIZE_cubehash224   224

/**
 * Output size (in bits) for CubeHash-256.
 */
#define SPH_SIZE_cubehash256   256

/**
 * Output size (in bits) for CubeHash-384.
 */
#define SPH_SIZE_cubehash384   384

/**
 * Output size (in bits) for CubeHash-512.
 */
#define SPH_SIZE_cubehash512   512

/**
 * This structure is a context for CubeHash computations: it contains the
 * intermediate values and some data from the last entered block. Once
 * a CubeHash computation has been performed, the context can be reused for
 * another computation.
 *
 * The contents of this structure are private. A running CubeHash computation
 * can be cloned by copying the context (e.g. with a simple
 * <code>memcpy()</code>).
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[32];    /* first field, for alignment */
	size_t ptr;
	sph_u32 state[32];
#endif
} sph_cubehash_context;

/**
 * Type for a CubeHash-224 context (identical to the common context).
 */
typedef sph_cubehash_context sph_cubehash224_context;

/**
 * Type for a CubeHash-256 context (identical to the common context).
 */
typedef sph_cubehash_context sph_cubehash256_context;

/**
 * Type for a CubeHash-384 context (identical to the common context).
 */
typedef sph_cubehash_context sph_cubehash384_context;

/**
 * Type for a CubeHash-512 context (identical to the common context).
 */
typedef sph_cubehash_context sph_cubehash512_context;

/**
 * Initialize a CubeHash-224 context. This process performs no memory
 * allocation.
 *
 * @param cc   the CubeHash-224 context (pointer to a
 *             <code>sph_cubehash224_context</code>)
 */
void sph_cubehash224_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the CubeHash-224 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_cubehash224(void *cc, const void *data, size_t len);

/**
 * Terminate the current CubeHash-224 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (28 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the CubeHash-224 context
 * @param dst   the destination buffer
 */
void sph_cubehash224_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (28 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the CubeHash-224 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_cubehash224_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

/**
 * Initialize a CubeHash-256 context. This process performs no memory
 * allocation.
 *
 * @param cc   the CubeHash-256 context (pointer to a
 *             <code>sph_cubehash256_context</code>)
 */
void sph_cubehash256_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the CubeHash-256 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_cubehash256(void *cc, const void *data, size_t len);

/**
 * Terminate the current CubeHash-256 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (32 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the CubeHash-256 context
 * @param dst   the destination buffer
 */
void sph_cubehash256_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (32 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the CubeHash-256 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_cubehash256_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

/**
 * Initialize a CubeHash-384 context. This process performs no memory
 * allocation.
 *
 * @param cc   the CubeHash-384 context (pointer to a
 *             <code>sph_cubehash384_context</code>)
 */
void sph_cubehash384_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the CubeHash-384 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_cubehash384(void *cc, const void *data, size_t len);

/**
 * Terminate the current CubeHash-384 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (48 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the CubeHash-384 context
 * @param dst   the destination buffer
 */
void sph_cubehash384_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (48 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the CubeHash-384 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_cubehash384_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

/**
 * Initialize a CubeHash-512 context. This process performs no memory
 * allocation.
 *
 * @param cc   the CubeHash-512 context (pointer to a
 *             <code>sph_cubehash512_context</code>)
 */
void sph_cubehash512_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the CubeHash-512 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_cubehash512(void *cc, const void *data, size_t len);

/**
 * Terminate the current CubeHash-512 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (64 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the CubeHash-512 context
 * @param dst   the destination buffer
 */
void sph_cubehash512_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (64 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the CubeHash-512 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_cubehash512_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);
#ifdef __cplusplus
}
#endif

#endif
