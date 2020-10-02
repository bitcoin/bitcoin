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
 * Implementation of the Zhang-Safavi-Naini-Susilo short signature protocol.

 *
 * @ingroup cp
 */

#include "relic.h"
#include "relic_test.h"
#include "relic_bench.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_zss_gen(bn_t d, g1_t q, gt_t z) {
	bn_t n;
	g2_t g;
	int result = STS_OK;

	bn_null(n);
	g2_null(g);

	TRY {
		bn_new(n);
		g2_new(g);

		g1_get_gen(q);
		g2_get_gen(g);

		/* z = e(g1, g2). */
		pc_map(z, q, g);

		g2_get_ord(n);

		bn_rand_mod(d, n);
		g1_mul_gen(q, d);
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(n);
		g2_free(g);
	}
	return result;
}

int cp_zss_sig(g2_t s, uint8_t *msg, int len, int hash, bn_t d) {
	bn_t m, n, r, t;
	uint8_t h[MD_LEN];
	int result = STS_OK;

	bn_null(m);
	bn_null(n);
	bn_null(r);
	bn_null(t);

	TRY {
		bn_new(m);
		bn_new(n);
		bn_new(r);
		bn_new(t);

		g1_get_ord(n);

		/* m = H(msg). */
		if (hash) {
			bn_read_bin(m, msg, len);
		} else {
			md_map(h, msg, len);
			bn_read_bin(m, h, MD_LEN);
		}
		bn_mod(m, m, n);

        /* Compute (H(m) + d) and invert. */
        bn_add(t, m, d);
        bn_mod(t, t, n);
        bn_gcd_ext(r, t, NULL, t, n);
		if (bn_sign(t) == BN_NEG) {
			bn_add(t, t, n);
		}

		/* Compute the sinature. */
		g2_mul_gen(s, t);
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
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
	uint8_t h[MD_LEN];
	int result = 0;

	bn_null(m);
	bn_null(n);
	g1_null(g);
	gt_null(e);

	TRY {
		bn_new(m);
		bn_new(n);
		g1_new(g);
		gt_new(e);

		g1_get_ord(n);

		/* m = H(msg). */
		if (hash) {
			bn_read_bin(m, msg, len);
		} else {
			md_map(h, msg, len);
			bn_read_bin(m, h, MD_LEN);
		}
		bn_mod(m, m, n);

		g1_mul_gen(g, m);
		g1_add(g, g, q);
		g1_norm(g, g);

		pc_map(e, g, s);

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
		g1_free(g);
		gt_free(e);
	}
	return result;
}
