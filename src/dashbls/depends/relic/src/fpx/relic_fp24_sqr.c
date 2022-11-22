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
 * Implementation of squaring in a 24-degree extension of a prime field.
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

void fp24_sqr_basic(fp24_t c, fp24_t a) {
	fp8_t t0, t1, t2, t3, t4;

	fp8_null(t0);
	fp8_null(t1);
	fp8_null(t2);
	fp8_null(t3);
	fp8_null(t4);

	RLC_TRY {
		fp8_new(t0);
		fp8_new(t1);
		fp8_new(t2);
		fp8_new(t3);
		fp8_new(t4);

		/* t0 = a_0^2 */
		fp8_sqr(t0, a[0]);

		/* t1 = 2 * a_1 * a_2 */
		fp8_mul(t1, a[1], a[2]);
		fp8_dbl(t1, t1);

		/* t2 = a_2^2. */
		fp8_sqr(t2, a[2]);

		/* c_2 = a_0 + a_2. */
		fp8_add(c[2], a[0], a[2]);

		/* t3 = (a_0 + a_2 + a_1)^2. */
		fp8_add(t3, c[2], a[1]);
		fp8_sqr(t3, t3);

		/* c_2 = (a_0 + a_2 - a_1)^2. */
		fp8_sub(c[2], c[2], a[1]);
		fp8_sqr(c[2], c[2]);

		/* c_2 = (c_2 + t3)/2. */
		fp8_add(c[2], c[2], t3);
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				fp_hlv(c[2][0][i][j], c[2][0][i][j]);
				fp_hlv(c[2][1][i][j], c[2][1][i][j]);
			}
		}

		/* t3 = t3 - c_2 - t1. */
		fp8_sub(t3, t3, c[2]);
		fp8_sub(t3, t3, t1);

		/* c_2 = c_2 - t0 - t2. */
		fp8_sub(c[2], c[2], t0);
		fp8_sub(c[2], c[2], t2);

		/* c_0 = t0 + t1 * E. */
		fp8_mul_art(t4, t1);
		fp8_add(c[0], t0, t4);

		/* c_1 = t3 + t2 * E. */
		fp8_mul_art(t4, t2);
		fp8_add(c[1], t3, t4);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp8_free(t0);
		fp8_free(t1);
		fp8_free(t2);
		fp8_free(t3);
		fp8_free(t4);
	}
}

void fp24_sqr_cyc_basic(fp24_t c, fp24_t a) {
	fp4_t t0, t1, t2, t3, t4, t5, t6;

	fp4_null(t0);
	fp4_null(t1);
	fp4_null(t2);
	fp4_null(t3);
	fp4_null(t4);
	fp4_null(t5);
	fp4_null(t6);

	RLC_TRY {
		fp4_new(t0);
		fp4_new(t1);
		fp4_new(t2);
		fp4_new(t3);
		fp4_new(t4);
		fp4_new(t5);
		fp4_new(t6);

		fp4_sqr(t2, a[0][0]);
		fp4_sqr(t3, a[0][1]);
		fp4_add(t1, a[0][0], a[0][1]);

		fp4_mul_art(t0, t3);
		fp4_add(t0, t0, t2);

		fp4_sqr(t1, t1);
		fp4_sub(t1, t1, t2);
		fp4_sub(t1, t1, t3);

		fp4_sub(c[0][0], t0, a[0][0]);
		fp4_add(c[0][0], c[0][0], c[0][0]);
		fp4_add(c[0][0], t0, c[0][0]);

		fp4_add(c[0][1], t1, a[0][1]);
		fp4_add(c[0][1], c[0][1], c[0][1]);
		fp4_add(c[0][1], t1, c[0][1]);

		fp4_sqr(t0, a[2][0]);
		fp4_sqr(t1, a[2][1]);
		fp4_add(t5, a[2][0], a[2][1]);
		fp4_sqr(t2, t5);

		fp4_add(t3, t0, t1);
		fp4_sub(t5, t2, t3);

		fp4_add(t6, a[1][0], a[1][1]);
		fp4_sqr(t3, t6);
		fp4_sqr(t2, a[1][0]);

		fp4_mul_art(t6, t5);
		fp4_add(t5, t6, a[1][0]);
		fp4_dbl(t5, t5);
		fp4_add(c[1][0], t5, t6);

		fp4_mul_art(t4, t1);
		fp4_add(t5, t0, t4);
		fp4_sub(t6, t5, a[1][1]);

		fp4_sqr(t1, a[1][1]);

		fp4_dbl(t6, t6);
		fp4_add(c[1][1], t6, t5);

		fp4_mul_art(t4, t1);
		fp4_add(t5, t2, t4);
		fp4_sub(t6, t5, a[2][0]);
		fp4_dbl(t6, t6);
		fp4_add(c[2][0], t6, t5);

		fp4_add(t0, t2, t1);
		fp4_sub(t5, t3, t0);
		fp4_add(t6, t5, a[2][1]);
		fp4_dbl(t6, t6);
		fp4_add(c[2][1], t5, t6);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp4_free(t0);
		fp4_free(t1);
		fp4_free(t2);
		fp4_free(t3);
		fp4_free(t4);
		fp4_free(t5);
		fp4_free(t6);
	}
}

