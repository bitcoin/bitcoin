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
 * Implementation of the /dev/urandom wrapper.
 *
 * @ingroup rand
 */

#include <unistd.h>

#include "relic_conf.h"
#include "relic_core.h"
#include "relic_label.h"
#include "relic_rand.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * The path to the char device that supplies entropy.
 */
#define RAND_PATH		"/dev/urandom"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if RAND == UDEV

void rand_bytes(uint8_t *buf, int size) {
	int c, l, *fd = (int *)&(core_get()->rand);

	l = 0;
	do {
		c = read(*fd, buf + l, size - l);
		l += c;
		if (c == -1) {
			THROW(ERR_NO_READ);
		}
	} while (l < size);
}

void rand_seed(uint8_t *buf, int size) {
	/* Do nothing, only mark as seeded. */
	core_get()->seeded = 1;
}

#endif
