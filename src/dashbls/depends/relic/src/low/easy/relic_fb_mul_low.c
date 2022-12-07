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
 * Implementation of the low-level binary field bit shifting functions.
 *
 * @ingroup fb
 */

#include <stdlib.h>

#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_bn_low.h"
#include "relic_util.h"
#include "relic_alloc.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_mul1_low(dig_t *c, const dig_t *a, dig_t digit) {
	int j, k;
	dig_t b1, b2;

	if (digit == 0) {
		dv_zero(c, RLC_FB_DIGS + 1);
		return;
	}
	if (digit == 1) {
		dv_copy(c, a, RLC_FB_DIGS);
		c[RLC_FB_DIGS] = 0;
		return;
	}

	c[RLC_FB_DIGS] = fb_lshb_low(c, a, util_bits_dig(digit) - 1);
	for (int i = util_bits_dig(digit) - 2; i > 0; i--) {
		if (digit & ((dig_t)1 << i)) {
			j = RLC_DIG - i;
			b1 = a[0];
			c[0] ^= (b1 << i);
			for (k = 1; k < RLC_FB_DIGS; k++) {
				b2 = a[k];
				c[k] ^= ((b2 << i) | (b1 >> j));
				b1 = b2;
			}
			c[RLC_FB_DIGS] ^= (b1 >> j);
		}
	}
	if (digit & (dig_t)1) {
		for (int i = 0; i < RLC_FB_DIGS; i++) {
			c[i] ^= a[i];
		}
	}
}

void fb_muln_low(dig_t *c, const dig_t *a, const dig_t *b) {
	rlc_align dig_t t[16][RLC_FB_DIGS + 1];
	dig_t r0, r1, r2, r4, r8, u, carry, *tmpc;
	const dig_t *tmpa;
	int i, j;

	for (i = 0; i < 2 * RLC_FB_DIGS; i++) {
		c[i] = 0;
	}

	for (i = 0; i < 16; i++) {
		dv_zero(t[i], RLC_FB_DIGS + 1);
	}

	u = 0;
	for (i = 0; i < RLC_FB_DIGS; i++) {
		r1 = r0 = b[i];
		r2 = (r0 << 1) | (u >> (RLC_DIG - 1));
		r4 = (r0 << 2) | (u >> (RLC_DIG - 2));
		r8 = (r0 << 3) | (u >> (RLC_DIG - 3));
		t[0][i] = 0;
		t[1][i] = r1;
		t[2][i] = r2;
		t[3][i] = r1 ^ r2;
		t[4][i] = r4;
		t[5][i] = r1 ^ r4;
		t[6][i] = r2 ^ r4;
		t[7][i] = r1 ^ r2 ^ r4;
		t[8][i] = r8;
		t[9][i] = r1 ^ r8;
		t[10][i] = r2 ^ r8;
		t[11][i] = r1 ^ r2 ^ r8;
		t[12][i] = r4 ^ r8;
		t[13][i] = r1 ^ r4 ^ r8;
		t[14][i] = r2 ^ r4 ^ r8;
		t[15][i] = r1 ^ r2 ^ r4 ^ r8;
		u = r1;
	}

	if (u > 0) {
		r2 = u >> (RLC_DIG - 1);
		r4 = u >> (RLC_DIG - 2);
		r8 = u >> (RLC_DIG - 3);
		t[0][RLC_FB_DIGS] = t[1][RLC_FB_DIGS] = 0;
		t[2][RLC_FB_DIGS] = t[3][RLC_FB_DIGS] = r2;
		t[4][RLC_FB_DIGS] = t[5][RLC_FB_DIGS] = r4;
		t[6][RLC_FB_DIGS] = t[7][RLC_FB_DIGS] = r2 ^ r4;
		t[8][RLC_FB_DIGS] = t[9][RLC_FB_DIGS] = r8;
		t[10][RLC_FB_DIGS] = t[11][RLC_FB_DIGS] = r2 ^ r8;
		t[12][RLC_FB_DIGS] = t[13][RLC_FB_DIGS] = r4 ^ r8;
		t[14][RLC_FB_DIGS] = t[15][RLC_FB_DIGS] = r2 ^ r4 ^ r8;
	}

	for (i = RLC_DIG - 4; i > 0; i -= 4) {
		tmpa = a;
		tmpc = c;
		for (j = 0; j < RLC_FB_DIGS; j++, tmpa++, tmpc++) {
			u = (*tmpa >> i) & 0x0F;
			fb_addn_low(tmpc, tmpc, t[u]);
			*(tmpc + RLC_FB_DIGS) ^= t[u][RLC_FB_DIGS];
		}
		carry = fb_lshb_low(c, c, 4);
		fb_lshb_low(c + RLC_FB_DIGS, c + RLC_FB_DIGS, 4);
		c[RLC_FB_DIGS] ^= carry;
	}

	for (j = 0; j < RLC_FB_DIGS; j++, a++, c++) {
		u = *a & 0x0F;
		fb_addn_low(c, c, t[u]);
		*(c + RLC_FB_DIGS) ^= t[u][RLC_FB_DIGS];
	}
}

