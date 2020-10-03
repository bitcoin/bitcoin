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
 * Implementation of squaring in a octodecic extension of a prime field.
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

void fp18_sqr_basic(fp18_t c, fp18_t a) {
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

		/* t0 = a_0^2 */
		fp6_sqr(t0, a[0]);

		/* t1 = 2 * a_1 * a_2 */
		fp6_mul(t1, a[1], a[2]);
		fp6_dbl(t1, t1);

		/* t2 = a_2^2. */
		fp6_sqr(t2, a[2]);

		/* c_2 = a_0 + a_2. */
		fp6_add(c[2], a[0], a[2]);

		/* t3 = (a_0 + a_2 + a_1)^2. */
		fp6_add(t3, c[2], a[1]);
		fp6_sqr(t3, t3);

		/* c_2 = (a_0 + a_2 - a_1)^2. */
		fp6_sub(c[2], c[2], a[1]);
		fp6_sqr(c[2], c[2]);

		/* c_2 = (c_2 + t3)/2. */
		fp6_add(c[2], c[2], t3);
		for (int i = 0; i < 2; i++) {
			fp_hlv(c[2][0][i], c[2][0][i]);
			fp_hlv(c[2][1][i], c[2][1][i]);
			fp_hlv(c[2][2][i], c[2][2][i]);
		}

		/* t3 = t3 - c_2 - t1. */
		fp6_sub(t3, t3, c[2]);
		fp6_sub(t3, t3, t1);

		/* c_2 = c_2 - t0 - t2. */
		fp6_sub(c[2], c[2], t0);
		fp6_sub(c[2], c[2], t2);

		/* c_0 = t0 + t1 * E. */
		fp6_mul_art(t4, t1);
		fp6_add(c[0], t0, t4);

		/* c_1 = t3 + t2 * E. */
		fp6_mul_art(t4, t2);
		fp6_add(c[1], t3, t4);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp6_free(t0);
		fp6_free(t1);
		fp6_free(t2);
		fp6_free(t3);
		fp6_free(t4);
	}
}

#endif

#if PP_EXT == LAZYR || !defined(STRIP)

void fp18_sqr_lazyr(fp18_t c, fp18_t a) {
	dv6_t u0, u1, u2, u3, u4;
	fp6_t t0;

	dv6_null(u0);
	dv6_null(u1);
	dv6_null(u2);
	dv6_null(u3);
	dv6_null(u4);
	fp6_null(t0);

	TRY {
		dv6_new(u0);
		dv6_new(u1);
		dv6_new(u2);
		dv6_new(u3);
		dv6_new(u4);
		fp6_new(t0);

		/* u0 = a_0^2 */
		fp6_sqr_unr(u0, a[0]);

		/* u1 = 2 * a_1 * a_2 */
		fp6_mul_unr(u1, a[1], a[2]);
		for (int i = 0; i < 3; i++) {
			fp2_addc_low(u1[i], u1[i], u1[i]);
		}

		/* t2 = a_2^2. */
		fp6_sqr_unr(u2, a[2]);

		/* c_2 = a_0 + a_2. */
		fp6_add(c[2], a[0], a[2]);

		/* u3 = (a_0 + a_2 + a_1)^2. */
		fp6_add(t0, c[2], a[1]);
		fp6_sqr_unr(u3, t0);

		/* u4 = (a_0 + a_2 - a_1)^2. */
		fp6_sub(c[2], c[2], a[1]);
		fp6_sqr_unr(u4, c[2]);

		/* u4 = (c_2 + t3)/2. */
		for (int i = 0; i < 3; i++) {
			fp2_addc_low(u4[i], u4[i], u3[i]);
			fp_hlvd_low(u4[i][0], u4[i][0]);
			fp_hlvd_low(u4[i][1], u4[i][1]);
		}

		/* t3 = t3 - u4 - t1. */
		for (int i = 0; i < 3; i++) {
			fp2_subc_low(u3[i], u3[i], u4[i]);
			fp2_subc_low(u3[i], u3[i], u1[i]);
		}

		/* c_2 = u4 - t0 - t2. */
		for (int i = 0; i < 3; i++) {
			fp2_subc_low(u4[i], u4[i], u0[i]);
			fp2_subc_low(u4[i], u4[i], u2[i]);
			fp2_rdcn_low(c[2][i], u4[i]);
		}

		/* c_0 = t0 + t1 * E. */
		fp2_nord_low(u4[0], u1[2]);
		fp2_addc_low(u1[2], u4[0], u0[0]);
		fp2_rdcn_low(c[0][0], u1[2]);
		fp2_addc_low(u1[2], u1[0], u0[1]);
		fp2_rdcn_low(c[0][1], u1[2]);
		fp2_addc_low(u1[2], u1[1], u0[2]);
		fp2_rdcn_low(c[0][2], u1[2]);

		/* c_1 = t3 + t2 * E. */
		fp2_nord_low(u4[0], u2[2]);
		fp2_addc_low(u1[0], u4[0], u3[0]);
		fp2_rdcn_low(c[1][0], u1[0]);
		fp2_addc_low(u1[1], u2[0], u3[1]);
		fp2_rdcn_low(c[1][1], u1[1]);
		fp2_addc_low(u1[2], u2[1], u3[2]);
		fp2_rdcn_low(c[1][2], u1[2]);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		dv6_free(u0);
		dv6_free(u1);
		dv6_free(u2);
		dv6_free(u3);
		dv6_free(u4);
		fp6_free(t0);
	}
}

