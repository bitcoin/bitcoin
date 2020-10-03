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
 * Implementation of the multiple precision multiplication functions.
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
 * Multiplies two multiple precision integers using recursive Karatsuba
 * multiplication.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the first multiple precision integer.
 * @param[in] b				- the second multiple precision integer.
 * @param[in] level			- the number of Karatsuba steps to apply.
 */
static void bn_mul_karat_imp(bn_t c, const bn_t a, const bn_t b, int level) {
	int h;
	bn_t a0, a1, b0, b1, a0b0, a1b1;
	bn_t t;
	const dig_t *tmpa, *tmpb;
	dig_t *t0;

	bn_null(a0);
	bn_null(a1);
	bn_null(b0);
	bn_null(b1);
	bn_null(a0b0);
	bn_null(a1b1);
	bn_null(t);

	/* Compute half the digits of a or b. */
	h = MIN(a->used, b->used) >> 1;

	TRY {
		/* Allocate the temp variables. */
		bn_new(a0);
		bn_new(a1);
		bn_new(b0);
		bn_new(b1);
		bn_new(a0b0);
		bn_new(a1b1);
		bn_new(t);

		a0->used = b0->used = h;
		a1->used = a->used - h;
		b1->used = b->used - h;

		tmpa = a->dp;
		tmpb = b->dp;

		/* a = a1 || a0 */
		t0 = a0->dp;
		for (int i = 0; i < h; i++, t0++, tmpa++)
			*t0 = *tmpa;
		t0 = a1->dp;
		for (int i = 0; i < a1->used; i++, t0++, tmpa++)
			*t0 = *tmpa;

		/* b = b1 || b0 */
		t0 = b0->dp;
		for (int i = 0; i < h; i++, t0++, tmpb++)
			*t0 = *tmpb;
		t0 = b1->dp;
		for (int i = 0; i < b1->used; i++, t0++, tmpb++)
			*t0 = *tmpb;

		bn_trim(a0);
		bn_trim(b0);
		bn_trim(a1);
		bn_trim(b1);

		/* a0b0 = a0 * b0 and a1b1 = a1 * b1 */
		if (level <= 1) {
#if BN_MUL == BASIC
			bn_mul_basic(a0b0, a0, b0);
			bn_mul_basic(a1b1, a1, b1);
#elif BN_MUL == COMBA
			bn_mul_comba(a0b0, a0, b0);
			bn_mul_comba(a1b1, a1, b1);
#endif
		} else {
			bn_mul_karat_imp(a0b0, a0, b0, level - 1);
			bn_mul_karat_imp(a1b1, a1, b1, level - 1);
		}

		/* t = (a1 + a0) */
		bn_add(a1, a1, a0);
		/* t2 = (b1 + b0) */
		bn_add(b1, b1, b0);

		/* t = (a1 + a0)*(b1 + b0) */
		if (level <= 1) {
#if BN_MUL == BASIC
			bn_mul_basic(t, a1, b1);
#elif BN_MUL == COMBA
			bn_mul_comba(t, a1, b1);
#endif
		} else {
			bn_mul_karat_imp(t, a1, b1, level - 1);
		}
		/* t2 = (a0*b0 + a1*b1) */
		bn_sub(t, t, a0b0);

		/* t = (a1 + a0)*(b1 + b0) - (a0*b0 + a1*b1) */
		bn_sub(t, t, a1b1);

		/* t = (a1 + a0)*(b1 + b0) - (a0*b0 + a1*b1) << h digits */
		bn_lsh(t, t, h * BN_DIGIT);

		/* t2 = a1 * b1 << 2*h digits */
		bn_lsh(a1b1, a1b1, 2 * h * BN_DIGIT);

		/* t = t + a0*b0 */
		bn_add(t, t, a0b0);

		/* c = t + a1*b1 */
		bn_add(t, t, a1b1);

		t->sign = a->sign ^ b->sign;
		bn_copy(c, t);
		bn_trim(c);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(a0);
		bn_free(a1);
		bn_free(b0);
		bn_free(b1);
		bn_free(a0b0);
		bn_free(a1b1);
		bn_free(t);
	}
}

#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void bn_mul_dig(bn_t c, const bn_t a, dig_t b) {
	bn_grow(c, a->used + 1);
	c->sign = a->sign;
	c->dp[a->used] = bn_mul1_low(c->dp, a->dp, b, a->used);
	c->used = a->used + 1;
	bn_trim(c);
}

#if BN_MUL == BASIC || !defined(STRIP)

void bn_mul_basic(bn_t c, const bn_t a, const bn_t b) {
	int i;
	bn_t t;
	dig_t carry;

	bn_null(t);

	TRY {
		/* We need a temporary variable so that c can be a or b. */
		bn_new_size(t, a->used + b->used);
		bn_zero(t);
		t->used = a->used + b->used;

		for (i = 0; i < a->used; i++) {
			carry = bn_mula_low(t->dp + i, b->dp, *(a->dp + i), b->used);
			*(t->dp + i + b->used) = carry;
		}
		t->sign = a->sign ^ b->sign;
		bn_trim(t);

		/* Swap c and t. */
		bn_copy(c, t);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(t);
	}
}

#endif

#if BN_MUL == COMBA || !defined(STRIP)

void bn_mul_comba(bn_t c, const bn_t a, const bn_t b) {
	int digits;
	bn_t t;

	bn_null(t);

	TRY {
		digits = a->used + b->used;

		/* We need a temporary variable so that c can be a or b. */
		bn_new_size(t, digits);
		t->used = digits;

		if (a->used == b->used) {
			bn_muln_low(t->dp, a->dp, b->dp, a->used);
		} else {
			if (a->used > b->used) {
				bn_muld_low(t->dp, a->dp, a->used, b->dp, b->used, 0,
						a->used + b->used);
			} else {
				bn_muld_low(t->dp, b->dp, b->used, a->dp, a->used, 0,
						a->used + b->used);
			}
		}

		t->sign = a->sign ^ b->sign;
		bn_trim(t);
		bn_copy(c, t);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(t);
	}
}

#endif

#if BN_KARAT > 0 || !defined(STRIP)

void bn_mul_karat(bn_t c, const bn_t a, const bn_t b) {
	bn_mul_karat_imp(c, a, b, BN_KARAT);
}

#endif
