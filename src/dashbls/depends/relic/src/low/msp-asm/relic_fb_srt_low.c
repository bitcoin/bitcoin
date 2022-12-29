/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2012 RELIC Authors
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

#if FB_POLYN != 271 && FB_POLYN != 353

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#define HALF ((int)((RLC_FB_BITS / 2)/(RLC_DIG) + ((RLC_FB_BITS / 2) % RLC_DIG > 0)))

static const dig_t table_evens[16] = {
	0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15
};
static const dig_t table_odds[16] = {
	0, 4, 1, 5, 8, 12, 9, 13, 2, 6, 3, 7, 10, 14, 11, 15
};

static void fb_srtt_low(dig_t *c, const dig_t *a, int fa) {
	int i, j, n, h, sh, rh, lh, sa, la, ra;
	dig_t d, d_e, d_o;
	align dig_t t[2 * RLC_FB_DIGS] = { 0 };

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
		for (j = 0; j < RLC_DIG / 8; j++) {
			d_e |= table_evens[((d & 0x05) + ((d & 0x50) >> 3))] << (j << 2);
			d_o |= table_odds[((d & 0x0A) + ((d & 0xA0) >> 5))] << (j << 2);
			d >>= 8;
		}

		i++;
		if (i < sh) {
			d = a[i];

			for (j = 0; j < RLC_DIG / 8; j++) {
				d_e |= table_evens[((d & 0x05) + ((d & 0x50) >> 3))] <<
						((RLC_DIG / 2) + (j << 2));
				d_o |= table_odds[((d & 0x0A) + ((d & 0xA0) >> 5))] <<
						((RLC_DIG / 2) + (j << 2));
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
	align dig_t t[] = { 0 };

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
		for (j = 0; j < RLC_DIG / 8; j++) {
			d_e |= table_evens[((d & 0x5) + ((d & 0x50) >> 3))] << (j << 2);
			d_o |= table_odds[((d & 0xA) + ((d & 0xA0) >> 5))] << (j << 2);
			d >>= 8;
		}
		i++;

		if (i < sh) {
			d = a[i];
			for (j = 0; j < RLC_DIG / 8; j++) {
				d_e |= table_evens[((d & 0x5) + ((d & 0x50) >> 3))] <<
						(RLC_DIG / 2 + (j << 2));
				d_o |= table_odds[((d & 0xA) + ((d & 0xA0) >> 5))] <<
						(RLC_DIG / 2 + (j << 2));
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
	align dig_t t[2 * RLC_FB_DIGS] = { 0 }, s[RLC_FB_DIGS + 1] = { 0 };
	align dig_t t_e[RLC_FB_DIGS] = { 0 }, t_o[RLC_FB_DIGS] = { 0 };

	sh = 1 + (RLC_FB_BITS >> RLC_DIG_LOG);

	for (i = 0; i < RLC_FB_DIGS; i++) {
		n = i >> 1;
		d = a[i];

		d_e = d_o = 0;
		for (j = 0; j < RLC_DIG / 8; j++) {
			d_e |= table_evens[((d & 0x05) + ((d & 0x50) >> 3))] << (j << 2);
			d_o |= table_odds[((d & 0x0A) + ((d & 0xA0) >> 5))] << (j << 2);
			d >>= 8;
		}

		i++;
		if (i < sh && i < RLC_FB_DIGS) {
			d = a[i];

			for (j = 0; j < RLC_DIG / 8; j++) {
				d_e |= table_evens[((d & 0x05) + ((d & 0x50) >> 3))] <<
						((RLC_DIG / 2) + (j << 2));
				d_o |= table_odds[((d & 0x0A) + ((d & 0xA0) >> 5))] <<
						((RLC_DIG / 2) + (j << 2));
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

#else

const uint8_t fb_srt_table_evens[256] = { 0, 1, 16, 17, 2, 3, 18, 19, 32, 33,
	48, 49, 34, 35, 50, 51, 4, 5, 20, 21, 6, 7, 22, 23, 36, 37, 52, 53,	38, 39,
	54, 55, 64, 65, 80, 81, 66, 67, 82, 83, 96, 97, 112, 113, 98, 99, 114, 115,
	68, 69, 84, 85, 70, 71, 86, 87, 100, 101, 116, 117, 102, 103, 118, 119, 8,
	9, 24, 25, 10, 11, 26, 27, 40, 41, 56, 57, 42, 43, 58, 59, 12, 13, 28, 29,
	14, 15, 30, 31, 44, 45, 60, 61, 46, 47, 62, 63, 72, 73, 88, 89, 74, 75, 90,
	91, 104, 105, 120, 121, 106, 107, 122, 123,	76, 77, 92, 93, 78, 79, 94, 95,
	108, 109, 124, 125, 110, 111, 126, 127, 128, 129, 144, 145, 130, 131, 146,
	147, 160, 161, 176, 177, 162, 163, 178, 179, 132, 133, 148, 149, 134, 135,
	150, 151, 164, 165, 180, 181, 166, 167, 182, 183, 192, 193, 208, 209, 194,
	195, 210, 211, 224, 225, 240, 241, 226, 227, 242, 243, 196, 197, 212, 213,
	198, 199, 214, 215, 228, 229, 244, 245, 230, 231, 246, 247, 136, 137, 152,
	153, 138, 139, 154, 155, 168, 169, 184, 185, 170, 171, 186, 187, 140, 141,
	156, 157, 142, 143, 158, 159, 172, 173, 188, 189, 174, 175, 190, 191, 200,
	201, 216, 217, 202, 203, 218, 219, 232, 233, 248, 249, 234, 235, 250, 251,
	204, 205, 220, 221, 206, 207, 222, 223, 236, 237, 252, 253, 238, 239, 254,
	255 };

const uint8_t fb_srt_table_odds[256] = { 0, 16, 1, 17, 32, 48, 33, 49, 2, 18, 3,
	19, 34, 50, 35, 51, 64, 80, 65, 81, 96, 112, 97, 113, 66, 82, 67, 83, 98,
	114, 99, 115, 4, 20, 5, 21, 36, 52, 37, 53, 6, 22, 7, 23, 38, 54, 39, 55,
	68, 84, 69, 85, 100, 116, 101, 117, 70, 86, 71, 87,	102, 118, 103, 119, 128,
	144, 129, 145, 160, 176, 161, 177, 130, 146, 131, 147, 162, 178, 163, 179,
	192, 208, 193, 209, 224, 240, 225, 241,	194, 210, 195, 211, 226, 242, 227,
	243, 132, 148, 133, 149, 164, 180,	165, 181, 134, 150, 135, 151, 166, 182,
	167, 183, 196, 212, 197, 213, 228, 244, 229, 245, 198, 214, 199, 215, 230,
	246, 231, 247, 8, 24, 9, 25, 40, 56, 41, 57, 10, 26, 11, 27, 42, 58, 43, 59,
	72, 88, 73, 89,	104, 120, 105, 121, 74, 90, 75, 91, 106, 122, 107, 123, 12,
	28, 13, 29, 44, 60, 45, 61, 14, 30, 15, 31, 46, 62, 47, 63, 76, 92, 77, 93,
	108, 124, 109, 125, 78, 94, 79, 95, 110, 126, 111, 127, 136, 152, 137, 153,
	168, 184, 169, 185, 138, 154, 139, 155, 170, 186, 171, 187, 200, 216, 201,
	217, 232, 248, 233, 249, 202, 218, 203, 219, 234, 250, 235, 251, 140, 156,
	141, 157, 172, 188, 173, 189, 142, 158, 143, 159, 174, 190, 175, 191, 204,
	220, 205, 221, 236, 252, 237, 253, 206, 222, 207, 223, 238, 254, 239, 255 };

#endif
