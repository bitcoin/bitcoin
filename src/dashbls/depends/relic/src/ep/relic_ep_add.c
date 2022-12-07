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
 * Implementation of the point addition on prime elliptic curves.
 *
 * @ingroup ep
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
 * @param[out] r			- the result.
 * @param[out] s			- the slope.
 * @param[in] p				- the first point to add.
 * @param[in] q				- the second point to add.
 */
static void ep_add_basic_imp(ep_t r, fp_t s, const ep_t p, const ep_t q) {
	fp_t t0, t1, t2;

	fp_null(t0);
	fp_null(t1);
	fp_null(t2);

	RLC_TRY {
		fp_new(t0);
		fp_new(t1);
		fp_new(t2);

		/* t0 = x2 - x1. */
		fp_sub(t0, q->x, p->x);
		/* t1 = y2 - y1. */
		fp_sub(t1, q->y, p->y);

		/* If t0 is zero. */
		if (fp_is_zero(t0)) {
			if (fp_is_zero(t1)) {
				/* If t1 is zero, q = p, should have doubled. */
				ep_dbl_basic(r, p);
			} else {
				/* If t1 is not zero and t0 is zero, q = -p and r = infinity. */
				ep_set_infty(r);
			}
		} else {
			/* t2 = 1/(x2 - x1). */
			fp_inv(t2, t0);
			/* t2 = lambda = (y2 - y1)/(x2 - x1). */
			fp_mul(t2, t1, t2);

			/* x3 = lambda^2 - x2 - x1. */
			fp_sqr(t1, t2);
			fp_sub(t0, t1, p->x);
			fp_sub(t0, t0, q->x);

			/* y3 = lambda * (x1 - x3) - y1. */
			fp_sub(t1, p->x, t0);
			fp_mul(t1, t2, t1);
			fp_sub(r->y, t1, p->y);

			fp_copy(r->x, t0);
			fp_copy(r->z, p->z);

			if (s != NULL) {
				fp_copy(s, t2);
			}

			r->coord = BASIC;
		}
		fp_free(t0);
		fp_free(t1);
		fp_free(t2);
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

#endif /* EP_ADD == BASIC */

#if EP_ADD == PROJC || !defined(STRIP)

/**
 * Adds a point represented in homogeneous coordinates to a point represented in
 * projective coordinates.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the projective point.
 * @param[in] q				- the affine point.
 */
static void ep_add_projc_mix(ep_t r, const ep_t p, const ep_t q) {
	fp_t t0, t1, t2, t3, t4, t5;

	fp_null(t0);
	fp_null(t1);
	fp_null(t2);
	fp_null(t3);
	fp_null(t4);
	fp_null(t5);

	RLC_TRY {
		fp_new(t0);
		fp_new(t1);
		fp_new(t2);
		fp_new(t3);
		fp_new(t4);
		fp_new(t5);

		/* Formulas for mixed addition from
		 * "Complete addition formulas for prime order elliptic curves"
		 * by Joost Renes, Craig Costello, and Lejla Batina
		 * https://eprint.iacr.org/2015/1060.pdf
		 */

		 fp_mul(t0, p->x, q->x);
		 fp_mul(t1, p->y, q->y);
		 fp_add(t3, q->x, q->y);
		 fp_add(t4, p->x, p->y);
		 fp_mul(t3, t3, t4);
		 fp_add(t4, t0, t1);
		 fp_sub(t3, t3, t4);

		 if (ep_curve_opt_a() == RLC_ZERO) {
 			/* Cost of 11M + 2m_3b + 13a. */
			if (p->coord == BASIC) {
				/* Save 1M + 1m_3b if z1 = 1. */
				fp_add(t4, q->y, p->y);
	 			fp_add(r->y, q->x, p->x);
				fp_add(r->z, t1, ep_curve_get_b3());
	 			fp_sub(t1, t1, ep_curve_get_b3());
			} else {
				fp_mul(t4, q->y, p->z);
				fp_add(t4, t4, p->y);
	 			fp_mul(r->y, q->x, p->z);
	 			fp_add(r->y, r->y, p->x);
	 			ep_curve_mul_b3(t2, p->z);
				fp_add(r->z, t1, t2);
	 			fp_sub(t1, t1, t2);
			}
			fp_dbl(r->x, t0);
			fp_add(t0, t0, r->x);
 			ep_curve_mul_b3(r->y, r->y);
 			fp_mul(r->x, t4, r->y);
 			fp_mul(t2, t3, t1);
 			fp_sub(r->x, t2, r->x);
 			fp_mul(r->y, t0, r->y);
 			fp_mul(t1, t1, r->z);
 			fp_add(r->y, t1, r->y);
 			fp_mul(t0, t0, t3);
 			fp_mul(r->z, r->z, t4);
 			fp_add(r->z, r->z, t0);
 		 } else if (ep_curve_opt_a() == RLC_MIN3) {
 			/* Cost of 11M + 2m_b + 23a. */
			if (p->coord == BASIC) {
				/* Save 2M + 3a if z1 = 1. */
				fp_set_dig(t2, 3);
	 			fp_add(t4, q->y, p->y);
	 			fp_add(r->y, q->x, p->x);
	 			fp_sub(r->x, r->y, ep_curve_get_b());
			} else {
				fp_dbl(t2, p->z);
	 			fp_add(t2, t2, p->z);
				fp_mul(t4, q->y, p->z);
	 			fp_add(t4, t4, p->y);
	 			fp_mul(r->y, q->x, p->z);
	 			fp_add(r->y, r->y, p->x);
				ep_curve_mul_b(r->z, p->z);
	 			fp_sub(r->x, r->y, r->z);
			}
 			fp_dbl(r->z, r->x);
 			fp_add(r->x, r->x, r->z);
 			fp_sub(r->z, t1, r->x);
 			fp_add(r->x, t1, r->x);
 			ep_curve_mul_b(r->y, r->y);
 			fp_sub(r->y, r->y, t2);
 			fp_sub(r->y, r->y, t0);
 			fp_dbl(t1, r->y);
 			fp_add(r->y, t1, r->y);
 			fp_dbl(t1, t0);
 			fp_add(t0, t1, t0);
 			fp_sub(t0, t0, t2);
 			fp_mul(t1, t4, r->y);
 			fp_mul(t2, t0, r->y);
 			fp_mul(r->y, r->x, r->z);
 			fp_add(r->y, r->y, t2);
 			fp_mul(r->x, t3, r->x);
 			fp_sub(r->x, r->x, t1);
 			fp_mul(r->z, t4, r->z);
 			fp_mul(t1, t3, t0);
 			fp_add(r->z, r->z, t1);
 		} else {
			/* Cost of 11M + 3m_a + 2m_3b + 17a. */
			if (p->coord == BASIC) {
				/* Save 1M + 1m_a + 1m_3b if z1 = 1. */
				fp_copy(t2, ep_curve_get_a());
				fp_add(t4, q->x, p->x);
				fp_add(t5, q->y, p->y);
				ep_curve_mul_a(r->z, t4);
				fp_add(r->z, r->z, ep_curve_get_b3());
			} else {
				ep_curve_mul_a(t2, p->z);
				fp_mul(t4, q->x, p->z);
				fp_add(t4, t4, p->x);
				fp_mul(t5, q->y, p->z);
				fp_add(t5, t5, p->y);
				ep_curve_mul_b3(r->x, p->z);
				ep_curve_mul_a(r->z, t4);
				fp_add(r->z, r->x, r->z);
			}
			fp_sub(r->x, t1, r->z);
			fp_add(r->z, t1, r->z);
			fp_mul(r->y, r->x, r->z);
			fp_dbl(t1, t0);
			fp_add(t1, t1, t0);
			ep_curve_mul_b3(t4, t4);
			fp_add(t1, t1, t2);
			fp_sub(t2, t0, t2);
			ep_curve_mul_a(t2, t2);
			fp_add(t4, t4, t2);
			fp_mul(t0, t1, t4);
			fp_add(r->y, r->y, t0);
			fp_mul(t0, t5, t4);
			fp_mul(r->x, t3, r->x);
			fp_sub(r->x, r->x, t0);
			fp_mul(t0, t3, t1);
			fp_mul(r->z, t5, r->z);
			fp_add(r->z, r->z, t0);
		}

		r->coord = PROJC;
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp_free(t0);
		fp_free(t1);
		fp_free(t2);
		fp_free(t3);
		fp_free(t4);
		fp_free(t5);
	}
}

/**
 * Adds two points represented in homogeneous coordinates on an ordinary prime
 * elliptic curve.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the first point to add.
 * @param[in] q				- the second point to add.
 */
static void ep_add_projc_imp(ep_t r, const ep_t p, const ep_t q) {
#if defined(EP_MIXED) && defined(STRIP)
	/* If code size is a problem, leave only the mixed version. */
	ep_add_projc_mix(r, p, q);
#else /* General addition. */

#if defined(EP_MIXED) || !defined(STRIP)
	/* Test if z2 = 1 only if mixed coordinates are turned on. */
	if (q->coord == BASIC) {
		ep_add_projc_mix(r, p, q);
		return;
	}
#endif

	fp_t t0, t1, t2, t3, t4, t5;

	fp_null(t0);
	fp_null(t1);
	fp_null(t2);
	fp_null(t3);
	fp_null(t4);
	fp_null(t5);

	RLC_TRY {
		fp_new(t0);
		fp_new(t1);
		fp_new(t2);
		fp_new(t3);
		fp_new(t4);
		fp_new(t5);

		/* Formulas for point addition from
		 * "Complete addition formulas for prime order elliptic curves"
		 * by Joost Renes, Craig Costello, and Lejla Batina
		 * https://eprint.iacr.org/2015/1060.pdf
		 */
		fp_mul(t0, p->x, q->x);
		fp_mul(t1, p->y, q->y);
		fp_mul(t2, p->z, q->z);
		fp_add(t3, p->x, p->y);
		fp_add(t4, q->x, q->y);
		fp_mul(t3, t3, t4);
		fp_add(t4, t0, t1);
		fp_sub(t3, t3, t4);
		if (ep_curve_opt_a() == RLC_ZERO) {
			/* Cost of 12M + 2m_3b + 19a. */
			fp_add(t4, p->y, p->z);
			fp_add(t5, q->y, q->z);
			fp_mul(t4, t4, t5);
			fp_add(t5, t1, t2);
			fp_sub(t4, t4, t5);
			fp_add(r->y, q->x, q->z);
			fp_add(r->x, p->x, p->z);
			fp_mul(r->x, r->x, r->y);
			fp_add(r->y, t0, t2);
			fp_sub(r->y, r->x, r->y);
			fp_dbl(r->x, t0);
			fp_add(t0, t0, r->x);
			ep_curve_mul_b3(t2, t2);
			fp_add(r->z, t1, t2);
			fp_sub(t1, t1, t2);
			ep_curve_mul_b3(r->y, r->y);
			fp_mul(r->x, t4, r->y);
			fp_mul(t2, t3, t1);
			fp_sub(r->x, t2, r->x);
			fp_mul(r->y, t0, r->y);
			fp_mul(t1, t1, r->z);
			fp_add(r->y, t1, r->y);
			fp_mul(t0, t0, t3);
			fp_mul(r->z, r->z, t4);
			fp_add(r->z, r->z, t0);
		} else if (ep_curve_opt_a() == RLC_MIN3) {
			/* Cost of 12M + 2m_b + 29a. */
			fp_add(t4, p->y, p->z);
			fp_add(t5, q->y, q->z);
			fp_mul(t4, t4, t5);
			fp_add(t5, t1, t2);
			fp_sub(t4, t4, t5);
			fp_add(r->x, p->x, p->z);
			fp_add(r->y, q->x, q->z);
			fp_mul(r->x, r->x, r->y);
			fp_add(r->y, t0, t2);
			fp_sub(r->y, r->x, r->y);
			ep_curve_mul_b(r->z, t2);
			fp_sub(r->x, r->y, r->z);
			fp_dbl(r->z, r->x);
			fp_add(r->x, r->x, r->z);
			fp_sub(r->z, t1, r->x);
			fp_add(r->x, t1, r->x);
			ep_curve_mul_b(r->y, r->y);
			fp_dbl(t1, t2);
			fp_add(t2, t1, t2);
			fp_sub(r->y, r->y, t2);
			fp_sub(r->y, r->y, t0);
			fp_dbl(t1, r->y);
			fp_add(r->y, t1, r->y);
			fp_dbl(t1, t0);
			fp_add(t0, t1, t0);
			fp_sub(t0, t0, t2);
			fp_mul(t1, t4, r->y);
			fp_mul(t2, t0, r->y);
			fp_mul(r->y, r->x, r->z);
			fp_add(r->y, r->y, t2);
			fp_mul(r->x, t3, r->x);
			fp_sub(r->x, r->x, t1);
			fp_mul(r->z, t4, r->z);
			fp_mul(t1, t3, t0);
			fp_add(r->z, r->z, t1);
		} else {
			 /* Cost of 12M + 3m_a + 2_m3b + 23a. */
			fp_add(t4, p->x, p->z);
			fp_add(t5, q->x, q->z);
			fp_mul(t4, t4, t5);
			fp_add(t5, t0, t2);
			fp_sub(t4, t4, t5);
			fp_add(t5, p->y, p->z);
			fp_add(r->x, q->y, q->z);
			fp_mul(t5, t5, r->x);
			fp_add(r->x, t1, t2);
			fp_sub(t5, t5, r->x);
			ep_curve_mul_a(r->z, t4);
			ep_curve_mul_b3(r->x, t2);
			fp_add(r->z, r->x, r->z);
			fp_sub(r->x, t1, r->z);
			fp_add(r->z, t1, r->z);
			fp_mul(r->y, r->x, r->z);
			fp_dbl(t1, t0);
			fp_add(t1, t1, t0);
			ep_curve_mul_a(t2, t2);
			ep_curve_mul_b3(t4, t4);
			fp_add(t1, t1, t2);
			fp_sub(t2, t0, t2);
			ep_curve_mul_a(t2, t2);
			fp_add(t4, t4, t2);
			fp_mul(t0, t1, t4);
			fp_add(r->y, r->y, t0);
			fp_mul(t0, t5, t4);
			fp_mul(r->x, t3, r->x);
			fp_sub(r->x, r->x, t0);
			fp_mul(t0, t3, t1);
			fp_mul(r->z, t5, r->z);
			fp_add(r->z, r->z, t0);
		}

		r->coord = PROJC;
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp_free(t0);
		fp_free(t1);
		fp_free(t2);
		fp_free(t3);
		fp_free(t4);
		fp_free(t5);
	}
#endif
}

#endif /* EP_ADD == PROJC */

#if EP_ADD == JACOB || !defined(STRIP)

/**
 * Adds a point represented in Jacobian coordinates to a point represented in
 * projective coordinates.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the projective point.
 * @param[in] q				- the affine point.
 */
static void ep_add_jacob_mix(ep_t r, const ep_t p, const ep_t q) {
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

		/* madd-2007-bl formulas: 7M + 4S + 9add + 1*4 + 3*2. */
		/* http://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-3.html#addition-madd-2007-bl */

		if (p->coord != BASIC) {
			/* t0 = z1^2. */
			fp_sqr(t0, p->z);

			/* t3 = U2 = x2 * z1^2. */
			fp_mul(t3, q->x, t0);

			/* t1 = S2 = y2 * z1^3. */
			fp_mul(t1, t0, p->z);
			fp_mul(t1, t1, q->y);

			/* t3 = H = U2 - x1. */
			fp_sub(t3, t3, p->x);

			/* t1 = R = 2 * (S2 - y1). */
			fp_sub(t1, t1, p->y);
			fp_dbl(t1, t1);
		} else {
			/* H = x2 - x1. */
			fp_sub(t3, q->x, p->x);

			/* t1 = R = 2 * (y2 - y1). */
			fp_sub(t1, q->y, p->y);
			fp_dbl(t1, t1);
		}

		/* t2 = HH = H^2. */
		fp_sqr(t2, t3);

		/* If H is zero. */
		if (fp_is_zero(t3)) {
			if (fp_is_zero(t1)) {
				/* If I is zero, p = q, should have doubled. */
				ep_dbl_jacob(r, p);
			} else {
				/* If I is not zero, q = -p, r = infinity. */
				ep_set_infty(r);
			}
		} else {
			/* t4 = I = 4*HH. */
			fp_dbl(t4, t2);
			fp_dbl(t4, t4);

			/* t5 = J = H * I. */
			fp_mul(t5, t3, t4);

			/* t4 = V = x1 * I. */
			fp_mul(t4, p->x, t4);

			/* x3 = R^2 - J - 2 * V. */
			fp_sqr(r->x, t1);
			fp_sub(r->x, r->x, t5);
			fp_dbl(t6, t4);
			fp_sub(r->x, r->x, t6);

			/* y3 = R * (V - x3) - 2 * Y1 * J. */
			fp_sub(t4, t4, r->x);
			fp_mul(t4, t4, t1);
			fp_mul(t1, p->y, t5);
			fp_dbl(t1, t1);
			fp_sub(r->y, t4, t1);

			if (p->coord != BASIC) {
				/* z3 = (z1 + H)^2 - z1^2 - HH. */
				fp_add(r->z, p->z, t3);
				fp_sqr(r->z, r->z);
				fp_sub(r->z, r->z, t0);
				fp_sub(r->z, r->z, t2);
			} else {
				/* z3 = 2 * H. */
				fp_dbl(r->z, t3);
			}
		}
		r->coord = JACOB;
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp_free(t0);
		fp_free(t1);
		fp_free(t2);
		fp_free(t3);
		fp_free(t4);
		fp_free(t5);
		fp_free(t6);
	}
}

/**
 * Adds two points represented in Jacobian coordinates on an ordinary prime
 * elliptic curve.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the first point to add.
 * @param[in] q				- the second point to add.
 */
static void ep_add_jacob_imp(ep_t r, const ep_t p, const ep_t q) {
#if defined(EP_MIXED) && defined(STRIP)
	/* If code size is a problem, leave only the mixed version. */
	ep_add_jacob_mix(r, p, q);
#else /* General addition. */

#if defined(EP_MIXED) || !defined(STRIP)
	/* Test if z2 = 1 only if mixed coordinates are turned on. */
	if (q->coord == BASIC) {
		ep_add_jacob_mix(r, p, q);
		return;
	}
#endif

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

		/* add-2007-bl formulas: 11M + 5S + 9add + 4*2 */
		/* http://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-3.html#addition-add-2007-bl */

		/* t0 = z1^2. */
		fp_sqr(t0, p->z);

		/* t1 = z2^2. */
		fp_sqr(t1, q->z);

		/* t2 = U1 = x1 * z2^2. */
		fp_mul(t2, p->x, t1);

		/* t3 = U2 = x2 * z1^2. */
		fp_mul(t3, q->x, t0);

		/* t6 = z1^2 + z2^2. */
		fp_add(t6, t0, t1);

		/* t0 = S2 = y2 * z1^3. */
		fp_mul(t0, t0, p->z);
		fp_mul(t0, t0, q->y);

		/* t1 = S1 = y1 * z2^3. */
		fp_mul(t1, t1, q->z);
		fp_mul(t1, t1, p->y);

		/* t3 = H = U2 - U1. */
		fp_sub(t3, t3, t2);

		/* t0 = R = 2 * (S2 - S1). */
		fp_sub(t0, t0, t1);
		fp_dbl(t0, t0);

		/* If E is zero. */
		if (fp_is_zero(t3)) {
			if (fp_is_zero(t0)) {
				/* If I is zero, p = q, should have doubled. */
				ep_dbl_jacob(r, p);
			} else {
				/* If I is not zero, q = -p, r = infinity. */
				ep_set_infty(r);
			}
		} else {
			/* t4 = I = (2*H)^2. */
			fp_dbl(t4, t3);
			fp_sqr(t4, t4);

			/* t5 = J = H * I. */
			fp_mul(t5, t3, t4);

			/* t4 = V = U1 * I. */
			fp_mul(t4, t2, t4);

			/* x3 = R^2 - J - 2 * V. */
			fp_sqr(r->x, t0);
			fp_sub(r->x, r->x, t5);
			fp_dbl(t2, t4);
			fp_sub(r->x, r->x, t2);

			/* y3 = R * (V - x3) - 2 * S1 * J. */
			fp_sub(t4, t4, r->x);
			fp_mul(t4, t4, t0);
			fp_mul(t1, t1, t5);
			fp_dbl(t1, t1);
			fp_sub(r->y, t4, t1);

			/* z3 = ((z1 + z2)^2 - z1^2 - z2^2) * H. */
			fp_add(r->z, p->z, q->z);
			fp_sqr(r->z, r->z);
			fp_sub(r->z, r->z, t6);
			fp_mul(r->z, r->z, t3);
		}
		r->coord = JACOB;
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp_free(t0);
		fp_free(t1);
		fp_free(t2);
		fp_free(t3);
		fp_free(t4);
		fp_free(t5);
		fp_free(t6);
	}
#endif
}

