/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2019 RELIC Authors
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
 * Implementation of pairing computation for curves with embedding degree 12.
 *
 * @ingroup pp
 */

#include "relic_core.h"
#include "relic_pp.h"
#include "relic_util.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Compute the Miller loop for pairings of type G_2 x G_1 over the bits of a
 * given parameter represented in sparse form.
 *
 * @param[out] r			- the result.
 * @param[out] t			- the resulting point.
 * @param[in] q				- the vector of first arguments in affine coordinates.
 * @param[in] p				- the vector of second arguments in affine coordinates.
 * @param[in] n 			- the number of pairings to evaluate.
 * @param[in] a				- the loop parameter.
 */
static void pp_mil_k12(fp12_t r, ep2_t *t, ep2_t *q, ep_t *p, int m, bn_t a) {
	fp12_t l;
	ep_t *_p = RLC_ALLOCA(ep_t, m);
	ep2_t *_q = RLC_ALLOCA(ep2_t, m);
	int i, j, len = bn_bits(a) + 1;
	int8_t s[RLC_FP_BITS + 1];

	if (m == 0) {
		return;
	}

	fp12_null(l);

	RLC_TRY {
		fp12_new(l);
		if (_p == NULL || _q == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}
		for (j = 0; j < m; j++) {
			ep_null(_p[j]);
			ep2_null(_q[j]);
			ep_new(_p[j]);
			ep2_new(_q[j]);
			ep2_copy(t[j], q[j]);
			ep2_neg(_q[j], q[j]);
#if EP_ADD == BASIC
			ep_neg(_p[j], p[j]);
#else
			fp_add(_p[j]->x, p[j]->x, p[j]->x);
			fp_add(_p[j]->x, _p[j]->x, p[j]->x);
			fp_neg(_p[j]->y, p[j]->y);
#endif
		}

		fp12_zero(l);
		bn_rec_naf(s, &len, a, 2);
		pp_dbl_k12(r, t[0], t[0], _p[0]);
		for (j = 1; j < m; j++) {
			pp_dbl_k12(l, t[j], t[j], _p[j]);
			fp12_mul_dxs(r, r, l);
		}
		if (s[len - 2] > 0) {
			for (j = 0; j < m; j++) {
				pp_add_k12(l, t[j], q[j], p[j]);
				fp12_mul_dxs(r, r, l);
			}
		}
		if (s[len - 2] < 0) {
			for (j = 0; j < m; j++) {
				pp_add_k12(l, t[j], _q[j], p[j]);
				fp12_mul_dxs(r, r, l);
			}
		}

		for (i = len - 3; i >= 0; i--) {
			fp12_sqr(r, r);
			for (j = 0; j < m; j++) {
				pp_dbl_k12(l, t[j], t[j], _p[j]);
				fp12_mul_dxs(r, r, l);
				if (s[i] > 0) {
					pp_add_k12(l, t[j], q[j], p[j]);
					fp12_mul_dxs(r, r, l);
				}
				if (s[i] < 0) {
					pp_add_k12(l, t[j], _q[j], p[j]);
					fp12_mul_dxs(r, r, l);
				}
			}
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp12_free(l);
		for (j = 0; j < m; j++) {
			ep_free(_p[j]);
			ep2_free(_q[j]);
		}
		RLC_FREE(_p);
		RLC_FREE(_q);
	}
}

/**
 * Compute the Miller loop for pairings of type G_1 x G_2 over the bits of a
 * given parameter.
 *
 * @param[out] r			- the result.
 * @param[out] t			- the resulting point.
 * @param[in] p				- the first pairing argument in affine coordinates.
 * @param[in] q				- the second pairing argument in affine coordinates.
 * @param[in] n 			- the number of pairings to evaluate.
 * @param[in] a				- the loop parameter.
 */
static void pp_mil_lit_k12(fp12_t r, ep_t *t, ep_t *p, ep2_t *q, int m, bn_t a) {
	fp12_t l;
	ep2_t *_q = RLC_ALLOCA(ep2_t, m);
	int j;

	fp12_null(l);

	RLC_TRY {
		if (_q == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}
		fp12_new(l);
		for (j = 0; j < m; j++) {
			ep2_null(_q[j]);
			ep2_new(_q[j]);
			ep_copy(t[j], p[j]);
			ep2_neg(_q[j], q[j]);
		}

		fp12_zero(l);
		for (int i = bn_bits(a) - 2; i >= 0; i--) {
			fp12_sqr(r, r);
			for (j = 0; j < m; j++) {
				pp_dbl_lit_k12(l, t[j], t[j], _q[j]);
				fp12_mul(r, r, l);
				if (bn_get_bit(a, i)) {
					pp_add_lit_k12(l, t[j], p[j], q[j]);
					fp12_mul(r, r, l);
				}
			}
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp12_free(l);
		for (j = 0; j < m; j++) {
			ep2_free(_q[j]);
		}
		RLC_FREE(_q);
	}
}

/**
 * Compute the final lines for optimal ate pairings.
 *
 * @param[out] r			- the result.
 * @param[out] t			- the resulting point.
 * @param[in] q				- the first point of the pairing, in G_2.
 * @param[in] p				- the second point of the pairing, in G_1.
 * @param[in] a				- the loop parameter.
 */
static void pp_fin_k12_oatep(fp12_t r, ep2_t t, ep2_t q, ep_t p) {
	ep2_t q1, q2;
	fp12_t tmp;

	fp12_null(tmp);
	ep2_null(q1);
	ep2_null(q2);

	RLC_TRY {
		ep2_new(q1);
		ep2_new(q2);
		fp12_new(tmp);
		fp12_zero(tmp);

		fp2_set_dig(q1->z, 1);
		fp2_set_dig(q2->z, 1);

		ep2_frb(q1, q, 1);
		ep2_frb(q2, q, 2);
		ep2_neg(q2, q2);

		pp_add_k12(tmp, t, q1, p);
		fp12_mul_dxs(r, r, tmp);
		pp_add_k12(tmp, t, q2, p);
		fp12_mul_dxs(r, r, tmp);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp12_free(tmp);
		ep2_free(q1);
		ep2_free(q2);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if PP_MAP == TATEP || !defined(STRIP)

void pp_map_tatep_k12(fp12_t r, ep_t p, ep2_t q) {
	ep_t _p[1], t[1];
	ep2_t _q[1];
	bn_t n;

	ep_null(_p[0]);
	ep_null(t[0]);
	ep2_null(_q[0]);
	bn_null(n);

	RLC_TRY {
		ep_new(_p[0]);
		ep_new(t[0]);
		ep2_new(_q[0]);
		bn_new(n);

		ep_norm(_p[0], p);
		ep2_norm(_q[0], q);
		ep_curve_get_ord(n);
		fp12_set_dig(r, 1);

		if (!ep_is_infty(p) && !ep2_is_infty(q)) {
			pp_mil_lit_k12(r, t, _p, _q, 1, n);
			pp_exp_k12(r, r);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ep_free(_p[0]);
		ep_free(t[0]);
		ep2_free(_q[0]);
		bn_free(n);
	}
}

void pp_map_sim_tatep_k12(fp12_t r, ep_t *p, ep2_t *q, int m) {
	ep_t *_p = RLC_ALLOCA(ep_t, m), *t = RLC_ALLOCA(ep_t, m);
	ep2_t *_q = RLC_ALLOCA(ep2_t, m);
	bn_t n;
	int i, j;

	bn_null(n);

	RLC_TRY {
		bn_new(n);
		if (_p == NULL || _q == NULL || t == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}
		for (i = 0; i < m; i++) {
			ep_null(_p[i]);
			ep_null(t[i]);
			ep2_null(_q[i]);
			ep_new(_p[i]);
			ep_new(t[i]);
			ep2_new(_q[i]);
		}

		j = 0;
		for (i = 0; i < m; i++) {
			if (!ep_is_infty(p[i]) && !ep2_is_infty(q[i])) {
				ep_norm(_p[j], p[i]);
				ep2_norm(_q[j++], q[i]);
			}
		}

		ep_curve_get_ord(n);
		fp12_set_dig(r, 1);
		if (j > 0) {
			pp_mil_lit_k12(r, t, _p, _q, j, n);
			pp_exp_k12(r, r);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(n);
		for (i = 0; i < m; i++) {
			ep_free(_p[i]);
			ep_free(t[i]);
			ep2_free(_q[i]);
		}
		RLC_FREE(_p);
		RLC_FREE(t);
		RLC_FREE(_q);
	}
}

#endif

#if PP_MAP == WEILP || !defined(STRIP)

void pp_map_weilp_k12(fp12_t r, ep_t p, ep2_t q) {
	ep_t _p[1], t0[1];
	ep2_t _q[1], t1[1];
	fp12_t r0, r1;
	bn_t n;

	ep_null(_p[0]);
	ep_null(t0[0]);
	ep2_null(_q[0]);
	ep2_null(t1[0]);
	fp12_null(r0);
	fp12_null(r1);
	bn_null(n);

	RLC_TRY {
		ep_new(_p[0]);
		ep_new(t0[0]);
		ep2_new(_q[0]);
		ep2_new(t1[0]);
		fp12_new(r0);
		fp12_new(r1);
		bn_new(n);

		ep_norm(_p[0], p);
		ep2_norm(_q[0], q);
		ep_curve_get_ord(n);
		bn_sub_dig(n, n, 1);
		fp12_set_dig(r0, 1);
		fp12_set_dig(r1, 1);

		if (!ep_is_infty(_p[0]) && !ep2_is_infty(_q[0])) {
			pp_mil_lit_k12(r0, t0, _p, _q, 1, n);
			pp_mil_k12(r1, t1, _q, _p, 1, n);
			fp12_inv(r1, r1);
			fp12_mul(r0, r0, r1);
			fp12_inv(r1, r0);
			fp12_inv_cyc(r0, r0);
		}
		fp12_mul(r, r0, r1);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ep_free(_p[0]);
		ep_free(t0[0]);
		ep2_free(_q[0]);
		ep2_free(t1[0]);
		fp12_free(r0);
		fp12_free(r1);
		bn_free(n);
	}
}

void pp_map_sim_weilp_k12(fp12_t r, ep_t *p, ep2_t *q, int m) {
	ep_t *_p = RLC_ALLOCA(ep_t, m), *t0 = RLC_ALLOCA(ep_t, m);
	ep2_t *_q = RLC_ALLOCA(ep2_t, m), *t1 = RLC_ALLOCA(ep2_t, m);
	fp12_t r0, r1;
	bn_t n;
	int i, j;

	fp12_null(r0);
	fp12_null(r1);
	bn_null(n);

	RLC_TRY {
		fp12_new(r0);
		fp12_new(r1);
		bn_new(n);
		if (_p == NULL || _q == NULL || t0 == NULL || t1 == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}
		for (i = 0; i < m; i++) {
			ep_null(_p[i]);
			ep_null(t0[i]);
			ep2_null(_q[i]);
			ep2_null(t1[i]);
			ep_new(_p[i]);
			ep_new(t0[i]);
			ep2_new(_q[i]);
			ep2_new(t1[i]);
		}

		j = 0;
		for (i = 0; i < m; i++) {
			if (!ep_is_infty(p[i]) && !ep2_is_infty(q[i])) {
				ep_norm(_p[j], p[i]);
				ep2_norm(_q[j++], q[i]);
			}
		}

		ep_curve_get_ord(n);
		bn_sub_dig(n, n, 1);
		fp12_set_dig(r0, 1);
		fp12_set_dig(r1, 1);

		if (j > 0) {
			pp_mil_lit_k12(r0, t0, _p, _q, j, n);
			pp_mil_k12(r1, t1, _q, _p, j, n);
			fp12_inv(r1, r1);
			fp12_mul(r0, r0, r1);
			fp12_inv(r1, r0);
			fp12_inv_cyc(r0, r0);
		}
		fp12_mul(r, r0, r1);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp12_free(r0);
		fp12_free(r1);
		bn_free(n);
		for (i = 0; i < m; i++) {
			ep_free(_p[i]);
			ep_free(t0[i]);
			ep2_free(_q[i]);
			ep2_free(t1[i]);
		}
		RLC_FREE(_p);
		RLC_FREE(_q);
		RLC_FREE(t0);
		RLC_FREE(t1);
	}
}

#endif

#if PP_MAP == OATEP || !defined(STRIP)

void pp_map_oatep_k12(fp12_t r, ep_t p, ep2_t q) {
	ep_t _p[1];
	ep2_t t[1], _q[1];
	bn_t a;

	ep_null(_p[0]);
	ep2_null(_q[0]);
	ep2_null(t[0]);
	bn_null(a);

	RLC_TRY {
		ep_new(_p[0]);
		ep2_new(_q[0]);
		ep2_new(t[0]);
		bn_new(a);

		fp_prime_get_par(a);
		fp12_set_dig(r, 1);

		ep_norm(_p[0], p);
		ep2_norm(_q[0], q);

		if (!ep_is_infty(_p[0]) && !ep2_is_infty(_q[0])) {
			switch (ep_curve_is_pairf()) {
				case EP_BN:
					bn_mul_dig(a, a, 6);
					bn_add_dig(a, a, 2);
					/* r = f_{|a|,Q}(P). */
					pp_mil_k12(r, t, _q, _p, 1, a);
					if (bn_sign(a) == RLC_NEG) {
						/* f_{-a,Q}(P) = 1/f_{a,Q}(P). */
						fp12_inv_cyc(r, r);
						ep2_neg(t[0], t[0]);
					}
					pp_fin_k12_oatep(r, t[0], _q[0], _p[0]);
					pp_exp_k12(r, r);
					break;
				case EP_B12:
					/* r = f_{|a|,Q}(P). */
					pp_mil_k12(r, t, _q, _p, 1, a);
					if (bn_sign(a) == RLC_NEG) {
						fp12_inv_cyc(r, r);
						ep2_neg(t[0], t[0]);
					}
					pp_exp_k12(r, r);
					break;
			}
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ep_free(_p[0]);
		ep2_free(_q[0]);
		ep2_free(t[0]);
		bn_free(a);
	}
}

void pp_map_sim_oatep_k12(fp12_t r, ep_t *p, ep2_t *q, int m) {
	ep_t *_p = RLC_ALLOCA(ep_t, m);
	ep2_t *t = RLC_ALLOCA(ep2_t, m), *_q = RLC_ALLOCA(ep2_t, m);
	bn_t a;
	int i, j;

	RLC_TRY {
		bn_null(a);
		bn_new(a);
		if (_p == NULL || _q == NULL || t == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}
		for (i = 0; i < m; i++) {
			ep_null(_p[i]);
			ep2_null(_q[i]);
			ep2_null(t[i]);
			ep_new(_p[i]);
			ep2_new(_q[i]);
			ep2_new(t[i]);
		}

		j = 0;
		for (i = 0; i < m; i++) {
			if (!ep_is_infty(p[i]) && !ep2_is_infty(q[i])) {
				ep_norm(_p[j], p[i]);
				ep2_norm(_q[j++], q[i]);
			}
		}

		fp_prime_get_par(a);
		fp12_set_dig(r, 1);

		if (j > 0) {
			switch (ep_curve_is_pairf()) {
				case EP_BN:
					bn_mul_dig(a, a, 6);
					bn_add_dig(a, a, 2);
					/* r = f_{|a|,Q}(P). */
					pp_mil_k12(r, t, _q, _p, j, a);
					if (bn_sign(a) == RLC_NEG) {
						/* f_{-a,Q}(P) = 1/f_{a,Q}(P). */
						fp12_inv_cyc(r, r);
					}
					for (i = 0; i < j; i++) {
						if (bn_sign(a) == RLC_NEG) {
							ep2_neg(t[i], t[i]);
						}
						pp_fin_k12_oatep(r, t[i], _q[i], _p[i]);
					}
					pp_exp_k12(r, r);
					break;
				case EP_B12:
					/* r = f_{|a|,Q}(P). */
					pp_mil_k12(r, t, _q, _p, j, a);
					if (bn_sign(a) == RLC_NEG) {
						fp12_inv_cyc(r, r);
					}
					pp_exp_k12(r, r);
					break;
			}
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(a);
		for (i = 0; i < m; i++) {
			ep_free(_p[i]);
			ep2_free(_q[i]);
			ep2_free(t[i]);
		}
		RLC_FREE(_p);
		RLC_FREE(_q);
		RLC_FREE(t);
	}
}

#endif
