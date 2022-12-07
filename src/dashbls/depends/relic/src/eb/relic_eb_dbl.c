/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2009 RELIC Authors
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
 * Implementation of point doubling on binary elliptic curves.
 *
 * @ingroup eb
 */

#include "string.h"

#include "relic_core.h"
#include "relic_eb.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if EB_ADD == BASIC || !defined(STRIP)

/**
 * Doubles a point represented in affine coordinates on an ordinary binary
 * elliptic curve.
 *
 * @param[out] r				- the result.
 * @param[in] p					- the point to double.
 */
static void eb_dbl_basic_imp(eb_t r, const eb_t p) {
	fb_t t0, t1, t2;

	fb_null(t0);
	fb_null(t1);
	fb_null(t2);

	RLC_TRY {
		fb_new(t0);
		fb_new(t1);
		fb_new(t2);

		/* t0 = 1/x1. */
		fb_inv(t0, p->x);
		/* t0 = y1/x1. */
		fb_mul(t0, t0, p->y);
		/* t0 = lambda = x1 + y1/x1. */
		fb_add(t0, t0, p->x);
		/* t1 = lambda^2. */
		fb_sqr(t1, t0);
		/* t2 = lambda^2 + lambda. */
		fb_add(t2, t1, t0);

		/* t2 = lambda^2 + lambda + a2. */
		switch (eb_curve_opt_a()) {
			case RLC_ZERO:
				break;
			case RLC_ONE:
				fb_add_dig(t2, t2, (dig_t)1);
				break;
			case RLC_TINY:
				fb_add_dig(t2, t2, eb_curve_get_a()[0]);
				break;
			default:
				fb_add(t2, t2, eb_curve_get_a());
				break;
		}

		/* t1 = x1 + x3. */
		fb_add(t1, t2, p->x);

		/* t1 = lambda * (x1 + x3). */
		fb_mul(t1, t0, t1);

		fb_copy(r->x, t2);
		/* y3 = lambda * (x1 + x3) + x3 + y1. */
		fb_add(t1, t1, r->x);
		fb_add(r->y, t1, p->y);

		fb_copy(r->z, p->z);

		r->coord = BASIC;
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fb_free(t0);
		fb_free(t1);
		fb_free(t2);
	}
}

#endif /* EB_ADD == BASIC */

#if EB_ADD == PROJC || !defined(STRIP)

/**
 * Doubles a point represented in projective coordinates on an ordinary binary
 * elliptic curve.
 *
 * @param[out] r				- the result.
 * @param[in] p					- the point to double.
 */
static void eb_dbl_projc_imp(eb_t r, const eb_t p) {
	fb_t t0, t1;

	fb_null(t0);
	fb_null(t1);

	RLC_TRY {
		fb_new(t0);
		fb_new(t1);

		/* t0 = B = x1^2. */
		fb_sqr(t0, p->x);
		/* C = B + y1. */
		fb_add(r->y, t0, p->y);

		if (p->coord != BASIC) {
			/* A = x1 * z1. */
			fb_mul(t1, p->x, p->z);
			/* z3 = A^2. */
			fb_sqr(r->z, t1);
		} else {
			/* if z1 = 1, A = x1. */
			fb_copy(t1, p->x);
			/* if z1 = 1, z3 = x1^2. */
			fb_copy(r->z, t0);
		}

		/* t1 = D = A * C. */
		fb_mul(t1, t1, r->y);

		/* C^2 + D. */
		fb_sqr(r->y, r->y);
		fb_add(r->x, t1, r->y);

		/* C^2 + D + a2 * z3. */
		switch (eb_curve_opt_a()) {
			case RLC_ZERO:
				break;
			case RLC_ONE:
				fb_add(r->x, r->z, r->x);
				break;
			case RLC_TINY:
				fb_mul_dig(r->y, r->z, eb_curve_get_a()[0]);
				fb_add(r->x, r->y, r->x);
				break;
			default:
				fb_mul(r->y, r->z, eb_curve_get_a());
				fb_add(r->x, r->y, r->x);
				break;
		}

		/* t1 = (D + z3). */
		fb_add(t1, t1, r->z);
		/* t0 = B^2. */
		fb_sqr(t0, t0);
		/* t0 = B^2 * z3. */
		fb_mul(t0, t0, r->z);
		/* y3 = (D + z3) * r3 + B^2 * z3. */
		fb_mul(r->y, t1, r->x);
		fb_add(r->y, r->y, t0);

		r->coord = PROJC;
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fb_free(t0);
		fb_free(t1);
	}
}

#endif /* EB_ADD == PROJC */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if EB_ADD == BASIC || !defined(STRIP)

void eb_dbl_basic(eb_t r, const eb_t p) {
	if (eb_is_infty(p)) {
		eb_set_infty(r);
		return;
	}

	eb_dbl_basic_imp(r, p);
}

#endif

#if EB_ADD == PROJC || !defined(STRIP)

void eb_dbl_projc(eb_t r, const eb_t p) {

	if (eb_is_infty(p)) {
		eb_set_infty(r);
		return;
	}

	eb_dbl_projc_imp(r, p);
}

#endif
