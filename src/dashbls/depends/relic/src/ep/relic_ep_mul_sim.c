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
	int i, l, l0, l1, l2, l3, sk0, sk1, sl0, sl1, w, g = 0;
	int8_t naf0[RLC_FP_BITS + 1], naf1[RLC_FP_BITS + 1], *t0, *t1, u;
	int8_t naf2[RLC_FP_BITS + 1], naf3[RLC_FP_BITS + 1], *t2, *t3;
	bn_t n, k0, k1, m0, m1;
	bn_t v1[3], v2[3];
	ep_t v;
	ep_t tab0[1 << (EP_WIDTH - 2)];
	ep_t tab1[1 << (EP_WIDTH - 2)];

	bn_null(n);
	bn_null(k0);
	bn_null(k1);
	bn_null(m0);
	bn_null(m1);
	ep_null(v);

	for (i = 0; i < (1 << (EP_WIDTH - 2)); i++) {
		ep_null(tab0[i]);
		ep_null(tab1[i]);
	}

	RLC_TRY {
		bn_new(n);
		bn_new(k0);
		bn_new(k1);
		bn_new(m0);
		bn_new(m1);
		ep_new(v);

		for (i = 0; i < 3; i++) {
			bn_null(v1[i]);
			bn_null(v2[i]);
			bn_new(v1[i]);
			bn_new(v2[i]);
		}

		ep_curve_get_ord(n);
		ep_curve_get_v1(v1);
		ep_curve_get_v2(v2);

		bn_rec_glv(k0, k1, k, n, (const bn_t *)v1, (const bn_t *)v2);
		sk0 = bn_sign(k0);
		sk1 = bn_sign(k1);
		bn_abs(k0, k0);
		bn_abs(k1, k1);

		bn_rec_glv(m0, m1, m, n, (const bn_t *)v1, (const bn_t *)v2);
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
		l0 = l1 = l2 = l3 = RLC_FP_BITS + 1;
		bn_rec_naf(naf0, &l0, k0, w);
		bn_rec_naf(naf1, &l1, k1, w);
		bn_rec_naf(naf2, &l2, m0, EP_WIDTH);
		bn_rec_naf(naf3, &l3, m1, EP_WIDTH);

		l = RLC_MAX(RLC_MAX(l0, l1), RLC_MAX(l2, l3));
		t0 = naf0 + l - 1;
		t1 = naf1 + l - 1;
		t2 = naf2 + l - 1;
		t3 = naf3 + l - 1;

		if (bn_sign(k) == RLC_NEG) {
			for (i =  0; i < l0; i++) {
				naf0[i] = -naf0[i];
			}
			for (i =  0; i < l1; i++) {
				naf1[i] = -naf1[i];
			}
		}
		if (bn_sign(m) == RLC_NEG) {
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

			u = *t0;
			if (u > 0) {
				if (sk0 == RLC_POS) {
					ep_add(r, r, t[u / 2]);
				} else {
					ep_sub(r, r, t[u / 2]);
				}
			}
			if (u < 0) {
				if (sk0 == RLC_POS) {
					ep_sub(r, r, t[-u / 2]);
				} else {
					ep_add(r, r, t[-u / 2]);
				}
			}
			u = *t1;
			if (u > 0) {
				ep_psi(v, t[u / 2]);
				if (sk1 == RLC_NEG) {
					ep_neg(v, v);
				}
				ep_add(r, r, v);
			}
			if (u < 0) {
				ep_psi(v, t[-u / 2]);
				if (sk1 == RLC_NEG) {
					ep_neg(v, v);
				}
				ep_sub(r, r, v);
			}

			u = *t2;
			if (u > 0) {
				if (sl0 == RLC_POS) {
					ep_add(r, r, tab1[u / 2]);
				} else {
					ep_sub(r, r, tab1[u / 2]);
				}
			}
			if (u < 0) {
				if (sl0 == RLC_POS) {
					ep_sub(r, r, tab1[-u / 2]);
				} else {
					ep_add(r, r, tab1[-u / 2]);
				}
			}
			u = *t3;
			if (u > 0) {
				ep_psi(v, tab1[u / 2]);
				if (sl1 == RLC_NEG) {
					ep_neg(v, v);
				}
				ep_add(r, r, v);
			}
			if (u < 0) {
				ep_psi(v, tab1[-u / 2]);
				if (sl1 == RLC_NEG) {
					ep_neg(v, v);
				}
				ep_sub(r, r, v);
			}
		}
		/* Convert r to affine coordinates. */
		ep_norm(r, r);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(k0);
		bn_free(k1);
		bn_free(m0);
		bn_free(m1);
		ep_free(v);

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
	int i, l, l0, l1, w, gen;
	int8_t naf0[RLC_FP_BITS + 1], naf1[RLC_FP_BITS + 1], n0, n1, *u, *v;
	ep_t t0[1 << (EP_WIDTH - 2)];
	ep_t t1[1 << (EP_WIDTH - 2)];

	RLC_TRY {
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
		l0 = l1 = RLC_FP_BITS + 1;
		bn_rec_naf(naf0, &l0, k, w);
		bn_rec_naf(naf1, &l1, m, EP_WIDTH);

		l = RLC_MAX(l0, l1);
		if (bn_sign(k) == RLC_NEG) {
			for (i =  0; i < l0; i++) {
				naf0[i] = -naf0[i];
			}
		}
		if (bn_sign(m) == RLC_NEG) {
			for (i =  0; i < l1; i++) {
				naf1[i] = -naf1[i];
			}
		}

		u = naf0 + l - 1;
		v = naf1 + l - 1;
		ep_set_infty(r);
		for (i = l - 1; i >= 0; i--, u--, v--) {
			ep_dbl(r, r);

			n0 = *u;
			n1 = *v;
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
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
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

	RLC_TRY {
		ep_new(t);
		ep_mul(t, q, m);
		ep_mul(r, p, k);
		ep_add(t, t, r);
		ep_norm(r, t);

	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ep_free(t);
	}
}

#endif

#if EP_SIM == TRICK || !defined(STRIP)

void ep_mul_sim_trick(ep_t r, const ep_t p, const bn_t k, const ep_t q,
		const bn_t m) {
	ep_t t0[1 << (EP_WIDTH / 2)], t1[1 << (EP_WIDTH / 2)], t[1 << EP_WIDTH];
	bn_t n, _k, _m;
	int l0, l1, w = EP_WIDTH / 2;
	uint8_t w0[RLC_FP_BITS + 1], w1[RLC_FP_BITS + 1];

	if (bn_is_zero(k) || ep_is_infty(p)) {
		ep_mul(r, q, m);
		return;
	}
	if (bn_is_zero(m) || ep_is_infty(q)) {
		ep_mul(r, p, k);
		return;
	}

	bn_null(n);
	bn_null(_k);
	bn_null(_m);

	RLC_TRY {
		bn_new(n);
		bn_new(_k);
		bn_new(_m);

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

		ep_curve_get_ord(n);
		bn_copy(_k, k);
		if (bn_cmp_abs(_k, n) == RLC_GT) {
			bn_mod(_k, _k, n);
		}
		bn_copy(_m, m);
		if (bn_cmp_abs(_m, n) == RLC_GT) {
			bn_mod(_m, _m, n);
		}

		ep_set_infty(t0[0]);
		ep_copy(t0[1], p);
		if (bn_sign(_k) == RLC_NEG) {
			ep_neg(t0[1], t0[1]);
		}
		for (int i = 2; i < (1 << w); i++) {
			ep_add(t0[i], t0[i - 1], t0[1]);
		}

		ep_set_infty(t1[0]);
		ep_copy(t1[1], q);
		if (bn_sign(_m) == RLC_NEG) {
			ep_neg(t1[1], t1[1]);
		}
		for (int i = 2; i < (1 << w); i++) {
			ep_add(t1[i], t1[i - 1], t1[1]);
		}

		for (int i = 0; i < (1 << w); i++) {
			for (int j = 0; j < (1 << w); j++) {
				ep_add(t[(i << w) + j], t0[i], t1[j]);
			}
		}

#if EP_WIDTH > 2 && defined(EP_MIXED)
		ep_norm_sim(t + 1, (const ep_t *)(t + 1), (1 << EP_WIDTH) - 1);
#endif

		l0 = l1 = RLC_CEIL(RLC_FP_BITS + 1, w);
		bn_rec_win(w0, &l0, _k, w);
		bn_rec_win(w1, &l1, _m, w);
		for (int i = l0; i < l1; i++) {
			w0[i] = 0;
		}
		for (int i = l1; i < l0; i++) {
			w1[i] = 0;
		}

		ep_set_infty(r);
		for (int i = RLC_MAX(l0, l1) - 1; i >= 0; i--) {
			for (int j = 0; j < w; j++) {
				ep_dbl(r, r);
			}
			ep_add(r, r, t[(w0[i] << w) + w1[i]]);
		}
		ep_norm(r, r);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(_k);
		bn_free(_m);
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
	int flag = 0;
	bn_t n, _k, _m;

	if (bn_is_zero(k) || ep_is_infty(p)) {
		ep_mul(r, q, m);
		return;
	}
	if (bn_is_zero(m) || ep_is_infty(q)) {
		ep_mul(r, p, k);
		return;
	}

	bn_null(n);
	bn_null(_k);
	bn_null(_m);

	RLC_TRY {
		bn_new(n);
		bn_new(_k);
		bn_new(_m);

		/* Handle this here to reduce complexity of static functions. */
		ep_curve_get_ord(n);
		bn_copy(_k, k);
		if (bn_cmp_abs(_k, n) == RLC_GT) {
			bn_mod(_k, _k, n);
		}
		bn_copy(_m, m);
		if (bn_cmp_abs(_m, n) == RLC_GT) {
			bn_mod(_m, _m, n);
		}

#if defined(EP_ENDOM)
		if (ep_curve_is_endom()) {
			ep_mul_sim_endom(r, p, _k, q, _m, NULL);
			flag = 1;
		}
#endif

#if defined(EP_PLAIN) || defined(EP_SUPER)
		if (!flag) {
			ep_mul_sim_plain(r, p, _k, q, _m, NULL);
		}
#endif
		(void)flag;
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(_k);
		bn_free(_m);
	}
}

#endif

#if EP_SIM == JOINT || !defined(STRIP)

void ep_mul_sim_joint(ep_t r, const ep_t p, const bn_t k, const ep_t q,
		const bn_t m) {
	bn_t n, _k, _m;
	ep_t t[5];
	int i, l, u_i, offset;
	int8_t jsf[2 * (RLC_FP_BITS + 1)];

	if (bn_is_zero(k) || ep_is_infty(p)) {
		ep_mul(r, q, m);
		return;
	}
	if (bn_is_zero(m) || ep_is_infty(q)) {
		ep_mul(r, p, k);
		return;
	}

	bn_null(n);
	bn_null(_k);
	bn_null(_m);

	RLC_TRY {
		bn_new(n);
		bn_new(_k);
		bn_new(_m);
		for (i = 0; i < 5; i++) {
			ep_null(t[i]);
			ep_new(t[i]);
		}

		ep_curve_get_ord(n);
		bn_copy(_k, k);
		if (bn_cmp_abs(_k, n) == RLC_GT) {
			bn_mod(_k, _k, n);
		}
		bn_copy(_m, m);
		if (bn_cmp_abs(_m, n) == RLC_GT) {
			bn_mod(_m, _m, n);
		}

		ep_set_infty(t[0]);
		ep_copy(t[1], q);
		if (bn_sign(_m) == RLC_NEG) {
			ep_neg(t[1], t[1]);
		}
		ep_copy(t[2], p);
		if (bn_sign(_k) == RLC_NEG) {
			ep_neg(t[2], t[2]);
		}
		ep_add(t[3], t[2], t[1]);
		ep_sub(t[4], t[2], t[1]);
#if defined(EP_MIXED)
		ep_norm_sim(t + 3, (const ep_t *)t + 3, 2);
#endif

		l = 2 * (RLC_FP_BITS + 1);
		bn_rec_jsf(jsf, &l, _k, _m);

		ep_set_infty(r);

		offset = RLC_MAX(bn_bits(_k), bn_bits(_m)) + 1;
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
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(_k);
		bn_free(_m);
		for (i = 0; i < 5; i++) {
			ep_free(t[i]);
		}
	}
}

#endif

void ep_mul_sim_gen(ep_t r, const bn_t k, const ep_t q, const bn_t m) {
	ep_t g;
	bn_t n, _k, _m;

	if (bn_is_zero(k)) {
		ep_mul(r, q, m);
		return;
	}
	if (bn_is_zero(m) || ep_is_infty(q)) {
		ep_mul_gen(r, k);
		return;
	}

	ep_null(g);
	bn_null(n);
	bn_null(_k);
	bn_null(_m);

	RLC_TRY {
		ep_new(g);
		bn_new(n);
		bn_new(_k);
		bn_new(_m);

		ep_curve_get_gen(g);
		ep_curve_get_ord(n);

		bn_copy(_k, k);
		if (bn_cmp_abs(_k, n) == RLC_GT) {
			bn_mod(_k, _k, n);
		}
		bn_copy(_m, m);
		if (bn_cmp_abs(_m, n) == RLC_GT) {
			bn_mod(_m, _m, n);
		}

#if defined(EP_ENDOM)
#if EP_SIM == INTER && EP_FIX == LWNAF && defined(EP_PRECO)
		if (ep_curve_is_endom()) {
			ep_mul_sim_endom(r, g, _k, q, _m, ep_curve_get_tab());
		}
#else
		if (ep_curve_is_endom()) {
			ep_mul_sim(r, g, _k, q, _m);
		}
#endif
#endif

#if defined(EP_PLAIN) || defined(EP_SUPER)
#if EP_SIM == INTER && EP_FIX == LWNAF && defined(EP_PRECO)
		if (!ep_curve_is_endom()) {
			ep_mul_sim_plain(r, g, _k, q, _m, ep_curve_get_tab());
		}
#else
		if (!ep_curve_is_endom()) {
			ep_mul_sim(r, g, _k, q, _m);
		}
#endif
#endif
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ep_free(g);
		bn_free(n);
		bn_free(_k);
		bn_free(_m);
	}
}

void ep_mul_sim_dig(ep_t r, const ep_t p[], const dig_t k[], int len) {
	ep_t t;
	int max;

	ep_null(t);

	max = util_bits_dig(k[0]);
	for (int i = 1; i < len; i++) {
		max = RLC_MAX(max, util_bits_dig(k[i]));
	}

	RLC_TRY {
		ep_new(t);

		ep_set_infty(t);
		for (int i = max - 1; i >= 0; i--) {
			ep_dbl(t, t);
			for (int j = 0; j < len; j++) {
				if (k[j] & ((dig_t)1 << i)) {
					ep_add(t, t, p[j]);
				}
			}
		}

		ep_norm(r, t);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ep_free(t);
	}
}

void ep_mul_sim_lot(ep_t r, ep_t p[], const bn_t k[], int n) {
	const int len = RLC_FP_BITS + 1;
	int i, j, m, l, _l[2];
	bn_t _k[2], q, v1[3], v2[3];
	int8_t *naf = RLC_ALLOCA(int8_t, 2 * n * len);

	bn_null(q);

	if (n <= 10) {
		ep_t *_p = RLC_ALLOCA(ep_t, 2 * n);

		RLC_TRY {
			if (naf == NULL || _p == NULL) {
				RLC_THROW(ERR_NO_MEMORY);
			}
			bn_new(q);
			for (j = 0; j < 2; j++) {
				bn_null(_k[j]);
				bn_new(_k[j]);
			}
			for (i = 0; i < n; i++) {
				ep_null(_p[i]);
				ep_new(_p[i]);
			}

			for (i = 0; i < 3; i++) {
				bn_null(v1[i]);
				bn_null(v2[i]);
				bn_new(v1[i]);
				bn_new(v2[i]);
			}

			l = 0;
			ep_curve_get_ord(q);
			ep_curve_get_v1(v1);
			ep_curve_get_v2(v2);
			for (i = 0; i < n; i++) {
				ep_norm(_p[2*i], p[i]);
				ep_psi(_p[2*i + 1], _p[2*i]);
				bn_rec_glv(_k[0], _k[1], k[i], q, (const bn_t *)v1, (const bn_t *)v2);
				if (bn_sign(k[i]) == RLC_NEG) {
					bn_neg(_k[0], _k[0]);
					bn_neg(_k[1], _k[1]);
				}
				for (j = 0; j < 2; j++) {
					_l[j] = len;
					bn_rec_naf(&naf[(2*i + j)*len], &_l[j], _k[j], 2);
					if (bn_sign(_k[j]) == RLC_NEG) {
						ep_neg(_p[2*i + j], _p[2*i + j]);
					}
					l = RLC_MAX(l, _l[j]);
				}
			}

			ep_set_infty(r);
			for (i = l - 1; i >= 0; i--) {
				ep_dbl(r, r);
				for (j = 0; j < n; j++) {
					for (m = 0; m < 2; m++) {
						if (naf[(2*j + m)*len + i] > 0) {
							ep_add(r, r, _p[2*j + m]);
						}
						if (naf[(2*j + m)*len + i] < 0) {
							ep_sub(r, r, _p[2*j + m]);
						}
					}
				}
			}

			ep_norm(r, r);
		} RLC_CATCH_ANY {
			RLC_THROW(ERR_CAUGHT);
		} RLC_FINALLY {
			bn_free(q);
			bn_free(_k[0]);
			bn_free(_k[1]);
			for (i = 0; i < n; i++) {
				ep_free(_p[i]);
			}
			RLC_FREE(_p);
			RLC_FREE(naf);
			for (i = 0; i < 3; i++) {
				bn_free(v1[i]);
				bn_free(v2[i]);
			}
		}
	} else {
		const int w = RLC_MAX(2, util_bits_dig(n) - 2), c = (1 << (w - 2));
		ep_t s, t, u, v, *_p = RLC_ALLOCA(ep_t, 2 * c);
		int8_t ptr, *sk = RLC_ALLOCA(int8_t, 2 * n);

		ep_null(s);
		ep_null(t);
		ep_null(u);
		ep_null(v);

		RLC_TRY {
			if (naf == NULL || _p == NULL) {
				RLC_THROW(ERR_NO_MEMORY);
			}
			bn_new(q);
			ep_new(s);
			ep_new(t);
			ep_new(u);
			ep_new(v);
			for (i = 0; i < 2; i++) {
				bn_null(_k[i]);
				bn_new(_k[i]);
				for (j = 0; j < c; j++) {
					ep_null(_p[i*c + j]);
					ep_new(_p[i*c + j]);
					ep_set_infty(_p[i*c + j]);
				}
			}
			for (i = 0; i < 3; i++) {
				bn_null(v1[i]);
				bn_null(v2[i]);
				bn_new(v1[i]);
				bn_new(v2[i]);
			}

			l = 0;
			ep_curve_get_ord(q);
			ep_curve_get_v1(v1);
			ep_curve_get_v2(v2);
			for (i = 0; i < n; i++) {
				bn_rec_glv(_k[0], _k[1], k[i], q, (const bn_t *)v1, (const bn_t *)v2);
				if (bn_sign(k[i]) == RLC_NEG) {
					bn_neg(_k[0], _k[0]);
					bn_neg(_k[1], _k[1]);
				}
				for (j = 0; j < 2; j++) {
					_l[j] = len;
					bn_rec_naf(&naf[(2*i + j)*len], &_l[j], _k[j], w);
					if (bn_sign(_k[j]) == RLC_NEG) {
						for (m = 0; m < _l[j]; m++) {
							naf[(2*i + j)*len + m] = -naf[(2*i + j)*len + m];
						}
					}
					l = RLC_MAX(l, _l[j]);
				}
			}

			ep_set_infty(s);
			for (i = l - 1; i >= 0; i--) {
				for (j = 0; j < n; j++) {
					for (m = 0; m < 2; m++) {
						ptr = naf[(2*j + m)*len + i];
						if (ptr != 0) {
							ep_copy(t, p[j]);
							if (ptr < 0) {
								ptr = -ptr;
								ep_neg(t, t);
							}
							ep_add(_p[m*c + (ptr >> 1)], _p[m*c + (ptr >> 1)], t);
						}
					}
				}

				ep_set_infty(t);
				for (m = 1; m >= 0; m--) {
					ep_psi(t, t);
					ep_set_infty(u);
					ep_set_infty(v);
					for (j = c - 1; j >= 0; j--) {
						ep_add(u, u, _p[m*c + j]);
						if (j == 0) {
							ep_dbl(v, v);
						}
						ep_add(v, v, u);
						ep_set_infty(_p[m*c + j]);
					}
					ep_add(t, t, v);
				}
				ep_dbl(s, s);
				ep_add(s, s, t);
			}

			/* Convert r to affine coordinates. */
			ep_norm(r, s);
		} RLC_CATCH_ANY {
			RLC_THROW(ERR_CAUGHT);
		} RLC_FINALLY {
			bn_free(q);
			ep_free(s);
			ep_free(t);
			ep_free(u);
			ep_free(v);
			for (i = 0; i < 2; i++) {
				bn_free(_k[i]);
				for (j = 0; j < c; j++) {
					ep_free(_p[i*c + j]);
				}
			}
			RLC_FREE(sk);
			RLC_FREE(_p);
			RLC_FREE(naf);
			for (i = 0; i < 3; i++) {
				bn_free(v1[i]);
				bn_free(v2[i]);
			}
		}
	}
}
