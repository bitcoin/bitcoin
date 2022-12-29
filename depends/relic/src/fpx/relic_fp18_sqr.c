/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2012 RELIC Authors
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

#if FPX_RDC == BASIC || !defined(STRIP)

void fp18_sqr_basic(fp18_t c, fp18_t a) {
	fp9_t t0, t1;

	fp9_null(t0);
	fp9_null(t1);

	RLC_TRY {
		fp9_new(t0);
		fp9_new(t1);

		fp9_add(t0, a[0], a[1]);
		fp9_mul_art(t1, a[1]);
		fp9_add(t1, a[0], t1);
		fp9_mul(t0, t0, t1);
		fp9_mul(c[1], a[0], a[1]);
		fp9_sub(c[0], t0, c[1]);
		fp9_mul_art(t1, c[1]);
		fp9_sub(c[0], c[0], t1);
		fp9_dbl(c[1], c[1]);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp9_free(t0);
		fp9_free(t1);
	}
}

#endif

#if FPX_RDC == LAZYR || !defined(STRIP)

void fp18_sqr_unr(dv18_t c, fp18_t a) {
	fp9_t t;
	dv9_t u0, u1, u2;

	fp9_null(t);
	dv9_null(u0);
	dv9_null(u1);
	dv9_null(u2);

	RLC_TRY {
		fp9_new(t);
		dv9_new(u0);
		dv9_new(u1);
		dv9_new(u2);

		/* t0 = a^2. */
		fp9_sqr_unr(u0, a[0]);
		/* t1 = b^2. */
		fp9_sqr_unr(u1, a[1]);

		fp9_add(t, a[0], a[1]);

		/* c = a^2 + b^2 * E. */
		dv_copy(u2[1][0], u1[0][0], 2 * RLC_FP_DIGS);
		dv_copy(u2[1][1], u1[0][1], 2 * RLC_FP_DIGS);
		dv_copy(u2[1][2], u1[0][2], 2 * RLC_FP_DIGS);
		dv_copy(u2[2][0], u1[1][0], 2 * RLC_FP_DIGS);
		dv_copy(u2[2][1], u1[1][1], 2 * RLC_FP_DIGS);
		dv_copy(u2[2][2], u1[1][2], 2 * RLC_FP_DIGS);
		fp3_nord_low(u2[0], u1[2]);
		fp3_addc_low(c[0][0], u2[0], u0[0]);
		fp3_addc_low(c[0][1], u2[1], u0[1]);
		fp3_addc_low(c[0][2], u2[2], u0[2]);

		/* d = (a + b)^2 - a^2 - b^2 = 2 * a * b. */
		fp3_addc_low(u1[0], u1[0], u0[0]);
		fp3_addc_low(u1[1], u1[1], u0[1]);
		fp3_addc_low(u1[2], u1[2], u0[2]);

		fp9_sqr_unr(u0, t);
		fp3_subc_low(c[1][0], u0[0], u1[0]);
		fp3_subc_low(c[1][1], u0[1], u1[1]);
		fp3_subc_low(c[1][2], u0[2], u1[2]);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp9_free(t);
		dv9_free(u0);
		dv9_free(u1);
		dv9_free(u2);
	}
}

void fp18_sqr_lazyr(fp18_t c, fp18_t a) {
	dv18_t t;

	dv18_null(t);

	RLC_TRY {
		dv18_new(t);
		fp18_sqr_unr(t, a);
		for (int i = 0; i < 3; i++) {
			fp3_rdcn_low(c[0][i], t[0][i]);
			fp3_rdcn_low(c[1][i], t[1][i]);
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		dv18_free(t);
	}
}

#endif
