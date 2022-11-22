/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2020 RELIC Authors
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
 * Implementation of the Damgård-Jurik-Nielsen Generalized Homomorphic
 * Probabilistic Encryption.
 *
 * @ingroup cp
 */

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_ghpe_gen(bn_t pub, bn_t prv, int bits) {
	int result = RLC_OK;
	bn_t p, q, r;

	bn_null(p);
	bn_null(q);
	bn_null(r);

	RLC_TRY {
		bn_new(p);
		bn_new(q);
		bn_new(r);

		/* Generate primes p and q of equivalent length. */
		do {
			bn_gen_prime(p, bits / 2);
			bn_mod_2b(r, p, 2);
		} while (bn_cmp_dig(r, 3) != RLC_EQ);

		do {
			bn_gen_prime(q, bits / 2);
			bn_mod_2b(r, q, 2);
		} while (bn_cmp(p, q) == RLC_EQ || bn_cmp_dig(r, 3) != RLC_EQ);

		/* Compute n = pq and l = \phi(n). */
		bn_sub_dig(p, p, 1);
		bn_sub_dig(q, q, 1);
		bn_mul(prv, p, q);
		bn_add_dig(p, p, 1);
		bn_add_dig(q, q, 1);
		bn_mul(pub, p, q);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(p);
		bn_free(q);
		bn_free(r);
	}

	return result;
}

int cp_ghpe_enc(bn_t c, bn_t m, bn_t pub, int s) {
	bn_t g, r, t;
	int result = RLC_OK;

	bn_null(g);
	bn_null(r);
	bn_null(t);

	if (pub == NULL || bn_bits(m) > s * bn_bits(pub)) {
		return RLC_ERR;
	}

	RLC_TRY {
		bn_new(g);
		bn_new(r);
		bn_new(t);

		/* Generate r in Z_n^*. */
		bn_rand_mod(r, pub);
		/* Compute c = (g^m)(r^n) mod n^2. */
		bn_add_dig(g, pub, 1);

		/* t = n^(s + 1). */
		bn_copy(t, pub);
		for (int i = 0; i < s; i++) {
			bn_mul(t, t, pub);
		}

		bn_mxp(c, g, m, t);
		for (int i = 0; i < s; i++) {
			bn_mxp(r, r, pub, t);
		}

		bn_mul(c, c, r);
		bn_mod(c, c, t);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(g);
		bn_free(r);
		bn_free(t);
	}

	return result;
}

int cp_ghpe_dec(bn_t m, bn_t c, bn_t pub, bn_t prv, int s) {
	bn_t i, l, r, t, u, v, x;
	int result = RLC_OK;
	dig_t fk;

	if (bn_bits(c) > (s + 1) * bn_bits(pub)) {
		return RLC_ERR;
	}

	bn_null(i);
	bn_null(l);
	bn_null(r);
	bn_null(t);
	bn_null(u);
	bn_null(v);
	bn_null(x);

	RLC_TRY {
		bn_new(i);
		bn_new(l);
		bn_new(r);
		bn_new(t);
		bn_new(u);
		bn_new(v);
		bn_new(x);

		/* t = n^(s + 1). */
		bn_sqr(t, pub);
		for (int i = 1; i < s; i++) {
			bn_mul(t, t, pub);
		}

		/* Compute (c^l mod n^(s + 1)) * u mod n. */
		bn_mxp(m, c, prv, t);

		bn_zero(i);
		bn_copy(t, pub);
		for (int j = 1; j <= s; j++) {
			/* u = t1 = L(c mod n^(j+1)). */
			bn_mul(r, t, pub);
			bn_mod(u, m, r);
			bn_sub_dig(u, u, 1);
			bn_div(u, u, pub);
			/* v = t2 = i. */
			bn_copy(v, i);
			bn_copy(l, pub);
			fk = 1;
			for (int k = 2; k <= j; k++) {
				bn_sub_dig(i, i, 1);
				/* Accumulate fk = f!. */
				fk = fk * k;
				/* t2 = t2 * i mod n^j. */
				bn_mul(v, v, i);
				bn_mod(v, v, t);
				/* t1 = t1 - t2 * n^(k-1)/k! mod n^j. */
				bn_mul(x, v, l);
				bn_div_dig(x, x, fk);
				bn_mod(x, x, t);
				bn_sub(u, u, x);
				while (bn_sign(u) == RLC_NEG) {
					bn_add(u, u, t);
				}
				bn_mod(u, u, t);
				bn_mul(l, l, pub);
			}
			if (j < s) {
				bn_copy(t, r);
				bn_copy(i, u);
			}
		}
		bn_mod_inv(v, prv, t);
		bn_mul(m, u, v);
		bn_mod(m, m, t);
	} RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(i);
		bn_free(l);
		bn_free(r);
		bn_free(t);
		bn_free(u);
		bn_free(v);
		bn_free(x);
	}

	return result;
}
