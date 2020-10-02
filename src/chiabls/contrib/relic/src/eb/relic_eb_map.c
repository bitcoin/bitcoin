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
 * Implementation of hashing to a binary elliptic curve.
 *
 * @ingroup eb
 */

#include "relic_core.h"
#include "relic_md.h"
#include "relic_eb.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void eb_map(eb_t p, const uint8_t *msg, int len) {
	bn_t k;
	fb_t t0, t1;
	int i;
	uint8_t digest[MD_LEN];

	bn_null(k);
	fb_null(t0);
	fb_null(t1);

	TRY {
		bn_new(k);
		fb_new(t0);
		fb_new(t1);

		md_map(digest, msg, len);
		bn_read_bin(k, digest, MIN(FB_BYTES, MD_LEN));
		fb_set_dig(p->z, 1);

		i = 0;
		while (1) {
			bn_add_dig(k, k, 1);
			bn_mod_2b(k, k, FB_BITS);
			dv_copy(p->x, k->dp, FB_DIGS);

			eb_rhs(t1, p);

			/* t0 = 1/x1^2. */
			fb_sqr(t0, p->x);
			fb_inv(t0, t0);
			/* t0 = t1/x1^2. */
			fb_mul(t0, t0, t1);
			/* Solve t1^2 + t1 = t0. */
			if (fb_trc(t0) != 0) {
				i++;
			} else {
				fb_slv(t1, t0);
				/* x3 = x1, y3 = t1 * x1, z3 = 1. */
				fb_mul(p->y, t1, p->x);
				fb_set_dig(p->z, 1);

				p->norm = 1;
				break;
			}
		}
		/* Now, multiply by cofactor to get the correct group. */
		eb_curve_get_cof(k);
		if (bn_bits(k) < BN_DIGIT) {
			eb_mul_dig(p, p, k->dp[0]);
		} else {
			eb_mul(p, p, k);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(k);
		fb_free(t0);
		fb_free(t1);
	}
}
