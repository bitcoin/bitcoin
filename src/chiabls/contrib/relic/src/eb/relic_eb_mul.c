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
 * Implementation of point multiplication on binary elliptic curves.
 *
 * @ingroup eb
 */

#include "relic_core.h"
#include "relic_fb_low.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if EB_MUL == LWNAF || !defined(STRIP)

#if defined(EB_KBLTZ)

/**
 * Multiplies a binary elliptic curve point by an integer using the w-TNAF
 * method.
 *
 * @param[out] r 				- the result.
 * @param[in] p					- the point to multiply.
 * @param[in] k					- the integer.
 */
static void eb_mul_ltnaf_imp(eb_t r, const eb_t p, const bn_t k) {
	int i, l, n;
	int8_t tnaf[FB_BITS + 8], u;
	eb_t t[1 << (EB_WIDTH - 2)];

	if (eb_curve_opt_a() == OPT_ZERO) {
		u = -1;
	} else {
		u = 1;
	}

	TRY {
		/* Prepare the precomputation table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_null(t[i]);
			eb_new(t[i]);
		}
		/* Compute the precomputation table. */
		eb_tab(t, p, EB_WIDTH);

		/* Compute the w-TNAF representation of k. */
		l = sizeof(tnaf);
		bn_rec_tnaf(tnaf, &l, k, u, FB_BITS, EB_WIDTH);

		n = tnaf[l - 1];
		if (n > 0) {
			eb_copy(r, t[n / 2]);
		} else {
			eb_neg(r, t[-n / 2]);
		}

		for (i = l - 2; i >= 0; i--) {
			eb_frb(r, r);

			n = tnaf[i];
			if (n > 0) {
				eb_add(r, r, t[n / 2]);
			}
			if (n < 0) {
				eb_sub(r, r, t[-n / 2]);
			}
		}
		/* Convert r to affine coordinates. */
		eb_norm(r, r);
		if (bn_sign(k) == BN_NEG) {
			eb_neg(r, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		/* Free the precomputation table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_free(t[i]);
		}
	}
}

#endif

#if defined(EB_PLAIN)

/**
 * Multiplies a binary elliptic curve point by an integer using the
 * left-to-right w-NAF method.
 *
 * @param[out] r 				- the result.
 * @param[in] p					- the point to multiply.
 * @param[in] k					- the integer.
 */
static void eb_mul_lnaf_imp(eb_t r, const eb_t p, const bn_t k) {
	int i, l, n;
	int8_t naf[FB_BITS + 1];
	eb_t t[1 << (EB_WIDTH - 2)];

	TRY {
		/* Prepare the precomputation table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_null(t[i]);
			eb_new(t[i]);
			eb_set_infty(t[i]);
			fb_set_dig(t[i]->z, 1);
			t[i]->norm = 1;
		}

		/* Compute the precomputation table. */
		eb_tab(t, p, EB_WIDTH);

		/* Compute the w-NAF representation of k. */
		l = sizeof(naf);
		bn_rec_naf(naf, &l, k, EB_WIDTH);

		n = naf[l - 1];
		if (n > 0) {
			eb_copy(r, t[n / 2]);
		}

		for (i = l - 2; i >= 0; i--) {
			eb_dbl(r, r);

			n = naf[i];
			if (n > 0) {
				eb_add(r, r, t[n / 2]);
			}
			if (n < 0) {
				eb_sub(r, r, t[-n / 2]);
			}
		}
		/* Convert r to affine coordinates. */
		eb_norm(r, r);
		if (bn_sign(k) == BN_NEG) {
			eb_neg(r, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		/* Free the precomputation table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_free(t[i]);
		}
	}
}

#endif /* EB_PLAIN */
#endif /* EB_MUL == LWNAF */

#if EB_MUL == RWNAF || !defined(STRIP)

#if defined(EB_KBLTZ)

/**
 * Multiplies a binary elliptic curve point by an integer using the w-TNAF
 * method.
 *
 * @param[out] r 				- the result.
 * @param[in] p					- the point to multiply.
 * @param[in] k					- the integer.
 */
static void eb_mul_rtnaf_imp(eb_t r, const eb_t p, const bn_t k) {
	int i, l, n;
	int8_t tnaf[FB_BITS + 8], u;
	eb_t t[1 << (EB_WIDTH - 2)];

	if (eb_curve_opt_a() == OPT_ZERO) {
		u = -1;
	} else {
		u = 1;
	}

	TRY {
		/* Prepare the precomputation table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_null(t[i]);
			eb_new(t[i]);
			eb_set_infty(t[i]);
		}

		/* Compute the w-TNAF representation of k. */
		l = sizeof(tnaf);
		bn_rec_tnaf(tnaf, &l, k, u, FB_BITS, EB_WIDTH);

		eb_copy(r, p);
		for (i = 0; i < l; i++) {
			n = tnaf[i];
			if (n > 0) {
				eb_add(t[n / 2], t[n / 2], r);
			}
			if (n < 0) {
				eb_sub(t[-n / 2], t[-n / 2], r);
			}

			/* We can avoid a function call here. */
			fb_sqr(r->x, r->x);
			fb_sqr(r->y, r->y);
		}

		eb_copy(r, t[0]);

#if defined(EB_MIXED) && defined(STRIP) && (EB_WIDTH > 2)
		eb_norm_sim(t + 1, (const eb_t *)t + 1, (1 << (EB_WIDTH - 2)) - 1);
#endif

#if EB_WIDTH == 3
		eb_frb(t[0], t[1]);
		if (u == 1) {
			eb_sub(t[1], t[1], t[0]);
		} else {
			eb_add(t[1], t[1], t[0]);
		}
#endif

#if EB_WIDTH == 4
		eb_frb(t[0], t[3]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);

		if (u == 1) {
			eb_neg(t[0], t[0]);
		}
		eb_sub(t[3], t[0], t[3]);

		eb_frb(t[0], t[1]);
		eb_frb(t[0], t[0]);
		eb_sub(t[1], t[0], t[1]);

		eb_frb(t[0], t[2]);
		eb_frb(t[0], t[0]);
		eb_add(t[2], t[0], t[2]);
#endif

#if EB_WIDTH == 5
		eb_frb(t[0], t[3]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		if (u == 1) {
			eb_neg(t[0], t[0]);
		}
		eb_sub(t[3], t[0], t[3]);

		eb_frb(t[0], t[1]);
		eb_frb(t[0], t[0]);
		eb_sub(t[1], t[0], t[1]);

		eb_frb(t[0], t[2]);
		eb_frb(t[0], t[0]);
		eb_add(t[2], t[0], t[2]);

		eb_frb(t[0], t[4]);
		eb_frb(t[0], t[0]);
		eb_add(t[0], t[0], t[4]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		if (u == 1) {
			eb_neg(t[0], t[0]);
		}
		eb_add(t[4], t[0], t[4]);

		eb_frb(t[0], t[5]);
		eb_frb(t[0], t[0]);
		eb_add(t[0], t[0], t[5]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_neg(t[0], t[0]);
		eb_sub(t[5], t[0], t[5]);

		eb_frb(t[0], t[6]);
		eb_frb(t[0], t[0]);
		eb_add(t[0], t[0], t[6]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_neg(t[0], t[0]);
		eb_add(t[6], t[0], t[6]);

		eb_frb(t[0], t[7]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_sub(t[7], t[0], t[7]);
#endif

#if EB_WIDTH == 6
		eb_frb(t[0], t[1]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		if (u == -1) {
			eb_neg(t[0], t[0]);
		}
		eb_add(t[0], t[0], t[1]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_sub(t[1], t[0], t[1]);

		eb_frb(t[0], t[2]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		if (u == -1) {
			eb_neg(t[0], t[0]);
		}
		eb_add(t[0], t[0], t[2]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_add(t[2], t[0], t[2]);

		eb_frb(t[0], t[3]);
		eb_frb(t[0], t[0]);
		eb_add(t[0], t[0], t[3]);
		eb_neg(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		if (u == -1) {
			eb_neg(t[0], t[0]);
		}
		eb_sub(t[3], t[0], t[3]);

		eb_frb(t[0], t[4]);
		eb_frb(t[0], t[0]);
		eb_add(t[0], t[0], t[4]);
		eb_neg(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		if (u == -1) {
			eb_neg(t[0], t[0]);
		}
		eb_add(t[4], t[0], t[4]);

		eb_frb(t[0], t[5]);
		eb_frb(t[0], t[0]);
		eb_add(t[0], t[0], t[5]);
		eb_neg(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_sub(t[5], t[0], t[5]);

		eb_frb(t[0], t[6]);
		eb_frb(t[0], t[0]);
		eb_add(t[0], t[0], t[6]);
		eb_neg(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_add(t[6], t[0], t[6]);

		eb_frb(t[0], t[7]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_sub(t[7], t[0], t[7]);

		eb_frb(t[0], t[8]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_add(t[8], t[0], t[8]);

		eb_frb(t[0], t[9]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		if (u == -1) {
			eb_neg(t[0], t[0]);
		}
		eb_add(t[0], t[0], t[9]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_sub(t[0], t[0], t[9]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_add(t[0], t[0], t[9]);
		eb_neg(t[9], t[0]);

		eb_frb(t[0], t[10]);
		eb_frb(t[0], t[0]);
		eb_neg(t[0], t[0]);
		eb_add(t[0], t[0], t[10]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_add(t[10], t[0], t[10]);

		eb_frb(t[0], t[11]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		if (u == -1) {
			eb_neg(t[0], t[0]);
		}
		eb_sub(t[11], t[0], t[11]);

		eb_frb(t[0], t[12]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		if (u == -1) {
			eb_neg(t[0], t[0]);
		}
		eb_add(t[12], t[0], t[12]);

		eb_frb(t[0], t[13]);
		eb_frb(t[0], t[0]);
		eb_add(t[0], t[0], t[13]);
		eb_neg(t[13], t[0]);

		eb_frb(t[0], t[14]);
		eb_frb(t[0], t[0]);
		eb_neg(t[0], t[0]);
		eb_add(t[14], t[0], t[14]);

		eb_frb(t[0], t[15]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		eb_frb(t[0], t[0]);
		if (u == -1) {
			eb_neg(t[0], t[0]);
		}
		eb_sub(t[15], t[0], t[15]);
#endif

#if defined(EB_MIXED) && defined(STRIP) && (EB_WIDTH > 2)
		eb_norm_sim(t + 1, (const eb_t *)t + 1, (1 << (EB_WIDTH - 2)) - 1);
#endif

		/* Add accumulators */
		for (i = 1; i < (1 << (EB_WIDTH - 2)); i++) {
			if (r->norm) {
				eb_add(r, t[i], r);
			} else {
				eb_add(r, r, t[i]);
			}
		}
		/* Convert r to affine coordinates. */
		eb_norm(r, r);
		if (bn_sign(k) == BN_NEG) {
			eb_neg(r, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		/* Free the precomputation table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_free(t[i]);
		}
	}
}

#endif /* EB_KBLTZ */

#if defined(EB_PLAIN)

/**
 * Multiplies a binary elliptic curve point by an integer using the
 * right-to-left w-NAF method.
 *
 * @param[out] r 				- the result.
 * @param[in] p					- the point to multiply.
 * @param[in] k					- the integer.
 */
static void eb_mul_rnaf_imp(eb_t r, const eb_t p, const bn_t k) {
	int i, l, n;
	int8_t naf[FB_BITS + 1];
	eb_t t[1 << (EB_WIDTH - 2)];

	TRY {
		/* Prepare the accumulator table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_null(t[i]);
			eb_new(t[i]);
			eb_set_infty(t[i]);
		}

		/* Compute the w-NAF representation of k. */
		l = sizeof(naf);
		bn_rec_naf(naf, &l, k, EB_WIDTH);

		eb_copy(r, p);
		for (i = 0; i < l; i++) {
			n = naf[i];
			if (n > 0) {
				eb_add(t[n / 2], t[n / 2], r);
			}
			if (n < 0) {
				eb_sub(t[-n / 2], t[-n / 2], r);
			}

			eb_dbl(r, r);
		}

		eb_copy(r, t[0]);

#if EB_WIDTH >= 3
		/* Compute 3 * T[1]. */
		eb_dbl(t[0], t[1]);
		eb_add(t[1], t[0], t[1]);
#endif
#if EB_WIDTH >= 4
		/* Compute 5 * T[2]. */
		eb_dbl(t[0], t[2]);
		eb_dbl(t[0], t[0]);
		eb_add(t[2], t[0], t[2]);

		/* Compute 7 * T[3]. */
		eb_dbl(t[0], t[3]);
		eb_dbl(t[0], t[0]);
		eb_dbl(t[0], t[0]);
		eb_sub(t[3], t[0], t[3]);
#endif
#if EB_WIDTH >= 5
		/* Compute 9 * T[4]. */
		eb_dbl(t[0], t[4]);
		eb_dbl(t[0], t[0]);
		eb_dbl(t[0], t[0]);
		eb_add(t[4], t[0], t[4]);

		/* Compute 11 * T[5]. */
		eb_dbl(t[0], t[5]);
		eb_dbl(t[0], t[0]);
		eb_add(t[0], t[0], t[5]);
		eb_dbl(t[0], t[0]);
		eb_add(t[5], t[0], t[5]);

		/* Compute 13 * T[6]. */
		eb_dbl(t[0], t[6]);
		eb_add(t[0], t[0], t[6]);
		eb_dbl(t[0], t[0]);
		eb_dbl(t[0], t[0]);
		eb_add(t[6], t[0], t[6]);

		/* Compute 15 * T[7]. */
		eb_dbl(t[0], t[7]);
		eb_dbl(t[0], t[0]);
		eb_dbl(t[0], t[0]);
		eb_dbl(t[0], t[0]);
		eb_sub(t[7], t[0], t[7]);
#endif
#if EB_WIDTH == 6
		for (i = 8; i < 15; i++) {
			eb_mul_dig(t[i], t[i], 2 * i + 1);
		}
		eb_dbl(t[0], t[15]);
		eb_dbl(t[0], t[0]);
		eb_dbl(t[0], t[0]);
		eb_dbl(t[0], t[0]);
		eb_dbl(t[0], t[0]);
		eb_sub(t[15], t[0], t[15]);
#endif

		/* Add accumulators */
		for (i = 1; i < (1 << (EB_WIDTH - 2)); i++) {
			if (r->norm) {
				eb_add(r, t[i], r);
			} else {
				eb_add(r, r, t[i]);
			}
		}
		/* Convert r to affine coordinates. */
		eb_norm(r, r);
		if (bn_sign(k) == BN_NEG) {
			eb_neg(r, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		/* Free the accumulator table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_free(t[i]);
		}
	}
}

#endif /* EB_PLAIN */
#endif /* EB_MUL == RWNAF */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if EB_MUL == BASIC || !defined(STRIP)

void eb_mul_basic(eb_t r, const eb_t p, const bn_t k) {
	eb_t t;

	if (bn_is_zero(k) || eb_is_infty(p)) {
		eb_set_infty(r);
		return;
	}

	eb_null(t);

	TRY {
		eb_new(t);

		eb_copy(t, p);
		for (int i = bn_bits(k) - 2; i >= 0; i--) {
			eb_dbl(t, t);
			if (bn_get_bit(k, i)) {
				eb_add(t, t, p);
			}
		}

		eb_norm(r, t);
		if (bn_sign(k) == BN_NEG) {
			eb_neg(r, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		eb_free(t);
	}
}

#endif

#if EB_MUL == LODAH || !defined(STRIP)

void eb_mul_lodah(eb_t r, const eb_t p, const bn_t k) {
	int i, t;
	dv_t x1, z1, x2, z2, r1, r2, r3, r4, r5;
	const dig_t *b;

	if (bn_is_zero(k)) {
		eb_set_infty(r);
		return;
	}

	dv_null(x1);
	dv_null(z1);
	dv_null(x2);
	dv_null(z2);
	dv_null(r1);
	dv_null(r2);
	dv_null(r3);
	dv_null(r4);
	dv_null(r5);

	TRY {
		dv_new(x1);
		dv_new(z1);
		dv_new(x2);
		dv_new(z2);
		dv_new(r1);
		dv_new(r2);
		dv_new(r3);
		dv_new(r4);
		dv_new(r5);

		fb_copy(x1, p->x);
		fb_zero(z1);
		fb_set_bit(z1, 0, 1);
		fb_sqr(z2, p->x);
		fb_sqr(x2, z2);
		dv_zero(r5, 2 * FB_DIGS);

		b = eb_curve_get_b();

		switch (eb_curve_opt_b()) {
			case OPT_ZERO:
				break;
			case OPT_ONE:
				fb_add_dig(x2, x2, (dig_t)1);
				break;
			case OPT_DIGIT:
				fb_add_dig(x2, x2, b[0]);
				break;
			default:
				fb_addn_low(x2, x2, b);
				break;
		}

		for (i = bn_bits(k) - 2; i >= 0; i--) {
			fb_mul(r1, x1, z2);
			fb_mul(r2, x2, z1);
			fb_add(r3, r1, r2);
			fb_muln_low(r4, r1, r2);
			t = bn_get_bit(k, i);
			dv_swap_cond(x1, x2, FB_DIGS, t ^ 1);
			dv_swap_cond(z1, z2, FB_DIGS, t ^ 1);
			fb_sqr(z1, r3);
			fb_muln_low(r1, z1, p->x);
			fb_addd_low(x1, r1, r4, 2 * FB_DIGS);
			fb_rdcn_low(x1, x1);
			fb_sqr(r1, z2);
			fb_sqr(r2, x2);
			fb_mul(z2, r1, r2);
			switch (eb_curve_opt_b()) {
				case OPT_ZERO:
					fb_sqr(x2, r2);
					break;
				case OPT_ONE:
					fb_add(r1, r1, r2);
					fb_sqr(x2, r1);
					break;
				case OPT_DIGIT:
					fb_sqr(r1, r1);
					fb_sqrl_low(x2, r2);
					fb_mul1_low(r5, r1, b[0]);
					fb_addd_low(x2, x2, r5, FB_DIGS + 1);
					fb_rdcn_low(x2, x2);
					break;
				default:
					fb_sqr(r1, r1);
					fb_sqrl_low(x2, r2);
					fb_muln_low(r5, r1, b);
					fb_addd_low(x2, x2, r5, 2 * FB_DIGS);
					fb_rdcn_low(x2, x2);
					break;
			}
			dv_swap_cond(x1, x2, FB_DIGS, t ^ 1);
			dv_swap_cond(z1, z2, FB_DIGS, t ^ 1);
		}

		if (fb_is_zero(z1)) {
			/* The point q is at infinity. */
			eb_set_infty(r);
		} else {
			if (fb_is_zero(z2)) {
				fb_copy(r->x, p->x);
				fb_add(r->y, p->x, p->y);
				fb_zero(r->z);
				fb_set_bit(r->z, 0, 1);
			} else {
				/* r3 = z1 * z2. */
				fb_mul(r3, z1, z2);
				/* z1 = (x1 + x * z1). */
				fb_mul(z1, z1, p->x);
				fb_add(z1, z1, x1);
				/* z2 = x * z2. */
				fb_mul(z2, z2, p->x);
				/* x1 = x1 * z2. */
				fb_mul(x1, x1, z2);
				/* z2 = (x2 + x * z2)(x1 + x * z1). */
				fb_add(z2, z2, x2);
				fb_mul(z2, z2, z1);

				/* r4 = (x^2 + y) * z1 * z2 + (x2 + x * z2)(x1 + x * z1). */
				fb_sqr(r4, p->x);
				fb_add(r4, r4, p->y);
				fb_mul(r4, r4, r3);
				fb_add(r4, r4, z2);

				/* r3 = (z1 * z2 * x)^{-1}. */
				fb_mul(r3, r3, p->x);
				fb_inv(r3, r3);
				/* r4 = (x^2 + y) * z1 * z2 + (x2 + x * z2)(x1 + x * z1) * r3. */
				fb_mul(r4, r4, r3);
				/* x2 = x1 * x * z2 * (z1 * z2 * x)^{-1} = x1/z1. */
				fb_mul(x2, x1, r3);
				/* z2 = x + x1/z1. */
				fb_add(z2, x2, p->x);

				/* z2 = z2 * r4 + y. */
				fb_mul(z2, z2, r4);
				fb_add(z2, z2, p->y);

				fb_copy(r->x, x2);
				fb_copy(r->y, z2);
				fb_zero(r->z);
				fb_set_bit(r->z, 0, 1);

				r->norm = 1;
			}
		}

		if (bn_sign(k) == BN_NEG) {
			eb_neg(r, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(x1);
		dv_free(z1);
		dv_free(x2);
		dv_free(z2);
		dv_free(r1);
		dv_free(r2);
		dv_free(r3);
		dv_free(r4);
		dv_free(r5);
	}
}

#endif /* EB_MUL == LODAH */

#if EB_MUL == LWNAF || !defined(STRIP)

void eb_mul_lwnaf(eb_t r, const eb_t p, const bn_t k) {
	if (bn_is_zero(k) || eb_is_infty(p)) {
		eb_set_infty(r);
		return;
	}

#if defined(EB_KBLTZ)
	if (eb_curve_is_kbltz()) {
		eb_mul_ltnaf_imp(r, p, k);
		return;
	}
#endif

#if defined(EB_PLAIN)
	eb_mul_lnaf_imp(r, p, k);
#endif
}

#endif

#if EB_MUL == RWNAF || !defined(STRIP)

void eb_mul_rwnaf(eb_t r, const eb_t p, const bn_t k) {
	if (bn_is_zero(k) || eb_is_infty(p)) {
		eb_set_infty(r);
		return;
	}

#if defined(EB_KBLTZ)
	if (eb_curve_is_kbltz()) {
		eb_mul_rtnaf_imp(r, p, k);
		return;
	}
#endif

#if defined(EB_PLAIN)
#if defined(EB_MIXED) && defined(STRIP)
	/* It is impossible to run a right-to-left algorithm using ordinary curves
	 * and only mixed additions. */
	THROW(ERR_NO_CONFIG);
#else
	eb_mul_rnaf_imp(r, p, k);
#endif
#endif
}

#endif

#if EB_MUL == HALVE || !defined(STRIP)

void eb_mul_halve(eb_t r, const eb_t p, const bn_t k) {
	int i, j, l, trc, cof;
	int8_t naf[FB_BITS + 1] = { 0 }, *_k;
	eb_t q, s, t[1 << (EB_WIDTH - 2)];
	bn_t n, m;
	fb_t u, v, w, z;

	if (bn_is_zero(k) || eb_is_infty(p)) {
		eb_set_infty(r);
		return;
	}

	bn_null(m);
	bn_null(n);
	eb_null(q);
	eb_null(s);
	for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
		eb_null(t[i]);
	}
	fb_null(u);
	fb_null(v);
	fb_null(w);
	fb_null(z);

	TRY {
		bn_new(n);
		bn_new(m);
		eb_new(q);
		eb_new(s);
		fb_new(u);
		fb_new(v);
		fb_new(w);
		fb_new(z);

		/* Prepare the precomputation table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_new(t[i]);
			eb_set_infty(t[i]);
		}

		/* Convert k to alternate representation k' = (2^{t-1}k mod n). */
		eb_curve_get_ord(n);
		bn_lsh(m, k, bn_bits(n) - 1);
		bn_mod(m, m, n);

		/* Compute the w-NAF representation of k'. */
		l = sizeof(naf);
		bn_rec_naf(naf, &l, m, EB_WIDTH);

		for (i = l; i <= bn_bits(n); i++) {
			naf[i] = 0;
		}
		if (naf[bn_bits(n)] == 1) {
			eb_dbl(t[0], p);
		}
		l = bn_bits(n);
		_k = naf + l - 1;

		eb_copy(q, p);
		eb_curve_get_cof(n);

		/* Test if curve has a cofactor bigger than 2. */
		if (bn_cmp_dig(n, 2) == CMP_GT) {
			cof = 1;
		} else {
			cof = 0;
		}

		trc = fb_trc(eb_curve_get_a());

		if (cof) {
			/* Curves with cofactor > 2, u = sqrt(a), v = Solve(u). */
			fb_srt(u, eb_curve_get_a());
			fb_slv(v, u);

			bn_rand(n, BN_POS, l);

			for (i = l - 1; i >= 0; i--, _k--) {
				j = *_k;
				if (j > 0) {
					eb_norm(s, q);
					eb_add(t[j / 2], t[j / 2], s);
				}
				if (j < 0) {
					eb_norm(s, q);
					eb_sub(t[-j / 2], t[-j / 2], s);
				}

				/* T = 1/2(Q). */
				eb_hlv(s, q);

				/* If Tr(x_T) != Tr(a). */
				if (fb_trc(s->x) != 0) {
					/* z = l_t, w = sqrt(l_Q), l_T = l_T + sqrt(l_Q) + v. */
					fb_copy(z, s->y);
					fb_srt(w, q->y);
					fb_add(s->y, s->y, w);
					fb_add(s->y, s->y, v);
					/* z = (z + x_Q + v + sqrt(a)). */
					fb_add(z, z, q->x);
					fb_add(z, z, v);
					fb_add(z, z, u);
					/* w = sqrt(w + x_Q + l_Q + sqrt(a)). */
					fb_add(w, w, q->x);
					fb_add(w, w, q->y);
					fb_add(w, w, u);
					/* x_T = sqrt(w * z), . */
					fb_mul(w, w, z);
					fb_srt(s->x, w);
					fb_set_dig(s->z, 1);
					s->norm = 2;
				}
				eb_copy(q, s);
			}
		} else {
			for (i = l - 1; i >= 0; i--, _k--) {
				j = *_k;
				if (j > 0) {
					eb_norm(q, q);
					eb_add(t[j / 2], t[j / 2], q);
				}
				if (j < 0) {
					eb_norm(q, q);
					eb_sub(t[-j / 2], t[-j / 2], q);
				}
				eb_hlv(q, q);
			}
		}

#if EB_WIDTH == 2
		eb_norm(r, t[0]);
#else
		/* Compute Q_i = Q_i + Q_{i+2} for i from 2^{w-1}-3 to 1. */
		for (i = (1 << (EB_WIDTH - 1)) - 3; i >= 1; i -= 2) {
			eb_add(t[i / 2], t[i / 2], t[(i + 2) / 2]);
		}
		/* Compute R = Q_1 + 2 * sum_{i != 1}Q_i. */
		eb_copy(r, t[1]);
		for (i = 2; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_add(r, r, t[i]);
		}
		eb_dbl(r, r);
		eb_add(r, r, t[0]);
		eb_norm(r, r);
#endif

		/* We may need to fix an error of a 2-torsion point if the curve has a
		 * 4-cofactor. */
		if (cof) {
			eb_hlv(s, r);
			if (fb_trc(s->x) != trc) {
				fb_zero(s->x);
				fb_srt(s->y, eb_curve_get_b());
				fb_set_dig(s->z, 1);
				eb_add(r, r, s);
				eb_norm(r, r);
			}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		/* Free the precomputation table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_free(t[i]);
		}
		bn_free(n);
		bn_free(m);
		eb_free(q);
		eb_free(s);
		fb_free(u);
		fb_free(v);
		fb_free(w);
		fb_free(z);
	}
}

#endif

void eb_mul_gen(eb_t r, const bn_t k) {
#ifdef EB_PRECO
	eb_mul_fix(r, eb_curve_get_tab(), k);
#else
	eb_t g;

	eb_null(g);

	TRY {
		eb_new(g);
		eb_curve_get_gen(g);
		eb_mul(r, g, k);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		eb_free(g);
	}
#endif
}

void eb_mul_dig(eb_t r, const eb_t p, dig_t k) {
	eb_t t;

	if (k == 0 || eb_is_infty(p)) {
		eb_set_infty(r);
		return;
	}

	eb_null(t);

	TRY {
		eb_new(t);

		eb_copy(t, p);
		for (int i = util_bits_dig(k) - 2; i >= 0; i--) {
			eb_dbl(t, t);
			if (k & ((dig_t)1 << i)) {
				eb_add(t, t, p);
			}
		}

		eb_norm(r, t);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		eb_free(t);
	}
}
