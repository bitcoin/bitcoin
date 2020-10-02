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
 * @file
 *
 * Implementation of the binary elliptic curve utilities.
 *
 * @ingroup eb
 */

#include "relic_core.h"
#include "relic_eb.h"
#include "relic_conf.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Detects an optimization based on the curve coefficients.
 *
 * @param opt		- the resulting optimization.
 * @param a			- the curve coefficient.
 */
static void detect_opt(int *opt, fb_t a) {
	if (fb_is_zero(a)) {
		*opt = OPT_ZERO;
	} else {
		if (fb_cmp_dig(a, 1) == CMP_EQ) {
			*opt = OPT_ONE;
		} else {
			if (fb_bits(a) <= FB_DIGIT) {
				*opt = OPT_DIGIT;
			} else {
				*opt = RELIC_OPT_NONE;
			}
		}
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void eb_curve_init(void) {
	ctx_t *ctx = core_get();
#ifdef EB_PRECO
	for (int i = 0; i < RELIC_EB_TABLE; i++) {
		ctx->eb_ptr[i] = &(ctx->eb_pre[i]);
	}
#endif
#if ALLOC == STATIC
	fb_new(ctx->eb_g.x);
	fb_new(ctx->eb_g.y);
	fb_new(ctx->eb_g.z);
	for (int i = 0; i < RELIC_EB_TABLE; i++) {
		fb_new(ctx->eb_pre[i].x);
		fb_new(ctx->eb_pre[i].y);
		fb_new(ctx->eb_pre[i].z);
	}
#endif
	fb_zero(ctx->eb_g.x);
	fb_zero(ctx->eb_g.y);
	fb_zero(ctx->eb_g.z);
	bn_init(&(ctx->eb_r), FB_DIGS);
	bn_init(&(ctx->eb_h), FB_DIGS);
}

void eb_curve_clean(void) {
	ctx_t *ctx = core_get();
#if ALLOC == STATIC
	fb_free(ctx->eb_g.x);
	fb_free(ctx->eb_g.y);
	fb_free(ctx->eb_g.z);
	for (int i = 0; i < RELIC_EB_TABLE; i++) {
		fb_free(ctx->eb_pre[i].x);
		fb_free(ctx->eb_pre[i].y);
		fb_free(ctx->eb_pre[i].z);
	}
#endif
	bn_clean(&(ctx->eb_r));
	bn_clean(&(ctx->eb_h));
}

dig_t *eb_curve_get_a(void) {
	return core_get()->eb_a;
}

int eb_curve_opt_a(void) {
	return core_get()->eb_opt_a;
}

dig_t *eb_curve_get_b(void) {
	return core_get()->eb_b;
}

int eb_curve_opt_b(void) {
	return core_get()->eb_opt_b;
}

int eb_curve_is_kbltz(void) {
	return core_get()->eb_is_kbltz;
}

void eb_curve_get_gen(eb_t g) {
	eb_copy(g, &(core_get()->eb_g));
}

void eb_curve_get_ord(bn_t n) {
	bn_copy(n, &(core_get()->eb_r));
}

void eb_curve_get_cof(bn_t h) {
	bn_copy(h, &(core_get()->eb_h));
}

const eb_t *eb_curve_get_tab(void) {
#if defined(EB_PRECO)

	/* Return a meaningful pointer. */
#if ALLOC == AUTO
	return (const eb_t *)*(core_get()->eb_ptr);
#else
	return (const eb_t *)core_get()->eb_ptr;
#endif

#else
	/* Return a null pointer. */
	return NULL;
#endif
}

void eb_curve_set(const fb_t a, const fb_t b, const eb_t g, const bn_t r,
		const bn_t h) {
	ctx_t *ctx = core_get();
	fb_copy(ctx->eb_a, a);
	fb_copy(ctx->eb_b, b);

	detect_opt(&(ctx->eb_opt_a), ctx->eb_a);
	detect_opt(&(ctx->eb_opt_b), ctx->eb_b);

	if (fb_cmp_dig(ctx->eb_b, 1) == CMP_EQ) {
		ctx->eb_is_kbltz = 1;
	} else {
		ctx->eb_is_kbltz = 0;
	}
	eb_norm(&(ctx->eb_g), g);
	bn_copy(&(ctx->eb_r), r);
	bn_copy(&(ctx->eb_h), h);
#if defined(EB_PRECO)
	eb_mul_pre((eb_t *)eb_curve_get_tab(), &(ctx->eb_g));
#endif
}
