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
 * Implementation of simultaneous point multiplication on a prime elliptic
 * curve over a quadratic extension.
 *
 * @ingroup epx
 */

#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if EP_SIM == INTER || !defined(STRIP)

static void ep2_mul_sim_plain(ep2_t r, ep2_t p, bn_t k, ep2_t q, bn_t m,
		ep2_t *t) {
	int i, l, l0, l1, n0, n1, w, gen;
	int8_t naf0[2 * FP_BITS + 1], naf1[2 * FP_BITS + 1], *_k, *_m;
	ep2_t t0[1 << (EP_WIDTH - 2)];
	ep2_t t1[1 << (EP_WIDTH - 2)];

	TRY {
		gen = (t == NULL ? 0 : 1);
		if (!gen) {
			for (i = 0; i < (1 << (EP_WIDTH - 2)); i++) {
				ep2_null(t0[i]);
				ep2_new(t0[i]);
			}
			ep2_tab(t0, p, EP_WIDTH);
			t = (ep2_t *)t0;
		}

		/* Prepare the precomputation table. */
		for (i = 0; i < (1 << (EP_WIDTH - 2)); i++) {
			ep2_null(t1[i]);
			ep2_new(t1[i]);
		}
		/* Compute the precomputation table. */
		ep2_tab(t1, q, EP_WIDTH);

		/* Compute the w-TNAF representation of k. */
		if (gen) {
			w = EP_DEPTH;
		} else {
			w = EP_WIDTH;
		}
		l0 = l1 = 2 * FP_BITS + 1;
		bn_rec_naf(naf0, &l0, k, w);
		bn_rec_naf(naf1, &l1, m, EP_WIDTH);

		l = MAX(l0, l1);
		_k = naf0 + l - 1;
		_m = naf1 + l - 1;
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

		ep2_set_infty(r);
		for (i = l - 1; i >= 0; i--, _k--, _m--) {
			ep2_dbl(r, r);

			n0 = *_k;
			n1 = *_m;
			if (n0 > 0) {
				ep2_add(r, r, t[n0 / 2]);
			}
			if (n0 < 0) {
				ep2_sub(r, r, t[-n0 / 2]);
			}
			if (n1 > 0) {
				ep2_add(r, r, t1[n1 / 2]);
			}
			if (n1 < 0) {
				ep2_sub(r, r, t1[-n1 / 2]);
			}
		}
		/* Convert r to affine coordinates. */
		ep2_norm(r, r);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		/* Free the precomputation tables. */
		if (!gen) {
			for (i = 0; i < (1 << (EP_WIDTH - 2)); i++) {
				ep2_free(t0[i]);
			}
		}
		for (i = 0; i < (1 << (EP_WIDTH - 2)); i++) {
			ep2_free(t1[i]);
		}
	}
}

#endif /* EP_SIM == INTER */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if EP_SIM == BASIC || !defined(STRIP)

void ep2_mul_sim_basic(ep2_t r, ep2_t p, bn_t k, ep2_t q, bn_t l) {
	ep2_t t;

	ep2_null(t);

	TRY {
		ep2_new(t);
		ep2_mul(t, q, l);
		ep2_mul(r, p, k);
		ep2_add(t, t, r);
		ep2_norm(r, t);

	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep2_free(t);
	}
}

#endif

#if EP_SIM == TRICK || !defined(STRIP)

void ep2_mul_sim_trick(ep2_t r, ep2_t p, bn_t k, ep2_t q, bn_t m) {
	ep2_t t0[1 << (EP_WIDTH / 2)];
	ep2_t t1[1 << (EP_WIDTH / 2)];
	ep2_t t[1 << EP_WIDTH];
	bn_t n;
	int l0, l1, w = EP_WIDTH / 2;
	uint8_t w0[CEIL(2 * FP_BITS, w)], w1[CEIL(2 * FP_BITS, w)];

	bn_null(n);

	if (bn_is_zero(k) || ep2_is_infty(p)) {
		ep2_mul(r, q, m);
		return;
	}
	if (bn_is_zero(m) || ep2_is_infty(q)) {
		ep2_mul(r, p, k);
		return;
	}

	TRY {
		bn_new(n);

		ep2_curve_get_ord(n);

		for (int i = 0; i < (1 << w); i++) {
			ep2_null(t0[i]);
			ep2_null(t1[i]);
			ep2_new(t0[i]);
			ep2_new(t1[i]);
		}
		for (int i = 0; i < (1 << EP_WIDTH); i++) {
			ep2_null(t[i]);
			ep2_new(t[i]);
		}

		ep2_set_infty(t0[0]);
		ep2_copy(t0[1], p);
		if (bn_sign(k) == BN_NEG) {
			ep2_neg(t0[1], t0[1]);
		}
		for (int i = 2; i < (1 << w); i++) {
			ep2_add(t0[i], t0[i - 1], t0[1]);
		}

		ep2_set_infty(t1[0]);
		ep2_copy(t1[1], q);
		if (bn_sign(m) == BN_NEG) {
			ep2_neg(t1[1], t1[1]);
		}
		for (int i = 1; i < (1 << w); i++) {
			ep2_add(t1[i], t1[i - 1], t1[1]);
		}

		for (int i = 0; i < (1 << w); i++) {
			for (int j = 0; j < (1 << w); j++) {
				ep2_add(t[(i << w) + j], t0[i], t1[j]);
			}
		}

#if defined(EP_MIXED)
		ep2_norm_sim(t + 1, t + 1, (1 << (EP_WIDTH)) - 1);
#endif

		l0 = l1 = CEIL(2 * FP_BITS, w);
		bn_rec_win(w0, &l0, k, w);
		bn_rec_win(w1, &l1, m, w);

		for (int i = l0; i < l1; i++) {
			w0[i] = 0;
		}
		for (int i = l1; i < l0; i++) {
			w1[i] = 0;
		}

		ep2_set_infty(r);
		for (int i = MAX(l0, l1) - 1; i >= 0; i--) {
			for (int j = 0; j < w; j++) {
				ep2_dbl(r, r);
			}
			ep2_add(r, r, t[(w0[i] << w) + w1[i]]);
		}
		ep2_norm(r, r);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(n);
		for (int i = 0; i < (1 << w); i++) {
			ep2_free(t0[i]);
			ep2_free(t1[i]);
		}
		for (int i = 0; i < (1 << EP_WIDTH); i++) {
			ep2_free(t[i]);
		}
	}
}
#endif

