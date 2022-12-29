/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2010 RELIC Authors
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
 * Benchmarks for Pairing-Based Cryptography.
 *
 * @ingroup bench
 */

#include <stdio.h>

#include "relic.h"
#include "relic_bench.h"

static void memory1(void) {
	g1_t a[BENCH];

	BENCH_FEW("g1_null", g1_null(a[i]), 1);

	BENCH_FEW("g1_new", g1_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		g1_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		g1_new(a[i]);
	}
	BENCH_FEW("g1_free", g1_free(a[i]), 1);

	(void)a;
}

static void util1(void) {
	g1_t p, q;
	uint8_t bin[2 * RLC_PC_BYTES + 1];
	int l;

	g1_null(p);
	g1_null(q);

	g1_new(p);
	g1_new(q);

	BENCH_RUN("g1_is_infty") {
		g1_rand(p);
		BENCH_ADD(g1_is_infty(p));
	}
	BENCH_END;

	BENCH_RUN("g1_set_infty") {
		g1_rand(p);
		BENCH_ADD(g1_set_infty(p));
	}
	BENCH_END;

	BENCH_RUN("g1_copy") {
		g1_rand(p);
		g1_rand(q);
		BENCH_ADD(g1_copy(p, q));
	}
	BENCH_END;

	BENCH_RUN("g1_cmp") {
		g1_rand(p);
		g1_dbl(p, p);
		g1_rand(q);
		g1_dbl(q, q);
		BENCH_ADD(g1_cmp(p, q));
	} BENCH_END;

	BENCH_RUN("g1_cmp (1 norm)") {
		g1_rand(p);
		g1_dbl(p, p);
		g1_rand(q);
		BENCH_ADD(g1_cmp(p, q));
	} BENCH_END;

	BENCH_RUN("g1_cmp (2 norm)") {
		g1_rand(p);
		g1_rand(q);
		BENCH_ADD(g1_cmp(p, q));
	} BENCH_END;

	BENCH_RUN("g1_rand") {
		BENCH_ADD(g1_rand(p));
	}
	BENCH_END;

	BENCH_RUN("g1_is_valid") {
		g1_rand(p);
		BENCH_ADD(g1_is_valid(p));
	} BENCH_END;

	BENCH_RUN("g1_size_bin (0)") {
		g1_rand(p);
		BENCH_ADD(g1_size_bin(p, 0));
	} BENCH_END;

	BENCH_RUN("g1_size_bin (1)") {
		g1_rand(p);
		BENCH_ADD(g1_size_bin(p, 1));
	} BENCH_END;

	BENCH_RUN("g1_write_bin (0)") {
		g1_rand(p);
		l = g1_size_bin(p, 0);
		BENCH_ADD(g1_write_bin(bin, l, p, 0));
	} BENCH_END;

	BENCH_RUN("g1_write_bin (1)") {
		g1_rand(p);
		l = g1_size_bin(p, 1);
		BENCH_ADD(g1_write_bin(bin, l, p, 1));
	} BENCH_END;

	BENCH_RUN("g1_read_bin (0)") {
		g1_rand(p);
		l = g1_size_bin(p, 0);
		g1_write_bin(bin, l, p, 0);
		BENCH_ADD(g1_read_bin(p, bin, l));
	} BENCH_END;

	BENCH_RUN("g1_read_bin (1)") {
		g1_rand(p);
		l = g1_size_bin(p, 1);
		g1_write_bin(bin, l, p, 1);
		BENCH_ADD(g1_read_bin(p, bin, l));
	} BENCH_END;
}

