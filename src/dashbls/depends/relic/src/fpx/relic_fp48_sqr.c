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
 * Implementation of squaring in a dodecic extension of a prime field.
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

void fp48_sqr_basic(fp48_t c, fp48_t a) {
	fp24_t t0, t1;

	fp24_null(t0);
	fp24_null(t1);

	RLC_TRY {
		fp24_new(t0);
		fp24_new(t1);

		fp24_add(t0, a[0], a[1]);
		fp24_mul_art(t1, a[1]);
		fp24_add(t1, a[0], t1);
		fp24_mul(t0, t0, t1);
		fp24_mul(c[1], a[0], a[1]);
		fp24_sub(c[0], t0, c[1]);
		fp24_mul_art(t1, c[1]);
		fp24_sub(c[0], c[0], t1);
		fp24_dbl(c[1], c[1]);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp24_free(t0);
		fp24_free(t1);
	}
}

void fp48_sqr_cyc_basic(fp48_t c, fp48_t a) {
	fp8_t t0, t1, t2, t3, t4, t5, t6;

	fp8_null(t0);
	fp8_null(t1);
	fp8_null(t2);
	fp8_null(t3);
	fp8_null(t4);
	fp8_null(t5);
	fp8_null(t6);

	RLC_TRY {
		fp8_new(t0);
		fp8_new(t1);
		fp8_new(t2);
		fp8_new(t3);
		fp8_new(t4);
		fp8_new(t5);
		fp8_new(t6);

		fp8_sqr(t2, a[0][0]);
		fp8_sqr(t3, a[1][1]);
		fp8_add(t1, a[0][0], a[1][1]);

		fp8_mul_art(t0, t3);
		fp8_add(t0, t0, t2);

		fp8_sqr(t1, t1);
		fp8_sub(t1, t1, t2);
		fp8_sub(t1, t1, t3);

		fp8_sub(c[0][0], t0, a[0][0]);
		fp8_add(c[0][0], c[0][0], c[0][0]);
		fp8_add(c[0][0], t0, c[0][0]);

		fp8_add(c[1][1], t1, a[1][1]);
		fp8_add(c[1][1], c[1][1], c[1][1]);
		fp8_add(c[1][1], t1, c[1][1]);

		fp8_sqr(t0, a[0][1]);
		fp8_sqr(t1, a[1][2]);
		fp8_add(t5, a[0][1], a[1][2]);
		fp8_sqr(t2, t5);

		fp8_add(t3, t0, t1);
		fp8_sub(t5, t2, t3);

		fp8_add(t6, a[1][0], a[0][2]);
		fp8_sqr(t3, t6);
		fp8_sqr(t2, a[1][0]);

		fp8_mul_art(t6, t5);
		fp8_add(t5, t6, a[1][0]);
		fp8_dbl(t5, t5);
		fp8_add(c[1][0], t5, t6);

		fp8_mul_art(t4, t1);
		fp8_add(t5, t0, t4);
		fp8_sub(t6, t5, a[0][2]);

		fp8_sqr(t1, a[0][2]);

		fp8_dbl(t6, t6);
		fp8_add(c[0][2], t6, t5);

		fp8_mul_art(t4, t1);
		fp8_add(t5, t2, t4);
		fp8_sub(t6, t5, a[0][1]);
		fp8_dbl(t6, t6);
		fp8_add(c[0][1], t6, t5);

		fp8_add(t0, t2, t1);
		fp8_sub(t5, t3, t0);
		fp8_add(t6, t5, a[1][2]);
		fp8_dbl(t6, t6);
		fp8_add(c[1][2], t5, t6);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp8_free(t0);
		fp8_free(t1);
		fp8_free(t2);
		fp8_free(t3);
		fp8_free(t4);
		fp8_free(t5);
		fp8_free(t6);
	}
}

