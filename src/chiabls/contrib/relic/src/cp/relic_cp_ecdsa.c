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
 * Implementation of the ECDSA protocol.
 *
 * @ingroup cp
 */

#include "relic.h"
#include "relic_test.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_ecdsa_gen(bn_t d, ec_t q) {
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

int cp_ecdsa_sig(bn_t r, bn_t s, uint8_t *msg, int len, int hash, bn_t d) {
	bn_t n, k, x, e;
	ec_t p;
	uint8_t h[MD_LEN];
	int result = STS_OK;

	bn_null(n);
	bn_null(k);
	bn_null(x);
	bn_null(e);
	ec_null(p);

	TRY {
		bn_new(n);
		bn_new(k);
		bn_new(x);
		bn_new(e);
		ec_new(p);

		ec_curve_get_ord(n);
		do {
			do {
				bn_rand_mod(k, n);
				ec_mul_gen(p, k);
				ec_get_x(x, p);
				bn_mod(r, x, n);
			} while (bn_is_zero(r));

			if (!hash) {
				md_map(h, msg, len);
				msg = h;
				len = MD_LEN;
			}
			if (8 * len > bn_bits(n)) {
				len = CEIL(bn_bits(n), 8);
				bn_read_bin(e, msg, len);
				bn_rsh(e, e, 8 * len - bn_bits(n));
			} else {
				bn_read_bin(e, msg, len);
			}

			bn_mul(s, d, r);
			bn_mod(s, s, n);
			bn_add(s, s, e);
			bn_mod(s, s, n);
			bn_gcd_ext(x, k, NULL, k, n);
			if (bn_sign(k) == BN_NEG) {
				bn_add(k, k, n);
			}
			bn_mul(s, s, k);
			bn_mod(s, s, n);
		} while (bn_is_zero(s));
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		bn_free(n);
		bn_free(k);
		bn_free(x);
		bn_free(e);
		ec_free(p);
	}
	return result;
}

int cp_ecdsa_ver(bn_t r, bn_t s, uint8_t *msg, int len, int hash, ec_t q) {
	bn_t n, k, e, v;
	ec_t p;
	uint8_t h[MD_LEN];
	int result = 0;

	bn_null(n);
	bn_null(k);
	bn_null(e);
	bn_null(v);
	ec_null(p);

	TRY {
		bn_new(n);
		bn_new(e);
		bn_new(v);
		bn_new(k);
		ec_new(p);

		ec_curve_get_ord(n);

		if (bn_sign(r) == BN_POS && bn_sign(s) == BN_POS &&
				!bn_is_zero(r) && !bn_is_zero(s)) {
			if (bn_cmp(r, n) == CMP_LT && bn_cmp(s, n) == CMP_LT) {
				bn_gcd_ext(e, k, NULL, s, n);
				if (bn_sign(k) == BN_NEG) {
					bn_add(k, k, n);
				}

				if (!hash) {
					md_map(h, msg, len);
					msg = h;
					len = MD_LEN;
				}
				if (8 * len > bn_bits(n)) {
					len = CEIL(bn_bits(n), 8);
					bn_read_bin(e, msg, len);
					bn_rsh(e, e, 8 * len - bn_bits(n));
				} else {
					bn_read_bin(e, msg, len);
				}

				bn_mul(e, e, k);
				bn_mod(e, e, n);
				bn_mul(v, r, k);
				bn_mod(v, v, n);

				ec_mul_sim_gen(p, e, q, v);
				ec_get_x(v, p);

				bn_mod(v, v, n);

				result = dv_cmp_const(v->dp, r->dp, MIN(v->used, r->used));
				result = (result == CMP_NE ? 0 : 1);

				if (v->used != r->used) {
					result = 0;
				}

				if (ec_is_infty(p)) {
					result = 0;
				}
			}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(n);
		bn_free(e);
		bn_free(v);
		bn_free(k);
		ec_free(p);
	}
	return result;
}
