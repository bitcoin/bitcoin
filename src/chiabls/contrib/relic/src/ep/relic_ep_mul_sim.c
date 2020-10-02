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
 * Implementation of simultaneous point multiplication on binary elliptic
 * curves.
 *
 * @ingroup ep
 */

#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if EP_SIM == INTER || !defined(STRIP)

#if defined(EP_ENDOM)

/**
 * Multiplies and adds two prime elliptic curve points simultaneously,
 * optionally choosing the first point as the generator depending on an optional
 * table of precomputed points.
 *
 * @param[out] r 				- the result.
 * @param[in] p					- the first point to multiply.
 * @param[in] k					- the first integer.
 * @param[in] q					- the second point to multiply.
 * @param[in] m					- the second integer.
 * @param[in] t					- the pointer to the precomputed table.
 */
static void ep_mul_sim_endom(ep_t r, const ep_t p, const bn_t k, const ep_t q,
		const bn_t m, const ep_t *t) {
	int i, l, l0, l1, l2, l3, n, sk0, sk1, sl0, sl1, w, g = 0;
	int8_t naf0[FP_BITS + 1], naf1[FP_BITS + 1], *t0, *t1;
	int8_t naf2[FP_BITS + 1], naf3[FP_BITS + 1], *t2, *t3;
	bn_t k0, k1, m0, m1;
	bn_t ord, v1[3], v2[3];
	ep_t u;
	ep_t tab0[1 << (EP_WIDTH - 2)];
	ep_t tab1[1 << (EP_WIDTH - 2)];

	bn_null(ord);
	bn_null(k0);
	bn_null(k1);
	bn_null(m0);
	bn_null(m1);
	ep_null(u);

	for (i = 0; i < (1 << (EP_WIDTH - 2)); i++) {
		ep_null(tab0[i]);
		ep_null(tab1[i]);
	}

	bn_new(ord);
	bn_new(k0);
	bn_new(k1);
	bn_new(m0);
	bn_new(m1);
	ep_new(u);

	TRY {
		for (i = 0; i < 3; i++) {
			bn_null(v1[i]);
			bn_null(v2[i]);
			bn_new(v1[i]);
			bn_new(v2[i]);
		}

		ep_curve_get_ord(ord);
		ep_curve_get_v1(v1);
		ep_curve_get_v2(v2);

		bn_rec_glv(k0, k1, k, ord, (const bn_t *)v1, (const bn_t *)v2);
		sk0 = bn_sign(k0);
		sk1 = bn_sign(k1);
		bn_abs(k0, k0);
		bn_abs(k1, k1);

		bn_rec_glv(m0, m1, m, ord, (const bn_t *)v1, (const bn_t *)v2);
		sl0 = bn_sign(m0);
		sl1 = bn_sign(m1);
		bn_abs(m0, m0);
		bn_abs(m1, m1);

		g = (t == NULL ? 0 : 1);
		if (!g) {
			for (i = 0; i < (1 << (EP_WIDTH - 2)); i++) {
				ep_new(tab0[i]);
			}
			ep_tab(tab0, p, EP_WIDTH);
			t = (const ep_t *)tab0;
		}

		/* Prepare the precomputation table. */
		for (i = 0; i < (1 << (EP_WIDTH - 2)); i++) {
			ep_new(tab1[i]);
		}
		/* Compute the precomputation table. */
		ep_tab(tab1, q, EP_WIDTH);

		/* Compute the w-TNAF representation of k and l */
		if (g) {
			w = EP_DEPTH;
		} else {
			w = EP_WIDTH;
		}
		l0 = l1 = l2 = l3 = FP_BITS + 1;
		bn_rec_naf(naf0, &l0, k0, w);
		bn_rec_naf(naf1, &l1, k1, w);
		bn_rec_naf(naf2, &l2, m0, EP_WIDTH);
		bn_rec_naf(naf3, &l3, m1, EP_WIDTH);

		l = MAX(MAX(l0, l1), MAX(l2, l3));
		t0 = naf0 + l - 1;
		t1 = naf1 + l - 1;
		t2 = naf2 + l - 1;
		t3 = naf3 + l - 1;
		for (i = l0; i < l; i++) {
			naf0[i] = 0;
		}
		for (i = l1; i < l; i++) {
			naf1[i] = 0;
		}
		for (i = l2; i < l; i++) {
			naf2[i] = 0;
		}
		for (i = l3; i < l; i++) {
			naf3[i] = 0;
		}

		if (bn_sign(k) == BN_NEG) {
			for (i =  0; i < l0; i++) {
				naf0[i] = -naf0[i];
			}
			for (i =  0; i < l1; i++) {
				naf1[i] = -naf1[i];
			}
		}
		if (bn_sign(m) == BN_NEG) {
			for (i =  0; i < l2; i++) {
				naf2[i] = -naf2[i];
			}
			for (i =  0; i < l3; i++) {
				naf3[i] = -naf3[i];
			}
		}

		ep_set_infty(r);
		for (i = l - 1; i >= 0; i--, t0--, t1--, t2--, t3--) {
			ep_dbl(r, r);

			n = *t0;
			if (n > 0) {
				if (sk0 == BN_POS) {
					ep_add(r, r, t[n / 2]);
				} else {
					ep_sub(r, r, t[n / 2]);
				}
			}
			if (n < 0) {
				if (sk0 == BN_POS) {
					ep_sub(r, r, t[-n / 2]);
				} else {
					ep_add(r, r, t[-n / 2]);
				}
			}
			n = *t1;
			if (n > 0) {
				ep_copy(u, t[n / 2]);
				fp_mul(u->x, u->x, ep_curve_get_beta());
				if (sk1 == BN_NEG) {
					ep_neg(u, u);
				}
				ep_add(r, r, u);
			}
			if (n < 0) {
				ep_copy(u, t[-n / 2]);
				fp_mul(u->x, u->x, ep_curve_get_beta());
				if (sk1 == BN_NEG) {
					ep_neg(u, u);
				}
				ep_sub(r, r, u);
			}

			n = *t2;
			if (n > 0) {
				if (sl0 == BN_POS) {
					ep_add(r, r, tab1[n / 2]);
				} else {
					ep_sub(r, r, tab1[n / 2]);
				}
			}
			if (n < 0) {
				if (sl0 == BN_POS) {
					ep_sub(r, r, tab1[-n / 2]);
				} else {
					ep_add(r, r, tab1[-n / 2]);
				}
			}
			n = *t3;
			if (n > 0) {
				ep_copy(u, tab1[n / 2]);
				fp_mul(u->x, u->x, ep_curve_get_beta());
				if (sl1 == BN_NEG) {
					ep_neg(u, u);
				}
				ep_add(r, r, u);
			}
			if (n < 0) {
				ep_copy(u, tab1[-n / 2]);
				fp_mul(u->x, u->x, ep_curve_get_beta());
				if (sl1 == BN_NEG) {
					ep_neg(u, u);
				}
				ep_sub(r, r, u);
			}
		}
		/* Convert r to affine coordinates. */
		ep_norm(r, r);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(ord);
		bn_free(k0);
		bn_free(k1);
		bn_free(m0);
		bn_free(m1);
		ep_free(u);

		if (!g) {
			for (i = 0; i < 1 << (EP_WIDTH - 2); i++) {
				ep_free(tab0[i]);
			}
		}
		/* Free the precomputation tables. */
		for (i = 0; i < 1 << (EP_WIDTH - 2); i++) {
			ep_free(tab1[i]);
		}
		for (i = 0; i < 3; i++) {
			bn_free(v1[i]);
			bn_free(v2[i]);
		}
	}
}

