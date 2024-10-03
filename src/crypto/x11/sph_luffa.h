/* $Id: sph_luffa.h 154 2010-04-26 17:00:24Z tp $ */
/**
 * Luffa interface. Luffa is a family of functions which differ by
 * their output size; this implementation defines Luffa for output
 * sizes 224, 256, 384 and 512 bits.
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
 * @file     sph_luffa.h
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#ifndef SPH_LUFFA_H__
#define SPH_LUFFA_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <stddef.h>
#include "sph_types.h"

/**
 * Output size (in bits) for Luffa-224.
 */
#define SPH_SIZE_luffa224   224

/**
 * Output size (in bits) for Luffa-256.
 */
#define SPH_SIZE_luffa256   256

/**
 * Output size (in bits) for Luffa-384.
 */
#define SPH_SIZE_luffa384   384

/**
 * Output size (in bits) for Luffa-512.
 */
#define SPH_SIZE_luffa512   512

/**
 * This structure is a context for Luffa-224 computations: it contains
 * the intermediate values and some data from the last entered block.
 * Once a Luffa computation has been performed, the context can be
 * reused for another computation.
 *
 * The contents of this structure are private. A running Luffa
 * computation can be cloned by copying the context (e.g. with a simple
 * <code>memcpy()</code>).
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[32];    /* first field, for alignment */
	size_t ptr;
	sph_u32 V[3][8];
#endif
} sph_luffa224_context;

/**
 * This structure is a context for Luffa-256 computations. It is
 * identical to <code>sph_luffa224_context</code>.
 */
typedef sph_luffa224_context sph_luffa256_context;

/**
 * This structure is a context for Luffa-384 computations.
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[32];    /* first field, for alignment */
	size_t ptr;
	sph_u32 V[4][8];
#endif
} sph_luffa384_context;

/**
 * This structure is a context for Luffa-512 computations.
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[32];    /* first field, for alignment */
	size_t ptr;
	sph_u32 V[5][8];
#endif
} sph_luffa512_context;

/**
 * Initialize a Luffa-224 context. This process performs no memory allocation.
 *
 * @param cc   the Luffa-224 context (pointer to a
 *             <code>sph_luffa224_context</code>)
 */
void sph_luffa224_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the Luffa-224 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_luffa224(void *cc, const void *data, size_t len);

/**
 * Terminate the current Luffa-224 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (28 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the Luffa-224 context
 * @param dst   the destination buffer
 */
void sph_luffa224_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (28 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the Luffa-224 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_luffa224_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

/**
 * Initialize a Luffa-256 context. This process performs no memory allocation.
 *
 * @param cc   the Luffa-256 context (pointer to a
 *             <code>sph_luffa256_context</code>)
 */
void sph_luffa256_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the Luffa-256 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_luffa256(void *cc, const void *data, size_t len);

/**
 * Terminate the current Luffa-256 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (32 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the Luffa-256 context
 * @param dst   the destination buffer
 */
void sph_luffa256_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (32 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the Luffa-256 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_luffa256_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

/**
 * Initialize a Luffa-384 context. This process performs no memory allocation.
 *
 * @param cc   the Luffa-384 context (pointer to a
 *             <code>sph_luffa384_context</code>)
 */
void sph_luffa384_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the Luffa-384 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_luffa384(void *cc, const void *data, size_t len);

/**
 * Terminate the current Luffa-384 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (48 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the Luffa-384 context
 * @param dst   the destination buffer
 */
void sph_luffa384_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (48 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the Luffa-384 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_luffa384_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

/**
 * Initialize a Luffa-512 context. This process performs no memory allocation.
 *
 * @param cc   the Luffa-512 context (pointer to a
 *             <code>sph_luffa512_context</code>)
 */
void sph_luffa512_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the Luffa-512 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_luffa512(void *cc, const void *data, size_t len);

/**
 * Terminate the current Luffa-512 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (64 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the Luffa-512 context
 * @param dst   the destination buffer
 */
void sph_luffa512_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (64 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the Luffa-512 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_luffa512_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

#ifdef __cplusplus
}
#endif

#endif
