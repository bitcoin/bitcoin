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
 * Implementation of the multiple precision division functions.
 *
 * @ingroup bn
 */

#include "relic_core.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Divides two multiple precision integers, computing the quotient and the
 * remainder.
 *
 * @param[out] c		- the quotient.
 * @param[out] d		- the remainder.
 * @param[in] a			- the dividend.
 * @param[in] b			- the the divisor.
 */
static void bn_div_imp(bn_t c, bn_t d, const bn_t a, const bn_t b) {
	bn_t q, x, y, r;
	int sign;

	bn_null(q);
	bn_null(x);
	bn_null(y);
	bn_null(r);

	/* If |a| < |b|, we're done. */
	if (bn_cmp_abs(a, b) == RLC_LT) {
		if (bn_sign(a) == bn_sign(b)) {
			if (c != NULL) {
				bn_zero(c);
			}
			if (d != NULL) {
				bn_copy(d, a);
			}
		} else {
			if (c != NULL) {
				bn_set_dig(c, 1);
				bn_neg(c, c);
			}
			if (d != NULL) {
				bn_add(d, a, b);
			}
		}
		return;
	}

	RLC_TRY {
		/* Be conservative about space for scratch memory, many attempts to
		 * optimize these had invalid reads. */
		bn_new_size(x, a->used + 1);
		bn_new_size(q, a->used + 1);
		bn_new_size(y, a->used + 1);
		bn_new_size(r, a->used + 1);
		bn_zero(q);
		bn_zero(r);
		bn_abs(x, a);
		bn_abs(y, b);

		/* Find the sign. */
		sign = (a->sign == b->sign ? RLC_POS : RLC_NEG);

		bn_divn_low(q->dp, r->dp, x->dp, a->used, y->dp, b->used);

		q->used = a->used - b->used + 1;
		q->sign = sign;
		bn_trim(q);

		r->used = b->used;
		r->sign = b->sign;
		bn_trim(r);

		/* We have the quotient in q and the remainder in r. */
		if (c != NULL) {
			if ((bn_is_zero(r)) || (bn_sign(a) == bn_sign(b))) {
				bn_copy(c, q);
			} else {
				bn_sub_dig(c, q, 1);
			}
		}

		if (d != NULL) {
			if ((bn_is_zero(r)) || (bn_sign(a) == bn_sign(b))) {
				bn_copy(d, r);
			} else {
				bn_sub(d, b, r);
			}
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(r);
		bn_free(q);
		bn_free(x);
		bn_free(y);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void bn_div(bn_t c, const bn_t a, const bn_t b) {
	if (bn_is_zero(b)) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}
	bn_div_imp(c, NULL, a, b);
}

void bn_div_rem(bn_t c, bn_t d, const bn_t a, const bn_t b) {
	if (bn_is_zero(b)) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}
	bn_div_imp(c, d, a, b);
}

void bn_div_dig(bn_t c, const bn_t a, dig_t b) {
	bn_t q;
	dig_t r;

	bn_null(q);

	if (b == 0) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}

	if (b == 1 || bn_is_zero(a) == 1) {
		if (c != NULL) {
			bn_copy(c, a);
		}
		return;
	}

	RLC_TRY {
		bn_new_size(q, a->used);
		bn_div1_low(q->dp, &r, (const dig_t *)a->dp, a->used, b);

		if (c != NULL) {
			q->used = a->used;
			q->sign = a->sign;
			bn_trim(q);
			bn_copy(c, q);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(q);
	}
}

void bn_div_rem_dig(bn_t c, dig_t *d, const bn_t a, dig_t b) {
	bn_t q;
	dig_t r;

	bn_null(q);

	if (b == 0) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}

	if (b == 1 || bn_is_zero(a) == 1) {
		if (d != NULL) {
			*d = 0;
		}
		if (c != NULL) {
			bn_copy(c, a);
		}
		return;
	}

	RLC_TRY {
		bn_new_size(q, a->used);
		bn_div1_low(q->dp, &r, (const dig_t *)a->dp, a->used, b);

		if (c != NULL) {
			q->used = a->used;
			q->sign = a->sign;
			bn_trim(q);
			bn_copy(c, q);
		}

		if (d != NULL) {
			*d = r;
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(q);
	}
}
