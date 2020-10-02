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
 * Implementation of square root in extensions defined over prime fields.
 *
 * @ingroup fpx
 */

#include "relic_core.h"
#include "relic_fpx_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int fp2_srt(fp2_t c, fp2_t a) {
	int r = 0;
	fp_t t1;
	fp_t t2;
	fp_t t3;

	fp_null(t1);
	fp_null(t2);
	fp_null(t3);

	TRY {
		fp_new(t1);
		fp_new(t2);
		fp_new(t3);

		/* t1 = a[0]^2 - u^2 * a[1]^2 */
		fp_sqr(t1, a[0]);
		fp_sqr(t2, a[1]);
		for (int i = -1; i > fp_prime_get_qnr(); i--) {
			fp_add(t1, t1, t2);
		}
		for (int i = 0; i <= fp_prime_get_qnr(); i++) {
			fp_sub(t1, t1, t2);
		}
		fp_add(t1, t1, t2);

		if (fp_srt(t2, t1)) {
			/* t1 = (a_0 + sqrt(t1)) / 2 */
			fp_add(t1, a[0], t2);
			fp_set_dig(t3, 2);
			fp_inv(t3, t3);
			fp_mul(t1, t1, t3);

			if (!fp_srt(t3, t1)) {
				/* t1 = (a_0 - sqrt(t1)) / 2 */
				fp_sub(t1, a[0], t2);
				fp_set_dig(t3, 2);
				fp_inv(t3, t3);
				fp_mul(t1, t1, t3);
				fp_srt(t3, t1);
			}
			/* c_0 = sqrt(t1) */
			fp_copy(c[0], t3);
			/* c_1 = a_1 / (2 * sqrt(t1)) */
			fp_dbl(t3, t3);
			fp_inv(t3, t3);
			fp_mul(c[1], a[1], t3);
			r = 1;
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp_free(t1);
		fp_free(t2);
		fp_free(t3);
	}
	return r;
}

int fp3_srt(fp3_t c, fp3_t a) {
	int r = 0;
	fp3_t t0, t1, t2, t3;
	bn_t e;

	fp3_null(t0);
	fp3_null(t1);
	fp3_null(t2);
	fp3_null(t3);
	bn_null(e);

	TRY {
		fp3_new(t0);
		fp3_new(t1);
		fp3_new(t2);
		fp3_new(t3);
		bn_new(e);

		fp3_dbl(t3, a);
		fp3_frb(t0, t3, 1);

		fp3_sqr(t1, t0);
		fp3_mul(t2, t1, t0);
		fp3_mul(t1, t1, t2);

		fp3_frb(t0, t0, 1);
		fp3_mul(t3, t3, t1);
		fp3_mul(t0, t0, t3);

		e->used = FP_DIGS;
		dv_copy(e->dp, fp_prime_get(), FP_DIGS);
		bn_sub_dig(e, e, 5);
		bn_div_dig(e, e, 8);
		fp3_exp(t0, t0, e);

		fp3_mul(t0, t0, t2);
		fp3_sqr(t1, t0);
		fp3_mul(t1, t1, a);
		fp3_dbl(t1, t1);

		fp3_mul(t0, t0, a);
		fp_sub_dig(t1[0], t1[0], 1);
		fp3_mul(c, t0, t1);

		fp3_sqr(t0, c);
		if (fp3_cmp(t0, a) == CMP_EQ) {
			r = 1;
		}
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp3_free(t0);
		fp3_free(t1);
		fp3_free(t2);
		fp3_free(t3);
		bn_free(e);
	}

	return r;
}
