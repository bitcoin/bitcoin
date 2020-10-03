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
 * Implementation of multiplication in a octodecic extension of a prime field.
 *
 * @ingroup fpx
 */

#include "relic_core.h"
#include "relic_fp_low.h"
#include "relic_fpx_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if PP_EXT == BASIC || !defined(STRIP)

void fp18_mul_basic(fp18_t c, fp18_t a, fp18_t b) {
	fp6_t t0, t1, t2, t3, t4, t5;

	fp6_null(t0);
	fp6_null(t1);
	fp6_null(t2);
	fp6_null(t3);
	fp6_null(t4);
	fp6_null(t5);

	TRY {
		fp6_new(t0);
		fp6_new(t1);
		fp6_new(t2);
		fp6_new(t3);
		fp6_new(t4);
		fp6_new(t5);

		/* Karatsuba algorithm. */

		/* t0 = a_0 * b_0. */
		fp6_mul(t0, a[0], b[0]);
		/* t1 = a_1 * b_1. */
		fp6_mul(t1, a[1], b[1]);
		/* t2 = a_2 * b_2. */
		fp6_mul(t2, a[2], b[2]);

		fp6_add(t3, a[1], a[2]);
		fp6_add(t4, b[1], b[2]);
		fp6_mul(t3, t3, t4);
		fp6_sub(t3, t3, t1);
		fp6_sub(t3, t3, t2);
		fp6_mul_art(t3, t3);
		fp6_add(t3, t3, t0);

		fp6_add(t4, a[0], a[1]);
		fp6_add(t5, b[0], b[1]);
		fp6_mul(t4, t4, t5);
		fp6_sub(t4, t4, t0);
		fp6_sub(t4, t4, t1);
		fp6_mul_art(t5, t2);
		fp6_add(c[1], t4, t5);

		fp6_add(t4, a[0], a[2]);
		fp6_add(t5, b[0], b[2]);
		fp6_mul(c[2], t4, t5);
		fp6_sub(c[2], c[2], t0);
		fp6_add(c[2], c[2], t1);
		fp6_sub(c[2], c[2], t2);

		fp6_copy(c[0], t3);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp6_free(t0);
		fp6_free(t1);
		fp6_free(t2);
		fp6_free(t3);
		fp6_free(t4);
		fp6_free(t5);
	}
}

static void fp6_mul_dxs_basic(fp6_t c, fp6_t a, fp6_t b) {
	fp3_t t0, t1, t2, t3, t4;

	fp3_null(t0);
	fp3_null(t1);
	fp3_null(t2);
	fp3_null(t3);
	fp3_null(t4);

	TRY {
		fp3_new(t0);
		fp3_new(t1);
		fp3_new(t2);
		fp3_new(t3);
		fp3_new(t4);

		fp_copy(t0[0], a[0][0]);
		fp_copy(t0[1], a[2][0]);
		fp_copy(t0[2], a[1][1]);
		fp_copy(t1[0], a[1][0]);
		fp_copy(t1[1], a[0][1]);
		fp_copy(t1[2], a[2][1]);
		fp_copy(t2[0], b[1][0]);
		fp_copy(t2[1], b[0][1]);
		fp_copy(t2[2], b[2][1]);

		fp3_mul(t3, t1, t2);
		fp3_add(t4, t0, t1);
		fp3_mul(t4, t4, t2);
		fp3_sub(t4, t4, t3);
		fp3_mul_nor(t2, t3);

		fp_copy(c[0][0], t2[0]);
		fp_copy(c[2][0], t2[1]);
		fp_copy(c[1][1], t2[2]);
		fp_copy(c[1][0], t4[0]);
		fp_copy(c[0][1], t4[1]);
		fp_copy(c[2][1], t4[2]);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp3_free(t0);
		fp3_free(t1);
		fp3_free(t2);
		fp3_free(t3);
		fp3_free(t4);
	}
}