#endif /* EP_ENDOM */

#if defined(EP_PLAIN) || defined(EP_SUPER)

/**
 * Multiplies and adds two prime elliptic curve points simultaneously,
 * optionally choosing the first point as the generator depending on an optional
 * table of precomputed points.
 *
 * @param[out] r 				- the result.
 * @param[in] p					- the first point to multiply.
 * @param[in] k					- the first integer.
 * @param[in] q					- the second point to multiply.
 * @param[in] m					- the second integer.
 * @param[in] t					- the pointer to the precomputed table.
 */
static void ep_mul_sim_plain(ep_t r, const ep_t p, const bn_t k, const ep_t q,
		const bn_t m, const ep_t *t) {
	int i, l, l0, l1, n0, n1, w, gen;
	int8_t naf0[FP_BITS + 1], naf1[FP_BITS + 1], *_k, *_m;
	ep_t t0[1 << (EP_WIDTH - 2)];
	ep_t t1[1 << (EP_WIDTH - 2)];

	TRY {
		gen = (t == NULL ? 0 : 1);
		if (!gen) {
			for (i = 0; i < (1 << (EP_WIDTH - 2)); i++) {
				ep_null(t0[i]);
				ep_new(t0[i]);
			}
			ep_tab(t0, p, EP_WIDTH);
			t = (const ep_t *)t0;
		}

		/* Prepare the precomputation table. */
		for (i = 0; i < (1 << (EP_WIDTH - 2)); i++) {
			ep_null(t1[i]);
			ep_new(t1[i]);
		}
		/* Compute the precomputation table. */
		ep_tab(t1, q, EP_WIDTH);

		/* Compute the w-TNAF representation of k. */
		if (gen) {
			w = EP_DEPTH;
		} else {
			w = EP_WIDTH;
		}
		l0 = l1 = FP_BITS + 1;
		bn_rec_naf(naf0, &l0, k, w);
		bn_rec_naf(naf1, &l1, m, EP_WIDTH);

		l = MAX(l0, l1);
		for (i = l0; i < l; i++) {
			naf0[i] = 0;
		}
		for (i = l1; i < l; i++) {
			naf1[i] = 0;
		}

		if (bn_sign(k) == BN_NEG) {
			for (i =  0; i < l0; i++) {
				naf0[i] = -naf0[i];
			}
		}
		if (bn_sign(m) == BN_NEG) {
			for (i =  0; i < l1; i++) {
				naf1[i] = -naf1[i];
			}
		}

		_k = naf0 + l - 1;
		_m = naf1 + l - 1;
		ep_set_infty(r);
		for (i = l - 1; i >= 0; i--, _k--, _m--) {
			ep_dbl(r, r);

			n0 = *_k;
			n1 = *_m;
			if (n0 > 0) {
				ep_add(r, r, t[n0 / 2]);
			}
			if (n0 < 0) {
				ep_sub(r, r, t[-n0 / 2]);
			}
			if (n1 > 0) {
				ep_add(r, r, t1[n1 / 2]);
			}
			if (n1 < 0) {
				ep_sub(r, r, t1[-n1 / 2]);
			}
		}
		/* Convert r to affine coordinates. */
		ep_norm(r, r);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		/* Free the precomputation tables. */
		if (!gen) {
			for (i = 0; i < 1 << (EP_WIDTH - 2); i++) {
				ep_free(t0[i]);
			}
		}
		for (i = 0; i < 1 << (EP_WIDTH - 2); i++) {
			ep_free(t1[i]);
		}
	}
}

