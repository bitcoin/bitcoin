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
 * Implementation of Miller addition for curves of embedding degree 2.
 *
 * @ingroup pp
 */

#include "relic_core.h"
#include "relic_pp.h"
#include "relic_fp_low.h"
#include "relic_fpx_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if EP_ADD == BASIC || !defined(STRIP)

void pp_add_k8_basic(fp8_t l, ep2_t r, ep2_t q, ep_t p) {
	int one = 1, zero = 0;
	fp2_t s;
	ep2_t t;

	fp2_null(s);
	ep2_null(t);

	RLC_TRY {
		fp2_new(s);
		ep2_new(t);

		ep2_copy(t, r);
		ep2_add_slp_basic(r, s, r, q);

		if (ep2_curve_is_twist() == RLC_EP_MTYPE) {
			one ^= 1;
			zero ^= 1;
		}

		fp_mul(l[one][zero][0], s[0], p->x);
		fp_mul(l[one][zero][1], s[1], p->x);
		fp2_mul(l[one][one], s, t->x);
		fp2_sub(l[one][one], t->y, l[one][one]);
		fp_copy(l[zero][zero][0], p->y);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp2_free(s);
		ep2_free(t);
	}
}

#endif

#if EP_ADD == PROJC || EP_ADD == JACOB || !defined(STRIP)

#if PP_EXT == BASIC || !defined(STRIP)

void pp_add_k8_projc_basic(fp8_t l, ep2_t r, ep2_t q, ep_t p) {
	fp2_t t0, t1, t2, t3, t4, t5;
	int one = 1, zero = 0;

	fp2_null(t0);
	fp2_null(t1);
	fp2_null(t2);
	fp2_null(t3);
	fp2_null(t4);

	RLC_TRY {
		fp2_new(t0);
		fp2_new(t1);
		fp2_new(t2);
		fp2_new(t3);
		fp2_new(t4);
		fp2_new(t5);

		if (ep2_curve_is_twist() == RLC_EP_MTYPE) {
			one ^= 1;
			zero ^= 1;
		}

		/* t0 = A = Z1^2, t1 = B = X2*Z1. */
		fp2_sqr(t0, r->z);
		fp2_mul(t1, r->z, q->x);

		/* t0 = C = y2*A, t2 = D = (x1 - B) */
		fp2_mul(t0, t0, q->y);
		fp2_sub(t2, r->x, t1);

		/* t3 = E = 2*(y1 - C), t4 = F = 2*D*z1, t2 = G = 4*D*F. */
		fp2_sub(t3, r->y, t0);
		fp2_dbl(t3, t3);
		fp2_dbl(t2, t2);
		fp2_mul(t4, t2, r->z);
		fp2_mul(t2, t2, t4);
		fp2_dbl(t2, t2);

		/* l = E*X2 - F*Y2 - E*xQ + F*yQ. */
		fp2_mul(l[one][one], t3, q->x);
		fp2_mul(t0, t4, q->y);
		fp2_sub(l[one][one], l[one][one], t0);
		fp_mul(l[one][zero][0], t3[0], p->x);
		fp_mul(l[one][zero][1], t3[1], p->x);
		fp_mul(l[zero][zero][0], t4[0], p->y);
		fp_mul(l[zero][zero][1], t4[1], p->y);

		/* z3 = F^2, t4 = (F + E)^2, t3 = E^2. */
		fp2_sqr(r->z, t4);
		fp2_add(t4, t4, t3);
		fp2_sqr(t4, t4);
		fp2_sqr(t3, t3);

		/* t5 = x3 = 2*E^2 - (x1 + B)*G. */
		fp2_add(t1, t1, r->x);
		fp2_mul(t1, t1, t2);
		fp2_dbl(t5, t3);
		fp2_sub(t5, t5, t1);

		/* y3 = ((F + E)^2 - E^2 - F^2)*(x1*G - x3) - y1*G^2. */
		fp2_sub(t4, t4, r->z);
		fp2_sub(t4, t4, t3);
		fp2_mul(t1, r->x, t2);
		fp2_sub(t1, t1, t5);
		fp2_mul(t4, t4, t1);
		fp2_sqr(t2, t2);
		fp2_mul(r->y, r->y, t2);
		fp2_sub(r->y, t4, r->y);

		/* Z3 = 2*F^2. */
		fp2_dbl(r->z, r->z);
		fp2_copy(r->x, t5);

		r->coord = PROJC;
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp2_free(t0);
		fp2_free(t1);
		fp2_free(t2);
		fp2_free(t3);
		fp2_free(t4);
		fp2_free(t5);
	}
}

