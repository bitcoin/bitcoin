/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2017 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * RELIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RELIC. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * Implementation of addition on prime elliptic curves over quadratic
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
static void ep2_add_basic_imp(ep2_t r, fp2_t s, ep2_t p, ep2_t q) {
	fp2_t t0, t1, t2;

	fp2_null(t0);
	fp2_null(t1);
	fp2_null(t2);

	TRY {
		fp2_new(t0);
		fp2_new(t1);
		fp2_new(t2);

		/* t0 = x2 - x1. */
		fp2_sub(t0, q->x, p->x);
		/* t1 = y2 - y1. */
		fp2_sub(t1, q->y, p->y);

		/* If t0 is zero. */
		if (fp2_is_zero(t0)) {
			if (fp2_is_zero(t1)) {
				/* If t1 is zero, q = p, should have doubled. */
				ep2_dbl_basic(r, p);
			} else {
				/* If t1 is not zero and t0 is zero, q = -p and r = infty. */
				ep2_set_infty(r);
			}
		} else {
			/* t2 = 1/(x2 - x1). */
			fp2_inv(t2, t0);
			/* t2 = lambda = (y2 - y1)/(x2 - x1). */
			fp2_mul(t2, t1, t2);

			/* x3 = lambda^2 - x2 - x1. */
			fp2_sqr(t1, t2);
			fp2_sub(t0, t1, p->x);
			fp2_sub(t0, t0, q->x);

			/* y3 = lambda * (x1 - x3) - y1. */
			fp2_sub(t1, p->x, t0);
			fp2_mul(t1, t2, t1);
			fp2_sub(r->y, t1, p->y);

			fp2_copy(r->x, t0);
			fp2_copy(r->z, p->z);

			if (s != NULL) {
				fp2_copy(s, t2);
			}

			r->norm = 1;
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp2_free(t0);
		fp2_free(t1);
		fp2_free(t2);
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
static void ep2_add_projc_mix(ep2_t r, ep2_t p, ep2_t q) {
	fp2_t t0, t1, t2, t3, t4, t5, t6;

	fp2_null(t0);
	fp2_null(t1);
	fp2_null(t2);
	fp2_null(t3);
	fp2_null(t4);
	fp2_null(t5);
	fp2_null(t6);

	TRY {
		fp2_new(t0);
		fp2_new(t1);
		fp2_new(t2);
		fp2_new(t3);
		fp2_new(t4);
		fp2_new(t5);
		fp2_new(t6);

		if (!p->norm) {
			/* t0 = z1^2. */
			fp2_sqr(t0, p->z);

			/* t3 = U2 = x2 * z1^2. */
			fp2_mul(t3, q->x, t0);

			/* t1 = S2 = y2 * z1^3. */
			fp2_mul(t1, t0, p->z);
			fp2_mul(t1, t1, q->y);

			/* t3 = H = U2 - x1. */
			fp2_sub(t3, t3, p->x);

			/* t1 = R = 2 * (S2 - y1). */
			fp2_sub(t1, t1, p->y);
		} else {
			/* H = x2 - x1. */
			fp2_sub(t3, q->x, p->x);

			/* t1 = R = 2 * (y2 - y1). */
			fp2_sub(t1, q->y, p->y);
		}

		/* t2 = HH = H^2. */
		fp2_sqr(t2, t3);

		/* If E is zero. */
		if (fp2_is_zero(t3)) {
			if (fp2_is_zero(t1)) {
				/* If I is zero, p = q, should have doubled. */
				ep2_dbl_projc(r, p);
			} else {
				/* If I is not zero, q = -p, r = infinity. */
				ep2_set_infty(r);
			}
		} else {
			/* t5 = J = H * HH. */
			fp2_mul(t5, t3, t2);

			/* t4 = V = x1 * HH. */
			fp2_mul(t4, p->x, t2);

			/* x3 = R^2 - J - 2 * V. */
			fp2_sqr(r->x, t1);
			fp2_sub(r->x, r->x, t5);
			fp2_dbl(t6, t4);
			fp2_sub(r->x, r->x, t6);

			/* y3 = R * (V - x3) - Y1 * J. */
			fp2_sub(t4, t4, r->x);
			fp2_mul(t4, t4, t1);
			fp2_mul(t1, p->y, t5);
			fp2_sub(r->y, t4, t1);

			if (!p->norm) {
				/* z3 = z1 * H. */
				fp2_mul(r->z, p->z, t3);
			} else {
				/* z3 = H. */
				fp2_copy(r->z, t3);
			}
		}
		r->norm = 0;
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
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

/**
 * Adds two points represented in projective coordinates on an ordinary prime
 * elliptic curve.
 *
 * @param r					- the result.
 * @param p					- the first point to add.
 * @param q					- the second point to add.
 */
static void ep2_add_projc_imp(ep2_t r, ep2_t p, ep2_t q) {
#if defined(EP_MIXED) && defined(STRIP)
	ep2_add_projc_mix(r, p, q);
#else /* General addition. */
	fp2_t t0, t1, t2, t3, t4, t5, t6;

	fp2_null(t0);
	fp2_null(t1);
	fp2_null(t2);
	fp2_null(t3);
	fp2_null(t4);
	fp2_null(t5);
	fp2_null(t6);

	TRY {
		fp2_new(t0);
		fp2_new(t1);
		fp2_new(t2);
		fp2_new(t3);
		fp2_new(t4);
		fp2_new(t5);
		fp2_new(t6);

		if (q->norm) {
			ep2_add_projc_mix(r, p, q);
		} else {
			/* t0 = z1^2. */
			fp2_sqr(t0, p->z);

			/* t1 = z2^2. */
			fp2_sqr(t1, q->z);

			/* t2 = U1 = x1 * z2^2. */
			fp2_mul(t2, p->x, t1);

			/* t3 = U2 = x2 * z1^2. */
			fp2_mul(t3, q->x, t0);

			/* t6 = z1^2 + z2^2. */
			fp2_add(t6, t0, t1);

			/* t0 = S2 = y2 * z1^3. */
			fp2_mul(t0, t0, p->z);
			fp2_mul(t0, t0, q->y);

			/* t1 = S1 = y1 * z2^3. */
			fp2_mul(t1, t1, q->z);
			fp2_mul(t1, t1, p->y);

			/* t3 = H = U2 - U1. */
			fp2_sub(t3, t3, t2);

			/* t0 = R = 2 * (S2 - S1). */
			fp2_sub(t0, t0, t1);

			fp2_dbl(t0, t0);

			/* If E is zero. */
			if (fp2_is_zero(t3)) {
				if (fp2_is_zero(t0)) {
					/* If I is zero, p = q, should have doubled. */
					ep2_dbl_projc(r, p);
				} else {
					/* If I is not zero, q = -p, r = infinity. */
					ep2_set_infty(r);
				}
			} else {
				/* t4 = I = (2*H)^2. */
				fp2_dbl(t4, t3);
				fp2_sqr(t4, t4);

				/* t5 = J = H * I. */
				fp2_mul(t5, t3, t4);

				/* t4 = V = U1 * I. */
				fp2_mul(t4, t2, t4);

				/* x3 = R^2 - J - 2 * V. */
				fp2_sqr(r->x, t0);
				fp2_sub(r->x, r->x, t5);
				fp2_dbl(t2, t4);
				fp2_sub(r->x, r->x, t2);

				/* y3 = R * (V - x3) - 2 * S1 * J. */
				fp2_sub(t4, t4, r->x);
				fp2_mul(t4, t4, t0);
				fp2_mul(t1, t1, t5);
				fp2_dbl(t1, t1);
				fp2_sub(r->y, t4, t1);

				/* z3 = ((z1 + z2)^2 - z1^2 - z2^2) * H. */
				fp2_add(r->z, p->z, q->z);
				fp2_sqr(r->z, r->z);
				fp2_sub(r->z, r->z, t6);
				fp2_mul(r->z, r->z, t3);
			}
		}
		r->norm = 0;
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp2_free(t0);
		fp2_free(t1);
		fp2_free(t2);
		fp2_free(t3);
		fp2_free(t4);
		fp2_free(t5);
		fp2_free(t6);
	}
#endif
}

#endif /* EP_ADD == PROJC */

/*============================================================================*/
	/* Public definitions                                                         */
/*============================================================================*/

#if EP_ADD == BASIC || !defined(STRIP)

void ep2_add_basic(ep2_t r, ep2_t p, ep2_t q) {
	if (ep2_is_infty(p)) {
		ep2_copy(r, q);
		return;
	}

	if (ep2_is_infty(q)) {
		ep2_copy(r, p);
		return;
	}

	ep2_add_basic_imp(r, NULL, p, q);
}

void ep2_add_slp_basic(ep2_t r, fp2_t s, ep2_t p, ep2_t q) {
	if (ep2_is_infty(p)) {
		ep2_copy(r, q);
		return;
	}

	if (ep2_is_infty(q)) {
		ep2_copy(r, p);
		return;
	}

	ep2_add_basic_imp(r, s, p, q);
}

void ep2_sub_basic(ep2_t r, ep2_t p, ep2_t q) {
	ep2_t t;

	ep2_null(t);

	if (p == q) {
		ep2_set_infty(r);
		return;
	}

	TRY {
		ep2_new(t);

		ep2_neg_basic(t, q);
		ep2_add_basic(r, p, t);

		r->norm = 1;
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep2_free(t);
	}
}

#endif

#if EP_ADD == PROJC || !defined(STRIP)

void ep2_add_projc(ep2_t r, ep2_t p, ep2_t q) {
	if (ep2_is_infty(p)) {
		ep2_copy(r, q);
		return;
	}

	if (ep2_is_infty(q)) {
		ep2_copy(r, p);
		return;
	}

	if (p == q) {
		/* TODO: This is a quick hack. Should we fix this? */
		ep2_dbl(r, p);
		return;
	}

	ep2_add_projc_imp(r, p, q);
}

void ep2_sub_projc(ep2_t r, ep2_t p, ep2_t q) {
	ep2_t t;

	ep2_null(t);

	if (p == q) {
		ep2_set_infty(r);
		return;
	}

	TRY {
		ep2_new(t);

		ep2_neg_projc(t, q);
		ep2_add_projc(r, p, t);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep2_free(t);
	}
}

#endif
