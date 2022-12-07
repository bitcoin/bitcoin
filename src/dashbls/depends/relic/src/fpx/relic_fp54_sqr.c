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
 * Implementation of squaring in a 54-degree extension of a prime field.
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

void fp54_sqr_basic(fp54_t c, fp54_t a) {
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

		/* t0 = a_0^2 */
		fp18_sqr(t0, a[0]);

		/* t1 = 2 * a_1 * a_2 */
		fp18_mul(t1, a[1], a[2]);
		fp18_dbl(t1, t1);

		/* t2 = a_2^2. */
		fp18_sqr(t2, a[2]);

		/* c_2 = a_0 + a_2. */
		fp18_add(c[2], a[0], a[2]);

		/* t3 = (a_0 + a_2 + a_1)^2. */
		fp18_add(t3, c[2], a[1]);
		fp18_sqr(t3, t3);

		/* c_2 = (a_0 + a_2 - a_1)^2. */
		fp18_sub(c[2], c[2], a[1]);
		fp18_sqr(c[2], c[2]);

		/* c_2 = (c_2 + t3)/2. */
		fp18_add(c[2], c[2], t3);
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				fp_hlv(c[2][0][i][j], c[2][0][i][j]);
				fp_hlv(c[2][1][i][j], c[2][1][i][j]);
			}
		}

		/* t3 = t3 - c_2 - t1. */
		fp18_sub(t3, t3, c[2]);
		fp18_sub(t3, t3, t1);

		/* c_2 = c_2 - t0 - t2. */
		fp18_sub(c[2], c[2], t0);
		fp18_sub(c[2], c[2], t2);

		/* c_0 = t0 + t1 * E. */
		fp18_mul_art(t4, t1);
		fp18_add(c[0], t0, t4);

		/* c_1 = t3 + t2 * E. */
		fp18_mul_art(t4, t2);
		fp18_add(c[1], t3, t4);
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

void fp54_sqr_cyc_basic(fp54_t c, fp54_t a) {
	fp9_t t0, t1, t2, t3, t4, t5, t6;

	fp9_null(t0);
	fp9_null(t1);
	fp9_null(t2);
	fp9_null(t3);
	fp9_null(t4);
	fp9_null(t5);
	fp9_null(t6);

	RLC_TRY {
		fp9_new(t0);
		fp9_new(t1);
		fp9_new(t2);
		fp9_new(t3);
		fp9_new(t4);
		fp9_new(t5);
		fp9_new(t6);

		fp9_sqr(t2, a[0][0]);
		fp9_sqr(t3, a[0][1]);
		fp9_add(t1, a[0][0], a[0][1]);

		fp9_mul_art(t0, t3);
		fp9_add(t0, t0, t2);

		fp9_sqr(t1, t1);
		fp9_sub(t1, t1, t2);
		fp9_sub(t1, t1, t3);

		fp9_sub(c[0][0], t0, a[0][0]);
		fp9_add(c[0][0], c[0][0], c[0][0]);
		fp9_add(c[0][0], t0, c[0][0]);

		fp9_add(c[0][1], t1, a[0][1]);
		fp9_add(c[0][1], c[0][1], c[0][1]);
		fp9_add(c[0][1], t1, c[0][1]);

		fp9_sqr(t0, a[2][0]);
		fp9_sqr(t1, a[2][1]);
		fp9_add(t5, a[2][0], a[2][1]);
		fp9_sqr(t2, t5);

		fp9_add(t3, t0, t1);
		fp9_sub(t5, t2, t3);

		fp9_add(t6, a[1][0], a[1][1]);
		fp9_sqr(t3, t6);
		fp9_sqr(t2, a[1][0]);

		fp9_mul_art(t6, t5);
		fp9_add(t5, t6, a[1][0]);
		fp9_dbl(t5, t5);
		fp9_add(c[1][0], t5, t6);

		fp9_mul_art(t4, t1);
		fp9_add(t5, t0, t4);
		fp9_sub(t6, t5, a[1][1]);

		fp9_sqr(t1, a[1][1]);

		fp9_dbl(t6, t6);
		fp9_add(c[1][1], t6, t5);

		fp9_mul_art(t4, t1);
		fp9_add(t5, t2, t4);
		fp9_sub(t6, t5, a[2][0]);
		fp9_dbl(t6, t6);
		fp9_add(c[2][0], t6, t5);

		fp9_add(t0, t2, t1);
		fp9_sub(t5, t3, t0);
		fp9_add(t6, t5, a[2][1]);
		fp9_dbl(t6, t6);
		fp9_add(c[2][1], t5, t6);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp9_free(t0);
		fp9_free(t1);
		fp9_free(t2);
		fp9_free(t3);
		fp9_free(t4);
		fp9_free(t5);
		fp9_free(t6);
	}
}

