/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007, 2008, 2009 RELIC Authors
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
 * Implementation of the low-level modular reduction functions.
 *
 * @ingroup fb
 */

#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_rdc1_low(dig_t *c, dig_t *a) {
	int fa, fb, fc;
	int sh, lh, rh, sa, la, ra, sb, lb, rb, sc, lc, rc;
	dig_t d;

	fb_poly_get_rdc(&fa, &fb, &fc);

	sh = lh = rh = sa = la = ra = sb = lb = rb = sc = lc = rc = 0;

	SPLIT(rh, sh, FB_BITS, FB_DIG_LOG);
	sh++;
	lh = FB_DIGIT - rh;

	SPLIT(ra, sa, FB_BITS - fa, FB_DIG_LOG);
	sa++;
	la = FB_DIGIT - ra;

	if (fb != -1) {
		SPLIT(rb, sb, FB_BITS - fb, FB_DIG_LOG);
		sb++;
		lb = FB_DIGIT - rb;

		SPLIT(rc, sc, FB_BITS - fc, FB_DIG_LOG);
		sc++;
		lc = FB_DIGIT - rc;
	}

	d = a[FB_DIGS];
	a[FB_DIGS] = 0;

	if (rh == 0) {
		a[FB_DIGS - sh + 1] ^= d;
	} else {
		a[FB_DIGS - sh + 1] ^= (d >> rh);
		a[FB_DIGS - sh] ^= (d << lh);
	}
	if (ra == 0) {
		a[FB_DIGS - sa + 1] ^= d;
	} else {
		a[FB_DIGS - sa + 1] ^= (d >> ra);
		a[FB_DIGS - sa] ^= (d << la);
	}

	if (fb != -1) {
		if (rb == 0) {
			a[FB_DIGS - sb + 1] ^= d;
		} else {
			a[FB_DIGS - sb + 1] ^= (d >> rb);
			a[FB_DIGS - sb] ^= (d << lb);
		}
		if (rc == 0) {
			a[FB_DIGS - sc + 1] ^= d;
		} else {
			a[FB_DIGS - sc + 1] ^= (d >> rc);
			a[FB_DIGS - sc] ^= (d << lc);
		}
	}

	d = a[sh - 1] >> rh;

	if (d != 0) {
		a[0] ^= d;
		d <<= rh;

		if (ra == 0) {
			a[sh - sa] ^= d;
		} else {
			a[sh - sa] ^= (d >> ra);
			if (sh > sa) {
				a[sh - sa - 1] ^= (d << la);
			}
		}
		if (fb != -1) {
			if (rb == 0) {
				a[sh - sb] ^= d;
			} else {
				a[sh - sb] ^= (d >> rb);
				if (sh > sb) {
					a[sh - sb - 1] ^= (d << lb);
				}
			}
			if (rc == 0) {
				a[sh - sc] ^= d;
			} else {
				a[sh - sc] ^= (d >> rc);
				if (sh > sc) {
					a[sh - sc - 1] ^= (d << lc);
				}
			}
		}
		a[sh - 1] ^= d;
	}

	fb_copy(c, a);
}
