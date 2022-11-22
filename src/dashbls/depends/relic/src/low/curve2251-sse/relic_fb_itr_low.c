/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2012 RELIC Authors
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
 * Implementation of the low-level iterated squaring/square-root.
 *
 * @ingroup fb
 */

#include <tmmintrin.h>
#include <xmmintrin.h>

#include "relic_fb.h"
#include "relic_dv.h"
#include "relic_fb_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_itrn_low(dig_t *c, const dig_t *a, dig_t *t) {
	int i, j;
	dig_t u, *p;
	const dig_t *tmp;

	__m128i r0, r1;
	r0 = r1 = _mm_setzero_si128();
	for (i = RLC_DIG - 4; i >= 0; i -= 4) {
		tmp = a;
		for (j = 0; j < RLC_FB_DIGS - 1; j++, tmp++) {
			u = (*tmp >> i) & 0x0F;
			p = (t + ((j * RLC_DIG + i) * 4 + u) * RLC_FB_DIGS);
			r0 = _mm_xor_si128(r0, *(__m128i *)(p));
			r1 = _mm_xor_si128(r1, *(__m128i *)(p + 2));
		}
	}
	for (i = RLC_DIG - 8; i >= 0; i -= 4) {
		tmp = a + RLC_FB_DIGS - 1;
		u = (*tmp >> i) & 0x0F;
		p = (t + ((j * RLC_DIG + i) * 4 + u) * RLC_FB_DIGS);
		r0 = _mm_xor_si128(r0, *(__m128i *)(p));
		r1 = _mm_xor_si128(r1, *(__m128i *)(p + 2));
	}

	_mm_store_si128((__m128i *)c, r0);
	_mm_store_si128((__m128i *)(c + 2), r1);
}
