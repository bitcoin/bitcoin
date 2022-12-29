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
 * Implementation of the final exponentiation for curves of embedding degree 12.
 *
 * @ingroup pp
 */

#include "relic_core.h"
#include "relic_pp.h"
#include "relic_util.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Computes the final exponentiation of a pairing defined over a Barreto-Naehrig
 * curve.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the extension field element to exponentiate.
 */
static void pp_exp_bn(fp12_t c, fp12_t a) {
	fp12_t t0, t1, t2, t3;
	bn_t x;
	const int *b;
	int l;

	fp12_null(t0);
	fp12_null(t1);
	fp12_null(t2);
	fp12_null(t3);
	bn_null(x);

	RLC_TRY {
		fp12_new(t0);
		fp12_new(t1);
		fp12_new(t2);
		fp12_new(t3);
		bn_new(x);

		/*
		 * New final exponentiation following Fuentes-Castañeda, Knapp and
		 * Rodríguez-Henríquez: Fast Hashing to G_2.
		 */
		fp_prime_get_par(x);
		b = fp_prime_get_par_sps(&l);

		/* First, compute m = f^(p^6 - 1)(p^2 + 1). */
		fp12_conv_cyc(c, a);

		/* Now compute m^((p^4 - p^2 + 1) / r). */
		/* t0 = m^2x. */
		fp12_exp_cyc_sps(t0, c, b, l, RLC_POS);
		fp12_sqr_cyc(t0, t0);
		/* t1 = m^6x. */
		fp12_sqr_cyc(t1, t0);
		fp12_mul(t1, t1, t0);
		/* t2 = m^6x^2. */
		fp12_exp_cyc_sps(t2, t1, b, l, RLC_POS);
		/* t3 = m^12x^3. */
		fp12_sqr_cyc(t3, t2);
		fp12_exp_cyc_sps(t3, t3, b, l, RLC_POS);

		if (bn_sign(x) == RLC_NEG) {
			fp12_inv_cyc(t0, t0);
			fp12_inv_cyc(t1, t1);
			fp12_inv_cyc(t3, t3);
		}

		/* t3 = a = m^12x^3 * m^6x^2 * m^6x. */
		fp12_mul(t3, t3, t2);
		fp12_mul(t3, t3, t1);

		/* t0 = b = 1/(m^2x) * t3. */
		fp12_inv_cyc(t0, t0);
		fp12_mul(t0, t0, t3);

		/* Compute t2 * t3 * m * b^p * a^p^2 * [b * 1/m]^p^3. */
		fp12_mul(t2, t2, t3);
		fp12_mul(t2, t2, c);
		fp12_inv_cyc(c, c);
		fp12_mul(c, c, t0);
		fp12_frb(c, c, 3);
		fp12_mul(c, c, t2);
		fp12_frb(t0, t0, 1);
		fp12_mul(c, c, t0);
		fp12_frb(t3, t3, 2);
		fp12_mul(c, c, t3);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp12_free(t0);
		fp12_free(t1);
		fp12_free(t2);
		fp12_free(t3);
		bn_free(x);
	}
}

/**
 * Computes the final exponentiation of a pairing defined over a
 * Barreto-Lynn-Scott curve.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the extension field element to exponentiate.
 */
static void pp_exp_b12(fp12_t c, fp12_t a) {
	fp12_t t0, t1, t2, t3;
	bn_t x;
	const int *b;
	int l;

	fp12_null(t0);
	fp12_null(t1);
	fp12_null(t2);
	fp12_null(t3);
	bn_null(x);

	RLC_TRY {
		fp12_new(t0);
		fp12_new(t1);
		fp12_new(t2);
		fp12_new(t3);
		bn_new(x);

		/*
		 * Final exponentiation following Ghammam and Fouotsa:
		 * On the Computation of Optimal Ate Pairing at the 192-bit Level.
		 */
		fp_prime_get_par(x);
		b = fp_prime_get_par_sps(&l);

		/* First, compute m^(p^6 - 1)(p^2 + 1). */
		fp12_conv_cyc(c, a);

		/* Now compute m^((p^4 - p^2 + 1) / r). */
		/* t0 = f^2. */
		fp12_sqr_cyc(t0, c);

		/* t1 = f^x. */
		fp12_exp_cyc_sps(t1, c, b, l, bn_sign(x));

		/* t2 = f^(x^2). */
		fp12_exp_cyc_sps(t2, t1, b, l, bn_sign(x));

		/* t1 = t2/(t1^2 * f). */
		fp12_inv_cyc(t3, c);
		fp12_sqr_cyc(t1, t1);
		fp12_mul(t1, t1, t3);
		fp12_inv_cyc(t1, t1);
		fp12_mul(t1, t1, t2);

		/* t2 = t1^x. */
		fp12_exp_cyc_sps(t2, t1, b, l, bn_sign(x));

		/* t3 = t2^x/t1. */
		fp12_exp_cyc_sps(t3, t2, b, l, bn_sign(x));
		fp12_inv_cyc(t1, t1);
		fp12_mul(t3, t1, t3);

		/* t1 = t1^(-p^3 ) * t2^(p^2). */
		fp12_inv_cyc(t1, t1);
		fp12_frb(t1, t1, 3);
		fp12_frb(t2, t2, 2);
		fp12_mul(t1, t1, t2);

		/* t2 = f * f^2 * t3^x. */
		fp12_exp_cyc_sps(t2, t3, b, l, bn_sign(x));
		fp12_mul(t2, t2, t0);
		fp12_mul(t2, t2, c);

		/* Compute t1 * t2 * t3^p. */
		fp12_mul(t1, t1, t2);
		fp12_frb(t2, t3, 1);
		fp12_mul(c, t1, t2);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp12_free(t0);
		fp12_free(t1);
		fp12_free(t2);
		fp12_free(t3);
		bn_free(x);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void pp_exp_k12(fp12_t c, fp12_t a) {
	switch (ep_curve_is_pairf()) {
		case EP_BN:
			pp_exp_bn(c, a);
			break;
		case EP_B12:
			pp_exp_b12(c, a);
			break;
	}
}
