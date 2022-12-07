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

void fp12_sqr_basic(fp12_t c, fp12_t a) {
	fp6_t t0, t1;

	fp6_null(t0);
	fp6_null(t1);

	RLC_TRY {
		fp6_new(t0);
		fp6_new(t1);

		fp6_add(t0, a[0], a[1]);
		fp6_mul_art(t1, a[1]);
		fp6_add(t1, a[0], t1);
		fp6_mul(t0, t0, t1);
		fp6_mul(c[1], a[0], a[1]);
		fp6_sub(c[0], t0, c[1]);
		fp6_mul_art(t1, c[1]);
		fp6_sub(c[0], c[0], t1);
		fp6_dbl(c[1], c[1]);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp6_free(t0);
		fp6_free(t1);
	}
}

void fp12_sqr_cyc_basic(fp12_t c, fp12_t a) {
	fp2_t t0, t1, t2, t3, t4, t5, t6;

	fp2_null(t0);
	fp2_null(t1);
	fp2_null(t2);
	fp2_null(t3);
	fp2_null(t4);
	fp2_null(t5);
	fp2_null(t6);

	RLC_TRY {
		fp2_new(t0);
		fp2_new(t1);
		fp2_new(t2);
		fp2_new(t3);
		fp2_new(t4);
		fp2_new(t5);
		fp2_new(t6);

		/* Define z = sqrt(E) */

		/* Now a is seen as (t0,t1) + (t2,t3) * w + (t4,t5) * w^2 */

		/* (t0, t1) = (a00 + a11*z)^2. */
		fp2_sqr(t2, a[0][0]);
		fp2_sqr(t3, a[1][1]);
		fp2_add(t1, a[0][0], a[1][1]);

		fp2_mul_nor(t0, t3);
		fp2_add(t0, t0, t2);

		fp2_sqr(t1, t1);
		fp2_sub(t1, t1, t2);
		fp2_sub(t1, t1, t3);

		fp2_sub(c[0][0], t0, a[0][0]);
		fp2_add(c[0][0], c[0][0], c[0][0]);
		fp2_add(c[0][0], t0, c[0][0]);

		fp2_add(c[1][1], t1, a[1][1]);
		fp2_add(c[1][1], c[1][1], c[1][1]);
		fp2_add(c[1][1], t1, c[1][1]);

		fp2_sqr(t0, a[0][1]);
		fp2_sqr(t1, a[1][2]);
		fp2_add(t5, a[0][1], a[1][2]);
		fp2_sqr(t2, t5);

		fp2_add(t3, t0, t1);
		fp2_sub(t5, t2, t3);

		fp2_add(t6, a[1][0], a[0][2]);
		fp2_sqr(t3, t6);
		fp2_sqr(t2, a[1][0]);

		fp2_mul_nor(t6, t5);
		fp2_add(t5, t6, a[1][0]);
		fp2_dbl(t5, t5);
		fp2_add(c[1][0], t5, t6);

		fp2_mul_nor(t4, t1);
		fp2_add(t5, t0, t4);
		fp2_sub(t6, t5, a[0][2]);

		fp2_sqr(t1, a[0][2]);

		fp2_dbl(t6, t6);
		fp2_add(c[0][2], t6, t5);

		fp2_mul_nor(t4, t1);
		fp2_add(t5, t2, t4);
		fp2_sub(t6, t5, a[0][1]);
		fp2_dbl(t6, t6);
		fp2_add(c[0][1], t6, t5);

		fp2_add(t0, t2, t1);
		fp2_sub(t5, t3, t0);
		fp2_add(t6, t5, a[1][2]);
		fp2_dbl(t6, t6);
		fp2_add(c[1][2], t5, t6);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp2_free(t0);
		fp2_free(t1);
		fp2_free(t2);
		fp2_free(t3);
		fp2_free(t4);
		fp2_free(t5);
		fp2_free(t6);
	}
}

