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
 * Implementation of the final exponentiation for curves of embedding degree 24.
 *
 * @ingroup pp
 */

#include "relic_core.h"
#include "relic_pp.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void pp_exp_k24(fp24_t c, fp24_t a) {
	fp24_t t[8];
	const int *b;
	bn_t x;
	int l;

	bn_null(x);

	RLC_TRY {
		bn_new(x);
		for (int i = 0; i < 8; i++) {
			fp24_null(t[i]);
			fp24_new(t[i]);
		}

		fp_prime_get_par(x);
		b = fp_prime_get_par_sps(&l);
		/* First, compute m^(p^12 - 1)(p^4 + 1). */
		fp24_conv_cyc(c, a);

		/* v7 = f^x. */
		fp24_exp_cyc_sps(t[7], c, b, l, bn_sign(x));

		/* v6 = f^(-2x + 1). */
		fp24_sqr_cyc(t[6], t[7]);
		fp24_inv_cyc(t[6], t[6]);
		fp24_mul(t[6], t[6], c);

		/* v7 = f^(x^2 - 2x + 1). */
		fp24_exp_cyc_sps(t[7], t[7], b, l, bn_sign(x));
		fp24_mul(t[7], t[7], t[6]);

		fp24_exp_cyc_sps(t[6], t[7], b, l, bn_sign(x));

		fp24_exp_cyc_sps(t[5], t[6], b, l, bn_sign(x));

		fp24_exp_cyc_sps(t[4], t[5], b, l, bn_sign(x));

		fp24_exp_cyc_sps(t[3], t[4], b, l, bn_sign(x));
		fp24_inv_cyc(t[2], t[7]);
		fp24_mul(t[3], t[3], t[2]);

		fp24_exp_cyc_sps(t[2], t[3], b, l, bn_sign(x));

		fp24_exp_cyc_sps(t[1], t[2], b, l, bn_sign(x));

		/* c = c^3. */
		fp24_sqr_cyc(t[0], c);
		fp24_mul(c, c, t[0]);

		fp24_exp_cyc_sps(t[0], t[1], b, l, bn_sign(x));

		fp24_mul(c, c, t[0]);
		for (int i = 1; i < 8; i++) {
			fp24_frb(t[i], t[i], i);
			fp24_mul(c, c, t[i]);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(x);
		for (int i = 0; i < 8; i++) {
			fp24_free(t[i]);
		}
	}
}
