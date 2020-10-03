/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2017 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * RELIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RELIC. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @defgroup relic Core functions
 */

/**
 * @file
 *
 * Interface of the library core functions.
 *
 * @ingroup relic
 */

#ifndef RELIC_CORE_H
#define RELIC_CORE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "relic_err.h"
#include "relic_bn.h"
#include "relic_eb.h"
#include "relic_epx.h"
#include "relic_ed.h"
#include "relic_conf.h"
#include "relic_bench.h"
#include "relic_rand.h"
#include "relic_pool.h"
#include "relic_label.h"

#if MULTI != RELIC_NONE
#include <math.h>

#if MULTI == OPENMP
#include <omp.h>
#elif MULTI == PTHREAD
#include <pthread.h>
#endif /* OPENMP */

#endif /* MULTI != RELIC_NONE */

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

/**
 * Indicates that the function executed correctly.
 */
#define STS_OK			0

/**
 * Indicates that an error occurred during the function execution.
 */
#define STS_ERR			1

/**
 * Indicates that a comparison returned that the first argument was lesser than
 * the second argument.
 */
#define CMP_LT			-1

/**
 * Indicates that a comparison returned that the first argument was equal to
 * the second argument.
 */
#define CMP_EQ			0

/**
 * Indicates that a comparison returned that the first argument was greater than
 * the second argument.
 */
#define CMP_GT			1

/**
 * Indicates that two incomparable elements are not equal.
 */
#define CMP_NE			2

/**
 * Optimization identifer for the case where a coefficient is 0.
 */
#define OPT_ZERO		0

/**
 * Optimization identifer for the case where a coefficient is 1.
 */
#define OPT_ONE			1

/**
 * Optimization identifer for the case where a coefficient is 1.
 */
#define OPT_TWO			2

/**
 * Optimization identifer for the case where a coefficient is small.
 */
#define OPT_DIGIT		3

/**
 * Optimization identifier for the case where a coefficient is -3.
 */
#define OPT_MINUS3		4

/**
 * Optimization identifier for the case where the coefficient is random
 */
#define RELIC_OPT_NONE		5

/**
 * Maximum number of terms to describe a sparse object.
 */
#define MAX_TERMS		16

/*============================================================================*/
/* Type definitions                                                           */
/*============================================================================*/

/**
 * Library context.
 */
