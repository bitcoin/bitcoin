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
 * Implementation of pairing computation for curves with embedding degree 54.
 *
 * @ingroup pp
 */

#include "relic_core.h"
#include "relic_pp.h"
#include "relic_util.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

static void ep9_dbl_basic(fp9_t s, fp9_t rx, fp9_t ry) {
	fp9_t t0, t1, t2;

	fp9_null(t0);
	fp9_null(t1);
	fp9_null(t2);

	RLC_TRY {
		fp9_new(t0);
		fp9_new(t1);
		fp9_new(t2);

		/* t0 = 1/(2 * y1). */
		fp9_dbl(t0, ry);
		fp9_inv(t0, t0);

		/* t1 = 3 * x1^2 + a. */
		fp9_sqr(t1, rx);
		fp9_copy(t2, t1);
		fp9_dbl(t1, t1);
		fp9_add(t1, t1, t2);

		/* a = 0. */
		/* t1 = (3 * x1^2 + a)/(2 * y1). */
		fp9_mul(t1, t1, t0);

		if (s != NULL) {
			fp9_copy(s, t1);
		}

		/* t2 = t1^2. */
		fp9_sqr(t2, t1);

		/* x3 = t1^2 - 2 * x1. */
		fp9_dbl(t0, rx);
		fp9_sub(t0, t2, t0);

		/* y3 = t1 * (x1 - x3) - y1. */
		fp9_sub(t2, rx, t0);
		fp9_mul(t1, t1, t2);

		fp9_sub(ry, t1, ry);

		fp9_copy(rx, t0);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp9_free(t0);
		fp9_free(t1);
		fp9_free(t2);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if EP_ADD == PROJC || !defined(STRIP)

void pp_dbl_k54_basic(fp54_t l, fp9_t rx, fp9_t ry, ep_t p) {
	fp9_t s, tx, ty;

	fp9_null(s);
	fp9_null(tx);
	fp9_null(ty);

	RLC_TRY {
		fp9_new(s);
		fp9_new(tx);
		fp9_new(ty);

		fp9_copy(tx, rx);
		fp9_copy(ty, ry);
		ep9_dbl_basic(s, rx, ry);
		fp54_zero(l);

		fp_mul(l[2][0][0][0], p->x, s[0][0]);
		fp_mul(l[2][0][0][1], p->x, s[0][1]);
		fp_mul(l[2][0][0][2], p->x, s[0][2]);
		fp_mul(l[2][0][1][0], p->x, s[1][0]);
		fp_mul(l[2][0][1][1], p->x, s[1][1]);
		fp_mul(l[2][0][1][2], p->x, s[1][2]);
		fp_mul(l[2][0][2][0], p->x, s[2][0]);
		fp_mul(l[2][0][2][1], p->x, s[2][1]);
		fp_mul(l[2][0][2][2], p->x, s[2][2]);

		fp9_mul(l[0][0], s, tx);
		fp9_sub(l[0][0], ty, l[0][0]);

		fp_copy(l[0][1][0][0], p->y);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp9_free(s);
		fp9_free(tx);
		fp9_free(ty);
	}
}

#endif

#if EP_ADD == PROJC || !defined(STRIP)

void pp_dbl_k54_projc(fp54_t l, fp9_t rx, fp9_t ry, fp9_t rz, ep_t p) {
	fp9_t t0, t1, t2, t3, t4, t5, t6;

	fp9_null(t0);
	fp9_null(t1);
	fp9_null(t2);
	fp9_null(t3);
	fp9_null(t4);
	fp9_null(t5);
	fp9_null(t6);

	RLC_TRY {
		fp9_new(t0);
		fp9_new(t1);
		fp9_new(t2);
		fp9_new(t3);
		fp9_new(t4);
		fp9_new(t5);
		fp9_new(t6);

		/* A = x1^2. */
		fp9_sqr(t0, rx);
		/* B = y1^2. */
		fp9_sqr(t1, ry);
		/* C = z1^2. */
		fp9_sqr(t2, rz);
		/* D = 3bC, general b. */
		fp9_dbl(t3, t2);
		fp9_add(t3, t3, t2);
		fp9_zero(t4);
		fp_copy(t4[1][0], ep_curve_get_b());

		fp9_mul(t3, t3, t4);
		/* E = (x1 + y1)^2 - A - B. */
		fp9_add(t4, rx, ry);
		fp9_sqr(t4, t4);
		fp9_sub(t4, t4, t0);
		fp9_sub(t4, t4, t1);

		/* F = (y1 + z1)^2 - B - C. */
		fp9_add(t5, ry, rz);
		fp9_sqr(t5, t5);
		fp9_sub(t5, t5, t1);
		fp9_sub(t5, t5, t2);

		/* G = 3D. */
		fp9_dbl(t6, t3);
		fp9_add(t6, t6, t3);

		/* x3 = E * (B - G). */
		fp9_sub(rx, t1, t6);
		fp9_mul(rx, rx, t4);

		/* y3 = (B + G)^2 -12D^2. */
		fp9_add(t6, t6, t1);
		fp9_sqr(t6, t6);
		fp9_sqr(t2, t3);
		fp9_dbl(ry, t2);
		fp9_dbl(t2, ry);
		fp9_dbl(ry, t2);
		fp9_add(ry, ry, t2);
		fp9_sub(ry, t6, ry);

		/* z3 = 4B * F. */
		fp9_dbl(rz, t1);
		fp9_dbl(rz, rz);
		fp9_mul(rz, rz, t5);

		/* l11 = D - B. */
		fp9_sub(l[0][0], t3, t1);

		/* l10 = (3 * xp) * A. */
		fp_mul(l[2][0][0][0], p->x, t0[0][0]);
		fp_mul(l[2][0][0][1], p->x, t0[0][1]);
		fp_mul(l[2][0][0][2], p->x, t0[0][2]);
		fp_mul(l[2][0][1][0], p->x, t0[1][0]);
		fp_mul(l[2][0][1][1], p->x, t0[1][1]);
		fp_mul(l[2][0][1][2], p->x, t0[1][2]);
		fp_mul(l[2][0][2][0], p->x, t0[2][0]);
		fp_mul(l[2][0][2][1], p->x, t0[2][1]);
		fp_mul(l[2][0][2][2], p->x, t0[2][2]);

		/* l00 = F * (-yp). */
		fp_mul(l[0][1][0][0], p->y, t5[0][0]);
		fp_mul(l[0][1][0][1], p->y, t5[0][1]);
		fp_mul(l[0][1][0][2], p->y, t5[0][2]);
		fp_mul(l[0][1][1][0], p->y, t5[1][0]);
		fp_mul(l[0][1][1][1], p->y, t5[1][1]);
		fp_mul(l[0][1][1][2], p->y, t5[1][2]);
		fp_mul(l[0][1][2][0], p->y, t5[2][0]);
		fp_mul(l[0][1][2][1], p->y, t5[2][1]);
		fp_mul(l[0][1][2][2], p->y, t5[2][2]);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp9_free(t0);
		fp9_free(t1);
		fp9_free(t2);
		fp9_free(t3);
		fp9_free(t4);
		fp9_free(t5);
		fp9_free(t6);
	}
}

#endif
