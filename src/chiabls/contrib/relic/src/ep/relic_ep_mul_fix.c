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
 * Implementation of fixed point multiplication on binary elliptic curves.
 *
 * @ingroup ep
 */

#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if EP_FIX == LWNAF || !defined(STRIP)

/**
 * Multiplies a binary elliptic curve point by an integer using the w-NAF
 * method.
 *
 * @param[out] r 				- the result.
 * @param[in] t					- the precomputed table.
 * @param[in] k					- the integer.
 */
static void ep_mul_fix_plain(ep_t r, const ep_t *t, const bn_t k) {
	int l, i, n;
	int8_t naf[FP_BITS + 1];

	if (bn_is_zero(k)) {
		ep_set_infty(r);
		return;
	}

	/* Compute the w-TNAF representation of k. */
	l = FP_BITS + 1;
	bn_rec_naf(naf, &l, k, EP_DEPTH);

	n = naf[l - 1];
	if (n > 0) {
		ep_copy(r, t[n / 2]);
	} else {
		ep_neg(r, t[-n / 2]);
	}

	for (i = l - 2; i >= 0; i--) {
		ep_dbl(r, r);

		n = naf[i];
		if (n > 0) {
			ep_add(r, r, t[n / 2]);
		}
		if (n < 0) {
			ep_sub(r, r, t[-n / 2]);
		}
	}
	/* Convert r to affine coordinates. */
	ep_norm(r, r);
	if (bn_sign(k) == BN_NEG) {
		ep_neg(r, r);
	}
}

#endif /* EP_FIX == LWNAF */

#if EP_FIX == COMBS || !defined(STRIP)

#if defined(EP_ENDOM)

/**
 * Multiplies a prime elliptic curve point by an integer using the COMBS
 * method.
 *
 * @param[out] r 				- the result.
 * @param[in] t					- the precomputed table.
 * @param[in] k					- the integer.
 */
