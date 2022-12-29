/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2014 RELIC Authors
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
 * Implementation of Paillier's Homomorphic Probabilistic Encryption.
 *
 * @ingroup cp
 */

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_phpe_gen(bn_t pub, phpe_t prv, int bits) {
	int result = RLC_OK;

	/* Generate primes p and q of equivalent length. */
	do {
		bn_gen_prime(prv->p, bits / 2);
		bn_gen_prime(prv->q, bits / 2);
	} while (bn_cmp(prv->p, prv->q) == RLC_EQ);

	/* Compute n = pq and l = \phi(n). */
	bn_mul(prv->n, prv->p, prv->q);

#ifdef CP_CRT
	/* Fix g = n + 1. */
	bn_add_dig(pub, prv->n, 1);

	/* Precompute dp = 1/(pow(g, p-1, p^2)//p mod p. */
	bn_sqr(prv->dp, prv->p);
	bn_sub_dig(prv->p, prv->p, 1);
	bn_mxp(prv->dp, pub, prv->p, prv->dp);
	bn_sub_dig(prv->dp, prv->dp, 1);
	bn_div(prv->dp, prv->dp, prv->p);
	/* Precompute dq = 1/(pow(g, q-1, q^2)//q mod q. */
	bn_sqr(prv->dq, prv->q);
	bn_sub_dig(prv->q, prv->q, 1);
	bn_mxp(prv->dq, pub, prv->q, prv->dq);
	bn_sub_dig(prv->dq, prv->dq, 1);
	bn_div(prv->dq, prv->dq, prv->q);

	/* Restore p and q. */
	bn_add_dig(prv->p, prv->p, 1);
	bn_add_dig(prv->q, prv->q, 1);
	bn_mod_inv(prv->dp, prv->dp, prv->p);
	bn_mod_inv(prv->dq, prv->dq, prv->q);

	/* qInv = q^(-1) mod p. */
	bn_mod_inv(prv->qi, prv->q, prv->p);
#endif

	bn_copy(pub, prv->n);
	return result;
}

int cp_phpe_enc(bn_t c, bn_t m, bn_t pub) {
	bn_t g, r, s;
	int result = RLC_OK;

	bn_null(g);
	bn_null(r);
	bn_null(s);

	if (pub == NULL || bn_bits(m) > bn_bits(pub)) {
		return RLC_ERR;
	}

	RLC_TRY {
		bn_new(g);
		bn_new(r);
		bn_new(s);

		/* Generate r in Z_n^*. */
		bn_rand_mod(r, pub);
		/* Compute c = (g^m)(r^n) mod n^2. */
		bn_add_dig(g, pub, 1);
		bn_sqr(s, pub);
		bn_mxp(c, g, m, s);
		bn_mxp(r, r, pub, s);
		bn_mul(c, c, r);
		bn_mod(c, c, s);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(g);
		bn_free(r);
		bn_free(s);
	}

	return result;
}

int cp_phpe_dec(bn_t m, bn_t c, phpe_t prv) {
	bn_t s, t, u, v;
	int result = RLC_OK;

	if (prv == NULL || bn_bits(c) > 2 * bn_bits(prv->n)) {
		return RLC_ERR;
	}

	bn_null(s);
	bn_null(t);
	bn_null(u);
	bn_null(v);

	RLC_TRY {
		bn_new(s);
		bn_new(t);
		bn_new(u);
		bn_new(v);

#if !defined(CP_CRT)
		bn_sub_dig(s, prv->p, 1);
		bn_sub_dig(t, prv->q, 1);
		bn_mul(s, s, t);
		/* Compute (c^l mod n^2) * u mod n. */
		bn_sqr(t, prv->n);
		bn_mxp(m, c, s, t);

		bn_sub_dig(m, m, 1);
		bn_div(m, m, prv->n);
		bn_mod_inv(t, s, prv->n);
		bn_mul(m, m, t);
		bn_mod(m, m, prv->n);
#else

#if MULTI == OPENMP
		omp_set_num_threads(CORES);
		#pragma omp parallel copyin(core_ctx) firstprivate(c, prv)
		{
			#pragma omp sections
			{
				#pragma omp section
				{
#endif
					/* Compute m_p = (c^(p-1) mod p^2) * dp mod p. */
					bn_sub_dig(t, prv->p, 1);
					bn_sqr(s, prv->p);
					bn_mxp(s, c, t, s);
					bn_sub_dig(s, s, 1);
					bn_div(s, s, prv->p);
					bn_mul(s, s, prv->dp);
					bn_mod(s, s, prv->p);
#if MULTI == OPENMP
				}
				#pragma omp section
				{
#endif
					/* Compute m_q = (c^(q-1) mod q^2) * dq mod q. */
					bn_sub_dig(v, prv->q, 1);
					bn_sqr(u, prv->q);
					bn_mxp(u, c, v, u);
					bn_sub_dig(u, u, 1);
					bn_div(u, u, prv->q);
					bn_mul(u, u, prv->dq);
					bn_mod(u, u, prv->q);
#if MULTI == OPENMP
				}
			}
		}
#endif

		/* m = (m_p - m_q) mod p. */
		bn_sub(m, s, u);
		while (bn_sign(m) == RLC_NEG) {
			bn_add(m, m, prv->p);
		}
		bn_mod(m, m, prv->p);
		/* m1 = qInv(m_p - m_q) mod p. */
		bn_mul(m, m, prv->qi);
		bn_mod(m, m, prv->p);
		/* m = m2 + m1 * q. */
		bn_mul(m, m, prv->q);
		bn_add(m, m, u);
		bn_mod(m, m, prv->n);
#endif
	} RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(s);
		bn_free(t);
		bn_free(u);
		bn_free(v);
	}

	return result;
}
