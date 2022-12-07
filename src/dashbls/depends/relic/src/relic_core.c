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
 * @file
 *
 * Implementation of the library basic functions.
 *
 * @ingroup relic
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "relic_core.h"
#include "relic_bench.h"
#include "relic_multi.h"
#include "relic_rand.h"
#include "relic_types.h"
#include "relic_err.h"
#include "relic_arch.h"
#include "relic_fp.h"
#include "relic_fb.h"
#include "relic_ep.h"
#include "relic_eb.h"
#include "relic_cp.h"
#include "relic_pp.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/** Error message respective to ERR_NO_MEMORY. */
#define MSG_NO_MEMORY 		"not enough memory"
/** Error message respective to ERR_PRECISION. */
#define MSG_NO_PRECI 		"insufficient precision"
/** Error message respective to ERR_NO FILE. */
#define MSG_NO_FILE			"file not found"
/** Error message respective to ERR_NO_READ. */
#define MSG_NO_READ			"can't read bytes from file"
/** Error message respective to ERR_NO_VALID. */
#define MSG_NO_VALID		"invalid value passed as input"
/** Error message respective to ERR_NO_BUFFER. */
#define MSG_NO_BUFFER		"insufficient buffer capacity"
/** Error message respective to ERR_NO_FIELD. */
#define MSG_NO_FIELD		"no finite field supported at this security level"
/** Error message respective to ERR_NO_CURVE. */
#define MSG_NO_CURVE		"no curve supported at this security level"
/** Error message respective to ERR_NO_CONFIG. */
#define MSG_NO_CONFIG		"invalid library configuration"
/** Error message respective to ERR_NO_CURVE. */
#define MSG_NO_RAND			"faulty pseudo-random number generator"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

/**
 * Default library context.
 */
#if MULTI
rlc_thread ctx_t first_ctx;
#else
static ctx_t first_ctx;
#endif

#if defined(MULTI)
/*
 * Initializer function to call for every thread's context
 */
void (*core_thread_initializer)(void* init_ptr) = NULL;
void* core_init_ptr = NULL;
#endif

#if MULTI
rlc_thread ctx_t *core_ctx = NULL;
#else
static ctx_t *core_ctx = NULL;
#endif

int core_init(void) {
	if (core_ctx == NULL) {
		core_ctx = &(first_ctx);
	}

#ifdef CHECK
	core_ctx->reason[ERR_NO_MEMORY] = MSG_NO_MEMORY;
	core_ctx->reason[ERR_NO_PRECI] = MSG_NO_PRECI;
	core_ctx->reason[ERR_NO_FILE] = MSG_NO_FILE;
	core_ctx->reason[ERR_NO_READ] = MSG_NO_READ;
	core_ctx->reason[ERR_NO_VALID] = MSG_NO_VALID;
	core_ctx->reason[ERR_NO_BUFFER] = MSG_NO_BUFFER;
	core_ctx->reason[ERR_NO_FIELD] = MSG_NO_FIELD;
	core_ctx->reason[ERR_NO_CURVE] = MSG_NO_CURVE;
	core_ctx->reason[ERR_NO_CONFIG] = MSG_NO_CONFIG;
	core_ctx->reason[ERR_NO_RAND] = MSG_NO_RAND;
	core_ctx->last = NULL;
#endif /* CHECK */

	core_ctx->code = RLC_OK;

	RLC_TRY {
		arch_init();
		rand_init();

#if BENCH > 0
		bench_init();
#endif

#ifdef WITH_FP
		fp_prime_init();
#endif
#ifdef WITH_FB
		fb_poly_init();
#endif
#ifdef WITH_EP
		ep_curve_init();
#endif
#ifdef WITH_EB
		eb_curve_init();
#endif
#ifdef WITH_ED
		ed_curve_init();
#endif
#ifdef WITH_PP
		pp_map_init();
#endif
#ifdef WITH_PC
		pc_core_init();
#endif
	} RLC_CATCH_ANY {
		return RLC_ERR;
	}

	return RLC_OK;
}

int core_clean(void) {
#ifdef WITH_FP
	fp_prime_clean();
#endif
#ifdef WITH_FB
	fb_poly_clean();
#endif
#ifdef WITH_EP
	ep_curve_clean();
#endif
#ifdef WITH_EB
	eb_curve_clean();
#endif
#ifdef WITH_ED
	ed_curve_clean();
#endif
#ifdef WITH_PP
	pp_map_clean();
#endif
#ifdef WITH_PC
	pc_core_clean();
#endif

#if BENCH > 0
		bench_clean();
#endif

	arch_clean();
	rand_clean();

	if (core_ctx != NULL) {
		int result = core_ctx->code;
		core_ctx = NULL;
		return result;
	}
	return RLC_OK;
}

ctx_t *core_get(void) {
#if defined(MULTI)
    if (core_ctx == NULL && core_thread_initializer != NULL) {
        core_thread_initializer(core_init_ptr);
    }
#endif
	return core_ctx;
}

void core_set(ctx_t *ctx) {
	core_ctx = ctx;
}

#if defined(MULTI)
void core_set_thread_initializer(void(*init)(void *init_ptr), void* init_ptr) {
    core_thread_initializer = init;
    core_init_ptr = init_ptr;
}
#endif