#endif /* EP_PLAIN || EP_SUPER */

#endif /* EP_SIM == INTER */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if EP_SIM == BASIC || !defined(STRIP)

void ep_mul_sim_basic(ep_t r, const ep_t p, const bn_t k, const ep_t q,
		const bn_t m) {
	ep_t t;

	ep_null(t);

	TRY {
		ep_new(t);
		ep_mul(t, q, m);
		ep_mul(r, p, k);
		ep_add(t, t, r);
		ep_norm(r, t);

	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep_free(t);
	}
}

#endif

#if EP_SIM == TRICK || !defined(STRIP)

void ep_mul_sim_trick(ep_t r, const ep_t p, const bn_t k, const ep_t q,
		const bn_t m) {
	ep_t t0[1 << (EP_WIDTH / 2)], t1[1 << (EP_WIDTH / 2)], t[1 << EP_WIDTH];
	bn_t n;
	int l0, l1, w = EP_WIDTH / 2;
	uint8_t w0[CEIL(FP_BITS + 1, w)], w1[CEIL(FP_BITS + 1, w)];

	bn_null(n);

	if (bn_is_zero(k) || ep_is_infty(p)) {
		ep_mul(r, q, m);
		return;
	}
	if (bn_is_zero(m) || ep_is_infty(q)) {
		ep_mul(r, p, k);
		return;
	}

	TRY {
		bn_new(n);

		ep_curve_get_ord(n);

		for (int i = 0; i < (1 << w); i++) {
			ep_null(t0[i]);
			ep_null(t1[i]);
			ep_new(t0[i]);
			ep_new(t1[i]);
		}
		for (int i = 0; i < (1 << EP_WIDTH); i++) {
			ep_null(t[i]);
			ep_new(t[i]);
		}

		ep_set_infty(t0[0]);
		ep_copy(t0[1], p);
		if (bn_sign(k) == BN_NEG) {
			ep_neg(t0[1], t0[1]);
		}
		for (int i = 2; i < (1 << w); i++) {
			ep_add(t0[i], t0[i - 1], t0[1]);
		}

		ep_set_infty(t1[0]);
		ep_copy(t1[1], q);
		if (bn_sign(m) == BN_NEG) {
			ep_neg(t1[1], t1[1]);
		}
		for (int i = 1; i < (1 << w); i++) {
			ep_add(t1[i], t1[i - 1], t1[1]);
		}

		for (int i = 0; i < (1 << w); i++) {
			for (int j = 0; j < (1 << w); j++) {
				ep_add(t[(i << w) + j], t0[i], t1[j]);
			}
		}

#if defined(EP_MIXED)
		ep_norm_sim(t + 1, (const ep_t *)t + 1, (1 << (EP_WIDTH)) - 1);
#endif

		l0 = l1 = CEIL(FP_BITS, w);
		bn_rec_win(w0, &l0, k, w);
		bn_rec_win(w1, &l1, m, w);

		for (int i = l0; i < l1; i++) {
			w0[i] = 0;
		}
		for (int i = l1; i < l0; i++) {
			w1[i] = 0;
		}

		ep_set_infty(r);
		for (int i = MAX(l0, l1) - 1; i >= 0; i--) {
			for (int j = 0; j < w; j++) {
				ep_dbl(r, r);
			}
			ep_add(r, r, t[(w0[i] << w) + w1[i]]);
		}
		ep_norm(r, r);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(n);
		for (int i = 0; i < (1 << w); i++) {
			ep_free(t0[i]);
			ep_free(t1[i]);
		}
		for (int i = 0; i < (1 << EP_WIDTH); i++) {
			ep_free(t[i]);
		}
	}
}
#endif

