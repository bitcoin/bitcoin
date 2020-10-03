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
 * Implementation of the multiple precision addition and subtraction functions.
 *
 * @ingroup bn
 */

#include "relic_core.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Adds two multiple precision integers, where a >= b.
 *
 * @param[out] c        - the result.
 * @param[in] a         - the first multiple precision integer to add.
 * @param[in] b         - the second multiple precision integer to add.
 */
static void bn_add_imp(bn_t c, const bn_t a, const bn_t b) {
	int max, min;
	dig_t carry;

	max = a->used;
	min = b->used;

	/* Grow the result. */
	bn_grow(c, max);

	if (a->used == b->used) {
		carry = bn_addn_low(c->dp, a->dp, b->dp, max);
	} else {
		carry = bn_addn_low(c->dp, a->dp, b->dp, min);
		carry = bn_add1_low(c->dp + min, a->dp + min, carry, max - min);
	}
	if (carry) {
		bn_grow(c, max + 1);
		c->dp[max] = carry;
	}
	c->used = max + carry;
	bn_trim(c);
}

/**
 * Subtracts two multiple precision integers, where a >= b.
 *
 * @param[out] c        - the result.
 * @param[in] a         - the first multiple precision integer to subtract.
 * @param[in] b         - the second multiple precision integer to subtract.
 */
static void bn_sub_imp(bn_t c, const bn_t a, const bn_t b) {
	int max, min;
	dig_t carry;

	max = a->used;
	min = b->used;

	/* Grow the destination to accomodate the result. */
	bn_grow(c, max);

	if (a->used == b->used) {
		carry = bn_subn_low(c->dp, a->dp, b->dp, min);
	} else {
		carry = bn_subn_low(c->dp, a->dp, b->dp, min);
		carry = bn_sub1_low(c->dp + min, a->dp + min, carry, max - min);
	}
	c->used = max;
	bn_trim(c);
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void bn_add(bn_t c, const bn_t a, const bn_t b) {
	int sa, sb;

	sa = a->sign;
	sb = b->sign;

	if (sa == sb) {
		/* If the signs are equal, copy the sign and add. */
		c->sign = sa;
		if (bn_cmp_abs(a, b) == CMP_LT) {
			bn_add_imp(c, b, a);
		} else {
			bn_add_imp(c, a, b);
		}
	} else {
		/* If the signs are different, subtract. */
		if (bn_cmp_abs(a, b) == CMP_LT) {
			c->sign = sb;
			bn_sub_imp(c, b, a);
		} else {
			c->sign = sa;
			bn_sub_imp(c, a, b);
		}
	}
}

void bn_add_dig(bn_t c, const bn_t a, dig_t b) {
	dig_t carry;

	bn_grow(c, a->used);

	if (a->sign == BN_POS) {
		carry = bn_add1_low(c->dp, a->dp, b, a->used);
		if (carry) {
			bn_grow(c, a->used + 1);
			c->dp[a->used] = carry;
		}
		c->used = a->used + carry;
		c->sign = BN_POS;
	} else {
		/* If a < 0 && |a| >= b, compute c = -(|a| - b). */
		if (a->used > 1 || a->dp[0] >= b) {
			carry = bn_sub1_low(c->dp, a->dp, b, a->used);
			c->used = a->used;
			c->sign = BN_NEG;
		} else {
			/* If a < 0 && |a| < b. */
			if (a->used == 1) {
				c->dp[0] = b - a->dp[0];
			} else {
				c->dp[0] = b;
			}
			c->used = 1;
			c->sign = BN_POS;
		}
	}
	bn_trim(c);
}

void bn_sub(bn_t c, const bn_t a, const bn_t b) {
	int sa, sb;

	sa = a->sign;
	sb = b->sign;

	if (sa != sb) {
		/* If the signs are different, copy the sign of the first number and
		 * add. */
		c->sign = sa;
		if (bn_cmp_abs(a, b) == CMP_LT) {
			bn_add_imp(c, b, a);
		} else {
			bn_add_imp(c, a, b);
		}
	} else {
		/* If the signs are equal, adjust the sign and subtract. */
		if (bn_cmp_abs(a, b) != CMP_LT) {
			c->sign = sa;
			bn_sub_imp(c, a, b);
		} else {
			c->sign = (sa == BN_POS) ? BN_NEG : BN_POS;
			bn_sub_imp(c, b, a);
		}
	}
}

void bn_sub_dig(bn_t c, const bn_t a, dig_t b) {
	dig_t carry;

	bn_grow(c, a->used);

	/* If a < 0, compute c = -(|a| + b). */
	if (a->sign == BN_NEG) {
		carry = bn_add1_low(c->dp, a->dp, b, a->used);
		if (carry) {
			bn_grow(c, a->used + 1);
			c->dp[a->used] = carry;
		}
		c->used = a->used + carry;
		c->sign = BN_NEG;
	} else {
		/* If a > 0 && |a| >= b, compute c = (|a| - b). */
		if (a->used > 1 || a->dp[0] >= b) {
			carry = bn_sub1_low(c->dp, a->dp, b, a->used);
			c->used = a->used;
			c->sign = BN_POS;
		} else {
			/* If a > 0 && a < b. */
			if (a->used == 1) {
				c->dp[0] = b - a->dp[0];
			} else {
				c->dp[0] = b;
			}
			c->used = 1;
			c->sign = BN_NEG;
		}
	}
	bn_trim(c);
}
