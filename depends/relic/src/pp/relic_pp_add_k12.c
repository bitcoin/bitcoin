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
 * Implementation of Miller addition for curves of embedding degree 12.
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

void pp_add_k12_basic(fp12_t l, ep2_t r, ep2_t q, ep_t p) {
	int one = 1, zero = 0;
	fp2_t s;
	ep2_t t;

	fp2_null(s);
	ep2_null(t);

	RLC_TRY {
		fp2_new(s);
		ep2_new(t);

		ep2_copy(t, r);
		ep2_add_slp_basic(r, s, r, q);

		if (ep2_curve_is_twist() == RLC_EP_MTYPE) {
			one ^= 1;
			zero ^= 1;
		}

		fp_mul(l[one][zero][0], s[0], p->x);
		fp_mul(l[one][zero][1], s[1], p->x);
		fp2_mul(l[one][one], s, t->x);
		fp2_sub(l[one][one], t->y, l[one][one]);
		fp_neg(l[zero][zero][0], p->y);
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

void pp_add_k12_projc_basic(fp12_t l, ep2_t r, ep2_t q, ep_t p) {
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

		/* B = t0 = x1 - x2 * z1. */
		fp2_mul(t0, r->z, q->x);
		fp2_sub(t0, r->x, t0);
		/* A = t1 = y1 - y2 * z1. */
		fp2_mul(t1, r->z, q->y);
		fp2_sub(t1, r->y, t1);

		/* D = B^2. */
		fp2_sqr(t2, t0);
		/* G = x1 * D. */
		fp2_mul(r->x, r->x, t2);
		/* E = B^3. */
		fp2_mul(t2, t2, t0);
		/* C = A^2. */
		fp2_sqr(t3, t1);
		/* F = E + z1 * C. */
		fp2_mul(t3, t3, r->z);
		fp2_add(t3, t2, t3);

		if (ep2_curve_is_twist() == RLC_EP_MTYPE) {
			one ^= 1;
			zero ^= 1;
		}

		/* l10 = - (A * xp). */
		fp_neg(t4[0], p->x);
		fp_mul(l[one][zero][0], t1[0], t4[0]);
		fp_mul(l[one][zero][1], t1[1], t4[0]);

		/* t4 = B * x2. */
		fp2_mul(t4, q->x, t1);

		/* H = F - 2 * G. */
		fp2_sub(t3, t3, r->x);
		fp2_sub(t3, t3, r->x);
		/* y3 = A * (G - H) - y1 * E. */
		fp2_sub(r->x, r->x, t3);
		fp2_mul(t1, t1, r->x);
		fp2_mul(r->y, t2, r->y);
		fp2_sub(r->y, t1, r->y);
		/* x3 = B * H. */
		fp2_mul(r->x, t0, t3);
		/* z3 = z1 * E. */
		fp2_mul(r->z, r->z, t2);

		/* l11 = J = A * x2 - B * y2. */
		fp2_mul(t2, q->y, t0);
		fp2_sub(l[one][one], t4, t2);

		/* l00 = B * yp. */
		fp_mul(l[zero][zero][0], t0[0], p->y);
		fp_mul(l[zero][zero][1], t0[1], p->y);

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
	}
}

#endif

#if PP_EXT == LAZYR || !defined(STRIP)

void pp_add_k12_projc_lazyr(fp12_t l, ep2_t r, ep2_t q, ep_t p) {
	fp2_t t0, t1, t2, t3;
	dv2_t u0, u1;
	int one = 1, zero = 0;

	fp2_null(t0);
	fp2_null(t1);
	fp2_null(t2);
	fp2_null(t3);
	dv2_null(u0);
	dv2_null(u1);

	RLC_TRY {
		fp2_new(t0);
		fp2_new(t1);
		fp2_new(t2);
		fp2_new(t3);
		dv2_new(u0);
		dv2_new(u1);

		fp2_mul(t0, r->z, q->x);
		fp2_sub(t0, r->x, t0);
		fp2_mul(t1, r->z, q->y);
		fp2_sub(t1, r->y, t1);

		fp2_sqr(t2, t0);
		fp2_mul(r->x, t2, r->x);
		fp2_mul(t2, t0, t2);
		fp2_sqr(t3, t1);
		fp2_mul(t3, t3, r->z);
		fp2_add(t3, t2, t3);

		fp2_sub(t3, t3, r->x);
		fp2_sub(t3, t3, r->x);
		fp2_sub(r->x, r->x, t3);
#ifdef RLC_FP_ROOM
		fp2_mulc_low(u0, t1, r->x);
		fp2_mulc_low(u1, t2, r->y);
#else
		fp2_muln_low(u0, t1, r->x);
		fp2_muln_low(u1, t2, r->y);
#endif
		fp2_subc_low(u1, u0, u1);
		fp2_rdcn_low(r->y, u1);
		fp2_mul(r->x, t0, t3);
		fp2_mul(r->z, r->z, t2);

		if (ep2_curve_is_twist() == RLC_EP_MTYPE) {
			one ^= 1;
			zero ^= 1;
		}

		fp_neg(t3[0], p->x);
		fp_mul(l[one][zero][0], t1[0], t3[0]);
		fp_mul(l[one][zero][1], t1[1], t3[0]);

#ifdef RLC_FP_ROOM
		fp2_mulc_low(u0, q->x, t1);
		fp2_mulc_low(u1, q->y, t0);
#else
		fp2_muln_low(u0, q->x, t1);
		fp2_muln_low(u1, q->y, t0);
#endif
		fp2_subc_low(u0, u0, u1);
		fp2_rdcn_low(l[one][one], u0);

		fp_mul(l[zero][zero][0], t0[0], p->y);
		fp_mul(l[zero][zero][1], t0[1], p->y);

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
		dv2_free(u0);
		dv2_free(u1);
	}
}

#endif

#endif

void pp_add_lit_k12(fp12_t l, ep_t r, ep_t p, ep2_t q) {
	fp_t t0, t1, t2, t3;
	int one = 1, zero = 0;

	fp_null(t0);
	fp_null(t1);
	fp_null(t2);
	fp_null(t3);

	RLC_TRY {
		fp_new(t0);
		fp_new(t1);
		fp_new(t2);
		fp_new(t3);

		fp_mul(t0, r->z, p->x);
		fp_sub(t0, r->x, t0);
		fp_mul(t1, r->z, p->y);
		fp_sub(t1, r->y, t1);
		fp_mul(t2, p->x, t1);
		r->coord = PROJC;

		if (ep2_curve_is_twist() == RLC_EP_MTYPE) {
			one ^= 1;
			zero ^= 1;
		}

		fp_mul(l[zero][zero][0], t0, p->y);
		fp_sub(l[zero][zero][0], t2, l[zero][zero][0]);

		fp_mul(l[zero][one][0], q->x[0], t1);
		fp_mul(l[zero][one][1], q->x[1], t1);
		fp2_neg(l[zero][one], l[zero][one]);

		fp_mul(l[one][one][0], q->y[0], t0);
		fp_mul(l[one][one][1], q->y[1], t0);

		fp_sqr(t2, t0);
		fp_mul(r->x, t2, r->x);
		fp_mul(t2, t0, t2);
		fp_sqr(t3, t1);
		fp_mul(t3, t3, r->z);
		fp_add(t3, t2, t3);
		fp_sub(t3, t3, r->x);
		fp_sub(t3, t3, r->x);
		fp_sub(r->x, r->x, t3);
		fp_mul(t1, t1, r->x);
		fp_mul(r->y, t2, r->y);
		fp_sub(r->y, t1, r->y);
		fp_mul(r->x, t0, t3);
		fp_mul(r->z, r->z, t2);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp_free(t0);
		fp_free(t1);
		fp_free(t2);
		fp_free(t3);
	}
}
