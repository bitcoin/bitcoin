/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2013 RELIC Authors
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
 * Implementation of the ECIES cryptosystem.
 *
 * @ingroup cp
 */

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_ecies_gen(bn_t d, ec_t q) {
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

int cp_ecies_enc(ec_t r, uint8_t *out, int *out_len, uint8_t *in, int in_len,
		ec_t q) {
	bn_t k, n, x;
	ec_t p;
	int l, result = RLC_OK, size = RLC_CEIL(RLC_MAX(128, ec_param_level()), 8);
	uint8_t _x[RLC_FC_BYTES + 1], iv[RLC_BC_LEN] = { 0 };
	uint8_t key[2 * 8 * (RLC_FC_BYTES + 1)];

	bn_null(k);
	bn_null(n);
	bn_null(x);
	ec_null(p);

	RLC_TRY {
		bn_new(k);
		bn_new(n);
		bn_new(x);
		ec_new(p);

		ec_curve_get_ord(n);
		bn_rand_mod(k, n);

		ec_mul_gen(r, k);
		ec_mul(p, q, k);
		ec_get_x(x, p);
		l = bn_size_bin(x);
		if (bn_bits(x) % 8 == 0) {
			/* Compatibility with BouncyCastle. */
			l = l + 1;
		}
		bn_write_bin(_x, l, x);
		md_kdf(key, 2 * size, _x, l);
		l = *out_len;
		if (bc_aes_cbc_enc(out, out_len, in, in_len, key, size, iv)
				!= RLC_OK || (*out_len + RLC_MD_LEN) > l) {
			result = RLC_ERR;
		} else {
			md_hmac(out + *out_len, out, *out_len, key + size, size);
			*out_len += RLC_MD_LEN;
		}
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(k);
		bn_free(n);
		bn_free(x);
		ec_free(p);
	}

	return result;
}

int cp_ecies_dec(uint8_t *out, int *out_len, ec_t r, uint8_t *in, int in_len,
		bn_t d) {
	ec_t p;
	bn_t x;

	int l, result = RLC_OK, size = RLC_CEIL(RLC_MAX(128, ec_param_level()), 8);
	uint8_t _x[RLC_FC_BYTES + 1], h[RLC_MD_LEN], iv[RLC_BC_LEN] = { 0 };
	uint8_t key[2 * 8 * (RLC_FC_BYTES + 1)];

	bn_null(x);
	ec_null(p);

	RLC_TRY {
		bn_new(x);
		ec_new(p);

		ec_mul(p, r, d);
		ec_get_x(x, p);
		l = bn_size_bin(x);
		if (bn_bits(x) % 8 == 0) {
			/* Compatibility with BouncyCastle. */
			l = l + 1;
		}
		bn_write_bin(_x, l, x);
		md_kdf(key, 2 * size, _x, l);
		md_hmac(h, in, in_len - RLC_MD_LEN, key + size, size);
		if (util_cmp_const(h, in + in_len - RLC_MD_LEN, RLC_MD_LEN)) {
			result = RLC_ERR;
		} else {
			if (bc_aes_cbc_dec(out, out_len, in, in_len - RLC_MD_LEN, key, size, iv)
					!= RLC_OK) {
				result = RLC_ERR;
			}
		}
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(x);
		ec_free(p);
	}

	return result;
}
