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
 * @version $Id$
 * @ingroup ep
 */

#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if ED_SIM == INTER || !defined(STRIP)

#if defined(ED_ENDOM)

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
void ed_mul_sim_endom(ed_t r, const ed_t p, const bn_t k, const ed_t q,
		const bn_t m, const ed_t * t) {
	int len, len0, len1, len2, len3, i, n, sk0, sk1, sl0, sl1, w, g = 0;
	int8_t naf0[FP_BITS + 1], naf1[FP_BITS + 1], *t0, *t1;
	int8_t naf2[FP_BITS + 1], naf3[FP_BITS + 1], *t2, *t3;
	bn_t k0, k1, l0, l1;
	bn_t ord, v1[3], v2[3];
	ed_t u;
	ed_t tab0[1 << (ED_WIDTH - 2)];
	ed_t tab1[1 << (ED_WIDTH - 2)];

	bn_null(ord);
	bn_null(k0);
	bn_null(k1);
	bn_null(l0);
	bn_null(l1);
	ed_null(u);

	for (i = 0; i < (1 << (ED_WIDTH - 2)); i++) {
		ed_null(tab0[i]);
		ed_null(tab1[i]);
	}

	bn_new(ord);
	bn_new(k0);
	bn_new(k1);
	bn_new(l0);
	bn_new(l1);
	ed_new(u);

	TRY {
		for (i = 0; i < 3; i++) {
			bn_null(v1[i]);
			bn_null(v2[i]);
			bn_new(v1[i]);
			bn_new(v2[i]);
		}

		ed_curve_get_ord(ord);
		ed_curve_get_v1(v1);
		ed_curve_get_v2(v2);

		bn_rec_glv(k0, k1, k, ord, (const bn_t *)v1, (const bn_t *)v2);
		sk0 = bn_sign(k0);
		sk1 = bn_sign(k1);
		bn_abs(k0, k0);
		bn_abs(k1, k1);

		bn_rec_glv(l0, l1, m, ord, (const bn_t *)v1, (const bn_t *)v2);
		sl0 = bn_sign(l0);
		sl1 = bn_sign(l1);
		bn_abs(l0, l0);
		bn_abs(l1, l1);

		g = (t == NULL ? 0 : 1);
		if (!g) {
			for (i = 0; i < (1 << (ED_WIDTH - 2)); i++) {
				ed_new(tab0[i]);
			}
			ed_tab(tab0, p, ED_WIDTH);
			t = (const ed_t *)tab0;
		}

		/* Prepare the precomputation table. */
		for (i = 0; i < (1 << (ED_WIDTH - 2)); i++) {
			ed_new(tab1[i]);
		}
		/* Compute the precomputation table. */
		ed_tab(tab1, q, ED_WIDTH);

		/* Compute the w-TNAF representation of k and l */
		if (g) {
			w = ED_DEPTH;
		} else {
			w = ED_WIDTH;
		}
		len0 = len1 = len2 = len3 = FP_BITS + 1;
		bn_rec_naf(naf0, &len0, k0, w);
		bn_rec_naf(naf1, &len1, k1, w);
		bn_rec_naf(naf2, &len2, l0, ED_WIDTH);
		bn_rec_naf(naf3, &len3, l1, ED_WIDTH);

		len = MAX(MAX(len0, len1), MAX(len2, len3));
		t0 = naf0 + len - 1;
		t1 = naf1 + len - 1;
		t2 = naf2 + len - 1;
		t3 = naf3 + len - 1;
		for (i = len0; i < len; i++) {
			naf0[i] = 0;
		}
		for (i = len1; i < len; i++) {
			naf1[i] = 0;
		}
		for (i = len2; i < len; i++) {
			naf2[i] = 0;
		}
		for (i = len3; i < len; i++) {
			naf3[i] = 0;
		}

		ed_set_infty(r);
		for (i = len - 1; i >= 0; i--, t0--, t1--, t2--, t3--) {
			ed_dbl(r, r);

			n = *t0;
			if (n > 0) {
				if (sk0 == BN_POS) {
					ed_add(r, r, t[n / 2]);
				} else {
					ed_sub(r, r, t[n / 2]);
				}
			}
			if (n < 0) {
				if (sk0 == BN_POS) {
					ed_sub(r, r, t[-n / 2]);
				} else {
					ed_add(r, r, t[-n / 2]);
				}
			}
			n = *t1;
			if (n > 0) {
				ed_copy(u, t[n / 2]);
				fp_mul(u->x, u->x, ed_curve_get_beta());
				if (sk1 == BN_NEG) {
					ed_neg(u, u);
				}
				ed_add(r, r, u);
			}
			if (n < 0) {
				ed_copy(u, t[-n / 2]);
				fp_mul(u->x, u->x, ed_curve_get_beta());
				if (sk1 == BN_NEG) {
					ed_neg(u, u);
				}
				ed_sub(r, r, u);
			}

			n = *t2;
			if (n > 0) {
				if (sl0 == BN_POS) {
					ed_add(r, r, tab1[n / 2]);
				} else {
					ed_sub(r, r, tab1[n / 2]);
				}
			}
			if (n < 0) {
				if (sl0 == BN_POS) {
					ed_sub(r, r, tab1[-n / 2]);
				} else {
					ed_add(r, r, tab1[-n / 2]);
				}
			}
			n = *t3;
			if (n > 0) {
				ed_copy(u, tab1[n / 2]);
				fp_mul(u->x, u->x, ed_curve_get_beta());
				if (sl1 == BN_NEG) {
					ed_neg(u, u);
				}
				ed_add(r, r, u);
			}
			if (n < 0) {
				ed_copy(u, tab1[-n / 2]);
				fp_mul(u->x, u->x, ed_curve_get_beta());
				if (sl1 == BN_NEG) {
					ed_neg(u, u);
				}
				ed_sub(r, r, u);
			}
		}
		/* Convert r to affine coordinates. */
		ed_norm(r, r);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(ord);
		bn_free(k0);
		bn_free(k1);
		bn_free(l0);
		bn_free(l1);
		ed_free(u);

		if (!g) {
			for (i = 0; i < 1 << (ED_WIDTH - 2); i++) {
				ed_free(tab0[i]);
			}
		}
		/* Free the precomputation tables. */
		for (i = 0; i < 1 << (ED_WIDTH - 2); i++) {
			ed_free(tab1[i]);
		}
		for (i = 0; i < 3; i++) {
			bn_free(v1[i]);
			bn_free(v2[i]);
		}
	}
}

