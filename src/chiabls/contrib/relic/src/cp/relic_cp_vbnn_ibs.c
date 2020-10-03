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
 * Implementation of the vBNN-IBS idenenty-based signature algorithm.
 *
 * Paper: "IMBAS: Identity-based multi-user broadcast authentication in wireless sensor networks"
 *
 * @version $Id$
 * @ingroup cp
 */

#include "relic.h"
#include "relic_test.h"
#include "relic_bench.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_vbnn_ibs_kgc_gen(vbnn_ibs_kgc_t kgc) {
	int result = STS_OK;

	/* order of the ECC group */
	bn_t n;

	/* zero variables */
	bn_null(n);

	TRY {
		/* initialize variables */
		bn_new(n);

		/* get order of ECC group */
		ec_curve_get_ord(n);

		/* calculate master secret key */
		bn_rand_mod(kgc->msk, n);

		/* calculate master public key */
		ec_mul_gen(kgc->mpk, kgc->msk);
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		/* free variables */
		bn_free(n);
	}
	return result;
}

int cp_vbnn_ibs_kgc_extract_key(vbnn_ibs_user_t user, vbnn_ibs_kgc_t kgc, uint8_t *identity, int identity_len) {
	int result = STS_OK;

	uint8_t *buffer_id_and_R;
	int buffer_id_and_R_size;
	int len;
	uint8_t hash[MD_LEN];

	/* order of the ECC group */
	bn_t n;

	bn_t r;

	/* zero variables */
	bn_null(n);
	bn_null(r);
	buffer_id_and_R = NULL;

	TRY {
		/* initialize variables */
		bn_new(n);
		bn_new(r);

		/* get order of ECC group */
		ec_curve_get_ord(n);

		/* extract user key from identity */
		bn_rand_mod(r, n);

		/* calculate R part of the user key */
		ec_mul_gen(user->R, r);

		/* calculate s part of the user key */
		buffer_id_and_R_size = identity_len + ec_size_bin(user->R, 1);
		len = buffer_id_and_R_size;
		buffer_id_and_R = (uint8_t*)malloc(buffer_id_and_R_size);
		memcpy(buffer_id_and_R, identity, identity_len);
		ec_write_bin(buffer_id_and_R + identity_len, ec_size_bin(user->R, 1), user->R, 1);

		md_map(hash, buffer_id_and_R, len);
		len = MD_LEN;

		if (8 * len > bn_bits(n)) {
			len = CEIL(bn_bits(n), 8);
			bn_read_bin(user->s, hash, len);
			bn_rsh(user->s, user->s, 8 * len - bn_bits(n));
		} else {
			bn_read_bin(user->s, hash, len);
		}

		bn_mul(user->s, user->s, kgc->msk);
		bn_add(user->s, user->s, r);
		bn_mod(user->s, user->s, n);
	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		/* free variables */
		bn_free(n);
		bn_free(r);
		free(buffer_id_and_R);
	}
	return result;
}

int cp_vbnn_ibs_user_sign(ec_t sig_R, bn_t sig_z, bn_t sig_h, uint8_t *identity, int identity_len, uint8_t *msg, int msg_len, vbnn_ibs_user_t user) {
	int result = STS_OK;

	uint8_t *buffer_id_and_message_and_R_and_Y = NULL;
	uint8_t *buffer_i = NULL;
	int buffer_id_and_message_and_R_and_Y_size;
	int len;
	uint8_t hash[MD_LEN];

	/* order of the ECC group */
	bn_t n;
	bn_t y;
	ec_t Y;

	/* zero variables */
	bn_null(n);
	bn_null(y);
	ec_null(Y);

	TRY {
		bn_new(n);
		bn_new(y);
		ec_new(Y);

		/* get order of ECC group */
		ec_curve_get_ord(n);

		bn_rand_mod(y, n);
		ec_mul_gen(Y, y);

		/* calculate h part of the signature */
		buffer_id_and_message_and_R_and_Y_size = identity_len + msg_len + ec_size_bin(Y, 1) + ec_size_bin(user->R, 1);
		len = buffer_id_and_message_and_R_and_Y_size;
		buffer_id_and_message_and_R_and_Y = (uint8_t*)malloc(buffer_id_and_message_and_R_and_Y_size);
		buffer_i = buffer_id_and_message_and_R_and_Y;

		memcpy(buffer_i, identity, identity_len);
		buffer_i += identity_len;

		memcpy(buffer_i, msg, msg_len);
		buffer_i += msg_len;

		ec_write_bin(buffer_i, ec_size_bin(user->R, 1), user->R, 1);
		buffer_i += ec_size_bin(user->R, 1);

		ec_write_bin(buffer_i, ec_size_bin(Y, 1), Y, 1);

		md_map(hash, buffer_id_and_message_and_R_and_Y, len);
		len = MD_LEN;

		if (8 * len > bn_bits(n)) {
			len = CEIL(bn_bits(n), 8);
			bn_read_bin(sig_h, hash, len);
			bn_rsh(sig_h, sig_h, 8 * len - bn_bits(n));
		} else {
			bn_read_bin(sig_h, hash, len);
		}

		/* calculate z part of the signature */
		bn_mul(sig_z, sig_h, user->s);
		bn_add(sig_z, sig_z, y);
		bn_mod(sig_z, sig_z, n);

		/* calculate R part of the signature */
		ec_copy(sig_R, user->R);

	}
	CATCH_ANY {
		result = STS_ERR;
	}
	FINALLY {
		/* free variables */
		bn_free(n);
		bn_free(y);
		ec_free(Y);
		free(buffer_id_and_message_and_R_and_Y);
	}
	return result;
}

