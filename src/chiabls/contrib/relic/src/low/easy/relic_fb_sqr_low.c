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
 * Implementation of the low-level binary field shifting.
 *
 * @ingroup fb
 */

#include "relic_fb.h"
#include "relic_dv.h"
#include "relic_fb_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Precomputed table of the squaring of all polynomial with degree less than 4.
 */
static const dig_t table[16] = {
	0x00, 0x01, 0x04, 0x05, 0x10, 0x11, 0x14, 0x15,
	0x40, 0x41, 0x44, 0x45, 0x50, 0x51, 0x54, 0x55
};

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_sqrn_low(dig_t *c, const dig_t *a) {
	dig_t *tmp, x, y, z;

	tmp = c;
#if DIGIT == 8
	for (int i = 0; i < FB_DIGS; i++, tmp++) {
		x = a[i];
		y = x & 0x0F;
		z = x >> 4;
		y = (y | (y << 2)) & 0x33;
		y = (y | (y << 1)) & 0x55;
		*tmp++ = y;
		z = (z | (z << 2)) & 0x33;
		z = (z | (z << 1)) & 0x55;
		*tmp = z;
	}
#elif DIGIT == 16
	for (int i = 0; i < FB_DIGS; i++, tmp++) {
		x = a[i];
		y = x & 0x00FF;
		z = x >> 8;
		y = (y | (y << 4)) & 0x0F0F;
		y = (y | (y << 2)) & 0x3333;
		y = (y | (y << 1)) & 0x5555;
		*tmp++ = y;
		z = (z | (z << 4)) & 0x0F0F;
		z = (z | (z << 2)) & 0x3333;
		z = (z | (z << 1)) & 0x5555;
		*tmp = z;
	}
#elif DIGIT == 32
	for (int i = 0; i < FB_DIGS; i++, tmp++) {
		x = a[i];
		y = x & 0x0000FFFF;
		z = x >> 16;
		y = (y | (y << 8)) & 0x00FF00FF;
		y = (y | (y << 4)) & 0x0F0F0F0F;
		y = (y | (y << 2)) & 0x33333333;
		y = (y | (y << 1)) & 0x55555555;
		*tmp++ = y;
		z = (z | (z << 8)) & 0x00FF00FF;
		z = (z | (z << 4)) & 0x0F0F0F0F;
		z = (z | (z << 2)) & 0x33333333;
		z = (z | (z << 1)) & 0x55555555;
		*tmp = z;
	}
#elif DIGIT == 64
	for (int i = 0; i < FB_DIGS; i++, tmp++) {
		x = a[i];
		y = x & 0x00000000FFFFFFFF;
		z = x >> 32;
		y = (y | (y << 16)) & 0x0000FFFF0000FFFF;
		y = (y | (y << 8)) & 0x00FF00FF00FF00FF;
		y = (y | (y << 4)) & 0x0F0F0F0F0F0F0F0F;
		y = (y | (y << 2)) & 0x3333333333333333;
		y = (y | (y << 1)) & 0x5555555555555555;
		*tmp++ = y;
		z = (z | (z << 16)) & 0x0000FFFF0000FFFF;
		z = (z | (z << 8)) & 0x00FF00FF00FF00FF;
		z = (z | (z << 4)) & 0x0F0F0F0F0F0F0F0F;
		z = (z | (z << 2)) & 0x3333333333333333;
		z = (z | (z << 1)) & 0x5555555555555555;
		*tmp = z;
	}
#endif
}

void fb_sqrl_low(dig_t *c, const dig_t *a) {
	dig_t d, *tmpt;

	tmpt = c;
#if DIGIT == 8
	for (int i = 0; i < FB_DIGS; i++, tmpt++) {
		d = a[i];
		*tmpt = table[LOW(d)];
		tmpt++;
		*tmpt = table[HIGH(d)];
	}
#elif DIGIT == 16
	for (int i = 0; i < FB_DIGS; i++, tmpt++) {
		d = a[i];
		*tmpt = (table[d & 0xF]) | (table[(d >> 4) & 0xF] << 8);
		tmpt++;
		*tmpt = (table[(d >> 8) & 0xF] << 00) | (table[(d >> 12) & 0xF] << 8);
	}
#elif DIGIT == 32
	for (int i = 0; i < FB_DIGS; i++, tmpt++) {
		d = a[i];
		*tmpt = (table[d & 0xF]) | (table[(d >> 4) & 0xF] << 8) | (table[(d >> 8) & 0xF] << 16) | (table[(d >> 12) & 0xF] << 24);
		tmpt++;
		*tmpt = (table[(d >> 16) & 0xF] << 00) | (table[(d >> 20) & 0xF] << 8) | (table[(d >> 24) & 0xF] << 16) | (table[(d >> 28) & 0xF] << 24);
	}
#elif DIGIT == 64
	for (int i = 0; i < FB_DIGS; i++, tmpt++) {
		d = a[i];
		*tmpt = (table[d & 0xF] << 00) | (table[(d >> 4) & 0xF] << 8) |
				(table[(d >> 8) & 0xF] << 16) | (table[(d >> 12) & 0xF] << 24) |
				(table[(d >> 16) & 0xF] << 32) | (table[(d >> 20) & 0xF] << 40) | (table[(d >> 24) & 0xF] << 48) | (table[(d >> 28) & 0xF] << 56);
		tmpt++;
		*tmpt = (table[(d >> 32) & 0xF] << 00) | (table[(d >> 36) & 0xF] << 8) |
				(table[(d >> 40) & 0xF] << 16) | (table[(d >> 44) & 0xF] << 24) |
				(table[(d >> 48) & 0xF] << 32) | (table[(d >> 52) & 0xF] << 40) | (table[(d >> 56) & 0xF] << 48) | (table[(d >> 60) & 0xF] << 56);
	}
#endif
}

void fb_sqrm_low(dig_t *c, const dig_t *a) {
	relic_align dig_t t[2 * FB_DIGS];

	fb_sqrl_low(t, a);
	fb_rdc(c, t);
}