#endif /* ED_ENDOM */

#if defined(ED_PLAIN) || defined(ED_SUPER)

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
		const bn_t m, const ed_t * t) {
	int len, l0, l1, i, n0, n1, w, gen;
	int8_t naf0[FP_BITS + 1], naf1[FP_BITS + 1], *_k, *_m;
	ed_t t0[1 << (ED_WIDTH - 2)];
	ed_t t1[1 << (ED_WIDTH - 2)];

	for (i = 0; i < (1 << (ED_WIDTH - 2)); i++) {
		ed_null(t0[i]);
		ed_null(t1[i]);
	}

	TRY {
		gen = (t == NULL ? 0 : 1);
		if (!gen) {
			for (i = 0; i < (1 << (ED_WIDTH - 2)); i++) {
				ed_new(t0[i]);
			}
			ed_tab(t0, p, ED_WIDTH);
			t = (const ed_t *)t0;
		}

		/* Prepare the precomputation table. */
		for (i = 0; i < (1 << (ED_WIDTH - 2)); i++) {
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
		l0 = l1 = FP_BITS + 1;
		bn_rec_naf(naf0, &l0, k, w);
		bn_rec_naf(naf1, &l1, m, ED_WIDTH);

		len = MAX(l0, l1);
		_k = naf0 + len - 1;
		_m = naf1 + len - 1;
		for (i = l0; i < len; i++)
			naf0[i] = 0;
		for (i = l1; i < len; i++)
			naf1[i] = 0;

		ed_set_infty(r);
		for (i = len - 1; i >= 0; i--, _k--, _m--) {
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
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
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

#endif /* ED_PLAIN || ED_SUPER */

#endif /* ED_SIM == INTER */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if ED_SIM == BASIC || !defined(STRIP)

void ed_mul_sim_basic(ed_t r, const ed_t p, const bn_t k, const ed_t q,
		const bn_t m) {
	ed_t t;

	ed_null(t);

	TRY {
		ed_new(t);
		ed_mul(t, q, m);
		ed_mul(r, p, k);
		ed_add(t, t, r);
		ed_norm(r, t);

	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
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
	uint8_t w0[CEIL(FP_BITS + 1, w)], w1[CEIL(FP_BITS + 1, w)];

	bn_null(n);

	for (int i = 0; i < 1 << ED_WIDTH; i++) {
		ed_null(t[i]);
	}

	for (int i = 0; i < 1 << (ED_WIDTH / 2); i++) {
		ed_null(t0[i]);
		ed_null(t1[i]);
	}

	TRY {
		bn_new(n);

		ed_curve_get_ord(n);

		for (int i = 0; i < (1 << w); i++) {
			ed_new(t0[i]);
			ed_new(t1[i]);
		}
		for (int i = 0; i < (1 << ED_WIDTH); i++) {
			ed_new(t[i]);
		}

		ed_set_infty(t0[0]);
		for (int i = 1; i < (1 << w); i++) {
			ed_add(t0[i], t0[i - 1], p);
		}

		ed_set_infty(t1[0]);
		for (int i = 1; i < (1 << w); i++) {
			ed_add(t1[i], t1[i - 1], q);
		}

		for (int i = 0; i < (1 << w); i++) {
			for (int j = 0; j < (1 << w); j++) {
				ed_add(t[(i << w) + j], t0[i], t1[j]);
			}
		}

#if defined(ED_MIXED)
		ed_norm_sim(t + 1, (const ed_t *)t + 1, (1 << (ED_WIDTH)) - 1);
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

		ed_set_infty(r);
		for (int i = MAX(l0, l1) - 1; i >= 0; i--) {
			for (int j = 0; j < w; j++) {
				ed_dbl(r, r);
			}
			ed_add(r, r, t[(w0[i] << w) + w1[i]]);
		}
		ed_norm(r, r);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
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
#if defined(ED_ENDOM)
	if (ed_curve_is_endom()) {
		ed_mul_sim_endom(r, p, k, q, m, NULL);
		return;
	}
#endif

#if defined(ED_PLAIN) || defined(ED_SUPER)
	ed_mul_sim_plain(r, p, k, q, m, NULL);
#endif
}

#endif

#if ED_SIM == JOINT || !defined(STRIP)

void ed_mul_sim_joint(ed_t r, const ed_t p, const bn_t k, const ed_t q,
		const bn_t m) {
	ed_t t[5];
	int u_i, len, offset;
	int8_t jsf[2 * (FP_BITS + 1)];
	int i;

	ed_null(t[0]);
	ed_null(t[1]);
	ed_null(t[2]);
	ed_null(t[3]);
	ed_null(t[4]);

	TRY {
		for (i = 0; i < 5; i++) {
			ed_new(t[i]);
		}

		ed_set_infty(t[0]);
		ed_copy(t[1], q);
		ed_copy(t[2], p);
		ed_add(t[3], p, q);
		ed_sub(t[4], p, q);
#if defined(ED_MIXED)
		ed_norm_sim(t + 3, (const ed_t *)t + 3, 2);
#endif

		len = 2 * (FP_BITS + 1);
		bn_rec_jsf(jsf, &len, k, m);

		ed_set_infty(r);

		offset = MAX(bn_bits(k), bn_bits(m)) + 1;
		for (i = len - 1; i >= 0; i--) {
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
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		for (i = 0; i < 5; i++) {
			ed_free(t[i]);
		}
	}
}

#endif

void ed_mul_sim_gen(ed_t r, const bn_t k, const ed_t q, const bn_t m) {
	ed_t g;

	ed_null(g);

	TRY {
		ed_new(g);

		ed_curve_get_gen(g);

#if defined(ED_ENDOM)
#if ED_SIM == INTER && ED_FIX == LWNAF && defined(ED_PRECO)
		if (ed_curve_is_endom()) {
			ed_mul_sim_endom(r, g, k, q, m, ed_curve_get_tab());
		}
#else
		if (ed_curve_is_endom()) {
			ed_mul_sim(r, g, k, q, m);
		}
#endif
#endif

#if defined(ED_PLAIN) || defined(ED_SUPER)
#if ED_SIM == INTER && ED_FIX == LWNAF && defined(ED_PRECO)
		//if (!ed_curve_is_endom()) {
		ed_mul_sim_plain(r, g, k, q, m, ed_curve_get_tab());
		//}
#else
		//if (!ed_curve_is_endom()) {
		ed_mul_sim(r, g, k, q, m);
		//}
#endif
#endif
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ed_free(g);
	}
}
