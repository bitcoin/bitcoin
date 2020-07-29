/* $Id: sph_echo.h 216 2010-06-08 09:46:57Z tp $ */
/**
 * ECHO interface. ECHO is a family of functions which differ by
 * their output size; this implementation defines ECHO for output
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
 * @file     sph_echo.h
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#ifndef SPH_ECHO_H__
#define SPH_ECHO_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <stddef.h>
#include <crypto/sph_types.h>

/**
 * Output size (in bits) for ECHO-224.
 */
#define SPH_SIZE_echo224   224

/**
 * Output size (in bits) for ECHO-256.
 */
#define SPH_SIZE_echo256   256

/**
 * Output size (in bits) for ECHO-384.
 */
#define SPH_SIZE_echo384   384

/**
 * Output size (in bits) for ECHO-512.
 */
#define SPH_SIZE_echo512   512

/**
 * This structure is a context for ECHO computations: it contains the
 * intermediate values and some data from the last entered block. Once
 * an ECHO computation has been performed, the context can be reused for
 * another computation. This specific structure is used for ECHO-224
 * and ECHO-256.
 *
 * The contents of this structure are private. A running ECHO computation
 * can be cloned by copying the context (e.g. with a simple
 * <code>memcpy()</code>).
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[192];    /* first field, for alignment */
	size_t ptr;
	union {
		sph_u32 Vs[4][4];
#if SPH_64
		sph_u64 Vb[4][2];
#endif
	} u;
	sph_u32 C0, C1, C2, C3;
#endif
} sph_echo_small_context;

/**
 * This structure is a context for ECHO computations: it contains the
 * intermediate values and some data from the last entered block. Once
 * an ECHO computation has been performed, the context can be reused for
 * another computation. This specific structure is used for ECHO-384
 * and ECHO-512.
 *
 * The contents of this structure are private. A running ECHO computation
 * can be cloned by copying the context (e.g. with a simple
 * <code>memcpy()</code>).
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[128];    /* first field, for alignment */
	size_t ptr;
	union {
		sph_u32 Vs[8][4];
#if SPH_64
		sph_u64 Vb[8][2];
#endif
	} u;
	sph_u32 C0, C1, C2, C3;
#endif
} sph_echo_big_context;

/**
 * Type for a ECHO-224 context (identical to the common "small" context).
 */
typedef sph_echo_small_context sph_echo224_context;

/**
 * Type for a ECHO-256 context (identical to the common "small" context).
 */
typedef sph_echo_small_context sph_echo256_context;

/**
 * Type for a ECHO-384 context (identical to the common "big" context).
 */
typedef sph_echo_big_context sph_echo384_context;

/**
 * Type for a ECHO-512 context (identical to the common "big" context).
 */
typedef sph_echo_big_context sph_echo512_context;

/**
 * Initialize an ECHO-224 context. This process performs no memory allocation.
 *
 * @param cc   the ECHO-224 context (pointer to a
 *             <code>sph_echo224_context</code>)
 */
void sph_echo224_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the ECHO-224 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_echo224(void *cc, const void *data, size_t len);

/**
 * Terminate the current ECHO-224 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (28 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the ECHO-224 context
 * @param dst   the destination buffer
 */
void sph_echo224_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (28 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the ECHO-224 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_echo224_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

/**
 * Initialize an ECHO-256 context. This process performs no memory allocation.
 *
 * @param cc   the ECHO-256 context (pointer to a
 *             <code>sph_echo256_context</code>)
 */
void sph_echo256_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the ECHO-256 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_echo256(void *cc, const void *data, size_t len);

/**
 * Terminate the current ECHO-256 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (32 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the ECHO-256 context
 * @param dst   the destination buffer
 */
void sph_echo256_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (32 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the ECHO-256 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_echo256_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

/**
 * Initialize an ECHO-384 context. This process performs no memory allocation.
 *
 * @param cc   the ECHO-384 context (pointer to a
 *             <code>sph_echo384_context</code>)
 */
void sph_echo384_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the ECHO-384 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_echo384(void *cc, const void *data, size_t len);

/**
 * Terminate the current ECHO-384 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (48 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the ECHO-384 context
 * @param dst   the destination buffer
 */
void sph_echo384_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (48 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the ECHO-384 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_echo384_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

/**
 * Initialize an ECHO-512 context. This process performs no memory allocation.
 *
 * @param cc   the ECHO-512 context (pointer to a
 *             <code>sph_echo512_context</code>)
 */
void sph_echo512_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the ECHO-512 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_echo512(void *cc, const void *data, size_t len);

/**
 * Terminate the current ECHO-512 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (64 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the ECHO-512 context
 * @param dst   the destination buffer
 */
void sph_echo512_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (64 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the ECHO-512 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void sph_echo512_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

#ifdef __cplusplus
}
#endif

#endif
