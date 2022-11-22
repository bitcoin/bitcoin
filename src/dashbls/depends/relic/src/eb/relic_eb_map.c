/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2010 RELIC Authors
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
	uint8_t digest[RLC_MD_LEN];

	bn_null(k);
	fb_null(t0);
	fb_null(t1);

	RLC_TRY {
		bn_new(k);
		fb_new(t0);
		fb_new(t1);

		md_map(digest, msg, len);
		bn_read_bin(k, digest, RLC_MIN(RLC_FB_BYTES, RLC_MD_LEN));
		fb_set_dig(p->z, 1);

		i = 0;
		while (1) {
			bn_add_dig(k, k, 1);
			bn_mod_2b(k, k, RLC_FB_BITS);
			dv_copy(p->x, k->dp, RLC_FB_DIGS);

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

				p->coord = BASIC;
				break;
			}
		}
		/* Now, multiply by cofactor to get the correct group. */
		eb_curve_get_cof(k);
		if (bn_bits(k) < RLC_DIG) {
			eb_mul_dig(p, p, k->dp[0]);
		} else {
			eb_mul(p, p, k);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(k);
		fb_free(t0);
		fb_free(t1);
	}
}
