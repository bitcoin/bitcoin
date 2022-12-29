/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2014 RELIC Authors
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
 * @version $Id$
 * @ingroup ep
 */

#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if ED_SIM == INTER || !defined(STRIP)

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
static void ed_mul_sim_plain(ed_t r, const ed_t p, const bn_t k, const ed_t q,
		const bn_t m, const ed_t *t) {
	int i, l, l0, l1, n0, n1, w, gen;
	int8_t naf0[RLC_FP_BITS + 1], naf1[RLC_FP_BITS + 1], *_k, *_m;
	ed_t t0[1 << (ED_WIDTH - 2)];
	ed_t t1[1 << (ED_WIDTH - 2)];

	RLC_TRY {
		gen = (t == NULL ? 0 : 1);
		if (!gen) {
			for (i = 0; i < (1 << (ED_WIDTH - 2)); i++) {
				ed_null(t0[i]);
				ed_new(t0[i]);
			}
			ed_tab(t0, p, ED_WIDTH);
			t = (const ed_t *)t0;
		}

		/* Prepare the precomputation table. */
		for (i = 0; i < (1 << (ED_WIDTH - 2)); i++) {
			ed_null(t1[i]);
			ed_new(t1[i]);
		}
		/* Compute the precomputation table. */
		ed_tab(t1, q, ED_WIDTH);

		/* Compute the w-TNAF representation of k. */
		if (gen) {
			w = ED_DEPTH;
		} else {
			w = ED_WIDTH;
		}
		l0 = l1 = RLC_FP_BITS + 1;
		bn_rec_naf(naf0, &l0, k, w);
		bn_rec_naf(naf1, &l1, m, ED_WIDTH);

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

		_k = naf0 + l - 1;
		_m = naf1 + l - 1;
		ed_set_infty(r);
		for (i = l - 1; i >= 0; i--, _k--, _m--) {
			ed_dbl(r, r);

			n0 = *_k;
			n1 = *_m;
			if (n0 > 0) {
				ed_add(r, r, t[n0 / 2]);
			}
			if (n0 < 0) {
				ed_sub(r, r, t[-n0 / 2]);
			}
			if (n1 > 0) {
				ed_add(r, r, t1[n1 / 2]);
			}
			if (n1 < 0) {
				ed_sub(r, r, t1[-n1 / 2]);
			}
		}
		/* Convert r to affine coordinates. */
		ed_norm(r, r);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		/* Free the precomputation tables. */
		if (!gen) {
			for (i = 0; i < 1 << (ED_WIDTH - 2); i++) {
				ed_free(t0[i]);
			}
		}
		for (i = 0; i < 1 << (ED_WIDTH - 2); i++) {
			ed_free(t1[i]);
		}
	}
}

#endif /* ED_SIM == INTER */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if ED_SIM == BASIC || !defined(STRIP)

void ed_mul_sim_basic(ed_t r, const ed_t p, const bn_t k, const ed_t q,
		const bn_t m) {
	ed_t t;

	ed_null(t);

	RLC_TRY {
		ed_new(t);
		ed_mul(t, q, m);
		ed_mul(r, p, k);
		ed_add(t, t, r);
		ed_norm(r, t);

	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ed_free(t);
	}
}

#endif

#if ED_SIM == TRICK || !defined(STRIP)

void ed_mul_sim_trick(ed_t r, const ed_t p, const bn_t k, const ed_t q,
		const bn_t m) {
	ed_t t0[1 << (ED_WIDTH / 2)], t1[1 << (ED_WIDTH / 2)], t[1 << ED_WIDTH];
	bn_t n;
	int l0, l1, w = ED_WIDTH / 2;
	uint8_t w0[RLC_FP_BITS + 1], w1[RLC_FP_BITS + 1];

	bn_null(n);

	if (bn_is_zero(k) || ed_is_infty(p)) {
		ed_mul(r, q, m);
		return;
	}
	if (bn_is_zero(m) || ed_is_infty(q)) {
		ed_mul(r, p, k);
		return;
	}

	RLC_TRY {
		bn_new(n);

		ed_curve_get_ord(n);

		for (int i = 0; i < (1 << w); i++) {
			ed_null(t0[i]);
			ed_null(t1[i]);
			ed_new(t0[i]);
			ed_new(t1[i]);
		}
		for (int i = 0; i < (1 << ED_WIDTH); i++) {
			ed_null(t[i]);
			ed_new(t[i]);
		}

		ed_set_infty(t0[0]);
		ed_copy(t0[1], p);
		if (bn_sign(k) == RLC_NEG) {
			ed_neg(t0[1], t0[1]);
		}
		for (int i = 2; i < (1 << w); i++) {
			ed_add(t0[i], t0[i - 1], t0[1]);
		}

		ed_set_infty(t1[0]);
		ed_copy(t1[1], q);
		if (bn_sign(m) == RLC_NEG) {
			ed_neg(t1[1], t1[1]);
		}
		for (int i = 1; i < (1 << w); i++) {
			ed_add(t1[i], t1[i - 1], t1[1]);
		}

		for (int i = 0; i < (1 << w); i++) {
			for (int j = 0; j < (1 << w); j++) {
				ed_add(t[(i << w) + j], t0[i], t1[j]);
			}
		}

#if defined(ED_MIXED)
		ed_norm_sim(t + 1, (const ed_t *)t + 1, (1 << (ED_WIDTH)) - 1);
#endif

		l0 = l1 = RLC_CEIL(RLC_FP_BITS, w);
		bn_rec_win(w0, &l0, k, w);
		bn_rec_win(w1, &l1, m, w);

		for (int i = l0; i < l1; i++) {
			w0[i] = 0;
		}
		for (int i = l1; i < l0; i++) {
			w1[i] = 0;
		}

		ed_set_infty(r);
		for (int i = RLC_MAX(l0, l1) - 1; i >= 0; i--) {
			for (int j = 0; j < w; j++) {
				ed_dbl(r, r);
			}
			ed_add(r, r, t[(w0[i] << w) + w1[i]]);
		}
		ed_norm(r, r);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(n);
		for (int i = 0; i < (1 << w); i++) {
			ed_free(t0[i]);
			ed_free(t1[i]);
		}
		for (int i = 0; i < (1 << ED_WIDTH); i++) {
			ed_free(t[i]);
		}
	}
}
#endif

