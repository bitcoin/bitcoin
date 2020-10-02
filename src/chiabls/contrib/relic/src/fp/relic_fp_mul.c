/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2017 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * RELIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RELIC. If not, see <http://www.gnu.org/licenses/>.
 */
/**
 * @file
 *
 * Implementation of the prime field multiplication functions.
 *
 * @ingroup fp
 */

#include "relic_core.h"
#include "relic_fp_low.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if FP_KARAT > 0 || !defined(STRIP)

/**
 * Multiplies two prime field elements using recursive Karatsuba
 * multiplication.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first prime field element.
 * @param[in] b				- the second prime field element.
 * @param[in] size			- the number of digits to multiply.
 * @param[in] level			- the number of Karatsuba steps to apply.
 */
static void fp_mul_karat_imp(dv_t c, const fp_t a, const fp_t b, int size,
		int level) {
	int i, h, h1;
	dv_t a1, b1, a0b0, a1b1, t;
	dig_t carry;

	/* Compute half the digits of a or b. */
	h = size >> 1;
	h1 = size - h;

	dv_null(a1);
	dv_null(b1);
	dv_null(a0b0);
	dv_null(a1b1);

	TRY {
		/* Allocate the temp variables. */
		dv_new(a1);
		dv_new(b1);
		dv_new(a0b0);
		dv_new(a1b1);
		dv_new(t);
		dv_zero(a1, h1 + 1);
		dv_zero(b1, h1 + 1);
		dv_zero(a0b0, 2 * h);
		dv_zero(a1b1, 2 * h1);
		dv_zero(t, 2 * h1 + 1);

		/* a0b0 = a0 * b0 and a1b1 = a1 * b1 */
		if (level <= 1) {
#if FP_MUL == BASIC
			for (i = 0; i < h; i++) {
				carry = bn_mula_low(a0b0 + i, a, *(b + i), h);
				*(a0b0 + i + h) = carry;
			}
			for (i = 0; i < h1; i++) {
				carry = bn_mula_low(a1b1 + i, a + h, *(b + h + i), h1);
				*(a1b1 + i + h1) = carry;
			}
#elif FP_MUL == COMBA || FP_MUL == INTEG
			bn_muln_low(a0b0, a, b, h);
			bn_muln_low(a1b1, a + h, b + h, h1);
#endif
		} else {
			fp_mul_karat_imp(a0b0, a, b, h, level - 1);
			fp_mul_karat_imp(a1b1, a + h, b + h, h1, level - 1);
		}

		for (i = 0; i < 2 * h; i++) {
			c[i] = a0b0[i];
		}
		for (i = 0; i < 2 * h1 + 1; i++) {
			c[2 * h + i] = a1b1[i];
		}

		/* a1 = (a1 + a0) */
		carry = bn_addn_low(a1, a, a + h, h);
		bn_add1_low(a1 + h, a1 + h, carry, 2);
		if (h1 > h) {
			bn_add1_low(a1 + h, a1 + h, *(a + 2 * h), 2);
		}

		/* b1 = (b1 + b0) */
		carry = bn_addn_low(b1, b, b + h, h);
		bn_add1_low(b1 + h, b1 + h, carry, 2);
		if (h1 > h) {
			bn_add1_low(b1 + h, b1 + h, *(b + 2 * h), 2);
		}

		if (level <= 1) {
			/* t = (a1 + a0)*(b1 + b0) */
#if FP_MUL == BASIC
			for (i = 0; i < h1 + 1; i++) {
				carry = bn_mula_low(t + i, a1, *(b1 + i), h1 + 1);
				*(t + i + h1 + 1) = carry;
			}
#elif FP_MUL == COMBA || FP_MUL == INTEG
			bn_muln_low(t, a1, b1, h1 + 1);
#endif
		} else {
			fp_mul_karat_imp(t, a1, b1, h1 + 1, level - 1);
		}

		/* t = t - (a0*b0 << h digits) */
		carry = bn_subn_low(t, t, a0b0, 2 * h);
		bn_sub1_low(t + 2 * h, t + 2 * h, carry, 2 * (h1 + 1) - 2 * h);

		/* t = t - (a1*b1 << h digits) */
		carry = bn_subn_low(t, t, a1b1, 2 * h1);
		bn_sub1_low(t + 2 * h1, t + 2 * h1, carry, 2 * (h1 + 1) - 2 * h1);

		/* c = c + [(a1 + a0)*(b1 + b0) << digits] */
		c += h;
		carry = bn_addn_low(c, c, t, 2 * (h1 + 1));
		c += 2 * (h1 + 1);
		bn_add1_low(c, c, carry, 2 * size - h - 2 * (h1 + 1));
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(a1);
		dv_free(b1);
		dv_free(a0b0);
		dv_free(a1b1);
		dv_free(t);
	}
}

#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp_mul_dig(fp_t c, const fp_t a, dig_t b) {
	dv_t t;

	dv_null(t);

	TRY {
		dv_new(t);
		fp_prime_conv_dig(t, b);
		fp_mul(c, a, t);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(t);
	}
}

#if FP_MUL == BASIC || !defined(STRIP)

void fp_mul_basic(fp_t c, const fp_t a, const fp_t b) {
	int i;
	dv_t t;
	dig_t carry;

	dv_null(t);

	TRY {
		/* We need a temporary variable so that c can be a or b. */
		dv_new(t);
		dv_zero(t, 2 * FP_DIGS);
		for (i = 0; i < FP_DIGS; i++) {
			carry = fp_mula_low(t + i, b, *(a + i));
			*(t + i + FP_DIGS) = carry;
		}

		fp_rdc(c, t);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(t);
	}
}

#endif

#if FP_MUL == COMBA || !defined(STRIP)

void fp_mul_comba(fp_t c, const fp_t a, const fp_t b) {
	dv_t t;

	dv_null(t);

	TRY {
		/* We need a temporary variable so that c can be a or b. */
		dv_new(t);

		fp_muln_low(t, a, b);

		fp_rdc(c, t);

		dv_free(t);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(t);
	}
}

#endif

#if FP_MUL == INTEG || !defined(STRIP)

void fp_mul_integ(fp_t c, const fp_t a, const fp_t b) {
	fp_mulm_low(c, a, b);
}

#endif

#if FP_KARAT > 0 || !defined(STRIP)

void fp_mul_karat(fp_t c, const fp_t a, const fp_t b) {
	dv_t t;

	dv_null(t);

	TRY {
		/* We need a temporary variable so that c can be a or b. */
		dv_new(t);

		dv_zero(t, 2 * FP_DIGS);

		if (FP_DIGS > 1) {
			fp_mul_karat_imp(t, a, b, FP_DIGS, FP_KARAT);
		} else {
			fp_muln_low(t, a, b);
		}

		fp_rdc(c, t);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(t);
	}
}

#endif
