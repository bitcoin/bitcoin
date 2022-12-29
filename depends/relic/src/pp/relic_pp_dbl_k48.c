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
 * Implementation of Miller doubling for curves with embedding degree 48.
 *
 * @ingroup pp
 */

#include "relic_core.h"
#include "relic_pp.h"
#include "relic_util.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

static void ep8_dbl_basic(fp8_t s, fp8_t rx, fp8_t ry) {
	fp8_t t0, t1, t2;

	fp8_null(t0);
	fp8_null(t1);
	fp8_null(t2);

	RLC_TRY {
		fp8_new(t0);
		fp8_new(t1);
		fp8_new(t2);

		/* t0 = 1/(2 * y1). */
		fp8_dbl(t0, ry);
		fp8_inv(t0, t0);

		/* t1 = 3 * x1^2 + a. */
		fp8_sqr(t1, rx);
		fp8_copy(t2, t1);
		fp8_dbl(t1, t1);
		fp8_add(t1, t1, t2);

		/* a = 0. */
		/* t1 = (3 * x1^2 + a)/(2 * y1). */
		fp8_mul(t1, t1, t0);

		if (s != NULL) {
			fp8_copy(s, t1);
		}

		/* t2 = t1^2. */
		fp8_sqr(t2, t1);

		/* x3 = t1^2 - 2 * x1. */
		fp8_dbl(t0, rx);
		fp8_sub(t0, t2, t0);

		/* y3 = t1 * (x1 - x3) - y1. */
		fp8_sub(t2, rx, t0);
		fp8_mul(t1, t1, t2);

		fp8_sub(ry, t1, ry);

		fp8_copy(rx, t0);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp8_free(t0);
		fp8_free(t1);
		fp8_free(t2);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if EP_ADD == BASIC || !defined(STRIP)

void pp_dbl_k48_basic(fp48_t l, fp8_t rx, fp8_t ry, ep_t p) {
	fp8_t s, tx, ty;

	fp8_null(s);
	fp8_null(tx);
	fp8_null(ty);

	RLC_TRY {
		fp8_new(s);
		fp8_new(tx);
		fp8_new(ty);

		fp8_copy(tx, rx);
		fp8_copy(ty, ry);
		ep8_dbl_basic(s, rx, ry);
		fp48_zero(l);

		fp_mul(l[0][1][0][0][0], p->x, s[0][0][0]);
		fp_mul(l[0][1][0][0][1], p->x, s[0][0][1]);
		fp_mul(l[0][1][0][1][0], p->x, s[0][1][0]);
		fp_mul(l[0][1][0][1][1], p->x, s[0][1][1]);
		fp_mul(l[0][1][1][0][0], p->x, s[1][0][0]);
		fp_mul(l[0][1][1][0][1], p->x, s[1][0][1]);
		fp_mul(l[0][1][1][1][0], p->x, s[1][1][0]);
		fp_mul(l[0][1][1][1][1], p->x, s[1][1][1]);

		fp8_mul(l[0][0], s, tx);
		fp8_sub(l[0][0], ty, l[0][0]);

		fp_copy(l[1][1][0][0][0], p->y);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp8_free(s);
		fp8_free(tx);
		fp8_free(ty);
	}
}

#endif

#if EP_ADD == PROJC || !defined(STRIP)

void pp_dbl_k48_projc(fp48_t l, fp8_t rx, fp8_t ry, fp8_t rz, ep_t p) {
	fp8_t t0, t1, t2, t3, t4, t5, t6;

	fp8_null(t0);
	fp8_null(t1);
	fp8_null(t2);
	fp8_null(t3);
	fp8_null(t4);
	fp8_null(t5);
	fp8_null(t6);

	RLC_TRY {
		fp8_new(t0);
		fp8_new(t1);
		fp8_new(t2);
		fp8_new(t3);
		fp8_new(t4);
		fp8_new(t5);
		fp8_new(t6);

		/* A = x1^2. */
		fp8_sqr(t0, rx);
		/* B = y1^2. */
		fp8_sqr(t1, ry);
		/* C = z1^2. */
		fp8_sqr(t2, rz);
		/* D = 3bC, general b. */
		fp8_dbl(t3, t2);
		fp8_add(t3, t3, t2);
		fp8_zero(t4);
		fp_copy(t4[1][0][0], ep_curve_get_b());

		fp8_mul(t3, t3, t4);
		/* E = (x1 + y1)^2 - A - B. */
		fp8_add(t4, rx, ry);
		fp8_sqr(t4, t4);
		fp8_sub(t4, t4, t0);
		fp8_sub(t4, t4, t1);

		/* F = (y1 + z1)^2 - B - C. */
		fp8_add(t5, ry, rz);
		fp8_sqr(t5, t5);
		fp8_sub(t5, t5, t1);
		fp8_sub(t5, t5, t2);

		/* G = 3D. */
		fp8_dbl(t6, t3);
		fp8_add(t6, t6, t3);

		/* x3 = E * (B - G). */
		fp8_sub(rx, t1, t6);
		fp8_mul(rx, rx, t4);

		/* y3 = (B + G)^2 -12D^2. */
		fp8_add(t6, t6, t1);
		fp8_sqr(t6, t6);
		fp8_sqr(t2, t3);
		fp8_dbl(ry, t2);
		fp8_dbl(t2, ry);
		fp8_dbl(ry, t2);
		fp8_add(ry, ry, t2);
		fp8_sub(ry, t6, ry);

		/* z3 = 4B * F. */
		fp8_dbl(rz, t1);
		fp8_dbl(rz, rz);
		fp8_mul(rz, rz, t5);

		/* l11 = D - B. */
		fp8_sub(l[0][0], t3, t1);

		/* l10 = (3 * xp) * A. */
		fp_mul(l[0][1][0][0][0], p->x, t0[0][0][0]);
		fp_mul(l[0][1][0][0][1], p->x, t0[0][0][1]);
		fp_mul(l[0][1][0][1][0], p->x, t0[0][1][0]);
		fp_mul(l[0][1][0][1][1], p->x, t0[0][1][1]);
		fp_mul(l[0][1][1][0][0], p->x, t0[1][0][0]);
		fp_mul(l[0][1][1][0][1], p->x, t0[1][0][1]);
		fp_mul(l[0][1][1][1][0], p->x, t0[1][1][0]);
		fp_mul(l[0][1][1][1][1], p->x, t0[1][1][1]);

		/* l00 = F * (-yp). */
		fp_mul(l[1][1][0][0][0], p->y, t5[0][0][0]);
		fp_mul(l[1][1][0][0][1], p->y, t5[0][0][1]);
		fp_mul(l[1][1][0][1][0], p->y, t5[0][1][0]);
		fp_mul(l[1][1][0][1][1], p->y, t5[0][1][1]);
		fp_mul(l[1][1][1][0][0], p->y, t5[1][0][0]);
		fp_mul(l[1][1][1][0][1], p->y, t5[1][0][1]);
		fp_mul(l[1][1][1][1][0], p->y, t5[1][1][0]);
		fp_mul(l[1][1][1][1][1], p->y, t5[1][1][1]);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp8_free(t0);
		fp8_free(t1);
		fp8_free(t2);
		fp8_free(t3);
		fp8_free(t4);
		fp8_free(t5);
		fp8_free(t6);
	}
}

#endif