static void ep_mul_combs_endom(ep_t r, const ep_t *t, const bn_t k) {
	int i, j, l, w0, w1, n0, n1, p0, p1, s0, s1;
	bn_t n, k0, k1, v1[3], v2[3];
	ep_t u;

	if (bn_is_zero(k)) {
		ep_set_infty(r);
		return;
	}

	bn_null(n);
	bn_null(k0);
	bn_null(k1);
	ep_null(u);

	TRY {
		bn_new(n);
		bn_new(k0);
		bn_new(k1);
		ep_new(u);
		for (i = 0; i < 3; i++) {
			bn_null(v1[i]);
			bn_null(v2[i]);
			bn_new(v1[i]);
			bn_new(v2[i]);
		}

		ep_curve_get_ord(n);
		ep_curve_get_v1(v1);
		ep_curve_get_v2(v2);
		l = bn_bits(n);
		l = ((l % (2 * EP_DEPTH)) ==
				0 ? (l / (2 * EP_DEPTH)) : (l / (2 * EP_DEPTH)) + 1);

		bn_rec_glv(k0, k1, k, n, (const bn_t *)v1, (const bn_t *)v2);
		s0 = bn_sign(k0);
		s1 = bn_sign(k1);
		bn_abs(k0, k0);
		bn_abs(k1, k1);

		n0 = bn_bits(k0);
		n1 = bn_bits(k1);

		p0 = (EP_DEPTH) * l - 1;

		ep_set_infty(r);

		for (i = l - 1; i >= 0; i--) {
			ep_dbl(r, r);

			w0 = 0;
			w1 = 0;
			p1 = p0--;
			for (j = EP_DEPTH - 1; j >= 0; j--, p1 -= l) {
				w0 = w0 << 1;
				w1 = w1 << 1;
				if (p1 < n0 && bn_get_bit(k0, p1)) {
					w0 = w0 | 1;
				}
				if (p1 < n1 && bn_get_bit(k1, p1)) {
					w1 = w1 | 1;
				}
			}
			if (w0 > 0) {
				if (s0 == BN_POS) {
					ep_add(r, r, t[w0]);
				} else {
					ep_sub(r, r, t[w0]);
				}
			}
			if (w1 > 0) {
				ep_copy(u, t[w1]);
				fp_mul(u->x, u->x, ep_curve_get_beta());
				if (s1 == BN_NEG) {
					ep_neg(u, u);
				}
				ep_add(r, r, u);
			}
		}
		ep_norm(r, r);
		if (bn_sign(k) == BN_NEG) {
			ep_neg(r, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(n);
		bn_free(k0);
		bn_free(k1);
		ep_free(u);
		for (i = 0; i < 3; i++) {
			bn_free(v1[i]);
			bn_free(v2[i]);
		}
	}
}

#endif /* EP_ENDOM */

#if defined(EP_PLAIN) || defined(EP_SUPER)
/**
 * Multiplies a prime elliptic curve point by an integer using the COMBS
 * method.
 *
 * @param[out] r 				- the result.
 * @param[in] t					- the precomputed table.
 * @param[in] k					- the integer.
 */
static void ep_mul_combs_plain(ep_t r, const ep_t *t, const bn_t k) {
	int i, j, l, w, n0, p0, p1;
	bn_t n;

	if (bn_is_zero(k)) {
		ep_set_infty(r);
		return;
	}

	bn_null(n);

	TRY {
		bn_new(n);

		ep_curve_get_ord(n);
		l = CEIL(bn_bits(n), EP_DEPTH);

		n0 = bn_bits(k);
		p0 = (EP_DEPTH) * l - 1;

		w = 0;
		p1 = p0--;
		for (j = EP_DEPTH - 1; j >= 0; j--, p1 -= l) {
			w = w << 1;
			if (p1 < n0 && bn_get_bit(k, p1)) {
				w = w | 1;
			}
		}

		ep_copy(r, t[w]);
		for (i = l - 2; i >= 0; i--) {
			ep_dbl(r, r);

			w = 0;
			p1 = p0--;
			for (j = EP_DEPTH - 1; j >= 0; j--, p1 -= l) {
				w = w << 1;
				if (p1 < n0 && bn_get_bit(k, p1)) {
					w = w | 1;
				}
			}
			if (w > 0) {
				ep_add(r, r, t[w]);
			}
		}
		ep_norm(r, r);
		if (bn_sign(k) == BN_NEG) {
			ep_neg(r, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(n);
	}
}

#endif /* EP_PLAIN || EP_SUPER */

#endif /* EP_FIX == LWNAF */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if EP_FIX == BASIC || !defined(STRIP)

void ep_mul_pre_basic(ep_t *t, const ep_t p) {
	bn_t n;

	bn_null(n);

	TRY {
		bn_new(n);

		ep_curve_get_ord(n);

		ep_copy(t[0], p);
		for (int i = 1; i < bn_bits(n); i++) {
			ep_dbl(t[i], t[i - 1]);
		}

		ep_norm_sim(t + 1, (const ep_t *)t + 1, bn_bits(n) - 1);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(n);
	}
}

void ep_mul_fix_basic(ep_t r, const ep_t *t, const bn_t k) {
	if (bn_is_zero(k)) {
		ep_set_infty(r);
		return;
	}

	ep_set_infty(r);

	for (int i = 0; i < bn_bits(k); i++) {
		if (bn_get_bit(k, i)) {
			ep_add(r, r, t[i]);
		}
	}
	ep_norm(r, r);
	if (bn_sign(k) == BN_NEG) {
		ep_neg(r, r);
	}
}

#endif

#if EP_FIX == YAOWI || !defined(STRIP)

void ep_mul_pre_yaowi(ep_t *t, const ep_t p) {
	int l;
	bn_t n;

	bn_null(n);

	TRY {
		bn_new(n);

		ep_curve_get_ord(n);
		l = CEIL(bn_bits(n), EP_DEPTH);

		ep_copy(t[0], p);
		for (int i = 1; i < l; i++) {
			ep_dbl(t[i], t[i - 1]);
			for (int j = 1; j < EP_DEPTH; j++) {
				ep_dbl(t[i], t[i]);
			}
		}

		ep_norm_sim(t + 1, (const ep_t *)t + 1, l - 1);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		bn_free(n);
	}
}

void ep_mul_fix_yaowi(ep_t r, const ep_t *t, const bn_t k) {
	int i, j, l = FP_BITS;
	ep_t a;
	uint8_t win[CEIL(FP_BITS, EP_DEPTH)];

	if (bn_is_zero(k)) {
		ep_set_infty(r);
		return;
	}

	ep_null(a);

	TRY {
		ep_new(a);

		ep_set_infty(r);
		ep_set_infty(a);

		bn_rec_win(win, &l, k, EP_DEPTH);

		for (j = (1 << EP_DEPTH) - 1; j >= 1; j--) {
			for (i = 0; i < l; i++) {
				if (win[i] == j) {
					ep_add(a, a, t[i]);
				}
			}
			ep_add(r, r, a);
		}
		ep_norm(r, r);
		if (bn_sign(k) == BN_NEG) {
			ep_neg(r, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep_free(a);
	}
}

#endif

#if EP_FIX == NAFWI || !defined(STRIP)

void ep_mul_pre_nafwi(ep_t *t, const ep_t p) {
	int l;
	bn_t n;

	bn_null(n);

	TRY {
		bn_new(n);

		ep_curve_get_ord(n);
		l = CEIL(bn_bits(n) + 1, EP_DEPTH);

		ep_copy(t[0], p);
		for (int i = 1; i < l; i++) {
			ep_dbl(t[i], t[i - 1]);
			for (int j = 1; j < EP_DEPTH; j++) {
				ep_dbl(t[i], t[i]);
			}
		}

		ep_norm_sim(t + 1, (const ep_t *)t + 1, l - 1);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(n);
	}
}

void ep_mul_fix_nafwi(ep_t r, const ep_t *t, const bn_t k) {
	int i, j, l, d, m;
	ep_t a;
	int8_t naf[FP_BITS + 1];
	char w;

	if (bn_is_zero(k)) {
		ep_set_infty(r);
		return;
	}

	ep_null(a);

	TRY {
		ep_new(a);

		ep_set_infty(r);
		ep_set_infty(a);

		l = FP_BITS + 1;
		bn_rec_naf(naf, &l, k, 2);

		d = ((l % EP_DEPTH) == 0 ? (l / EP_DEPTH) : (l / EP_DEPTH) + 1);

		for (i = 0; i < d; i++) {
			w = 0;
			for (j = EP_DEPTH - 1; j >= 0; j--) {
				if (i * EP_DEPTH + j < l) {
					w = (char)(w << 1);
					w = (char)(w + naf[i * EP_DEPTH + j]);
				}
			}
			naf[i] = w;
		}

		if (EP_DEPTH % 2 == 0) {
			m = ((1 << (EP_DEPTH + 1)) - 2) / 3;
		} else {
			m = ((1 << (EP_DEPTH + 1)) - 1) / 3;
		}

		for (j = m; j >= 1; j--) {
			for (i = 0; i < d; i++) {
				if (naf[i] == j) {
					ep_add(a, a, t[i]);
				}
				if (naf[i] == -j) {
					ep_sub(a, a, t[i]);
				}
			}
			ep_add(r, r, a);
		}
		ep_norm(r, r);
		if (bn_sign(k) == BN_NEG) {
			ep_neg(r, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep_free(a);
	}
}

#endif

#if EP_FIX == COMBS || !defined(STRIP)

void ep_mul_pre_combs(ep_t *t, const ep_t p) {
	int i, j, l;
	bn_t n;

	bn_null(n);

	TRY {
		bn_new(n);

		ep_curve_get_ord(n);
		l = bn_bits(n);
		l = ((l % EP_DEPTH) == 0 ? (l / EP_DEPTH) : (l / EP_DEPTH) + 1);
#if defined(EP_ENDOM)
		if (ep_curve_is_endom()) {
			l = bn_bits(n);
			l = ((l % (2 * EP_DEPTH)) ==
					0 ? (l / (2 * EP_DEPTH)) : (l / (2 * EP_DEPTH)) + 1);
		}
#endif

		ep_set_infty(t[0]);

		ep_copy(t[1], p);
		for (j = 1; j < EP_DEPTH; j++) {
			ep_dbl(t[1 << j], t[1 << (j - 1)]);
			for (i = 1; i < l; i++) {
				ep_dbl(t[1 << j], t[1 << j]);
			}
#if defined(EP_MIXED)
			ep_norm(t[1 << j], t[1 << j]);
#endif
			for (i = 1; i < (1 << j); i++) {
				ep_add(t[(1 << j) + i], t[i], t[1 << j]);
			}
		}

		ep_norm_sim(t + 2, (const ep_t *)t + 2, RELIC_EP_TABLE_COMBS - 2);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(n);
	}
}

void ep_mul_fix_combs(ep_t r, const ep_t *t, const bn_t k) {
#if defined(EP_ENDOM)
	if (ep_curve_is_endom()) {
		ep_mul_combs_endom(r, t, k);
		return;
	}
#endif

#if defined(EP_PLAIN) || defined(EP_SUPER)
	ep_mul_combs_plain(r, t, k);
#endif
}
#endif

#if EP_FIX == COMBD || !defined(STRIP)

void ep_mul_pre_combd(ep_t *t, const ep_t p) {
	int i, j, d, e;
	bn_t n;

	bn_null(n);

	TRY {
		bn_new(n);

		ep_curve_get_ord(n);
		d = CEIL(bn_bits(n), EP_DEPTH);
		e = (d % 2 == 0 ? (d / 2) : (d / 2) + 1);

		ep_set_infty(t[0]);
		ep_copy(t[1], p);
		for (j = 1; j < EP_DEPTH; j++) {
			ep_dbl(t[1 << j], t[1 << (j - 1)]);
			for (i = 1; i < d; i++) {
				ep_dbl(t[1 << j], t[1 << j]);
			}
#if defined(EP_MIXED)
			ep_norm(t[1 << j], t[1 << j]);
#endif
			for (i = 1; i < (1 << j); i++) {
				ep_add(t[(1 << j) + i], t[i], t[1 << j]);
			}
		}
		ep_set_infty(t[1 << EP_DEPTH]);
		for (j = 1; j < (1 << EP_DEPTH); j++) {
			ep_dbl(t[(1 << EP_DEPTH) + j], t[j]);
			for (i = 1; i < e; i++) {
				ep_dbl(t[(1 << EP_DEPTH) + j], t[(1 << EP_DEPTH) + j]);
			}
		}

		ep_norm_sim(t + 2, (const ep_t *)t + 2, (1 << EP_DEPTH) - 2);
		ep_norm_sim(t + (1 << EP_DEPTH) + 1,
				(const ep_t *)t + (1 << EP_DEPTH) + 1, (1 << EP_DEPTH) - 1);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(n);
	}
}

void ep_mul_fix_combd(ep_t r, const ep_t *t, const bn_t k) {
	int i, j, d, e, w0, w1, n0, p0, p1;
	bn_t n;

	if (bn_is_zero(k)) {
		ep_set_infty(r);
		return;
	}

	bn_null(n);

	TRY {
		bn_new(n);

		ep_curve_get_ord(n);
		d = CEIL(bn_bits(n), EP_DEPTH);
		e = (d % 2 == 0 ? (d / 2) : (d / 2) + 1);

		ep_set_infty(r);
		n0 = bn_bits(k);

		p1 = (e - 1) + (EP_DEPTH - 1) * d;
		for (i = e - 1; i >= 0; i--) {
			ep_dbl(r, r);

			w0 = 0;
			p0 = p1;
			for (j = EP_DEPTH - 1; j >= 0; j--, p0 -= d) {
				w0 = w0 << 1;
				if (p0 < n0 && bn_get_bit(k, p0)) {
					w0 = w0 | 1;
				}
			}

			w1 = 0;
			p0 = p1-- + e;
			for (j = EP_DEPTH - 1; j >= 0; j--, p0 -= d) {
				w1 = w1 << 1;
				if (i + e < d && p0 < n0 && bn_get_bit(k, p0)) {
					w1 = w1 | 1;
				}
			}

			ep_add(r, r, t[w0]);
			ep_add(r, r, t[(1 << EP_DEPTH) + w1]);
		}
		ep_norm(r, r);
		if (bn_sign(k) == BN_NEG) {
			ep_neg(r, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(n);
	}
}

#endif

#if EP_FIX == LWNAF || !defined(STRIP)

void ep_mul_pre_lwnaf(ep_t *t, const ep_t p) {
	ep_tab(t, p, EP_DEPTH);
}

void ep_mul_fix_lwnaf(ep_t r, const ep_t *t, const bn_t k) {
	ep_mul_fix_plain(r, t, k);
}
#endif
