/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2013 RELIC Authors
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
 * Implementation of the AES block cipher.
 *
 * @ingroup bc
 */

#include <string.h>

#include "relic_core.h"
#include "relic_err.h"
#include "relic_bc.h"
#include "rijndael-api-fst.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int bc_aes_cbc_enc(uint8_t *out, int *out_len, uint8_t *in,
		int in_len, uint8_t *key, int key_len, uint8_t *iv) {
	keyInstance key_inst;
	cipherInstance cipher_inst;

	int pad_len = 16 - (in_len - 16 * (in_len/16));
	if (*out_len < in_len + pad_len) {
		return RLC_ERR;
	}
	if (makeKey2(&key_inst, DIR_ENCRYPT, 8 * key_len, (char *)key) != TRUE) {
		return RLC_ERR;
	}
	if (cipherInit(&cipher_inst, MODE_CBC, NULL) != TRUE) {
		return RLC_ERR;
	}
	memcpy(cipher_inst.IV, iv, RLC_BC_LEN);
	*out_len = padEncrypt(&cipher_inst, &key_inst, in, in_len, out);
	if (*out_len <= 0) {
		return RLC_ERR;
	}
	return RLC_OK;
}

int bc_aes_cbc_dec(uint8_t *out, int *out_len, uint8_t *in,
		int in_len, uint8_t *key, int key_len, uint8_t *iv) {
	keyInstance key_inst;
	cipherInstance cipher_inst;

	if (*out_len < in_len) {
		return RLC_ERR;
	}

	if (makeKey2(&key_inst, DIR_DECRYPT, 8 * key_len, (char *)key) != TRUE) {
		return RLC_ERR;
	}
	if (cipherInit(&cipher_inst, MODE_CBC, NULL) != TRUE) {
		return RLC_ERR;
	}
	memcpy(cipher_inst.IV, iv, RLC_BC_LEN);
	*out_len = padDecrypt(&cipher_inst, &key_inst, in, in_len, out);
	if (*out_len <= 0) {
		return RLC_ERR;
	}
	return RLC_OK;
}
