/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2020 RELIC Authors
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

static void mul_triple(void) {
	bn_t order, d[2], e[2], x[2], y[2];
	mt_t tri[2];

	bn_null(order);
	mt_null(tri[0]);
	mt_null(tri[1]);

	bn_new(order);
	mt_new(tri[0]);
	mt_new(tri[1]);
	for (int j = 0; j < 2; j++) {
		bn_null(d[j]);
		bn_null(e[j]);
		bn_null(x[j]);
		bn_null(y[j]);
		bn_new(d[j]);
		bn_new(e[j]);
		bn_new(x[j]);
		bn_new(y[j]);
	}

	bn_gen_prime(order, RLC_BN_BITS);

	BENCH_RUN("mt_gen") {
		BENCH_ADD(mt_gen(tri, order));
	} BENCH_END;

	BENCH_RUN("mt_mul_lcl") {
		BENCH_ADD(mt_mul_lcl(d[0], e[0], x[0], y[0], order, tri[0]));
		BENCH_ADD(mt_mul_lcl(d[1], e[1], x[1], y[1], order, tri[1]));
	} BENCH_DIV(2);

	BENCH_RUN("mt_mul_bct") {
		BENCH_ADD(mt_mul_bct(d, e, order););
	} BENCH_END;

	BENCH_RUN("mt_mul_mpc") {
		BENCH_ADD(mt_mul_mpc(d[0], d[0], e[0], order, tri[0], 0););
		BENCH_ADD(mt_mul_mpc(d[1], d[1], e[1], order, tri[1], 1););
	} BENCH_DIV(2);

	bn_free(order);
	mt_free(tri[0]);
	mt_free(tri[1]);
	for (int j = 0; j < 2; j++) {
		bn_free(d[j]);
		bn_free(e[j]);
		bn_free(x[j]);
		bn_free(y[j]);
	}
}

