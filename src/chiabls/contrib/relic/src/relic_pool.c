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
 * Implementation of the memory-management routines for manipulating the
 * global pool of digit vectors used by the static allocation mode.
 *
 * @ingroup relic
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "relic_core.h"
#include "relic_conf.h"
#include "relic_dv.h"
#include "relic_bn.h"
#include "relic_pool.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if ALLOC == STATIC

dig_t *pool_get(void) {
	int i, r;
	ctx_t *ctx = core_get();

	if (ctx->next == POOL_EMPTY) {
		return NULL;
	}

	/** Allocate a free element. */
	r = ctx->next;
	ctx->pool[r].state = POOL_USED;

	/* Search for a new free element. */
	for (i = (r + 1) % POOL_SIZE; i != r; i = (i + 1) % POOL_SIZE) {
		if (ctx->pool[i].state == POOL_FREE) {
			break;
		}
	}
	if (i == r) {
		ctx->next = POOL_EMPTY;
	} else {
		ctx->next = i;
	}
	ctx->pool[r].elem[DV_DIGS] = r;
	return (ctx->pool[r].elem);
}

void pool_put(dig_t *a) {
	int pos = a[DV_DIGS];
	ctx_t *ctx = core_get();

	ctx->pool[pos].state = POOL_FREE;
	ctx->next = pos;
}

#endif /* ALLOC == STATIC */
