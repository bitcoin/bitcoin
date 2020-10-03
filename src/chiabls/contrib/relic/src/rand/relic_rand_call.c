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
 * Implementation of stub generator overridden by callback function.
 *
 * @ingroup rand
 */

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "relic_conf.h"
#include "relic_core.h"
#include "relic_label.h"
#include "relic_rand.h"

/*============================================================================*/
/* Private definitions                                                         */
/*============================================================================*/

#if RAND == CALL

static void rand_stub(uint8_t *buf, int size, void *args) {
	int c, l, fd = open("/dev/urandom", O_RDONLY);

	if (fd == -1) {
		THROW(ERR_NO_FILE);
	}

	l = 0;
	do {
		c = read(fd, buf + l, size - l);
		l += c;
		if (c == -1) {
			THROW(ERR_NO_READ);
		}
	} while (l < size);

	close(fd);
}

#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if RAND == CALL

void rand_bytes(uint8_t *buf, int size) {
	ctx_t *ctx = core_get();

	ctx->rand_call(buf, size, ctx->rand_args);
}

void rand_seed(void (*callback)(uint8_t *, int, void *), void *args) {
	ctx_t *ctx = core_get();

	if (callback == NULL) {
		ctx->rand_call = rand_stub;
		ctx->rand_args = NULL;
	} else {
		ctx->rand_call = callback;
		ctx->rand_args = args;
	}
	core_get()->seeded = 1;
}

#endif