#endif

#if PP_EXT == LAZYR || !defined(STRIP)

void pp_add_k8_projc_lazyr(fp8_t l, ep2_t r, ep2_t q, ep_t p) {
	fp2_t t0, t1, t2, t3, t4, t5;
	int one = 1, zero = 0;

	fp2_null(t0);
	fp2_null(t1);
	fp2_null(t2);
	fp2_null(t3);
	fp2_null(t4);

	RLC_TRY {
		fp2_new(t0);
		fp2_new(t1);
		fp2_new(t2);
		fp2_new(t3);
		fp2_new(t4);
		fp2_new(t5);

		if (ep2_curve_is_twist() == RLC_EP_MTYPE) {
			one ^= 1;
			zero ^= 1;
		}

		/* t0 = A = Z1^2, t1 = B = X2*Z1. */
		fp2_sqr(t0, r->z);
		fp2_mul(t1, r->z, q->x);

		/* t0 = C = y2*A, t2 = D = (x1 - B) */
		fp2_mul(t0, t0, q->y);
		fp2_sub(t2, r->x, t1);

		/* t3 = E = 2*(y1 - C), t4 = F = 2*D*z1, t2 = G = 4*D*F. */
		fp2_sub(t3, r->y, t0);
		fp2_dbl(t3, t3);
		fp2_dbl(t2, t2);
		fp2_mul(t4, t2, r->z);
		fp2_mul(t2, t2, t4);
		fp2_dbl(t2, t2);

		/* l = E*X2 - F*Y2 - E*xQ + F*yQ. */
		fp2_mul(l[one][one], t3, q->x);
		fp2_mul(t0, t4, q->y);
		fp2_sub(l[one][one], l[one][one], t0);
		fp_mul(l[one][zero][0], t3[0], p->x);
		fp_mul(l[one][zero][1], t3[1], p->x);
		fp_mul(l[zero][zero][0], t4[0], p->y);
		fp_mul(l[zero][zero][1], t4[1], p->y);

		/* z3 = F^2, t4 = (F + E)^2, t3 = E^2. */
		fp2_sqr(r->z, t4);
		fp2_add(t4, t4, t3);
		fp2_sqr(t4, t4);
		fp2_sqr(t3, t3);

		/* t5 = x3 = 2*E^2 - (x1 + B)*G. */
		fp2_add(t1, t1, r->x);
		fp2_mul(t1, t1, t2);
		fp2_dbl(t5, t3);
		fp2_sub(t5, t5, t1);

		/* y3 = ((F + E)^2 - E^2 - F^2)*(x1*G - x3) - y1*G^2. */
		fp2_sub(t4, t4, r->z);
		fp2_sub(t4, t4, t3);
		fp2_mul(t1, r->x, t2);
		fp2_sub(t1, t1, t5);
		fp2_mul(t4, t4, t1);
		fp2_sqr(t2, t2);
		fp2_mul(r->y, r->y, t2);
		fp2_sub(r->y, t4, r->y);

		/* Z3 = 2*F^2. */
		fp2_dbl(r->z, r->z);
		fp2_copy(r->x, t5);

		r->coord = PROJC;
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp2_free(t0);
		fp2_free(t1);
		fp2_free(t2);
		fp2_free(t3);
		fp2_free(t4);
		fp2_free(t5);
	}
}

#endif

#endif
