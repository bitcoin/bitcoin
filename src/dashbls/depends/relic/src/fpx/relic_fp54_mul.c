/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 4007-4019 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or modify it under the
 * terms of the version 4.1 (or later) of the GNU Lesser General Public License
 * as published by the Free Software Foundation; or version 4.0 of the Apache
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
 * Implementation of multiplication in a 54-degree extension of a prime field.
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

void fp54_mul_basic(fp54_t c, fp54_t a, fp54_t b) {
	fp18_t t0, t1, t2, t3, t4, t5;

	fp18_null(t0);
	fp18_null(t1);
	fp18_null(t2);
	fp18_null(t3);
	fp18_null(t4);
	fp18_null(t5);

	RLC_TRY {
		fp18_new(t0);
		fp18_new(t1);
		fp18_new(t2);
		fp18_new(t3);
		fp18_new(t4);
		fp18_new(t5);

		/* Karatsuba algorithm. */

		/* t0 = a_0 * b_0. */
		fp18_mul(t0, a[0], b[0]);
		/* t1 = a_1 * b_1. */
		fp18_mul(t1, a[1], b[1]);
		/* t2 = a_2 * b_2. */
		fp18_mul(t2, a[2], b[2]);

		fp18_add(t3, a[1], a[2]);
		fp18_add(t4, b[1], b[2]);
		fp18_mul(t3, t3, t4);
		fp18_sub(t3, t3, t1);
		fp18_sub(t3, t3, t2);
		fp18_mul_art(t3, t3);
		fp18_add(t3, t3, t0);

		fp18_add(t4, a[0], a[1]);
		fp18_add(t5, b[0], b[1]);
		fp18_mul(t4, t4, t5);
		fp18_sub(t4, t4, t0);
		fp18_sub(t4, t4, t1);
		fp18_mul_art(t5, t2);
		fp18_add(c[1], t4, t5);

		fp18_add(t4, a[0], a[2]);
		fp18_add(t5, b[0], b[2]);
		fp18_mul(c[2], t4, t5);
		fp18_sub(c[2], c[2], t0);
		fp18_add(c[2], c[2], t1);
		fp18_sub(c[2], c[2], t2);

		fp18_copy(c[0], t3);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp18_free(t0);
		fp18_free(t1);
		fp18_free(t2);
		fp18_free(t3);
		fp18_free(t4);
		fp18_free(t5);
	}
}

#endif

#if FPX_RDC == LAZYR || !defined(STRIP)

void fp54_mul_lazyr(fp54_t c, fp54_t a, fp54_t b) {
	/* TODO: implement lazy reduction. */
	fp54_mul_basic(c, a, b);
}

#endif

void fp54_mul_dxs(fp54_t c, fp54_t a, fp54_t b) {
	fp18_t t0, t1, t2, t3, t4;

	fp18_null(t0);
	fp18_null(t1);
	fp18_null(t2);
	fp18_null(t3);
	fp18_null(t4);

	RLC_TRY {
		fp18_new(t0);
		fp18_new(t1);
		fp18_new(t2);
		fp18_new(t3);
		fp18_new(t4);

		/* Karatsuba algorithm. */

		/* t0 = a_0 * b_0. */
#if EP_ADD == BASIC
		fp18_mul_dxs(t0, a[0], b[0]);
#else
		fp18_mul(t0, a[0], b[0]);
#endif
		/* t2 = a_2 * b_2. */
		fp9_mul(t2[0], a[2][0], b[2][0]);
		fp9_mul(t2[1], a[2][1], b[2][0]);

		fp18_add(t3, a[1], a[2]);
		fp9_mul(t3[0], t3[0], b[2][0]);
		fp9_mul(t3[1], t3[1], b[2][0]);
		fp18_sub(t3, t3, t2);
		fp18_mul_art(t3, t3);
		fp18_add(t3, t3, t0);

		fp18_add(t4, a[0], a[1]);
#if EP_ADD == BASIC
		fp18_mul_dxs(t4, t4, b[0]);
#else
		fp18_mul(t4, t4, b[0]);
#endif
		fp18_sub(t4, t4, t0);
		fp18_mul_art(t1, t2);
		fp18_add(c[1], t4, t1);

		fp18_add(t4, a[0], a[2]);
		fp9_add(t1[0], b[0][0], b[2][0]);
		fp9_copy(t1[1], b[0][1]);
#if EP_ADD == BASIC
		fp18_mul_dxs(c[2], t4, t1);
#else
		fp18_mul(c[2], t4, t1);
#endif
		fp18_sub(c[2], c[2], t0);
		fp18_sub(c[2], c[2], t2);

		fp18_copy(c[0], t3);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp18_free(t0);
		fp18_free(t1);
		fp18_free(t2);
		fp18_free(t3);
		fp18_free(t4);
	}
}

void fp54_mul_art(fp54_t c, fp54_t a) {
	fp18_t t0;

	fp18_null(t0);

	RLC_TRY {
		fp18_new(t0);

		/* (a_0 + a_1 * v + a_2 * v^2) * v = a_2 + a_0 * v + a_1 * v^2 */
		fp18_copy(t0, a[0]);
		fp18_mul_art(c[0], a[2]);
		fp18_copy(c[2], a[1]);
		fp18_copy(c[1], t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp18_free(t0);
	}
}
