/*
 * RELIC is an Efficient LIbrary for Cryptography
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
 * Implementation of addition on prime elliptic curves over quartic
 * extensions.
 *
 * @ingroup epx
 */

#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if EP_ADD == BASIC || !defined(STRIP)

/**
 * Adds two points represented in affine coordinates on an ordinary prime
 * elliptic curve.
 *
 * @param r					- the result.
 * @param s					- the resulting slope.
 * @param p					- the first point to add.
 * @param q					- the second point to add.
 */
static void ep4_add_basic_imp(ep4_t r, fp4_t s, ep4_t p, ep4_t q) {
	fp4_t t0, t1, t2;

	fp4_null(t0);
	fp4_null(t1);
	fp4_null(t2);

	RLC_TRY {
		fp4_new(t0);
		fp4_new(t1);
		fp4_new(t2);

		/* t0 = x2 - x1. */
		fp4_sub(t0, q->x, p->x);
		/* t1 = y2 - y1. */
		fp4_sub(t1, q->y, p->y);

		/* If t0 is zero. */
		if (fp4_is_zero(t0)) {
			if (fp4_is_zero(t1)) {
				/* If t1 is zero, q = p, should have doubled. */
				ep4_dbl_slp_basic(r, s, p);
			} else {
				/* If t1 is not zero and t0 is zero, q = -p and r = infty. */
				ep4_set_infty(r);
			}
		} else {
			/* t2 = 1/(x2 - x1). */
			fp4_inv(t2, t0);
			/* t2 = lambda = (y2 - y1)/(x2 - x1). */
			fp4_mul(t2, t1, t2);

			/* x3 = lambda^2 - x2 - x1. */
			fp4_sqr(t1, t2);
			fp4_sub(t0, t1, p->x);
			fp4_sub(t0, t0, q->x);

			/* y3 = lambda * (x1 - x3) - y1. */
			fp4_sub(t1, p->x, t0);
			fp4_mul(t1, t2, t1);
			fp4_sub(r->y, t1, p->y);

			fp4_copy(r->x, t0);
			fp4_copy(r->z, p->z);

			if (s != NULL) {
				fp4_copy(s, t2);
			}

			r->coord = BASIC;
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp4_free(t0);
		fp4_free(t1);
		fp4_free(t2);
	}
}

#endif /* EP_ADD == BASIC */

#if EP_ADD == PROJC || !defined(STRIP)

#if defined(EP_MIXED) || !defined(STRIP)

/**
 * Adds a point represented in affine coordinates to a point represented in
 * projective coordinates.
 *
 * @param r					- the result.
 * @param s					- the slope.
 * @param p					- the affine point.
 * @param q					- the projective point.
 */
static void ep4_add_projc_mix(ep4_t r, ep4_t p, ep4_t q) {
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

		if (p->coord != BASIC) {
			/* t0 = z1^2. */
			fp4_sqr(t0, p->z);

			/* t3 = U2 = x2 * z1^2. */
			fp4_mul(t3, q->x, t0);

			/* t1 = S2 = y2 * z1^3. */
			fp4_mul(t1, t0, p->z);
			fp4_mul(t1, t1, q->y);

			/* t3 = H = U2 - x1. */
			fp4_sub(t3, t3, p->x);

			/* t1 = R = 2 * (S2 - y1). */
			fp4_sub(t1, t1, p->y);
		} else {
			/* H = x2 - x1. */
			fp4_sub(t3, q->x, p->x);

			/* t1 = R = 2 * (y2 - y1). */
			fp4_sub(t1, q->y, p->y);
		}

		/* t2 = HH = H^2. */
		fp4_sqr(t2, t3);

		/* If E is zero. */
		if (fp4_is_zero(t3)) {
			if (fp4_is_zero(t1)) {
				/* If I is zero, p = q, should have doubled. */
				ep4_dbl_projc(r, p);
			} else {
				/* If I is not zero, q = -p, r = infinity. */
				ep4_set_infty(r);
			}
		} else {
			/* t5 = J = H * HH. */
			fp4_mul(t5, t3, t2);

			/* t4 = V = x1 * HH. */
			fp4_mul(t4, p->x, t2);

			/* x3 = R^2 - J - 2 * V. */
			fp4_sqr(r->x, t1);
			fp4_sub(r->x, r->x, t5);
			fp4_dbl(t6, t4);
			fp4_sub(r->x, r->x, t6);

			/* y3 = R * (V - x3) - Y1 * J. */
			fp4_sub(t4, t4, r->x);
			fp4_mul(t4, t4, t1);
			fp4_mul(t1, p->y, t5);
			fp4_sub(r->y, t4, t1);

			if (p->coord != BASIC) {
				/* z3 = z1 * H. */
				fp4_mul(r->z, p->z, t3);
			} else {
				/* z3 = H. */
				fp4_copy(r->z, t3);
			}
		}
		r->coord = PROJC;
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

/**
 * Adds two points represented in projective coordinates on an ordinary prime
 * elliptic curve.
 *
 * @param r					- the result.
 * @param p					- the first point to add.
 * @param q					- the second point to add.
 */
static void ep4_add_projc_imp(ep4_t r, ep4_t p, ep4_t q) {
#if defined(EP_MIXED) && defined(STRIP)
	ep4_add_projc_mix(r, p, q);
#else /* General addition. */
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

		if (q->coord == BASIC) {
			ep4_add_projc_mix(r, p, q);
		} else {
			/* t0 = z1^2. */
			fp4_sqr(t0, p->z);

			/* t1 = z2^2. */
			fp4_sqr(t1, q->z);

			/* t2 = U1 = x1 * z2^2. */
			fp4_mul(t2, p->x, t1);

			/* t3 = U2 = x2 * z1^2. */
			fp4_mul(t3, q->x, t0);

			/* t6 = z1^2 + z2^2. */
			fp4_add(t6, t0, t1);

			/* t0 = S2 = y2 * z1^3. */
			fp4_mul(t0, t0, p->z);
			fp4_mul(t0, t0, q->y);

			/* t1 = S1 = y1 * z2^3. */
			fp4_mul(t1, t1, q->z);
			fp4_mul(t1, t1, p->y);

			/* t3 = H = U2 - U1. */
			fp4_sub(t3, t3, t2);

			/* t0 = R = 2 * (S2 - S1). */
			fp4_sub(t0, t0, t1);

			fp4_dbl(t0, t0);

			/* If E is zero. */
			if (fp4_is_zero(t3)) {
				if (fp4_is_zero(t0)) {
					/* If I is zero, p = q, should have doubled. */
					ep4_dbl_projc(r, p);
				} else {
					/* If I is not zero, q = -p, r = infinity. */
					ep4_set_infty(r);
				}
			} else {
				/* t4 = I = (2*H)^2. */
				fp4_dbl(t4, t3);
				fp4_sqr(t4, t4);

				/* t5 = J = H * I. */
				fp4_mul(t5, t3, t4);

				/* t4 = V = U1 * I. */
				fp4_mul(t4, t2, t4);

				/* x3 = R^2 - J - 2 * V. */
				fp4_sqr(r->x, t0);
				fp4_sub(r->x, r->x, t5);
				fp4_dbl(t2, t4);
				fp4_sub(r->x, r->x, t2);

				/* y3 = R * (V - x3) - 2 * S1 * J. */
				fp4_sub(t4, t4, r->x);
				fp4_mul(t4, t4, t0);
				fp4_mul(t1, t1, t5);
				fp4_dbl(t1, t1);
				fp4_sub(r->y, t4, t1);

				/* z3 = ((z1 + z2)^2 - z1^2 - z2^2) * H. */
				fp4_add(r->z, p->z, q->z);
				fp4_sqr(r->z, r->z);
				fp4_sub(r->z, r->z, t6);
				fp4_mul(r->z, r->z, t3);
			}
		}
		r->coord = PROJC;
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
#endif
}

#endif /* EP_ADD == PROJC */

/*============================================================================*/
	/* Public definitions                                                         */
/*============================================================================*/

#if EP_ADD == BASIC || !defined(STRIP)

void ep4_add_basic(ep4_t r, ep4_t p, ep4_t q) {
	if (ep4_is_infty(p)) {
		ep4_copy(r, q);
		return;
	}

	if (ep4_is_infty(q)) {
		ep4_copy(r, p);
		return;
	}

	ep4_add_basic_imp(r, NULL, p, q);
}

void ep4_add_slp_basic(ep4_t r, fp4_t s, ep4_t p, ep4_t q) {
	if (ep4_is_infty(p)) {
		ep4_copy(r, q);
		return;
	}

	if (ep4_is_infty(q)) {
		ep4_copy(r, p);
		return;
	}

	ep4_add_basic_imp(r, s, p, q);
}

#endif

#if EP_ADD == PROJC || !defined(STRIP)

void ep4_add_projc(ep4_t r, ep4_t p, ep4_t q) {
	if (ep4_is_infty(p)) {
		ep4_copy(r, q);
		return;
	}

	if (ep4_is_infty(q)) {
		ep4_copy(r, p);
		return;
	}

	if (p == q) {
		/* TODO: This is a quick hack. Should we fix this? */
		ep4_dbl(r, p);
		return;
	}

	ep4_add_projc_imp(r, p, q);
}

#endif

void ep4_sub(ep4_t r, ep4_t p, ep4_t q) {
	ep4_t t;

	ep4_null(t);

	if (p == q) {
		ep4_set_infty(r);
		return;
	}

	RLC_TRY {
		ep4_new(t);

		ep4_neg(t, q);
		ep4_add(r, p, t);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ep4_free(t);
	}
}