void fp12_sqr_pck_basic(fp12_t c, fp12_t a) {
	fp2_t t0, t1, t2, t3, t4, t5, t6;

	fp2_null(t0);
	fp2_null(t1);
	fp2_null(t2);
	fp2_null(t3);
	fp2_null(t4);
	fp2_null(t5);
	fp2_null(t6);

	RLC_TRY {
		fp2_new(t0);
		fp2_new(t1);
		fp2_new(t2);
		fp2_new(t3);
		fp2_new(t4);
		fp2_new(t5);
		fp2_new(t6);

		fp2_sqr(t0, a[0][1]);
		fp2_sqr(t1, a[1][2]);
		fp2_add(t5, a[0][1], a[1][2]);
		fp2_sqr(t2, t5);

		fp2_add(t3, t0, t1);
		fp2_sub(t5, t2, t3);

		fp2_add(t6, a[1][0], a[0][2]);
		fp2_sqr(t3, t6);
		fp2_sqr(t2, a[1][0]);

		fp2_mul_nor(t6, t5);
		fp2_add(t5, t6, a[1][0]);
		fp2_dbl(t5, t5);
		fp2_add(c[1][0], t5, t6);

		fp2_mul_nor(t4, t1);
		fp2_add(t5, t0, t4);
		fp2_sub(t6, t5, a[0][2]);

		fp2_sqr(t1, a[0][2]);

		fp2_dbl(t6, t6);
		fp2_add(c[0][2], t6, t5);

		fp2_mul_nor(t4, t1);
		fp2_add(t5, t2, t4);
		fp2_sub(t6, t5, a[0][1]);
		fp2_dbl(t6, t6);
		fp2_add(c[0][1], t6, t5);

		fp2_add(t0, t2, t1);
		fp2_sub(t5, t3, t0);
		fp2_add(t6, t5, a[1][2]);
		fp2_dbl(t6, t6);
		fp2_add(c[1][2], t5, t6);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp2_free(t0);
		fp2_free(t1);
		fp2_free(t2);
		fp2_free(t3);
		fp2_free(t4);
		fp2_free(t5);
		fp2_free(t6);
	}
}

#endif

#if FPX_RDC == LAZYR || !defined(STRIP)

void fp12_sqr_unr(dv12_t c, fp12_t a) {
	fp4_t t0, t1;
	dv4_t u0, u1, u2, u3, u4;

	fp4_null(t0);
	fp4_null(t1);
	dv4_null(u0);
	dv4_null(u1);
	dv4_null(u2);
	dv4_null(u3);
	dv4_null(u4);

	RLC_TRY {
		fp4_new(t0);
		fp4_new(t1);
		dv4_new(u0);
		dv4_new(u1);
		dv4_new(u2);
		dv4_new(u3);
		dv4_new(u4);

		/* a0 = (a00, a11). */
		/* a1 = (a10, a02). */
		/* a2 = (a01, a12). */

		/* (t0,t1) = a0^2 */
		fp2_copy(t0[0], a[0][0]);
		fp2_copy(t0[1], a[1][1]);
		fp4_sqr_unr(u0, t0);

		/* (t2,t3) = 2 * a1 * a2 */
		fp2_copy(t0[0], a[1][0]);
		fp2_copy(t0[1], a[0][2]);
		fp2_copy(t1[0], a[0][1]);
		fp2_copy(t1[1], a[1][2]);
		fp4_mul_unr(u1, t0, t1);
		fp2_addc_low(u1[0], u1[0], u1[0]);
		fp2_addc_low(u1[1], u1[1], u1[1]);

		/* (t4,t5) = a2^2. */
		fp4_sqr_unr(u2, t1);

		/* c2 = a0 + a2. */
		fp2_addm_low(t1[0], a[0][0], a[0][1]);
		fp2_addm_low(t1[1], a[1][1], a[1][2]);

		/* (t6,t7) = (a0 + a2 + a1)^2. */
		fp2_addm_low(t0[0], t1[0], a[1][0]);
		fp2_addm_low(t0[1], t1[1], a[0][2]);
		fp4_sqr_unr(u3, t0);

		/* c2 = (a0 + a2 - a1)^2. */
		fp2_subm_low(t0[0], t1[0], a[1][0]);
		fp2_subm_low(t0[1], t1[1], a[0][2]);
		fp4_sqr_unr(u4, t0);

		/* c2 = (c2 + (t6,t7))/2. */
#ifdef RLC_FP_ROOM
		fp2_addd_low(u4[0], u4[0], u3[0]);
		fp2_addd_low(u4[1], u4[1], u3[1]);
#else
		fp2_addc_low(u4[0], u4[0], u3[0]);
		fp2_addc_low(u4[1], u4[1], u3[1]);
#endif
		fp_hlvd_low(u4[0][0], u4[0][0]);
		fp_hlvd_low(u4[0][1], u4[0][1]);
		fp_hlvd_low(u4[1][0], u4[1][0]);
		fp_hlvd_low(u4[1][1], u4[1][1]);

		/* (t6,t7) = (t6,t7) - c2 - (t2,t3). */
		fp2_subc_low(u3[0], u3[0], u4[0]);
		fp2_subc_low(u3[1], u3[1], u4[1]);
		fp2_subc_low(u3[0], u3[0], u1[0]);
		fp2_subc_low(u3[1], u3[1], u1[1]);

		/* c2 = c2 - (t0,t1) - (t4,t5). */
		fp2_subc_low(u4[0], u4[0], u0[0]);
		fp2_subc_low(u4[1], u4[1], u0[1]);
		fp2_subc_low(c[0][1], u4[0], u2[0]);
		fp2_subc_low(c[1][2], u4[1], u2[1]);

		/* c1 = (t6,t7) + (t4,t5) * E. */
		fp2_nord_low(u4[1], u2[1]);
		fp2_addc_low(c[1][0], u3[0], u4[1]);
		fp2_addc_low(c[0][2], u3[1], u2[0]);

		/* c0 = (t0,t1) + (t2,t3) * E. */
		fp2_nord_low(u4[1], u1[1]);
		fp2_addc_low(c[0][0], u0[0], u4[1]);
		fp2_addc_low(c[1][1], u0[1], u1[0]);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp4_free(t0);
		fp4_free(t1);
		dv4_free(u0);
		dv4_free(u1);
		dv4_free(u2);
		dv4_free(u3);
		dv4_free(u4);
	}
}