static void arith1(void) {
	g1_t p, q, r, t[RLC_G1_TABLE];
	bn_t k, l, n;

	g1_null(p);
	g1_null(q);
	g1_null(r);
	for (int i = 0; i < RLC_G1_TABLE; i++) {
		g1_null(t[i]);
	}

	g1_new(p);
	g1_new(q);
	g1_new(r);
	bn_new(k);
	bn_new(n);
	bn_new(l);

	pc_get_ord(n);

	BENCH_RUN("g1_add") {
		g1_rand(p);
		g1_rand(q);
		g1_add(p, p, q);
		g1_rand(q);
		g1_rand(p);
		g1_add(q, q, p);
		BENCH_ADD(g1_add(r, p, q));
	}
	BENCH_END;

	BENCH_RUN("g1_sub") {
		g1_rand(p);
		g1_rand(q);
		g1_add(p, p, q);
		g1_rand(q);
		g1_rand(p);
		g1_add(q, q, p);
		BENCH_ADD(g1_sub(r, p, q));
	}
	BENCH_END;

	BENCH_RUN("g1_dbl") {
		g1_rand(p);
		g1_rand(q);
		g1_add(p, p, q);
		BENCH_ADD(g1_dbl(r, p));
	}
	BENCH_END;

	BENCH_RUN("g1_neg") {
		g1_rand(p);
		g1_rand(q);
		g1_add(p, p, q);
		BENCH_ADD(g1_neg(r, p));
	}
	BENCH_END;

	BENCH_RUN("g1_mul") {
		bn_rand(k, RLC_POS, bn_bits(n));
		bn_rand_mod(k, n);
		g1_rand(p);
		BENCH_ADD(g1_mul(q, p, k));
	}
	BENCH_END;

	BENCH_RUN("g1_mul_gen") {
		bn_rand(k, RLC_POS, bn_bits(n));
		bn_rand_mod(k, n);
		BENCH_ADD(g1_mul_gen(q, k));
	}
	BENCH_END;

	for (int i = 0; i < RLC_G1_TABLE; i++) {
		g1_new(t[i]);
	}

	BENCH_RUN("g1_mul_pre") {
		BENCH_ADD(g1_mul_pre(t, p));
	}
	BENCH_END;

	BENCH_RUN("g1_mul_fix") {
		bn_rand(k, RLC_POS, bn_bits(n));
		bn_rand_mod(k, n);
		g1_mul_pre(t, p);
		BENCH_ADD(g1_mul_fix(q, (const g1_t *)t, k));
	}
	BENCH_END;

	BENCH_RUN("g1_mul_sim") {
		bn_rand(k, RLC_POS, bn_bits(n));
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		g1_rand(p);
		g1_rand(q);
		BENCH_ADD(g1_mul_sim(r, p, k, q, l));
	}
	BENCH_END;

	BENCH_RUN("g1_mul_sim_gen") {
		bn_rand(k, RLC_POS, bn_bits(n));
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		g1_rand(q);
		BENCH_ADD(g1_mul_sim_gen(r, k, q, l));
	}
	BENCH_END;

	BENCH_RUN("g1_mul_dig") {
		bn_rand(k, RLC_POS, bn_bits(n));
		bn_rand_mod(k, n);
		g1_rand(p);
		BENCH_ADD(g1_mul_dig(q, p, k->dp[0]));
	}
	BENCH_END;

	BENCH_RUN("g1_map") {
		uint8_t msg[5];
		rand_bytes(msg, 5);
		BENCH_ADD(g1_map(p, msg, 5));
	} BENCH_END;

	g1_free(p);
	g1_free(q);
	bn_free(k);
	bn_free(l);
	bn_free(n);
	for (int i = 0; i < RLC_G1_TABLE; i++) {
		g1_free(t[i]);
	}
}

static void memory2(void) {
	g2_t a[BENCH];

	BENCH_FEW("g2_null", g2_null(a[i]), 1);

	BENCH_FEW("g2_new", g2_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		g2_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		g2_new(a[i]);
	}
	BENCH_FEW("g2_free", g2_free(a[i]), 1);

	(void)a;
}

