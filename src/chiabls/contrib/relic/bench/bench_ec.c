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
 * Benchmarks for Elliptic Curve Cryptography.
 *
 * @ingroup bench
 */

#include <stdio.h>

#include "relic.h"
#include "relic_bench.h"

static void memory(void) {
	ec_t a[BENCH];

	BENCH_SMALL("ec_null", ec_null(a[i]));

	BENCH_SMALL("ec_new", ec_new(a[i]));
	for (int i = 0; i < BENCH; i++) {
		ec_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		ec_new(a[i]);
	}
	BENCH_SMALL("ec_free", ec_free(a[i]));

	(void)a;
}

static void util(void) {
	ec_t p, q;
	uint8_t bin[2 * FC_BYTES + 1];
	int l;

	ec_null(p);
	ec_null(q);

	ec_new(p);
	ec_new(q);

	BENCH_BEGIN("ec_is_infty") {
		ec_rand(p);
		BENCH_ADD(ec_is_infty(p));
	}
	BENCH_END;

	BENCH_BEGIN("ec_set_infty") {
		ec_rand(p);
		BENCH_ADD(ec_set_infty(p));
	}
	BENCH_END;

	BENCH_BEGIN("ec_copy") {
		ec_rand(p);
		ec_rand(q);
		BENCH_ADD(ec_copy(p, q));
	}
	BENCH_END;

	BENCH_BEGIN("ec_cmp") {
		ec_rand(p);
		ec_dbl(p, p);
		ec_rand(q);
		ec_dbl(q, q);
		BENCH_ADD(ec_cmp(p, q));
	} BENCH_END;

	BENCH_BEGIN("ec_cmp (1 norm)") {
		ec_rand(p);
		ec_dbl(p, p);
		ec_rand(q);
		BENCH_ADD(ec_cmp(p, q));
	} BENCH_END;

	BENCH_BEGIN("ec_cmp (2 norm)") {
		ec_rand(p);
		ec_rand(q);
		BENCH_ADD(ec_cmp(p, q));
	} BENCH_END;

	BENCH_BEGIN("ec_rand") {
		BENCH_ADD(ec_rand(p));
	}
	BENCH_END;

	BENCH_BEGIN("ec_is_valid") {
		ec_rand(p);
		BENCH_ADD(ec_is_valid(p));
	} BENCH_END;

	BENCH_BEGIN("ec_size_bin (0)") {
		ec_rand(p);
		BENCH_ADD(ec_size_bin(p, 0));
	} BENCH_END;

	BENCH_BEGIN("ec_size_bin (1)") {
		ec_rand(p);
		BENCH_ADD(ec_size_bin(p, 1));
	} BENCH_END;

	BENCH_BEGIN("ec_write_bin (0)") {
		ec_rand(p);
		l = ec_size_bin(p, 0);
		BENCH_ADD(ec_write_bin(bin, l, p, 0));
	} BENCH_END;

	BENCH_BEGIN("ec_write_bin (1)") {
		ec_rand(p);
		l = ec_size_bin(p, 1);
		BENCH_ADD(ec_write_bin(bin, l, p, 1));
	} BENCH_END;

	BENCH_BEGIN("ec_read_bin (0)") {
		ec_rand(p);
		l = ec_size_bin(p, 0);
		ec_write_bin(bin, l, p, 0);
		BENCH_ADD(ec_read_bin(p, bin, l));
	} BENCH_END;

	BENCH_BEGIN("ec_read_bin (1)") {
		ec_rand(p);
		l = ec_size_bin(p, 1);
		ec_write_bin(bin, l, p, 1);
		BENCH_ADD(ec_read_bin(p, bin, l));
	} BENCH_END;

	ec_free(p);
	ec_free(q);
}

