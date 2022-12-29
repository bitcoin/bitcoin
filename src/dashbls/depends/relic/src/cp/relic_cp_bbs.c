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
 * Implementation of the Boneh-Boyen short signature protocol.
 *
 * @ingroup cp
 */

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_bbs_gen(bn_t d, g2_t q, gt_t z) {
	bn_t n;
	int result = RLC_OK;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		/* z = e(g1, g2). */
		gt_get_gen(z);

		pc_get_ord(n);

		/* Use short scalars. */
		do {
			bn_rand(d, RLC_POS, 2 * pc_param_level());
			bn_mod(d, d, n);
		} while (bn_is_zero(d));

		/* q = d * g2. */
		g2_mul_gen(q, d);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
	}
	return result;
}

int cp_bbs_sig(g1_t s, uint8_t *msg, int len, int hash, bn_t d) {
	bn_t m, n, r;
	uint8_t h[RLC_MD_LEN];
	int result = RLC_OK;

	bn_null(m);
	bn_null(n);
	bn_null(r);

	RLC_TRY {
		bn_new(m);
		bn_new(n);
		bn_new(r);

		pc_get_ord(n);

		/* m = H(msg). */
		if (hash) {
			bn_read_bin(m, msg, len);
		} else {
			md_map(h, msg, len);
			bn_read_bin(m, h, RLC_MD_LEN);
		}
		bn_mod(m, m, n);

		/* m = 1/(m + d) mod n. */
		bn_add(m, m, d);
		bn_mod_inv(m, m, n);
		/* s = 1/(m+d) * g1. */
		g1_mul_gen(s, m);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(m);
		bn_free(n);
		bn_free(r);
	}
	return result;
}

int cp_bbs_ver(g1_t s, uint8_t *msg, int len, int hash, g2_t q, gt_t z) {
	bn_t m, n;
	g2_t g;
	gt_t e;
	uint8_t h[RLC_MD_LEN];
	int result = 0;

	bn_null(m);
	bn_null(n);
	g2_null(g);
	gt_null(e);

	if (!g1_is_valid(s)) {
		return result;
	}

	RLC_TRY {
		bn_new(m);
		bn_new(n);
		g2_new(g);
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

		g2_mul_gen(g, m);
		g2_add(g, g, q);
		g2_norm(g, g);

		pc_map(e, s, g);

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
		g2_free(g);
		gt_free(e);
	}
	return result;
}