void fp24_sqr_pck_basic(fp24_t c, fp24_t a) {
	fp4_t t0, t1, t2, t3, t4, t5, t6;

	fp4_null(t0);
	fp4_null(t1);
	fp4_null(t2);
	fp4_null(t3);
	fp4_null(t4);
	fp4_null(t5);
	fp4_null(t6);

	RLC_TRY {
		fp4_new(t0);
		fp4_new(t1);
		fp4_new(t2);
		fp4_new(t3);
		fp4_new(t4);
		fp4_new(t5);
		fp4_new(t6);

		fp4_sqr(t0, a[2][0]);
		fp4_sqr(t1, a[2][1]);
		fp4_add(t5, a[2][0], a[2][1]);
		fp4_sqr(t2, t5);

		fp4_add(t3, t0, t1);
		fp4_sub(t5, t2, t3);

		fp4_add(t6, a[1][0], a[1][1]);
		fp4_sqr(t3, t6);
		fp4_sqr(t2, a[1][0]);

		fp4_mul_art(t6, t5);
		fp4_add(t5, t6, a[1][0]);
		fp4_dbl(t5, t5);
		fp4_add(c[1][0], t5, t6);

		fp4_mul_art(t4, t1);
		fp4_add(t5, t0, t4);
		fp4_sub(t6, t5, a[1][1]);

		fp4_sqr(t1, a[1][1]);

		fp4_dbl(t6, t6);
		fp4_add(c[1][1], t6, t5);

		fp4_mul_art(t4, t1);
		fp4_add(t5, t2, t4);
		fp4_sub(t6, t5, a[2][0]);
		fp4_dbl(t6, t6);
		fp4_add(c[2][0], t6, t5);

		fp4_add(t0, t2, t1);
		fp4_sub(t5, t3, t0);
		fp4_add(t6, t5, a[2][1]);
		fp4_dbl(t6, t6);
		fp4_add(c[2][1], t5, t6);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp4_free(t0);
		fp4_free(t1);
		fp4_free(t2);
		fp4_free(t3);
		fp4_free(t4);
		fp4_free(t5);
		fp4_free(t6);
	}
}

#endif

#if FPX_RDC == LAZYR || !defined(STRIP)

