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
 * Implementation of the point multiplication on Twisted Edwards elliptic curves.
 *
 * @version $Id$
 * @ingroup ed
 */

#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if ED_MUL == LWNAF || !defined(STRIP)

static void ed_mul_naf_imp(ed_t r, const ed_t p, const bn_t k) {
	int l, i, n;
	int8_t naf[RLC_FP_BITS + 1];
	ed_t t[1 << (ED_WIDTH - 2)];

	if (bn_is_zero(k)) {
		ed_set_infty(r);
		return;
	}

	RLC_TRY {
		/* Prepare the precomputation table. */
		for (i = 0; i < (1 << (ED_WIDTH - 2)); i++) {
			ed_null(t[i]);
			ed_new(t[i]);
		}
		/* Compute the precomputation table. */
		ed_tab(t, p, ED_WIDTH);

		/* Compute the w-NAF representation of k. */
		l = sizeof(naf);
		bn_rec_naf(naf, &l, k, EP_WIDTH);

		ed_set_infty(r);
		for (i = l - 1; i > 0; i--) {
			n = naf[i];
			if (n == 0) {
				/* This point will be doubled in the next iteration. */
#if ED_ADD == EXTND
				r->coord = EXTND;
#endif
			}
			ed_dbl(r, r);
			if (n > 0) {
				ed_add(r, r, t[n / 2]);
			} else if (n < 0) {
				ed_sub(r, r, t[-n / 2]);
			}
		}

		/* Last iteration. */
		n = naf[0];
		ed_dbl(r, r);
		if (n > 0) {
			ed_add(r, r, t[n / 2]);
		} else if (n < 0) {
			ed_sub(r, r, t[-n / 2]);
		}

		/* Convert r to affine coordinates. */
		ed_norm(r, r);
		if (bn_sign(k) == RLC_NEG) {
			ed_neg(r, r);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		/* Free the precomputation table. */
		for (i = 0; i < (1 << (ED_WIDTH - 2)); i++) {
			ed_free(t[i]);
		}
	}
}

#endif /* ED_MUL == LWNAF */

#if ED_MUL == LWREG || !defined(STRIP)

static void ed_mul_reg_imp(ed_t r, const ed_t p, const bn_t k) {
	bn_t _k;
	int i, j, l, n;
	int8_t s, reg[RLC_CEIL(RLC_FP_BITS + 1, ED_WIDTH - 1)];
	ed_t t[1 << (ED_WIDTH - 2)], u, v;

	bn_null(_k);
	if (bn_is_zero(k)) {
		ed_set_infty(r);
		return;
	}

	RLC_TRY {
		bn_new(_k);
		ed_new(u);
		ed_new(v);
		/* Prepare the precomputation table. */
		for (i = 0; i < (1 << (ED_WIDTH - 2)); i++) {
			ed_null(t[i]);
			ed_new(t[i]);
		}
		/* Compute the precomputation table. */
		ed_tab(t, p, ED_WIDTH);

		/* Make a copy of the scalar for processing. */
		bn_abs(_k, k);
		_k->dp[0] |= bn_is_even(_k);

		/* Compute the w-NAF representation of k. */
		l = RLC_CEIL(RLC_FP_BITS + 1, ED_WIDTH - 1);
		bn_rec_reg(reg, &l, _k, RLC_FP_BITS, ED_WIDTH);

		ed_set_infty(r);
		for (i = l - 1; i >= 0; i--) {
			for (j = 0; j < ED_WIDTH - 1; j++) {
#if ED_ADD == EXTND
				r->coord = EXTND;
#endif
				ed_dbl(r, r);
			}

			n = reg[i];
			s = (n >> 7);
			n = ((n ^ s) - s) >> 1;

			for (j = 0; j < (1 << (EP_WIDTH - 2)); j++) {
				dv_copy_cond(u->x, t[j]->x, RLC_FP_DIGS, j == n);
				dv_copy_cond(u->y, t[j]->y, RLC_FP_DIGS, j == n);
				dv_copy_cond(u->z, t[j]->z, RLC_FP_DIGS, j == n);
			}
			ed_neg(v, u);
			dv_copy_cond(u->x, v->x, RLC_FP_DIGS, s != 0);
			ed_add(r, r, u);
		}

		/* t[0] has an unmodified copy of p. */
		ed_sub(u, r, t[0]);
		dv_copy_cond(r->x, u->x, RLC_FP_DIGS, bn_is_even(k));
		dv_copy_cond(r->y, u->y, RLC_FP_DIGS, bn_is_even(k));
		dv_copy_cond(r->z, u->z, RLC_FP_DIGS, bn_is_even(k));
		/* Convert r to affine coordinates. */
		ed_norm(r, r);
		ed_neg(u, r);
		dv_copy_cond(r->x, u->x, RLC_FP_DIGS, bn_sign(k) == RLC_NEG);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		/* Free the precomputation table. */
		for (i = 0; i < (1 << (ED_WIDTH - 2)); i++) {
			ed_free(t[i]);
		}
		bn_free(_k);
	}
}

#endif /* ED_MUL == LWNAF */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if ED_MUL == BASIC || !defined(STRIP)

void ed_mul_basic(ed_t r, const ed_t p, const bn_t k) {
	ed_t t;

	ed_null(t);

	if (bn_is_zero(k) || ed_is_infty(p)) {
		ed_set_infty(r);
		return;
	}

	RLC_TRY {
		ed_new(t);

		ed_copy(t, p);
		for (int i = bn_bits(k) - 2; i >= 0; i--) {
			ed_dbl(t, t);
			if (bn_get_bit(k, i)) {
				ed_add(t, t, p);
			}
		}

		ed_norm(r, t);
		if (bn_sign(k) == RLC_NEG) {
			ed_neg(r, r);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ed_free(t);
	}
}

#endif

#if ED_MUL == SLIDE || !defined(STRIP)

void ed_mul_slide(ed_t r, const ed_t p, const bn_t k) {
	ed_t t[1 << (EP_WIDTH - 1)], q;
	int i, j, l;
	uint8_t win[RLC_FP_BITS + 1];

	ed_null(q);

	if (bn_is_zero(k) || ed_is_infty(p)) {
		ed_set_infty(r);
		return;
	}

	RLC_TRY {
		for (i = 0; i < (1 << (EP_WIDTH - 1)); i ++) {
			ed_null(t[i]);
			ed_new(t[i]);
		}

		ed_new(q);

		ed_copy(t[0], p);
		ed_dbl(q, p);

#if defined(EP_MIXED)
		ed_norm(q, q);
#endif

		/* Create table. */
		for (i = 1; i < (1 << (EP_WIDTH - 1)); i++) {
			ed_add(t[i], t[i - 1], q);
		}

#if defined(EP_MIXED)
		ed_norm_sim(t + 1, (const ed_t *)t + 1, (1 << (EP_WIDTH - 1)) - 1);
#endif

		ed_set_infty(q);
		l = RLC_FP_BITS + 1;
		bn_rec_slw(win, &l, k, EP_WIDTH);
		for (i = 0; i < l; i++) {
			if (win[i] == 0) {
				ed_dbl(q, q);
			} else {
				for (j = 0; j < util_bits_dig(win[i]); j++) {
					ed_dbl(q, q);
				}
				ed_add(q, q, t[win[i] >> 1]);
			}
		}

		ed_norm(r, q);
		if (bn_sign(k) == RLC_NEG) {
			ed_neg(r, r);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		for (i = 0; i < (1 << (EP_WIDTH - 1)); i++) {
			ed_free(t[i]);
		}
		ed_free(q);
	}
}

#endif

#if ED_MUL == MONTY || !defined(STRIP)

void ed_mul_monty(ed_t r, const ed_t p, const bn_t k) {
	ed_t t[2];

	ed_null(t[0]);
	ed_null(t[1]);

	if (bn_is_zero(k) || ed_is_infty(p)) {
		ed_set_infty(r);
		return;
	}

	RLC_TRY {
		ed_new(t[0]);
		ed_new(t[1]);

		ed_set_infty(t[0]);
		ed_copy(t[1], p);

		for (int i = bn_bits(k) - 1; i >= 0; i--) {
			int j = bn_get_bit(k, i);

			dv_swap_cond(t[0]->x, t[1]->x, RLC_FP_DIGS, j ^ 1);
			dv_swap_cond(t[0]->y, t[1]->y, RLC_FP_DIGS, j ^ 1);
			dv_swap_cond(t[0]->z, t[1]->z, RLC_FP_DIGS, j ^ 1);
#if ED_ADD == EXTND
			dv_swap_cond(t[0]->t, t[1]->t, RLC_FP_DIGS, j ^ 1);
#endif
			ed_add(t[0], t[0], t[1]);
			ed_dbl(t[1], t[1]);
			dv_swap_cond(t[0]->x, t[1]->x, RLC_FP_DIGS, j ^ 1);
			dv_swap_cond(t[0]->y, t[1]->y, RLC_FP_DIGS, j ^ 1);
			dv_swap_cond(t[0]->z, t[1]->z, RLC_FP_DIGS, j ^ 1);
#if ED_ADD == EXTND
			dv_swap_cond(t[0]->t, t[1]->t, RLC_FP_DIGS, j ^ 1);
#endif
		}

		ed_norm(r, t[0]);
		if (bn_sign(k) == RLC_NEG) {
			ed_neg(r, r);
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ed_free(t[1]);
		ed_free(t[0]);
	}
}

#endif

#if ED_MUL == LWNAF || !defined(STRIP)

void ed_mul_lwnaf(ed_t r, const ed_t p, const bn_t k) {
	if (bn_is_zero(k) || ed_is_infty(p)) {
		ed_set_infty(r);
		return;
	}

	ed_mul_naf_imp(r, p, k);
}

#endif

#if ED_MUL == LWREG || !defined(STRIP)

void ed_mul_lwreg(ed_t r, const ed_t p, const bn_t k) {
	if (bn_is_zero(k) || ed_is_infty(p)) {
		ed_set_infty(r);
		return;
	}

	ed_mul_reg_imp(r, p, k);
}

#endif

void ed_mul_gen(ed_t r, const bn_t k) {
	if (bn_is_zero(k)) {
		ed_set_infty(r);
		return;
	}
#ifdef ED_PRECO
	ed_mul_fix(r, ed_curve_get_tab(), k);
#else
	ed_t g;

	ed_null(g);

	RLC_TRY {
		ed_new(g);
		ed_curve_get_gen(g);
		ed_mul(r, g, k);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ed_free(g);
	}
#endif
}

void ed_mul_dig(ed_t r, const ed_t p, dig_t k) {
	ed_t t;
	bn_t _k;
	int8_t u, naf[RLC_DIG + 1];
	int l;

	ed_null(t);
	bn_null(_k);

	if (k == 0 || ed_is_infty(p)) {
		ed_set_infty(r);
		return;
	}

	RLC_TRY {
		ed_new(t);
		bn_new(_k);

		bn_set_dig(_k, k);

		l = RLC_DIG + 1;
		bn_rec_naf(naf, &l, _k, 2);

		ed_set_infty(t);
		for (int i = l - 1; i >= 0; i--) {
			ed_dbl(t, t);

			u = naf[i];
			if (u > 0) {
				ed_add(t, t, p);
			} else if (u < 0) {
				ed_sub(t, t, p);
			}
		}

		ed_norm(r, t);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ed_free(t);
		bn_free(_k);
	}
}
