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
 * Implementation of squaring in a nonic extension of a prime field.
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

void fp9_sqr_basic(fp9_t c, fp9_t a) {
	fp3_t t0, t1, t2, t3, t4;

	fp3_null(t0);
	fp3_null(t1);
	fp3_null(t2);
	fp3_null(t3);
	fp3_null(t4);

	RLC_TRY {
		fp3_new(t0);
		fp3_new(t1);
		fp3_new(t2);
		fp3_new(t3);
		fp3_new(t4);

		/* t0 = a_0^2 */
		fp3_sqr(t0, a[0]);

		/* t1 = 2 * a_1 * a_2 */
		fp3_mul(t1, a[1], a[2]);
		fp3_dbl(t1, t1);

		/* t2 = a_2^2. */
		fp3_sqr(t2, a[2]);

		/* c2 = a_0 + a_2. */
		fp3_add(c[2], a[0], a[2]);

		/* t3 = (a_0 + a_2 + a_1)^2. */
		fp3_add(t3, c[2], a[1]);
		fp3_sqr(t3, t3);

		/* c2 = (a_0 + a_2 - a_1)^2. */
		fp3_sub(c[2], c[2], a[1]);
		fp3_sqr(c[2], c[2]);

		/* c2 = (c2 + t3)/2. */
		fp3_add(c[2], c[2], t3);
		fp_hlv(c[2][0], c[2][0]);
		fp_hlv(c[2][1], c[2][1]);
		fp_hlv(c[2][2], c[2][2]);

		/* t3 = t3 - c2 - t1. */
		fp3_sub(t3, t3, c[2]);
		fp3_sub(t3, t3, t1);

		/* c2 = c2 - t0 - t2. */
		fp3_sub(c[2], c[2], t0);
		fp3_sub(c[2], c[2], t2);

		/* c0 = t0 + t1 * E. */
		fp3_mul_nor(t4, t1);
		fp3_add(c[0], t0, t4);

		/* c1 = t3 + t2 * E. */
		fp3_mul_nor(t4, t2);
		fp3_add(c[1], t3, t4);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp3_free(t0);
		fp3_free(t1);
		fp3_free(t2);
		fp3_free(t3);
		fp3_free(t4);
	}
}

#endif

#if PP_EXT == LAZYR || !defined(STRIP)

void fp9_sqr_unr(dv9_t c, fp9_t a) {
	dv3_t u0, u1, u2, u3, u4, u5;
	fp3_t t0, t1, t2, t3;

	dv3_null(u0);
	dv3_null(u1);
	dv3_null(u2);
	dv3_null(u3);
	dv3_null(u4);
	dv3_null(u5);
	fp3_null(t0);
	fp3_null(t1);
	fp3_null(t2);
	fp3_null(t3);

	RLC_TRY {
		dv3_new(u0);
		dv3_new(u1);
		dv3_new(u2);
		dv3_new(u3);
		dv3_new(u4);
		dv3_new(u5);
		fp3_new(t0);
		fp3_new(t1);
		fp3_new(t2);
		fp3_new(t3);

		/* u0 = a_0^2 */
		fp3_sqrn_low(u0, a[0]);

		/* t1 = 2 * a_1 * a_2 */
		fp3_dblm_low(t0, a[1]);
		fp3_muln_low(u1, t0, a[2]);

		/* u2 = a_2^2. */
		fp3_sqrn_low(u2, a[2]);

		/* t4 = a_0 + a_2. */
		fp3_addm_low(t3, a[0], a[2]);

		/* u3 = (a_0 + a_2 + a_1)^2. */
		fp3_addm_low(t2, t3, a[1]);
		fp3_sqrn_low(u3, t2);

		/* u4 = (a_0 + a_2 - a_1)^2. */
		fp3_subm_low(t1, t3, a[1]);
		fp3_sqrn_low(u4, t1);

		/* u4 = (u4 + u3)/2. */
#ifdef RLC_FP_ROOM
		fp3_addc_low(u4, u4, u3);
#else
		fp3_addc_low(u4, u4, u3);
#endif
		fp_hlvd_low(u4[0], u4[0]);
		fp_hlvd_low(u4[1], u4[1]);
		fp_hlvd_low(u4[2], u4[2]);

		/* u3 = u3 - u4 - u1. */
		fp3_addc_low(u5, u1, u4);
		fp3_subc_low(u3, u3, u5);

		/* c2 = u4 - u0 - u2. */
		fp3_addc_low(u5, u0, u2);
		fp3_subc_low(c[2], u4, u5);

		/* c0 = u0 + u1 * E. */
		fp3_nord_low(u4, u1);
		fp3_addc_low(c[0], u0, u4);

		/* c1 = u3 + u2 * E. */
		fp3_nord_low(u4, u2);
		fp3_addc_low(c[1], u3, u4);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		dv3_free(u0);
		dv3_free(u1);
		dv3_free(u2);
		dv3_free(u3);
		dv3_free(u4);
		dv3_free(u5);
		fp3_free(t0);
		fp3_free(t1);
		fp3_free(t2);
		fp3_free(t3);
	}
}

void fp9_sqr_lazyr(fp9_t c, fp9_t a) {
	dv9_t t;

	dv9_null(t);

	RLC_TRY {
		dv9_new(t);
		fp9_sqr_unr(t, a);
		fp3_rdcn_low(c[0], t[0]);
		fp3_rdcn_low(c[1], t[1]);
		fp3_rdcn_low(c[2], t[2]);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		dv9_free(t);
	}
}

#endif
