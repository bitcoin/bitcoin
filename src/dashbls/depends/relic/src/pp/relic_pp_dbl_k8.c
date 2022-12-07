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
 * Implementation of Miller doubling for curves of embedding degree 2.
 *
 * @ingroup pp
 */

#include "relic_core.h"
#include "relic_pp.h"
#include "relic_fp_low.h"
#include "relic_fpx_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if EP_ADD == BASIC || !defined(STRIP)

void pp_dbl_k8_basic(fp8_t l, ep2_t r, ep2_t q, ep_t p) {
	fp2_t s;
	ep2_t t;
	int one = 1, zero = 0;

	fp2_null(s);
	ep2_null(t);

	RLC_TRY {
		fp2_new(s);
		ep2_new(t);
		ep2_copy(t, q);
		ep2_dbl_slp_basic(r, s, q);

		if (ep2_curve_is_twist() == RLC_EP_MTYPE) {
			one ^= 1;
			zero ^= 1;
		}

		fp_mul(l[one][zero][0], s[0], p->x);
		fp_mul(l[one][zero][1], s[1], p->x);
		fp2_mul(l[one][one], s, t->x);
		fp2_sub(l[one][one], t->y, l[one][one]);
		fp_copy(l[zero][zero][0], p->y);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp2_free(s);
		ep2_free(t);
	}
}

#endif

#if EP_ADD == PROJC || EP_ADD == JACOB || !defined(STRIP)

#if PP_EXT == BASIC || !defined(STRIP)

void pp_dbl_k8_projc_new(fp8_t l, ep2_t r, ep2_t q, ep_t p) {
	fp2_t t0, t1, t2, t3, t4;
	int one = 1, zero = 0;

	fp2_null(t0);
	fp2_null(t1);
	fp2_null(t2);
	fp2_null(t3);
	fp2_null(t4);

	RLC_TRY {
		fp2_new(t0);
		fp2_new(t1);
		fp2_new(t2);
		fp2_new(t3);
		fp2_new(t4);

		if (ep2_curve_is_twist() == RLC_EP_MTYPE) {
			one ^= 1;
			zero ^= 1;
		}

		/* t0 = z2 = z1^2, t1 = y1^2, t2 = x1*t1. */
		fp2_sqr(t0, q->z);
		fp2_sqr(t1, q->y);
		fp2_mul(t2, q->x, t1);

		/* t3 = 3/2*x1^2 + (a = 2)/2*Z2^2. */
		fp2_sqr(t3, q->x);
		fp_hlv(t4[0], t3[0]);
		fp_hlv(t4[1], t3[1]);
		fp2_add(t3, t3, t4);
		fp2_sqr(t4, t0);
		fp2_mul_art(t4, t4);
		fp2_add(t3, t3, t4);

		/* z3 := z1*y1, l := z3*z2*yP + (t3*x1 - t1) + t3*z2*xP. */
		fp2_mul(r->z, q->y, q->z);
		fp2_mul(l[one][one], t3, q->x);
		fp2_sub(l[one][one], l[one][one], t1);
		fp2_mul(l[zero][zero], r->z, t0);
		fp_mul(l[zero][zero][0], l[zero][zero][0], p->y);
		fp_mul(l[zero][zero][1], l[zero][zero][1], p->y);
		fp2_mul(l[one][zero], t3, t0);
		fp_mul(l[one][zero][0], l[one][zero][0], p->x);
		fp_mul(l[one][zero][1], l[one][zero][1], p->x);

		/* x3 := t3^2 - 2*t2. */
		fp2_sqr(r->x, t3);
		fp2_sub(r->x, r->x, t2);
		fp2_sub(r->x, r->x, t2);
		/* y3 := t3*(t2 - x3) - t1^2. */
		fp2_sub(t2, t2, r->x);
		fp2_mul(r->y, t3, t2);
		fp2_sqr(t1, t1);
		fp2_sub(r->y, r->y, t1);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp2_free(t0);
		fp2_free(t1);
		fp2_free(t2);
		fp2_free(t3);
		fp2_free(t4);
	}
}