void fp54_sqr_pck_basic(fp54_t c, fp54_t a) {
	fp9_t t0, t1, t2, t3, t4, t5, t6;

	fp9_null(t0);
	fp9_null(t1);
	fp9_null(t2);
	fp9_null(t3);
	fp9_null(t4);
	fp9_null(t5);
	fp9_null(t6);

	RLC_TRY {
		fp9_new(t0);
		fp9_new(t1);
		fp9_new(t2);
		fp9_new(t3);
		fp9_new(t4);
		fp9_new(t5);
		fp9_new(t6);

		fp9_sqr(t0, a[2][0]);
		fp9_sqr(t1, a[2][1]);
		fp9_add(t5, a[2][0], a[2][1]);
		fp9_sqr(t2, t5);

		fp9_add(t3, t0, t1);
		fp9_sub(t5, t2, t3);

		fp9_add(t6, a[1][0], a[1][1]);
		fp9_sqr(t3, t6);
		fp9_sqr(t2, a[1][0]);

		fp9_mul_art(t6, t5);
		fp9_add(t5, t6, a[1][0]);
		fp9_dbl(t5, t5);
		fp9_add(c[1][0], t5, t6);

		fp9_mul_art(t4, t1);
		fp9_add(t5, t0, t4);
		fp9_sub(t6, t5, a[1][1]);

		fp9_sqr(t1, a[1][1]);

		fp9_dbl(t6, t6);
		fp9_add(c[1][1], t6, t5);

		fp9_mul_art(t4, t1);
		fp9_add(t5, t2, t4);
		fp9_sub(t6, t5, a[2][0]);
		fp9_dbl(t6, t6);
		fp9_add(c[2][0], t6, t5);

		fp9_add(t0, t2, t1);
		fp9_sub(t5, t3, t0);
		fp9_add(t6, t5, a[2][1]);
		fp9_dbl(t6, t6);
		fp9_add(c[2][1], t5, t6);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp9_free(t0);
		fp9_free(t1);
		fp9_free(t2);
		fp9_free(t3);
		fp9_free(t4);
		fp9_free(t5);
		fp9_free(t6);
	}
}

#endif

#if FPX_RDC == LAZYR || !defined(STRIP)

void fp54_sqr_lazyr(fp54_t c, fp54_t a) {
	/* TODO: implement lazy reduction. */
	fp54_sqr_basic(c, a);
}

void fp54_sqr_cyc_lazyr(fp54_t c, fp54_t a) {
	/* TODO: implement lazy reduction. */
	fp54_sqr_cyc_basic(c, a);
}

void fp54_sqr_pck_lazyr(fp54_t c, fp54_t a) {
	fp9_t t0, t1, t2;
	dv9_t u0, u1, u2, u3;

	fp9_null(t0);
	fp9_null(t1);
	fp9_null(t2);
	dv9_null(u0);
	dv9_null(u1);
	dv9_null(u2);
	dv9_null(u3);

	RLC_TRY {
		fp9_new(t0);
		fp9_new(t1);
		fp9_new(t2);
		dv9_new(u0);
		dv9_new(u1);
		dv9_new(u2);
		dv9_new(u3);

		fp9_sqr_unr(u0, a[2][0]);
		fp9_sqr_unr(u1, a[2][1]);
		fp9_add(t0, a[2][0], a[2][1]);
		fp9_sqr_unr(u2, t0);

		for (int i = 0; i < 3; i++) {
			fp3_addc_low(u3[i], u0[i], u1[i]);
			fp3_subc_low(u3[i], u2[i], u3[i]);
			fp3_rdcn_low(t0[i], u3[i]);
		}

		fp9_add(t1, a[1][0], a[1][1]);
		fp9_sqr(t2, t1);
		fp9_sqr_unr(u2, a[1][0]);

		fp9_mul_art(t1, t0);
		fp9_add(t0, t1, a[1][0]);
		fp9_dbl(t0, t0);
		fp9_add(c[1][0], t0, t1);

		fp3_nord_low(u3[0], u1[2]);
		fp3_addc_low(u3[0], u0[0], u3[0]);
		fp3_addc_low(u3[1], u0[1], u1[0]);
		fp3_addc_low(u3[2], u0[2], u1[1]);
		fp9_sqr_unr(u1, a[1][1]);
		for (int i = 0; i < 3; i++) {
			fp3_rdcn_low(t0[i], u3[i]);
		}
		fp9_sub(t1, t0, a[1][1]);
		fp9_dbl(t1, t1);
		fp9_add(c[1][1], t1, t0);

		for (int i = 0; i < 3; i++) {
			fp3_addc_low(u0[i], u2[i], u1[i]);
			fp3_rdcn_low(t0[i], u0[i]);
		}
		fp9_sub(t0, t2, t0);
		fp9_add(t1, t0, a[2][1]);
		fp9_dbl(t1, t1);
		fp9_add(c[2][1], t0, t1);

		fp3_nord_low(u3[0], u1[2]);
		fp3_addc_low(u3[0], u2[0], u3[0]);
		fp3_addc_low(u3[1], u2[1], u1[0]);
		fp3_addc_low(u3[2], u2[2], u1[1]);
		for (int i = 0; i < 3; i++) {
			fp3_rdcn_low(t0[i], u3[i]);
		}
		fp9_sub(t1, t0, a[2][0]);
		fp9_dbl(t1, t1);
		fp9_add(c[2][0], t1, t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp9_free(t0);
		fp9_free(t1);
		fp9_free(t2);
		dv9_free(u0);
		dv9_free(u1);
		dv9_free(u2);
		dv9_free(u3);
	}
}

#endif