void fp48_sqr_pck_basic(fp48_t c, fp48_t a) {
	fp8_t t0, t1, t2, t3, t4, t5, t6;

	fp8_null(t0);
	fp8_null(t1);
	fp8_null(t2);
	fp8_null(t3);
	fp8_null(t4);
	fp8_null(t5);
	fp8_null(t6);

	RLC_TRY {
		fp8_new(t0);
		fp8_new(t1);
		fp8_new(t2);
		fp8_new(t3);
		fp8_new(t4);
		fp8_new(t5);
		fp8_new(t6);

		fp8_sqr(t0, a[0][1]);
		fp8_sqr(t1, a[1][2]);
		fp8_add(t5, a[0][1], a[1][2]);
		fp8_sqr(t2, t5);

		fp8_add(t3, t0, t1);
		fp8_sub(t5, t2, t3);

		fp8_add(t6, a[1][0], a[0][2]);
		fp8_sqr(t3, t6);
		fp8_sqr(t2, a[1][0]);

		fp8_mul_art(t6, t5);
		fp8_add(t5, t6, a[1][0]);
		fp8_dbl(t5, t5);
		fp8_add(c[1][0], t5, t6);

		fp8_mul_art(t4, t1);
		fp8_add(t5, t0, t4);
		fp8_sub(t6, t5, a[0][2]);

		fp8_sqr(t1, a[0][2]);

		fp8_dbl(t6, t6);
		fp8_add(c[0][2], t6, t5);

		fp8_mul_art(t4, t1);
		fp8_add(t5, t2, t4);
		fp8_sub(t6, t5, a[0][1]);
		fp8_dbl(t6, t6);
		fp8_add(c[0][1], t6, t5);

		fp8_add(t0, t2, t1);
		fp8_sub(t5, t3, t0);
		fp8_add(t6, t5, a[1][2]);
		fp8_dbl(t6, t6);
		fp8_add(c[1][2], t5, t6);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp8_free(t0);
		fp8_free(t1);
		fp8_free(t2);
		fp8_free(t3);
		fp8_free(t4);
		fp8_free(t5);
		fp8_free(t6);
	}
}

#endif

#if FPX_RDC == LAZYR || !defined(STRIP)

void fp48_sqr_lazyr(fp48_t c, fp48_t a) {
	/* TODO: implement lazy reduction. */
	fp48_sqr_basic(c, a);
}

void fp48_sqr_cyc_lazyr(fp48_t c, fp48_t a) {
	fp8_t t0, t1, t2, t3, t4, t5, t6;

	fp8_null(t0);
	fp8_null(t1);
	fp8_null(t2);
	fp8_null(t3);
	fp8_null(t4);
	fp8_null(t5);
	fp8_null(t6);

	RLC_TRY {
		fp8_new(t0);
		fp8_new(t1);
		fp8_new(t2);
		fp8_new(t3);
		fp8_new(t4);
		fp8_new(t5);
		fp8_new(t6);

		fp8_sqr(t2, a[0][0]);
		fp8_sqr(t3, a[1][1]);
		fp8_add(t1, a[0][0], a[1][1]);

		fp8_mul_art(t0, t3);
		fp8_add(t0, t0, t2);

		fp8_sqr(t1, t1);
		fp8_sub(t1, t1, t2);
		fp8_sub(t1, t1, t3);

		fp8_sub(c[0][0], t0, a[0][0]);
		fp8_add(c[0][0], c[0][0], c[0][0]);
		fp8_add(c[0][0], t0, c[0][0]);

		fp8_add(c[1][1], t1, a[1][1]);
		fp8_add(c[1][1], c[1][1], c[1][1]);
		fp8_add(c[1][1], t1, c[1][1]);

		fp8_sqr(t0, a[0][1]);
		fp8_sqr(t1, a[1][2]);
		fp8_add(t5, a[0][1], a[1][2]);
		fp8_sqr(t2, t5);

		fp8_add(t3, t0, t1);
		fp8_sub(t5, t2, t3);

		fp8_add(t6, a[1][0], a[0][2]);
		fp8_sqr(t3, t6);
		fp8_sqr(t2, a[1][0]);

		fp8_mul_art(t6, t5);
		fp8_add(t5, t6, a[1][0]);
		fp8_dbl(t5, t5);
		fp8_add(c[1][0], t5, t6);

		fp8_mul_art(t4, t1);
		fp8_add(t5, t0, t4);
		fp8_sub(t6, t5, a[0][2]);

		fp8_sqr(t1, a[0][2]);

		fp8_dbl(t6, t6);
		fp8_add(c[0][2], t6, t5);

		fp8_mul_art(t4, t1);
		fp8_add(t5, t2, t4);
		fp8_sub(t6, t5, a[0][1]);
		fp8_dbl(t6, t6);
		fp8_add(c[0][1], t6, t5);

		fp8_add(t0, t2, t1);
		fp8_sub(t5, t3, t0);
		fp8_add(t6, t5, a[1][2]);
		fp8_dbl(t6, t6);
		fp8_add(c[1][2], t5, t6);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp8_free(t0);
		fp8_free(t1);
		fp8_free(t2);
		fp8_free(t3);
		fp8_free(t4);
		fp8_free(t5);
		fp8_free(t6);
	}
}

