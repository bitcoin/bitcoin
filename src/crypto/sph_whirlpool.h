/* $Id: sph_whirlpool.h 216 2010-06-08 09:46:57Z tp $ */
/**
 * WHIRLPOOL interface.
 *
 * WHIRLPOOL knows three variants, dubbed "WHIRLPOOL-0" (original
 * version, published in 2000, studied by NESSIE), "WHIRLPOOL-1"
 * (first revision, 2001, with a new S-box) and "WHIRLPOOL" (current
 * version, 2003, with a new diffusion matrix, also described as "plain
 * WHIRLPOOL"). All three variants are implemented here.
 *
 * The original WHIRLPOOL (i.e. WHIRLPOOL-0) was published in: P. S. L.
 * M. Barreto, V. Rijmen, "The Whirlpool Hashing Function", First open
 * NESSIE Workshop, Leuven, Belgium, November 13--14, 2000.
 *
 * The current WHIRLPOOL specification and a reference implementation
 * can be found on the WHIRLPOOL web page:
 * http://paginas.terra.com.br/informatica/paulobarreto/WhirlpoolPage.html
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
 * @file     sph_whirlpool.h
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#ifndef SPH_WHIRLPOOL_H__
#define SPH_WHIRLPOOL_H__

#include <stddef.h>
#include "sph_types.h"

#ifdef __cplusplus
extern "C"{
#endif

#if SPH_64

/**
 * Output size (in bits) for WHIRLPOOL.
 */
#define SPH_SIZE_whirlpool   512

/**
 * Output size (in bits) for WHIRLPOOL-0.
 */
#define SPH_SIZE_whirlpool0   512

/**
 * Output size (in bits) for WHIRLPOOL-1.
 */
#define SPH_SIZE_whirlpool1   512

/**
 * This structure is a context for WHIRLPOOL computations: it contains the
 * intermediate values and some data from the last entered block. Once
 * a WHIRLPOOL computation has been performed, the context can be reused for
 * another computation.
 *
 * The contents of this structure are private. A running WHIRLPOOL computation
 * can be cloned by copying the context (e.g. with a simple
 * <code>memcpy()</code>).
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[64];    /* first field, for alignment */
	sph_u64 state[8];
#if SPH_64
	sph_u64 count;
#else
	sph_u32 count_high, count_low;
#endif
#endif
} sph_whirlpool_context;

/**
 * Initialize a WHIRLPOOL context. This process performs no memory allocation.
 *
 * @param cc   the WHIRLPOOL context (pointer to a
 *             <code>sph_whirlpool_context</code>)
 */
void sph_whirlpool_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing). This function applies the
 * plain WHIRLPOOL algorithm.
 *
 * @param cc     the WHIRLPOOL context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_whirlpool(void *cc, const void *data, size_t len);

/**
 * Terminate the current WHIRLPOOL computation and output the result into the
 * provided buffer. The destination buffer must be wide enough to
 * accomodate the result (64 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the WHIRLPOOL context
 * @param dst   the destination buffer
 */
void sph_whirlpool_close(void *cc, void *dst);

/**
 * WHIRLPOOL-0 uses the same structure than plain WHIRLPOOL.
 */
typedef sph_whirlpool_context sph_whirlpool0_context;

#ifdef DOXYGEN_IGNORE
/**
 * Initialize a WHIRLPOOL-0 context. This function is identical to
 * <code>sph_whirlpool_init()</code>.
 *
 * @param cc   the WHIRLPOOL context (pointer to a
 *             <code>sph_whirlpool0_context</code>)
 */
void sph_whirlpool0_init(void *cc);
#endif

#ifndef DOXYGEN_IGNORE
#define sph_whirlpool0_init   sph_whirlpool_init
#endif

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing). This function applies the
 * WHIRLPOOL-0 algorithm.
 *
 * @param cc     the WHIRLPOOL context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_whirlpool0(void *cc, const void *data, size_t len);

/**
 * Terminate the current WHIRLPOOL-0 computation and output the result into the
 * provided buffer. The destination buffer must be wide enough to
 * accomodate the result (64 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the WHIRLPOOL-0 context
 * @param dst   the destination buffer
 */
void sph_whirlpool0_close(void *cc, void *dst);

/**
 * WHIRLPOOL-1 uses the same structure than plain WHIRLPOOL.
 */
typedef sph_whirlpool_context sph_whirlpool1_context;

#ifdef DOXYGEN_IGNORE
/**
 * Initialize a WHIRLPOOL-1 context. This function is identical to
 * <code>sph_whirlpool_init()</code>.
 *
 * @param cc   the WHIRLPOOL context (pointer to a
 *             <code>sph_whirlpool1_context</code>)
 */
void sph_whirlpool1_init(void *cc);
#endif

#ifndef DOXYGEN_IGNORE
#define sph_whirlpool1_init   sph_whirlpool_init
#endif

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing). This function applies the
 * WHIRLPOOL-1 algorithm.
 *
 * @param cc     the WHIRLPOOL context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_whirlpool1(void *cc, const void *data, size_t len);

/**
 * Terminate the current WHIRLPOOL-1 computation and output the result into the
 * provided buffer. The destination buffer must be wide enough to
 * accomodate the result (64 bytes). The context is automatically
 * reinitialized.
 *
 * @param cc    the WHIRLPOOL-1 context
 * @param dst   the destination buffer
 */
void sph_whirlpool1_close(void *cc, void *dst);

#endif

#endif

#ifdef __cplusplus
}
#endif