#if ED_SIM == INTER || !defined(STRIP)

void ed_mul_sim_inter(ed_t r, const ed_t p, const bn_t k, const ed_t q,
		const bn_t m) {

	if (bn_is_zero(k) || ed_is_infty(p)) {
		ed_mul(r, q, m);
		return;
	}
	if (bn_is_zero(m) || ed_is_infty(q)) {
		ed_mul(r, p, k);
		return;
	}

	ed_mul_sim_plain(r, p, k, q, m, NULL);
}

#endif

#if ED_SIM == JOINT || !defined(STRIP)

void ed_mul_sim_joint(ed_t r, const ed_t p, const bn_t k, const ed_t q,
		const bn_t m) {
	ed_t t[5];
	int i, l, u_i, offset;
	int8_t jsf[2 * (RLC_FP_BITS + 1)];

	if (bn_is_zero(k) || ed_is_infty(p)) {
		ed_mul(r, q, m);
		return;
	}
	if (bn_is_zero(m) || ed_is_infty(q)) {
		ed_mul(r, p, k);
		return;
	}

	RLC_TRY {
		for (i = 0; i < 5; i++) {
			ed_null(t[i]);
			ed_new(t[i]);
		}

		ed_set_infty(t[0]);
		ed_copy(t[1], q);
		if (bn_sign(m) == RLC_NEG) {
			ed_neg(t[1], t[1]);
		}
		ed_copy(t[2], p);
		if (bn_sign(k) == RLC_NEG) {
			ed_neg(t[2], t[2]);
		}
		ed_add(t[3], t[2], t[1]);
		ed_sub(t[4], t[2], t[1]);
#if defined(ED_MIXED)
		ed_norm_sim(t + 3, (const ed_t *)t + 3, 2);
#endif

		l = 2 * (RLC_FP_BITS + 1);
		bn_rec_jsf(jsf, &l, k, m);

		ed_set_infty(r);

		offset = RLC_MAX(bn_bits(k), bn_bits(m)) + 1;
		for (i = l - 1; i >= 0; i--) {
			ed_dbl(r, r);
			if (jsf[i] != 0 && jsf[i] == -jsf[i + offset]) {
				u_i = jsf[i] * 2 + jsf[i + offset];
				if (u_i < 0) {
					ed_sub(r, r, t[4]);
				} else {
					ed_add(r, r, t[4]);
				}
			} else {
				u_i = jsf[i] * 2 + jsf[i + offset];
				if (u_i < 0) {
					ed_sub(r, r, t[-u_i]);
				} else {
					ed_add(r, r, t[u_i]);
				}
			}
		}
		ed_norm(r, r);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		for (i = 0; i < 5; i++) {
			ed_free(t[i]);
		}
	}
}

#endif

void ed_mul_sim_gen(ed_t r, const bn_t k, const ed_t q, const bn_t m) {
	ed_t g;

	ed_null(g);

	if (bn_is_zero(k)) {
		ed_mul(r, q, m);
		return;
	}
	if (bn_is_zero(m) || ed_is_infty(q)) {
		ed_mul_gen(r, k);
		return;
	}

	RLC_TRY {
		ed_new(g);

		ed_curve_get_gen(g);

#if ED_SIM == INTER && ED_FIX == LWNAF && defined(ED_PRECO)
		ed_mul_sim_plain(r, g, k, q, m, ed_curve_get_tab());
#else
		ed_mul_sim(r, g, k, q, m);
#endif
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ed_free(g);
	}
}
