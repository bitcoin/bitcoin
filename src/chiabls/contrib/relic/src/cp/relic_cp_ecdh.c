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
 * Implementation of the ECDH protocol.
 *
 * @ingroup cp
 */

#include "relic.h"
#include "relic_test.h"
#include "relic_bench.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_ecdh_gen(bn_t d, ec_t q) {
	bn_t n;
	int result = STS_OK;

	bn_null(n);

	TRY {
		bn_new(n);

		ec_curve_get_ord(n);
		bn_rand_mod(d, n);
		ec_mul_gen(q, d);
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(n);
	}

	return result;
}

int cp_ecdh_key(uint8_t *key, int key_len, bn_t d, ec_t q) {
	ec_t p;
	bn_t x, h;
	int l, result = STS_OK;
	uint8_t _x[FC_BYTES];

	ec_null(p);
	bn_null(x);
	bn_null(h);

	TRY {
		ec_new(p);
		bn_new(x);
		bn_new(h);

		ec_curve_get_cof(h);
		if (bn_bits(h) < BN_DIGIT) {
			ec_mul_dig(p, q, h->dp[0]);
		} else {
			ec_mul(p, q, h);
		}
		ec_mul(p, p, d);
		if (ec_is_infty(p)) {
			result = STS_ERR;
		}
		ec_get_x(x, p);
		l = bn_size_bin(x);
		bn_write_bin(_x, l, x);
		md_kdf2(key, key_len, _x, l);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ec_free(p);
		bn_free(x);
		bn_free(h);
	}
	return result;
}
