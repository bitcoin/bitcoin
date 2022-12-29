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
 * Implementation of Miller doubling for curves of embedding degree 12.
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

void pp_dbl_k12_basic(fp12_t l, ep2_t r, ep2_t q, ep_t p) {
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

void pp_dbl_k12_projc_basic(fp12_t l, ep2_t r, ep2_t q, ep_t p) {
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

		if (ep_curve_opt_b() == RLC_TWO) {
			/* t0 = x1^2. */
			fp2_sqr(t0, q->x);
			/* t1 = B = y1^2. */
			fp2_sqr(t1, q->y);
			/* t2 = C = z1^2. */
			fp2_sqr(t2, q->z);
			/* t4 = A = (x1 * y1)/2. */
			fp2_mul(t4, q->x, q->y);
			fp_hlv(t4[0], t4[0]);
			fp_hlv(t4[1], t4[1]);
			/* t3 = E = 3b'C = 3C * (1 - i). */
			fp2_dbl(t3, t2);
			fp2_add(t2, t2, t3);
			fp_add(t3[0], t2[0], t2[1]);
			fp_sub(t3[1], t2[1], t2[0]);
			/* t2 = F = 3E. */
			fp2_dbl(t2, t3);
			fp2_add(t2, t3, t2);
			/* x3 = A * (B - F). */
			fp2_sub(r->x, t1, t2);
			fp2_mul(r->x, r->x, t4);
			/* t2 = G = (B + F)/2. */
			fp2_add(t2, t1, t2);
			fp_hlv(t2[0], t2[0]);
			fp_hlv(t2[1], t2[1]);
			/* y3 = G^2 - 3E^2. */
			fp2_sqr(t2, t2);
			fp2_sqr(t4, t3);
			/* t5 = y1 * z1. */
			fp2_mul(t5, q->y, q->z);
			fp2_dbl(r->y, t4);
			fp2_add(r->y, r->y, t4);
			fp2_sub(r->y, t2, r->y);

			/* t2 = H = 2 * y1 * z1. */
			fp2_dbl(t2, t5);
			/* z3 = B * H. */
			fp2_mul(r->z, t1, t2);

			/* l11 = E - B. */
			fp2_sub(l[one][one], t3, t1);

			/* l10 = (3 * xp) * t0. */
			fp_mul(l[one][zero][0], p->x, t0[0]);
			fp_mul(l[one][zero][1], p->x, t0[1]);

			/* l00 = H * (-yp). */
			fp_mul(l[zero][zero][0], t2[0], p->y);
			fp_mul(l[zero][zero][1], t2[1], p->y);
		} else {
			/* A = x1^2. */
			fp2_sqr(t0, q->x);
			/* B = y1^2. */
			fp2_sqr(t1, q->y);
			/* C = z1^2. */
			fp2_sqr(t2, q->z);
			/* D = 3bC, general b. */
			fp2_dbl(t3, t2);
			fp2_add(t3, t3, t2);
			fp2_mul(t3, t3, ep2_curve_get_b());
			/* E = (x1 + y1)^2 - A - B. */
			fp2_add(t4, q->x, q->y);
			fp2_sqr(t4, t4);
			fp2_sub(t4, t4, t0);
			fp2_sub(t4, t4, t1);

			/* F = (y1 + z1)^2 - B - C. */
			fp2_add(t5, q->y, q->z);
			fp2_sqr(t5, t5);
			fp2_sub(t5, t5, t1);
			fp2_sub(t5, t5, t2);

			/* G = 3D. */
			fp2_dbl(t6, t3);
			fp2_add(t6, t6, t3);

			/* x3 = E * (B - G). */
			fp2_sub(r->x, t1, t6);
			fp2_mul(r->x, r->x, t4);

			/* y3 = (B + G)^2 -12D^2. */
			fp2_add(t6, t6, t1);
			fp2_sqr(t6, t6);
			fp2_sqr(t2, t3);
			fp2_dbl(r->y, t2);
			fp2_dbl(t2, r->y);
			fp2_dbl(r->y, t2);
			fp2_add(r->y, r->y, t2);
			fp2_sub(r->y, t6, r->y);

			/* z3 = 4B * F. */
			fp2_dbl(r->z, t1);
			fp2_dbl(r->z, r->z);
			fp2_mul(r->z, r->z, t5);

			/* l11 = D - B. */
			fp2_sub(l[one][one], t3, t1);

			/* l10 = (3 * xp) * A. */
			fp_mul(l[one][zero][0], p->x, t0[0]);
			fp_mul(l[one][zero][1], p->x, t0[1]);

			/* l00 = F * (-yp). */
			fp_mul(l[zero][zero][0], t5[0], p->y);
			fp_mul(l[zero][zero][1], t5[1], p->y);
		}
		r->coord = PROJC;
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

void pp_dbl_k12_projc_lazyr(fp12_t l, ep2_t r, ep2_t q, ep_t p) {
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

		if (ep_curve_opt_b() == RLC_TWO) {
			/* C = z1^2. */
			fp2_sqr(t0, q->z);
			/* B = y1^2. */
			fp2_sqr(t1, q->y);
			/* t5 = B + C. */
			fp2_add(t5, t0, t1);
			/* t3 = E = 3b'C = 3C * (1 - i). */
			fp2_dbl(t3, t0);
			fp2_add(t0, t0, t3);
			fp_add(t2[0], t0[0], t0[1]);
			fp_sub(t2[1], t0[1], t0[0]);

			/* t0 = x1^2. */
			fp2_sqr(t0, q->x);
			/* t4 = A = (x1 * y1)/2. */
			fp2_mul(t4, q->x, q->y);
			fp_hlv(t4[0], t4[0]);
			fp_hlv(t4[1], t4[1]);
			/* t3 = F = 3E. */
			fp2_dbl(t3, t2);
			fp2_add(t3, t3, t2);
			/* x3 = A * (B - F). */
			fp2_sub(r->x, t1, t3);
			fp2_mul(r->x, r->x, t4);

			/* G = (B + F)/2. */
			fp2_add(t3, t1, t3);
			fp_hlv(t3[0], t3[0]);
			fp_hlv(t3[1], t3[1]);

			/* y3 = G^2 - 3E^2. */
			fp2_sqrn_low(u0, t2);
			fp2_addd_low(u1, u0, u0);
			fp2_addd_low(u1, u1, u0);
			fp2_sqrn_low(u0, t3);
			fp2_subc_low(u0, u0, u1);

			/* H = (Y + Z)^2 - B - C. */
			fp2_add(t3, q->y, q->z);
			fp2_sqr(t3, t3);
			fp2_sub(t3, t3, t5);

			fp2_rdcn_low(r->y, u0);

			/* z3 = B * H. */
			fp2_mul(r->z, t1, t3);

			/* l11 = E - B. */
			fp2_sub(l[1][1], t2, t1);

			/* l10 = (3 * xp) * t0. */
			fp_mul(l[one][zero][0], p->x, t0[0]);
			fp_mul(l[one][zero][1], p->x, t0[1]);

			/* l01 = F * (-yp). */
			fp_mul(l[zero][zero][0], t3[0], p->y);
			fp_mul(l[zero][zero][1], t3[1], p->y);
		} else {
			/* A = x1^2. */
			fp2_sqr(t0, q->x);
			/* B = y1^2. */
			fp2_sqr(t1, q->y);
			/* C = z1^2. */
			fp2_sqr(t2, q->z);
			/* D = 3bC, for general b. */
			fp2_dbl(t3, t2);
			fp2_add(t3, t3, t2);
			fp2_mul(t3, t3, ep2_curve_get_b());
			/* E = (x1 + y1)^2 - A - B. */
			fp2_add(t4, q->x, q->y);
			fp2_sqr(t4, t4);
			fp2_sub(t4, t4, t0);
			fp2_sub(t4, t4, t1);

			/* F = (y1 + z1)^2 - B - C. */
			fp2_add(t5, q->y, q->z);
			fp2_sqr(t5, t5);
			fp2_sub(t5, t5, t1);
			fp2_sub(t5, t5, t2);

			/* G = 3D. */
			fp2_dbl(t6, t3);
			fp2_add(t6, t6, t3);

			/* x3 = E * (B - G). */
			fp2_sub(r->x, t1, t6);
			fp2_mul(r->x, r->x, t4);

			/* y3 = (B + G)^2 -12D^2. */
			fp2_add(t6, t6, t1);
			fp2_sqr(t6, t6);
			fp2_sqr(t2, t3);
			fp2_dbl(r->y, t2);
			fp2_dbl(t2, r->y);
			fp2_dbl(r->y, t2);
			fp2_add(r->y, r->y, t2);
			fp2_sub(r->y, t6, r->y);

			/* z3 = 4B * F. */
			fp2_dbl(r->z, t1);
			fp2_dbl(r->z, r->z);
			fp2_mul(r->z, r->z, t5);

			/* l00 = D - B. */
			fp2_sub(l[one][one], t3, t1);

			/* l10 = (3 * xp) * A. */
			fp_mul(l[one][zero][0], p->x, t0[0]);
			fp_mul(l[one][zero][1], p->x, t0[1]);

			/* l01 = F * (-yp). */
			fp_mul(l[zero][zero][0], t5[0], p->y);
			fp_mul(l[zero][zero][1], t5[1], p->y);
		}
		r->coord = PROJC;
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
		dv2_free(u1);
	}
}

#endif

#endif

void pp_dbl_lit_k12(fp12_t l, ep_t r, ep_t p, ep2_t q) {
	fp_t t0, t1, t2, t3, t4, t5, t6;
	int one = 1, zero = 0;

	fp_null(t0);
	fp_null(t1);
	fp_null(t2);
	fp_null(t3);
	fp_null(t4);
	fp_null(t5);
	fp_null(t6);

	RLC_TRY {
		fp_new(t0);
		fp_new(t1);
		fp_new(t2);
		fp_new(t3);
		fp_new(t4);
		fp_new(t5);
		fp_new(t6);

		fp_sqr(t0, p->x);
		fp_sqr(t1, p->y);
		fp_sqr(t2, p->z);

		fp_mul(t4, ep_curve_get_b(), t2);

		fp_dbl(t3, t4);
		fp_add(t3, t3, t4);

		fp_add(t4, p->x, p->y);
		fp_sqr(t4, t4);
		fp_sub(t4, t4, t0);
		fp_sub(t4, t4, t1);
		fp_add(t5, p->y, p->z);
		fp_sqr(t5, t5);
		fp_sub(t5, t5, t1);
		fp_sub(t5, t5, t2);
		fp_dbl(t6, t3);
		fp_add(t6, t6, t3);
		fp_sub(r->x, t1, t6);
		fp_mul(r->x, r->x, t4);
		fp_add(r->y, t1, t6);
		fp_sqr(r->y, r->y);
		fp_sqr(t4, t3);
		fp_dbl(t6, t4);
		fp_add(t6, t6, t4);
		fp_dbl(t6, t6);
		fp_dbl(t6, t6);
		fp_sub(r->y, r->y, t6);
		fp_mul(r->z, t1, t5);
		fp_dbl(r->z, r->z);
		fp_dbl(r->z, r->z);
		r->coord = PROJC;

		if (ep2_curve_is_twist() == RLC_EP_MTYPE) {
			one ^= 1;
			zero ^= 1;
		}

		fp2_dbl(l[zero][one], q->x);
		fp2_add(l[zero][one], l[zero][one], q->x);
		fp_mul(l[zero][one][0], l[zero][one][0], t0);
		fp_mul(l[zero][one][1], l[zero][one][1], t0);

		fp_sub(l[zero][zero][0], t3, t1);
		fp_zero(l[zero][zero][1]);

		fp_mul(l[one][one][0], q->y[0], t5);
		fp_mul(l[one][one][1], q->y[1], t5);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp_free(t0);
		fp_free(t1);
		fp_free(t2);
		fp_free(t3);
		fp_free(t4);
		fp_free(t5);
		fp_free(t6);
	}
}