void fp24_sqr_unr(dv24_t c, fp24_t a) {
	dv8_t u0, u1, u2, u3, u4;
	fp8_t t0, t1;

	dv8_null(u0);
	dv8_null(u1);
	dv8_null(u2);
	dv8_null(u3);
	dv8_null(u4);
	fp8_null(t0);
	fp8_null(t1);

	RLC_TRY {
		dv8_new(u0);
		dv8_new(u1);
		dv8_new(u2);
		dv8_new(u3);
		dv8_new(u4);
		fp8_new(t0);
		fp8_new(t1);

		/* u0 = a_0^2 */
		fp8_sqr_unr(u0, a[0]);

		/* u1 = 2 * a_1 * a_2 */
		fp8_mul_unr(u1, a[1], a[2]);
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				fp2_addc_low(u1[i][j], u1[i][j], u1[i][j]);
			}
		}

		/* t2 = a_2^2. */
		fp8_sqr_unr(u2, a[2]);

		/* t0 = a_0 + a_2. */
		fp8_add(t0, a[0], a[2]);

		/* u3 = (a_0 + a_2 + a_1)^2. */
		fp8_add(t1, t0, a[1]);
		fp8_sqr_unr(u3, t1);

		/* u4 = (a_0 + a_2 - a_1)^2. */
		fp8_sub(t0, t0, a[1]);
		fp8_sqr_unr(u4, t0);

		/* u4 = (u4 + u3)/2. */
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				fp2_addc_low(u4[i][j], u4[i][j], u3[i][j]);
				fp_hlvd_low(u4[i][j][0], u4[i][j][0]);
				fp_hlvd_low(u4[i][j][1], u4[i][j][1]);
			}
		}

		/* t3 = t3 - u4 - t1. */
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				fp2_subc_low(u3[i][j], u3[i][j], u4[i][j]);
				fp2_subc_low(u3[i][j], u3[i][j], u1[i][j]);
			}
		}

		/* c_2 = u4 - t0 - t2. */
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				fp2_subc_low(u4[i][j], u4[i][j], u0[i][j]);
				fp2_subc_low(c[2][i][j], u4[i][j], u2[i][j]);
			}
		}

		/* c_0 = t0 + t1 * E. */
		fp2_nord_low(u4[0][0], u1[1][1]);
		dv_copy(u4[0][1][0], u1[1][0][0], 2 * RLC_FP_DIGS);
		dv_copy(u4[0][1][1], u1[1][0][1], 2 * RLC_FP_DIGS);
		for (int j = 0; j < 2; j++) {
			fp2_addc_low(c[0][0][j], u4[0][j], u0[0][j]);
			fp2_addc_low(c[0][1][j], u1[0][j], u0[1][j]);
		}

		/* c_1 = t3 + t2 * E. */
		fp2_nord_low(u4[0][0], u2[1][1]);
		dv_copy(u4[0][1][0], u2[1][0][0], 2 * RLC_FP_DIGS);
		dv_copy(u4[0][1][1], u2[1][0][1], 2 * RLC_FP_DIGS);
		for (int j = 0; j < 2; j++) {
			fp2_addc_low(c[1][0][j], u4[0][j], u3[0][j]);
			fp2_addc_low(c[1][1][j], u2[0][j], u3[1][j]);
		}

	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		dv8_free(u0);
		dv8_free(u1);
		dv8_free(u2);
		dv8_free(u3);
		dv8_free(u4);
		fp8_free(t0);
		fp8_free(t1);
	}
}

void fp24_sqr_lazyr(fp24_t c, fp24_t a) {
	dv24_t t;

	dv24_null(t);

	RLC_TRY {
		dv24_new(t);
		fp24_sqr_unr(t, a);
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 2; j++) {
				fp2_rdcn_low(c[i][j][0], t[i][j][0]);
				fp2_rdcn_low(c[i][j][1], t[i][j][1]);
			}
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		dv24_free(t);
	}
}

void fp24_sqr_cyc_lazyr(fp24_t c, fp24_t a) {
	/* TODO: implement lazy reduction. */
	fp24_sqr_cyc_basic(c, a);
}

void fp24_sqr_pck_lazyr(fp24_t c, fp24_t a) {
	/* TODO: implement lazy reduction. */
	fp24_sqr_pck_basic(c, a);
}

#endif
