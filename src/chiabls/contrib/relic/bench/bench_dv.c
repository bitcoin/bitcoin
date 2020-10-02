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
 * Benchmarks for manipulating temporary double-precision digit vectors.
 *
 * @ingroup bench
 */

#include <stdio.h>

#include "relic.h"
#include "relic_bench.h"

static void memory(void) {
	dv_t a[BENCH];

	BENCH_SMALL("dv_null", dv_null(a[i]));

	BENCH_SMALL("dv_new", dv_new(a[i]));
	for (int i = 0; i < BENCH; i++) {
		dv_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		dv_new(a[i]);
	}
	BENCH_SMALL("dv_free", dv_free(a[i]));

	(void)a;
}

static void copy(void) {
	dv_t a, b;

	dv_null(a);
	dv_null(b);

	dv_new(a);
	dv_new(b);

	BENCH_BEGIN("dv_copy") {
		rand_bytes((uint8_t *)a, DV_DIGS * sizeof(dig_t));
		rand_bytes((uint8_t *)b, DV_DIGS * sizeof(dig_t));
		BENCH_ADD(dv_copy(a, b, DV_DIGS));
	} BENCH_END;

	BENCH_BEGIN("dv_copy_cond") {
		rand_bytes((uint8_t *)a, DV_DIGS * sizeof(dig_t));
		rand_bytes((uint8_t *)b, DV_DIGS * sizeof(dig_t));
		BENCH_ADD(dv_copy_cond(a, b, DV_DIGS, 1));
	} BENCH_END;

	BENCH_BEGIN("dv_swap_cond") {
		rand_bytes((uint8_t *)a, DV_DIGS * sizeof(dig_t));
		rand_bytes((uint8_t *)b, DV_DIGS * sizeof(dig_t));
		BENCH_ADD(dv_swap_cond(a, b, DV_DIGS, 1));
	} BENCH_END;

	BENCH_BEGIN("dv_cmp_const") {
		rand_bytes((uint8_t *)a, DV_DIGS * sizeof(dig_t));
		rand_bytes((uint8_t *)b, DV_DIGS * sizeof(dig_t));
		BENCH_ADD(dv_cmp_const(a, b, DV_DIGS));
	} BENCH_END;

	dv_free(a);
	dv_free(b);
}

int main(void) {
	if (core_init() != STS_OK) {
		core_clean();
		return 1;
	}

	conf_print();
	util_banner("Benchmarks for the DV module:", 0);
	util_banner("Utilities:\n", 0);
	memory();
	copy();
	core_clean();
	return 0;
}
