/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2015 RELIC Authors
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
 * Implementation of the Zhang-Safavi-Naini-Susilo short signature protocol.

 *
 * @ingroup cp
 */

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_zss_gen(bn_t d, g1_t q, gt_t z) {
	bn_t n;
	int result = RLC_OK;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		/* z = e(g1, g2). */
		gt_get_gen(z);

		pc_get_ord(n);

		bn_rand_mod(d, n);
		g1_mul_gen(q, d);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
	}
	return result;
}

int cp_zss_sig(g2_t s, uint8_t *msg, int len, int hash, bn_t d) {
	bn_t m, n, r, t;
	uint8_t h[RLC_MD_LEN];
	int result = RLC_OK;

	bn_null(m);
	bn_null(n);
	bn_null(r);
	bn_null(t);

	RLC_TRY {
		bn_new(m);
		bn_new(n);
		bn_new(r);
		bn_new(t);

		pc_get_ord(n);

		/* m = H(msg). */
		if (hash) {
			bn_read_bin(m, msg, len);
		} else {
			md_map(h, msg, len);
			bn_read_bin(m, h, RLC_MD_LEN);
		}
		bn_mod(m, m, n);

        /* Compute (H(m) + d) and invert. */
        bn_add(t, m, d);
        bn_mod(t, t, n);
		bn_mod_inv(t, t, n);

		/* Compute the sinature. */
		g2_mul_gen(s, t);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(m);
		bn_free(n);
		bn_free(r);
		bn_free(t);
	}
	return result;
}

int cp_zss_ver(g2_t s, uint8_t *msg, int len, int hash, g1_t q, gt_t z) {
	bn_t m, n;
	g1_t g;
	gt_t e;
	uint8_t h[RLC_MD_LEN];
	int result = 0;

	bn_null(m);
	bn_null(n);
	g1_null(g);
	gt_null(e);

	RLC_TRY {
		bn_new(m);
		bn_new(n);
		g1_new(g);
		gt_new(e);

		pc_get_ord(n);

		/* m = H(msg). */
		if (hash) {
			bn_read_bin(m, msg, len);
		} else {
			md_map(h, msg, len);
			bn_read_bin(m, h, RLC_MD_LEN);
		}
		bn_mod(m, m, n);

		g1_mul_gen(g, m);
		g1_add(g, g, q);
		g1_norm(g, g);

		pc_map(e, g, s);

		if (gt_cmp(e, z) == RLC_EQ) {
			result = 1;
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(m);
		bn_free(n);
		g1_free(g);
		gt_free(e);
	}
	return result;
}
