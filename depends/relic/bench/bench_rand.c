/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2014 RELIC Authors
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
 * Benchmarks for random number generation.
 *
 * @ingroup rand
 */

#include <stdio.h>

#include "relic.h"
#include "relic_bench.h"

#if RAND == CALL

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

static void test_bytes(uint8_t *buf, int size, void *args) {
	int c, l, fd = *(int *)args;

	if (fd == -1) {
		RLC_THROW(ERR_NO_FILE);
	}

	l = 0;
	do {
		c = read(fd, buf + l, size - l);
		l += c;
		if (c == -1) {
			RLC_THROW(ERR_NO_READ);
		}
	} while (l < size);
}

static void rng(void) {
	uint8_t buffer[64];
	int fd = open("/dev/urandom", O_RDONLY);

	BENCH_RUN("rand_seed") {
		rand_bytes(buffer, k);
		BENCH_ADD(rand_seed(&test_bytes, (void *)&fd));
	} BENCH_END;

	for (int k = 1; k <= sizeof(buffer); k *= 2) {
		BENCH_RUN("rand_bytes (from 1 to 256)") {
			BENCH_ADD(rand_bytes(buffer, k));
		} BENCH_END;
	}

	close(fd);
}

#else

static void rng(void) {
	uint8_t buffer[256];

	BENCH_RUN("rand_seed (20)") {
		rand_bytes(buffer, 20);
		BENCH_ADD(rand_seed(buffer, 20));
	} BENCH_END;

	for (int k = 1; k <= sizeof(buffer); k *= 2) {
		BENCH_RUN("rand_bytes (from 1 to 256)") {
			BENCH_ADD(rand_bytes(buffer, k));
		} BENCH_END;
	}
}

#endif

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	conf_print();
	util_banner("Benchmarks for the RAND module:", 0);
	util_banner("Utilities:\n", 0);
	rng();
	core_clean();
	return 0;
}
