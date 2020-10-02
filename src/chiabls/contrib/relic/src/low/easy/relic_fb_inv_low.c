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
 * Implementation of the low-level inversion functions.
 *
 * @ingroup fb
 */

#include <stdio.h>
#include "relic_fb.h"
#include "relic_fb_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_invn_low(dig_t *c, const dig_t *a) {
	int j, d, lu, lv, lt, l1, l2, bu, bv;
	relic_align dig_t _u[2 * FB_DIGS], _v[2 * FB_DIGS];
	relic_align dig_t _g1[2 * FB_DIGS], _g2[2 * FB_DIGS];
	dig_t *t = NULL, *u = NULL, *v = NULL, *g1 = NULL, *g2 = NULL, carry;

	dv_zero(_g1, FB_DIGS + 1);
	dv_zero(_g2, FB_DIGS + 1);

	u = _u;
	v = _v;
	g1 = _g1;
	g2 = _g2;

	/* u = a, v = f, g1 = 1, g2 = 0. */
	dv_copy(u, a, FB_DIGS);
	dv_copy(v, fb_poly_get(), FB_DIGS);
	g1[0] = 1;

	lu = lv = FB_DIGS;
	l1 = l2 = 1;

	bu = fb_bits(u);
	bv = FB_BITS + 1;
	j = bu - bv;

	/* While (u != 1). */
	while (1) {
		/* If j < 0 then swap(u, v), swap(g1, g2), j = -j. */
		if (j < 0) {
			t = u;
			u = v;
			v = t;

			lt = lu;
			lu = lv;
			lv = lt;

			t = g1;
			g1 = g2;
			g2 = t;

			lt = l1;
			l1 = l2;
			l2 = lt;

			j = -j;
		}

		SPLIT(j, d, j, FB_DIG_LOG);

		/* u = u + v * z^j. */
		if (j > 0) {
			carry = fb_lsha_low(u + d, v, j, lv);
			u[d + lv] ^= carry;
		} else {
			fb_addd_low(u + d, u + d, v, lv);
		}

		/* g1 = g1 + g2 * z^j. */
		if (j > 0) {
			carry = fb_lsha_low(g1 + d, g2, j, l2);
			l1 = (l2 + d >= l1 ? l2 + d : l1);
			if (carry) {
				g1[d + l2] ^= carry;
				l1 = (l2 + d >= l1 ? l1 + 1 : l1);
			}
		} else {
			fb_addd_low(g1 + d, g1 + d, g2, l2);
			l1 = (l2 + d > l1 ? l2 + d : l1);
		}

		while (u[lu - 1] == 0)
			lu--;
		while (v[lv - 1] == 0)
			lv--;

		if (lu == 1 && u[0] == 1)
			break;

		/* j = deg(u) - deg(v). */
		lt = util_bits_dig(u[lu - 1]) - util_bits_dig(v[lv - 1]);
		j = ((lu - lv) << FB_DIG_LOG) + lt;
	}
	/* Return g1. */
	fb_copy(c, g1);
}
