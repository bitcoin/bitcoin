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
 * Implementation of prime field exponentiation functions.
 *
 * @ingroup bn
 */

#include <string.h>

#include "relic_conf.h"
#include "relic_core.h"
#include "relic_fb.h"
#include "relic_bn.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if FB_EXP == BASIC || !defined(STRIP)

void fb_exp_basic(fb_t c, const fb_t a, const bn_t b) {
	int i, l;
	fb_t r;

	if (bn_is_zero(b)) {
		fb_set_dig(c, 1);
		return;
	}

	fb_null(r);

	TRY {
		fb_new(r);

		l = bn_bits(b);

		fb_copy(r, a);

		for (i = l - 2; i >= 0; i--) {
			fb_sqr(r, r);
			if (bn_get_bit(b, i)) {
				fb_mul(r, r, a);
			}
		}

		if (bn_sign(b) == BN_NEG) {
			fb_inv(c, r);
		} else {
			fb_copy(c, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fb_free(r);
	}
}

#endif

#if FB_EXP == SLIDE || !defined(STRIP)

void fb_exp_slide(fb_t c, const fb_t a, const bn_t b) {
	fb_t t[1 << (FB_WIDTH - 1)], r;
	int i, j, l;
	uint8_t win[FB_BITS + 1];

	fb_null(r);

	if (bn_is_zero(b)) {
		fb_set_dig(c, 1);
		return;
	}


	/* Initialize table. */
	for (i = 0; i < (1 << (FB_WIDTH - 1)); i++) {
		fb_null(t[i]);
	}

	TRY {
		for (i = 0; i < (1 << (FB_WIDTH - 1)); i ++) {
			fb_new(t[i]);
		}
		fb_new(r);

		fb_copy(t[0], a);
		fb_sqr(r, a);

		/* Create table. */
		for (i = 1; i < 1 << (FB_WIDTH - 1); i++) {
			fb_mul(t[i], t[i - 1], r);
		}

		fb_set_dig(r, 1);
		l = FB_BITS + 1;
		bn_rec_slw(win, &l, b, FB_WIDTH);
		for (i = 0; i < l; i++) {
			if (win[i] == 0) {
				fb_sqr(r, r);
			} else {
				for (j = 0; j < util_bits_dig(win[i]); j++) {
					fb_sqr(r, r);
				}
				fb_mul(r, r, t[win[i] >> 1]);
			}
		}

		if (bn_sign(b) == BN_NEG) {
			fb_inv(c, r);
		} else {
			fb_copy(c, r);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		for (i = 0; i < (1 << (FB_WIDTH - 1)); i++) {
			fb_free(t[i]);
		}
		fb_free(r);
	}
}

#endif

#if FB_EXP == MONTY || !defined(STRIP)

void fb_exp_monty(fb_t c, const fb_t a, const bn_t b) {
	fb_t t[2];

	if (bn_is_zero(b)) {
		fb_set_dig(c, 1);
		return;
	}

	fb_null(t[0]);
	fb_null(t[1]);

	TRY {
		fb_new(t[0]);
		fb_new(t[1]);

		fb_set_dig(t[0], 1);
		fb_copy(t[1], a);

		for (int i = bn_bits(b) - 1; i >= 0; i--) {
			int j = bn_get_bit(b, i);
			dv_swap_cond(t[0], t[1], FB_DIGS, j ^ 1);
			fb_mul(t[0], t[0], t[1]);
			fb_sqr(t[1], t[1]);
			dv_swap_cond(t[0], t[1], FB_DIGS, j ^ 1);
		}

		if (bn_sign(b) == BN_NEG) {
			fb_inv(c, t[0]);
		} else {
			fb_copy(c, t[0]);
		}
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fb_free(t[1]);
		fb_free(t[0]);
	}
}

#endif
