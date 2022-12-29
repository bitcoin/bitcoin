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
 * @defgroup bc Block ciphers
 */

/**
 * @file
 *
 * Interface of the module for encrypting with block ciphers.
 *
 * @ingroup bc
 */

#ifndef RLC_BC_H
#define RLC_BC_H

#include "relic_conf.h"
#include "relic_types.h"
#include "relic_label.h"

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

/**
 * Length in bytes of the default block cipher length.
 */
#define RLC_BC_LEN					16

/*============================================================================*/
/* Function prototypes                                                        */
/*============================================================================*/

/**
 * Encrypts with AES in CBC mode.
 *
 * @param[out] out			- the resulting ciphertext.
 * @param[in,out] out_len	- the buffer capacity and number of bytes written.
 * @param[in] in			- the bytes to be encrypted.
 * @param[in] in_len		- the number of bytes to encrypt.
 * @param[in] key			- the key.
 * @param[in] key_len		- the key size in bytes.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int bc_aes_cbc_enc(uint8_t *out, int *out_len, uint8_t *in,
		int in_len, uint8_t *key, int key_len, uint8_t *iv);

/**
 * Decrypts with AES in CBC mode.
 *
 * @param[out] out			- the resulting plaintext.
 * @param[in,out] out_len	- the buffer capacity and number of bytes written.
 * @param[in] in			- the bytes to be decrypted.
 * @param[in] in_len		- the number of bytes to decrypt.
 * @param[in] key			- the key.
 * @param[in] key_len		- the key size in bytes.
 * @return RLC_OK if no errors occurred, RLC_ERR otherwise.
 */
int bc_aes_cbc_dec(uint8_t *out, int *out_len, uint8_t *in,
		int in_len, uint8_t *key, int key_len, uint8_t *iv);

#endif /* !RLC_BC_H */