static void fp6_mul0_basic(fp6_t c, fp6_t a, fp6_t b) {
	fp2_t v0, v1, v2, t0, t1, t2;

	fp2_null(v0);
	fp2_null(v1);
	fp2_null(v2);
	fp2_null(t0);
	fp2_null(t1);
	fp2_null(t2);

	TRY {
		fp2_new(v0);
		fp2_new(v1);
		fp2_new(v2);
		fp2_new(t0);
		fp2_new(t1);
		fp2_new(t2);

		/* v0 = a_0b_0 */
		fp2_mul(v0, a[0], b[0]);

		/* v1 = a_1b_1 */
		fp_mul(v1[0], a[1][0], b[1][0]);
		fp_mul(v1[1], a[1][1], b[1][0]);

		/* v2 = a_2b_2 */
		fp_mul(v2[0], a[2][0], b[2][1]);
		fp_mul(v2[1], a[2][1], b[2][1]);
		fp2_mul_art(v2, v2);

		/* t2 (c_0) = v0 + E((a_1 + a_2)(b_1 + b_2) - v1 - v2) */
		fp2_add(t0, a[1], a[2]);
		fp_copy(t1[0], b[1][0]);
		fp_copy(t1[1], b[2][1]);
		fp2_mul(t2, t0, t1);
		fp2_sub(t2, t2, v1);
		fp2_sub(t2, t2, v2);
		fp2_mul_nor(t0, t2);
		fp2_add(t2, t0, v0);

		/* c_1 = (a_0 + a_1)(b_0 + b_1) - v0 - v1 + Ev2 */
		fp2_add(t0, a[0], a[1]);
		fp_add(t1[0], b[0][0], b[1][0]);
		fp_copy(t1[1], b[0][1]);
		fp2_mul(c[1], t0, t1);
		fp2_sub(c[1], c[1], v0);
		fp2_sub(c[1], c[1], v1);
		fp2_mul_nor(t0, v2);
		fp2_add(c[1], c[1], t0);

		/* c_2 = (a_0 + a_2)(b_0 + b_2) - v0 + v1 - v2 */
		fp2_add(t0, a[0], a[2]);
		fp_copy(t1[0], b[0][0]);
		fp_add(t1[1], b[0][1], b[2][1]);
		fp2_mul(c[2], t0, t1);
		fp2_sub(c[2], c[2], v0);
		fp2_add(c[2], c[2], v1);
		fp2_sub(c[2], c[2], v2);

		/* c_0 = t2 */
		fp2_copy(c[0], t2);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp2_free(t2);
		fp2_free(t1);
		fp2_free(t0);
		fp2_free(v2);
		fp2_free(v1);
		fp2_free(v0);
	}
}

static void fp6_mul1_basic(fp6_t c, fp6_t a, fp6_t b) {
	fp2_t v0, v1, v2, t0, t1, t2;

	fp2_null(v0);
	fp2_null(v1);
	fp2_null(v2);
	fp2_null(t0);
	fp2_null(t1);
	fp2_null(t2);

	TRY {
		fp2_new(v0);
		fp2_new(v1);
		fp2_new(v2);
		fp2_new(t0);
		fp2_new(t1);
		fp2_new(t2);

		/* v0 = a_0b_0 */
		fp_mul(v0[0], a[0][0], b[0][0]);
		fp_mul(v0[1], a[0][1], b[0][0]);

		/* v1 = a_1b_1 */
		fp_mul(v1[0], a[1][0], b[1][1]);
		fp_mul(v1[1], a[1][1], b[1][1]);
		fp2_mul_art(v1, v1);

		/* v2 = a_2b_2 */
		fp_mul(v2[0], a[2][0], b[2][0]);
		fp_mul(v2[1], a[2][1], b[2][0]);

		/* t2 (c_0) = v0 + E((a_1 + a_2)(b_1 + b_2) - v1 - v2) */
		fp2_add(t0, a[1], a[2]);
		fp_copy(t1[0], b[2][0]);
		fp_copy(t1[1], b[1][1]);
		fp2_mul(t2, t0, t1);
		fp2_sub(t2, t2, v1);
		fp2_sub(t2, t2, v2);
		fp2_mul_nor(t0, t2);
		fp2_add(t2, t0, v0);

		/* c_1 = (a_0 + a_1)(b_0 + b_1) - v0 - v1 + Ev2 */
		fp2_add(t0, a[0], a[1]);
		fp_copy(t1[0], b[0][0]);
		fp_copy(t1[1], b[1][1]);
		fp2_mul(c[1], t0, t1);
		fp2_sub(c[1], c[1], v0);
		fp2_sub(c[1], c[1], v1);
		fp2_mul_nor(t0, v2);
		fp2_add(c[1], c[1], t0);

		/* c_2 = (a_0 + a_2)(b_0 + b_2) - v0 + v1 - v2 */
		fp2_add(t0, a[0], a[2]);
		fp_add(t1[0], b[0][0], b[2][0]);
		fp_zero(t1[1]);
		fp_mul(c[2][0], t0[0], t1[0]);
		fp_mul(c[2][1], t0[1], t1[0]);
		fp2_sub(c[2], c[2], v0);
		fp2_add(c[2], c[2], v1);
		fp2_sub(c[2], c[2], v2);

		/* c_0 = t2 */
		fp2_copy(c[0], t2);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp2_free(t2);
		fp2_free(t1);
		fp2_free(t0);
		fp2_free(v2);
		fp2_free(v1);
		fp2_free(v0);
	}
}

