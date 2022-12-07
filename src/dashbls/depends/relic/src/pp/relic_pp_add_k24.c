/*
 * RELIC is an Efficient LIbrar->y for Cr->yptography
 * Copyright (c) 2021 RELIC Authors
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

#if EP_ADD == BASIC || !defined(STRIP)

void pp_add_k24_basic(fp24_t l, ep4_t r, ep4_t q, ep_t p) {
	fp4_t s;
	ep4_t t;

	fp4_null(s);
	ep4_null(t);

	RLC_TRY {
		fp4_new(s);
		ep4_new(t);

		ep4_copy(t, r);
		ep4_add_slp_basic(r, s, r, q);

		fp_mul(l[1][1][0][0], s[0][0], p->x);
		fp_mul(l[1][1][0][1], s[0][1], p->x);
		fp_mul(l[1][1][1][0], s[1][0], p->x);
		fp_mul(l[1][1][1][1], s[1][1], p->x);

		fp4_mul(l[0][0], s, t->x);
		fp4_sub(l[0][0], t->y, l[0][0]);
		fp4_mul_art(l[0][0], l[0][0]);

		fp_neg(l[0][1][0][0], p->y);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp4_free(s);
		ep4_free(t);
	}
}

#endif

#if EP_ADD == PROJC || !defined(STRIP)

void pp_add_k24_projc(fp24_t l, ep4_t r, ep4_t q, ep_t p) {
	fp4_t t0, t1, t2, t3, t4;

	fp4_null(t0);
	fp4_null(t1);
	fp4_null(t2);
	fp4_null(t3);
	fp4_null(t4);

	RLC_TRY {
		fp4_new(t0);
		fp4_new(t1);
		fp4_new(t2);
		fp4_new(t3);
		fp4_new(t4);

		/* B = t0 = x1 - x2 * z1. */
		fp4_mul(t0, r->z, q->x);
		fp4_sub(t0, r->x, t0);
		/* A = t1 = y1 - y2 * z1. */
		fp4_mul(t1, r->z, q->y);
		fp4_sub(t1, r->y, t1);

		/* D = B^2. */
		fp4_sqr(t2, t0);
		/* G = x1 * D. */
		fp4_mul(r->x, r->x, t2);
		/* E = B^3. */
		fp4_mul(t2, t2, t0);
		/* C = A^2. */
		fp4_sqr(t3, t1);
		/* F = E + z1 * C. */
		fp4_mul(t3, t3, r->z);
		fp4_add(t3, t2, t3);

		/* l10 = - (A * xp). */
		fp_neg(t4[0][0], p->x);
		fp_mul(l[1][1][0][0], t1[0][0], t4[0][0]);
		fp_mul(l[1][1][0][1], t1[0][1], t4[0][0]);
		fp_mul(l[1][1][1][0], t1[1][0], t4[0][0]);
		fp_mul(l[1][1][1][1], t1[1][1], t4[0][0]);

		/* t4 = B * x2. */
		fp4_mul(t4, q->x, t1);

		/* H = E + F - 2 * G. */
		fp4_sub(t3, t3, r->x);
		fp4_sub(t3, t3, r->x);
		/* y3 = A * (G - H) - y1 * E. */
		fp4_sub(r->x, r->x, t3);
		fp4_mul(t1, t1, r->x);
		fp4_mul(r->y, t2, r->y);
		fp4_sub(r->y, t1, r->y);
		/* x3 = B * H. */
		fp4_mul(r->x, t0, t3);
		/* z3 = z1 * E. */
		fp4_mul(r->z, r->z, t2);

		/* l11 = J = B * x2 - A * y2. */
		fp4_mul(t2, q->y, t0);
		fp4_sub(l[0][0], t4, t2);
		fp4_mul_art(l[0][0], l[0][0]);

		/* l00 = B * yp. */
		fp_mul(l[0][1][0][0], p->y, t0[0][0]);
		fp_mul(l[0][1][0][1], p->y, t0[0][1]);
		fp_mul(l[0][1][1][0], p->y, t0[1][0]);
		fp_mul(l[0][1][1][1], p->y, t0[1][1]);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp4_free(t0);
		fp4_free(t1);
		fp4_free(t2);
		fp4_free(t3);
		fp4_free(t4);
	}
}

#endif