static void pair_triple(void) {
	g1_t d[2], p[2], _p;
	g2_t e[2], q[2], _q;
	gt_t f[2], r[2], _r;
	bn_t k[2], l[2], n;
	mt_t tri[2];
	pt_t t[2];

	g1_null(_p);
	g2_null(_q);
	gt_null(_r);
	bn_null(n);

	g1_new(_p);
	g2_new(_q);
	gt_new(_r);
	bn_new(n);
	for (int j = 0; j < 2; j++) {
		g1_null(d[j]);
		g2_null(e[j]);
		bn_null(k[j]);
		bn_null(l[j]);
		g1_null(p[j]);
		g2_null(q[j]);
		gt_null(r[j]);
		mt_null(tri[j]);
		pt_null(t[j]);
		g1_new(d[j]);
		g2_new(e[j]);
		bn_new(k[j]);
		bn_new(l[j]);
		g1_new(p[j]);
		g2_new(q[j]);
		gt_new(r[j]);
		gt_new(f[j]);
		mt_new(tri[j]);
		pt_new(t[j]);
	}

	g1_get_ord(n);

	mt_gen(tri, n);
	BENCH_RUN("g1_mul") {
		/* Generate random inputs. */
		g1_rand(p[0]);
		bn_rand_mod(k[0], n);
		BENCH_ADD(g1_mul(_p, p[0], k[0]));
	} BENCH_END;

	/* Secret share inputs. */
	g1_rand(p[1]);
	g1_sub(p[0], p[0], p[1]);
	g1_norm(p[0], p[0]);
	bn_rand_mod(k[1], n);
	bn_sub(k[0], k[0], k[1]);
	if (bn_sign(k[0]) == RLC_NEG) {
		bn_add(k[0], k[0], n);
	}
	bn_mod(k[0], k[0], n);
	tri[0]->b1 = tri[0]->c1 = &p[0];
	tri[1]->b1 = tri[1]->c1 = &p[1];

	BENCH_RUN("g1_mul_lcl") {
		BENCH_ADD(g1_mul_lcl(l[0], d[0], k[0], p[0], tri[0]));
		BENCH_ADD(g1_mul_lcl(l[1], d[1], k[1], p[1], tri[1]))
	} BENCH_DIV(2);

	BENCH_RUN("g1_mul_bct") {
		BENCH_ADD(g1_mul_bct(l, d));
	} BENCH_END;

	BENCH_RUN("g1_mul_mpc") {
		BENCH_ADD(g1_mul_mpc(d[0], l[0], d[0], tri[0], 0));
		BENCH_ADD(g1_mul_mpc(d[1], l[1], d[1], tri[1], 1));
	} BENCH_DIV(2);

	mt_gen(tri, n);
	BENCH_RUN("g2_mul") {
		/* Generate random inputs. */
		g2_rand(q[0]);
		bn_rand_mod(k[0], n);
		BENCH_ADD(g2_mul(_q, q[0], k[0]));
	} BENCH_END;
	/* Secret share inputs. */
	g2_rand(q[1]);
	g2_sub(q[0], q[0], q[1]);
	g2_norm(q[0], q[0]);
	bn_rand_mod(k[1], n);
	bn_sub(k[0], k[0], k[1]);
	if (bn_sign(k[0]) == RLC_NEG) {
		bn_add(k[0], k[0], n);
	}
	bn_mod(k[0], k[0], n);
	tri[0]->b2 = tri[0]->c2 = &q[0];
	tri[1]->b2 = tri[1]->c2 = &q[1];

	BENCH_RUN("g2_mul_lcl") {
		BENCH_ADD(g2_mul_lcl(l[0], e[0], k[0], q[0], tri[0]));
		BENCH_ADD(g2_mul_lcl(l[1], e[1], k[1], q[1], tri[1]));
	} BENCH_DIV(2);

	BENCH_RUN("g2_mul_bct") {
		BENCH_ADD(g2_mul_bct(l, e));
	} BENCH_END;

	BENCH_RUN("g2_mul_mpc") {
		BENCH_ADD(g2_mul_mpc(e[0], l[0], e[0], tri[0], 0));
		BENCH_ADD(g2_mul_mpc(e[1], l[1], e[1], tri[1], 1));
	} BENCH_DIV(2);

	mt_gen(tri, n);
	BENCH_RUN("gt_exp") {
		/* Generate random inputs. */
		gt_rand(r[0]);
		bn_rand_mod(k[0], n);
		BENCH_ADD(gt_exp(_r, r[0], k[0]));
	} BENCH_END;
	/* Secret share inputs. */
	gt_rand(r[1]);
	gt_mul(r[0], r[0], r[1]);
	gt_inv(r[1], r[1]);
	bn_rand_mod(k[1], n);
	bn_sub(k[0], k[0], k[1]);
	if (bn_sign(k[0]) == RLC_NEG) {
		bn_add(k[0], k[0], n);
	}
	bn_mod(k[0], k[0], n);
	tri[0]->bt = tri[0]->ct = &r[0];
	tri[1]->bt = tri[1]->ct = &r[1];

	BENCH_RUN("gt_exp_lcl") {
		BENCH_ADD(gt_exp_lcl(l[0], f[0], k[0], r[0], tri[0]));
		BENCH_ADD(gt_exp_lcl(l[1], f[1], k[1], r[1], tri[1]));
	} BENCH_DIV(2);

	BENCH_RUN("gt_exp_bct") {
		BENCH_ADD(gt_exp_bct(l, f));
	} BENCH_END;

	BENCH_RUN("gt_exp_mpc") {
		BENCH_ADD(gt_exp_mpc(f[0], l[0], f[0], tri[0], 0));
		BENCH_ADD(gt_exp_mpc(f[1], l[1], f[1], tri[1], 1));
	} BENCH_DIV(2);

	/* Generate random inputs and triple. */
	pc_map_tri(t);
	BENCH_RUN("pc_map") {
		g1_rand(p[0]);
		g2_rand(q[0]);
		BENCH_ADD(pc_map(r[0], p[0], q[0]));
	} BENCH_END;
	/* Secret share inputs. */
	g1_rand(p[1]);
	g1_sub(p[0], p[0], p[1]);
	g1_norm(p[0], p[0]);
	g2_rand(q[1]);
	g2_sub(q[0], q[0], q[1]);
	g2_norm(q[0], q[0]);

	BENCH_RUN("pc_map_lcl") {
		BENCH_ADD(pc_map_lcl(d[0], e[0], p[0], q[0], t[0]));
		BENCH_ADD(pc_map_lcl(d[1], e[1], p[1], q[1], t[1]));
	} BENCH_DIV(2);

	BENCH_RUN("pc_map_bct") {
		BENCH_ADD(pc_map_bct(d, e));
	} BENCH_END;

	BENCH_RUN("pc_map_mpc") {
		BENCH_ADD(pc_map_mpc(r[0], d[0], e[0], t[0], 0));
		BENCH_ADD(pc_map_mpc(r[1], d[1], e[1], t[1], 1));
	} BENCH_DIV(2);

	g1_free(_p);
	g2_free(_q);
	gt_free(_r);
	bn_free(n);
	for (int j = 0; j < 2; j++) {
		g1_free(d[j]);
		g2_free(e[j]);
		bn_free(k[j]);
		bn_free(l[j]);
		g1_free(p[j]);
		g2_free(q[j]);
		gt_free(r[j]);
		gt_free(f[j]);
		pt_free(t[j]);
		mt_free(tri[j]);
	}
}

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	conf_print();
	util_banner("Benchmarks for the MPC module:", 0);
	util_banner("Utilities:\n", 0);

#if defined(WITH_BN)
	mul_triple();
#endif

#if defined(WITH_PC)
	if (pc_param_set_any() != RLC_OK) {
		RLC_THROW(ERR_NO_CURVE);
		core_clean();
		return 0;
	}

	pc_param_print();

	util_banner("Arithmetic:", 1);

	pair_triple();
#endif

	core_clean();
	return 0;
}