void fp18_mul_dxs_basic(fp18_t c, fp18_t a, fp18_t b) {
#if EP_ADD == BASIC
	fp6_t t0, t1, t2, t3, t4;

	fp6_null(t0);
	fp6_null(t1);
	fp6_null(t2);
	fp6_null(t2);
	fp6_null(t3);
	fp6_null(t4);

	TRY {
		fp6_new(t0);
		fp6_new(t1);
		fp6_new(t2);
		fp6_new(t2);
		fp6_new(t3);
		fp6_new(t4);

		/* Karatsuba algorithm. */
		/* t0 = a_0 * b_0. */
		fp6_mul0_basic(t0, a[0], b[0]);
		/* t1 = a_1 * b_1. */
		fp6_mul1_basic(t1, a[1], b[1]);
		/* t2 = a_2 * b_2 = 0. */

		fp6_mul1_basic(t2, a[2], b[1]);
		fp6_mul_art(t2, t2);
		fp6_add(t2, t2, t0);

		fp6_add(t3, a[0], a[1]);
		fp6_add(t4, b[0], b[1]);

		fp6_mul(t3, t3, t4);
		fp6_sub(t3, t3, t0);
		fp6_sub(c[1], t3, t1);

		fp6_mul0_basic(c[2], a[2], b[0]);
		fp6_add(c[2], c[2], t1);

		fp6_copy(c[0], t2);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp6_free(t0);
		fp6_free(t1);
		fp6_free(t2);
		fp6_free(t2);
		fp6_free(t3);
		fp6_free(t4);
	}
#elif EP_ADD == PROJC
	fp6_t t0, t1, t2, t3, t4;

	fp6_null(t0);
	fp6_null(t1);
	fp6_null(t2);
	fp6_null(t3);
	fp6_null(t4);

	TRY {
		fp6_new(t0);
		fp6_new(t1);
		fp6_new(t2);
		fp6_new(t3);
		fp6_new(t4);

		/* Karatsuba algorithm. */

		/* t0 = a_0 * b_0. */
		fp6_mul(t0, a[0], b[0]);
		/* t1 = a_1 * b_1. */
		fp6_mul_dxs_basic(t1, a[1], b[1]);
		/* t2 = a_2 * b_2 = 0. */

		fp6_mul_dxs_basic(t3, a[2], b[1]);
		fp6_mul_art(t2, t2);
		fp6_add(t2, t2, t0);

		fp6_add(t3, a[0], a[1]);
		fp6_add(t4, b[0], b[1]);
		fp6_mul(t3, t3, t4);
		fp6_sub(t3, t3, t0);
		fp6_sub(c[1], t3, t1);

		fp6_mul(c[2], a[2], b[0]);
		fp6_add(c[2], c[2], t1);

		fp6_copy(c[0], t2);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp6_free(t0);
		fp6_free(t1);
		fp6_free(t2);
		fp6_free(t3);
		fp6_free(t4);
	}
#endif
}

#endif

#if PP_EXT == LAZYR || !defined(STRIP)

