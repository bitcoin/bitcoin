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
 * Implementation of the final exponentiation for curves of embedding degree 2.
 *
 * @ingroup pp
 */

#include "relic_core.h"
#include "relic_pp.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void pp_exp_k8(fp8_t c, fp8_t a) {
	fp8_t t0, t1, t2, t3, t4;
	bn_t x, t;

	fp8_null(t0);
	fp8_null(t1);
	fp8_null(t2);
	fp8_null(t3);
	bn_null(x);
	bn_null(t);

	RLC_TRY {
		fp8_new(t0);
		fp8_new(t1);
		fp8_new(t2);
		fp8_new(t3);
		fp8_new(t4);
		bn_new(x);
		bn_new(t);

		fp_prime_get_par(x);

		/* First, compute m = f^(p^4 - 1). */
		fp8_conv_cyc(c, a);

		/* Now compute d = m^(p + T)(p^2 + T^2). */
		fp8_frb(t0, c, 1);
		fp8_exp_cyc(t1, c, x);
		fp8_mul(t0, t0, t1);
		fp8_exp_cyc(t1, t0, x);
		fp8_exp_cyc(t1, t1, x);
		fp8_frb(t0, t0, 2);
		fp8_mul(t0, t0, t1);

		/* Now compute d^(p + T)/r. */
		bn_set_dig(t, 0xdc);
		bn_lsh(t, t, 8);
		bn_add_dig(t, t, 0x4);
		/* t1 = a_y = a^y, t2 = a_u = a^u */
		fp8_exp_cyc(t1, t0, t);
		fp8_copy(t2, t0);
		/* t3 = a_q = a_y^y * a_u^u, b = a_q * !a_u. */
		fp8_exp_cyc(t3, t1, t);
		fp8_copy(t4, t2);
		fp8_mul(t3, t3, t4);
		fp8_inv_cyc(t2, t2);
		fp8_mul(t4, t3, t2);

		bn_rsh(x, x, 2);
		/* b = (b^4 * a)^(T/4) * a_y. */
		fp8_sqr_cyc(t4, t4);
		fp8_sqr_cyc(t4, t4);
		fp8_mul(t4, t4, t0);
		fp8_exp_cyc(t4, t4, x);
		fp8_mul(t4, t4, t1);

		/* b = (b^4)^U * !a_y. */
		fp8_sqr_cyc(t4, t4);
		fp8_sqr_cyc(t4, t4);
		fp8_exp_cyc(t4, t4, x);
		fp8_inv_cyc(t1, t1);
		fp8_mul(t4, t4, t1);

		/* b = (b^4 * a)^(T/4) * !a_u * a. */
		fp8_sqr_cyc(t4, t4);
		fp8_sqr_cyc(t4, t4);
		fp8_mul(t4, t4, t0);
		fp8_exp_cyc(t4, t4, x);
		fp8_mul(t4, t4, t2);
		fp8_mul(t4, t4, t0);

		/* b = (b^4)^U * a_q. */
		fp8_sqr_cyc(t4, t4);
		fp8_sqr_cyc(t4, t4);
		fp8_exp_cyc(t4, t4, x);
		fp8_mul(t4, t4, t3);

		/* return f^(p^4 - 1)*(1 + (p + T)(p^2 + T^2)(p + T)/r) */
		fp8_mul(c, c, t4);
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
		bn_free(x);
		bn_free(t);
	}
}
