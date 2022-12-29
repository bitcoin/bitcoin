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
 * Implementation of the multiple precision arithmetic squaring
 * functions.
 *
 * @ingroup bn
 */

#include "relic_core.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if BN_KARAT > 0 || !defined(STRIP)

/**
 * Computes the square of a multiple precision integer using recursive Karatsuba
 * squaring.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the multiple precision integer to square.
 * @param[in] level			- the number of Karatsuba steps to apply.
 */
static void bn_sqr_karat_imp(bn_t c, const bn_t a, int level) {
	bn_t a0, a1, a0a0, a1a1, t;
	int h = a->used >> 1;

	bn_null(a0);
	bn_null(a1);
	bn_null(a0a0);
	bn_null(a1a1);
	bn_null(t);

	RLC_TRY {
		/* Allocate the temp variables. */
		bn_new_size(a0, h);
		bn_new_size(a1, a->used - h);
		bn_new(a0a0);
		bn_new(a1a1);
		bn_new(t);

		/* a = a1 || a0 */
		a0->used = h;
		a1->used = a->used - h;
		dv_copy(a0->dp, a->dp, h);
		dv_copy(a1->dp, a->dp + h, a->used - h);
		bn_trim(a0);
		bn_trim(a1);

		if (level <= 1) {
			/* a0a0 = a0 * a0 and a1a1 = a1 * a1 */
#if BN_SQR == BASIC
			bn_sqr_basic(a0a0, a0);
			bn_sqr_basic(a1a1, a1);
#elif BN_SQR == COMBA
			bn_sqr_comba(a0a0, a0);
			bn_sqr_comba(a1a1, a1);
#elif BN_SQR == MULTP
			bn_mul_comba(a0a0, a0, a0);
			bn_mul_comba(a1a1, a1, a1);
#endif
		} else {
			bn_sqr_karat_imp(a0a0, a0, level - 1);
			bn_sqr_karat_imp(a1a1, a1, level - 1);
		}

		/* t = (a1 + a0) */
		bn_add(t, a1, a0);

		if (level <= 1) {
			/* t = (a1 + a0)*(a1 + a0) */
#if BN_SQR == BASIC
			bn_sqr_basic(t, t);
#elif BN_SQR == COMBA
			bn_sqr_comba(t, t);
#elif BN_SQR == MULTP
			bn_mul_comba(t, t, t);
#endif
		} else {
			bn_sqr_karat_imp(t, t, level - 1);
		}

		/* t2 = (a0*a0 + a1*a1) */
		bn_add(a0, a0a0, a1a1);
		/* t = (a1 + a0)*(b1 + b0) - (a0*a0 + a1*a1) */
		bn_sub(t, t, a0);

		/* t = (a1 + a0)*(a1 + a0) - (a0*a0 + a1*a1) << h digits */
		bn_lsh(t, t, h * RLC_DIG);

		/* t2 = a1 * b1 << 2*h digits */
		bn_lsh(a1a1, a1a1, 2 * h * RLC_DIG);

		/* t = t + a0*a0 */
		bn_add(t, t, a0a0);
		/* c = t + a1*a1 */
		bn_add(t, t, a1a1);

		t->sign = RLC_POS;
		bn_copy(c, t);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(a0);
		bn_free(a1);
		bn_free(a0a0);
		bn_free(a1a1);
		bn_free(t);
	}
}

#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if BN_SQR == BASIC || !defined(STRIP)

void bn_sqr_basic(bn_t c, const bn_t a) {
	int i;
	bn_t t;

	bn_null(t);

	RLC_TRY {
		bn_new_size(t, 2 * a->used);
		bn_zero(t);

		for (i = 0; i < a->used - 1; i++) {
			t->dp[a->used + i + 1] =
					bn_sqra_low(t->dp + 2 * i, a->dp + i, a->used - i);
		}
		bn_sqra_low(t->dp + 2 * i, a->dp + i, 1);

		t->used = 2 * a->used;
		t->sign = RLC_POS;
		bn_trim(t);
		bn_copy(c, t);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t);
	}
}

#endif

#if BN_SQR == COMBA || !defined(STRIP)

void bn_sqr_comba(bn_t c, const bn_t a) {
	int digits;
	bn_t t;

	bn_null(t);

	digits = 2 * a->used;

	RLC_TRY {
		/* We need a temporary variable so that c can be a or b. */
		bn_new_size(t, digits);
		t->used = digits;

		bn_sqrn_low(t->dp, a->dp, a->used);

		t->sign = RLC_POS;
		bn_trim(t);
		bn_copy(c, t);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t);
	}
}

#endif

#if BN_KARAT > 0 || !defined(STRIP)

void bn_sqr_karat(bn_t c, const bn_t a) {
	bn_sqr_karat_imp(c, a, BN_KARAT);
}

#endif