#endif

void fp18_sqr_cyc(fp18_t c, fp18_t a) {
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

		fp6_sqr(t0, a[0]);
		fp6_sqr(t1, a[2]);
		fp6_sqr(t2, a[1]);

		fp6_add(t3, t0, t0);
		fp6_add(t3, t3, t0);
		fp6_copy(t4, a[0]);
		fp_neg(t4[1][0], t4[1][0]);
		fp_neg(t4[0][1], t4[0][1]);
		fp_neg(t4[2][1], t4[2][1]);
		fp6_add(t4, t4, t4);
		fp6_sub(c[0], t3, t4);

		fp6_add(t3, t1, t1);
		fp6_add(t3, t3, t1);
		fp6_mul_art(t3, t3);
		fp6_copy(t4, a[1]);
		fp_neg(t4[1][0], t4[1][0]);
		fp_neg(t4[0][1], t4[0][1]);
		fp_neg(t4[2][1], t4[2][1]);
		fp6_add(t4, t4, t4);
		fp6_add(c[1], t3, t4);

		fp6_add(t3, t2, t2);
		fp6_add(t3, t3, t2);
		fp6_copy(t4, a[2]);
		fp_neg(t4[1][0], t4[1][0]);
		fp_neg(t4[0][1], t4[0][1]);
		fp_neg(t4[2][1], t4[2][1]);
		fp6_add(t4, t4, t4);
		fp6_sub(c[2], t3, t4);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp6_free(t0);
		fp6_free(t1);
		fp6_free(t2);
		fp6_free(t3);
		fp6_free(t4);
	}
}

void fp18_sqr_pck(fp18_t c, fp18_t a) {
	fp6_t t1, t2, t3, t4;

	fp6_null(t1);
	fp6_null(t2);
	fp6_null(t3);
	fp6_null(t4);

	TRY {
		fp6_new(t1);
		fp6_new(t2);
		fp6_new(t3);
		fp6_new(t4);

		fp6_sqr(t1, a[2]);
		fp6_sqr(t2, a[1]);

		fp6_add(t3, t1, t1);
		fp6_add(t3, t3, t1);
		fp6_mul_art(t3, t3);
		fp6_copy(t4, a[1]);
		fp_neg(t4[1][0], t4[1][0]);
		fp_neg(t4[0][1], t4[0][1]);
		fp_neg(t4[2][1], t4[2][1]);
		fp6_add(t4, t4, t4);
		fp6_add(c[1], t3, t4);

		fp6_add(t3, t2, t2);
		fp6_add(t3, t3, t2);
		fp6_copy(t4, a[2]);
		fp_neg(t4[1][0], t4[1][0]);
		fp_neg(t4[0][1], t4[0][1]);
		fp_neg(t4[2][1], t4[2][1]);
		fp6_add(t4, t4, t4);
		fp6_sub(c[2], t3, t4);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp6_free(t1);
		fp6_free(t2);
		fp6_free(t3);
		fp6_free(t4);
	}
}
