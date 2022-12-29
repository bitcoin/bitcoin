/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2014 RELIC Authors
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
* Implementation of the point doubling on prime elliptic twisted Edwards curves.
*
* @version $Id$
* @ingroup ed
*/

#include "relic_core.h"
#include "relic_label.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if ED_ADD == BASIC || !defined(STRIP)

void ed_dbl_basic(ed_t r, const ed_t p) {
	fp_t t0, t1, t2;

	fp_null(t0);
	fp_null(t1);
	fp_null(t2);

	RLC_TRY {
		fp_new(t0);
		fp_new(t1);
		fp_new(t2);

		/* x3 = (x1*y1+y1*x1)/(1+d*x1*x1*y1*y1)
		 * y3 = (y1*y1-a*x1*x1)/(1-d*x1*x1*y1*y1) */

		fp_mul(t0, p->x, p->y);
		fp_sqr(t1, t0);

		fp_mul(t1, t1, core_get()->ed_d);
		fp_add_dig(t2, t1, 1);
		fp_inv(t2, t2);
		fp_sub_dig(t1, t1, 1);
		fp_neg(t1, t1);
		fp_inv(t1, t1);

		fp_dbl(t0, t0);
		fp_mul(t0, t0, t2);

		fp_sqr(t2, p->x);
		fp_mul(t2, t2, core_get()->ed_a);
		fp_sqr(r->y, p->y);
		fp_sub(r->y, r->y, t2);
		fp_mul(r->y, r->y, t1);

		fp_copy(r->x, t0);
		fp_copy(r->z, p->z);

		r->coord = BASIC;
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp_free(t0);
		fp_free(t1);
		fp_free(t2);
	}
}

#endif /* ED_ADD == BASIC */

#if ED_ADD == PROJC || !defined(STRIP)

void ed_dbl_projc(ed_t r, const ed_t p) {
	fp_t t0, t1, t2, t3, t4, t5, t6;

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

		// 3M + 4S + 1D + 7add

		/* B = (x1 + y1)^2 */
		fp_add(t0, p->x, p->y);
		fp_sqr(t0, t0);

		/* C = x1^2 */
		fp_sqr(t1, p->x);

		/*  D = y1^2 */
		fp_sqr(t2, p->y);

		/* E = a * C */
		fp_mul(t3, core_get()->ed_a, t1);

		/* F = E + D, H = Z^2 */
		fp_add(t4, t3, t2);
		fp_sqr(t5, p->z);

		// J = F - 2H
		fp_dbl(t6, t5);
		fp_sub(t6, t4, t6);

		// x3 = (B - C - D) * J
		fp_sub(r->x, t0, t1);
		fp_sub(r->x, r->x, t2);
		fp_mul(r->x, r->x, t6);

		// x3 = F * (E - D)
		fp_sub(r->y, t3, t2);
		fp_mul(r->y, t4, r->y);

		/* x3 = F * J */
		fp_mul(r->z, t4, t6);

		r->coord = PROJC;
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp_free(t0);
		fp_free(t1);
		fp_free(t2);
		fp_free(t3);
		fp_free(t4);
		fp_free(t5);
		fp_free(t6);
	}
}

#endif /* ED_PROJC */

#if ED_ADD == EXTND || !defined(STRIP)

void ed_dbl_extnd(ed_t r, const ed_t p) {
	fp_t t0, t1, t2, t3, t4;

	fp_null(t0);
	fp_null(t1);
	fp_null(t2);
	fp_null(t3);
	fp_null(t4);

	RLC_TRY {
		fp_new(t0);
		fp_new(t1);
		fp_new(t2);
		fp_new(t3);
		fp_new(t4);

		// efd: dbl-2008-hwcd, rfc edition
		// 4M + 4S + 1D + 7add

		/* A = X^2, B = Y^2 */
		fp_sqr(t0, p->x);
		fp_sqr(t1, p->y);

		/* C = 2 * Z^2 */
		fp_sqr(r->z, p->z);
		fp_dbl(r->z, r->z);

		/* D = a * A */
		fp_mul(r->t, core_get()->ed_a, t0);

		/* E = (X + Y) ^ 2 - A - B */
		fp_add(t2, p->x, p->y);
		fp_sqr(t2, t2);
		fp_sub(t2, t2, t0);
		fp_sub(t2, t2, t1);

		/* G = D + B, F = G - C, H = D - B */
		fp_add(t4, r->t, t1);
		fp_sub(t3, t4, r->z);
		fp_sub(r->z, r->t, t1);

		/* X = E * F */
		fp_mul(r->x, t2, t3);
		/* Y = G * H */
		fp_mul(r->y, t4, r->z);
		if (r->coord != EXTND) {
			/* T = E * H */
			fp_mul(r->t, t2, r->z);
		}
		/* Z = F * G */
		fp_mul(r->z, t3, t4);

		r->coord = PROJC;
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp_free(t0);
		fp_free(t1);
		fp_free(t2);
		fp_free(t3);
		fp_free(t4);
	}
}

#endif /* ED_EXTND */
