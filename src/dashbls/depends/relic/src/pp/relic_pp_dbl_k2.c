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
 * Implementation of Miller doubling for curves of embedding degree 2.
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

void pp_dbl_k2_basic(fp2_t l, ep_t r, ep_t p, ep_t q) {
	fp_t s;
	ep_t t;

	fp_null(s);
	ep_null(t);

	RLC_TRY {
		fp_new(s);
		ep_new(t);

		ep_copy(t, p);
		ep_dbl_slp_basic(r, s, p);
		fp_add(l[0], t->x, q->x);
		fp_mul(l[0], l[0], s);
		fp_sub(l[0], t->y, l[0]);
		fp_copy(l[1], q->y);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp_free(s);
		ep_free(t);
	}
}

#endif

#if EP_ADD == PROJC || EP_ADD == JACOB || !defined(STRIP)

#if PP_EXT == BASIC || !defined(STRIP)

void pp_dbl_k2_projc_basic(fp2_t l, ep_t r, ep_t p, ep_t q) {
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

		/* For these curves, we always can choose a = -3. */
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

		fp_dbl(t2, t2);
		fp_dbl(t2, t2);

		/* z3 = (y1 + z1)^2 - gamma - delta. */
		fp_add(r->z, p->y, p->z);
		fp_sqr(r->z, r->z);
		fp_sub(r->z, r->z, t1);
		fp_sub(r->z, r->z, t0);

		/* l0 = 2 * gamma - alpha * (delta * xq + x1). */
		fp_dbl(t1, t1);
		fp_mul(t5, t0, q->x);
		fp_add(t5, t5, p->x);
		fp_mul(t5, t5, t3);
		fp_sub(l[0], t1, t5);

		/* x3 = alpha^2 - 8 * beta. */
		fp_dbl(t5, t2);
		fp_sqr(r->x, t3);
		fp_sub(r->x, r->x, t5);

		/* y3 = alpha * (4 * beta - x3) - 8 * gamma^2. */
		fp_sqr(t4, t1);
		fp_dbl(t4, t4);
		fp_sub(r->y, t2, r->x);
		fp_mul(r->y, r->y, t3);
		fp_sub(r->y, r->y, t4);

		/* l1 = - z3 * delta * yq. */
		fp_mul(l[1], r->z, t0);
		fp_mul(l[1], l[1], q->y);

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

#endif

#if PP_EXT == LAZYR || !defined(STRIP)

void pp_dbl_k2_projc_lazyr(fp2_t l, ep_t r, ep_t p, ep_t q) {
	fp_t t0, t1, t2, t3, t4, t5;
	dv_t u0, u1;

	fp_null(t0);
	fp_null(t1);
	fp_null(t2);
	fp_null(t3);
	fp_null(t4);
	fp_null(t5);
	dv_null(u0);
	dv_null(u1);

	RLC_TRY {
		fp_new(t0);
		fp_new(t1);
		fp_new(t2);
		fp_new(t3);
		fp_new(t4);
		fp_new(t5);
		dv_new(u0);
		dv_new(u1);

		/* For these curves, we always can choose a = -3. */
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

		/* t2 = 4 * beta. */
		fp_dbl(t2, t2);
		fp_dbl(t2, t2);

		/* z3 = (y1 + z1)^2 - gamma - delta. */
		fp_add(r->z, p->y, p->z);
		fp_sqr(r->z, r->z);
		fp_sub(r->z, r->z, t1);
		fp_sub(r->z, r->z, t0);

		/* l0 = 2 * gamma - alpha * (delta * xq + x1). */
		fp_dbl(t1, t1);
		fp_mul(t5, t0, q->x);
		fp_add(t5, t5, p->x);
		fp_mul(t5, t5, t3);
		fp_sub(l[0], t1, t5);

		/* x3 = alpha^2 - 8 * beta. */
		fp_dbl(t5, t2);
		fp_sqr(r->x, t3);
		fp_sub(r->x, r->x, t5);

		/* y3 = alpha * (4 * beta - x3) - 8 * gamma^2. */
		fp_sqrn_low(u0, t1);
		fp_addc_low(u0, u0, u0);
		fp_subm_low(r->y, t2, r->x);
		fp_muln_low(u1, r->y, t3);
		fp_subc_low(u1, u1, u0);
		fp_rdcn_low(r->y, u1);

		/* l1 = - z3 * delta * yq. */
		fp_mul(l[1], r->z, t0);
		fp_mul(l[1], l[1], q->y);

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
		dv_free(u0);
		dv_free(u1);
	}
}

#endif

#endif