void fb_muld_low(dig_t *c, const dig_t *a, const dig_t *b, int size) {
    dig_t *tt = RLC_ALLOCA(dig_t, 16 * (size + 1));
	dig_t *t[16];
	dig_t u, r0, r1, r2, r4, r8, *tmpc;
	const dig_t *tmpa;
	int i, j;

    for(i = 0; i < 16; i++) {
        t[i] = tt + i * (size + 1);
	}

	dv_zero(c, 2 * size);

	for (i = 0; i < 16; i++) {
		dv_zero(t[i], size + 1);
	}

	u = 0;
	for (i = 0; i < size; i++) {
		r1 = r0 = b[i];
		r2 = (r0 << 1) | (u >> (RLC_DIG - 1));
		r4 = (r0 << 2) | (u >> (RLC_DIG - 2));
		r8 = (r0 << 3) | (u >> (RLC_DIG - 3));
		t[0][i] = 0;
		t[1][i] = r1;
		t[2][i] = r2;
		t[3][i] = r1 ^ r2;
		t[4][i] = r4;
		t[5][i] = r1 ^ r4;
		t[6][i] = r2 ^ r4;
		t[7][i] = r1 ^ r2 ^ r4;
		t[8][i] = r8;
		t[9][i] = r1 ^ r8;
		t[10][i] = r2 ^ r8;
		t[11][i] = r1 ^ r2 ^ r8;
		t[12][i] = r4 ^ r8;
		t[13][i] = r1 ^ r4 ^ r8;
		t[14][i] = r2 ^ r4 ^ r8;
		t[15][i] = r1 ^ r2 ^ r4 ^ r8;
		u = r1;
	}

	if (u > 0) {
		r2 = u >> (RLC_DIG - 1);
		r4 = u >> (RLC_DIG - 2);
		r8 = u >> (RLC_DIG - 3);
		t[0][size] = t[1][size] = 0;
		t[2][size] = t[3][size] = r2;
		t[4][size] = t[5][size] = r4;
		t[6][size] = t[7][size] = r2 ^ r4;
		t[8][size] = t[9][size] = r8;
		t[10][size] = t[11][size] = r2 ^ r8;
		t[12][size] = t[13][size] = r4 ^ r8;
		t[14][size] = t[15][size] = r2 ^ r4 ^ r8;
	}

	for (i = RLC_DIG - 4; i > 0; i -= 4) {
		tmpa = a;
		tmpc = c;
		for (j = 0; j < size; j++, tmpa++, tmpc++) {
			u = (*tmpa >> i) & 0x0F;
			fb_addd_low(tmpc, tmpc, t[u], size + 1);
		}
		bn_lshb_low(c, c, 2 * size, 4);
	}

	for (j = 0; j < size; j++, a++, c++) {
		u = *a & 0x0F;
		fb_addd_low(c, c, t[u], size + 1);
	}

	RLC_FREE(tt);
}

void fb_mulm_low(dig_t *c, const dig_t *a, const dig_t *b) {
	rlc_align dig_t t[2 * RLC_FB_DIGS];

	fb_muln_low(t, a, b);
	fb_rdc(c, t);
}