typedef struct _ctx_t {
	/** The value returned by the last call, can be STS_OK or STS_ERR. */
	int code;

#ifdef CHECK
	/** The state of the last error caught. */
	sts_t *last;
	/** Error state to be used outside try-catch blocks. */
	sts_t error;
	/** Error number to be used outside try-catch blocks. */
	err_t number;
	/** The error message respective to the last error. */
	char *reason[ERR_MAX];
	/** A flag to indicate if the last error was already caught. */
	int caught;
#endif /* CHECK */

#if defined(CHECK) && defined(TRACE)
	/** The current trace size. */
	int trace;
#endif /* CHECK && TRACE */

#if ALLOC == STATIC
	/** The static pool of digit vectors. */
	pool_t pool[POOL_SIZE];
	/** The index of the next free digit vector in the pool. */
	int next;
#endif /* ALLOC == STATIC */

#ifdef WITH_FB
	/** Identifier of the currently configured binary field. */
	int fb_id;
	/** Irreducible binary polynomial. */
	fb_st fb_poly;
	/** Non-zero coefficients of a trinomial or pentanomial. */
	int fb_pa, fb_pb, fb_pc;
	/** Positions of the non-zero coefficients of trinomials or pentanomials. */
	int fb_na, fb_nb, fb_nc;
#if FB_TRC == QUICK || !defined(STRIP)
	/** Powers of z with non-zero traces. */
	int fb_ta, fb_tb, fb_tc;
#endif /* FB_TRC == QUICK */
#if FB_SLV == QUICK || !defined(STRIP)
	/** Table of precomputed half-traces. */
	fb_st fb_half[(FB_DIGIT / 8 + 1) * FB_DIGS][16];
#endif /* FB_SLV == QUICK */
#if FB_SRT == QUICK || !defined(STRIP)
	/** Square root of z. */
	fb_st fb_srz;
#ifdef FB_PRECO
	/** Multiplication table for the z^(1/2). */
	fb_st fb_tab_srz[256];
#endif /* FB_PRECO */
#endif /* FB_SRT == QUICK */
#if FB_INV == ITOHT || !defined(STRIP)
	/** Stores an addition chain for (FB_BITS - 1). */
	int chain[MAX_TERMS + 1];
	/** Stores the length of the addition chain. */
	int chain_len;
	/** Tables for repeated squarings. */
	fb_st fb_tab_sqr[MAX_TERMS][RELIC_FB_TABLE];
	/** Pointers to the elements in the tables of repeated squarings. */
	fb_st *fb_tab_ptr[MAX_TERMS][RELIC_FB_TABLE];
#endif /* FB_INV == ITOHT */
#endif /* WITH_FB */

#ifdef WITH_EB
	/** Identifier of the currently configured binary elliptic curve. */
	int eb_id;
	/** The 'a' coefficient of the elliptic curve. */
	fb_st eb_a;
	/** The 'b' coefficient of the elliptic curve. */
	fb_st eb_b;
	/** Optimization identifier for the 'a' coefficient. */
	int eb_opt_a;
	/** Optimization identifier for the 'b' coefficient. */
	int eb_opt_b;
	/** The generator of the elliptic curve. */
	eb_st eb_g;
	/** The order of the group of points in the elliptic curve. */
	bn_st eb_r;
	/** The cofactor of the group order in the elliptic curve. */
	bn_st eb_h;
	/** Flag that stores if the binary curve has efficient endomorphisms. */
	int eb_is_kbltz;
#ifdef EB_PRECO
	/** Precomputation table for generator multiplication. */
	eb_st eb_pre[RELIC_EB_TABLE];
	/** Array of pointers to the precomputation table. */
	eb_st *eb_ptr[RELIC_EB_TABLE];
#endif /* EB_PRECO */
#endif /* WITH_EB */

#ifdef WITH_FP
	/** Identifier of the currently configured prime field. */
	int fp_id;
	/** Prime modulus. */
	bn_st prime;
#if FP_RDC == MONTY || !defined(STRIP)
	/** Value (R^2 mod p) for converting small integers to Montgomery form. */
	bn_st conv;
	/** Value of constant one in Montgomery form. */
	bn_st one;
#endif /* FP_RDC == MONTY */
	/** Prime modulus modulo 8. */
	dig_t mod8;
	/** Value derived from the prime used for modular reduction. */
	dig_t u;
	/** Quadratic non-residue. */
	int qnr;
	/** Cubic non-residue. */
	int cnr;
#if FP_RDC == QUICK || !defined(STRIP)
	/** Sparse representation of prime modulus. */
	int sps[MAX_TERMS + 1];
	/** Length of sparse prime representation. */
	int sps_len;
#endif /* FP_RDC == QUICK */
#endif /* WITH_FP */

#ifdef WITH_EP
	/** Identifier of the currently configured prime elliptic curve. */
	int ep_id;
	/** The 'a' coefficient of the elliptic curve. */
	fp_st ep_a;
	/** The 'b' coefficient of the elliptic curve. */
	fp_st ep_b;
	/** The generator of the elliptic curve. */
	ep_st ep_g;
	/** The order of the group of points in the elliptic curve. */
	bn_st ep_r;
	/** The cofactor of the group order in the elliptic curve. */
	bn_st ep_h;
#ifdef EP_ENDOM
#if EP_MUL == LWNAF || EP_FIX == COMBS || EP_FIX == LWNAF || EP_SIM == INTER || !defined(STRIP)
	/** Parameters required by the GLV method. @{ */
	fp_st beta;
	bn_st ep_v1[3];
	bn_st ep_v2[3];
	/** @} */
#endif /* EP_ENDOM */
#endif /* EP_MUL */
	/** Optimization identifier for the a-coefficient. */
	int ep_opt_a;
	/** Optimization identifier for the b-coefficient. */
	int ep_opt_b;
	/** Flag that stores if the prime curve has efficient endomorphisms. */
	int ep_is_endom;
	/** Flag that stores if the prime curve is supersingular. */
	int ep_is_super;
#ifdef EP_PRECO
	/** Precomputation table for generator multiplication. */
	ep_st ep_pre[RELIC_EP_TABLE];
	/** Array of pointers to the precomputation table. */
	ep_st *ep_ptr[RELIC_EP_TABLE];
#endif /* EP_PRECO */
#endif /* WITH_EP */

#ifdef WITH_EPX
	/** The generator of the elliptic curve. */
	ep2_st ep2_g;
#if ALLOC == STATIC || ALLOC == DYNAMIC || ALLOC == STACK
	/** The first coordinate of the generator. */
	fp2_st ep2_gx;
	/** The second coordinate of the generator. */
	fp2_st ep2_gy;
	/** The third coordinate of the generator. */
	fp2_st ep2_gz;
#endif
	/** The 'a' coefficient of the curve. */
	fp2_st ep2_a;
	/** The 'b' coefficient of the curve. */
	fp2_st ep2_b;
	/** The order of the group of points in the elliptic curve. */
	bn_st ep2_r;
	/** The cofactor of the group order in the elliptic curve. */
	bn_st ep2_h;
	/** sqrt(-3) in the field for this curve */
	bn_st ep2_s3;
	/** (sqrt(-3) - 1) / 2 in the field for this curve */
	bn_st ep2_s32;
	/** Flag that stores if the prime curve is a twist. */
	int ep2_is_twist;
#ifdef EP_PRECO
	/** Precomputation table for generator multiplication.*/
	ep2_st ep2_pre[RELIC_EP_TABLE];
	/** Array of pointers to the precomputation table. */
	ep2_st *ep2_ptr[RELIC_EP_TABLE];
#endif /* EP_PRECO */
#if ALLOC == STACK
/** In case of stack allocation, we need to get global memory for the table. */
	fp2_st _ep2_pre[3 * RELIC_EP_TABLE];
#endif /* ALLOC == STACK */
#endif /* WITH_EPX */

#ifdef WITH_ED
	/** Identifier of the currently configured prime Twisted Edwards elliptic curve. */
	int ed_id;
	/** The 'a' coefficient of the Twisted Edwards elliptic curve. */
	fp_st ed_a;
	/** The 'd' coefficient of the Twisted Edwards elliptic curve. */
	fp_st ed_d;
	/** The generator of the elliptic curve. */
	ed_st ed_g;
	/** The order of the group of points in the elliptic curve. */
	bn_st ed_r;
	/** The cofactor of the Twisted Edwards elliptic curve */
	bn_st ed_h;

#ifdef ED_PRECO
	/** Precomputation table for generator multiplication. */
	ed_st ed_pre[RELIC_ED_TABLE];
	/** Array of pointers to the precomputation table. */
	ed_st *ed_ptr[RELIC_ED_TABLE];
#endif /* ED_PRECO */
#endif

#ifdef WITH_PP
	/** Constants for computing Frobenius maps in higher extensions. @{ */
	fp2_st fp2_p[5];
	fp_st fp2_p2[4];
	fp2_st fp2_p3[5];
	/** @} */
	/** Constants for computing Frobenius maps in higher extensions. @{ */
	fp_st fp3_base[2];
	fp_st fp3_p[5];
	fp_st fp3_p2[5];
	fp_st fp3_p3[5];
	fp_st fp3_p4[5];
	fp_st fp3_p5[5];
	/** @} */
#endif /* WITH_PP */

#if BENCH > 0
	/** Stores the time measured before the execution of the benchmark. */
	bench_t before;
	/** Stores the time measured after the execution of the benchmark. */
	bench_t after;
	/** Stores the sum of timings for the current benchmark. */
	long long total;
#ifdef OVERH
	/** Benchmarking overhead to be measured and subtracted from benchmarks. */
	long long over;
#endif
#endif

#if RAND != CALL
	/** Internal state of the PRNG. */
	uint8_t rand[RAND_SIZE];
#else
	void (*rand_call)(uint8_t *, int, void *);
	void *rand_args;
#endif
	/** Flag to indicate if PRNG is seed. */
	int seeded;
	/** Counter to keep track of number of calls since last seeding. */
	int counter;
} ctx_t;

