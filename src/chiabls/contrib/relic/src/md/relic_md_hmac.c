/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2015 RELIC Authors
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
 * Implementation of Hash-based Message Authentication Code.
 *
 * @ingroup md
 */

#include <string.h>

#include "relic_conf.h"
#include "relic_core.h"
#include "relic_util.h"
#include "relic_md.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void md_hmac(uint8_t *mac, const uint8_t *in, int in_len, const uint8_t *key,
		int key_len) {
#if MD_MAP == SHONE || MD_MAP == SH224 || MD_MAP == SH256 || MD_MAP == BLAKE2S_160 || MD_MAP == BLAKE2S_256
	int block_size = 64;
#elif MD_MAP == SH384 || MD_MAP == SH512
	int block_size = 128;
#endif
	uint8_t opad[block_size + MD_LEN], ipad[block_size + in_len];
	uint8_t _key[MAX(MD_LEN, block_size)];

	if (key_len > block_size) {
		md_map(_key, key, key_len);
		key = _key;
		key_len = MD_LEN;
	}
	if (key_len <= block_size) {
		memcpy(_key, key, key_len);
		memset(_key + key_len, 0, block_size - key_len);
		key = _key;
	}
	for (int i = 0; i < block_size; i++) {
		opad[i] = 0x5C ^ key[i];
		ipad[i] = 0x36 ^ key[i];
	}
	memcpy(ipad + block_size, in, in_len);
	md_map(opad + block_size, ipad, block_size + in_len);
	md_map(mac, opad, block_size + MD_LEN);
}
