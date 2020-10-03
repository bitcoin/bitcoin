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

/**f
 * @file
 *
 * Implementation of the prime elliptic curve utilities.
 *
 * @ingroup ep
 */

#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Detects an optimization based on the curve coefficients.
 *
 * @param[out] opt		- the resulting optimization.
 * @param[in] a			- the curve coefficient.
 */
static void detect_opt(int *opt, fp_t a) {
	fp_t t;

	fp_null(t);

	TRY {
		fp_new(t);
		fp_prime_conv_dig(t, 3);
		fp_neg(t, t);

		if (fp_cmp(a, t) == CMP_EQ) {
			*opt = OPT_MINUS3;
		} else {
			if (fp_is_zero(a)) {
				*opt = OPT_ZERO;
			} else {
				fp_set_dig(t, 1);
				if (fp_cmp_dig(a, 1) == CMP_EQ) {
					*opt = OPT_ONE;
				} else {
					if (fp_cmp_dig(a, 2) == CMP_EQ) {
						*opt = OPT_TWO;
					} else {
						if (fp_bits(a) <= FP_DIGIT) {
							*opt = OPT_DIGIT;
						} else {
							*opt = RELIC_OPT_NONE;
						}
					}
				}
			}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp_free(t);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ep_curve_init(void) {
	ctx_t *ctx = core_get();
#ifdef EP_PRECO
	for (int i = 0; i < RELIC_EP_TABLE; i++) {
		ctx->ep_ptr[i] = &(ctx->ep_pre[i]);
	}
#endif
#if ALLOC == STATIC
	fp_new(ctx->ep_g.x);
	fp_new(ctx->ep_g.y);
	fp_new(ctx->ep_g.z);
#ifdef EP_PRECO
	for (int i = 0; i < RELIC_EP_TABLE; i++) {
		fp_new(ctx->ep_pre[i].x);
		fp_new(ctx->ep_pre[i].y);
		fp_new(ctx->ep_pre[i].z);
	}
#endif
#endif
	ep_set_infty(&ctx->ep_g);
	bn_init(&ctx->ep_r, FP_DIGS);
	bn_init(&ctx->ep_h, FP_DIGS);
#if defined(EP_ENDOM) && (EP_MUL == LWNAF || EP_FIX == COMBS || EP_FIX == LWNAF || !defined(STRIP))
	for (int i = 0; i < 3; i++) {
		bn_init(&(ctx->ep_v1[i]), FP_DIGS);
		bn_init(&(ctx->ep_v2[i]), FP_DIGS);
	}
#endif
}

void ep_curve_clean(void) {
	ctx_t *ctx = core_get();
#if ALLOC == STATIC
	fp_free(ctx->ep_g.x);
	fp_free(ctx->ep_g.y);
	fp_free(ctx->ep_g.z);
#ifdef EP_PRECO
	for (int i = 0; i < RELIC_EP_TABLE; i++) {
		fp_free(ctx->ep_pre[i].x);
		fp_free(ctx->ep_pre[i].y);
		fp_free(ctx->ep_pre[i].z);
	}
#endif
#endif
	bn_clean(&ctx->ep_r);
	bn_clean(&ctx->ep_h);
#if defined(EP_ENDOM) && (EP_MUL == LWNAF || EP_FIX == LWNAF || !defined(STRIP))
	for (int i = 0; i < 3; i++) {
		bn_clean(&(ctx->ep_v1[i]));
		bn_clean(&(ctx->ep_v2[i]));
	}
#endif
}

dig_t *ep_curve_get_b(void) {
	return core_get()->ep_b;
}

dig_t *ep_curve_get_a(void) {
	return core_get()->ep_a;
}

#if defined(EP_ENDOM) && (EP_MUL == LWNAF || EP_FIX == COMBS || EP_FIX == LWNAF || EP_SIM == INTER || !defined(STRIP))

dig_t *ep_curve_get_beta(void) {
	return core_get()->beta;
}

void ep_curve_get_v1(bn_t v[]) {
	ctx_t *ctx = core_get();
	for (int i = 0; i < 3; i++) {
		bn_copy(v[i], &(ctx->ep_v1[i]));
	}
}

void ep_curve_get_v2(bn_t v[]) {
	ctx_t *ctx = core_get();
	for (int i = 0; i < 3; i++) {
		bn_copy(v[i], &(ctx->ep_v2[i]));
	}
}

#endif

int ep_curve_opt_a(void) {
	return core_get()->ep_opt_a;
}

int ep_curve_opt_b(void) {
	return core_get()->ep_opt_b;
}

int ep_curve_is_endom(void) {
	return core_get()->ep_is_endom;
}

int ep_curve_is_super(void) {
	return core_get()->ep_is_super;
}

void ep_curve_get_gen(ep_t g) {
	ep_copy(g, &core_get()->ep_g);
}

void ep_curve_get_ord(bn_t n) {
	bn_copy(n, &core_get()->ep_r);
}

void ep_curve_get_cof(bn_t h) {
	bn_copy(h, &core_get()->ep_h);
}

const ep_t *ep_curve_get_tab(void) {
#if defined(EP_PRECO)

	/* Return a meaningful pointer. */
#if ALLOC == AUTO
	return (const ep_t *)*core_get()->ep_ptr;
#else
	return (const ep_t *)core_get()->ep_ptr;
#endif

#else
	/* Return a null pointer. */
	return NULL;
#endif
}

#if defined(EP_PLAIN)

void ep_curve_set_plain(const fp_t a, const fp_t b, const ep_t g, const bn_t r,
		const bn_t h) {
	ctx_t *ctx = core_get();
	ctx->ep_is_endom = 0;
	ctx->ep_is_super = 0;

	fp_copy(ctx->ep_a, a);
	fp_copy(ctx->ep_b, b);

	detect_opt(&(ctx->ep_opt_a), ctx->ep_a);
	detect_opt(&(ctx->ep_opt_b), ctx->ep_b);

	ep_norm(&(ctx->ep_g), g);
	bn_copy(&(ctx->ep_r), r);
	bn_copy(&(ctx->ep_h), h);

#if defined(EP_PRECO)
	ep_mul_pre((ep_t *)ep_curve_get_tab(), &(ctx->ep_g));
#endif
}

#endif

#if defined(EP_SUPER)

void ep_curve_set_super(const fp_t a, const fp_t b, const ep_t g, const bn_t r,
		const bn_t h) {
	ctx_t *ctx = core_get();
	ctx->ep_is_endom = 0;
	ctx->ep_is_super = 1;

	fp_copy(ctx->ep_a, a);
	fp_copy(ctx->ep_b, b);

	detect_opt(&(ctx->ep_opt_a), ctx->ep_a);
	detect_opt(&(ctx->ep_opt_b), ctx->ep_b);

	ep_norm(&(ctx->ep_g), g);
	bn_copy(&(ctx->ep_r), r);
	bn_copy(&(ctx->ep_h), h);

#if defined(EP_PRECO)
	ep_mul_pre((ep_t *)ep_curve_get_tab(), &(ctx->ep_g));
#endif
}

#endif

#if defined(EP_ENDOM)

void ep_curve_set_endom(const fp_t b, const ep_t g, const bn_t r, const bn_t h,
		const fp_t beta, const bn_t l) {
	int bits = bn_bits(r);
	ctx_t *ctx = core_get();
	ctx->ep_is_endom = 1;
	ctx->ep_is_super = 0;

	fp_zero(ctx->ep_a);
	fp_copy(ctx->ep_b, b);

	detect_opt(&(ctx->ep_opt_a), ctx->ep_a);
	detect_opt(&(ctx->ep_opt_b), ctx->ep_b);

#if EP_MUL == LWNAF || EP_FIX == COMBS || EP_FIX == LWNAF || EP_SIM == INTER || !defined(STRIP)
	fp_copy(ctx->beta, beta);
	bn_gcd_ext_mid(&(ctx->ep_v1[1]), &(ctx->ep_v1[2]), &(ctx->ep_v2[1]),
			&(ctx->ep_v2[2]), l, r);
	/* l = v1[1] * v2[2] - v1[2] * v2[1], r = l / 2. */
	bn_mul(&(ctx->ep_v1[0]), &(ctx->ep_v1[1]), &(ctx->ep_v2[2]));
	bn_mul(&(ctx->ep_v2[0]), &(ctx->ep_v1[2]), &(ctx->ep_v2[1]));
	bn_sub(&(ctx->ep_r), &(ctx->ep_v1[0]), &(ctx->ep_v2[0]));
	bn_hlv(&(ctx->ep_r), &(ctx->ep_r));
	/* v1[0] = round(v2[2] * 2^|n| / l). */
	bn_lsh(&(ctx->ep_v1[0]), &(ctx->ep_v2[2]), bits + 1);
	if (bn_sign(&(ctx->ep_v1[0])) == BN_POS) {
		bn_add(&(ctx->ep_v1[0]), &(ctx->ep_v1[0]), &(ctx->ep_r));
	} else {
		bn_sub(&(ctx->ep_v1[0]), &(ctx->ep_v1[0]), &(ctx->ep_r));
	}
	bn_dbl(&(ctx->ep_r), &(ctx->ep_r));
	bn_div(&(ctx->ep_v1[0]), &(ctx->ep_v1[0]), &(ctx->ep_r));
	if (bn_sign(&ctx->ep_v1[0]) == BN_NEG) {
		bn_add_dig(&(ctx->ep_v1[0]), &(ctx->ep_v1[0]), 1);
	}
	/* v2[0] = round(v1[2] * 2^|n| / l). */
	bn_lsh(&(ctx->ep_v2[0]), &(ctx->ep_v1[2]), bits + 1);
	if (bn_sign(&(ctx->ep_v2[0])) == BN_POS) {
		bn_add(&(ctx->ep_v2[0]), &(ctx->ep_v2[0]), &(ctx->ep_r));
	} else {
		bn_sub(&(ctx->ep_v2[0]), &(ctx->ep_v2[0]), &(ctx->ep_r));
	}
	bn_div(&(ctx->ep_v2[0]), &(ctx->ep_v2[0]), &(ctx->ep_r));
	if (bn_sign(&ctx->ep_v2[0]) == BN_NEG) {
		bn_add_dig(&(ctx->ep_v2[0]), &(ctx->ep_v2[0]), 1);
	}
	bn_neg(&(ctx->ep_v2[0]), &(ctx->ep_v2[0]));
#endif

	ep_norm(&(ctx->ep_g), g);
	bn_copy(&(ctx->ep_r), r);
	bn_copy(&(ctx->ep_h), h);

#if defined(EP_PRECO)
	ep_mul_pre((ep_t *)ep_curve_get_tab(), &(ctx->ep_g));
#endif
}

#endif
