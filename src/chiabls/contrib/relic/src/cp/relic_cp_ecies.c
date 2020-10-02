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
 * Implementation of the ECIES cryptosystem.
 *
 * @ingroup cp
 */

#include <string.h>

#include "relic_core.h"
#include "relic_bn.h"
#include "relic_util.h"
#include "relic_cp.h"
#include "relic_md.h"
#include "relic_bc.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_ecies_gen(bn_t d, ec_t q) {
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

int cp_ecies_enc(ec_t r, uint8_t *out, int *out_len, uint8_t *in, int in_len,
		ec_t q) {
	bn_t k, n, x;
	ec_t p;
	int l, result = STS_OK, size = CEIL(ec_param_level(), 8);
	uint8_t _x[FC_BYTES + 1], key[2 * size], iv[BC_LEN] = { 0 };

	bn_null(k);
	bn_null(n);
	bn_null(x);
	ec_null(p);

	TRY {
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
		md_kdf2(key, 2 * size, _x, l);
		l = *out_len;
		if (bc_aes_cbc_enc(out, out_len, in, in_len, key, 8 * size, iv)
				!= STS_OK || (*out_len + MD_LEN) > l) {
			result = STS_ERR;
		} else {
			md_hmac(out + *out_len, out, *out_len, key + size, size);
			*out_len += MD_LEN;
		}
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
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
	int l, result = STS_OK, size = CEIL(ec_param_level(), 8);
	uint8_t _x[FC_BYTES + 1], h[MD_LEN], key[2 * size], iv[BC_LEN] = { 0 };

	bn_null(x);
	ec_null(p);

	TRY {
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
		md_kdf2(key, 2 * size, _x, l);
		md_hmac(h, in, in_len - MD_LEN, key + size, size);
		if (util_cmp_const(h, in + in_len - MD_LEN, MD_LEN)) {
			result = STS_ERR;
		} else {
			if (bc_aes_cbc_dec(out, out_len, in, in_len - MD_LEN, key, 8 * size, iv)
					!= STS_OK) {
				result = STS_ERR;
			}
		}
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(x);
		ec_free(p);
	}

	return result;
}
