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
 * Implementation of the low-level binary field bit shifting functions.
 *
 * @ingroup fb
 */

#include <stdlib.h>

#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_bn_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_mul1_low(dig_t *c, const dig_t *a, dig_t digit) {
	int j, k;
	dig_t b1, b2;

	if (digit == 0) {
		dv_zero(c, FB_DIGS + 1);
		return;
	}
	if (digit == 1) {
		dv_copy(c, a, FB_DIGS);
		return;
	}

	c[FB_DIGS] = fb_lshb_low(c, a, util_bits_dig(digit) - 1);
	for (int i = util_bits_dig(digit) - 2; i > 0; i--) {
		if (digit & ((dig_t)1 << i)) {
			j = FB_DIGIT - i;
			b1 = a[0];
			c[0] ^= (b1 << i);
			for (k = 1; k < FB_DIGS; k++) {
				b2 = a[k];
				c[k] ^= ((b2 << i) | (b1 >> j));
				b1 = b2;
			}
			c[FB_DIGS] ^= (b1 >> j);
		}
	}
	if (digit & (dig_t)1) {
		fb_addn_low(c, c, a);
	}
}

/*
void fb_muln_low(dig_t *c, dig_t *a, dig_t *b) {
	dv_t table[16];
	dig_t r0, r1, r2, r4, r8, u, carry, *tmpa, *tmpc;
	int i, j;

	for (i = 0; i < 16; i++) {
		dv_null(table[i]);
	}
	for (i = 0; i < 2 * FB_DIGS; i++) {
		c[i] = 0;
	}
	for (i = 0; i < 16; i++) {
		dv_new(table[i]);
		dv_zero(table[i], FB_DIGS + 1);
	}

	u = 0;
	for (i = 0; i < FB_DIGS; i++) {
		r1 = r0 = b[i];
		r2 = (r0 << 1) | (u >> (FB_DIGIT - 1));
		r4 = (r0 << 2) | (u >> (FB_DIGIT - 2));
		r8 = (r0 << 3) | (u >> (FB_DIGIT - 3));
		table[0][i] = 0;
		table[1][i] = r1;
		table[2][i] = r2;
		table[3][i] = r1 ^ r2;
		table[4][i] = r4;
		table[5][i] = r1 ^ r4;
		table[6][i] = r2 ^ r4;
		table[7][i] = r1 ^ r2 ^ r4;
		table[8][i] = r8;
		table[9][i] = r1 ^ r8;
		table[10][i] = r2 ^ r8;
		table[11][i] = r1 ^ r2 ^ r8;
		table[12][i] = r4 ^ r8;
		table[13][i] = r1 ^ r4 ^ r8;
		table[14][i] = r2 ^ r4 ^ r8;
		table[15][i] = r1 ^ r2 ^ r4 ^ r8;
		u = r1;
	}

	if (u > 0) {
		r2 = u >> (FB_DIGIT - 1);
		r4 = u >> (FB_DIGIT - 2);
		r8 = u >> (FB_DIGIT - 3);
		table[0][FB_DIGS] = table[1][FB_DIGS] = 0;
		table[2][FB_DIGS] = table[3][FB_DIGS] = r2;
		table[4][FB_DIGS] = table[5][FB_DIGS] = r4;
		table[6][FB_DIGS] = table[7][FB_DIGS] = r2 ^ r4;
		table[8][FB_DIGS] = table[9][FB_DIGS] = r8;
		table[10][FB_DIGS] = table[11][FB_DIGS] = r2 ^ r8;
		table[12][FB_DIGS] = table[13][FB_DIGS] = r4 ^ r8;
		table[14][FB_DIGS] = table[15][FB_DIGS] = r2 ^ r4 ^ r8;
	}

	for (i = FB_DIGIT - 4; i >= 4; i -= 4) {
		tmpa = a;
		tmpc = c;
		for (j = 0; j < FB_DIGS; j++, tmpa++, tmpc++) {
			u = (*tmpa >> i) & 0x0F;
			fb_addn_low(tmpc, tmpc, table[u]);
			*(tmpc + FB_DIGS) ^= table[u][FB_DIGS];
		}
		carry = fb_lshb_low(c, c, 4);
		fb_lshb_low(c + FB_DIGS, c + FB_DIGS, 4);
		c[FB_DIGS] ^= carry;
	}
	for (j = 0; j < FB_DIGS; j++, a++, c++) {
		u = *a & 0x0F;
		fb_addn_low(c, c, table[u]);
		*(c + FB_DIGS) ^= table[u][FB_DIGS];
	}

	for (i = 0; i < 16; i++) {
		dv_free(table[i]);
	}
}
*/
void fb_muld_low(dig_t *c, const dig_t *a, const dig_t *b, int size) {
	dv_t table[16];
	dig_t u, *tmpc, r0, r1, r2, r4, r8;
	const dig_t *tmpa;
	int i, j;

	for (i = 0; i < 16; i++) {
		dv_null(table[i]);
	}

	dv_zero(c, 2 * size);

	for (i = 0; i < 16; i++) {
		dv_new(table[i]);
		dv_zero(table[i], size + 1);
	}

	u = 0;
	for (i = 0; i < size; i++) {
		r1 = r0 = b[i];
		r2 = (r0 << 1) | (u >> (FB_DIGIT - 1));
		r4 = (r0 << 2) | (u >> (FB_DIGIT - 2));
		r8 = (r0 << 3) | (u >> (FB_DIGIT - 3));
		table[0][i] = 0;
		table[1][i] = r1;
		table[2][i] = r2;
		table[3][i] = r1 ^ r2;
		table[4][i] = r4;
		table[5][i] = r1 ^ r4;
		table[6][i] = r2 ^ r4;
		table[7][i] = r1 ^ r2 ^ r4;
		table[8][i] = r8;
		table[9][i] = r1 ^ r8;
		table[10][i] = r2 ^ r8;
		table[11][i] = r1 ^ r2 ^ r8;
		table[12][i] = r4 ^ r8;
		table[13][i] = r1 ^ r4 ^ r8;
		table[14][i] = r2 ^ r4 ^ r8;
		table[15][i] = r1 ^ r2 ^ r4 ^ r8;
		u = r1;
	}

	if (u > 0) {
		r2 = u >> (FB_DIGIT - 1);
		r4 = u >> (FB_DIGIT - 2);
		r8 = u >> (FB_DIGIT - 3);
		table[0][size] = table[1][size] = 0;
		table[2][size] = table[3][size] = r2;
		table[4][size] = table[5][size] = r4;
		table[6][size] = table[7][size] = r2 ^ r4;
		table[8][size] = table[9][size] = r8;
		table[10][size] = table[11][size] = r2 ^ r8;
		table[12][size] = table[13][size] = r4 ^ r8;
		table[14][size] = table[15][size] = r2 ^ r4 ^ r8;
	}

	for (i = FB_DIGIT - 4; i >= 4; i -= 4) {
		tmpa = a;
		tmpc = c;
		for (j = 0; j < size; j++, tmpa++, tmpc++) {
			u = (*tmpa >> i) & 0x0F;
			fb_addd_low(tmpc, tmpc, table[u], size + 1);
		}
		bn_lshb_low(c, c, 2 * size, 4);
	}
	for (j = 0; j < size; j++, a++, c++) {
		u = *a & 0x0F;
		fb_addd_low(c, c, table[u], size + 1);
	}
	for (i = 0; i < 16; i++) {
		dv_free(table[i]);
	}
}

void fb_mulm_low(dig_t *c, const dig_t *a, const dig_t *b) {
	dig_t relic_align t[2 * FB_DIGS];

	fb_muln_low(t, a, b);
	fb_rdc(c, t);
}