#endif /* EP_ADD == JACOB */

/*============================================================================*/
	/* Public definitions                                                         */
/*============================================================================*/

#if EP_ADD == BASIC || !defined(STRIP)

void ep_add_basic(ep_t r, const ep_t p, const ep_t q) {
	if (ep_is_infty(p)) {
		ep_copy(r, q);
		return;
	}

	if (ep_is_infty(q)) {
		ep_copy(r, p);
		return;
	}

	ep_add_basic_imp(r, NULL, p, q);
}

void ep_add_slp_basic(ep_t r, fp_t s, const ep_t p, const ep_t q) {
	if (ep_is_infty(p)) {
		ep_copy(r, q);
		return;
	}

	if (ep_is_infty(q)) {
		ep_copy(r, p);
		return;
	}

	ep_add_basic_imp(r, s, p, q);
}

#endif

#if EP_ADD == PROJC || !defined(STRIP)

void ep_add_projc(ep_t r, const ep_t p, const ep_t q) {
	if (ep_is_infty(p)) {
		ep_copy(r, q);
		return;
	}

	if (ep_is_infty(q)) {
		ep_copy(r, p);
		return;
	}

	ep_add_projc_imp(r, p, q);
}

#endif

#if EP_ADD == JACOB || !defined(STRIP)

void ep_add_jacob(ep_t r, const ep_t p, const ep_t q) {
	if (ep_is_infty(p)) {
		ep_copy(r, q);
		return;
	}

	if (ep_is_infty(q)) {
		ep_copy(r, p);
		return;
	}

	ep_add_jacob_imp(r, p, q);
}

#endif

void ep_sub(ep_t r, const ep_t p, const ep_t q) {
	ep_t t;

	ep_null(t);

	if (p == q) {
		ep_set_infty(r);
		return;
	}

	RLC_TRY {
		ep_new(t);
		ep_neg(t, q);
		ep_add(r, p, t);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ep_free(t);
	}
}
