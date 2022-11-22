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

	RLC_RIP(bits, digits, bits);

	if (digits) {
		dv_lshd(c, a, RLC_FB_DIGS, digits);
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

	RLC_RIP(bits, digits, bits);

	if (digits) {
		dv_rshd(c, a, RLC_FB_DIGS, digits);
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