static void fp6_mul_dxs_unr(dv6_t c, fp6_t a, fp6_t b) {
	fp3_t t0, t1, t2;
	dv3_t t3, t4, t5;

	fp3_null(t0);
	fp3_null(t1);
	fp3_null(t2);
	fp3_null(t3);
	fp3_null(t4);
	fp3_null(t5);

	TRY {
		fp3_new(t0);
		fp3_new(t1);
		fp3_new(t2);
		fp3_new(t3);
		fp3_new(t4);
		fp3_new(t5);

		fp_copy(t0[0], a[0][0]);
		fp_copy(t0[1], a[2][0]);
		fp_copy(t0[2], a[1][1]);
		fp_copy(t1[0], a[1][0]);
		fp_copy(t1[1], a[0][1]);
		fp_copy(t1[2], a[2][1]);
		fp_copy(t2[0], b[1][0]);
		fp_copy(t2[1], b[0][1]);
		fp_copy(t2[2], b[2][1]);

		fp3_muln_low(t4, t1, t2);
		fp3_add(t0, t0, t1);
		fp3_muln_low(t5, t0, t2);
		for (int i = 0; i < 3; i++) {
			fp_subc_low(t5[i], t5[i], t4[i]);
		}

		dv_zero(t3[0], 2 * FP_DIGS);
		for (int i = -1; i >= fp_prime_get_cnr(); i--) {
			fp_subc_low(t3[0], t3[0], t4[2]);
		}

		dv_copy(c[0][0], t3[0], 2 * FP_DIGS);
		dv_copy(c[2][0], t4[0], 2 * FP_DIGS);
		dv_copy(c[1][1], t4[1], 2 * FP_DIGS);
		dv_copy(c[1][0], t5[0], 2 * FP_DIGS);
		dv_copy(c[0][1], t5[1], 2 * FP_DIGS);
		dv_copy(c[2][1], t5[2], 2 * FP_DIGS);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp3_free(t0);
		fp3_free(t1);
		fp3_free(t2);
		fp3_free(t3);
		fp3_free(t4);
		fp3_free(t5);
	}
}

void fp18_mul_lazyr(fp18_t c, fp18_t a, fp18_t b) {
	dv6_t u0, u1, u2, u3, u4, u5;
	fp6_t t0, t1;

	dv6_null(u0);
	dv6_null(u1);
	dv6_null(u2);
	dv6_null(u3);
	dv6_null(u4);
	dv6_null(u5);
	fp6_null(t0);
	fp6_null(t1);

	TRY {
		dv6_new(u0);
		dv6_new(u1);
		dv6_new(u2);
		dv6_new(u3);
		dv6_new(u4);
		dv6_new(u5);
		fp6_new(t0);
		fp6_new(t1);

		/* Karatsuba algorithm. */

		/* u0 = a_0 * b_0. */
		fp6_mul_unr(u0, a[0], b[0]);
		/* u1 = a_1 * b_1. */
		fp6_mul_unr(u1, a[1], b[1]);
		/* u2 = a_2 * b_2. */
		fp6_mul_unr(u2, a[2], b[2]);

		fp6_add(t0, a[1], a[2]);
		fp6_add(t1, b[1], b[2]);
		fp6_mul_unr(u3, t0, t1);
		for (int i = 0; i < 3; i++) {
			fp2_subc_low(u3[i], u3[i], u1[i]);
			fp2_subc_low(u3[i], u3[i], u2[i]);
		}
		fp2_nord_low(u4[0], u3[2]);
		fp2_addc_low(u3[2], u3[1], u0[2]);
		fp2_addc_low(u3[1], u3[0], u0[1]);
		fp2_addc_low(u3[0], u4[0], u0[0]);

		fp6_add(t0, a[0], a[1]);
		fp6_add(t1, b[0], b[1]);
		fp6_mul_unr(u4, t0, t1);
		for (int i = 0; i < 3; i++) {
			fp2_subc_low(u4[i], u4[i], u0[i]);
			fp2_subc_low(u4[i], u4[i], u1[i]);
		}
		fp2_nord_low(u5[0], u2[2]);
		dv_copy(u5[1][0], u2[0][0], 2 * FP_DIGS);
		dv_copy(u5[1][1], u2[0][1], 2 * FP_DIGS);
		dv_copy(u5[2][0], u2[1][0], 2 * FP_DIGS);
		dv_copy(u5[2][1], u2[1][1], 2 * FP_DIGS);
		for (int i = 0; i < 3; i++) {
			fp2_addc_low(u4[i], u4[i], u5[i]);
			fp2_rdcn_low(c[1][i], u4[i]);
		}

		fp6_add(t0, a[0], a[2]);
		fp6_add(t1, b[0], b[2]);
		fp6_mul_unr(u4, t0, t1);
		for (int i = 0; i < 3; i++) {
			fp2_subc_low(u4[i], u4[i], u0[i]);
			fp2_addc_low(u4[i], u4[i], u1[i]);
			fp2_subc_low(u4[i], u4[i], u2[i]);
			fp2_rdcn_low(c[2][i], u4[i]);
			fp2_rdcn_low(c[0][i], u3[i]);
		}
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		dv6_free(u0);
		dv6_free(u1);
		dv6_free(u2);
		dv6_free(u3);
		dv6_free(u4);
		dv6_free(u5);
		fp6_free(t0);
		fp6_free(t1);
	}
}

