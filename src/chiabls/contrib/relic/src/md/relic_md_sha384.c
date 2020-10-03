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
 * Implementation of the SHA-384 hash function.
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

#if MD_MAP == SH384 || !defined(STRIP)

void md_map_sh384(uint8_t *hash, const uint8_t *msg, int len) {
	SHA384Context ctx;

	if (SHA384Reset(&ctx) != shaSuccess) {
		THROW(ERR_NO_VALID);
	}
	if (SHA384Input(&ctx, msg, len) != shaSuccess) {
		THROW(ERR_NO_VALID);
	}
	if (SHA384Result(&ctx, hash) != shaSuccess) {
		THROW(ERR_NO_VALID);
	}
}

#endif
