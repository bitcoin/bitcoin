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
 * Source: "Efficient Group Signature for Large Groups"
 * Authors: Jan Camenisch, Markus Stadler
 */

int cp_sokdl_sig(bn_t c, bn_t s, uint8_t *msg, int len, ec_t y, bn_t x) {
	bn_t n, r;
	ec_t t;
	uint8_t h[RLC_MD_LEN];
	uint8_t *buf, *m = RLC_ALLOCA(uint8_t, len + 3 * (RLC_FC_BYTES + 1));
	int l, result = RLC_OK;

	bn_null(n);
	bn_null(r);
	ec_null(t);

	RLC_TRY {
		bn_new(n);
		bn_new(r);
		ec_new(t);
		if (m == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}

		buf = m;
		ec_curve_get_ord(n);
		ec_curve_get_gen(t);
		memcpy(buf, msg, len);
		buf += len;
		l = ec_size_bin(t, 1);
		ec_write_bin(buf, l, t, 1);
		buf += l;
		l = ec_size_bin(y, 1);
		ec_write_bin(buf, l, y, 1);
		buf += l;

		bn_rand_mod(r, n);
		ec_mul_gen(t, r);
		l = ec_size_bin(t, 1);
		ec_write_bin(buf, l, t, 1);
		buf += l;

		md_map(h, m, len + 3 * (RLC_FC_BYTES + 1));
		bn_read_bin(c, h, RLC_MD_LEN);
		bn_mod(c, c, n);

		bn_mul(s, c, x);
		bn_sub(s, r, s);
		bn_mod(s, s, n);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(r);
		ec_free(t);
		RLC_FREE(m);
	}
	return result;
}

int cp_sokdl_ver(bn_t c, bn_t s, uint8_t *msg, int len, ec_t y) {
	bn_t n, v;
	ec_t t;
	uint8_t h[RLC_MD_LEN];
	uint8_t *buf, *m = RLC_ALLOCA(uint8_t, len + 3 * (RLC_FC_BYTES + 1));
	int l, result = 0;

	bn_null(n);
	bn_null(v);
	ec_null(t);

	RLC_TRY {
		bn_new(n);
		bn_new(v);
		ec_new(t);
		if (m == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}

		buf = m;
		ec_curve_get_ord(n);
		ec_curve_get_gen(t);
		memcpy(buf, msg, len);
		buf += len;
		l = ec_size_bin(t, 1);
		ec_write_bin(buf, l, t, 1);
		buf += l;
		l = ec_size_bin(y, 1);
		ec_write_bin(buf, l, y, 1);
		buf += l;

		/* Compute t' = [r]G + [c]Y. */
		ec_mul_sim_gen(t, s, y, c);
		l = ec_size_bin(t, 1);
		ec_write_bin(buf, l, t, 1);

		md_map(h, m, len + 3 * (RLC_FC_BYTES + 1));
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
		RLC_FREE(m);
	}
	return result;
}

int cp_sokor_sig(bn_t c[2], bn_t s[2], uint8_t *msg, int len, ec_t y[2],
		bn_t x, int first) {
	bn_t n, v[2], z;
	ec_t g, t[2];
	uint8_t h[RLC_MD_LEN];
	uint8_t *buf, *m = RLC_ALLOCA(uint8_t, len + 6 * (RLC_FC_BYTES + 1));
	int l, result = RLC_OK;
	int one = 1 ^ first;
	int zero = 0 ^ first;

	bn_null(n);
	bn_null(v[0]);
	bn_null(v[1]);
	bn_null(z);
	ec_null(g);
	ec_null(t[0]);
	ec_null(t[1]);

	RLC_TRY {
		bn_new(n);
		bn_new(v[0]);
		bn_new(v[1]);
		bn_new(z);
		ec_new(g);
		ec_new(t[0]);
		ec_new(t[1]);
		if (m == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}

		buf = m;
		ec_curve_get_ord(n);
		ec_curve_get_gen(g);
		bn_rand_mod(c[zero], n);
		memcpy(buf, msg, len);
		buf += len;

		bn_rand_mod(v[0], n);
		bn_rand_mod(v[1], n);
		if (first) {
			ec_mul_gen(t[0], v[0]);
			ec_mul_sim_gen(t[1], v[1], y[1], c[zero]);
		} else {
			/* T1 = [w]Y1 + [v1]G. */
			ec_mul_sim_gen(t[0], v[0], y[0], c[zero]);
			/* T2 = [v2]G. */
			ec_mul_gen(t[1], v[1]);
		}

		for (int i = 0; i < 2; i++) {
			l = ec_size_bin(g, 1);
			ec_write_bin(buf, l, g, 1);
			buf += l;
			l = ec_size_bin(y[i], 1);
			ec_write_bin(buf, l, y[i], 1);
			buf += l;
			l = ec_size_bin(t[i], 1);
			ec_write_bin(buf, l, t[i], 1);
			buf += l;
		}
		md_map(h, m, len + 6 * (RLC_FC_BYTES + 1));
		bn_read_bin(z, h, RLC_MD_LEN);
		bn_mod(z, z, n);

		bn_sub(c[one], z, c[zero]);
		bn_mod(c[one], c[one], n);

		bn_copy(s[zero], v[zero]);
		bn_mul(s[one], c[one], x);
		bn_sub(s[one], v[one], s[one]);
		bn_mod(s[one], s[one], n);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(v[0]);
		bn_free(v[1]);
		bn_free(z);
		ec_free(g);
		ec_free(t[0]);
		ec_free(t[1]);
		RLC_FREE(m);
	}
	return result;
}

int cp_sokor_ver(bn_t c[2], bn_t s[2], uint8_t *msg, int len, ec_t y[2]) {
	bn_t n, v[2], z;
	ec_t t;
	uint8_t h[RLC_MD_LEN];
	uint8_t *buf, *m = RLC_ALLOCA(uint8_t, len + 6 * (RLC_FC_BYTES + 1));
	int l, result = 0;

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
		if (m == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}

		buf = m;
		ec_curve_get_ord(n);
		memcpy(buf, msg, len);
		buf += len;

		for (int i = 0; i < 2; i++) {
			ec_curve_get_gen(t);
			l = ec_size_bin(t, 1);
			ec_write_bin(buf, l, t, 1);
			buf += l;
			l = ec_size_bin(y[i], 1);
			ec_write_bin(buf, l, y[i], 1);
			buf += l;

			ec_mul_sim_gen(t, s[i], y[i], c[i]);
			l = ec_size_bin(t, 1);
			ec_write_bin(buf, l, t, 1);
			buf += l;
		}
		md_map(h, m, len + 6 * (RLC_FC_BYTES + 1));
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
		RLC_FREE(m);
	}
	return result;
}
