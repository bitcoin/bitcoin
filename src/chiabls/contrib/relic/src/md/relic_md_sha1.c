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
 * Implementation of the SHA-1 hash function.
 *
 * @ingroup md
 */

#include <string.h>

#include "relic_conf.h"
#include "relic_core.h"
#include "relic_util.h"
#include "relic_md.h"
#include "sha.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if MD_MAP == SHONE || !defined(STRIP)

void md_map_shone(uint8_t *hash, const uint8_t *msg, int len) {
	SHA1Context ctx;

	if (SHA1Reset(&ctx) != shaSuccess) {
		THROW(ERR_NO_VALID);
	}
	if (SHA1Input(&ctx, msg, len) != shaSuccess) {
		THROW(ERR_NO_VALID);
	}
	if (SHA1Result(&ctx, hash) != shaSuccess) {
		THROW(ERR_NO_VALID);
	}
}

#endif

#if RAND == FIPS

void md_map_shone_mid(uint8_t *state, uint8_t *msg, int len) {
	SHA1Context ctx;
	uint8_t dummy[64 - MD_LEN_SHONE] = { 0 };

	if (SHA1Reset(&ctx) != shaSuccess) {
		THROW(ERR_NO_VALID);
	}

	if (SHA1Input(&ctx, msg, len) != shaSuccess) {
		THROW(ERR_NO_VALID);
	}
	if (SHA1Input(&ctx, dummy, sizeof(dummy)) != shaSuccess) {
		THROW(ERR_NO_VALID);
	}

	uint32_t t[5];
	for (int i = 0; i < 5; i++) {
		t[i] = util_conv_big(ctx.Intermediate_Hash[i]);
	}

	memcpy(state, t, sizeof(t));
}

#endif
