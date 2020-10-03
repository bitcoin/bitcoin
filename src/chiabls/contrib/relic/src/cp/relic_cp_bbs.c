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
 * Implementation of the Boneh-Boyen short signature protocol.
 *
 * @ingroup cp
 */

#include "relic.h"
#include "relic_test.h"
#include "relic_bench.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_bbs_gen(bn_t d, g2_t q, gt_t z) {
	bn_t n;
	g1_t g;
	int result = STS_OK;

	bn_null(n);
	g1_null(g);

	TRY {
		bn_new(n);
		g1_new(g);

		g1_get_gen(g);
		g2_get_gen(q);

		/* z = e(g1, g2). */
		pc_map(z, g, q);

		g2_get_ord(n);

		/* Use short scalars. */
		do {
			bn_rand(d, BN_POS, 2 * pc_param_level());
			bn_mod(d, d, n);
		} while (bn_is_zero(d));

		/* q = d * g2. */
		g2_mul_gen(q, d);
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(n);
		g1_free(g);
	}
	return result;
}

int cp_bbs_sig(g1_t s, uint8_t *msg, int len, int hash, bn_t d) {
	bn_t m, n, r;
	uint8_t h[MD_LEN];
	int result = STS_OK;

	bn_null(m);
	bn_null(n);
	bn_null(r);

	TRY {
		bn_new(m);
		bn_new(n);
		bn_new(r);

		g1_get_ord(n);

		/* m = H(msg). */
		if (hash) {
			bn_read_bin(m, msg, len);
		} else {
			md_map(h, msg, len);
			bn_read_bin(m, h, MD_LEN);
		}
		bn_mod(m, m, n);

		/* m = 1/(m + d) mod n. */
		bn_add(m, m, d);
		bn_gcd_ext(r, m, NULL, m, n);
		if (bn_sign(m) == BN_NEG) {
			bn_add(m, m, n);
		}
		/* s = 1/(m+d) * g1. */
		g1_mul_gen(s, m);
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
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
	uint8_t h[MD_LEN];
	int result = 0;

	bn_null(m);
	bn_null(n);
	g2_null(g);
	gt_null(e);

	if (!g1_is_valid(s)) {
		return result;
	}

	TRY {
		bn_new(m);
		bn_new(n);
		g2_new(g);
		gt_new(e);

		g2_get_ord(n);

		/* m = H(msg). */
		if (hash) {
			bn_read_bin(m, msg, len);
		} else {
			md_map(h, msg, len);
			bn_read_bin(m, h, MD_LEN);
		}
		bn_mod(m, m, n);

		g2_mul_gen(g, m);
		g2_add(g, g, q);
		g2_norm(g, g);

		pc_map(e, s, g);

		if (gt_cmp(e, z) == CMP_EQ) {
			result = 1;
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(m);
		bn_free(n);
		g2_free(g);
		gt_free(e);
	}
	return result;
}
