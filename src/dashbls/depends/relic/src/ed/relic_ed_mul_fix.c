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
 * Implementation of fixed point multiplication on prime elliptic Edwards curves.
 *
 * @version $Id$
 * @ingroup ed
 */

#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if ED_FIX == LWNAF || !defined(STRIP)

/**
 * Multiplies a binary elliptic curve point by an integer using the w-NAF mixed coordinate
 * method.
 *
 * @param[out] r 				- the result.
 * @param[in] t					- the precomputed table.
 * @param[in] k					- the integer.
 */
static void ed_mul_fix_plain(ed_t r, const ed_t * t, const bn_t k) {
	int l, i, n;
	int8_t naf[RLC_FP_BITS + 1], *_k;

	/* Compute the w-TNAF representation of k. */
	l = RLC_FP_BITS + 1;
	bn_rec_naf(naf, &l, k, ED_DEPTH);

	_k = naf + l - 1;
	ed_set_infty(r);
	for (i = l - 1; i >= 0; i--, _k--) {
		n = *_k;
		if (n == 0) {
			/* doubling is followed by another doubling */
			if (i > 0) {
				r->coord = EXTND;
				ed_dbl(r, r);
			} else {
				/* use full extended coordinate doubling for last step */
				ed_dbl(r, r);
			}
		} else {
			ed_dbl(r, r);
			if (n > 0) {
				ed_add(r, r, t[n / 2]);
			} else if (n < 0) {
				ed_sub(r, r, t[-n / 2]);
			}
		}
	}
	/* Convert r to affine coordinates. */
	ed_norm(r, r);
	if (bn_sign(k) == RLC_NEG) {
		ed_neg(r, r);
	}
}

#endif /* ED_FIX == LWNAF */

#if ED_FIX == COMBS || !defined(STRIP)

/**
 * Multiplies a prime elliptic curve point by an integer using the COMBS
 * method.
 *
 * @param[out] r 				- the result.
 * @param[in] t					- the precomputed table.
 * @param[in] k					- the integer.
 */
static void ed_mul_combs_plain(ed_t r, const ed_t * t, const bn_t k) {
	int i, j, l, w, n0, p0, p1;
	bn_t n;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		ed_curve_get_ord(n);
		l = bn_bits(n);
		l = ((l % ED_DEPTH) == 0 ? (l / ED_DEPTH) : (l / ED_DEPTH) + 1);

		n0 = bn_bits(k);

		p0 = (ED_DEPTH) * l - 1;

		w = 0;
		p1 = p0--;
		for (j = ED_DEPTH - 1; j >= 0; j--, p1 -= l) {
			w = w << 1;
			if (p1 < n0 && bn_get_bit(k, p1)) {
				w = w | 1;
			}
		}
		ed_copy(r, t[w]);

		for (i = l - 2; i >= 0; i--) {
			ed_dbl(r, r);

			w = 0;
			p1 = p0--;
			for (j = ED_DEPTH - 1; j >= 0; j--, p1 -= l) {
				w = w << 1;
				if (p1 < n0 && bn_get_bit(k, p1)) {
					w = w | 1;
				}
			}
			if (w > 0) {
				ed_add(r, r, t[w]);
			}
		}
		ed_norm(r, r);
		if (bn_sign(k) == RLC_NEG) {
			ed_neg(r, r);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(n);
	}
}

#endif /* ED_FIX == COMBS */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if ED_FIX == BASIC || !defined(STRIP)

void ed_mul_pre_basic(ed_t * t, const ed_t p) {
	bn_t n;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		ed_curve_get_ord(n);

		ed_copy(t[0], p);
		for (int i = 1; i < bn_bits(n); i++) {
			ed_dbl(t[i], t[i - 1]);
		}

		ed_norm_sim(t + 1, (const ed_t *)t + 1, bn_bits(n) - 1);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(n);
	}
}

void ed_mul_fix_basic(ed_t r, const ed_t *t, const bn_t k) {
	if (bn_is_zero(k)) {
		ed_set_infty(r);
		return;
	}

	ed_set_infty(r);

	for (int i = 0; i < bn_bits(k); i++) {
		if (bn_get_bit(k, i)) {
			ed_add(r, r, t[i]);
		}
	}
	ed_norm(r, r);
	if (bn_sign(k) == RLC_NEG) {
		ed_neg(r, r);
	}
}

#endif

#if ED_FIX == COMBS || !defined(STRIP)

