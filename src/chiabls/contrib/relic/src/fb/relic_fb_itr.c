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
 * Implementation of the iterated squaring/square-root of a binary field
 * element.
 *
 * @ingroup fb
 */

#include <string.h>

#include "relic_core.h"
#include "relic_conf.h"
#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_bn_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_itr_basic(fb_t c, const fb_t a, int b) {
	fb_copy(c, a);
	if (b >= 0) {
		for (int i = 0; i < b; i++) {
			fb_sqr(c, c);
		}
	} else {
		for (int i = 0; i < -b; i++) {
			fb_srt(c, c);
		}
	}
}

void fb_itr_pre_quick(fb_t *t, int b) {
	int i, j, k;
	fb_t r;

	fb_null(r);

	TRY {
		fb_new(r);

		for (i = 0; i < FB_DIGS * FB_DIGIT; i += 4) {
			for (j = 0; j < 16; j++) {
				fb_zero(r);
				fb_set_dig(r, j);
				fb_lsh(r, r, i);
				if (b >= 0) {
					for (k = 0; k < b; k++) {
						fb_sqr(r, r);
					}
				} else {
					for (k = 0; k < -b; k++) {
						fb_srt(r, r);
					}
				}

#if ALLOC == AUTO
				fb_copy((dig_t *)t + (4 * i + j) * FB_DIGS, r);
#else
				fb_copy(t[4 * i + j], r);
#endif
			}
		}
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fb_free(r);
	}
}

void fb_itr_quick(fb_t c, const fb_t a, const fb_t *t) {
	fb_itrn_low(c, a, (dig_t *)t);
}