static void util2(void) {
	g2_t p, q;
	uint8_t bin[8 * RLC_PC_BYTES + 1];
	int l;

	g2_null(p);
	g2_null(q);

	g2_new(p);
	g2_new(q);

	BENCH_RUN("g2_is_infty") {
		g2_rand(p);
		BENCH_ADD(g2_is_infty(p));
	}
	BENCH_END;

	BENCH_RUN("g2_set_infty") {
		g2_rand(p);
		BENCH_ADD(g2_set_infty(p));
	}
	BENCH_END;

	BENCH_RUN("g2_copy") {
		g2_rand(p);
		g2_rand(q);
		BENCH_ADD(g2_copy(p, q));
	}
	BENCH_END;

	BENCH_RUN("g2_cmp") {
		g2_rand(p);
		g2_dbl(p, p);
		g2_rand(q);
		g2_dbl(q, q);
		BENCH_ADD(g2_cmp(p, q));
	} BENCH_END;

	BENCH_RUN("g2_cmp (1 norm)") {
		g2_rand(p);
		g2_dbl(p, p);
		g2_rand(q);
		BENCH_ADD(g2_cmp(p, q));
	} BENCH_END;

	BENCH_RUN("g2_cmp (2 norm)") {
		g2_rand(p);
		g2_rand(q);
		BENCH_ADD(g2_cmp(p, q));
	} BENCH_END;

	BENCH_RUN("g2_rand") {
		BENCH_ADD(g2_rand(p));
	}
	BENCH_END;

	BENCH_RUN("g2_is_valid") {
		BENCH_ADD(g2_is_valid(p));
	}
	BENCH_END;

	BENCH_RUN("g2_size_bin (0)") {
		g2_rand(p);
		BENCH_ADD(g2_size_bin(p, 0));
	} BENCH_END;

	BENCH_RUN("g2_size_bin (1)") {
		g2_rand(p);
		BENCH_ADD(g2_size_bin(p, 1));
	} BENCH_END;

	BENCH_RUN("g2_write_bin (0)") {
		g2_rand(p);
		l = g2_size_bin(p, 0);
		BENCH_ADD(g2_write_bin(bin, l, p, 0));
	} BENCH_END;

	BENCH_RUN("g2_write_bin (1)") {
		g2_rand(p);
		l = g2_size_bin(p, 1);
		BENCH_ADD(g2_write_bin(bin, l, p, 1));
	} BENCH_END;

	BENCH_RUN("g2_read_bin (0)") {
		g2_rand(p);
		l = g2_size_bin(p, 0);
		g2_write_bin(bin, l, p, 0);
		BENCH_ADD(g2_read_bin(p, bin, l));
	} BENCH_END;

	BENCH_RUN("g2_read_bin (1)") {
		g2_rand(p);
		l = g2_size_bin(p, 1);
		g2_write_bin(bin, l, p, 1);
		BENCH_ADD(g2_read_bin(p, bin, l));
	} BENCH_END;

	g2_free(p)
	g2_free(q);
}

static void arith2(void) {
	g2_t p, q, r, t[RLC_G2_TABLE];
	bn_t k, l, n;

	g2_null(p);
	g2_null(q);
	g2_null(r);
	for (int i = 0; i < RLC_G2_TABLE; i++) {
		g2_null(t[i]);
	}

	g2_new(p);
	g2_new(q);
	g2_new(r);
	bn_new(k);
	bn_new(n);
	bn_new(l);

	pc_get_ord(n);

	BENCH_RUN("g2_add") {
		g2_rand(p);
		g2_rand(q);
		g2_add(p, p, q);
		g2_rand(q);
		g2_rand(p);
		g2_add(q, q, p);
		BENCH_ADD(g2_add(r, p, q));
	}
	BENCH_END;

	BENCH_RUN("g2_sub") {
		g2_rand(p);
		g2_rand(q);
		g2_add(p, p, q);
		g2_rand(q);
		g2_rand(p);
		g2_add(q, q, p);
		BENCH_ADD(g2_sub(r, p, q));
	}
	BENCH_END;

	BENCH_RUN("g2_dbl") {
		g2_rand(p);
		g2_rand(q);
		g2_add(p, p, q);
		BENCH_ADD(g2_dbl(r, p));
	}
	BENCH_END;

	BENCH_RUN("g2_neg") {
		g2_rand(p);
		g2_rand(q);
		g2_add(p, p, q);
		BENCH_ADD(g2_neg(r, p));
	}
	BENCH_END;

	BENCH_RUN("g2_mul") {
		bn_rand(k, RLC_POS, bn_bits(n));
		bn_rand_mod(k, n);
		g2_rand(p);
		BENCH_ADD(g2_mul(q, p, k));
	}
	BENCH_END;

	BENCH_RUN("g2_mul_gen") {
		bn_rand(k, RLC_POS, bn_bits(n));
		bn_rand_mod(k, n);
		BENCH_ADD(g2_mul_gen(q, k));
	}
	BENCH_END;

	for (int i = 0; i < RLC_G1_TABLE; i++) {
		g2_new(t[i]);
	}

	BENCH_RUN("g2_mul_pre") {
		BENCH_ADD(g2_mul_pre(t, p));
	}
	BENCH_END;

	BENCH_RUN("g2_mul_fix") {
		bn_rand(k, RLC_POS, bn_bits(n));
		bn_rand_mod(k, n);
		g2_mul_pre(t, p);
		BENCH_ADD(g2_mul_fix(q, t, k));
	}
	BENCH_END;

	BENCH_RUN("g2_mul_sim") {
		bn_rand(k, RLC_POS, bn_bits(n));
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		g2_rand(p);
		g2_rand(q);
		BENCH_ADD(g2_mul_sim(r, p, k, q, l));
	}
	BENCH_END;

	BENCH_RUN("g2_mul_sim_gen") {
		bn_rand(k, RLC_POS, bn_bits(n));
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		g2_rand(q);
		BENCH_ADD(g2_mul_sim_gen(r, k, q, l));
	}
	BENCH_END;

	BENCH_RUN("g2_mul_dig") {
		bn_rand(k, RLC_POS, bn_bits(n));
		bn_rand_mod(k, n);
		g2_rand(p);
		BENCH_ADD(g2_mul_dig(q, p, k->dp[0]));
	}
	BENCH_END;

#if FP_PRIME != 509
	BENCH_RUN("g2_map") {
		uint8_t msg[5];
		rand_bytes(msg, 5);
		BENCH_ADD(g2_map(p, msg, 5));
	} BENCH_END;
#endif

	g2_free(p);
	g2_free(q);
	bn_free(k);
	bn_free(l);
	bn_free(n);
	for (int i = 0; i < RLC_G1_TABLE; i++) {
		g2_free(t[i]);
	}
}

