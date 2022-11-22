/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2009 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or modify it under the
 * terms of the version 2.1 (or later) of the GNU Lesser General Public License
 * as published by the Free Software Foundation; or version 2.0 of the Apache
 * License as published by the Apache Software Foundation. See the LICENSE files
 * for more details.
 *
 * RELIC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the LICENSE files for more details.
 *
 * You should have received a copy of the GNU Lesser General Public or the
 * Apache License along with RELIC. If not, see <https://www.gnu.org/licenses/>
 * or <https://www.apache.org/licenses/>.
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

#ifndef RLC_CORE_H
#define RLC_CORE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "relic_err.h"
#include "relic_bn.h"
#include "relic_eb.h"
#include "relic_epx.h"
#include "relic_ed.h"
#include "relic_pc.h"
#include "relic_conf.h"
#include "relic_bench.h"
#include "relic_rand.h"
#include "relic_label.h"
#include "relic_alloc.h"

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

/**
 * Indicates that the function executed correctly.
 */
#define RLC_OK			0

/**
 * Indicates that an error occurred during the function execution.
 */
#define RLC_ERR			1

/**
 * Indicates that a comparison returned that the first argument was lesser than
 * the second argument.
 */
#define RLC_LT			-1

/**
 * Indicates that a comparison returned that the first argument was equal to
 * the second argument.
 */
#define RLC_EQ			0

/**
 * Indicates that a comparison returned that the first argument was greater than
 * the second argument.
 */
#define RLC_GT			1

/**
 * Indicates that two incomparable elements are not equal.
 */
#define RLC_NE			2

/**
 * Optimization identifer for the case where a coefficient is 0.
 */
#define RLC_ZERO		0

/**
 * Optimization identifer for the case where a coefficient is 1.
 */
#define RLC_ONE			1

/**
 * Optimization identifer for the case where a coefficient is 2.
 */
#define RLC_TWO			2

/**
 * Optimization identifier for the case where a coefficient is -3.
 */
#define RLC_MIN3		3

/**
 * Optimization identifer for the case where a coefficient is small.
 */
#define RLC_TINY		4

/**
 * Optimization identifier for the case where the coefficient is arbitrary.
 */
#define RLC_HUGE		5

/**
 * Maximum number of terms to describe a sparse object.
 */
#define RLC_TERMS		16

/*============================================================================*/
/* Type definitions                                                           */
/*============================================================================*/

/**
 * Library context.
 */
