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
 * Implementation of a Mask Generation Function.
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

void md_mgf(uint8_t *key, int key_len, const uint8_t *in,
		int in_len) {
	uint32_t i, j, d;
	uint8_t *buffer = RLC_ALLOCA(uint8_t, in_len + sizeof(uint32_t));
	uint8_t *t = RLC_ALLOCA(uint8_t, key_len + RLC_MD_LEN);

	if (buffer == NULL || t == NULL) {
		RLC_FREE(buffer);
		RLC_FREE(t);
		RLC_THROW(ERR_NO_MEMORY);
		return;
	}

	/* d = ceil(kLen/hLen). */
	d = RLC_CEIL(key_len, RLC_MD_LEN);
	memcpy(buffer, in, in_len);
	for (i = 0; i < d; i++) {
		j = util_conv_big(i);
		/* c = integer_to_string(c, 4). */
		memcpy(buffer + in_len, &j, sizeof(uint32_t));
		/* t = t || hash(z || c). */
		md_map(t + i * RLC_MD_LEN, buffer, in_len + sizeof(uint32_t));
	}
	memcpy(key, t, key_len);

	RLC_FREE(buffer);
	RLC_FREE(t);
}
