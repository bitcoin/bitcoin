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

#define HALF ((int)((RLC_FB_BITS / 2)/(RLC_DIG) + ((RLC_FB_BITS / 2) % RLC_DIG > 0)))

static const dig_t t0[16] = {
	0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15
};
static const dig_t t1[16] = {
	0, 4, 1, 5, 8, 12, 9, 13, 2, 6, 3, 7, 10, 14, 11, 15
};

static void fb_srtt_low(dig_t *c, const dig_t *a, int fa) {
	int i, j, n, h, sh, rh, lh, sa, la, ra;
	dig_t d, d_e, d_o;
	rlc_align dig_t t[2 * RLC_FB_DIGS] = { 0 };

	sh = 1 + (RLC_FB_BITS >> RLC_DIG_LOG);
	h = (sh + 1) >> 1;
	rh = (h << RLC_DIG_LOG) - 1 - (RLC_FB_BITS - 1) / 2;
	lh = RLC_DIG - rh;

	RLC_RIP(la, sa, (fa + 1) >> 1);
	ra = RLC_DIG - la;

	for (i = 0; i < RLC_FB_DIGS; i++) {
		n = i >> 1;
		d = a[i];

		d_e = d_o = 0;
		for (j = 0; j < RLC_DIG / 2; j += 4) {
			d_e |= t0[((d & 0x05) + ((d & 0x50) >> 3))] << j;
			d_o |= t1[((d & 0x0A) + ((d & 0xA0) >> 5))] << j;
			d >>= 8;
		}

		i++;
		if (i < sh) {
			d = a[i];

			for (j = RLC_DIG / 2; j < RLC_DIG; j += 4) {
				d_e |= t0[((d & 0x05) + ((d & 0x50) >> 3))] << j;
				d_o |= t1[((d & 0x0A) + ((d & 0xA0) >> 5))] << j;
				d >>= 8;
			}
		}

		t[n] ^= d_e;

		if (rh == 0) {
			t[h + n] = d_o;
		} else {
			t[h + n - 1] ^= (d_o << lh);
			t[h + n] ^= (d_o >> rh);
		}
		if (la == 0) {
			t[n + sa] ^= d_o;
		} else {
			t[n + sa] ^= (d_o << la);
			t[n + sa + 1] ^= (d_o >> ra);
		}
	}
	fb_copy(c, t);
}

static void fb_srtp_low(dig_t *c, const dig_t *a, int fa, int fb, int fc) {
	int i, j, n, h, sh, rh, lh, sa, la, ra, sb, lb, rb, sc, lc, rc;
	dig_t d, d_e, d_o;
	rlc_align dig_t t[RLC_FB_DIGS] = { 0 };

	sh = 1 + (RLC_FB_BITS >> RLC_DIG_LOG);
	h = (sh + 1) >> 1;
	rh = (h << RLC_DIG_LOG) - 1 - (RLC_FB_BITS - 1) / 2;
	lh = RLC_DIG - rh;

	RLC_RIP(la, sa, (fa + 1) >> 1);
	ra = RLC_DIG - la;

	RLC_RIP(lb, sb, (fb + 1) >> 1);
	rb = RLC_DIG - lb;

	RLC_RIP(lc, sc, (fc + 1) >> 1);
	rc = RLC_DIG - lc;

	for (i = 0; i < sh; i++) {
		n = i >> 1;
		d = a[i];

		d_e = d_o = 0;
		for (j = 0; j < RLC_DIG / 2; j += 4) {
			d_e |= t0[((d & 0x5) + ((d & 0x50) >> 3))] << j;
			d_o |= t1[((d & 0xA) + ((d & 0xA0) >> 5))] << j;
			d >>= 8;
		}
		i++;

		if (i < sh) {
			d = a[i];
			for (j = RLC_DIG / 2; j < RLC_DIG; j += 4) {
				d_e |= t0[((d & 0x5) + ((d & 0x50) >> 3))] << j;
				d_o |= t1[((d & 0xA) + ((d & 0xA0) >> 5))] << j;
				d >>= 8;
			}
		}

		t[n] ^= d_e;

		if (rh == 0) {
			t[h + n] = d_o;
		} else {
			t[h + n - 1] ^= (d_o << lh);
			t[h + n] ^= (d_o >> rh);
		}
		if (la == 0) {
			t[n + sa] ^= d_o;
		} else {
			t[n + sa] ^= (d_o << la);
			t[n + sa + 1] ^= (d_o >> ra);
		}
		if (lb == 0) {
			t[n + sb] ^= d_o;
		} else {
			t[n + sb] ^= (d_o << lb);
			t[n + sb + 1] ^= (d_o >> rb);
		}
		if (lc == 0) {
			t[n + sc] ^= d_o;
		} else {
			t[n + sc] ^= (d_o << lc);
			t[n + sc + 1] ^= (d_o >> rc);
		}
	}
	fb_copy(c, t);
}

