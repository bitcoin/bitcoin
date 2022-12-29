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
 * Implementation of multiplication in a 24-degree extension of a prime field.
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

void fp24_mul_basic(fp24_t c, fp24_t a, fp24_t b) {
	fp8_t t0, t1, t2, t3, t4, t5;

	fp8_null(t0);
	fp8_null(t1);
	fp8_null(t2);
	fp8_null(t3);
	fp8_null(t4);
	fp8_null(t5);

	RLC_TRY {
		fp8_new(t0);
		fp8_new(t1);
		fp8_new(t2);
		fp8_new(t3);
		fp8_new(t4);
		fp8_new(t5);

		/* Karatsuba algorithm. */

		/* t0 = a_0 * b_0. */
		fp8_mul(t0, a[0], b[0]);
		/* t1 = a_1 * b_1. */
		fp8_mul(t1, a[1], b[1]);
		/* t2 = a_2 * b_2. */
		fp8_mul(t2, a[2], b[2]);

		fp8_add(t3, a[1], a[2]);
		fp8_add(t4, b[1], b[2]);
		fp8_mul(t3, t3, t4);
		fp8_sub(t3, t3, t1);
		fp8_sub(t3, t3, t2);
		fp8_mul_art(t3, t3);
		fp8_add(t3, t3, t0);

		fp8_add(t4, a[0], a[1]);
		fp8_add(t5, b[0], b[1]);
		fp8_mul(t4, t4, t5);
		fp8_sub(t4, t4, t0);
		fp8_sub(t4, t4, t1);
		fp8_mul_art(t5, t2);
		fp8_add(c[1], t4, t5);

		fp8_add(t4, a[0], a[2]);
		fp8_add(t5, b[0], b[2]);
		fp8_mul(c[2], t4, t5);
		fp8_sub(c[2], c[2], t0);
		fp8_add(c[2], c[2], t1);
		fp8_sub(c[2], c[2], t2);

		fp8_copy(c[0], t3);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp8_free(t0);
		fp8_free(t1);
		fp8_free(t2);
		fp8_free(t3);
		fp8_free(t4);
		fp8_free(t5);
	}
}

#endif

#if FPX_RDC == LAZYR || !defined(STRIP)

void fp24_mul_unr(dv24_t c, fp24_t a, fp24_t b) {
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

		/* Karatsuba algorithm. */

		/* u0 = a_0 * b_0. */
		fp8_mul_unr(u0, a[0], b[0]);
		/* u1 = a_1 * b_1. */
		fp8_mul_unr(u1, a[1], b[1]);
		/* u2 = a_2 * b_2. */
		fp8_mul_unr(u2, a[2], b[2]);

		fp8_add(t0, a[1], a[2]);
		fp8_add(t1, b[1], b[2]);
		fp8_mul_unr(u3, t0, t1);
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				fp2_subc_low(u3[i][j], u3[i][j], u1[i][j]);
				fp2_subc_low(u3[i][j], u3[i][j], u2[i][j]);
			}
		}

		fp2_nord_low(u4[0][0], u3[1][1]);
		dv_copy(u4[0][1][0], u3[1][0][0], 2 * RLC_FP_DIGS);
		dv_copy(u4[0][1][1], u3[1][0][1], 2 * RLC_FP_DIGS);
		for (int j = 0; j < 2; j++) {
			fp2_addc_low(c[0][0][j], u4[0][j], u0[0][j]);
			fp2_addc_low(c[0][1][j], u3[0][j], u0[1][j]);
		}

		fp8_add(t0, a[0], a[1]);
		fp8_add(t1, b[0], b[1]);
		fp8_mul_unr(u4, t0, t1);
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				fp2_subc_low(u4[i][j], u4[i][j], u0[i][j]);
				fp2_subc_low(u4[i][j], u4[i][j], u1[i][j]);
			}
		}
		fp2_nord_low(u3[0][0], u2[1][1]);
		dv_copy(u3[0][1][0], u2[1][0][0], 2 * RLC_FP_DIGS);
		dv_copy(u3[0][1][1], u2[1][0][1], 2 * RLC_FP_DIGS);
		for (int j = 0; j < 2; j++) {
			fp2_addc_low(c[1][0][j], u4[0][j], u3[0][j]);
			fp2_addc_low(c[1][1][j], u4[1][j], u2[0][j]);
		}

		fp8_add(t0, a[0], a[2]);
		fp8_add(t1, b[0], b[2]);
		fp8_mul_unr(u4, t0, t1);
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				fp2_subc_low(u4[i][j], u4[i][j], u0[i][j]);
				fp2_addc_low(u4[i][j], u4[i][j], u1[i][j]);
				fp2_subc_low(c[2][i][j], u4[i][j], u2[i][j]);
			}
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

void fp24_mul_lazyr(fp24_t c, fp24_t a, fp24_t b) {
	dv24_t t;

	dv24_null(t);

	RLC_TRY {
		dv24_new(t);
		fp24_mul_unr(t, a, b);
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

#endif

void fp24_mul_art(fp24_t c, fp24_t a) {
	fp8_t t0;

	fp8_null(t0);

	RLC_TRY {
		fp8_new(t0);

		/* (a_0 + a_1 * v + a_2 * v^2) * v = a_2 + a_0 * v + a_1 * v^2 */
		fp8_copy(t0, a[0]);
		fp8_mul_art(c[0], a[2]);
		fp8_copy(c[2], a[1]);
		fp8_copy(c[1], t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp8_free(t0);
	}
}

void fp24_mul_dxs(fp24_t c, fp24_t a, fp24_t b) {
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

		/* Karatsuba algorithm. */

		/* t0 = a_0 * b_0. */
		fp8_mul(t0, a[0], b[0]);
		/* t1 = a_1 * b_1. */
		fp8_mul(t1, a[1], b[1]);
		/* b_2 = 0. */

		fp8_add(t3, a[1], a[2]);
		fp8_mul(t3, t3, b[1]);
		fp8_sub(t3, t3, t1);
		fp8_mul_art(t3, t3);
		fp8_add(t3, t3, t0);

		fp8_add(t4, a[0], a[1]);
		fp8_add(t2, b[0], b[1]);
		fp8_mul(t4, t4, t2);
		fp8_sub(t4, t4, t0);
		fp8_sub(c[1], t4, t1);

		fp8_add(t4, a[0], a[2]);
		fp8_mul(c[2], t4, b[0]);
		fp8_sub(c[2], c[2], t0);
		fp8_add(c[2], c[2], t1);

		fp8_copy(c[0], t3);
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
