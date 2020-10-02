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
 * Implementation of the low-level binary field square root.
 *
 * @ingroup fb
 */

#include <stdlib.h>

#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

static const dig_t table_odds[16] = {
	0, 4, 1, 5, 8, 12, 9, 13, 2, 6, 3, 7, 10, 14, 11, 15
};

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_slvn_low(dig_t *c, const dig_t *a) {
	int i, j, k, b, d, v[FB_BITS];
	dig_t u, *p;
	relic_align dig_t s[FB_DIGS], t[FB_DIGS];
	dig_t mask;
	const void *tab = fb_poly_get_slv();

	dv_zero(s, FB_DIGS);
	dv_copy(t, a, FB_DIGS);

	for (i = (FB_BITS - 1)/2; i > 0; i--) {
		if (fb_get_bit(t, i + i)) {
			SPLIT(b, d, i, FB_DIG_LOG);
			t[d] ^= ((dig_t)1 << b);
			s[d] ^= ((dig_t)1 << b);
		}
	}

	k = 0;
	SPLIT(b, d, FB_BITS, FB_DIG_LOG);
	for (i = 0; i < d; i++) {
		u = t[i];
		for (j = 0; j < FB_DIGIT / 8; j++) {
			v[k++] = table_odds[((u & 0x0A) + ((u & 0xA0) >> 5))];
			u >>= 8;
		}
	}
	mask = (b == FB_DIGIT ? DMASK : MASK(b));
	u = t[d] & mask;
	/* We ignore the first even bit if it is present. */
	for (j = 1; j < b; j += 8) {
		v[k++] = table_odds[((u & 0x0A) + ((u & 0xA0) >> 5))];
		u >>= 8;
	}

	for (i = 0; i < k; i++) {
		p = (dig_t *)(tab + (16 * i + v[i]) * sizeof(fb_st));
		fb_add(s, s, p);
	}

	fb_copy(c, s);
}
