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
 * Implementation of the low-level modular reduction functions.
 *
 * @ingroup fb
 */

#include <stdlib.h>

#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_util.h"
#include "macros.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_rdcn_low(dig_t *c, dig_t *a) {
	dig_t d, a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4];
	const int ra = 52;
	const int rb = 55;
	const int rc = 57;
	const int rh = 59;
	const int lh = 5;
	const int la = 12;
	const int lb = 9;
	const int lc = 7;

	d = a[7];
	a4 ^= (d >> rh);
	a4 ^= (d >> ra);
	a4 ^= (d >> rb);
	a4 ^= (d >> rc);

	a3 ^= (d << lh);
	a3 ^= (d << la);
	a3 ^= (d << lb);
	a3 ^= (d << lc);

	d = a[6];
	a3 ^= (d >> rh);
	a3 ^= (d >> ra);
	a3 ^= (d >> rb);
	a3 ^= (d >> rc);

	a2 ^= (d << lh);
	a2 ^= (d << la);
	a2 ^= (d << lb);
	a2 ^= (d << lc);

	d = a[5];
	a2 ^= (d >> rh);
	a2 ^= (d >> ra);
	a2 ^= (d >> rb);
	a2 ^= (d >> rc);

	a1 ^= (d << lh);
	a1 ^= (d << la);
	a1 ^= (d << lb);
	a1 ^= (d << lc);

	d = a4;
	a1 ^= (d >> rh);
	a1 ^= (d >> ra);
	a1 ^= (d >> rb);
	a1 ^= (d >> rc);

	a0 ^= (d << lh);
	a0 ^= (d << la);
	a0 ^= (d << lb);
	a0 ^= (d << lc);

	d = a3 >> rh;
	a0 ^= d;
	d <<= rh;

	a0 ^= (d >> ra);
	a0 ^= (d >> rb);
	a0 ^= (d >> rc);
	a3 ^= d;

	c[3] = a3;
	c[2] = a2;
	c[1] = a1;
	c[0] = a0;
}

void fb_rdc1_low(dig_t *c, dig_t *a) {
	dig_t d;
	const int fa = 7;
	const int fb = 4;
	const int fc = 2;

	const int rh = FB_BITS % FB_DIGIT;
	const int sh = FB_BITS / FB_DIGIT + 1;
	const int lh = FB_DIGIT - rh;;
	const int ra = (FB_BITS - fa) % FB_DIGIT;
	const int sa = (FB_BITS - fa) / FB_DIGIT + 1;
	const int la = FB_DIGIT - ra;
	const int rb = (FB_BITS - fb) % FB_DIGIT;
	const int sb = (FB_BITS - fb) / FB_DIGIT + 1;
	const int lb = FB_DIGIT - rb;
	const int rc = (FB_BITS - fc) % FB_DIGIT;
	const int sc = (FB_BITS - fc) / FB_DIGIT + 1;
	const int lc = FB_DIGIT - rc;

	d = a[FB_DIGS];

	a[FB_DIGS - sh] ^= (d << lh);
	a[FB_DIGS - sa + 1] ^= (d >> ra);
	a[FB_DIGS - sa] ^= (d << la);

	a[FB_DIGS - sb + 1] ^= (d >> rb);
	a[FB_DIGS - sb] ^= (d << lb);
	a[FB_DIGS - sc + 1] ^= (d >> rc);
	a[FB_DIGS - sc] ^= (d << lc);

	d = a[sh - 1] >> rh;

	dig_t a0 = a[0];
	a0 ^= d;
	d <<= rh;

	a0 ^= (d >> ra);
	a0 ^= (d >> rb);
	a0 ^= (d >> rc);
	a[3] ^= d;

	c[0] = a0;
	c[1] = a[1];
	c[2] = a[2];
	c[3] = a[3];
}