static void memory(void) {
	gt_t a[BENCH];

	BENCH_FEW("gt_null", gt_null(a[i]), 1);

	BENCH_FEW("gt_new", gt_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		gt_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		gt_new(a[i]);
	}
	BENCH_FEW("gt_free", gt_free(a[i]), 1);

	(void)a;
}

static void util(void) {
	gt_t a, b;
	uint8_t bin[12 * RLC_PC_BYTES];
	int l;

	gt_null(a);
	gt_null(b);

	gt_new(a);
	gt_new(b);

	BENCH_RUN("gt_copy") {
		gt_rand(a);
		BENCH_ADD(gt_copy(b, a));
	}
	BENCH_END;

	BENCH_RUN("gt_zero") {
		gt_rand(a);
		BENCH_ADD(gt_zero(a));
	}
	BENCH_END;

	BENCH_RUN("gt_set_unity") {
		gt_rand(a);
		BENCH_ADD(gt_set_unity(a));
	}
	BENCH_END;

	BENCH_RUN("gt_is_unity") {
		gt_rand(a);
		BENCH_ADD((void)gt_is_unity(a));
	}
	BENCH_END;

	BENCH_RUN("gt_rand") {
		BENCH_ADD(gt_rand(a));
	}
	BENCH_END;

	BENCH_RUN("gt_cmp") {
		gt_rand(a);
		gt_rand(b);
		BENCH_ADD(gt_cmp(b, a));
	}
	BENCH_END;

	BENCH_RUN("gt_size_bin (0)") {
		gt_rand(a);
		BENCH_ADD(gt_size_bin(a, 0));
	} BENCH_END;

	BENCH_RUN("gt_write_bin (0)") {
		gt_rand(a);
		l = gt_size_bin(a, 0);
		BENCH_ADD(gt_write_bin(bin, l, a, 0));
	} BENCH_END;

	BENCH_RUN("gt_read_bin (0)") {
		gt_rand(a);
		l = gt_size_bin(a, 0);
		gt_write_bin(bin, l, a, 0);
		BENCH_ADD(gt_read_bin(a, bin, l));
	} BENCH_END;

	if (ep_param_embed() == 12) {
		BENCH_RUN("gt_size_bin (1)") {
			gt_rand(a);
			BENCH_ADD(gt_size_bin(a, 1));
		} BENCH_END;

		BENCH_RUN("gt_write_bin (1)") {
			gt_rand(a);
			l = gt_size_bin(a, 1);
			BENCH_ADD(gt_write_bin(bin, l, a, 1));
		} BENCH_END;

		BENCH_RUN("gt_read_bin (1)") {
			gt_rand(a);
			l = gt_size_bin(a, 1);
			gt_write_bin(bin, l, a, 1);
			BENCH_ADD(gt_read_bin(a, bin, l));
		} BENCH_END;
	}

	BENCH_RUN("gt_is_valid") {
		gt_rand(a);
		BENCH_ADD(gt_is_valid(a));
	} BENCH_END;

	gt_free(a);
	gt_free(b);
}

