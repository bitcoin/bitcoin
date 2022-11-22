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
 * Implementation of the ECDH protocol.
 *
 * @ingroup cp
 */

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_ecdh_gen(bn_t d, ec_t q) {
	bn_t n;
	int result = RLC_OK;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		ec_curve_get_ord(n);
		bn_rand_mod(d, n);
		ec_mul_gen(q, d);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
	}

	return result;
}

int cp_ecdh_key(uint8_t *key, int key_len, bn_t d, ec_t q) {
	ec_t p;
	bn_t x, h;
	int l, result = RLC_OK;
	uint8_t _x[RLC_FC_BYTES];

	ec_null(p);
	bn_null(x);
	bn_null(h);

	RLC_TRY {
		ec_new(p);
		bn_new(x);
		bn_new(h);

		ec_curve_get_cof(h);
		if (bn_bits(h) < RLC_DIG) {
			ec_mul_dig(p, q, h->dp[0]);
		} else {
			ec_mul(p, q, h);
		}
		ec_mul(p, p, d);
		if (ec_is_infty(p)) {
			result = RLC_ERR;
		}
		ec_get_x(x, p);
		l = bn_size_bin(x);
		bn_write_bin(_x, l, x);
		md_kdf(key, key_len, _x, l);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ec_free(p);
		bn_free(x);
		bn_free(h);
	}
	return result;
}
