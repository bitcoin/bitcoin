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
 * @defgroup bc Block ciphers
 */

/**
 * @file
 *
 * Interface of the module for encrypting with block ciphers.
 *
 * @ingroup bc
 */

#ifndef RELIC_BC_H
#define RELIC_BC_H

#include "relic_conf.h"
#include "relic_types.h"
#include "relic_label.h"

/*============================================================================*/
/* Constant definitions                                                       */
/*============================================================================*/

/**
 * Length in bytes of default block cipher length.
 */
#define BC_LEN					16

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
 * @param[in] key_len		- the key size in bits.
 * @return STS_OK if no errors occurred, STS_ERR otherwise.
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
 * @param[in] key_len		- the key size in bits.
 * @return STS_OK if no errors occurred, STS_ERR otherwise.
 */
int bc_aes_cbc_dec(uint8_t *out, int *out_len, uint8_t *in,
		int in_len, uint8_t *key, int key_len, uint8_t *iv);

#endif /* !RELIC_BC_H */
