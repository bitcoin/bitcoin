/* $Id: sph_shavite.h 208 2010-06-02 20:33:00Z tp $ */
/**
 * SHAvite-3 interface. This code implements SHAvite-3 with the
 * recommended parameters for SHA-3, with outputs of 224, 256, 384 and
 * 512 bits. In the following, we call the function "SHAvite" (without
 * the "-3" suffix), thus "SHAvite-224" is "SHAvite-3 with a 224-bit
 * output".
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
 * @file     sph_shavite.h
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#ifndef SPH_SHAVITE_H__
#define SPH_SHAVITE_H__

#include <stddef.h>
#include "sph_types.h"

#ifdef __cplusplus
extern "C"{
#endif

/**
 * Output size (in bits) for SHAvite-224.
 */
#define SPH_SIZE_shavite224   224

/**
 * Output size (in bits) for SHAvite-256.
 */
#define SPH_SIZE_shavite256   256

/**
 * Output size (in bits) for SHAvite-384.
 */
#define SPH_SIZE_shavite384   384

/**
 * Output size (in bits) for SHAvite-512.
 */
#define SPH_SIZE_shavite512   512

/**
 * This structure is a context for SHAvite-224 and SHAvite-256 computations:
 * it contains the intermediate values and some data from the last
 * entered block. Once a SHAvite computation has been performed, the
 * context can be reused for another computation.
 *
 * The contents of this structure are private. A running SHAvite
 * computation can be cloned by copying the context (e.g. with a simple
 * <code>memcpy()</code>).
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[64];    /* first field, for alignment */
	size_t ptr;
	sph_u32 h[8];
	sph_u32 count0, count1;
#endif
} sph_shavite_small_context;

/**
 * This structure is a context for SHAvite-224 computations. It is
 * identical to the common <code>sph_shavite_small_context</code>.
 */
typedef sph_shavite_small_context sph_shavite224_context;

/**
 * This structure is a context for SHAvite-256 computations. It is
 * identical to the common <code>sph_shavite_small_context</code>.
 */
typedef sph_shavite_small_context sph_shavite256_context;

/**
 * This structure is a context for SHAvite-384 and SHAvite-512 computations:
 * it contains the intermediate values and some data from the last
 * entered block. Once a SHAvite computation has been performed, the
 * context can be reused for another computation.
 *
 * The contents of this structure are private. A running SHAvite
 * computation can be cloned by copying the context (e.g. with a simple
 * <code>memcpy()</code>).
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[128];    /* first field, for alignment */
	size_t ptr;
	sph_u32 h[16];
	sph_u32 count0, count1, count2, count3;
#endif
} sph_shavite_big_context;

/**
 * This structure is a context for SHAvite-384 computations. It is
 * identical to the common <code>sph_shavite_small_context</code>.
 */
typedef sph_shavite_big_context sph_shavite384_context;

/**
 * This structure is a context for SHAvite-512 computations. It is
 * identical to the common <code>sph_shavite_small_context</code>.
 */
typedef sph_shavite_big_context sph_shavite512_context;

/**
 * Initialize a SHAvite-224 context. This process performs no memory allocation.
 *
 * @param cc   the SHAvite-224 context (pointer to a
 *             <code>sph_shavite224_context</code>)
 */
void sph_shavite224_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the SHAvite-224 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_shavite224(void *cc, const void *data, size_t len);

/**
 * Terminate the current SHAvite-224 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (28 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the SHAvite-224 context
 * @param dst   the destination buffer
 */
void sph_shavite224_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (28 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the SHAvite-224 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_shavite224_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

/**
 * Initialize a SHAvite-256 context. This process performs no memory allocation.
 *
 * @param cc   the SHAvite-256 context (pointer to a
 *             <code>sph_shavite256_context</code>)
 */
void sph_shavite256_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the SHAvite-256 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_shavite256(void *cc, const void *data, size_t len);

/**
 * Terminate the current SHAvite-256 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (32 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the SHAvite-256 context
 * @param dst   the destination buffer
 */
void sph_shavite256_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (32 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the SHAvite-256 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_shavite256_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

/**
 * Initialize a SHAvite-384 context. This process performs no memory allocation.
 *
 * @param cc   the SHAvite-384 context (pointer to a
 *             <code>sph_shavite384_context</code>)
 */
void sph_shavite384_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the SHAvite-384 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_shavite384(void *cc, const void *data, size_t len);

/**
 * Terminate the current SHAvite-384 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (48 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the SHAvite-384 context
 * @param dst   the destination buffer
 */
void sph_shavite384_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (48 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the SHAvite-384 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_shavite384_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

/**
 * Initialize a SHAvite-512 context. This process performs no memory allocation.
 *
 * @param cc   the SHAvite-512 context (pointer to a
 *             <code>sph_shavite512_context</code>)
 */
void sph_shavite512_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the SHAvite-512 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_shavite512(void *cc, const void *data, size_t len);

/**
 * Terminate the current SHAvite-512 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (64 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the SHAvite-512 context
 * @param dst   the destination buffer
 */
void sph_shavite512_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (64 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the SHAvite-512 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_shavite512_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);
	
#ifdef __cplusplus
}
#endif	
	
#endif