static void arith(void) {
	ec_t p, q, r, t[RELIC_EC_TABLE];
	bn_t k, l, n;

	ec_null(p);
	ec_null(q);
	ec_null(r);
	for (int i = 0; i < RELIC_EC_TABLE; i++) {
		ec_null(t[i]);
	}

	ec_new(p);
	ec_new(q);
	ec_new(r);
	bn_new(k);
	bn_new(n);
	bn_new(l);

	ec_curve_get_ord(n);

	BENCH_BEGIN("ec_add") {
		ec_rand(p);
		ec_rand(q);
		ec_add(p, p, q);
		ec_rand(q);
		ec_rand(p);
		ec_add(q, q, p);
		BENCH_ADD(ec_add(r, p, q));
	}
	BENCH_END;

	BENCH_BEGIN("ec_sub") {
		ec_rand(p);
		ec_rand(q);
		ec_add(p, p, q);
		ec_rand(q);
		ec_rand(p);
		ec_add(q, q, p);
		BENCH_ADD(ec_sub(r, p, q));
	}
	BENCH_END;

	BENCH_BEGIN("ec_dbl") {
		ec_rand(p);
		ec_rand(q);
		ec_add(p, p, q);
		BENCH_ADD(ec_dbl(r, p));
	}
	BENCH_END;

	BENCH_BEGIN("ec_neg") {
		ec_rand(p);
		ec_rand(q);
		ec_add(p, p, q);
		BENCH_ADD(ec_neg(r, p));
	}
	BENCH_END;

	BENCH_BEGIN("ec_mul") {
		bn_rand_mod(k, n);
		ec_rand(p);
		BENCH_ADD(ec_mul(q, p, k));
	}
	BENCH_END;

	BENCH_BEGIN("ec_mul_gen") {
		bn_rand_mod(k, n);
		BENCH_ADD(ec_mul_gen(q, k));
	}
	BENCH_END;

	for (int i = 0; i < RELIC_EC_TABLE; i++) {
		ec_new(t[i]);
	}

	BENCH_BEGIN("ec_mul_pre") {
		BENCH_ADD(ec_mul_pre(t, p));
	}
	BENCH_END;

	BENCH_BEGIN("ec_mul_fix") {
		bn_rand_mod(k, n);
		ec_mul_pre(t, p);
		BENCH_ADD(ec_mul_fix(q, (const ec_t *)t, k));
	}
	BENCH_END;

	BENCH_BEGIN("ec_mul_sim") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		ec_rand(p);
		ec_rand(q);
		BENCH_ADD(ec_mul_sim(r, p, k, q, l));
	}
	BENCH_END;

	BENCH_BEGIN("ec_mul_sim_gen") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		ec_rand(q);
		BENCH_ADD(ec_mul_sim_gen(r, k, q, l));
	}
	BENCH_END;

	BENCH_BEGIN("ec_map") {
		uint8_t msg[5];
		rand_bytes(msg, 5);
		BENCH_ADD(ec_map(p, msg, 5));
	} BENCH_END;

	BENCH_BEGIN("ec_pck") {
		ec_rand(p);
		BENCH_ADD(ec_pck(q, p));
	} BENCH_END;

	BENCH_BEGIN("ec_upk") {
		ec_rand(p);
		BENCH_ADD(ec_upk(q, p));
	} BENCH_END;

	ec_free(p);
	ec_free(q);
	bn_free(k);
	bn_free(l);
	bn_free(n);
	for (int i = 0; i < RELIC_EC_TABLE; i++) {
		ec_free(t[i]);
	}
}

int main(void) {
	if (core_init() != STS_OK) {
		core_clean();
		return 1;
	}

	conf_print();
	util_banner("Benchmarks for the EC module:", 0);

	if (ec_param_set_any() != STS_OK) {
		THROW(ERR_NO_CURVE);
		core_clean();
		return 0;
	}

	ec_param_print();

	util_banner("Utilities:", 1);
	memory();
	util();

	util_banner("Arithmetic:", 1);
	arith();

	core_clean();
	return 0;
}