static void arith(void) {
	gt_t a, b, c;
	bn_t d, e, f;

	gt_new(a);
	gt_new(b);
	gt_new(c);
	bn_new(d);
	bn_new(e);
	bn_new(f);

	BENCH_RUN("gt_mul") {
		gt_rand(a);
		gt_rand(b);
		BENCH_ADD(gt_mul(c, a, b));
	}
	BENCH_END;

	BENCH_RUN("gt_sqr") {
		gt_rand(a);
		gt_rand(b);
		BENCH_ADD(gt_sqr(c, a));
	}
	BENCH_END;

	BENCH_RUN("gt_inv") {
		gt_rand(a);
		BENCH_ADD(gt_inv(c, a));
	}
	BENCH_END;

	BENCH_RUN("gt_exp") {
		gt_rand(a);
		pc_get_ord(d);
		bn_rand_mod(e, d);
		BENCH_ADD(gt_exp(c, a, e));
	}
	BENCH_END;

	BENCH_RUN("gt_exp_sim") {
		gt_rand(a);
		gt_rand(b);
		gt_get_ord(d);
		bn_rand_mod(e, d);
		bn_rand_mod(f, d);
		BENCH_ADD(gt_exp_sim(c, a, e, b, f));
	}
	BENCH_END;

	BENCH_RUN("gt_exp_dig") {
		gt_rand(a);
		pc_get_ord(d);
		bn_rand(e, RLC_POS, bn_bits(d));
		BENCH_ADD(gt_exp_dig(c, a, e->dp[0]));
	}
	BENCH_END;

	gt_free(a);
	gt_free(b);
	gt_free(c);
	bn_free(d);
	bn_free(e);
	bn_free(f);
}

static void pairing(void) {
	g1_t p[2];
	g2_t q[2];
	gt_t r;

	g1_new(p[0]);
	g2_new(q[0]);
	g1_new(p[1]);
	g2_new(q[1]);
	gt_new(r);

	BENCH_RUN("pc_map") {
		g1_rand(p[0]);
		g2_rand(q[0]);
		BENCH_ADD(pc_map(r, p[0], q[0]));
	}
	BENCH_END;

	BENCH_RUN("pc_exp") {
		gt_rand(r);
		BENCH_ADD(pc_exp(r, r));
	}
	BENCH_END;

	BENCH_RUN("pc_map_sim (2)") {
		g1_rand(p[1]);
		g2_rand(q[1]);
		BENCH_ADD(pc_map_sim(r, p, q, 2));
	}
	BENCH_END;

	g1_free(p[0]);
	g2_free(q[0]);
	g1_free(p[1]);
	g2_free(q[1]);
	gt_free(r);
}

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	conf_print();
	util_banner("Benchmarks for the PC module:", 0);

	if (pc_param_set_any() != RLC_OK) {
		RLC_THROW(ERR_NO_CURVE);
		core_clean();
		return 0;
	}

	pc_param_print();

	util_banner("Group G_1:", 0);
	util_banner("Utilities:", 1);
	memory1();
	util1();

	util_banner("Arithmetic:", 1);
	arith1();

	util_banner("Group G_2:", 0);
	util_banner("Utilities:", 1);
	memory2();
	util2();

	util_banner("Arithmetic:", 1);
	arith2();

	util_banner("Group G_T:", 0);
	util_banner("Utilities:", 1);
	memory();
	util();

	util_banner("Arithmetic:", 1);
	arith();

	util_banner("Pairing:", 0);
	util_banner("Arithmetic:", 1);
	pairing();

	core_clean();
	return 0;
}
