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
 * Implementation of multiplication in a 48-degree extension of a prime field.
 *
 * @ingroup fpx
 */

#include "relic_core.h"
#include "relic_fp_low.h"
#include "relic_fpx_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if FPX_RDC == BASIC || !defined(STRIP)

void fp48_mul_basic(fp48_t c, fp48_t a, fp48_t b) {
	fp24_t t0, t1, t2;

	fp24_null(t0);
	fp24_null(t1);
	fp24_null(t2);

	RLC_TRY {
		fp24_new(t0);
		fp24_new(t1);
		fp24_new(t2);

		/* Karatsuba algorithm. */

		/* t0 = a_0 * b_0. */
		fp24_mul(t0, a[0], b[0]);
		/* t1 = a_1 * b_1. */
		fp24_mul(t1, a[1], b[1]);
		/* t2 = b_0 + b_1. */
		fp24_add(t2, b[0], b[1]);

		/* c_1 = a_0 + a_1. */
		fp24_add(c[1], a[0], a[1]);

		/* c_1 = (a_0 + a_1) * (b_0 + b_1) */
		fp24_mul(c[1], c[1], t2);
		fp24_sub(c[1], c[1], t0);
		fp24_sub(c[1], c[1], t1);

		/* c_0 = a_0b_0 + v * a_1b_1. */
		fp24_mul_art(t1, t1);
		fp24_add(c[0], t0, t1);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp24_free(t0);
		fp24_free(t1);
		fp24_free(t2);
	}
}

#endif

#if FPX_RDC == LAZYR || !defined(STRIP)

void fp48_mul_lazyr(fp48_t c, fp48_t a, fp48_t b) {
	/* TODO: implement lazy reduction. */
	fp48_mul_basic(c, a, b);
}

#endif

void fp48_mul_dxs(fp48_t c, fp48_t a, fp48_t b) {
	fp24_t t0, t1, t2;

	fp24_null(t0);
	fp24_null(t1);
	fp24_null(t2);

	RLC_TRY {
		fp24_new(t0);
		fp24_new(t1);
		fp24_new(t2);

		/* Karatsuba algorithm. */

		/* t0 = a_0 * b_0. */
		fp24_mul_dxs(t0, a[0], b[0]);
		/* t1 = a_1 * b_1. */
#if EP_ADD == BASIC
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				for (int k = 0; k < 2; k++) {
					fp_mul(t1[0][i][j][k], a[1][0][i][j][k], b[1][1][0][0][0]);
					fp_mul(t1[1][i][j][k], a[1][1][i][j][k], b[1][1][0][0][0]);
					fp_mul(t1[2][i][j][k], a[1][2][i][j][k], b[1][1][0][0][0]);
				}
			}
		}
#else
		fp8_mul(t1[0], a[1][0], b[1][1]);
		fp8_mul(t1[1], a[1][1], b[1][1]);
		fp8_mul(t1[2], a[1][2], b[1][1]);
#endif
		fp24_mul_art(t1, t1);
		/* t2 = b_0 + b_1. */
		fp8_copy(t2[0], b[0][0]);
#if EP_ADD == BASIC
		fp8_copy(t2[1], b[0][1]);
		fp_add(t2[1][0][0][0], t2[1][0][0][0], b[1][1][0][0][0]);
#else
		fp8_add(t2[1], b[0][1], b[1][1]);
#endif
		fp8_copy(t2[2], b[0][2]);

		/* c_1 = a_0 + a_1. */
		fp24_add(c[1], a[0], a[1]);

		/* c_1 = (a_0 + a_1) * (b_0 + b_1) */
		fp24_mul_dxs(c[1], c[1], t2);
		fp24_sub(c[1], c[1], t0);
		fp24_sub(c[1], c[1], t1);

		/* c_0 = a_0b_0 + v * a_1b_1. */
		fp24_mul_art(t1, t1);
		fp24_add(c[0], t0, t1);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp24_free(t0);
		fp24_free(t1);
		fp24_free(t2);
	}
}

void fp48_mul_art(fp48_t c, fp48_t a) {
	fp24_t t0;

	fp24_null(t0);

	RLC_TRY {
		fp24_new(t0);

		/* (a_0 + a_1 * v) * v = a_0 * v + a_1 * v^2 */
		fp24_copy(t0, a[0]);
		fp24_mul_art(c[0], a[1]);
		fp24_copy(c[1], t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp24_free(t0);
	}
}