#ifdef TRACE

/*
 * The default library context. This context is only visible when tracing is
 * enabled to avoid infinite recursion insde the trace functions.
 */
extern ctx_t first_ctx;

/*
 * The current library context. This context is only visible when tracing is
 * enabled to avoid infinite recursion insde the trace functions.
 */
extern ctx_t *core_ctx;

#endif

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Initializes the library.
 *
 * @return STS_OK if no error occurs, STS_ERR otherwise.
 */
int core_init(void);

/**
 * Finalizes the library.
 *
 * @return STS_OK if no error occurs, STS_ERR otherwise.
 */
int core_clean(void);

/**
 * Returns a pointer to the current library context.
 *
 * @return a pointer to the library context.
 */
ctx_t *core_get(void);

/**
 * Switched the library context to a new context.
 *
 * @param[in] ctx					- the new library context.
 */
void core_set(ctx_t *ctx);

#if MULTI != RELIC_NONE
/**
 * Set an initializer function which is called when the context
 * is uninitialized. This function is called for every thread.
 *
 * @param[in] init function to call when the current context is not initialized
 * @param[in] init_ptr a pointer which is passed to the initialized
 */
void core_set_thread_initializer(void(*init)(void *init_ptr), void* init_ptr);
#endif

#endif /* !RELIC_CORE_H */
