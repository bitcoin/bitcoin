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
 * Implementation of the multiple precision integer arithmetic multiplication
 * functions.
 *
 * @ingroup bn
 */

#include <gmp.h>

#include "relic_bn.h"
#include "relic_bn_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void bn_sqra_low(dig_t *c, const dig_t *a, int size) {
	dig_t carry, digit = *a;

	carry = bn_mula_low(c, a, digit, size);
	bn_add1_low(c + size, c + size, carry, size);
	if (size > 1) {
		carry = bn_mula_low(c + 1, a + 1, digit, size - 1);
		bn_add1_low(c + size, c + size, carry, size);
	}
}

void bn_sqrn_low(dig_t *c, const dig_t *a, int size) {
	dig_t scratch[mpn_sec_sqr_itch(size)];
	mpn_sec_sqr(c, a, size, scratch);
}
