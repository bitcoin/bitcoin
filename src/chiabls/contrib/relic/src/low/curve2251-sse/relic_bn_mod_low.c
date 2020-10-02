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
 * Implementation of the low-level multiple precision integer modular reduction
 * functions.
 *
 * @ingroup bn
 */

#include <gmp.h>
#include <string.h>

#include "relic_bn.h"
#include "relic_bn_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void bn_modn_low(dig_t *c, const dig_t *a, int sa, const dig_t *m, int sm, dig_t u) {
	int i;
	dig_t r, carry, *tmpc;

	tmpc = c;

	for (i = 0; i < sa; i++, tmpc++, a++) {
		*tmpc = *a;
	}

	tmpc = c;

	for (i = 0; i < sm; i++, tmpc++) {
		r = (dig_t)(*tmpc * u);
		carry = mpn_addmul_1(tmpc, m, sm, r);
		mpn_add_1(tmpc + sm, tmpc + sm,  sm - i + 1, carry);
	}
	bn_rshd_low(c, c, 2 * sm + 1, sm);
}
