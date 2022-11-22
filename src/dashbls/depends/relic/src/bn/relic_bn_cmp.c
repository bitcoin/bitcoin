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
 * Implementation of the multiple precision comparison functions.
 *
 * @ingroup bn
 */

#include "relic_core.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int bn_cmp_abs(const bn_t a, const bn_t b) {
	if (bn_is_zero(a) && bn_is_zero(b)) {
		return RLC_EQ;
	}

	if (a->used > b->used) {
		return RLC_GT;
	}

	if (a->used < b->used) {
		return RLC_LT;
	}

	return dv_cmp(a->dp, b->dp, a->used);
}

int bn_cmp_dig(const bn_t a, dig_t b) {
	if (a->sign == RLC_NEG) {
		return RLC_LT;
	}

	if (a->used > 1) {
		return RLC_GT;
	}

	if (a->dp[0] > b) {
		return RLC_GT;
	}

	if (a->dp[0] < b) {
		return RLC_LT;
	}

	return RLC_EQ;
}

int bn_cmp(const bn_t a, const bn_t b) {
	if (bn_is_zero(a) && bn_is_zero(b)) {
		return RLC_EQ;
	}

	if (a->sign == RLC_POS && b->sign == RLC_NEG) {
		return RLC_GT;
	}
	if (a->sign == RLC_NEG && b->sign == RLC_POS) {
		return RLC_LT;
	}

	if (a->sign == RLC_NEG) {
		return bn_cmp_abs(b, a);
	}

	return bn_cmp_abs(a, b);
}
