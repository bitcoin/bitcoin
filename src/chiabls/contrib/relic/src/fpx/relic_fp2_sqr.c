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
 * Implementation of squaring in a quadratic extension of a prime field.
 *
 * @ingroup fpx
 */

#include "relic_core.h"
#include "relic_fpx_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if FPX_QDR == BASIC || !defined(STRIP)

void fp2_sqr_basic(fp2_t c, fp2_t a) {
	fp_t t0, t1, t2;

	fp_null(t0);
	fp_null(t1);
	fp_null(t2);

	TRY {
		fp_new(t0);
		fp_new(t1);
		fp_new(t2);

		/* t0 = (a_0 + a_1). */
		fp_add(t0, a[0], a[1]);

		/* t1 = (a_0 - a_1). */
		fp_sub(t1, a[0], a[1]);

		/* t1 = a_0 + u^2 * a_1. */
		for (int i = -1; i > fp_prime_get_qnr(); i--) {
			fp_sub(t1, t1, a[1]);
		}
		for (int i = 0; i <= fp_prime_get_qnr(); i++) {
			fp_add(t1, t1, a[1]);
		}

		if (fp_prime_get_qnr() == -1) {
			/* t2 = 2 * a_0. */
			fp_dbl(t2, a[0]);
			/* c_1 = 2 * a_0 * a_1. */
			fp_mul(c[1], t2, a[1]);
			/* c_0 = a_0^2 + a_1^2 * u^2. */
			fp_mul(c[0], t0, t1);
		} else {
			/* c_1 = a_0 * a_1. */
			fp_mul(c[1], a[0], a[1]);
			/* c_0 = a_0^2 + a_1^2 * u^2. */
			fp_mul(c[0], t0, t1);
			for (int i = -1; i > fp_prime_get_qnr(); i--) {
				fp_add(c[0], c[0], c[1]);
			}
			for (int i = 0; i <= fp_prime_get_qnr(); i++) {
				fp_sub(c[0], c[0], c[1]);
			}
			/* c_1 = 2 * a_0 * a_1. */
			fp_dbl(c[1], c[1]);
		}
		/* c = c_0 + c_1 * u. */
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp_free(t0);
		fp_free(t1);
		fp_free(t2);
	}
}

#endif

#if FPX_QDR == INTEG || !defined(STRIP)

void fp2_sqr_integ(fp2_t c, fp2_t a) {
	fp2_sqrm_low(c, a);
}

#endif
