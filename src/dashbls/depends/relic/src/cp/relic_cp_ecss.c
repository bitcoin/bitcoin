/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2011 RELIC Authors
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
 * Implementation of the Schnorr protocol.
 *
 * @ingroup cp
 */

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_ecss_gen(bn_t d, ec_t q) {
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

int cp_ecss_sig(bn_t e, bn_t s, uint8_t *msg, int len, bn_t d) {
	bn_t n, k, x, r;
	ec_t p;
	uint8_t hash[RLC_MD_LEN];
	uint8_t *m = RLC_ALLOCA(uint8_t, len + RLC_FC_BYTES);
	int result = RLC_OK;

	bn_null(n);
	bn_null(k);
	bn_null(x);
	bn_null(r);
	ec_null(p);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		bn_new(x);
		bn_new(r);
		ec_new(p);
		if (m == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}

		ec_curve_get_ord(n);
		do {
			bn_rand_mod(k, n);
			ec_mul_gen(p, k);
			ec_get_x(x, p);
			bn_mod(r, x, n);
		} while (bn_is_zero(r));

		memcpy(m, msg, len);
		bn_write_bin(m + len, RLC_FC_BYTES, r);
		md_map(hash, m, len + RLC_FC_BYTES);

		if (8 * RLC_MD_LEN > bn_bits(n)) {
			len = RLC_CEIL(bn_bits(n), 8);
			bn_read_bin(e, hash, len);
			bn_rsh(e, e, 8 * RLC_MD_LEN - bn_bits(n));
		} else {
			bn_read_bin(e, hash, RLC_MD_LEN);
		}

		bn_mod(e, e, n);

		bn_mul(s, d, e);
		bn_mod(s, s, n);
		bn_sub(s, n, s);
		bn_add(s, s, k);
		bn_mod(s, s, n);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(k);
		bn_free(x);
		bn_free(r);
		ec_free(p);
		RLC_FREE(m);
	}
	return result;
}

int cp_ecss_ver(bn_t e, bn_t s, uint8_t *msg, int len, ec_t q) {
	bn_t n, ev, rv;
	ec_t p;
	uint8_t hash[RLC_MD_LEN];
	uint8_t *m = RLC_ALLOCA(uint8_t, len + RLC_FC_BYTES);
	int result = 0;

	bn_null(n);
	bn_null(ev);
	bn_null(rv);
	ec_null(p);

	RLC_TRY {
		bn_new(n);
		bn_new(ev);
		bn_new(rv);
		ec_new(p);
		if (m == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}

		ec_curve_get_ord(n);

		if (bn_sign(e) == RLC_POS && bn_sign(s) == RLC_POS && !bn_is_zero(s)) {
			if (bn_cmp(e, n) == RLC_LT && bn_cmp(s, n) == RLC_LT) {
				ec_mul_sim_gen(p, s, q, e);
				ec_get_x(rv, p);

				bn_mod(rv, rv, n);

				memcpy(m, msg, len);
				bn_write_bin(m + len, RLC_FC_BYTES, rv);
				md_map(hash, m, len + RLC_FC_BYTES);

				if (8 * RLC_MD_LEN > bn_bits(n)) {
					len = RLC_CEIL(bn_bits(n), 8);
					bn_read_bin(ev, hash, len);
					bn_rsh(ev, ev, 8 * RLC_MD_LEN - bn_bits(n));
				} else {
					bn_read_bin(ev, hash, RLC_MD_LEN);
				}

				bn_mod(ev, ev, n);

				result = dv_cmp_const(ev->dp, e->dp, RLC_MIN(ev->used, e->used));
				result = (result == RLC_NE ? 0 : 1);

				if (ev->used != e->used) {
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
		bn_free(ev);
		bn_free(rv);
		ec_free(p);
		RLC_FREE(m);
	}
	return result;
}
