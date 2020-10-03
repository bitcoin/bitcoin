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
 * Implementation of the pairings over prime curves.
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
 * given parameter.
 *
 * @param[out] r			- the result.
 * @param[out] t			- the resulting point.
 * @param[in] p				- the first pairing argument in affine coordinates.
 * @param[in] q				- the second pairing argument in affine coordinates.
 * @param[in] m 			- the number of pairings to evaluate.
 * @param[in] a				- the loop parameter.
 */
static void pp_mil_k2(fp2_t r, ep_t *t, ep_t *p, ep_t *q, int m, bn_t a) {
	fp2_t l;
	ep_t *_q;
	int i, j;

	fp2_null(l);

	_q = malloc(sizeof(ep_t) * m);

	TRY {
		fp2_new(l);
		for (j = 0; j < m; j++) {
			ep_null(_q[j]);
			ep_new(_q[j]);
			ep_copy(t[j], p[j]);
			ep_neg(_q[j], q[j]);
		}

		fp2_zero(l);

		for (i = bn_bits(a) - 2; i >= 0; i--) {
			fp2_sqr(r, r);
			for (j = 0; j < m; j++) {
				pp_dbl_k2(l, t[j], t[j], _q[j]);
				fp2_mul(r, r, l);
				if (bn_get_bit(a, i)) {
					pp_add_k2(l, t[j], p[j], q[j]);
					fp2_mul(r, r, l);
				}
			}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp2_free(l);
		for (j = 0; j < m; j++) {
			ep_free(_q[j]);
		}
		free(_q);
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
 * @param[in] a				- the loop parameter.
 */
static void pp_mil_lit_k2(fp2_t r, ep_t *t, ep_t *p, ep_t *q, int m, bn_t a) {
	fp2_t l, _l;
	ep_t *_q;
	int i, j;

	fp2_null(l);
	fp2_null(_l);

	_q = malloc(sizeof(ep_t) * m);

	TRY {
		fp2_new(l);
		fp2_new(_l);
		for (j = 0; j < m; j++) {
			ep_null(_q[j]);
			ep_new(_q[j]);
			ep_copy(t[j], p[j]);
			ep_neg(_q[j], q[j]);
		}

		for (i = bn_bits(a) - 2; i >= 0; i--) {
			fp2_sqr(r, r);
			for (j = 0; j < m; j++) {
				pp_dbl_k2(l, t[j], t[j], _q[j]);
				fp_copy(_l[0], l[1]);
				fp_copy(_l[1], l[0]);
				fp2_mul(r, r, _l);
				if (bn_get_bit(a, i)) {
					pp_add_k2(l, t[j], p[j], q[j]);
					fp_copy(_l[0], l[1]);
					fp_copy(_l[1], l[0]);
					fp2_mul(r, r, _l);
				}
			}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp2_free(l);
		fp2_free(_l);
		for (j = 0; j < m; j++) {
			ep_null(_q[j]);
		}
		free(_q);
	}
}

/**
 * Compute the Miller loop for pairings of type G_2 x G_1 over the bits of a
 * given parameter.
 *
 * @param[out] r			- the result.
 * @param[out] t			- the resulting point.
 * @param[in] q				- the first pairing argument in affine coordinates.
 * @param[in] p				- the second pairing argument in affine coordinates.
 * @param[in] n 			- the number of pairings to evaluate.
 * @param[in] a				- the loop parameter.
 */
static void pp_mil_k12(fp12_t r, ep2_t *t, ep2_t *q, ep_t *p, int m, bn_t a) {
	fp12_t l;
	ep_t *_p;
	int i, j;

	if (m == 0) {
		return;
	}

	fp12_null(l);

	_p = malloc(sizeof(ep_t) * m);

	TRY {
		fp12_new(l);

		for (j = 0; j < m; j++) {
			ep_null(_p[j]);
			ep_new(_p[j]);
#if EP_ADD == BASIC
			ep_neg(_p[j], p[i]);
#else
			fp_add(_p[j]->x, p[j]->x, p[j]->x);
			fp_add(_p[j]->x, _p[j]->x, p[j]->x);
			fp_neg(_p[j]->y, p[j]->y);
#endif
			ep2_copy(t[j], q[j]);
		}

		fp12_zero(l);

		/* Precomputing. */
		pp_dbl_k12(r, t[0], t[0], _p[0]);
		if (bn_get_bit(a, bn_bits(a) - 2)) {
			for (j = 0; j < m; j++) {
				pp_add_k12(l, t[j], q[j], p[j]);
				fp12_mul_dxs(r, r, l);
			}
		}

		for (i = bn_bits(a) - 3; i >= 0; i--) {
			fp12_sqr(r, r);
			for (j = 0; j < m; j++) {
				pp_dbl_k12(l, t[j], t[j], _p[j]);
				fp12_mul_dxs(r, r, l);
				if (bn_get_bit(a, i)) {
					pp_add_k12(l, t[j], q[j], p[j]);
					fp12_mul_dxs(r, r, l);
				}
			}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp12_free(l);
		for (j = 0; j < m; j++) {
			ep_free(_p[j]);
		}
		free(_p);
	}
}

/**
 * Compute the Miller loop for pairings of type G_2 x G_1 over the bits of a
 * given parameter represented in sparse form.
 *
 * @param[out] r			- the result.
 * @param[out] t			- the resulting point.
 * @param[in] q				- the vector of first arguments in affine coordinates.
 * @param[in] p				- the vector of second arguments in affine coordinates.
 * @param[in] n 			- the number of pairings to evaluate.
 * @param[in] s				- the loop parameter in sparse form.
 * @paramin] len			- the length of the loop parameter.
 */
static void pp_mil_sps_k12(fp12_t r, ep2_t *t, ep2_t *q, ep_t *p, int m, int *s,
		int len) {
	fp12_t l;
	ep_t *_p;
	ep2_t *_q;
	int i, j;

	if (m == 0) {
		return;
	}

	fp12_null(l);

	_p = malloc(sizeof(ep_t) * m);
	_q = malloc(sizeof(ep2_t) * m);

	TRY {
		fp12_new(l);
		fp12_zero(l);

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
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp12_free(l);
		for (j = 0; j < m; j++) {
			ep_free(_p[j]);
			ep2_free(_q[j]);
		}
		free(_p);
		free(_q);
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
	ep2_t *_q;
	int j;

	fp12_null(l);

	_q = malloc(sizeof(ep2_t) * m);

	TRY {
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
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp12_free(l);
		for (j = 0; j < m; j++) {
			ep2_free(_q[j]);
		}
		free(_q);
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

	TRY {
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
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp12_free(tmp);
		ep2_free(q1);
		ep2_free(q2);
	}
}


/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void pp_map_init(void) {
	ep2_curve_init();
}

void pp_map_clean(void) {
	ep2_curve_clean();
}

#if PP_MAP == TATEP || PP_MAP == OATEP || !defined(STRIP)

void pp_map_tatep_k2(fp2_t r, ep_t p, ep_t q) {
	ep_t _p[1], _q[1], t[1];
	bn_t n;

	ep_null(_p[0]);
	ep_null(_q[0]);
	ep_null(t[0]);
	bn_null(n);

	TRY {
		ep_new(t[0]);
		bn_new(n);

		ep_norm(_p[0], p);
		ep_norm(_q[0], q);
		ep_curve_get_ord(n);
		/* Since p has order n, we do not have to perform last iteration. */
		bn_sub_dig(n, n, 1);
		fp2_set_dig(r, 1);

		if (!ep_is_infty(p) && !ep_is_infty(q)) {
			pp_mil_k2(r, t, _p, _q, 1, n);
			pp_exp_k2(r, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep_free(_p[0]);
		ep_free(_q[0]);
		ep_free(t[0]);
		bn_free(n);
	}
}

void pp_map_sim_tatep_k2(fp2_t r, ep_t *p, ep_t *q, int m) {
	ep_t *_p, *_q, *t;
	bn_t n;
	int i, j;

	bn_null(n);

	_p = malloc(sizeof(ep_t) * m);
	_q = malloc(sizeof(ep_t) * m);
	t = malloc(sizeof(ep_t) * m);

	TRY {
		bn_new(n);
		for (i = 0; i < m; i++) {
			ep_null(_p[i]);
			ep_null(_q[i]);
			ep_null(t[i]);
			ep_new(_p[i]);
			ep_new(_q[i]);
			ep_new(t[i]);
		}

		j = 0;
		for (i = 0; i < m; i++) {
			if (!ep_is_infty(p[i]) && !ep_is_infty(q[i])) {
				ep_norm(_p[j], p[i]);
				ep_norm(_q[j++], q[i]);
			}
		}

		ep_curve_get_ord(n);
		bn_sub_dig(n, n, 1);
		fp2_set_dig(r, 1);
		if (j > 0) {
			pp_mil_k2(r, t, _p, _q, j, n);
			pp_exp_k2(r, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(n);
		for (i = 0; i < m; i++) {
			ep_free(_p[i]);
			ep_free(_q[i]);
			ep_free(t[i]);
		}
		free(_p);
		free(_q);
		free(t);
	}
}

#endif

#if PP_MAP == TATEP || !defined(STRIP)

void pp_map_tatep_k12(fp12_t r, ep_t p, ep2_t q) {
	ep_t _p[1], t[1];
	ep2_t _q[1];
	bn_t n;

	ep_null(_p[0]);
	ep_null(t[0]);
	ep2_null(_q[0]);
	bn_null(n);

	TRY {
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
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep_free(_p[0]);
		ep_free(t[0]);
		ep2_free(_q[0]);
		bn_free(n);
	}
}

void pp_map_sim_tatep_k12(fp12_t r, ep_t *p, ep2_t *q, int m) {
	ep_t *_p, *t;
	ep2_t *_q;
	bn_t n;
	int i, j;

	bn_null(n);

	_p = malloc(sizeof(ep_t) * m);
	_q = malloc(sizeof(ep2_t) * m);
	t = malloc(sizeof(ep_t) * m);

	TRY {
		bn_new(n);
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
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(n);
		for (i = 0; i < m; i++) {
			ep_free(_p[i]);
			ep_free(t[i]);
			ep2_free(_q[i]);
		}
		free(_p);
		free(_q);
		free(t);
	}
}

#endif

#if PP_MAP == WEILP || !defined(STRIP)

void pp_map_weilp_k2(fp2_t r, ep_t p, ep_t q) {
	ep_t _p[1], _q[1], t0[1], t1[1];
	fp2_t r0, r1;
	bn_t n;

	ep_null(_p[0]);
	ep_null(_q[0]);
	ep_null(t0[0]);
	ep_null(t1[0]);
	fp2_null(r0);
	fp2_null(r1);
	bn_null(n);

	TRY {
		ep_new(_p[0]);
		ep_new(_q[0]);
		ep_new(t0[0]);
		ep_new(t1[0]);
		fp2_new(r0);
		fp2_new(r1);
		bn_new(n);

		ep_norm(_p[0], p);
		ep_norm(_q[0], q);
		ep_curve_get_ord(n);
		/* Since p has order n, we do not have to perform last iteration. */
		bn_sub_dig(n, n, 1);
		fp2_set_dig(r0, 1);
		fp2_set_dig(r1, 1);

		if (!ep_is_infty(_p[0]) && !ep_is_infty(_q[0])) {
			pp_mil_lit_k2(r0, t0, _p, _q, 1, n);
			pp_mil_k2(r1, t1, _q, _p, 1, n);
			fp2_inv(r1, r1);
			fp2_mul(r0, r0, r1);
			fp2_inv(r1, r0);
			fp2_inv_uni(r0, r0);
		}
		fp2_mul(r, r0, r1);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep_free(_p[0]);
		ep_free(_q[0]);
		ep_free(t0[0]);
		ep_free(t1[0]);
		fp2_free(r0);
		fp2_free(r1);
		bn_free(n);
	}
}

void pp_map_sim_weilp_k2(fp2_t r, ep_t *p, ep_t *q, int m) {
	ep_t *_p, *_q, *t0, *t1;
	fp2_t r0, r1;
	bn_t n;
	int i, j;

	fp2_null(r0);
	fp2_null(r1);
	bn_null(r);

	_p = malloc(sizeof(ep_t) * m);
	_q = malloc(sizeof(ep_t) * m);
	t0 = malloc(sizeof(ep_t) * m);
	t1 = malloc(sizeof(ep_t) * m);

	TRY {
		fp2_new(r0);
		fp2_new(r1);
		bn_new(n);
		for (i = 0; i < m; i++) {
			ep_null(_p[i]);
			ep_null(_q[i]);
			ep_null(t0[i]);
			ep_null(t1[i]);
			ep_new(_p[i]);
			ep_new(_q[i]);
			ep_new(t0[i]);
			ep_new(t1[i]);
		}

		j = 0;
		for (i = 0; i < m; i++) {
			if (!ep_is_infty(p[i]) && !ep_is_infty(q[i])) {
				ep_norm(_p[j], p[i]);
				ep_norm(_q[j++], q[i]);
			}
		}

		ep_curve_get_ord(n);
		bn_sub_dig(n, n, 1);
		fp2_set_dig(r0, 1);
		fp2_set_dig(r1, 1);

		if (j > 0) {
			pp_mil_lit_k2(r0, t0, _p, _q, j, n);
			pp_mil_k2(r1, t1, _q, _p, j, n);
			fp2_inv(r1, r1);
			fp2_mul(r0, r0, r1);
			fp2_inv(r1, r0);
			fp2_inv_uni(r0, r0);
		}
		fp2_mul(r, r0, r1);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp2_free(r0);
		fp2_free(r1);
		bn_free(n);
		for (i = 0; i < m; i++) {
			ep_free(_p[i]);
			ep_free(_q[i]);
			ep_free(t0[i]);
			ep_free(t1[i]);
		}
		free(_p);
		free(_q);
		free(t0);
		free(t1);
	}
}

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

	TRY {
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
			fp12_inv_uni(r0, r0);
		}
		fp12_mul(r, r0, r1);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
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
	ep_t *_p, *t0;
	ep2_t *_q, *t1;
	fp12_t r0, r1;
	bn_t n;
	int i, j;

	fp12_null(r0);
	fp12_null(r1);
	bn_null(r);

	_p = malloc(sizeof(ep_t) * m);
	_q = malloc(sizeof(ep2_t) * m);
	t0 = malloc(sizeof(ep_t) * m);
	t1 = malloc(sizeof(ep2_t) * m);

	TRY {
		fp12_new(r0);
		fp12_new(r1);
		bn_new(n);
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
			fp12_inv_uni(r0, r0);
		}
		fp12_mul(r, r0, r1);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp12_free(r0);
		fp12_free(r1);
		bn_free(n);
		for (i = 0; i < m; i++) {
			ep_free(_p[i]);
			ep_free(t0[i]);
			ep2_free(_q[i]);
			ep2_free(t1[i]);
		}
		free(_p);
		free(_q);
		free(t0);
		free(t1);
	}
}

#endif


#if PP_MAP == OATEP || !defined(STRIP)

void pp_map_oatep_k12(fp12_t r, ep_t p, ep2_t q) {
	ep_t _p[1];
	ep2_t t[1], _q[1];
	bn_t a;
	int len = FP_BITS, s[FP_BITS];

	ep_null(_p[0]);
	ep2_null(_q[0]);
	ep2_null(t[0]);
	bn_null(a);

	TRY {
		ep_new(_p[0]);
		ep2_new(_q[0]);
		ep2_new(t[0]);
		bn_new(a);

		fp_param_get_var(a);
		bn_mul_dig(a, a, 6);
		bn_add_dig(a, a, 2);
		fp_param_get_map(s, &len);
		fp12_set_dig(r, 1);

		ep_norm(_p[0], p);
		ep2_norm(_q[0], q);

		if (!ep_is_infty(_p[0]) && !ep2_is_infty(_q[0])) {
			switch (ep_param_get()) {
				case BN_P158:
				case BN_P254:
				case BN_P256:
				case BN_P382:
				case BN_P638:
					/* r = f_{|a|,Q}(P). */
					pp_mil_sps_k12(r, t, _q, _p, 1, s, len);
					if (bn_sign(a) == BN_NEG) {
						/* f_{-a,Q}(P) = 1/f_{a,Q}(P). */
						fp12_inv_uni(r, r);
						ep2_neg(t[0], t[0]);
					}
					pp_fin_k12_oatep(r, t[0], _q[0], _p[0]);
					pp_exp_k12(r, r);
					break;
				case B12_P381:
				case B12_P455:
				case B12_P638:
					/* r = f_{|a|,Q}(P). */
					pp_mil_sps_k12(r, t, _q, _p, 1, s, len);
					if (bn_sign(a) == BN_NEG) {
						fp12_inv_uni(r, r);
						ep2_neg(t[0], t[0]);
					}
					pp_exp_k12(r, r);
					break;
			}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep_free(_p[0]);
		ep2_free(_q[0]);
		ep2_free(t[0]);
		bn_free(a);
	}
}

void pp_map_sim_oatep_k12(fp12_t r, ep_t *p, ep2_t *q, int m) {
	ep_t *_p;
	ep2_t *t, *_q;
	bn_t a;
	int i, j, len = FP_BITS, s[FP_BITS];

	_p = malloc(sizeof(ep_t) * m);
	_q = malloc(sizeof(ep2_t) * m);
	t = malloc(sizeof(ep2_t) * m);

	TRY {
		bn_null(a);
		bn_new(a);
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

		fp12_set_dig(r, 1);
		fp_param_get_var(a);
		bn_mul_dig(a, a, 6);
		bn_add_dig(a, a, 2);
		fp_param_get_map(s, &len);

		if (j > 0) {
			switch (ep_param_get()) {
				case BN_P158:
				case BN_P254:
				case BN_P256:
				case BN_P382:
				case BN_P638:
					/* r = f_{|a|,Q}(P). */
					pp_mil_sps_k12(r, t, _q, _p, j, s, len);
					if (bn_sign(a) == BN_NEG) {
						/* f_{-a,Q}(P) = 1/f_{a,Q}(P). */
						fp12_inv_uni(r, r);
					}
					for (i = 0; i < j; i++) {
						if (bn_sign(a) == BN_NEG) {
							ep2_neg(t[i], t[i]);
						}
						pp_fin_k12_oatep(r, t[i], _q[i], _p[i]);
					}
					pp_exp_k12(r, r);
					break;
				case B12_P381:
				case B12_P455:
				case B12_P638:
					/* r = f_{|a|,Q}(P). */
					pp_mil_sps_k12(r, t, _q, _p, j, s, len);
					if (bn_sign(a) == BN_NEG) {
						fp12_inv_uni(r, r);
					}
					pp_exp_k12(r, r);
					break;
			}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(a);
		for (i = 0; i < m; i++) {
			ep_free(_p[i]);
			ep2_free(_q[i]);
			ep2_free(t[i]);
		}
		free(_p);
		free(_q);
		free(t);
	}
}

#endif
