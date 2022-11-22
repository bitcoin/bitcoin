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
static void pp_mil_k24(fp24_t r, ep4_t *t, ep4_t *q, ep_t *p, int m, bn_t a) {
	fp24_t l;
	ep_t *_p = RLC_ALLOCA(ep_t, m);
	ep4_t *_q = RLC_ALLOCA(ep4_t, m);
	int i, j, len = bn_bits(a) + 1;
	int8_t s[RLC_FP_BITS + 1];

	if (m == 0) {
		return;
	}

	fp24_null(l);

	RLC_TRY {
		fp24_new(l);
		if (_p == NULL || _q == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}
		for (j = 0; j < m; j++) {
			ep_null(_p[j]);
			ep4_null(_q[j]);
			ep_new(_p[j]);
			ep4_new(_q[j]);
			ep4_copy(t[j], q[j]);
			ep4_neg(_q[j], q[j]);
#if EP_ADD == BASIC
			ep_neg(_p[j], p[j]);
#else
			fp_add(_p[j]->x, p[j]->x, p[j]->x);
			fp_add(_p[j]->x, _p[j]->x, p[j]->x);
			fp_neg(_p[j]->y, p[j]->y);
#endif
		}

		fp24_zero(l);
		bn_rec_naf(s, &len, a, 2);
		pp_dbl_k24(r, t[0], t[0], _p[0]);
		for (j = 1; j < m; j++) {
			pp_dbl_k24(l, t[j], t[j], _p[j]);
			fp24_mul_dxs(r, r, l);
		}
		if (s[len - 2] > 0) {
			for (j = 0; j < m; j++) {
				pp_add_k24(l, t[j], q[j], p[j]);
				fp24_mul_dxs(r, r, l);
			}
		}
		if (s[len - 2] < 0) {
			for (j = 0; j < m; j++) {
				pp_add_k24(l, t[j], _q[j], p[j]);
				fp24_mul_dxs(r, r, l);
			}
		}

		for (i = len - 3; i >= 0; i--) {
			fp24_sqr(r, r);
			for (j = 0; j < m; j++) {
				pp_dbl_k24(l, t[j], t[j], _p[j]);
				fp24_mul(r, r, l);
				if (s[i] > 0) {
					pp_add_k24(l, t[j], q[j], p[j]);
					fp24_mul_dxs(r, r, l);
				}
				if (s[i] < 0) {
					pp_add_k24(l, t[j], _q[j], p[j]);
					fp24_mul_dxs(r, r, l);
				}
			}
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp24_free(l);
		for (j = 0; j < m; j++) {
			ep_free(_p[j]);
			ep4_free(_q[j]);
		}
		RLC_FREE(_p);
		RLC_FREE(_q);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if PP_MAP == OATEP || !defined(STRIP)

void pp_map_k24(fp24_t r, ep_t p, ep4_t q) {
	ep_t _p[1];
	ep4_t t[1], _q[1];
	bn_t a;

	ep_null(_p[0]);
	ep4_null(_q[0]);
	ep4_null(t[0]);
	bn_null(a);

	RLC_TRY {
		ep_new(_p[0]);
		ep4_new(_q[0]);
		ep4_new(t[0]);
		bn_new(a);

		fp_prime_get_par(a);
		fp24_set_dig(r, 1);

		ep_norm(_p[0], p);
		ep4_norm(_q[0], q);

		if (!ep_is_infty(_p[0]) && !ep4_is_infty(_q[0])) {
			switch (ep_curve_is_pairf()) {
				case EP_B24:
					/* r = f_{|a|,Q}(P). */
					pp_mil_k24(r, t, _q, _p, 1, a);
					if (bn_sign(a) == RLC_NEG) {
						fp24_inv_cyc(r, r);
						ep4_neg(t[0], t[0]);
					}
					pp_exp_k24(r, r);
					break;
			}
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ep_free(_p[0]);
		ep4_free(_q[0]);
		ep4_free(t[0]);
		bn_free(a);
	}
}

void pp_map_sim_k24(fp24_t r, ep_t *p, ep4_t *q, int m) {
	ep_t *_p = RLC_ALLOCA(ep_t, m);
	ep4_t *t = RLC_ALLOCA(ep4_t, m), *_q = RLC_ALLOCA(ep4_t, m);
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
			ep4_null(_q[i]);
			ep4_null(t[i]);
			ep_new(_p[i]);
			ep4_new(_q[i]);
			ep4_new(t[i]);
		}

		j = 0;
		for (i = 0; i < m; i++) {
			if (!ep_is_infty(p[i]) && !ep4_is_infty(q[i])) {
				ep_norm(_p[j], p[i]);
				ep4_norm(_q[j++], q[i]);
			}
		}

		fp_prime_get_par(a);
		fp24_set_dig(r, 1);

		if (j > 0) {
			switch (ep_curve_is_pairf()) {
				case EP_B24:
					/* r = f_{|a|,Q}(P). */
					pp_mil_k24(r, t, _q, _p, j, a);
					if (bn_sign(a) == RLC_NEG) {
						fp24_inv_cyc(r, r);
					}
					pp_exp_k24(r, r);
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
			ep4_free(_q[i]);
			ep4_free(t[i]);
		}
		RLC_FREE(_p);
		RLC_FREE(_q);
		RLC_FREE(t);
	}
}

#endif