typedef struct _ctx_t {
	/** The value returned by the last call, can be RLC_OK or RLC_ERR. */
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
	fb_st fb_half[(RLC_DIG / 8 + 1) * RLC_FB_DIGS][16];
#endif /* FB_SLV == QUICK */
#if FB_SRT == QUICK || !defined(STRIP)
	/** Square root of z. */
	fb_st fb_srz;
#ifdef FB_PRECO
	/** Multiplication table for the polynomial z^(1/2). */
	fb_st fb_tab_srz[256];
#endif /* FB_PRECO */
#endif /* FB_SRT == QUICK */
#if FB_INV == ITOHT || !defined(STRIP)
	/** Stores an addition chain for (RLC_FB_BITS - 1). */
	int chain[RLC_TERMS + 1];
	/** Stores the length of the addition chain. */
	int chain_len;
	/** Tables for repeated squarings. */
	fb_st fb_tab_sqr[RLC_TERMS][RLC_FB_TABLE];
#endif /* FB_INV == ITOHT */
#endif /* WITH_FB */

#ifdef WITH_EB
	/** Identifier of the currently configured binary elliptic curve. */
	int eb_id;
	/** The a-coefficient of the elliptic curve. */
	fb_st eb_a;
	/** The b-coefficient of the elliptic curve. */
	fb_st eb_b;
	/** Optimization identifier for the a-coefficient. */
	int eb_opt_a;
	/** Optimization identifier for the b-coefficient. */
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
	eb_st eb_pre[RLC_EB_TABLE];
	/** Array of pointers to the precomputation table. */
	eb_st *eb_ptr[RLC_EB_TABLE];
#endif /* EB_PRECO */
#endif /* WITH_EB */

#ifdef WITH_FP
	/** Identifier of the currently configured prime field. */
	int fp_id;
	/** Prime modulus. */
	bn_st prime;
	/** Parameter for generating prime. */
	bn_st par;
	/** Parameter in sparse form. */
	int par_sps[RLC_TERMS + 1];
	/** Length of sparse prime representation. */
	int par_len;
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
	/** 2-adicity. */
	int ad2;
#if FP_RDC == QUICK || !defined(STRIP)
	/** Sparse representation of prime modulus. */
	int sps[RLC_TERMS + 1];
	/** Length of sparse prime representation. */
	int sps_len;
#endif /* FP_RDC == QUICK */
#endif /* WITH_FP */

#ifdef WITH_EP
	/** Identifier of the currently configured prime elliptic curve. */
	int ep_id;
	/** The a-coefficient of the elliptic curve. */
	fp_st ep_a;
	/** The b-coefficient of the elliptic curve. */
	fp_st ep_b;
	/** The value 3b used in elliptic curve arithmetic. */
	fp_st ep_b3;
	/** The generator of the elliptic curve. */
	ep_st ep_g;
	/** The order of the group of points in the elliptic curve. */
	bn_st ep_r;
	/** The cofactor of the group order in the elliptic curve. */
	bn_st ep_h;
	/** The distinguished non-square used by the mapping function */
	fp_st ep_map_u;
	/** Precomputed constants for hashing. */
	fp_st ep_map_c[4];
#ifdef EP_ENDOM
#if EP_MUL == LWNAF || EP_FIX == COMBS || EP_FIX == LWNAF || EP_SIM == INTER || !defined(STRIP)
	/** Parameters required by the GLV method. @{ */
	fp_st beta;
	bn_st ep_v1[3];
	bn_st ep_v2[3];
	/** @} */
#endif /* EP_MUL */
#endif /* EP_ENDOM */
	/** Optimization identifier for the a-coefficient. */
	int ep_opt_a;
	/** Optimization identifier for the b-coefficient. */
	int ep_opt_b;
	/** Optimization identifier for the b3 value. */
	int ep_opt_b3;
	/** Flag that stores if the prime curve has efficient endomorphisms. */
	int ep_is_endom;
	/** Flag that stores if the prime curve is supersingular. */
	int ep_is_super;
	/** Flag that stores if the prime curve is pairing-friendly. */
	int ep_is_pairf;
	/** Flag that indicates whether this curve uses an isogeny for the SSWU mapping. */
	int ep_is_ctmap;
#ifdef EP_PRECO
	/** Precomputation table for generator multiplication. */
	ep_st ep_pre[RLC_EP_TABLE];
	/** Array of pointers to the precomputation table. */
	ep_st *ep_ptr[RLC_EP_TABLE];
#endif /* EP_PRECO */
#ifdef EP_CTMAP
	/** The isogeny map coefficients for the SSWU mapping. */
	iso_st ep_iso;
#endif /* EP_CTMAP */
#endif /* WITH_EP */

#ifdef WITH_EPX
	/** The generator of the elliptic curve. */
	ep2_t ep2_g;
	/** The 'a' coefficient of the curve. */
	fp2_t ep2_a;
	/** The 'b' coefficient of the curve. */
	fp2_t ep2_b;
	/** The order of the group of points in the elliptic curve. */
	bn_st ep2_r;
	/** The cofactor of the group order in the elliptic curve. */
	bn_st ep2_h;
	/** The distinguished non-square used by the mapping function */
	fp2_t ep2_map_u;
	/** The constants needed for hashing. */
	fp2_t ep2_map_c[4];
	/** The constants needed for Frobenius. */
	fp2_t ep2_frb[2];
	/** Optimization identifier for the a-coefficient. */
	int ep2_opt_a;
	/** Optimization identifier for the b-coefficient. */
	int ep2_opt_b;
	/** Flag that stores if the prime curve is a twist. */
	int ep2_is_twist;
	/** Flag that indicates whether this curve uses an isogeny for the SSWU mapping. */
	int ep2_is_ctmap;
#ifdef EP_PRECO
	/** Precomputation table for generator multiplication.*/
	ep2_st ep2_pre[RLC_EP_TABLE];
	/** Array of pointers to the precomputation table. */
	ep2_st *ep2_ptr[RLC_EP_TABLE];
#endif /* EP_PRECO */
#ifdef EP_CTMAP
	/** The isogeny map coefficients for the SSWU mapping. */
	iso2_st ep2_iso;
#endif /* EP_CTMAP */
	/** The generator of the elliptic curve. */
	ep4_t ep4_g;
	/** The 'a' coefficient of the curve. */
	fp4_t ep4_a;
	/** The 'b' coefficient of the curve. */
	fp4_t ep4_b;
	/** The order of the group of points in the elliptic curve. */
	bn_st ep4_r;
	/** The cofactor of the group order in the elliptic curve. */
	bn_st ep4_h;
	/** Optimization identifier for the a-coefficient. */
	int ep4_opt_a;
	/** Optimization identifier for the b-coefficient. */
	int ep4_opt_b;
	/** Flag that stores if the prime curve is a twist. */
	int ep4_is_twist;
#ifdef EP_PRECO
	/** Precomputation table for generator multiplication.*/
	ep4_st ep4_pre[RLC_EP_TABLE];
	/** Array of pointers to the precomputation table. */
	ep4_st *ep4_ptr[RLC_EP_TABLE];
	#endif /* EP_PRECO */
#endif /* WITH_EPX */

#ifdef WITH_ED
	/** Identifier of the currently configured Edwards elliptic curve. */
	int ed_id;
	/** The 'a' coefficient of the Edwards elliptic curve. */
	fp_st ed_a;
	/** The 'd' coefficient of the Edwards elliptic curve. */
	fp_st ed_d;
	/** Precomputed constants for hashing. */
	fp_st ed_map_c[4];
	/** The generator of the Edwards elliptic curve. */
	ed_st ed_g;
	/** The order of the group of points in the Edwards elliptic curve. */
	bn_st ed_r;
	/** The cofactor of the Edwards elliptic curve. */
	bn_st ed_h;

#ifdef ED_PRECO
	/** Precomputation table for generator multiplication. */
	ed_st ed_pre[RLC_ED_TABLE];
	/** Array of pointers to the precomputation table. */
	ed_st *ed_ptr[RLC_ED_TABLE];
#endif /* ED_PRECO */
#endif

#if defined(WITH_FPX) || defined(WITH_PP)
	/** Integer part of the quadratic non-residue. */
	dis_t qnr2;
	/** Constants for computing Frobenius maps in higher extensions. @{ */
	fp2_st fp2_p1[5];
	fp2_st fp2_p2[3];
	fp2_st fp4_p1;
	/** @} */
	/** Constants for computing Frobenius maps in higher extensions. @{ */
	int frb3[3];
	fp_st fp3_p0[2];
	fp_st fp3_p1[5];
	fp_st fp3_p2[2];
	/** @} */
#endif /* WITH_PP */

#if defined(WITH_PC)
	gt_t gt_g;
#endif

#if BENCH > 0
	/** Stores the time measured before the execution of the benchmark. */
	ben_t before;
	/** Stores the time measured after the execution of the benchmark. */
	ben_t after;
	/** Stores the sum of timings for the current benchmark. */
	long long total;
#ifdef OVERH
	/** Benchmarking overhead to be measured and subtracted from benchmarks. */
	long long over;
#endif
#endif

#if RAND != CALL
	/** Internal state of the PRNG. */
	uint8_t rand[RLC_RAND_SIZE];
#else
	void (*rand_call)(uint8_t *, int, void *);
	void *rand_args;
#endif
	/** Flag to indicate if PRNG is seed. */
	int seeded;
	/** Counter to keep track of number of calls since last seeding. */
	int counter;

#if TIMER == PERF
	/** File descriptor for perf system call. */
	int perf_fd;
	/** Buffer for storing perf data, */
	struct perf_event_mmap_page *perf_buf;
#endif
} ctx_t;

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Initializes the library.
 *
 * @return RLC_OK if no error occurs, RLC_ERR otherwise.
 */
int core_init(void);

/**
 * Finalizes the library with the current error condition.
 *
 * @return RLC_OK if no error has occurred, RLC_ERR otherwise.
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

#if defined(MULTI)
/**
 * Set an initializer function which is called when the context
 * is uninitialized. This function is called for every thread.
 *
 * @param[in] init function to call when the current context is not initialized
 * @param[in] init_ptr a pointer which is passed to the initialized
 */
void core_set_thread_initializer(void (*init)(void *init_ptr), void *init_ptr);
#endif

#endif /* !RLC_CORE_H */
