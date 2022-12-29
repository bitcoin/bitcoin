/*
 * RELIC is an Efficient LIbrar->y for Cr->yptography
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
 * Implementation of pairing computation for curves with embedding degree 24.
 *
 * @ingroup pp
 */

#include "relic_core.h"
#include "relic_pp.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if EP_ADD == PROJC || !defined(STRIP)

void pp_dbl_k24_basic(fp24_t l, ep4_t r, ep4_t q, ep_t p) {
	fp4_t s;
	ep4_t t;

	fp4_null(s);
	ep4_null(t);

	RLC_TRY {
		fp4_new(s);
		ep4_new(t);
		ep4_copy(t, q);
		ep4_dbl_slp_basic(r, s, q);
		fp24_zero(l);

		fp_mul(l[1][1][0][0], s[0][0], p->x);
		fp_mul(l[1][1][0][1], s[0][1], p->x);
		fp_mul(l[1][1][1][0], s[1][0], p->x);
		fp_mul(l[1][1][1][1], s[1][1], p->x);

		fp4_mul(l[0][0], s, t->x);
		fp4_sub(l[0][0], t->y, l[0][0]);
		fp4_mul_art(l[0][0], l[0][0]);

		fp_copy(l[0][1][0][0], p->y);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp2_free(s);
		ep4_free(t);
	}
}

#endif

#if EP_ADD == PROJC || !defined(STRIP)

void pp_dbl_k24_projc(fp24_t l, ep4_t r, ep4_t q, ep_t p) {
	fp4_t t0, t1, t2, t3, t4, t5, t6;

	fp4_null(t0);
	fp4_null(t1);
	fp4_null(t2);
	fp4_null(t3);
	fp4_null(t4);
	fp4_null(t5);
	fp4_null(t6);

	RLC_TRY {
		fp4_new(t0);
		fp4_new(t1);
		fp4_new(t2);
		fp4_new(t3);
		fp4_new(t4);
		fp4_new(t5);
		fp4_new(t6);

		/* A = x1^2. */
		fp4_sqr(t0, q->x);
		/* B = y1^2. */
		fp4_sqr(t1, q->y);
		/* C = z1^2. */
		fp4_sqr(t2, q->z);
		/* D = 3bC, general b. */
		fp4_dbl(t3, t2);
		fp4_add(t3, t3, t2);
		ep4_curve_get_b(t4);
		fp4_mul(t3, t3, t4);

		/* E = (x1 + y1)^2 - A - B. */
		fp4_add(t4, q->x, q->y);
		fp4_sqr(t4, t4);
		fp4_sub(t4, t4, t0);
		fp4_sub(t4, t4, t1);

		/* F = (y1 + z1)^2 - B - C. */
		fp4_add(t5, q->y, q->z);
		fp4_sqr(t5, t5);
		fp4_sub(t5, t5, t1);
		fp4_sub(t5, t5, t2);

		/* G = 3D. */
		fp4_dbl(t6, t3);
		fp4_add(t6, t6, t3);

		/* x3 = E * (B - G). */
		fp4_sub(r->x, t1, t6);
		fp4_mul(r->x, r->x, t4);

		/* y3 = (B + G)^2 -12D^2. */
		fp4_add(t6, t6, t1);
		fp4_sqr(t6, t6);
		fp4_sqr(t2, t3);
		fp4_dbl(r->y, t2);
		fp4_dbl(t2, r->y);
		fp4_dbl(r->y, t2);
		fp4_add(r->y, r->y, t2);
		fp4_sub(r->y, t6, r->y);

		/* z3 = 4B * F. */
		fp4_dbl(r->z, t1);
		fp4_dbl(r->z, r->z);
		fp4_mul(r->z, r->z, t5);

		/* l11 = D - B. */
		fp4_sub(l[0][0], t3, t1);
		fp4_mul_art(l[0][0], l[0][0]);

		/* l10 = (3 * xp) * A. */
		fp_mul(l[1][1][0][0], p->x, t0[0][0]);
		fp_mul(l[1][1][0][1], p->x, t0[0][1]);
		fp_mul(l[1][1][1][0], p->x, t0[1][0]);
		fp_mul(l[1][1][1][1], p->x, t0[1][1]);

		/* l00 = F * (-yp). */
		fp_mul(l[0][1][0][0], p->y, t5[0][0]);
		fp_mul(l[0][1][0][1], p->y, t5[0][1]);
		fp_mul(l[0][1][1][0], p->y, t5[1][0]);
		fp_mul(l[0][1][1][1], p->y, t5[1][1]);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp4_free(t0);
		fp4_free(t1);
		fp4_free(t2);
		fp4_free(t3);
		fp4_free(t4);
		fp4_free(t5);
		fp4_free(t6);
	}
}

#endif
