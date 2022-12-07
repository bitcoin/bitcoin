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
 * Implementation of the final exponentiation for curves of embedding degree 54.
 *
 * @ingroup pp
 */

#include "relic_core.h"
#include "relic_pp.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/


void pp_exp_k54(fp54_t c, fp54_t a) {
	bn_t x;
	const int *b;
	fp54_t t0, t1, t2, t3, t4, t5, t6, t;
	int l;

	fp54_null(t0);
	fp54_null(t1);
	fp54_null(t2);
	fp54_null(t3);
	fp54_null(t4);
	fp54_null(t5);
	fp54_null(t6);
	fp54_null(t);
	bn_null(x);

	RLC_TRY {
		fp54_new(t0);
		fp54_new(t1);
		fp54_new(t2);
		fp54_new(t3);
		fp54_new(t4);
		fp54_new(t5);
		fp54_new(t6);
		fp54_new(t);
		bn_new(x);

		fp_prime_get_par(x);
		b = fp_prime_get_par_sps(&l);

		/* First, compute m^(p^27 - 1)(p^9 + 1). */
		fp54_conv_cyc(c, a);

		/* Now compute m^((p^18 - p^9 + 1) / r). */
		/*k0 + k1*p + k2*p^2 + k3*p^3 + k4*p^4 + k5*p^5 + k6*p^6 + k7*p^7 +
		 * k8*p^8 + k9*p^9 + k10*p^10 + k11*p^11 + k12*p^12 + k13*p^13 + k14*p^14 +
		 * k15*p^15 + k16*p^16 + k17*p^17
		 *
		 * k17:=3*x^2+3*x+1; k4:=9*x^4*k17; k11:=-3*x^2*k4; k8:=-9*x^3*k11+k17+3*x
		 * k16:=x*k8-x*k17; k7:=-2*k16-3*x*k17; k6:=3*x^2*k8+3*x^2*k17;
		 * k5:=-x*k6-3*x^3*k17; k15:=3*x^2*k17-k6; k13:=3*x*k5+k4;
		 * k12:=x*k4-x*k13; k3:=k12-3*x*k4; k14:=3*x^3*k17-2*x*k15;
		 * k2:=-3*x*k3-3*x^2*k4; k10:=x*k2-x*k11; k9:=-3*x*k10-3*x^2*k11-2;
		 * k1:=-2*x*k2-x*k11; k0:=3*x^2*k11-k9-1; */

		/* t2 = k17 = 3*x^2+3*x+1, t3 = 3*x. */
		fp54_exp_cyc_sps(t0, c, b, l, bn_sign(x));
		fp54_sqr_cyc(t, t0);
		fp54_mul(t0, t0, t);
		fp54_copy(t3, t0);
		fp54_exp_cyc_sps(t, t0, b, l, bn_sign(x));
		fp54_mul(t0, t0, c);
		fp54_copy(t6, c);
		fp54_mul(t2, t0, t);
		fp54_frb(c, t2, 17);

		/* t4 = k4 = 9*x^4*k17. */
		fp54_exp_cyc_sps(t, t2, b, l, bn_sign(x));
		fp54_sqr_cyc(t0, t);
		fp54_mul(t0, t0, t);
		fp54_exp_cyc_sps(t0, t0, b, l, bn_sign(x));
		fp54_exp_cyc_sps(t0, t0, b, l, bn_sign(x));
		fp54_sqr_cyc(t, t0);
		fp54_mul(t0, t0, t);
		fp54_exp_cyc_sps(t0, t0, b, l, bn_sign(x));
		fp54_copy(t4, t0);
		fp54_frb(t, t0, 4);
		fp54_mul(c, c, t);

		/* t0 = t5 = k11 = -3*x^2*k4, t1 = x*k4. */
		fp54_exp_cyc_sps(t1, t4, b, l, bn_sign(x));
		fp54_sqr_cyc(t0, t1);
		fp54_mul(t0, t0, t1);
		fp54_exp_cyc_sps(t0, t0, b, l, bn_sign(x));
		fp54_inv_cyc(t0, t0);
		fp54_copy(t5, t0);
		fp54_frb(t, t0, 11);
		fp54_mul(c, c, t);

		/* t0 = k8 = -9*x^3*k11 + k17 + 3*x. */
		fp54_exp_cyc_sps(t0, t0, b, l, bn_sign(x));
		fp54_sqr_cyc(t, t0);
		fp54_mul(t0, t0, t);
		fp54_exp_cyc_sps(t0, t0, b, l, bn_sign(x));
		fp54_exp_cyc_sps(t0, t0, b, l, bn_sign(x));
		fp54_sqr_cyc(t, t0);
		fp54_mul(t0, t0, t);
		fp54_inv_cyc(t0, t0);
		fp54_mul(t0, t0, t2);
		fp54_mul(t0, t0, t3);
		fp54_frb(t, t0, 8);
		fp54_mul(c, c, t);

		/* t0 = k16 = x*k8 - x*k17, t3 = x*k8, t2 = x*k17. */
		fp54_exp_cyc_sps(t0, t0, b, l, bn_sign(x));
		fp54_exp_cyc_sps(t2, t2, b, l, bn_sign(x));
		fp54_copy(t3, t0);
		fp54_inv_cyc(t2, t2);
		fp54_mul(t0, t0, t2);
		fp54_frb(t, t0, 16);
		fp54_mul(c, c, t);

		/* t0 = k7 = -2*k16 - 3*x*k17, t2 = 3*x*k17. */
		fp54_sqr_cyc(t0, t0);
		fp54_inv_cyc(t0, t0);
		fp54_sqr_cyc(t, t2);
		fp54_mul(t2, t2, t);
		fp54_mul(t0, t0, t2);
		fp54_frb(t, t0, 7);
		fp54_mul(c, c, t);

		/* t3 = k6 = 3*x^2*k8 + 3*x^2*k17, t2 = 3*x^2*k17. */
		fp54_exp_cyc_sps(t3, t3, b, l, bn_sign(x));
		fp54_inv_cyc(t2, t2);
		fp54_exp_cyc_sps(t2, t2, b, l, bn_sign(x));
		fp54_sqr(t0, t3);
		fp54_mul(t3, t3, t0);
		fp54_mul(t3, t3, t2);
		fp54_frb(t, t3, 6);
		fp54_mul(c, c, t);

		/* k15 = 3*x^2*k17 - k6. */
		fp54_inv_cyc(t0, t3);
		fp54_mul(t0, t0, t2);
		fp54_frb(t, t0, 15);
		fp54_mul(c, c, t);

		/* t3 = k5 = -x*k6 - 3*x^3*k17. */
		fp54_exp_cyc_sps(t3, t3, b, l, bn_sign(x));
		fp54_exp_cyc_sps(t2, t2, b, l, bn_sign(x));
		fp54_mul(t3, t3, t2);
		fp54_inv_cyc(t3, t3);
		fp54_frb(t, t3, 5);
		fp54_mul(c, c, t);

		/* k14 = 3*x^3*k17 - 2*x*k15. */
		fp54_exp_cyc_sps(t0, t0, b, l, bn_sign(x));
		fp54_sqr_cyc(t0, t0);
		fp54_inv_cyc(t0, t0);
		fp54_mul(t0, t0, t2);
		fp54_frb(t, t0, 14);
		fp54_mul(c, c, t);

		/* k13 = 3*x*k5 + k4. */
		fp54_exp_cyc_sps(t3, t3, b, l, bn_sign(x));
		fp54_sqr(t0, t3);
		fp54_mul(t0, t0, t3);
		fp54_mul(t0, t0, t4);
		fp54_frb(t, t0, 13);
		fp54_mul(c, c, t);

		/* k12 = x*k4-x*k13. */
		fp54_exp_cyc_sps(t0, t0, b, l, bn_sign(x));
		fp54_inv_cyc(t0, t0);
		fp54_mul(t0, t0, t1);
		fp54_frb(t, t0, 12);
		fp54_mul(c, c, t);

		/* k3 = k12-3*x*k4, t4 = 3*x*k4. */
		fp54_sqr_cyc(t, t1);
		fp54_mul(t4, t1, t);
		fp54_inv_cyc(t4, t4);
		fp54_mul(t0, t0, t4);
		fp54_frb(t, t0, 3);
		fp54_mul(c, c, t);

		/* k2 = -3*x*k3 - 3*x^2*k4. */
		fp54_exp_cyc_sps(t0, t0, b, l, bn_sign(x));
		fp54_sqr(t, t0);
		fp54_mul(t0, t0, t);
		fp54_inv_cyc(t0, t0);
		fp54_mul(t2, t0, t5);
		fp54_frb(t, t2, 2);
		fp54_mul(c, c, t);

		/* k10 = x*k2-x*k11. */
		fp54_exp_cyc_sps(t0, t2, b, l, bn_sign(x));
		fp54_exp_cyc_sps(t5, t5, b, l, bn_sign(x));
		fp54_inv_cyc(t5, t5);
		fp54_mul(t0, t0, t5);
		fp54_frb(t, t0, 10);
		fp54_mul(c, c, t);

		/* k1 = -2*x*k2 - x*k11. */
		fp54_exp_cyc_sps(t2, t2, b, l, bn_sign(x));
		fp54_sqr_cyc(t2, t2);
		fp54_inv_cyc(t2, t2);
		fp54_mul(t2, t2, t5);
		fp54_frb(t, t2, 1);
		fp54_mul(c, c, t);

		/* k9 = -3*x*k10 - 3*x^2*k11 - 2. */
		fp54_exp_cyc_sps(t0, t0, b, l, bn_sign(x));
		fp54_sqr(t, t0);
		fp54_mul(t0, t0, t);
		fp54_inv_cyc(t0, t0);
		fp54_exp_cyc_sps(t5, t5, b, l, bn_sign(x));
		fp54_sqr(t, t5);
		fp54_mul(t5, t5, t);
		fp54_mul(t0, t0, t5);
		fp54_inv_cyc(t6, t6);
		fp54_sqr_cyc(t, t6);
		fp54_mul(t0, t0, t);
		fp54_frb(t, t0, 9);
		fp54_mul(c, c, t);

		/* k0 = 3*x^2*k11 - k9 - 1. */
		fp54_mul(t0, t0, t5);
		fp54_inv_cyc(t0, t0);
		fp54_mul(t0, t0, t6);
		fp54_mul(c, c, t0);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp54_free(t0);
		fp54_free(t1);
		fp54_free(t2);
		fp54_free(t3);
		fp54_free(t4);
		fp54_free(t5);
		fp54_free(t6);
		fp54_free(t);
		bn_free(x);
	}
}
