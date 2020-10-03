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
 * Implementation of the Boneh-Lynn-Schacham short signature protocol.
 *
 * @ingroup cp
 */

#include "relic.h"
#include "relic_test.h"
#include "relic_bench.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_bls_gen(bn_t d, g2_t q) {
	bn_t n;
	int result = STS_OK;

	bn_null(n);

	TRY {
		bn_new(n);

		g2_get_ord(n);
		bn_rand_mod(d, n);
		g2_mul_gen(q, d);
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(n);
	}
	return result;
}

int cp_bls_sig(g1_t s, uint8_t *msg, int len, bn_t d) {
	g1_t p;
	int result = STS_OK;

	g1_null(p);

	TRY {
		g1_new(p);
		g1_map(p, msg, len);
		g1_mul(s, p, d);
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		g1_free(p);
	}
	return result;
}

int cp_bls_ver(g1_t s, uint8_t *msg, int len, g2_t q) {
	g1_t p[2];
	g2_t r[2];
	gt_t e;
	int result = 0;

	g1_null(p[0]);
	g1_null(p[1]);
	g2_null(r[0]);
	g2_null(r[1]);
	gt_null(e);

	TRY {
		g1_new(p[0]);
		g1_new(p[1]);
		g2_new(r[0]);
		g2_new(r[1]);
		gt_new(e);

		g1_map(p[0], msg, len);
		g1_copy(p[1], s);
		g2_copy(r[0], q);
		g2_get_gen(r[1]);
		g2_neg(r[1], r[1]);

		pc_map_sim(e, p, r, 2);
		if (gt_is_unity(e)) {
			result = 1;
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		g1_free(p[0]);
		g1_free(p[1]);
		g2_free(r[0]);
		g2_free(r[1]);
		gt_free(e);
	}
	return result;
}
