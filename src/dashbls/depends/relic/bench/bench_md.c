/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2021 RELIC Authors
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
 * Benchmarks for hash functions.
 *
 * @ingroup md
 */

#include <stdio.h>

#include "relic.h"
#include "relic_bench.h"

static void hash(void) {
	uint8_t hash[RLC_MD_LEN], buf[256];
	const uint8_t *dst = (const uint8_t *)"RELIC";

	BENCH_RUN("md_map (256)") {
		rand_bytes(buf, sizeof(buf));
		BENCH_ADD(md_map(hash, buf, sizeof(buf)));
	} BENCH_END;

	BENCH_RUN("md_xmd (256)") {
		rand_bytes(buf, sizeof(buf));
		BENCH_ADD(md_xmd(buf, sizeof(buf), buf, sizeof(buf), dst, 5));
	} BENCH_END;
}

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	conf_print();
	util_banner("Benchmarks for the MD module:\n", 0);
	hash();
	core_clean();
	return 0;
}
