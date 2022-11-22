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
 * Implementation of multiplication in an octic extension of a prime field.
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

void fp8_mul_basic(fp8_t c, fp8_t a, fp8_t b) {
	fp4_t t0, t1, t4;

	fp4_null(t0);
	fp4_null(t1);
	fp4_null(t4);

	RLC_TRY {
		fp4_new(t0);
		fp4_new(t1);
		fp4_new(t4);

		/* Karatsuba algorithm. */

		/* t0 = a_0 * b_0. */
		fp4_mul(t0, a[0], b[0]);
		/* t1 = a_1 * b_1. */
		fp4_mul(t1, a[1], b[1]);
		/* t4 = b_0 + b_1. */
		fp4_add(t4, b[0], b[1]);

		/* c_1 = a_0 + a_1. */
		fp4_add(c[1], a[0], a[1]);

		/* c_1 = (a_0 + a_1) * (b_0 + b_1) */
		fp4_mul(c[1], c[1], t4);
		fp4_sub(c[1], c[1], t0);
		fp4_sub(c[1], c[1], t1);

		/* c_0 = a_0b_0 + v * a_1b_1. */
		fp4_mul_art(t4, t1);
		fp4_add(c[0], t0, t4);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp4_free(t0);
		fp4_free(t1);
		fp4_free(t4);
	}
}

#endif

#if PP_EXT == LAZYR || !defined(STRIP)

static void fp4_mul_dxs_unr(dv4_t c, fp4_t a, fp4_t b) {
	fp2_t t0, t1;
	dv2_t u0, u1;

	fp2_null(t0);
	fp2_null(t1);
	dv2_null(u0);
	dv2_null(u1);

	RLC_TRY {
		fp2_new(t0);
		fp2_new(t1);
		dv2_new(u0);
		dv2_new(u1);

#ifdef RLC_FP_ROOM
		fp2_mulc_low(u1, a[1], b[1]);
		fp2_addn_low(t0, b[0], b[1]);
		fp2_addn_low(t1, a[0], a[1]);
#else
		fp2_muln_low(u1, a[1], b[1]);
		fp2_addm_low(t0, b[0], b[1]);
		fp2_addm_low(t1, a[0], a[1]);
#endif
		fp2_muln_low(c[1], t1, t0);
		fp2_subc_low(c[1], c[1], u1);
		fp2_norh_low(c[0], u1);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp2_free(t0);
		dv2_free(t1);
		dv2_free(u0);
		dv2_free(u1);
	}
}

void fp8_mul_dxs(fp8_t c, fp8_t a, fp8_t b) {
	fp4_t t0, t1;
	dv4_t u0, u1, u2, u3;

	fp4_null(t0);
	fp4_null(t1);
	dv4_null(u0);
	dv4_null(u1);
	dv4_null(u2);
	dv4_null(u3);

	RLC_TRY {
		fp4_new(t0);
		fp4_new(t1);
		dv4_new(u0);
		dv4_new(u1);
		dv4_new(u2);
		dv4_new(u3);

		/* Karatsuba algorithm. */

		/* u0 = a_0 * b_0. */
		fp4_mul_unr(u0, a[0], b[0]);
		/* u1 = a_1 * b_1. */
		fp4_mul_dxs_unr(u1, a[1], b[1]);

		/* t1 = a_0 + a_1. */
		fp4_add(t0, a[0], a[1]);
		/* t0 = b_0 + b_1. */
		fp4_add(t1, b[0], b[1]);
		/* u2 = (a_0 + a_1) * (b_0 + b_1) */
		fp4_mul_unr(u2, t0, t1);
		/* c_1 = u2 - a_0b_0 - a_1b_1. */
		for (int i = 0; i < 2; i++) {
			fp2_addc_low(u3[i], u0[i], u1[i]);
			fp2_subc_low(u2[i], u2[i], u3[i]);
			fp2_rdcn_low(c[1][i], u2[i]);
		}
		/* c_0 = a_0b_0 + v * a_1b_1. */
		fp2_nord_low(u2[0], u1[1]);
		dv_copy(u2[1][0], u1[0][0], 2 * RLC_FP_DIGS);
		dv_copy(u2[1][1], u1[0][1], 2 * RLC_FP_DIGS);
		for (int i = 0; i < 2; i++) {
			fp2_addc_low(u2[i], u0[i], u2[i]);
			fp2_rdcn_low(c[0][i], u2[i]);
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp4_free(t0);
		dv4_free(t1);
		dv4_free(u0);
		dv4_free(u1);
		dv4_free(u2);
		dv4_free(u3);
	}
}

void fp8_mul_unr(dv8_t c, fp8_t a, fp8_t b) {
	fp4_t t0, t1;
	dv4_t u0, u1, u2, u3;

	fp4_null(t0);
	fp4_null(t1);
	dv4_null(u0);
	dv4_null(u1);
	dv4_null(u2);
	dv4_null(u3);

	RLC_TRY {
		fp4_new(t0);
		fp4_new(t1);
		dv4_new(u0);
		dv4_new(u1);
		dv4_new(u2);
		dv4_new(u3);

		/* Karatsuba algorithm. */

		/* u0 = a_0 * b_0. */
		fp4_mul_unr(u0, a[0], b[0]);
		/* u1 = a_1 * b_1. */
		fp4_mul_unr(u1, a[1], b[1]);
		/* t1 = a_0 + a_1. */
		fp4_add(t0, a[0], a[1]);
		/* t0 = b_0 + b_1. */
		fp4_add(t1, b[0], b[1]);
		/* u2 = (a_0 + a_1) * (b_0 + b_1) */
		fp4_mul_unr(u2, t0, t1);
		/* c_1 = u2 - a_0b_0 - a_1b_1. */
		for (int i = 0; i < 2; i++) {
			fp2_addc_low(u3[i], u0[i], u1[i]);
			fp2_subc_low(c[1][i], u2[i], u3[i]);
		}
		/* c_0 = a_0b_0 + v * a_1b_1. */
		fp2_nord_low(u2[0], u1[1]);
		dv_copy(u2[1][0], u1[0][0], 2 * RLC_FP_DIGS);
		dv_copy(u2[1][1], u1[0][1], 2 * RLC_FP_DIGS);
		for (int i = 0; i < 2; i++) {
			fp2_addc_low(c[0][i], u0[i], u2[i]);
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp4_free(t0);
		fp4_free(t1);
		dv4_free(u0);
		dv4_free(u1);
		dv4_free(u2);
		dv4_free(u3);
	}
}

void fp8_mul_lazyr(fp8_t c, fp8_t a, fp8_t b) {
	dv8_t t;

	dv8_null(t);

	RLC_TRY {
		dv8_new(t);
		fp8_mul_unr(t, a, b);
		fp2_rdcn_low(c[0][0], t[0][0]);
		fp2_rdcn_low(c[0][1], t[0][1]);
		fp2_rdcn_low(c[1][0], t[1][0]);
		fp2_rdcn_low(c[1][1], t[1][1]);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		dv8_free(t);
	}
}

#endif

void fp8_mul_art(fp8_t c, fp8_t a) {
	fp4_t t0;

	fp4_null(t0);

	RLC_TRY {
		fp4_new(t0);

		/* (a_0 + a_1 * v) * v = a_0 * v + a_1 * v^4 */
		fp4_copy(t0, a[0]);
		fp4_mul_art(c[0], a[1]);
		fp4_copy(c[1], t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp4_free(t0);
	}
}
