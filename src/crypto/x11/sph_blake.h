/* $Id: sph_blake.h 252 2011-06-07 17:55:14Z tp $ */
/**
 * BLAKE interface. BLAKE is a family of functions which differ by their
 * output size; this implementation defines BLAKE for output sizes 224,
 * 256, 384 and 512 bits. This implementation conforms to the "third
 * round" specification.
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
 * @file     sph_blake.h
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#ifndef SPH_BLAKE_H__
#define SPH_BLAKE_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <stddef.h>
#include "sph_types.h"

/**
 * Output size (in bits) for BLAKE-224.
 */
#define SPH_SIZE_blake224   224

/**
 * Output size (in bits) for BLAKE-256.
 */
#define SPH_SIZE_blake256   256

#if SPH_64

/**
 * Output size (in bits) for BLAKE-384.
 */
#define SPH_SIZE_blake384   384

/**
 * Output size (in bits) for BLAKE-512.
 */
#define SPH_SIZE_blake512   512

#endif

/**
 * This structure is a context for BLAKE-224 and BLAKE-256 computations:
 * it contains the intermediate values and some data from the last
 * entered block. Once a BLAKE computation has been performed, the
 * context can be reused for another computation.
 *
 * The contents of this structure are private. A running BLAKE
 * computation can be cloned by copying the context (e.g. with a simple
 * <code>memcpy()</code>).
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[64];    /* first field, for alignment */
	size_t ptr;
	sph_u32 H[8];
	sph_u32 S[4];
	sph_u32 T0, T1;
#endif
} sph_blake_small_context;

/**
 * This structure is a context for BLAKE-224 computations. It is
 * identical to the common <code>sph_blake_small_context</code>.
 */
typedef sph_blake_small_context sph_blake224_context;

/**
 * This structure is a context for BLAKE-256 computations. It is
 * identical to the common <code>sph_blake_small_context</code>.
 */
typedef sph_blake_small_context sph_blake256_context;

#if SPH_64

/**
 * This structure is a context for BLAKE-384 and BLAKE-512 computations:
 * it contains the intermediate values and some data from the last
 * entered block. Once a BLAKE computation has been performed, the
 * context can be reused for another computation.
 *
 * The contents of this structure are private. A running BLAKE
 * computation can be cloned by copying the context (e.g. with a simple
 * <code>memcpy()</code>).
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[128];    /* first field, for alignment */
	size_t ptr;
	sph_u64 H[8];
	sph_u64 S[4];
	sph_u64 T0, T1;
#endif
} sph_blake_big_context;

/**
 * This structure is a context for BLAKE-384 computations. It is
 * identical to the common <code>sph_blake_small_context</code>.
 */
typedef sph_blake_big_context sph_blake384_context;

/**
 * This structure is a context for BLAKE-512 computations. It is
 * identical to the common <code>sph_blake_small_context</code>.
 */
typedef sph_blake_big_context sph_blake512_context;

#endif

/**
 * Initialize a BLAKE-224 context. This process performs no memory allocation.
 *
 * @param cc   the BLAKE-224 context (pointer to a
 *             <code>sph_blake224_context</code>)
 */
void sph_blake224_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the BLAKE-224 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_blake224(void *cc, const void *data, size_t len);

/**
 * Terminate the current BLAKE-224 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (28 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the BLAKE-224 context
 * @param dst   the destination buffer
 */
void sph_blake224_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (28 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the BLAKE-224 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_blake224_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

/**
 * Initialize a BLAKE-256 context. This process performs no memory allocation.
 *
 * @param cc   the BLAKE-256 context (pointer to a
 *             <code>sph_blake256_context</code>)
 */
void sph_blake256_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the BLAKE-256 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_blake256(void *cc, const void *data, size_t len);

/**
 * Terminate the current BLAKE-256 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (32 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the BLAKE-256 context
 * @param dst   the destination buffer
 */
void sph_blake256_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (32 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the BLAKE-256 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_blake256_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

#if SPH_64

/**
 * Initialize a BLAKE-384 context. This process performs no memory allocation.
 *
 * @param cc   the BLAKE-384 context (pointer to a
 *             <code>sph_blake384_context</code>)
 */
void sph_blake384_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the BLAKE-384 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_blake384(void *cc, const void *data, size_t len);

/**
 * Terminate the current BLAKE-384 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (48 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the BLAKE-384 context
 * @param dst   the destination buffer
 */
void sph_blake384_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (48 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the BLAKE-384 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_blake384_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

/**
 * Initialize a BLAKE-512 context. This process performs no memory allocation.
 *
 * @param cc   the BLAKE-512 context (pointer to a
 *             <code>sph_blake512_context</code>)
 */
void sph_blake512_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the BLAKE-512 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_blake512(void *cc, const void *data, size_t len);

/**
 * Terminate the current BLAKE-512 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (64 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the BLAKE-512 context
 * @param dst   the destination buffer
 */
void sph_blake512_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (64 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the BLAKE-512 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_blake512_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

#endif

#ifdef __cplusplus
}
#endif

#endif
