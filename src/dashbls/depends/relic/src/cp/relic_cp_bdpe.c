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
 * Implementation of Benaloh's Dense Probabilistic Encryption cryptosystem.
 *
 * @ingroup cp
 */

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_bdpe_gen(bdpe_t pub, bdpe_t prv, dig_t block, int bits) {
	bn_t t, r;
	int result = RLC_OK;

	bn_null(t);
	bn_null(r);

	RLC_TRY {
		bn_new(t);
		bn_new(r);

		prv->t = pub->t = block;

		/* Make sure that block size is prime. */
		bn_set_dig(t, block);
		if (bn_is_prime_basic(t) == 0) {
			RLC_THROW(ERR_NO_VALID);
		}

		/* Generate prime q such that gcd(block, (q - 1)) = 1. */
		do {
			bn_gen_prime(prv->q, bits / 2);
			bn_sub_dig(prv->q, prv->q, 1);
			bn_gcd_dig(t, prv->q, block);
			bn_add_dig(prv->q, prv->q, 1);
		} while (bn_cmp_dig(t, 1) != RLC_EQ);

		/* Generate different primes p and q. */
		do {
			/* Compute p = block * (x * block + b) + 1, 0 < b < block random. */
			bn_rand(prv->p, RLC_POS, bits / 2 - 2 * util_bits_dig(block));
			bn_mul_dig(prv->p, prv->p, block);
			bn_rand(t, RLC_POS, util_bits_dig(block));
			bn_add_dig(prv->p, prv->p, t->dp[0]);

			/* We know that block divides (p-1). */
			bn_gcd_dig(t, prv->p, block);
			bn_mul_dig(prv->p, prv->p, block);
			bn_add_dig(prv->p, prv->p, 1);
		} while (bn_cmp_dig(t, 1) != RLC_EQ || bn_is_prime(prv->p) == 0);

		/* Compute t = (p-1)*(q-1). */
		bn_sub_dig(prv->q, prv->q, 1);
		bn_sub_dig(prv->p, prv->p, 1);
		bn_mul(t, prv->p, prv->q);
		bn_div_dig(t, t, block);

		/* Restore factors p and q and compute n = p * q. */
		bn_add_dig(prv->p, prv->p, 1);
		bn_add_dig(prv->q, prv->q, 1);
		bn_mul(pub->n, prv->p, prv->q);
		bn_copy(prv->n, pub->n);

		/* Select random y such that y^{(p-1)(q-1)}/block \neq 1 mod N. */
		do {
			bn_rand(pub->y, RLC_POS, bits);
			bn_mxp(r, pub->y, t, pub->n);
		} while (bn_cmp_dig(r, 1) == RLC_EQ);

		bn_copy(prv->y, pub->y);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(t);
		bn_free(r);
	}

	return result;
}

int cp_bdpe_enc(uint8_t *out, int *out_len, dig_t in, bdpe_t pub) {
	bn_t m, u;
	int size, result = RLC_OK;

	bn_null(m);
	bn_null(u);

	size = bn_size_bin(pub->n);

	if (in > pub->t) {
		return RLC_ERR;
	}

	RLC_TRY {
		bn_new(m);
		bn_new(u);

		bn_set_dig(m, in);

		bn_rand_mod(u, pub->n);
		bn_mxp(m, pub->y, m, pub->n);
		bn_mxp_dig(u, u, pub->t, pub->n);
		bn_mul(m, m, u);
		bn_mod(m, m, pub->n);

		if (size <= *out_len) {
			*out_len = size;
			memset(out, 0, *out_len);
			bn_write_bin(out, size, m);
		} else {
			result = RLC_ERR;
		}
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(m);
		bn_free(u);
	}

	return result;
}

int cp_bdpe_dec(dig_t *out, uint8_t *in, int in_len, bdpe_t prv) {
	bn_t m, t, z;
	int size, result = RLC_OK;
	dig_t i;

	size = bn_size_bin(prv->n);

	if (in_len < 0 || in_len != size) {
		return RLC_ERR;
	}

	bn_null(m);
	bn_null(t);
	bn_null(z);

	RLC_TRY {
		bn_new(m);
		bn_new(t);
		bn_new(z);

		/* Compute t = (p-1)(q-1)/block. */
		bn_mul(t, prv->p, prv->q);
		bn_sub(t, t, prv->p);
		bn_sub(t, t, prv->q);
		bn_add_dig(t, t, 1);
		bn_div_dig(t, t, prv->t);
		bn_read_bin(m, in, in_len);
		bn_mxp(m, m, t, prv->n);
		bn_mxp(t, prv->y, t, prv->n);

		for (i = 0; i < prv->t; i++) {
			bn_mxp_dig(z, t, i, prv->n);
			if (bn_cmp(z, m) == RLC_EQ) {
				*out = i;
				break;
			}
		}

		if (i == prv->t) {
			result = RLC_ERR;
		}
	} RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(m);
		bn_free(t);
		bn_free(z);
	}

	return result;
}