static void fb_sqrt_low(dig_t *c, const dig_t *a) {
	int i, j, n, sh;
	dig_t d, d_e, d_o;
	rlc_align dig_t t[2 * RLC_FB_DIGS] = { 0 }, s[RLC_FB_DIGS + 1] = { 0 };
	rlc_align dig_t t_e[RLC_FB_DIGS] = { 0 }, t_o[RLC_FB_DIGS] = { 0 };

	sh = 1 + (RLC_FB_BITS >> RLC_DIG_LOG);

	for (i = 0; i < RLC_FB_DIGS; i++) {
		n = i >> 1;
		d = a[i];

		d_e = d_o = 0;
		for (j = 0; j < RLC_DIG / 2; j += 4) {
			d_e |= t0[((d & 0x05) + ((d & 0x50) >> 3))] << j;
			d_o |= t1[((d & 0x0A) + ((d & 0xA0) >> 5))] << j;
			d >>= 8;
		}

		i++;
		if (i < sh && i < RLC_FB_DIGS) {
			d = a[i];

			for (j = RLC_DIG / 2; j < RLC_DIG; j += 4) {
				d_e |= t0[((d & 0x05) + ((d & 0x50) >> 3))] << j;
				d_o |= t1[((d & 0x0A) + ((d & 0xA0) >> 5))] << j;
				d >>= 8;
			}
		}

		t_e[n] = d_e;
		t_o[n] = d_o;
	}
	if (fb_poly_tab_srz(0) == NULL) {
		dv_copy(s, fb_poly_get_srz() + HALF, RLC_FB_DIGS - HALF);
		fb_muld_low(t + HALF, t_o, s, HALF);
		fb_muld_low(s, t_o, fb_poly_get_srz(), HALF);
		fb_addd_low(t, t, s, RLC_FB_DIGS + 1);
		fb_rdcn_low(c, t);
		fb_addd_low(c, c, t_e, HALF);
	} else {
		dig_t u, carry, *tmpa, *tmpc;

		for (i = RLC_DIG - 8; i > 0; i -= 8) {
			tmpa = t_o;
			tmpc = t;
			for (j = 0; j < HALF; j++, tmpa++, tmpc++) {
				u = (*tmpa >> i) & 0xFF;
				fb_addn_low(tmpc, tmpc, fb_poly_tab_srz(u));
			}
			carry = fb_lshb_low(t, t, 8);
			fb_lshb_low(t + RLC_FB_DIGS, t + RLC_FB_DIGS, 8);
			t[RLC_FB_DIGS] ^= carry;
		}
		for (j = 0; j < HALF; j++) {
			u = t_o[j] & 0xFF;
			fb_addn_low(t + j, t + j, fb_poly_tab_srz(u));
		}
		fb_zero(c);
		fb_rdcn_low(c, t);
		fb_addd_low(c, c, t_e, HALF);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_srtn_low(dig_t *c, const dig_t *a) {
	int fa, fb, fc;

	fb_poly_get_rdc(&fa, &fb, &fc);

	if (fb == 0) {
		if (fa % 2 == 0) {
			fb_sqrt_low(c, a);
		} else {
			fb_srtt_low(c, a, fa);
		}
	} else {
		if ((fa % 2 == 0) || (fb % 2 == 0) || (fc % 2 == 0)) {
			fb_sqrt_low(c, a);
		} else {
			fb_srtp_low(c, a, fa, fb, fc);
		}
	}
}
