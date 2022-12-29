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
 * Benchmarks for cryptographic protocols.
 *
 * @version $Id$
 * @ingroup bench
 */

#include <stdio.h>

#include "relic.h"
#include "relic_bench.h"

#define MAX_KEYS	2048

#include "assert.h"

static void ers(void) {
	int size;
	ec_t pp, pk[MAX_KEYS], *ptr;
	bn_t sk[MAX_KEYS], td;
	ers_t ring[MAX_KEYS];
	uint8_t m[5] = { 0, 1, 2, 3, 4 };

	bn_null(td);
	ec_null(pp);

	bn_new(td);
	ec_new(pp);
	for (int i = 0; i < MAX_KEYS; i++) {
		bn_null(sk[i]);
		bn_new(sk[i]);
		ec_null(pk[i]);
		ec_new(pk[i]);
		ers_null(ring[i]);
		ers_new(ring[i]);
		cp_ers_gen_key(sk[i], pk[i]);
	}

	cp_ers_gen(pp);

	bench_reset();
	for (int j = 0; j < BENCH; j++)	{
		BENCH_ADD(cp_ers_sig(td, ring[0], m, 5, sk[0], pk[0], pp));
	}
	bench_compute(BENCH * BENCH);
	util_print("{\"1\": {\"time\": %lf, \"size\": null}", bench_total()/(double)1000000);

	for (int j = 1; j < MAX_KEYS; j = j << 1) {
		size = j;
		bench_before();
		for (int k = 0; k < j; k++) {
			cp_ers_ext(td, ring, &size, m, 5, pk[size], pp);
		}
		bench_after();
		bench_compute(1);
		util_print(", \"%d\": {\"time\": %lf, \"size\": null}", size, bench_total()/(double)1000000);
		assert(cp_ers_ver(td, ring, size, m, 5, pp));
	}
	util_print("}\n\n");

	/* Recompute the signature for verification. */
	cp_ers_sig(td, ring[0], m, 5, sk[0], pk[0], pp);
	bench_reset();
	for (int j = 0; j < BENCH; j++)	{
		BENCH_ADD(assert(cp_ers_ver(td, ring, 1, m, 5, pp)));
	}
	bench_compute(BENCH * BENCH);
	util_print("{\"1\": {\"time\": %lf, \"size\": %d}", bench_total()/(double)1000000, 9 * RLC_FP_BYTES);

	for (int j = 1; j < MAX_KEYS; j = j << 1) {
		size = j;
		/* Recompute the signatures for verification. */
		for (int k = 0; k < j; k++) {
			assert(cp_ers_ext(td, ring, &size, m, 5, pk[size], pp) == RLC_OK);
		}
		assert(cp_ers_ver(td, ring, size, m, 5, pp));
		bench_reset();
		for (int i = 0; i < BENCH; i++)	{
			BENCH_ADD(cp_ers_ver(td, ring, size, m, 5, pp));
		}
		bench_compute(BENCH * BENCH);
		util_print(", \"%d\": {\"time\": %lf, \"size\": %d}", size, bench_total()/(double)1000000, size * 9 * RLC_FP_BYTES);
	}
	util_print("}");

	bn_free(td);
	ec_free(pp);
	for (int i = 0; i < MAX_KEYS; i++) {
		bn_free(sk[i]);
		ec_free(pk[i]);
		ers_free(ring[i])
	}
}

#undef MAX_KEYS
#define MAX_KEYS	2048
#define MIN_KEYS	64

static void etrs(void) {
	int size;
	ec_t pp, pk[MAX_KEYS], *ptr;
	bn_t sk[MAX_KEYS], td[MAX_KEYS], y[MAX_KEYS];
	etrs_t ring[MAX_KEYS];
	uint8_t m[5] = { 0, 1, 2, 3, 4 };

	ec_null(pp);
	ec_new(pp);
	for (int i = 0; i < MAX_KEYS; i++) {
		bn_null(y[i]);
		bn_new(y[i]);
		bn_null(td[i]);
		bn_new(td[i]);
		bn_null(sk[i]);
		bn_new(sk[i]);
		ec_null(pk[i]);
		ec_new(pk[i]);
		ers_null(ring[i]);
		ers_new(ring[i]);
		cp_etrs_gen_key(sk[i], pk[i]);
	}

	cp_ers_gen(pp);

	util_print("{");
	for (int j = 1; j <= MAX_KEYS; j = j << 1) {
		bench_reset();
		bench_before();
		for (int i = 0; i < BENCH; i++)	{
			cp_etrs_sig(td, y, j, ring[0], m, 5, sk[0], pk[0], pp);
		}
		bench_after();
		bench_compute(BENCH);
		util_print("\"%d\": {\"time\": %lf, \"size\": null}, ", j, bench_total()/(double)1000000);
		assert(cp_etrs_ver(1, td, y, j, ring, 1, m, 5, pp));
	}
	util_print("}\n\n");

	for (int l = 2; l <= 8; l = l << 1) {
		for (int j = l; j <= MAX_KEYS; j = j << 1) {
			bench_reset();
			bench_before();
			size = 1;
			cp_etrs_sig(td, y, j, ring[0], m, 5, sk[0], pk[0], pp);
			for (int k = 1; k < j; k++) {
				cp_etrs_ext(td, y, j, ring, &size, m, 5, pk[size], pp);
			}
			bench_after();
			bench_compute(1);
			util_print("\"%d\": {\"time\": %lf, \"size\": null}, ", j, bench_total()/(double)1000000);
			assert(cp_etrs_ver(1, td+size-1, y+size-1, j-size+1, ring, size, m, 5, pp));
		}
		util_print("}\n\n");
	}

	for (int l = 1; l <= 8; l = l << 1) {
		for (int j = l; j <= MAX_KEYS; j = j << 1) {
			size = 1;
			cp_etrs_sig(td, y, j, ring[0], m, 5, sk[0], pk[0], pp);
			for (int k = 0; k < l; k++) {
				cp_etrs_ext(td, y, j, ring, &size, m, 5, pk[size], pp);
			}
			bench_reset();
			bench_before();
			for (int i = 0; i < BENCH; i++)	{
				cp_etrs_ver(1, td+size-1, y+size-1, j-size+1, ring, size, m, 5, pp);
			}
			bench_after();
			bench_compute(BENCH);
			util_print(", \"%d\": {\"time\": %lf, \"size\": null}", j, bench_total()/(double)1000000);
		}
		util_print("}\n\n");
	}

	ec_free(pp);
	for (int i = 0; i < MAX_KEYS; i++) {
		bn_free(td[i]);
		bn_free(y[i]);
		bn_free(sk[i]);
		ec_free(pk[i]);
		etrs_free(ring[i])
	}
}

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	conf_print();

	if (ec_param_set_any() == RLC_OK) {
		etrs();
		ers();
	} else {
		RLC_THROW(ERR_NO_CURVE);
	}

	core_clean();
	return 0;
}
