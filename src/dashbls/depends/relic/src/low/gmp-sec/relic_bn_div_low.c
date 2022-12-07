/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2015 RELIC Authors
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
 * Implementation of the low-level multiple precision division functions.
 *
 * @ingroup bn
 */

#include <gmp.h>

#include "relic_bn.h"
#include "relic_bn_low.h"
#include "relic_alloc.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void bn_divn_low(dig_t *c, dig_t *d, dig_t *a, int sa, dig_t *b, int sb) {
	dig_t u[sa], *t = RLC_ALLOCA(dig_t, mpn_sec_div_qr_itch(sa, sb));

	mpn_copyd(u, a, sa);
	c[sa - sb] = mpn_sec_div_qr(c, u, sa, b, sb, t);
	mpn_copyd(d, u, sa);
	RLC_FREE(t);
}

void bn_div1_low(dig_t *c, dig_t *d, const dig_t *a, int size, dig_t b) {
	dig_t u[size], *t = RLC_ALLOCA(dig_t, mpn_sec_div_qr_itch(size, 1));

	mpn_copyd(u, a, size);
	c[size - 1] = mpn_sec_div_qr(c, u, size, &b, 1, t);
	*d = u[0];
}
