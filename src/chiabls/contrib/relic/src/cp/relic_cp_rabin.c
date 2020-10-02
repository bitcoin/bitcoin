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
 * Implementation of the Rabin cryptosystem.
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
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Length in bytes of added redundancy.
 */
#define RABIN_PAD_LEN		8

/**
 * Byte used as padding unit in ciphertext.
 */
#define RABIN_PAD			(0xFF)

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_rabin_gen(rabin_t pub, rabin_t prv, int bits) {
	bn_t r;
	int result = STS_OK;

	bn_null(r);

	TRY {
		bn_new(r);

		/* Generate different primes p and q. */
		do {
			bn_gen_prime(prv->p, bits / 2);
			bn_mod_2b(r, prv->p, 2);
		} while (bn_cmp_dig(r, 3) != CMP_EQ);

		do {
			bn_gen_prime(prv->q, bits / 2);
			bn_mod_2b(r, prv->q, 2);
		} while (bn_cmp(prv->p, prv->q) == CMP_EQ ||
				bn_cmp_dig(r, 3) != CMP_EQ);

		/* Swap p and q so that p is smaller. */
		if (bn_cmp(prv->p, prv->q) == CMP_LT) {
			bn_copy(r, prv->p);
			bn_copy(prv->p, prv->q);
			bn_copy(prv->q, r);
		}

		bn_gcd_ext(r, prv->dp, prv->dq, prv->p, prv->q);
		if (bn_cmp_dig(r, 1) != CMP_EQ) {
			result = STS_ERR;
		}

		bn_mul(prv->n, prv->p, prv->q);
		bn_copy(pub->n, prv->n);
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(r);
	}

	return result;
}

int cp_rabin_enc(uint8_t *out, int *out_len, uint8_t *in, int in_len,
		rabin_t pub) {
	bn_t m, t;
	int size, result = STS_OK;

	bn_null(m);
	bn_null(t);

	size = bn_size_bin(pub->n);

	if (in_len <= 0 || in_len > (size - RABIN_PAD_LEN - 2)) {
		return STS_ERR;
	}

	TRY {
		bn_new(m);
		bn_new(t);
		bn_zero(m);

		/* EB = 00 | FF || D || SUFFIX */
		bn_lsh(m, m, 8);
		bn_add_dig(m, m, RABIN_PAD);
		bn_lsh(m, m, in_len * 8);
		bn_read_bin(t, in, in_len);
		bn_add(m, m, t);
		bn_mod_2b(t, m, 8 * RABIN_PAD_LEN);
		bn_lsh(m, m, 8 * RABIN_PAD_LEN);
		bn_add(m, m, t);

		bn_sqr(m, m);
		bn_mod(m, m, pub->n);

		if (size <= *out_len) {
			*out_len = size;
			memset(out, 0, *out_len);
			bn_write_bin(out, size, m);
		} else {
			result = STS_ERR;
		}
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(m);
		bn_free(t);
	}

	return result;
}

int cp_rabin_dec(uint8_t *out, int *out_len, uint8_t *in, int in_len,
		rabin_t prv) {
	bn_t m, m0, m1, t, n;
	int size, result = STS_OK;
	uint8_t pad;

	if (in_len < RABIN_PAD_LEN) {
		return STS_ERR;
	}

	bn_null(m);
	bn_null(m0);
	bn_null(m1);
	bn_null(n);
	bn_null(t);

	TRY {
		bn_new(m);
		bn_new(m0);
		bn_new(m1);
		bn_new(n);
		bn_new(t);

		bn_read_bin(m, in, in_len);

		bn_add_dig(t, prv->p, 1);
		bn_rsh(t, t, 2);
		bn_mxp(m0, m, t, prv->p);

		bn_add_dig(t, prv->q, 1);
		bn_rsh(t, t, 2);
		bn_mxp(m1, m, t, prv->q);

		bn_mul(m, prv->dp, prv->p);
		bn_mul(m, m, m1);
		bn_mul(t, prv->dq, prv->q);
		bn_mul(t, t, m0);
		bn_add(m0, m, t);
		bn_mod(m0, m0, prv->n);
		if (bn_sign(m0) == BN_NEG) {
			bn_add(m0, m0, prv->n);
		}
		bn_sub(m1, m, t);
		bn_mod(m1, m1, prv->n);
		if (bn_sign(m1) == BN_NEG) {
			bn_add(m1, m1, prv->n);
		}

		bn_mod_2b(m, m0, 8 * RABIN_PAD_LEN);
		bn_rsh(t, m0, 8 * RABIN_PAD_LEN);
		bn_mod_2b(t, t, 8 * RABIN_PAD_LEN);
		if (bn_cmp(t, m) == CMP_EQ) {
			bn_rsh(m, m0, 8 * RABIN_PAD_LEN);
		} else {
			bn_sub(m0, prv->n, m0);
			bn_mod_2b(m, m0, 8 * RABIN_PAD_LEN);
			bn_rsh(t, m0, 8 * RABIN_PAD_LEN);
			bn_mod_2b(t, t, 8 * RABIN_PAD_LEN);
			if (bn_cmp(t, m) == CMP_EQ) {
				bn_rsh(m, m0, 8 * RABIN_PAD_LEN);
			} else {
				bn_mod_2b(m, m1, 8 * RABIN_PAD_LEN);
				bn_rsh(t, m1, 8 * RABIN_PAD_LEN);
				bn_mod_2b(t, t, 8 * RABIN_PAD_LEN);
				if (bn_cmp(t, m) == CMP_EQ) {
					bn_rsh(m, m1, 8 * RABIN_PAD_LEN);
				} else {
					bn_sub(m1, prv->n, m1);
					bn_mod_2b(m, m1, 8 * RABIN_PAD_LEN);
					bn_rsh(t, m1, 8 * RABIN_PAD_LEN);
					bn_mod_2b(t, t, 8 * RABIN_PAD_LEN);
					if (bn_cmp(t, m) == CMP_EQ) {
						bn_rsh(m, m1, 8 * RABIN_PAD_LEN);
					} else {
						result = STS_ERR;
					}
				}
			}
		}

		if (result == STS_OK) {
			size = bn_size_bin(prv->n);
			size--;
			bn_rsh(t, m, 8 * size);

			if (!bn_is_zero(t)) {
				result = STS_ERR;
			} else {
				do {
					size--;
					bn_rsh(t, m, 8 * size);
					pad = (uint8_t)t->dp[0];
				} while (pad == 0);

				if (pad != RABIN_PAD) {
					result = STS_ERR;
				} else {
					bn_mod_2b(m, m, size * 8);
				}
			}

			if (size <= *out_len) {
				*out_len = size;
				memset(out, 0, size);
				bn_write_bin(out, size, m);
			} else {
				result = STS_ERR;
			}
		}
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(m);
		bn_free(m0);
		bn_free(m1);
		bn_free(n);
		bn_free(t);
	}

	return result;
}
