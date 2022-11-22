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
 * Implementation of the square root function.
 *
 * @ingroup bn
 */

#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int fp_srt(fp_t c, const fp_t a) {
	bn_t e;
	fp_t t0;
	fp_t t1;
	int r = 0;

	bn_null(e);
	fp_null(t0);
	fp_null(t1);

	if (fp_is_zero(a)) {
		fp_zero(c);
		return 1;
	}

	RLC_TRY {
		bn_new(e);
		fp_new(t0);
		fp_new(t1);

		/* Make e = p. */
		e->used = RLC_FP_DIGS;
		dv_copy(e->dp, fp_prime_get(), RLC_FP_DIGS);

		if (fp_prime_get_mod8() == 3 || fp_prime_get_mod8() == 7) {
			/* Easy case, compute a^((p + 1)/4). */
			bn_add_dig(e, e, 1);
			bn_rsh(e, e, 2);

			fp_exp(t0, a, e);
			fp_sqr(t1, t0);
			r = (fp_cmp(t1, a) == RLC_EQ);
			fp_copy(c, t0);
		} else {
			int f = 0, m = 0;

			/* First, check if there is a root. Compute t1 = a^((p - 1)/2). */
			bn_rsh(e, e, 1);
			fp_exp(t0, a, e);

			if (fp_cmp_dig(t0, 1) != RLC_EQ) {
				/* Nope, there is no square root. */
				r = 0;
			} else {
				r = 1;
				/* Find a quadratic non-residue modulo p, that is a number t2
				 * such that (t2 | p) = t2^((p - 1)/2)!= 1. */
				do {
					fp_rand(t1);
					fp_exp(t0, t1, e);
				} while (fp_cmp_dig(t0, 1) == RLC_EQ);

				/* Write p - 1 as (e * 2^f), odd e. */
				bn_lsh(e, e, 1);
				while (bn_is_even(e)) {
					bn_rsh(e, e, 1);
					f++;
				}

				/* Compute t2 = t2^e. */
				fp_exp(t1, t1, e);

				/* Compute t1 = a^e, c = a^((e + 1)/2) = a^(e/2 + 1), odd e. */
				bn_rsh(e, e, 1);
				fp_exp(t0, a, e);
				fp_mul(e->dp, t0, a);
				fp_sqr(t0, t0);
				fp_mul(t0, t0, a);
				fp_copy(c, e->dp);

				while (1) {
					if (fp_cmp_dig(t0, 1) == RLC_EQ) {
						break;
					}
					fp_copy(e->dp, t0);
					for (m = 0; (m < f) && (fp_cmp_dig(t0, 1) != RLC_EQ); m++) {
						fp_sqr(t0, t0);
					}
					fp_copy(t0, e->dp);
					for (int i = 0; i < f - m - 1; i++) {
						fp_sqr(t1, t1);
					}
					fp_mul(c, c, t1);
					fp_sqr(t1, t1);
					fp_mul(t0, t0, t1);
					f = m;
				}
			}
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(e);
		fp_free(t0);
		fp_free(t1);
	}
	return r;
}
