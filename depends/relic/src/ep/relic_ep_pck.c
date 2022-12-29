/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2010 RELIC Authors
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
 * Implementation of point compression on prime elliptic curves.
 *
 * @ingroup ep
 */

#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ep_pck(ep_t r, const ep_t p) {
	int b;
	bn_t halfQ, yValue;

	bn_null(halfQ);
	bn_null(yValue);

	RLC_TRY {
		bn_new(halfQ);
		bn_new(yValue);

		fp_copy(r->x, p->x);

		if (ep_curve_is_pairf()) {
			halfQ->used = RLC_FP_DIGS;
			dv_copy(halfQ->dp, fp_prime_get(), RLC_FP_DIGS);
			bn_hlv(halfQ, halfQ);

			fp_prime_back(yValue, p->y);

			b = bn_cmp(yValue, halfQ) == RLC_GT;
		} else {
			b = fp_get_bit(p->y, 0);
		}

		fp_zero(r->y);
		fp_set_bit(r->y, 0, b);
		fp_set_dig(r->z, 1);

		r->coord = BASIC;
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(yValue);
		bn_free(halfQ);
	}
}

int ep_upk(ep_t r, const ep_t p) {
	fp_t t;
	bn_t halfQ;
	bn_t yValue;
	int result = 0;

	fp_null(t);
	bn_null(halfQ);
	bn_null(yValue);

	RLC_TRY {
		fp_new(t);
		bn_new(halfQ);
		bn_new(yValue);

		ep_rhs(t, p);

		/* t0 = sqrt(x1^3 + a * x1 + b). */
		result = fp_srt(t, t);

		if (result) {
			if (ep_curve_is_pairf()) {
				/* Verify whether the y coordinate is the larger one, matches the
				 * compressed y-coordinate, from IETF pairing friendly spec:
					sign_F_p(y) :=  { 1 if y > (p - 1) / 2, else
									{ 0 otherwise.
				*/
				halfQ->used = RLC_FP_DIGS;
				dv_copy(halfQ->dp, fp_prime_get(), RLC_FP_DIGS);
				bn_hlv(halfQ, halfQ);  // This is equivalent to p - 1 / 2, floor division

				fp_prime_back(yValue, t);
				int sign_fpy = bn_cmp(yValue, halfQ) == RLC_GT;

				if (sign_fpy != fp_get_bit(p->y, 0)) {
					fp_neg(t, t);
				}
			} else {
				/* Verify if least significant bit of the result matches the
				 * compressed y-coordinate. */
				if (fp_get_bit(t, 0) != fp_get_bit(p->y, 0)) {
					fp_neg(t, t);
				}
			}
			fp_copy(r->x, p->x);
			fp_copy(r->y, t);
			fp_set_dig(r->z, 1);
			r->coord = BASIC;
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp_free(t);
		bn_free(yValue);
		bn_free(halfQ);
	}
	return result;
}
