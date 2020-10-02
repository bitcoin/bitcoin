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
 * Implementation of point compression on prime elliptic curves over quadratic
 * extensions.
 *
 * @ingroup ep
 */

#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ep2_pck(ep2_t r, ep2_t p) {
	bn_t halfQ;
	bn_null(halfQ);
	bn_new(halfQ);
	halfQ->used = FP_DIGS;
	dv_copy(halfQ->dp, fp_prime_get(), FP_DIGS);
	bn_hlv(halfQ, halfQ);

	bn_t yValue;
	bn_null(yValue);
	bn_new(yValue);
	fp_prime_back(yValue, p->y[1]);

	int b = bn_cmp(yValue, halfQ) == CMP_GT;

	fp2_copy(r->x, p->x);
	fp2_zero(r->y);
	fp_set_bit(r->y[0], 0, b);
	fp_zero(r->y[1]);
	fp_set_dig(r->z[0], 1);
	fp_zero(r->z[1]);
	r->norm = 1;

	bn_free(yValue);
	bn_free(halfQ);
}

int ep2_upk(ep2_t r, ep2_t p) {
	fp2_t t;
	bn_t halfQ;
	bn_t yValue;
	int result = 0;

	fp2_null(t);
	bn_null(halfQ);
	bn_null(yValue);

	TRY {
		fp2_new(t);

		ep2_rhs(t, p);

		/* t0 = sqrt(x1^3 + a * x1 + b). */
		result = fp2_srt(t, t);

		if (result) {
			/* Verify whether the y coordinate is the larger one, matches the
			 * compressed y-coordinate. */
			bn_new(halfQ);
			halfQ->used = FP_DIGS;
			dv_copy(halfQ->dp, fp_prime_get(), FP_DIGS);
			bn_hlv(halfQ, halfQ);

			bn_new(yValue);
			fp_prime_back(yValue, t[1]);

			int b = bn_cmp(yValue, halfQ) == CMP_GT;

			if (b != fp_get_bit(p->y[0], 0)) {
				fp2_neg(t, t);
			}
			fp2_copy(r->x, p->x);
			fp2_copy(r->y, t);
			fp_set_dig(r->z[0], 1);
			fp_zero(r->z[1]);
			r->norm = 1;
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp2_free(t);
		bn_free(yValue);
		bn_free(halfQ);
	}
	return result;
}
