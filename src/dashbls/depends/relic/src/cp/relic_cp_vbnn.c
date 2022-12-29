/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2018 RELIC Authors
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
 * Implementation of the vBNN-IBS idenenty-based signature algorithm.
 *
 * Paper: "IMBAS: id-based multi-user broadcast authentication in wireless sensor networks"
 *
 * @version $Id$
 * @ingroup cp
 */

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_vbnn_gen(bn_t msk, ec_t mpk) {
	int result = RLC_OK;

	/* order of the ECC group */
	bn_t n;

	/* zero variables */
	bn_null(n);

	RLC_TRY {
		/* initialize variables */
		bn_new(n);

		/* get order of ECC group */
		ec_curve_get_ord(n);

		/* calculate master secret key */
		bn_rand_mod(msk, n);

		/* calculate master public key */
		ec_mul_gen(mpk, msk);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		/* free variables */
		bn_free(n);
	}
	return result;
}

int cp_vbnn_gen_prv(bn_t sk, ec_t pk, bn_t msk, uint8_t *id, int id_len) {
	uint8_t hash[RLC_MD_LEN];
	int len, result = RLC_OK;
	uint8_t *buf = NULL;
	bn_t n, r;

	/* zero variables */
	bn_null(n);
	bn_null(r);

	RLC_TRY {
		/* initialize variables */
		bn_new(n);
		bn_new(r);

		/* get order of ECC group */
		ec_curve_get_ord(n);

		/* extract user key from id */
		bn_rand_mod(r, n);

		/* calculate R part of the user key */
		ec_mul_gen(pk, r);

		/* calculate s part of the user key */
		len = id_len + ec_size_bin(pk, 1);
		buf = RLC_ALLOCA(uint8_t, len);
		if (buf == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}
		memcpy(buf, id, id_len);
		ec_write_bin(buf + id_len, ec_size_bin(pk, 1), pk, 1);

		md_map(hash, buf, len);
		bn_read_bin(sk, hash, RLC_MD_LEN);
		bn_mod(sk, sk, n);
		bn_mul(sk, sk, msk);
		bn_add(sk, sk, r);
		bn_mod(sk, sk, n);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		/* free variables */
		bn_free(n);
		bn_free(r);
		RLC_FREE(buf);
	}
	return result;
}

int cp_vbnn_sig(ec_t r, bn_t z, bn_t h, uint8_t *id, int id_len,
		uint8_t *msg, int msg_len, bn_t sk, ec_t pk) {
	int len, result = RLC_OK;
	uint8_t *buf = NULL, *buf_i, hash[RLC_MD_LEN];
	bn_t n, y;
	ec_t t;

	/* zero variables */
	bn_null(n);
	bn_null(y);
	ec_null(t);

	RLC_TRY {
		bn_new(n);
		bn_new(y);
		ec_new(t);

		/* get order of ECC group */
		ec_curve_get_ord(n);

		bn_rand_mod(y, n);
		ec_mul_gen(t, y);

		/* calculate h part of the signature */
		len = id_len + msg_len + ec_size_bin(t, 1) + ec_size_bin(pk, 1);
		buf = RLC_ALLOCA(uint8_t, len);
		if (buf == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}

		buf_i = buf;
		memcpy(buf_i, id, id_len);
		buf_i += id_len;
		memcpy(buf_i, msg, msg_len);
		buf_i += msg_len;
		ec_write_bin(buf_i, ec_size_bin(pk, 1), pk, 1);
		buf_i += ec_size_bin(pk, 1);
		ec_write_bin(buf_i, ec_size_bin(t, 1), t, 1);

		md_map(hash, buf, len);
		bn_read_bin(h, hash, RLC_MD_LEN);
		bn_mod(h, h, n);

		/* calculate z part of the signature */
		bn_mul(z, h, sk);
		bn_add(z, z, y);
		bn_mod(z, z, n);

		/* calculate R part of the signature */
		ec_copy(r, pk);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		/* free variables */
		bn_free(n);
		bn_free(y);
		ec_free(t);
		RLC_FREE(buf);
	}
	return result;
}

int cp_vbnn_ver(ec_t r, bn_t z, bn_t h, uint8_t *id, int id_len,
		uint8_t *msg, int msg_len, ec_t mpk) {
	int len, result = 0;
	uint8_t *buf = NULL, *buf_i, hash[RLC_MD_LEN];
	bn_t n, c, _h;
	ec_t Z;
	ec_t t;

	/* zero variables */
	bn_null(n);
	bn_null(c);
	bn_null(_h);
	ec_null(Z);
	ec_null(t);

	RLC_TRY {
		bn_new(n);
		bn_new(c);
		bn_new(_h);
		ec_new(Z);
		ec_new(t);

		/* calculate c */
		len = id_len + msg_len + 2 * ec_size_bin(r, 1);
		buf = RLC_ALLOCA(uint8_t, len);
		if (buf == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}

		/* get order of ECC group */
		ec_curve_get_ord(n);

		buf_i = buf;
		memcpy(buf_i, id, id_len);
		buf_i += id_len;
		ec_write_bin(buf_i, ec_size_bin(r, 1), r, 1);

		len = id_len + ec_size_bin(r, 1);
		md_map(hash, buf, len);
		bn_read_bin(c, hash, RLC_MD_LEN);
		bn_mod(c, c, n);

		/* calculate Z */
		ec_mul_gen(Z, z);
		ec_mul(t, mpk, c);
		ec_add(t, t, r);
		ec_norm(t, t);
		ec_mul(t, t, h);
		ec_sub(Z, Z, t);
		ec_norm(Z, Z);

		/* calculate h_verify */
		buf_i = buf;
		memcpy(buf_i, id, id_len);
		buf_i += id_len;
		memcpy(buf_i, msg, msg_len);
		buf_i += msg_len;
		ec_write_bin(buf_i, ec_size_bin(r, 1), r, 1);
		buf_i += ec_size_bin(r, 1);
		ec_write_bin(buf_i, ec_size_bin(Z, 1), Z, 1);

		len = id_len + msg_len + ec_size_bin(r, 1) + ec_size_bin(Z, 1);
		md_map(hash, buf, len);
		bn_read_bin(_h, hash, RLC_MD_LEN);
		bn_mod(_h, _h, n);
		RLC_FREE(buf);

		if (bn_cmp(h, _h) == RLC_EQ) {
			result = 1;
		} else {
			result = 0;
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		/* free variables */
		bn_free(n);
		bn_free(c);
		bn_free(_h);
		ec_free(Z);
		ec_free(t);
		RLC_FREE(buf);
	}
	return result;
}
