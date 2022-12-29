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
 * Implementation of the point doubling on prime elliptic curves.
 *
 * @ingroup ep
 */

#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if EP_ADD == BASIC || !defined(STRIP)

/**
 * Doubles a point represented in affine coordinates on an ordinary prime
 * elliptic curve.
 *
 * @param[out] r			- the result.
 * @param[out] s			- the slope.
 * @param[in] p				- the point to double.
 */
static void ep_dbl_basic_imp(ep_t r, fp_t s, const ep_t p) {
	fp_t t0, t1, t2;

	fp_null(t0);
	fp_null(t1);
	fp_null(t2);

	RLC_TRY {
		fp_new(t0);
		fp_new(t1);
		fp_new(t2);

		/* t0 = 1/2 * y1. */
		fp_dbl(t0, p->y);
		fp_inv(t0, t0);

		/* t1 = 3 * x1^2 + a. */
		fp_sqr(t1, p->x);
		fp_copy(t2, t1);
		fp_dbl(t1, t1);
		fp_add(t1, t1, t2);

		switch (ep_curve_opt_a()) {
			case RLC_ZERO:
				break;
			case RLC_ONE:
				fp_add_dig(t1, t1, (dig_t)1);
				break;
#if FP_RDC != MONTY
			case RLC_TINY:
				fp_add_dig(t1, t1, ep_curve_get_a()[0]);
				break;
#endif
			default:
				fp_add(t1, t1, ep_curve_get_a());
				break;
		}

		/* t1 = (3 * x1^2 + a)/(2 * y1). */
		fp_mul(t1, t1, t0);

		if (s != NULL) {
			fp_copy(s, t1);
		}

		/* t2 = t1^2. */
		fp_sqr(t2, t1);

		/* x3 = t1^2 - 2 * x1. */
		fp_dbl(t0, p->x);
		fp_sub(t0, t2, t0);

		/* y3 = t1 * (x1 - x3) - y1. */
		fp_sub(t2, p->x, t0);
		fp_mul(t1, t1, t2);
		fp_sub(r->y, t1, p->y);

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

#endif /* EP_ADD == BASIC */

#if EP_ADD == PROJC || !defined(STRIP)

/**
 * Doubles a point represented in projective coordinates on an ordinary prime
 * elliptic curve.
 *
 * @param r					- the result.
 * @param p					- the point to double.
 */
static void ep_dbl_projc_imp(ep_t r, const ep_t p) {
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

		/* Formulas for point doubling from
		 * "Complete addition formulas for prime order elliptic curves"
		 * by Joost Renes, Craig Costello, and Lejla Batina
		 * https://eprint.iacr.org/2015/1060.pdf
		 */
		 if (ep_curve_opt_a() == RLC_ZERO) {
			/* Cost of 6M + 2S + 1m_3b + 9a. */
			fp_sqr(t0, p->y);
			fp_mul(t3, p->x, p->y);

 			if (p->coord == BASIC) {
				/* Save 1M + 1S + 1m_b3 if z1 = 1. */
				fp_copy(t1, p->y);
				fp_copy(t2, ep_curve_get_b3());
 			} else {
				fp_mul(t1, p->y, p->z);
				fp_sqr(t2, p->z);
				ep_curve_mul_b3(t2, t2);
 			}
			fp_dbl(r->z, t0);
			fp_dbl(r->z, r->z);
			fp_dbl(r->z, r->z);
 			fp_mul(r->x, t2, r->z);
			fp_add(r->y, t0, t2);
			fp_mul(r->z, t1, r->z);
			fp_dbl(t1, t2);
			fp_add(t2, t1, t2);
			fp_sub(t0, t0, t2);
			fp_mul(r->y, t0, r->y);
			fp_add(r->y, r->x, r->y);
			fp_mul(r->x, t0, t3);
			fp_dbl(r->x, r->x);
		} else {
			fp_sqr(t0, p->x);
			fp_sqr(t1, p->y);
			fp_mul(t3, p->x, p->y);
			fp_dbl(t3, t3);
			fp_mul(t4, p->y, p->z);

			if (ep_curve_opt_a() == RLC_MIN3) {
				/* Cost of 8M + 3S + 2mb + 21a. */
				if (p->coord == BASIC) {
					/* Save 1S + 1m_b + 2a if z1 = 1. */
					fp_set_dig(t2, 3);
					fp_copy(r->y, ep_curve_get_b());
				} else {
					fp_sqr(t2, p->z);
					ep_curve_mul_b(r->y, t2);
					fp_dbl(t5, t2);
					fp_add(t2, t2, t5);
				}
				fp_mul(r->z, p->x, p->z);
				fp_dbl(r->z, r->z);
				fp_sub(r->y, r->y, r->z);
				fp_dbl(r->x, r->y);
				fp_add(r->y, r->x, r->y);
				fp_sub(r->x, t1, r->y);
				fp_add(r->y, t1, r->y);
				fp_mul(r->y, r->x, r->y);
				fp_mul(r->x, t3, r->x);
				ep_curve_mul_b(r->z, r->z);
				fp_sub(t3, r->z, t2);
				fp_sub(t3, t3, t0);
				fp_dbl(r->z, t3);
				fp_add(t3, t3, r->z);
				fp_dbl(r->z, t0);
				fp_add(t0, t0, r->z);
				fp_sub(t0, t0, t2);
			} else {
				/* Common cost of 8M + 3S + 3m_a + 2m_3b + 15a. */
				if (p->coord == BASIC) {
					/* Save 1S + 1m_b + 1m_a if z1 = 1. */
					fp_copy(r->y, ep_curve_get_b3());
					fp_copy(t2, ep_curve_get_a());
				} else {
					fp_sqr(t2, p->z);
					ep_curve_mul_b3(r->y, t2);
					ep_curve_mul_a(t2, t2);
				}
				fp_mul(r->z, p->x, p->z);
				fp_dbl(r->z, r->z);
				ep_curve_mul_a(r->x, r->z);
				fp_add(r->y, r->x, r->y);
				fp_sub(r->x, t1, r->y);
				fp_add(r->y, t1, r->y);
				fp_mul(r->y, r->x, r->y);
				fp_mul(r->x, t3, r->x);
				ep_curve_mul_b3(r->z, r->z);
				fp_sub(t3, t0, t2);
				ep_curve_mul_a(t3, t3);
				fp_add(t3, t3, r->z);
				fp_dbl(r->z, t0);
				fp_add(t0, t0, r->z);
				fp_add(t0, t0, t2);
			}
			/* Common part with renamed variables. */
			fp_mul(t0, t0, t3);
			fp_add(r->y, r->y, t0);
			fp_dbl(t2, t4);
			fp_mul(t0, t2, t3);
			fp_sub(r->x, r->x, t0);
			fp_mul(r->z, t2, t1);
			fp_dbl(r->z, r->z);
			fp_dbl(r->z, r->z);
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

#endif /* EP_ADD == PROJC */

#if EP_ADD == JACOB || !defined(STRIP)

/**
 * Doubles a point represented in Jacobian coordinates on an ordinary prime
 * elliptic curve.
 *
 * @param r					- the result.
 * @param p					- the point to double.
 */
static void ep_dbl_jacob_imp(ep_t r, const ep_t p) {
	fp_t t0, t1, t2, t3, t4, t5;

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

		if (p->coord != BASIC && ep_curve_opt_a() == RLC_MIN3) {
			/* dbl-2001-b formulas: 3M + 5S + 8add + 1*4 + 2*8 + 1*3 */
			/* http://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-3.html#doubling-dbl-2001-b */

			/* t0 = delta = z1^2. */
			fp_sqr(t0, p->z);

			/* t1 = gamma = y1^2. */
			fp_sqr(t1, p->y);

			/* t2 = beta = x1 * y1^2. */
			fp_mul(t2, p->x, t1);

			/* t3 = alpha = 3 * (x1 - z1^2) * (x1 + z1^2). */
			fp_sub(t3, p->x, t0);
			fp_add(t4, p->x, t0);
			fp_mul(t4, t3, t4);
			fp_dbl(t3, t4);
			fp_add(t3, t3, t4);

			/* x3 = alpha^2 - 8 * beta. */
			fp_dbl(t2, t2);
			fp_dbl(t2, t2);
			fp_dbl(t5, t2);
			fp_sqr(r->x, t3);
			fp_sub(r->x, r->x, t5);

			/* z3 = (y1 + z1)^2 - gamma - delta. */
			fp_add(r->z, p->y, p->z);
			fp_sqr(r->z, r->z);
			fp_sub(r->z, r->z, t1);
			fp_sub(r->z, r->z, t0);

			/* y3 = alpha * (4 * beta - x3) - 8 * gamma^2. */
			fp_dbl(t1, t1);
			fp_sqr(t1, t1);
			fp_dbl(t1, t1);
			fp_sub(r->y, t2, r->x);
			fp_mul(r->y, r->y, t3);
			fp_sub(r->y, r->y, t1);
		} else if (ep_curve_opt_a() == RLC_ZERO) {
			/* dbl-2009-l formulas: 2M + 5S + 6add + 1*8 + 3*2 + 1*3. */
			/* http://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-0.html#doubling-dbl-2009-l */

			/* A = X1^2 */
			fp_sqr(t0, p->x);

			/* B = Y1^2 */
			fp_sqr(t1, p->y);

			/* C = B^2 */
			fp_sqr(t2, t1);

			/* D = 2*((X1+B)^2-A-C) */
			fp_add(t1, t1, p->x);
			fp_sqr(t1, t1);
			fp_sub(t1, t1, t0);
			fp_sub(t1, t1, t2);
			fp_dbl(t1, t1);

			/* E = 3*A */
			fp_dbl(t3, t0);
			fp_add(t0, t3, t0);

			/* F = E^2 */
			fp_sqr(t3, t0);

			/* Z3 = 2*Y1*Z1 */
			fp_mul(r->z, p->y, p->z);
			fp_dbl(r->z, r->z);

			/* X3 = F-2*D */
			fp_sub(r->x, t3, t1);
			fp_sub(r->x, r->x, t1);

			/* Y3 = E*(D-X3)-8*C */
			fp_sub(r->y, t1, r->x);
			fp_mul(r->y, r->y, t0);
			fp_dbl(t2, t2);
			fp_dbl(t2, t2);
			fp_dbl(t2, t2);
			fp_sub(r->y, r->y, t2);
		} else {
			/* dbl-2007-bl formulas: 1M + 8S + 1*a + 10add + 1*8 + 2*2 + 1*3 */
			/* http://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian.html#doubling-dbl-2007-bl */

			/* t0 = x1^2, t1 = y1^2, t2 = y1^4. */
			fp_sqr(t0, p->x);
			fp_sqr(t1, p->y);
			fp_sqr(t2, t1);

			if (p->coord != BASIC) {
				/* t3 = z1^2. */
				fp_sqr(t3, p->z);

				if (ep_curve_opt_a() == RLC_ZERO) {
					/* z3 = 2 * y1 * z1. */
					fp_mul(r->z, p->y, p->z);
					fp_dbl(r->z, r->z);
				} else {
					/* z3 = (y1 + z1)^2 - y1^2 - z1^2. */
					fp_add(r->z, p->y, p->z);
					fp_sqr(r->z, r->z);
					fp_sub(r->z, r->z, t1);
					fp_sub(r->z, r->z, t3);
				}
			} else {
				/* z3 = 2 * y1. */
				fp_dbl(r->z, p->y);
			}

			/* t4 = S = 2*((x1 + y1^2)^2 - x1^2 - y1^4). */
			fp_add(t4, p->x, t1);
			fp_sqr(t4, t4);
			fp_sub(t4, t4, t0);
			fp_sub(t4, t4, t2);
			fp_dbl(t4, t4);

			/* t5 = M = 3 * x1^2 + a * z1^4. */
			fp_dbl(t5, t0);
			fp_add(t5, t5, t0);
			if (p->coord != BASIC) {
				fp_sqr(t3, t3);
				switch (ep_curve_opt_a()) {
					case RLC_ZERO:
						break;
					case RLC_ONE:
						fp_add(t5, t5, t3);
						break;
					case RLC_TINY:
						fp_mul_dig(t1, t3, ep_curve_get_a()[0]);
						fp_add(t5, t5, t1);
						break;
					default:
						fp_mul(t1, t3, ep_curve_get_a());
						fp_add(t5, t5, t1);
						break;
				}
			} else {
				switch (ep_curve_opt_a()) {
					case RLC_ZERO:
						break;
					case RLC_ONE:
						fp_add_dig(t5, t5, (dig_t)1);
						break;
					case RLC_TINY:
						fp_add_dig(t5, t5, ep_curve_get_a()[0]);
						break;
					default:
						fp_add(t5, t5, ep_curve_get_a());
						break;
				}
			}

			/* x3 = T = M^2 - 2 * S. */
			fp_sqr(r->x, t5);
			fp_dbl(t1, t4);
			fp_sub(r->x, r->x, t1);

			/* y3 = M * (S - T) - 8 * y1^4. */
			fp_dbl(t2, t2);
			fp_dbl(t2, t2);
			fp_dbl(t2, t2);
			fp_sub(t4, t4, r->x);
			fp_mul(t5, t5, t4);
			fp_sub(r->y, t5, t2);
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
	}
}

#endif /* EP_ADD == JACOB */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if EP_ADD == BASIC || !defined(STRIP)

void ep_dbl_basic(ep_t r, const ep_t p) {
	if (ep_is_infty(p)) {
		ep_set_infty(r);
		return;
	}
	ep_dbl_basic_imp(r, NULL, p);
}

void ep_dbl_slp_basic(ep_t r, fp_t s, const ep_t p) {
	if (ep_is_infty(p)) {
		ep_set_infty(r);
		return;
	}

	ep_dbl_basic_imp(r, s, p);
}

#endif

#if EP_ADD == PROJC || !defined(STRIP)

void ep_dbl_projc(ep_t r, const ep_t p) {
	if (ep_is_infty(p)) {
		ep_set_infty(r);
		return;
	}

	ep_dbl_projc_imp(r, p);
}

#endif

#if EP_ADD == JACOB || !defined(STRIP)

void ep_dbl_jacob(ep_t r, const ep_t p) {
	if (ep_is_infty(p)) {
		ep_set_infty(r);
		return;
	}

	ep_dbl_jacob_imp(r, p);
}

#endif
