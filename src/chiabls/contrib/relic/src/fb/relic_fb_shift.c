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
 * Implementation of the binary field multiplication by the basis.
 *
 * @ingroup fb
 */

#include "relic_core.h"
#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_lsh(fb_t c, const fb_t a, int bits) {
	int digits;

	SPLIT(bits, digits, bits, FB_DIG_LOG);

	if (digits) {
		fb_lshd_low(c, a, digits);
	} else {
		if (c != a) {
			fb_copy(c, a);
		}
	}

	switch (bits) {
		case 0:
			break;
		case 1:
			fb_lsh1_low(c, c);
			break;
		default:
			fb_lshb_low(c, c, bits);
			break;
	}
}

void fb_rsh(fb_t c, const fb_t a, int bits) {
	int digits;

	SPLIT(bits, digits, bits, FB_DIG_LOG);

	if (digits) {
		fb_rshd_low(c, a, digits);
	} else {
		if (c != a) {
			fb_copy(c, a);
		}
	}

	switch (bits) {
		case 0:
			break;
		case 1:
			fb_rsh1_low(c, c);
			break;
		default:
			fb_rshb_low(c, c, bits);
			break;
	}
}
