/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2009 RELIC Authors
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
 * Implementation of the SHA-256 hash function.
 *
 * @ingroup md
 */

#include "relic_conf.h"
#include "relic_core.h"
#include "relic_md.h"
#include "sha.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if MD_MAP == SH256 || !defined(STRIP)

void md_map_sh256(uint8_t *hash, const uint8_t *msg, int len) {
	SHA256Context ctx;

	if (SHA256Reset(&ctx) != shaSuccess) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}
	if (SHA256Input(&ctx, msg, len) != shaSuccess) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}
	if (SHA256Result(&ctx, hash) != shaSuccess) {
		RLC_THROW(ERR_NO_VALID);
		return;
	}
}

#endif
