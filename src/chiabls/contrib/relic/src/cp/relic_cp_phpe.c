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
 * Implementation of Paillier's Homomorphic Probabilistic Encryption.
 *
 * @ingroup cp
 */

#include <string.h>

#include "relic_core.h"
#include "relic_conf.h"
#include "relic_rand.h"
#include "relic_bn.h"
#include "relic_util.h"
#include "relic_cp.h"
#include "relic_md.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_phpe_gen(bn_t n, bn_t l, int bits) {
	bn_t p, q;
	int result = STS_OK;

	bn_null(p);
	bn_null(q);

	TRY {
		bn_new(p);
		bn_new(q);

		/* Generate primes p and q of equivalent length. */
		do {
			bn_gen_prime(p, bits / 2);
			bn_gen_prime(q, bits / 2);
		} while (bn_cmp(p, q) == CMP_EQ);

		/* Compute n = pq and l = \phi(n). */
		bn_mul(n, p, q);
		bn_sub_dig(p, p, 1);
		bn_sub_dig(q, q, 1);
		bn_mul(l, p, q);
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(p);
		bn_free(q);
	}

	return result;
}

int cp_phpe_enc(uint8_t *out, int *out_len, uint8_t *in, int in_len, bn_t n) {
	bn_t g, m, r, s;
	int size, result = STS_OK;

	bn_null(g);
	bn_null(m);
	bn_null(r);
	bn_null(s);

	size = bn_size_bin(n);

	if (n == NULL || in_len <= 0 || in_len > size) {
		return STS_ERR;
	}

	TRY {
		bn_new(g);
		bn_new(m);
		bn_new(r);
		bn_new(s);

		/* Represent m as a padded element of Z_n. */
		bn_read_bin(m, in, in_len);

		/* Generate r in Z_n^*. */
		bn_rand_mod(r, n);

		/* Compute c = (g^m)(r^n) mod n^2. */
		bn_add_dig(g, n, 1);
		bn_sqr(s, n);
		bn_mxp(m, g, m, s);
		bn_mxp(r, r, n, s);
		bn_mul(m, m, r);
		bn_mod(m, m, s);
		if (2 * size <= *out_len) {
			*out_len = 2 * size;
			memset(out, 0, *out_len);
			bn_write_bin(out, *out_len, m);
		} else {
			result = STS_ERR;
		}
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(g);
		bn_free(m);
		bn_free(r);
		bn_free(s);
	}

	return result;
}

int cp_phpe_dec(uint8_t *out, int out_len, uint8_t *in, int in_len, bn_t n,
	bn_t l) {
	bn_t c, u, s;
	int size, result = STS_OK;

	size = bn_size_bin(n);

	if (in_len < 0 || in_len != 2 * size) {
		return STS_ERR;
	}

	bn_null(c);
	bn_null(u);
	bn_null(s);

	TRY {
		bn_new(c);
		bn_new(u);
		bn_new(s);

		/* Compute (c^l mod n^2) * u mod n. */
		bn_sqr(s, n);
		bn_read_bin(c, in, in_len);
		bn_mxp(c, c, l, s);
		bn_sub_dig(c, c, 1);
		bn_div(c, c, n);
		bn_gcd_ext(s, u, NULL, l, n);
		if (bn_sign(u) == BN_NEG) {
			bn_add(u, u, n);
		}
		bn_mul(c, c, u);
		bn_mod(c, c, n);

		size = bn_size_bin(c);
		if (size <= out_len) {
			memset(out, 0, out_len);
			bn_write_bin(out + (out_len - size), size, c);
		} else {
			result = STS_ERR;
		}
	} CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(c);
		bn_free(u);
		bn_free(s);
	}

	return result;
}
