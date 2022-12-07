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
 * Implementation of the final exponentiation for curves of embedding degree 48.
 *
 * @ingroup pp
 */

#include "relic_core.h"
#include "relic_pp.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void pp_exp_k48(fp48_t c, fp48_t a) {
	bn_t x;
	const int *b;
	fp48_t t0, t1, t2, t3;
	int l;

	fp48_null(t0);
	fp48_null(t1);
	fp48_null(t2);
	fp48_null(t3);
	bn_null(x);

	RLC_TRY {
		fp48_new(t0);
		fp48_new(t1);
		fp48_new(t2);
		fp48_new(t3);
		bn_new(x);

		/*
		 * Final exponentiation following Ghammam and Fouotsa:
		 * On the Computation of Optimal Ate Pairing at the 192-bit Level.
		 */
		fp_prime_get_par(x);
		b = fp_prime_get_par_sps(&l);
		/* First, compute m^(p^24 - 1)(p^8 + 1). */
		fp48_conv_cyc(c, a);

		/* Now compute m^((p^16 - p^8 + 1) / r). */
		/*k0 + k1^p + k2^p2 + k3^p3 + k4^p4 + k5^p5 + k6^p6 +
		 * k7^p7 + k8^p8 + k9^p9 + k10^p10 + k11^p11 + k12^p12 +
		 * k13^p13 + k14p^14 + k15p^15.
		 * 
		 * k15 = (x - 1)^2 , k14 = xk15, k13 = xk14, k12 = xk13, k11 = xk12,
		 * k10 = xk11, k9 = xk10, k8 = xk9 , k7 = xk8 - k15, k6 = xk7, k5 = xk6,
		 * k4 = xk5, k3 = xk4, k2 = xk3, k1 = xk2 , k0 = xk1 + 3 */

		/* t2 = c^k15, t3 = t2^p^15. */
		fp48_exp_cyc_sps(t0, c, b, l, bn_sign(x));
		fp48_inv_cyc(t1, c);
		fp48_mul(t0, t0, t1);
		fp48_exp_cyc_sps(t1, t0, b, l, bn_sign(x));
		fp48_inv_cyc(t2, t0);
		fp48_mul(t2, t1, t2);
		/* t3 = c^3 (for handling part of k0). */
		fp48_sqr_cyc(t3, c);
		fp48_mul(c, c, t3);
		fp48_frb(t3, t2, 15);
		fp48_mul(c, c, t3);

		/* t0 = c^k14, t3 = t0^p^14. */
		fp48_exp_cyc_sps(t0, t2, b, l, bn_sign(x));
		fp48_frb(t3, t0, 14);
		fp48_mul(c, c, t3);

		/* t1 = c^x13, t3 = t1^p^13. */
		fp48_exp_cyc_sps(t1, t0, b, l, bn_sign(x));
		fp48_frb(t3, t1, 13);
		fp48_mul(c, c, t3);

		/* t0 = c^x12, t3 = t0^p^12. */
		fp48_exp_cyc_sps(t0, t1, b, l, bn_sign(x));
		fp48_frb(t3, t0, 12);
		fp48_mul(c, c, t3);

		/* t0 = c^x11, t3 = t0^p^11. */
		fp48_exp_cyc_sps(t1, t0, b, l, bn_sign(x));
		fp48_frb(t3, t1, 11);
		fp48_mul(c, c, t3);

		/* t1 = c^x10, t3 = t1^p^10. */
		fp48_exp_cyc_sps(t0, t1, b, l, bn_sign(x));
		fp48_frb(t3, t0, 10);
		fp48_mul(c, c, t3);

		/* t0 = c^x9, t3 = t0^p^9. */
		fp48_exp_cyc_sps(t1, t0, b, l, bn_sign(x));
		fp48_frb(t3, t1, 9);
		fp48_mul(c, c, t3);

		/* t1 = c^x8, t3 = t1^p^8. */
		fp48_exp_cyc_sps(t0, t1, b, l, bn_sign(x));
		fp48_frb(t3, t0, 8);
		fp48_mul(c, c, t3);

		/* t1 = c^(xk8 - k15), t3 = t1^p^7. */
		fp48_exp_cyc_sps(t1, t0, b, l, bn_sign(x));
		fp48_inv_cyc(t2, t2);
		fp48_mul(t1, t1, t2);
		fp48_frb(t3, t1, 7);
		fp48_mul(c, c, t3);

		/* t0 = c^xk7, t3 = t0^p^6. */
		fp48_exp_cyc_sps(t0, t1, b, l, bn_sign(x));
		fp48_frb(t3, t0, 6);
		fp48_mul(c, c, t3);

		/* t1 = c^xk6, t3 = t1^p^5. */
		fp48_exp_cyc_sps(t1, t0, b, l, bn_sign(x));
		fp48_frb(t3, t1, 5);
		fp48_mul(c, c, t3);

		/* t0 = c^xk5, t3 = t0^p^4. */
		fp48_exp_cyc_sps(t0, t1, b, l, bn_sign(x));
		fp48_frb(t3, t0, 4);
		fp48_mul(c, c, t3);

		/* t0 = c^xk4, t3 = t0^p^3. */
		fp48_exp_cyc_sps(t1, t0, b, l, bn_sign(x));
		fp48_frb(t3, t1, 3);
		fp48_mul(c, c, t3);

		/* t0 = c^xk3, t3 = t0^p^2. */
		fp48_exp_cyc_sps(t0, t1, b, l, bn_sign(x));
		fp48_frb(t3, t0, 2);
		fp48_mul(c, c, t3);

		/* t1 = c^xk2, t3 = t1^p. */
		fp48_exp_cyc_sps(t1, t0, b, l, bn_sign(x));
		fp48_frb(t3, t1, 1);
		fp48_mul(c, c, t3);

		/* t1 = c^(xk1). */
		fp48_exp_cyc_sps(t0, t1, b, l, bn_sign(x));
		fp48_mul(c, c, t0);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp48_free(t0);
		fp48_free(t1);
		fp48_free(t2);
		fp48_free(t3);
		bn_free(x);
	}
}