void fp48_sqr_pck_lazyr(fp48_t c, fp48_t a) {
	fp8_t t0, t1, t2;
	dv8_t u0, u1, u2, u3;

	fp8_null(t0);
	fp8_null(t1);
	fp8_null(t2);
	dv8_null(u0);
	dv8_null(u1);
	dv8_null(u2);
	dv8_null(u3);

	RLC_TRY {
		fp8_new(t0);
		fp8_new(t1);
		fp8_new(t2);
		dv8_new(u0);
		dv8_new(u1);
		dv8_new(u2);
		dv8_new(u3);

		fp8_sqr_unr(u0, a[0][1]);
		fp8_sqr_unr(u1, a[1][2]);
		fp8_add(t0, a[0][1], a[1][2]);
		fp8_sqr_unr(u2, t0);

		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				fp2_addc_low(u3[i][j], u0[i][j], u1[i][j]);
				fp2_subc_low(u3[i][j], u2[i][j], u3[i][j]);
				fp2_rdcn_low(t0[i][j], u3[i][j]);
			}
		}

		fp8_add(t1, a[1][0], a[0][2]);
		fp8_sqr(t2, t1);
		fp8_sqr_unr(u2, a[1][0]);

		fp8_mul_art(t1, t0);
		fp8_add(t0, t1, a[1][0]);
		fp8_dbl(t0, t0);
		fp8_add(c[1][0], t0, t1);

		fp2_nord_low(u3[0][0], u1[1][1]);
		fp2_addc_low(u3[0][0], u0[0][0], u3[0][0]);
		fp2_addc_low(u3[0][1], u0[0][1], u1[1][0]);
		fp2_addc_low(u3[1][0], u0[1][0], u1[0][0]);
		fp2_addc_low(u3[1][1], u0[1][1], u1[0][1]);
		fp8_sqr_unr(u1, a[0][2]);
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				fp2_rdcn_low(t0[i][j], u3[i][j]);
			}
		}
		fp8_sub(t1, t0, a[0][2]);
		fp8_dbl(t1, t1);
		fp8_add(c[0][2], t1, t0);

		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				fp2_addc_low(u0[i][j], u2[i][j], u1[i][j]);
				fp2_rdcn_low(t0[i][j], u0[i][j]);
			}
		}
		fp8_sub(t0, t2, t0);
		fp8_add(t1, t0, a[1][2]);
		fp8_dbl(t1, t1);
		fp8_add(c[1][2], t0, t1);

		fp2_nord_low(u3[0][0], u1[1][1]);
		fp2_addc_low(u3[0][0], u2[0][0], u3[0][0]);
		fp2_addc_low(u3[0][1], u2[0][1], u1[1][0]);
		fp2_addc_low(u3[1][0], u2[1][0], u1[0][0]);
		fp2_addc_low(u3[1][1], u2[1][1], u1[0][1]);
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				fp2_rdcn_low(t0[i][j], u3[i][j]);
			}
		}
		fp8_sub(t1, t0, a[0][1]);
		fp8_dbl(t1, t1);
		fp8_add(c[0][1], t1, t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp8_free(t0);
		fp8_free(t1);
		fp8_free(t2);
		dv8_free(u0);
		dv8_free(u1);
		dv8_free(u2);
		dv8_free(u3);
	}
}

#endif