void pp_dbl_k8_projc_basic(fp8_t l, ep2_t r, ep2_t q, ep_t p) {
	fp2_t t0, t1, t2, t3, t4, t5, t6;
	int one = 1, zero = 0;

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

		if (ep2_curve_is_twist() == RLC_EP_MTYPE) {
			one ^= 1;
			zero ^= 1;
		}

		/* t0 = A = X1^2, t1 = B = Y1^2, t2 = C = Z1^2, t3 = D = a*C. */
		fp2_sqr(t0, q->x);
		fp2_sqr(t1, q->y);
		fp2_sqr(t2, q->z);
		switch (ep_curve_opt_a()) {
			case RLC_ZERO:
				fp2_zero(t3);
				break;
			case RLC_ONE:
				fp2_copy(t3, t2);
				break;
#if FP_RDC != MONTY
			case RLC_TINY:
				fp_mul_dig(t3[0], t2[0], ep_curve_get_a()[0]);
				fp_mul_dig(t3[1], t2[1], ep_curve_get_a()[0]);
				break;
#endif
			default:
				fp_mul(t3[0], t2[0], ep_curve_get_a());
				fp_mul(t3[1], t2[1], ep_curve_get_a());
				break;
		}
		fp2_mul_art(t3, t3);

		/* x3 = (A - D)^2, l11 = (A - D + x1)^2 - x3 - A. */
		fp2_sub(t5, t0, t3);
		fp2_add(l[one][one], t5, q->x);
		fp2_sqr(r->x, t5);
		fp2_sqr(l[one][one], l[one][one]);
		fp2_sub(l[one][one], l[one][one], r->x);
		fp2_sub(l[one][one], l[one][one], t0);

		/* l10 := -xp*z1*2*(3A + D). */
		fp2_add(t6, t0, t3);
		fp2_dbl(t0, t0);
		fp2_add(t0, t0, t6);
		fp2_dbl(t0, t0);
		fp2_mul(l[one][zero], t0, q->z);
		fp_mul(l[one][zero][0], l[one][zero][0], p->x);
		fp_mul(l[one][zero][1], l[one][zero][1], p->x);

		/* l01 = 2*((y1 + z1)^2 - B - C)*yP. */
		fp2_add(l[zero][zero], q->y, q->z);
		fp2_sqr(l[zero][zero], l[zero][zero]);
		fp2_sub(l[zero][zero], l[zero][zero], t1);
		fp2_sub(l[zero][zero], l[zero][zero], t2);
		fp2_dbl(l[zero][zero], l[zero][zero]);
		fp_mul(l[zero][zero][0], l[zero][zero][0], p->y);
		fp_mul(l[zero][zero][1], l[zero][zero][1], p->y);

		/* t4 = E = 2*(A + D)^2 - x3. */
		fp2_sqr(t4, t6);
		fp2_dbl(t4, t4);
		fp2_sub(t4, t4, r->x);
		/* y3 = E * ((A - D + y1)^2 - B - x3). */
		fp2_add(r->y, t5, q->y);
		fp2_sqr(r->y, r->y);
		fp2_sub(r->y, r->y, t1);
		fp2_sub(r->y, r->y, r->x);
		fp2_mul(r->y, r->y, t4);
		/* z3 = 4*B. */
		fp2_dbl(r->z, t1);
		fp2_dbl(r->z, r->z);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
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

#if PP_EXT == LAZYR || !defined(STRIP)

void pp_dbl_k8_projc_lazyr(fp8_t l, ep2_t r, ep2_t q, ep_t p) {
	fp2_t t0, t1, t2, t3, t4, t5, t6;
	dv2_t u0, u1;
	int one = 1, zero = 0;

	fp2_null(t0);
	fp2_null(t1);
	fp2_null(t2);
	fp2_null(t3);
	fp2_null(t4);
	fp2_null(t5);
	fp2_null(t6);
	dv2_null(u0);
	dv2_null(u1);

	RLC_TRY {
		fp2_new(t0);
		fp2_new(t1);
		fp2_new(t2);
		fp2_new(t3);
		fp2_new(t4);
		fp2_new(t5);
		fp2_new(t6);
		dv2_new(u0);
		dv2_new(u1);

		if (ep2_curve_is_twist() == RLC_EP_MTYPE) {
			one ^= 1;
			zero ^= 1;
		}

		/* t0 = A = X1^2, t1 = B = Y1^2, t2 = C = Z1^2, t3 = D = a*C. */
		fp2_sqr(t0, q->x);
		fp2_sqr(t1, q->y);
		fp2_sqr(t2, q->z);
		switch (ep_curve_opt_a()) {
			case RLC_ZERO:
				fp2_zero(t3);
				break;
			case RLC_ONE:
				fp2_copy(t3, t2);
				break;
			case RLC_TWO:
				fp2_dbl(t3, t2);
				break;
#if FP_RDC != MONTY
			case RLC_TINY:
				fp_mul_dig(t3[0], t2[0], ep_curve_get_a()[0]);
				fp_mul_dig(t3[1], t2[1], ep_curve_get_a()[0]);
				break;
#endif
			default:
				fp_mul(t3[0], t2[0], ep_curve_get_a());
				fp_mul(t3[1], t2[1], ep_curve_get_a());
				break;
		}
		fp2_mul_art(t3, t3);

		/* x3 = (A - D)^2, l11 = (A - D + x1)^2 - x3 - A. */
		fp2_sub(t5, t0, t3);
		fp2_add(l[one][one], t5, q->x);
		fp2_sqr(r->x, t5);
		fp2_sqr(l[one][one], l[one][one]);
		fp2_sub(l[one][one], l[one][one], r->x);
		fp2_sub(l[one][one], l[one][one], t0);

		/* l10 := -xp*z1*2*(3A + D). */
		fp2_add(t6, t0, t3);
		fp2_dbl(t0, t0);
		fp2_add(t0, t0, t6);
		fp2_dbl(t0, t0);
		fp2_mul(l[one][zero], t0, q->z);
		fp_mul(l[one][zero][0], l[one][zero][0], p->x);
		fp_mul(l[one][zero][1], l[one][zero][1], p->x);

		/* l01 = 2*yP*((y1 + z1)^2 - B - C). */
		fp2_add(l[zero][zero], q->y, q->z);
		fp2_sqr(l[zero][zero], l[zero][zero]);
		fp2_sub(l[zero][zero], l[zero][zero], t1);
		fp2_sub(l[zero][zero], l[zero][zero], t2);
		fp2_dbl(l[zero][zero], l[zero][zero]);
		fp_mul(l[zero][zero][0], l[zero][zero][0], p->y);
		fp_mul(l[zero][zero][1], l[zero][zero][1], p->y);

		/* t4 = E = 2*(A + D)^2 - x3. */
		fp2_sqr(t4, t6);
		fp2_dbl(t4, t4);
		fp2_sub(t4, t4, r->x);
		/* y3 = E * ((A - D + y1)^2 - B - x3). */
		fp2_add(r->y, t5, q->y);
		fp2_sqr(r->y, r->y);
		fp2_sub(r->y, r->y, t1);
		fp2_sub(r->y, r->y, r->x);
		fp2_mul(r->y, r->y, t4);
		/* z3 = 4*B. */
		fp2_dbl(r->z, t1);
		fp2_dbl(r->z, r->z);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp2_free(t0);
		fp2_free(t1);
		fp2_free(t2);
		fp2_free(t3);
		fp2_free(t4);
		fp2_free(t5);
		fp2_free(t6);
		dv2_free(u0);
		dv2_free(u1)
	}
}

void pp_dbl_k8_projc_lazyr_new(fp8_t l, ep2_t r, ep2_t q, ep_t p) {
	fp2_t t0, t1, t2, t3;
	dv2_t u2, u3;
	int one = 1, zero = 0;

	fp2_null(t0);
	fp2_null(t1);
	fp2_null(t2);
	fp2_null(t3);
	dv2_null(u2);
	dv2_null(u3);

	RLC_TRY {
		fp2_new(t0);
		fp2_new(t1);
		fp2_new(t2);
		fp2_new(t3);
		dv2_new(u2);
		dv2_new(u3);

		if (ep2_curve_is_twist() == RLC_EP_MTYPE) {
			one ^= 1;
			zero ^= 1;
		}

		/* t0 = z2 = z1^2, t1 = y1^2. */
		fp2_sqr(t0, q->z);
		fp2_sqr(t1, q->y);

		/* t3 = 3/2*x1^2 + (a = 2)/2*z2^2, t2 = x1*t1. */
		fp2_sqrn_low(u3, q->x);
		fp_hlvd_low(u2[0], u3[0]);
		fp_hlvd_low(u2[1], u3[1]);
		fp2_addc_low(u3, u3, u2);
		fp2_sqrn_low(u2, t0);
		fp2_nord_low(u2, u2);
		fp2_addc_low(u3, u3, u2);
		fp2_rdcn_low(t3, u3);

		/* z3 := z1*y1, l := z3*z2*yP + (t3*x1 - t1) + t3*z2*xP. */
		fp2_mul(r->z, q->y, q->z);
		fp2_mul(l[one][one], t3, q->x);
		fp2_sub(l[one][one], l[one][one], t1);
		fp2_mul(l[zero][zero], r->z, t0);
		fp_mul(l[zero][zero][0], l[zero][zero][0], p->y);
		fp_mul(l[zero][zero][1], l[zero][zero][1], p->y);
		fp2_mul(l[one][zero], t3, t0);
		fp_mul(l[one][zero][0], l[one][zero][0], p->x);
		fp_mul(l[one][zero][1], l[one][zero][1], p->x);

		/* x3 := t3^2 - 2*t2. */
		fp2_muln_low(u2, q->x, t1);
		fp2_sqrn_low(u3, t3);
		fp2_subc_low(u3, u3, u2);
		fp2_subc_low(u3, u3, u2);
		fp2_rdcn_low(r->x, u3);
		fp2_rdcn_low(t2, u2);
		/* y3 := t3*(t2 - x3) - t1^2. */
		fp2_sub(t2, t2, r->x);
		fp2_muln_low(u3, t3, t2);
		fp2_sqrn_low(u2, t1);
		fp2_subc_low(u3, u3, u2);
		fp2_rdcn_low(r->y, u3);
		/* t2 - x3 = t3*(3*t2 - t3^2). */
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp2_free(t0);
		fp2_free(t1);
		fp2_free(t2);
		fp2_free(t3);
	}
}

#endif

#endif