#if EP_SIM == INTER || !defined(STRIP)

void ep_mul_sim_inter(ep_t r, const ep_t p, const bn_t k, const ep_t q,
		const bn_t m) {

	if (bn_is_zero(k) || ep_is_infty(p)) {
		ep_mul(r, q, m);
		return;
	}
	if (bn_is_zero(m) || ep_is_infty(q)) {
		ep_mul(r, p, k);
		return;
	}

#if defined(EP_ENDOM)
	if (ep_curve_is_endom()) {
		ep_mul_sim_endom(r, p, k, q, m, NULL);
		return;
	}
#endif

#if defined(EP_PLAIN) || defined(EP_SUPER)
	ep_mul_sim_plain(r, p, k, q, m, NULL);
#endif
}

#endif

#if EP_SIM == JOINT || !defined(STRIP)

void ep_mul_sim_joint(ep_t r, const ep_t p, const bn_t k, const ep_t q,
		const bn_t m) {
	ep_t t[5];
	int i, l, u_i, offset;
	int8_t jsf[2 * (FP_BITS + 1)];

	if (bn_is_zero(k) || ep_is_infty(p)) {
		ep_mul(r, q, m);
		return;
	}
	if (bn_is_zero(m) || ep_is_infty(q)) {
		ep_mul(r, p, k);
		return;
	}

	TRY {
		for (i = 0; i < 5; i++) {
			ep_null(t[i]);
			ep_new(t[i]);
		}

		ep_set_infty(t[0]);
		ep_copy(t[1], q);
		if (bn_sign(m) == BN_NEG) {
			ep_neg(t[1], t[1]);
		}
		ep_copy(t[2], p);
		if (bn_sign(k) == BN_NEG) {
			ep_neg(t[2], t[2]);
		}
		ep_add(t[3], t[2], t[1]);
		ep_sub(t[4], t[2], t[1]);
#if defined(EP_MIXED)
		ep_norm_sim(t + 3, (const ep_t *)t + 3, 2);
#endif

		l = 2 * (FP_BITS + 1);
		bn_rec_jsf(jsf, &l, k, m);

		ep_set_infty(r);

		offset = MAX(bn_bits(k), bn_bits(m)) + 1;
		for (i = l - 1; i >= 0; i--) {
			ep_dbl(r, r);
			if (jsf[i] != 0 && jsf[i] == -jsf[i + offset]) {
				u_i = jsf[i] * 2 + jsf[i + offset];
				if (u_i < 0) {
					ep_sub(r, r, t[4]);
				} else {
					ep_add(r, r, t[4]);
				}
			} else {
				u_i = jsf[i] * 2 + jsf[i + offset];
				if (u_i < 0) {
					ep_sub(r, r, t[-u_i]);
				} else {
					ep_add(r, r, t[u_i]);
				}
			}
		}
		ep_norm(r, r);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		for (i = 0; i < 5; i++) {
			ep_free(t[i]);
		}
	}
}

#endif

void ep_mul_sim_gen(ep_t r, const bn_t k, const ep_t q, const bn_t m) {
	ep_t g;

	ep_null(g);

	if (bn_is_zero(k)) {
		ep_mul(r, q, m);
		return;
	}
	if (bn_is_zero(m) || ep_is_infty(q)) {
		ep_mul_gen(r, k);
		return;
	}

	TRY {
		ep_new(g);

		ep_curve_get_gen(g);

#if defined(EP_ENDOM)
#if EP_SIM == INTER && EP_FIX == LWNAF && defined(EP_PRECO)
		if (ep_curve_is_endom()) {
			ep_mul_sim_endom(r, g, k, q, m, ep_curve_get_tab());
		}
#else
		if (ep_curve_is_endom()) {
			ep_mul_sim(r, g, k, q, m);
		}
#endif
#endif

#if defined(EP_PLAIN) || defined(EP_SUPER)
#if EP_SIM == INTER && EP_FIX == LWNAF && defined(EP_PRECO)
		if (!ep_curve_is_endom()) {
			ep_mul_sim_plain(r, g, k, q, m, ep_curve_get_tab());
		}
#else
		if (!ep_curve_is_endom()) {
			ep_mul_sim(r, g, k, q, m);
		}
#endif
#endif
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep_free(g);
	}
}