int cp_vbnn_ibs_user_verify(ec_t sig_R, bn_t sig_z, bn_t sig_h, uint8_t *identity, int identity_len, uint8_t *msg, int msg_len, ec_t mpk) {
	int result = 0;

	uint8_t *buffer_hash;
	uint8_t *buffer_i;
	int buffer_hash_size;
	int len;
	uint8_t hash[MD_LEN];

	/* order of the ECC group */
	bn_t n;
	bn_t c;
	bn_t h_verify;
	ec_t Z;
	ec_t tmp;

	/* zero variables */
	bn_null(n);
	bn_null(c);
	bn_null(h_verify);
	ec_null(Z);
	ec_null(tmp);

	TRY {
		bn_new(n);
		bn_new(c);
		bn_new(h_verify);
		ec_new(Z);
		ec_new(tmp);

		/* get order of ECC group */
		ec_curve_get_ord(n);

		/* calculate c */
		buffer_hash_size = identity_len + ec_size_bin(sig_R, 1);
		len = buffer_hash_size;
		buffer_hash = (uint8_t*)malloc(buffer_hash_size);
		buffer_i = buffer_hash;

		memcpy(buffer_i, identity, identity_len);
		buffer_i += identity_len;

		ec_write_bin(buffer_i, ec_size_bin(sig_R, 1), sig_R, 1);

		md_map(hash, buffer_hash, len);
		len = MD_LEN;

		if (8 * len > bn_bits(n)) {
			len = CEIL(bn_bits(n), 8);
			bn_read_bin(c, hash, len);
			bn_rsh(c, c, 8 * len - bn_bits(n));
		} else {
			bn_read_bin(c, hash, len);
		}
		free(buffer_hash);
		buffer_hash = NULL;

		/* calculate Z */
		ec_mul_gen(Z, sig_z);
		ec_mul(tmp, mpk, c);
		ec_add(tmp, tmp, sig_R);
		ec_mul(tmp, tmp, sig_h);
		ec_sub(Z, Z, tmp);


		/* calculate h_verify */
		buffer_hash_size = identity_len + msg_len + ec_size_bin(sig_R, 1) + ec_size_bin(Z, 1);
		len = buffer_hash_size;
		buffer_hash = (uint8_t*)malloc(buffer_hash_size);
		buffer_i = buffer_hash;

		memcpy(buffer_i, identity, identity_len);
		buffer_i += identity_len;
		memcpy(buffer_i, msg, msg_len);
		buffer_i += msg_len;
		ec_write_bin(buffer_i, ec_size_bin(sig_R, 1), sig_R, 1);
		buffer_i += ec_size_bin(sig_R, 1);
		ec_write_bin(buffer_i, ec_size_bin(Z, 1), Z, 1);

		md_map(hash, buffer_hash, len);
		len = MD_LEN;

		if (8 * len > bn_bits(n)) {
			len = CEIL(bn_bits(n), 8);
			bn_read_bin(h_verify, hash, len);
			bn_rsh(h_verify, h_verify, 8 * len - bn_bits(n));
		} else {
			bn_read_bin(h_verify, hash, len);
		}

		if (bn_cmp(sig_h, h_verify) == CMP_EQ) {
			result = 1;
		} else {
			result = 0;
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		/* free variables */
		bn_free(n);
		bn_free(c);
		bn_free(h_verify);
		ec_free(Z);
		ec_free(tmp);
	}
	return result;
}