void fp12_sqr_lazyr(fp12_t c, fp12_t a) {
	dv12_t t;

	dv12_null(t);

	RLC_TRY {
		dv12_new(t);
		fp12_sqr_unr(t, a);
		for (int i = 0; i < 3; i++) {
			fp2_rdcn_low(c[0][i], t[0][i]);
			fp2_rdcn_low(c[1][i], t[1][i]);
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		dv12_free(t);
	}
}

void fp12_sqr_cyc_lazyr(fp12_t c, fp12_t a) {
	fp2_t t0, t1, t2;
	dv2_t u0, u1, u2, u3;

	fp2_null(t0);
	fp2_null(t1);
	fp2_null(t2);
	dv2_null(u0);
	dv2_null(u1);
	dv2_null(u2);
	dv2_null(u3);

	RLC_TRY {
		fp2_new(t0);
		fp2_new(t1);
		fp2_new(t2);
		dv2_new(u0);
		dv2_new(u1);
		dv2_new(u2);
		dv2_new(u3);

		fp2_sqrn_low(u2, a[0][0]);
		fp2_sqrn_low(u3, a[1][1]);
		fp2_addm_low(t1, a[0][0], a[1][1]);

		fp2_norh_low(u0, u3);
		fp2_addc_low(u0, u0, u2);
		fp2_rdcn_low(t0, u0);

		fp2_sqrn_low(u1, t1);
		fp2_addc_low(u2, u2, u3);
		fp2_subc_low(u1, u1, u2);
		fp2_rdcn_low(t1, u1);

		fp2_subm_low(c[0][0], t0, a[0][0]);
		fp2_addm_low(c[0][0], c[0][0], c[0][0]);
		fp2_addm_low(c[0][0], t0, c[0][0]);

		fp2_addm_low(c[1][1], t1, a[1][1]);
		fp2_addm_low(c[1][1], c[1][1], c[1][1]);
		fp2_addm_low(c[1][1], t1, c[1][1]);

		fp2_sqrn_low(u0, a[0][1]);
		fp2_sqrn_low(u1, a[1][2]);
		fp2_addm_low(t0, a[0][1], a[1][2]);
		fp2_sqrn_low(u2, t0);

		fp2_addc_low(u3, u0, u1);
		fp2_subc_low(u3, u2, u3);
		fp2_rdcn_low(t0, u3);

		fp2_addm_low(t1, a[1][0], a[0][2]);
		fp2_sqrm_low(t2, t1);
		fp2_sqrn_low(u2, a[1][0]);

		fp2_norm_low(t1, t0);
		fp2_addm_low(t0, t1, a[1][0]);
		fp2_addm_low(t0, t0, t0);
		fp2_addm_low(c[1][0], t0, t1);

		fp2_norh_low(u3, u1);
		fp2_addc_low(u3, u0, u3);
		fp2_rdcn_low(t0, u3);
		fp2_subm_low(t1, t0, a[0][2]);

		fp2_sqrn_low(u1, a[0][2]);

		fp2_addm_low(t1, t1, t1);
		fp2_addm_low(c[0][2], t1, t0);

		fp2_norh_low(u3, u1);
		fp2_addc_low(u3, u2, u3);
		fp2_rdcn_low(t0, u3);
		fp2_subm_low(t1, t0, a[0][1]);
		fp2_addm_low(t1, t1, t1);
		fp2_addm_low(c[0][1], t1, t0);

		fp2_addc_low(u0, u2, u1);
		fp2_rdcn_low(t0, u0);
		fp2_subm_low(t0, t2, t0);
		fp2_addm_low(t1, t0, a[1][2]);
		fp2_dblm_low(t1, t1);
		fp2_addm_low(c[1][2], t0, t1);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp2_free(t0);
		fp2_free(t1);
		fp2_free(t2);
		dv2_free(u0);
		dv2_free(u1);
		dv2_free(u2);
		dv2_free(u3);
	}
}

void fp12_sqr_pck_lazyr(fp12_t c, fp12_t a) {
	fp2_t t0, t1, t2;
	dv2_t u0, u1, u2, u3;

	fp2_null(t0);
	fp2_null(t1);
	fp2_null(t2);
	dv2_null(u0);
	dv2_null(u1);
	dv2_null(u2);
	dv2_null(u3);

	RLC_TRY {
		fp2_new(t0);
		fp2_new(t1);
		fp2_new(t2);
		dv2_new(u0);
		dv2_new(u1);
		dv2_new(u2);
		dv2_new(u3);

		fp2_sqrn_low(u0, a[0][1]);
		fp2_sqrn_low(u1, a[1][2]);
		fp2_addm_low(t0, a[0][1], a[1][2]);
		fp2_sqrn_low(u2, t0);

		fp2_addc_low(u3, u0, u1);
		fp2_subc_low(u3, u2, u3);
		fp2_rdcn_low(t0, u3);

		fp2_addm_low(t1, a[1][0], a[0][2]);
		fp2_sqrm_low(t2, t1);
		fp2_sqrn_low(u2, a[1][0]);

		fp2_norm_low(t1, t0);
		fp2_addm_low(t0, t1, a[1][0]);
		fp2_dblm_low(t0, t0);
		fp2_addm_low(c[1][0], t0, t1);

		fp2_norh_low(u3, u1);
		fp2_sqrn_low(u1, a[0][2]);
		fp2_addc_low(u3, u0, u3);
		fp2_rdcn_low(t0, u3);
		fp2_subm_low(t1, t0, a[0][2]);
		fp2_dblm_low(t1, t1);
		fp2_addm_low(c[0][2], t1, t0);

		fp2_addc_low(u0, u2, u1);
		fp2_rdcn_low(t0, u0);
		fp2_subm_low(t0, t2, t0);
		fp2_addm_low(t1, t0, a[1][2]);
		fp2_dblm_low(t1, t1);
		fp2_addm_low(c[1][2], t0, t1);

		fp2_norh_low(u3, u1);
		fp2_addc_low(u3, u2, u3);
		fp2_rdcn_low(t0, u3);
		fp2_subm_low(t1, t0, a[0][1]);
		fp2_dblm_low(t1, t1);
		fp2_addm_low(c[0][1], t1, t0);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp2_free(t0);
		fp2_free(t1);
		fp2_free(t2);
		dv2_free(u0);
		dv2_free(u1);
		dv2_free(u2);
		dv2_free(u3);
	}
}

#endif
