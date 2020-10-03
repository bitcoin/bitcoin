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

#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

static void fb_rdct_low(dig_t *c, dig_t *a, int fa) {
	int i, sh, lh, rh, sa, la, ra;
	dig_t d;

	SPLIT(rh, sh, FB_BITS, FB_DIG_LOG);
	sh++;
	lh = FB_DIGIT - rh;

	SPLIT(ra, sa, FB_BITS - fa, FB_DIG_LOG);
	sa++;
	la = FB_DIGIT - ra;

	for (i = 2 * FB_DIGS - 1; i >= sh; i--) {
		d = a[i];
		a[i] = 0;

		if (rh == 0) {
			a[i - sh + 1] ^= d;
		} else {
			a[i - sh + 1] ^= (d >> rh);
			a[i - sh] ^= (d << lh);
		}
		if (ra == 0) {
			a[i - sa + 1] ^= d;
		} else {
			a[i - sa + 1] ^= (d >> ra);
			a[i - sa] ^= (d << la);
		}
	}

	if (FB_BITS % FB_DIGIT == 0) {
		while (a[FB_DIGS] != 0) {

			d = a[sh - 1] >> rh;

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
			a[sh - 1] ^= d;
		}
	} else {
		d = a[sh - 1] >> rh;
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
		a[sh - 1] ^= d;
	}
	fb_copy(c, a);
}

static void fb_rdcp_low(dig_t *c, dig_t *a, int fa, int fb, int fc) {
	int i, sh, lh, rh, sa, la, ra, sb, lb, rb, sc, lc, rc;
	dig_t d;

	SPLIT(rh, sh, FB_BITS, FB_DIG_LOG);
	sh++;
	lh = FB_DIGIT - rh;

	SPLIT(ra, sa, FB_BITS - fa, FB_DIG_LOG);
	sa++;
	la = FB_DIGIT - ra;

	SPLIT(rb, sb, FB_BITS - fb, FB_DIG_LOG);
	sb++;
	lb = FB_DIGIT - rb;

	SPLIT(rc, sc, FB_BITS - fc, FB_DIG_LOG);
	sc++;
	lc = FB_DIGIT - rc;

	for (i = 2 * FB_DIGS - 1; i >= sh; i--) {
		d = a[i];
		a[i] = 0;

		if (rh == 0) {
			a[i - sh + 1] ^= d;
		} else {
			a[i - sh + 1] ^= (d >> rh);
			a[i - sh] ^= (d << lh);
		}
		if (ra == 0) {
			a[i - sa + 1] ^= d;
		} else {
			a[i - sa + 1] ^= (d >> ra);
			a[i - sa] ^= (d << la);
		}
		if (rb == 0) {
			a[i - sb + 1] ^= d;
		} else {
			a[i - sb + 1] ^= (d >> rb);
			a[i - sb] ^= (d << lb);
		}
		if (rc == 0) {
			a[i - sc + 1] ^= d;
		} else {
			a[i - sc + 1] ^= (d >> rc);
			a[i - sc] ^= (d << lc);
		}
	}

	if (FB_BITS % FB_DIGIT == 0) {
		while (a[FB_DIGS] != 0) {
			d = a[sh - 1] >> rh;

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
			a[sh - 1] ^= d;
		}
	} else {
		d = a[sh - 1] >> rh;

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
		a[sh - 1] ^= d;
	}

	fb_copy(c, a);
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_rdcn_low(dig_t *c, dig_t *a) {
	int fa, fb, fc;

	fb_poly_get_rdc(&fa, &fb, &fc);

	if (fb == 0) {
		fb_rdct_low(c, a, fa);
	} else {
		fb_rdcp_low(c, a, fa, fb, fc);
	}
}

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

	if (fb != 0) {
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

	if (fb != 0) {
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
		if (fb != 0) {
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
