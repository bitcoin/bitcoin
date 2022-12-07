/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2021 RELIC Authors
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
 * Implementation of protocols for proof of knowledge of discrete logarithms.
 *
 * @ingroup cp
 */

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

/*
 * Source: "Proof Systems for General Statements about Discrete Logarithms"
 * Authors: Jan Camenisc, Markus Stadler (1997)
 */

int cp_pokdl_prv(bn_t c, bn_t r, ec_t y, bn_t x) {
	bn_t n, v;
	ec_t t;
	int l, result = RLC_OK;
	uint8_t *buf, bin[3 * (RLC_FC_BYTES + 1)] = { 0 }, h[RLC_MD_LEN];

	bn_null(n);
	bn_null(v);
	ec_null(t);

	RLC_TRY {
		bn_new(n);
		bn_new(v);
		ec_new(t);

		buf = bin;
		ec_curve_get_ord(n);
		ec_curve_get_gen(t);
		l = ec_size_bin(t, 1);
		ec_write_bin(buf, l, t, 1);
		buf += l;
		l = ec_size_bin(y, 1);
		ec_write_bin(buf, l, y, 1);
		buf += l;

		bn_rand_mod(v, n);
		ec_mul_gen(t, v);
		l = ec_size_bin(t, 1);
		ec_write_bin(buf, l, t, 1);
		md_map(h, bin, sizeof(bin));
		bn_read_bin(c, h, RLC_MD_LEN);
		bn_mod(c, c, n);
		bn_mul(r, c, x);
		bn_sub(r, v, r);
		bn_mod(r, r, n);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(v);
		ec_free(t);
	}
	return result;
}

int cp_pokdl_ver(bn_t c, bn_t r, ec_t y) {
	bn_t n, v;
	ec_t t;
	int l, result = 0;
	uint8_t *buf, bin[3 * (RLC_FC_BYTES + 1)] = { 0 }, h[RLC_MD_LEN];

	bn_null(n);
	bn_null(v);
	ec_null(t);

	RLC_TRY {
		bn_new(n);
		bn_new(v);
		ec_new(t);

		buf = bin;
		ec_curve_get_ord(n);
		ec_curve_get_gen(t);
		l = ec_size_bin(t, 1);
		ec_write_bin(buf, l, t, 1);
		buf += l;
		l = ec_size_bin(y, 1);
		ec_write_bin(buf, l, y, 1);
		buf += l;

		/* Compute t' = [r]G + [c]Y. */
		ec_mul_sim_gen(t, r, y, c);
		l = ec_size_bin(t, 1);
		ec_write_bin(buf, l, t, 1);
		md_map(h, bin, sizeof(bin));
		bn_read_bin(v, h, RLC_MD_LEN);
		bn_mod(v, v, n);
		if (bn_cmp(v, c) == RLC_EQ) {
			result = 1;
		}
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(v);
		ec_free(t);
	}
	return result;
}

int cp_pokor_prv(bn_t c[2], bn_t r[2], ec_t y[2], bn_t x) {
	bn_t n, v[2], z;
	ec_t t;
	int l, result = RLC_OK;
	uint8_t *buf, bin[6 * (RLC_FC_BYTES + 1)] = { 0 }, h[RLC_MD_LEN];

	bn_null(n);
	bn_null(v[0]);
	bn_null(v[1]);
	bn_null(z);
	ec_null(t);

	RLC_TRY {
		bn_new(n);
		bn_new(v[0]);
		bn_new(v[1]);
		bn_new(z);
		ec_new(t);

		buf = bin;
		ec_curve_get_ord(n);
		bn_rand_mod(c[0], n);

		for (int i = 0; i < 2; i++) {
			bn_rand_mod(v[i], n);
			ec_curve_get_gen(t);
			l = ec_size_bin(t, 1);
			ec_write_bin(buf, l, t, 1);
			buf += l;
			l = ec_size_bin(y[i], 1);
			ec_write_bin(buf, l, y[i], 1);
			buf += l;

			if (i == 0) {
				/* T1 = [w]Y1 + [v1]G. */
				ec_mul_sim_gen(t, v[i], y[i], c[i]);
			} else {
				/* T2 = [v2]G. */
				ec_mul_gen(t, v[i]);
			}
			l = ec_size_bin(t, 1);
			ec_write_bin(buf, l, t, 1);
			buf += l;
		}
		md_map(h, bin, sizeof(bin));
		bn_read_bin(z, h, RLC_MD_LEN);
		bn_mod(z, z, n);

		bn_sub(c[1], z, c[0]);
		bn_mod(c[1], c[1], n);

		bn_copy(r[0], v[0]);
		bn_mul(r[1], c[1], x);
		bn_sub(r[1], v[1], r[1]);
		bn_mod(r[1], r[1], n);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(v[0]);
		bn_free(v[1]);
		bn_free(z);
		ec_free(t);
	}
	return result;
}

int cp_pokor_ver(bn_t c[2], bn_t r[2], ec_t y[2]) {
	bn_t n, v[2], z;
	ec_t t;
	int l, result = 0;
	uint8_t *buf, bin[6 * (RLC_FC_BYTES + 1)] = { 0 }, h[RLC_MD_LEN];

	bn_null(n);
	bn_null(v[0]);
	bn_null(v[1]);
	bn_null(z);
	ec_null(t);

	RLC_TRY {
		bn_new(n);
		bn_new(v[0]);
		bn_new(v[1]);
		bn_new(z);
		ec_new(t);

		buf = bin;
		ec_curve_get_ord(n);

		for (int i = 0; i < 2; i++) {
			ec_curve_get_gen(t);
			l = ec_size_bin(t, 1);
			ec_write_bin(buf, l, t, 1);
			buf += l;
			l = ec_size_bin(y[i], 1);
			ec_write_bin(buf, l, y[i], 1);
			buf += l;

			ec_mul_sim_gen(t, r[i], y[i], c[i]);
			l = ec_size_bin(t, 1);
			ec_write_bin(buf, l, t, 1);
			buf += l;
		}
		md_map(h, bin, sizeof(bin));
		bn_read_bin(z, h, RLC_MD_LEN);
		bn_mod(z, z, n);

		bn_sub(z, z, c[0]);
		bn_sub(z, z, c[1]);
		bn_mod(z, z, n);

		if (bn_is_zero(z)) {
			result = 1;
		}
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(v[0]);
		bn_free(v[1]);
		bn_free(z);
		ec_free(t);
	}
	return result;
}
