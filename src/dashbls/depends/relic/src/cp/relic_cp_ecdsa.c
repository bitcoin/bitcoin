/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2009 RELIC Authors
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
 * Implementation of the ECDSA protocol.
 *
 * @ingroup cp
 */

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_ecdsa_gen(bn_t d, ec_t q) {
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

int cp_ecdsa_sig(bn_t r, bn_t s, uint8_t *msg, int len, int hash, bn_t d) {
	bn_t n, k, x, e;
	ec_t p;
	uint8_t h[RLC_MD_LEN];
	int result = RLC_OK;

	bn_null(n);
	bn_null(k);
	bn_null(x);
	bn_null(e);
	ec_null(p);

	RLC_TRY {
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
				len = RLC_MD_LEN;
			}
			if (8 * len > bn_bits(n)) {
				len = RLC_CEIL(bn_bits(n), 8);
				bn_read_bin(e, msg, len);
				bn_rsh(e, e, 8 * len - bn_bits(n));
			} else {
				bn_read_bin(e, msg, len);
			}

			bn_mul(s, d, r);
			bn_mod(s, s, n);
			bn_add(s, s, e);
			bn_mod(s, s, n);
			bn_mod_inv(k, k, n);
			bn_mul(s, s, k);
			bn_mod(s, s, n);
		} while (bn_is_zero(s));
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
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
	uint8_t h[RLC_MD_LEN];
	int cmp, result = 0;

	bn_null(n);
	bn_null(k);
	bn_null(e);
	bn_null(v);
	ec_null(p);

	RLC_TRY {
		bn_new(n);
		bn_new(e);
		bn_new(v);
		bn_new(k);
		ec_new(p);

		ec_curve_get_ord(n);

		if (bn_sign(r) == RLC_POS && bn_sign(s) == RLC_POS &&
				!bn_is_zero(r) && !bn_is_zero(s) && ec_on_curve(q)) {
			if (bn_cmp(r, n) == RLC_LT && bn_cmp(s, n) == RLC_LT) {
				bn_mod_inv(k, s, n);

				if (!hash) {
					md_map(h, msg, len);
					msg = h;
					len = RLC_MD_LEN;
				}

				if (8 * len > bn_bits(n)) {
					len = RLC_CEIL(bn_bits(n), 8);
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

				cmp = dv_cmp_const(v->dp, r->dp, RLC_MIN(v->used, r->used));
				result = (cmp == RLC_NE ? 0 : 1);

				if (v->used != r->used) {
					result = 0;
				}

				if (ec_is_infty(p)) {
					result = 0;
				}
			}
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(e);
		bn_free(v);
		bn_free(k);
		ec_free(p);
	}
	return result;
}