void ed_mul_pre_combs(ed_t * t, const ed_t p) {
	int i, j, l;
	bn_t n;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		ed_curve_get_ord(n);
		l = bn_bits(n);
		l = ((l % ED_DEPTH) == 0 ? (l / ED_DEPTH) : (l / ED_DEPTH) + 1);

		ed_set_infty(t[0]);

		ed_copy(t[1], p);
		for (j = 1; j < ED_DEPTH; j++) {
			ed_dbl(t[1 << j], t[1 << (j - 1)]);
			for (i = 1; i < l; i++) {
				ed_dbl(t[1 << j], t[1 << j]);
			}
#if defined(ED_MIXED)
			ed_norm(t[1 << j], t[1 << j]);
#endif
			for (i = 1; i < (1 << j); i++) {
				ed_add(t[(1 << j) + i], t[i], t[1 << j]);
			}
		}

		ed_norm_sim(t + 2, (const ed_t *)t + 2, RLC_ED_TABLE_COMBS - 2);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(n);
	}
}

void ed_mul_fix_combs(ed_t r, const ed_t * t, const bn_t k) {
	ed_mul_combs_plain(r, t, k);
}
#endif

#if ED_FIX == COMBD || !defined(STRIP)

void ed_mul_pre_combd(ed_t * t, const ed_t p) {
	int i, j, d, e;
	bn_t n;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		ed_curve_get_ord(n);
		d = bn_bits(n);
		d = ((d % ED_DEPTH) == 0 ? (d / ED_DEPTH) : (d / ED_DEPTH) + 1);
		e = (d % 2 == 0 ? (d / 2) : (d / 2) + 1);

		ed_set_infty(t[0]);
		ed_copy(t[1], p);
		for (j = 1; j < ED_DEPTH; j++) {
			ed_dbl(t[1 << j], t[1 << (j - 1)]);
			for (i = 1; i < d; i++) {
				ed_dbl(t[1 << j], t[1 << j]);
			}
#if defined(ED_MIXED)
			ed_norm(t[1 << j], t[1 << j]);
#endif
			for (i = 1; i < (1 << j); i++) {
				ed_add(t[(1 << j) + i], t[i], t[1 << j]);
			}
		}
		ed_set_infty(t[1 << ED_DEPTH]);
		for (j = 1; j < (1 << ED_DEPTH); j++) {
			ed_dbl(t[(1 << ED_DEPTH) + j], t[j]);
			for (i = 1; i < e; i++) {
				ed_dbl(t[(1 << ED_DEPTH) + j], t[(1 << ED_DEPTH) + j]);
			}
		}

		ed_norm_sim(t + 2, (const ed_t *)t + 2, (1 << ED_DEPTH) - 2);
		ed_norm_sim(t + (1 << ED_DEPTH) + 1,
				(const ed_t *)t + (1 << ED_DEPTH) + 1, (1 << ED_DEPTH) - 1);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(n);
	}
}

void ed_mul_fix_combd(ed_t r, const ed_t * t, const bn_t k) {
	int i, j, d, e, w0, w1, n0, p0, p1;
	bn_t n;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		ed_curve_get_ord(n);
		d = bn_bits(n);
		d = ((d % ED_DEPTH) == 0 ? (d / ED_DEPTH) : (d / ED_DEPTH) + 1);
		e = (d % 2 == 0 ? (d / 2) : (d / 2) + 1);

		ed_set_infty(r);
		n0 = bn_bits(k);

		p1 = (e - 1) + (ED_DEPTH - 1) * d;
		for (i = e - 1; i >= 0; i--) {
			ed_dbl(r, r);

			w0 = 0;
			p0 = p1;
			for (j = ED_DEPTH - 1; j >= 0; j--, p0 -= d) {
				w0 = w0 << 1;
				if (p0 < n0 && bn_get_bit(k, p0)) {
					w0 = w0 | 1;
				}
			}

			w1 = 0;
			p0 = p1-- + e;
			for (j = ED_DEPTH - 1; j >= 0; j--, p0 -= d) {
				w1 = w1 << 1;
				if (i + e < d && p0 < n0 && bn_get_bit(k, p0)) {
					w1 = w1 | 1;
				}
			}

			ed_add(r, r, t[w0]);
			ed_add(r, r, t[(1 << ED_DEPTH) + w1]);
		}
		ed_norm(r, r);
		if (bn_sign(k) == RLC_NEG) {
			ed_neg(r, r);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(n);
	}
}

#endif

#if ED_FIX == LWNAF || !defined(STRIP)

void ed_mul_pre_lwnaf(ed_t * t, const ed_t p) {
	ed_tab(t, p, ED_DEPTH);
}

void ed_mul_fix_lwnaf(ed_t r, const ed_t * t, const bn_t k) {
	ed_mul_fix_plain(r, t, k);
}

#endif