void fp18_mul_dxs_lazyr(fp18_t c, fp18_t a, fp18_t b) {
#if EP_ADD == BASIC
	fp6_t t0, t1, t2, t3, t4;

	fp6_null(t0);
	fp6_null(t1);
	fp6_null(t2);
	fp6_null(t3);
	fp6_null(t4);

	TRY {
		fp6_new(t0);
		fp6_new(t1);
		fp6_new(t2);
		fp6_new(t3);
		fp6_new(t4);

		/* Karatsuba algorithm. */
		/* t0 = a_0 * b_0. */
		fp6_mul0_basic(t0, a[0], b[0]);
		/* t1 = a_1 * b_1. */
		fp6_mul1_basic(t1, a[1], b[1]);
		/* t2 = a_2 * b_2 = 0. */

		fp6_mul1_basic(t2, a[2], b[1]);
		fp6_mul_art(t2, t2);
		fp6_add(t2, t2, t0);

		fp6_add(t3, a[0], a[1]);
		fp6_add(t4, b[0], b[1]);

		fp6_mul(t3, t3, t4);
		fp6_sub(t3, t3, t0);
		fp6_sub(c[1], t3, t1);

		fp6_mul0_basic(c[2], a[2], b[0]);
		fp6_add(c[2], c[2], t1);

		fp6_copy(c[0], t2);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp6_free(t0);
		fp6_free(t1);
		fp6_free(t2);
		fp6_free(t3);
		fp6_free(t4);
	}
#elif EP_ADD == PROJC
	dv6_t u0, u1, u2, u3, u4, u5;
	fp6_t t0, t1;

	dv6_null(u0);
	dv6_null(u1);
	dv6_null(u2);
	dv6_null(u3);
	dv6_null(u4);
	dv6_null(u5);
	fp6_null(t0);
	fp6_null(t1);

	TRY {
		dv6_new(u0);
		dv6_new(u1);
		dv6_new(u2);
		dv6_new(u3);
		dv6_new(u4);
		dv6_new(u5);
		fp6_new(t0);
		fp6_new(t1);

		/* Karatsuba algorithm. */

		/* u0 = a_0 * b_0. */
		fp6_mul_unr(u0, a[0], b[0]);
		/* u1 = a_1 * b_1. */
		fp6_mul_dxs_unr(u1, a[1], b[1]);
		/* u2 = a_2 * b_2 = 0. */

		fp6_mul_dxs_unr(u3, a[2], b[1]);
		fp2_nord_low(u4[0], u3[2]);
		fp2_addc_low(u3[2], u3[1], u0[2]);
		fp2_addc_low(u3[1], u3[0], u0[1]);
		fp2_addc_low(u3[0], u4[0], u0[0]);

		fp6_add(t0, a[0], a[1]);
		fp6_add(t1, b[0], b[1]);
		fp6_mul_unr(u4, t0, t1);
		for (int i = 0; i < 3; i++) {
			fp2_subc_low(u4[i], u4[i], u0[i]);
			fp2_subc_low(u4[i], u4[i], u1[i]);
		}
		for (int i = 0; i < 3; i++) {
			fp2_rdcn_low(c[1][i], u4[i]);
		}

		fp6_mul_unr(u4, a[2], b[0]);
		for (int i = 0; i < 3; i++) {
			fp2_addc_low(u4[i], u4[i], u1[i]);
			fp2_rdcn_low(c[2][i], u4[i]);
			fp2_rdcn_low(c[0][i], u3[i]);
		}
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		dv6_free(u0);
		dv6_free(u1);
		dv6_free(u2);
		dv6_free(u3);
		dv6_free(u4);
		dv6_free(u5);
		fp6_free(t0);
		fp6_free(t1);
	}
#endif
}

#endif
