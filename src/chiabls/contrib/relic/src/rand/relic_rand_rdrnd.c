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
 * Implementation of the wrapper around the Intel RdRand instruction.
 *
 * @ingroup rand
 */

#include <immintrin.h>

#include "relic_conf.h"
#include "relic_core.h"
#include "relic_label.h"
#include "relic_rand.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if RAND == RDRND

void rand_bytes(uint8_t *buf, int size) {
	int i = 0, j;
	unsigned long long r;

	while (i < size) {
#ifdef __RDRND__
		while (_rdrand64_step(&r) == 0);
#else
#error "RdRand not available, check your compiler settings."
#endif
		for (j = 0; i < size && j < sizeof(ull_t); i++, j++) {
			buf[i] = r & 0xFF;
		}
	}
}

void rand_seed(uint8_t *buf, int size) {
	/* Do nothing, mark as seeded. */
	core_get()->seeded = 1;
}

#endif