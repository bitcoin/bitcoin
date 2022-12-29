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
 * Implementation of the point addition on prime elliptic twisted Edwards curves.
 *
 * @version $Id$
 * @ingroup ed
 */

#include <assert.h>

#include "relic_core.h"
#include "relic_label.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if ED_ADD == BASIC || !defined(STRIP)

void ed_add_basic(ed_t r, const ed_t p, const ed_t q) {
	fp_t t0, t1, t2;

	fp_null(t0);
	fp_null(t1);
	fp_null(t2);

	RLC_TRY {
		fp_new(t0);
		fp_new(t1);
		fp_new(t2);

		/* x3 = (x1*y2+y1*x2)/(1+d*x1*x2*y1*y2)
		 * y3 = (y1*y2-a*x1*x2)/(1-d*x1*x2*y1*y2) */

		fp_mul(t0, p->x, q->y);
		fp_mul(t1, p->y, q->x);
		fp_add(t0, t0, t1);

		fp_mul(t1, p->x, q->x);
		fp_mul(t2, p->y, q->y);
		fp_mul(t1, t1, t2);
		fp_mul(t1, t1, core_get()->ed_d);
		fp_add_dig(t2, t1, 1);
		fp_inv(t2, t2);
		fp_sub_dig(t1, t1, 1);
		fp_neg(t1, t1);
		fp_inv(t1, t1);

		fp_mul(t0, t0, t2);

		fp_mul(r->y, p->y, q->y);
		fp_mul(t2, p->x, q->x);
		fp_mul(t2, t2, core_get()->ed_a);
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

void ed_sub_basic(ed_t r, const ed_t p, const ed_t q) {
	ed_t t;

	ed_null(t);

	if (p == q) {
		ed_set_infty(r);
		return;
	}

	RLC_TRY {
		ed_new(t);

		ed_neg_basic(t, q);
		ed_add_basic(r, p, t);
		r->coord = BASIC;
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ed_free(t);
	}
}

#endif /* ED_ADD == BASIC */

#if ED_ADD == PROJC || !defined(STRIP)

void ed_add_projc(ed_t r, const ed_t p, const ed_t q) {
	fp_t t0, t1, t2, t3, t4, t5, t6, t7;

	fp_null(t0);
	fp_null(t1);
	fp_null(t2);
	fp_null(t3);
	fp_null(t4);
	fp_null(t5);
	fp_null(t6);
	fp_null(t7);

	RLC_TRY {
		fp_new(t0);
		fp_new(t1);
		fp_new(t2);
		fp_new(t3);
		fp_new(t4);
		fp_new(t5);
		fp_new(t6);
		fp_new(t7);

		/* A = z1 * z2, B = A^2 */
		fp_mul(t0, p->z, q->z);
		fp_sqr(t1, t0);

		/* C = x1 * x2, D = y1 * y2 */
		fp_mul(t2, p->x, q->x);
		fp_mul(t3, p->y, q->y);

		/* E = d * C * D */
		fp_mul(t4, core_get()->ed_d, t2);
		fp_mul(t4, t4, t3);

		/* F = B - E */
		fp_sub(t5, t1, t4);

		/* G = B + E */
		fp_add(t6, t1, t4);

		/* x3 = A * F * ((x1 + y1) * (x2 + y2) - C - D) */
		fp_mul(t7, t0, t5);
		fp_add(r->z, p->x, p->y);
		fp_add(r->x, q->x, q->y);
		fp_mul(r->x, r->z, r->x);
		fp_sub(r->x, r->x, t2);
		fp_sub(r->x, r->x, t3);
		fp_mul(r->x, t7, r->x);

		/* y3 = A * G * (D - a * C) */
		fp_mul(r->z, t0, t6);
		fp_mul(r->y, core_get()->ed_a, t2);
		fp_sub(r->y, t3, r->y);
		fp_mul(r->y, r->z, r->y);

		/* z3 = F * G */
		fp_mul(r->z, t5, t6);

		r->coord = PROJC;
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT)
	} RLC_FINALLY {
		fp_free(t0);
		fp_free(t1);
		fp_free(t2);
		fp_free(t3);
		fp_free(t4);
		fp_free(t5);
		fp_free(t6);
		fp_free(t7);
	}
}

#endif /* ED_ADD == PROJC */

#if ED_ADD == PROJC || !defined(STRIP)

void ed_sub_projc(ed_t r, const ed_t p, const ed_t q) {
	ed_t t;

	ed_null(t);

	if (p == q) {
		ed_set_infty(r);
		return;
	}

	RLC_TRY {
		ed_new(t);

		ed_neg_projc(t, q);
		ed_add_projc(r, p, t);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ed_free(t);
	}
}

#endif /* ED_ADD == PROJC || ED_ADD == EXTND */

#if ED_ADD == EXTND || !defined(STRIP)

void ed_add_extnd(ed_t r, const ed_t p, const ed_t q) {
	fp_t t0;
	fp_t t1;
	fp_t t2;
	fp_t t3;
	fp_t t4;

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

		/* A = x1 * x2, B = y1 * y2 */
		fp_mul(t0, p->x, q->x);
		fp_mul(t1, p->y, q->y);

		/* C = d * t1 * t2 */
		fp_mul(t2, core_get()->ed_d, p->t);
		fp_mul(r->t, t2, q->t);

		/* D = z1 * z2 */
		fp_mul(r->z, p->z, q->z);

		/* E = (x1 + y1) * (x2 + y2) - A - B */
		fp_add(t2, p->x, p->y);
		fp_add(t3, q->x, q->y);
		fp_mul(t2, t2, t3);
		fp_sub(t2, t2, t0);
		fp_sub(t2, t2, t1);

		/* F = D - C */
		fp_sub(t3, r->z, r->t);

		/* G = D + C */
		fp_add(t4, r->z, r->t);

		/* H = B - aA */
		fp_mul(r->x, core_get()->ed_a, t0);
		fp_sub(r->z, t1, r->x);

		/* x3 = E * F, y3 = G * H, t3 = E * H, z3 = F * G */
		fp_mul(r->x, t2, t3);
		fp_mul(r->y, t4, r->z);
		fp_mul(r->t, t2, r->z);
		fp_mul(r->z, t3, t4);

		r->coord = PROJC;
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT)
	} RLC_FINALLY {
		fp_free(t0);
		fp_free(t1);
		fp_free(t2);
		fp_free(t3);
		fp_free(t4);
	}
}

void ed_sub_extnd(ed_t r, const ed_t p, const ed_t q) {
	ed_t t;

	ed_null(t);

	if (p == q) {
		ed_set_infty(r);
		return;
	}

	RLC_TRY {
		ed_new(t);

		ed_neg_projc(t, q);
		ed_add_extnd(r, p, t);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ed_free(t);
	}
}

#endif