#if EP_SIM == INTER || !defined(STRIP)

void ep2_mul_sim_inter(ep2_t r, ep2_t p, bn_t k, ep2_t q, bn_t m) {
	if (bn_is_zero(k) || ep2_is_infty(p)) {
		ep2_mul(r, q, m);
		return;
	}
	if (bn_is_zero(m) || ep2_is_infty(q)) {
		ep2_mul(r, p, k);
		return;
	}

#if defined(EP_ENDOM)
	/* TODO. */
	//  if (ep_curve_is_endom()) {
	//      ep_mul_sim_endom(r, p, k, q, l, NULL);
	//      return;
	//  }
#endif

//#if defined(EP_PLAIN)
	ep2_mul_sim_plain(r, p, k, q, m, NULL);
//#endif
}

#endif

#if EP_SIM == JOINT || !defined(STRIP)

void ep2_mul_sim_joint(ep2_t r, ep2_t p, bn_t k, ep2_t q, bn_t m) {
	ep2_t t[5];
	int i, l, u_i, offset;
	int8_t jsf[4 * (FP_BITS + 1)];

	if (bn_is_zero(k) || ep2_is_infty(p)) {
		ep2_mul(r, q, m);
		return;
	}
	if (bn_is_zero(m) || ep2_is_infty(q)) {
		ep2_mul(r, p, k);
		return;
	}

	TRY {
		for (i = 0; i < 5; i++) {
			ep2_null(t[i]);
			ep2_new(t[i]);
		}

		ep2_set_infty(t[0]);
		ep2_copy(t[1], q);
		if (bn_sign(m) == BN_NEG) {
			ep2_neg(t[1], t[1]);
		}
		ep2_copy(t[2], p);
		if (bn_sign(k) == BN_NEG) {
			ep2_neg(t[2], t[2]);
		}
		ep2_add(t[3], t[2], t[1]);
		ep2_sub(t[4], t[2], t[1]);
#if defined(EP_MIXED)
		ep2_norm_sim(t + 3, t + 3, 2);
#endif

		l = 4 * (FP_BITS + 1);
		bn_rec_jsf(jsf, &l, k, m);

		ep2_set_infty(r);

		offset = MAX(bn_bits(k), bn_bits(m)) + 1;
		for (i = l - 1; i >= 0; i--) {
			ep2_dbl(r, r);
			if (jsf[i] != 0 && jsf[i] == -jsf[i + offset]) {
				u_i = jsf[i] * 2 + jsf[i + offset];
				if (u_i < 0) {
					ep2_sub(r, r, t[4]);
				} else {
					ep2_add(r, r, t[4]);
				}
			} else {
				u_i = jsf[i] * 2 + jsf[i + offset];
				if (u_i < 0) {
					ep2_sub(r, r, t[-u_i]);
				} else {
					ep2_add(r, r, t[u_i]);
				}
			}
		}
		ep2_norm(r, r);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		for (i = 0; i < 5; i++) {
			ep2_free(t[i]);
		}
	}
}

#endif

void ep2_mul_sim_gen(ep2_t r, bn_t k, ep2_t q, bn_t m) {
	ep2_t gen;

	ep2_null(gen);

	if (bn_is_zero(k)) {
		ep2_mul(r, q, m);
		return;
	}
	if (bn_is_zero(m) || ep2_is_infty(q)) {
		ep2_mul_gen(r, k);
		return;
	}

	TRY {
		ep2_new(gen);

		ep2_curve_get_gen(gen);
#if EP_FIX == LWNAF && defined(EP_PRECO)
		ep2_mul_sim_plain(r, gen, k, q, m, ep2_curve_get_tab());
#else
		ep2_mul_sim(r, gen, k, q, m);
#endif
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep2_free(gen);
	}
}
