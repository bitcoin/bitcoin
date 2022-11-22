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
 * Benchmarks for arithmetic on prime elliptic curves.
 *
 * @ingroup bench
 */

#include <stdio.h>

#include "relic.h"
#include "relic_bench.h"

static void memory(void) {
	ep_t a[BENCH];

	BENCH_FEW("ep_null", ep_null(a[i]), 1);

	BENCH_FEW("ep_new", ep_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		ep_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		ep_new(a[i]);
	}
	BENCH_FEW("ep_free", ep_free(a[i]), 1);

	(void)a;
}

static void util(void) {
	ep_t p, q, t[4];
	uint8_t bin[2 * RLC_FP_BYTES + 1];
	int l;

	ep_null(p);
	ep_null(q);
	for (int j = 0; j < 4; j++) {
		ep_null(t[j]);
	}

	ep_new(p);
	ep_new(q);
	for (int j = 0; j < 4; j++) {
		ep_new(t[j]);
	}

	BENCH_RUN("ep_is_infty") {
		ep_rand(p);
		BENCH_ADD(ep_is_infty(p));
	} BENCH_END;

	BENCH_RUN("ep_set_infty") {
		ep_rand(p);
		BENCH_ADD(ep_set_infty(p));
	} BENCH_END;

	BENCH_RUN("ep_copy") {
		ep_rand(p);
		ep_rand(q);
		BENCH_ADD(ep_copy(p, q));
	} BENCH_END;

	BENCH_RUN("ep_norm") {
		ep_rand(p);
		ep_dbl(p, p);
		BENCH_ADD(ep_norm(p, p));
	} BENCH_END;

	BENCH_RUN("ep_norm_sim (2)") {
		ep_rand(t[0]);
		ep_rand(t[1]);
		ep_dbl(t[0], t[0]);
		ep_dbl(t[1], t[1]);
		BENCH_ADD(ep_norm_sim(t, t, 2));
	} BENCH_END;

	BENCH_RUN("ep_cmp") {
		ep_rand(p);
		ep_dbl(p, p);
		ep_rand(q);
		ep_dbl(q, q);
		BENCH_ADD(ep_cmp(p, q));
	} BENCH_END;

	BENCH_RUN("ep_cmp (1 norm)") {
		ep_rand(p);
		ep_dbl(p, p);
		ep_rand(q);
		BENCH_ADD(ep_cmp(p, q));
	} BENCH_END;

	BENCH_RUN("ep_cmp (2 norm)") {
		ep_rand(p);
		ep_rand(q);
		BENCH_ADD(ep_cmp(p, q));
	} BENCH_END;

	BENCH_RUN("ep_rand") {
		BENCH_ADD(ep_rand(p));
	} BENCH_END;

	BENCH_RUN("ep_blind") {
		BENCH_ADD(ep_blind(p, p));
	} BENCH_END;

	BENCH_RUN("ep_rhs") {
		ep_rand(p);
		BENCH_ADD(ep_rhs(q->x, p));
	} BENCH_END;

	BENCH_RUN("ep_tab (4)") {
		ep_rand(p);
		BENCH_ADD(ep_tab(t, p, 4));
	} BENCH_END;

	BENCH_RUN("ep_on_curve") {
		ep_rand(p);
		BENCH_ADD(ep_on_curve(p));
	} BENCH_END;

	BENCH_RUN("ep_size_bin (0)") {
		ep_rand(p);
		BENCH_ADD(ep_size_bin(p, 0));
	} BENCH_END;

	BENCH_RUN("ep_size_bin (1)") {
		ep_rand(p);
		BENCH_ADD(ep_size_bin(p, 1));
	} BENCH_END;

	BENCH_RUN("ep_write_bin (0)") {
		ep_rand(p);
		l = ep_size_bin(p, 0);
		BENCH_ADD(ep_write_bin(bin, l, p, 0));
	} BENCH_END;

	BENCH_RUN("ep_write_bin (1)") {
		ep_rand(p);
		l = ep_size_bin(p, 1);
		BENCH_ADD(ep_write_bin(bin, l, p, 1));
	} BENCH_END;

	BENCH_RUN("ep_read_bin (0)") {
		ep_rand(p);
		l = ep_size_bin(p, 0);
		ep_write_bin(bin, l, p, 0);
		BENCH_ADD(ep_read_bin(p, bin, l));
	} BENCH_END;

	BENCH_RUN("ep_read_bin (1)") {
		ep_rand(p);
		l = ep_size_bin(p, 1);
		ep_write_bin(bin, l, p, 1);
		BENCH_ADD(ep_read_bin(p, bin, l));
	} BENCH_END;

	ep_free(p);
	ep_free(q);
	for (int j = 0; j < 4; j++) {
		ep_free(t[j]);
	}
}

static void arith(void) {
	ep_t p, q, r, t[RLC_EP_TABLE_MAX];
	bn_t k, l[2], n;

	ep_null(p);
	ep_null(q);
	ep_null(r);
	for (int i = 0; i < RLC_EP_TABLE_MAX; i++) {
		ep_null(t[i]);
	}

	ep_new(p);
	ep_new(q);
	ep_new(r);
	bn_new(k);
	bn_new(n);
	bn_new(l[0]);
	bn_new(l[1]);

	ep_curve_get_ord(n);

	BENCH_RUN("ep_add") {
		ep_rand(p);
		ep_rand(q);
		ep_add(p, p, q);
		ep_rand(q);
		ep_rand(p);
		ep_add(q, q, p);
		BENCH_ADD(ep_add(r, p, q));
	} BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("ep_add_basic") {
		ep_rand(p);
		ep_rand(q);
		BENCH_ADD(ep_add_basic(r, p, q));
	} BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)
	BENCH_RUN("ep_add_projc") {
		ep_rand(p);
		ep_rand(q);
		ep_add_projc(p, p, q);
		ep_rand(q);
		ep_rand(p);
		ep_add_projc(q, q, p);
		BENCH_ADD(ep_add_projc(r, p, q));
	} BENCH_END;

	BENCH_RUN("ep_add_projc (z2 = 1)") {
		ep_rand(p);
		ep_rand(q);
		ep_add_projc(p, p, q);
		ep_rand(q);
		ep_norm(q, q);
		BENCH_ADD(ep_add_projc(r, p, q));
	} BENCH_END;

	BENCH_RUN("ep_add_projc (z1,z2 = 1)") {
		ep_rand(p);
		ep_norm(p, p);
		ep_rand(q);
		ep_norm(q, q);
		BENCH_ADD(ep_add_projc(r, p, q));
	} BENCH_END;
#endif

#if EP_ADD == JACOB || !defined(STRIP)
	BENCH_RUN("ep_add_jacob") {
		ep_rand(p);
		ep_rand(q);
		ep_add_jacob(p, p, q);
		ep_rand(q);
		ep_rand(p);
		ep_add_jacob(q, q, p);
		BENCH_ADD(ep_add_jacob(r, p, q));
	} BENCH_END;

	BENCH_RUN("ep_add_jacob (z2 = 1)") {
		ep_rand(p);
		ep_rand(q);
		ep_add_jacob(p, p, q);
		ep_rand(q);
		ep_norm(q, q);
		BENCH_ADD(ep_add_jacob(r, p, q));
	} BENCH_END;

	BENCH_RUN("ep_add_jacob (z1,z2 = 1)") {
		ep_rand(p);
		ep_norm(p, p);
		ep_rand(q);
		ep_norm(q, q);
		BENCH_ADD(ep_add_jacob(r, p, q));
	} BENCH_END;
#endif

	BENCH_RUN("ep_sub") {
		ep_rand(p);
		ep_rand(q);
		ep_add(p, p, q);
		ep_rand(q);
		ep_rand(p);
		ep_add(q, q, p);
		BENCH_ADD(ep_sub(r, p, q));
	} BENCH_END;

	BENCH_RUN("ep_dbl") {
		ep_rand(p);
		ep_rand(q);
		ep_add(p, p, q);
		BENCH_ADD(ep_dbl(r, p));
	} BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("ep_dbl_basic") {
		ep_rand(p);
		BENCH_ADD(ep_dbl_basic(r, p));
	} BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)
	BENCH_RUN("ep_dbl_projc") {
		ep_rand(p);
		ep_rand(q);
		ep_add_projc(p, p, q);
		BENCH_ADD(ep_dbl_projc(r, p));
	} BENCH_END;

	BENCH_RUN("ep_dbl_projc (z1 = 1)") {
		ep_rand(p);
		ep_norm(p, p);
		BENCH_ADD(ep_dbl_projc(r, p));
	} BENCH_END;
#endif

#if EP_ADD == JACOB || !defined(STRIP)
	BENCH_RUN("ep_dbl_jacob") {
		ep_rand(p);
		ep_rand(q);
		ep_add_jacob(p, p, q);
		BENCH_ADD(ep_dbl_jacob(r, p));
	} BENCH_END;

	BENCH_RUN("ep_dbl_jacob (z1 = 1)") {
		ep_rand(p);
		ep_norm(p, p);
		BENCH_ADD(ep_dbl_jacob(r, p));
	} BENCH_END;
#endif

	BENCH_RUN("ep_neg") {
		ep_rand(p);
		ep_rand(q);
		ep_add(p, p, q);
		BENCH_ADD(ep_neg(r, p));
	} BENCH_END;

	BENCH_RUN("ep_mul") {
		bn_rand_mod(k, n);
		ep_rand(p);
		BENCH_ADD(ep_mul(q, p, k));
	} BENCH_END;

#if EP_MUL == BASIC || !defined(STRIP)
	BENCH_RUN("ep_mul_basic") {
		bn_rand_mod(k, n);
		BENCH_ADD(ep_mul_basic(q, p, k));
	} BENCH_END;
#endif

#if EP_MUL == SLIDE || !defined(STRIP)
	BENCH_RUN("ep_mul_slide") {
		bn_rand_mod(k, n);
		ep_rand(p);
		BENCH_ADD(ep_mul_slide(q, p, k));
	} BENCH_END;
#endif

#if EP_MUL == MONTY || !defined(STRIP)
	BENCH_RUN("ep_mul_monty") {
		bn_rand_mod(k, n);
		ep_rand(p);
		BENCH_ADD(ep_mul_monty(q, p, k));
	} BENCH_END;
#endif

#if EP_MUL == LWNAF || !defined(STRIP)
	BENCH_RUN("ep_mul_lwnaf") {
		bn_rand_mod(k, n);
		ep_rand(p);
		BENCH_ADD(ep_mul_lwnaf(q, p, k));
	} BENCH_END;
#endif

#if EP_MUL == LWREG || !defined(STRIP)
	BENCH_RUN("ep_mul_lwreg") {
		bn_rand_mod(k, n);
		ep_rand(p);
		BENCH_ADD(ep_mul_lwreg(q, p, k));
	} BENCH_END;
#endif

	BENCH_RUN("ep_mul_gen") {
		bn_rand_mod(k, n);
		BENCH_ADD(ep_mul_gen(q, k));
	} BENCH_END;

	BENCH_RUN("ep_mul_dig") {
		bn_rand(k, RLC_POS, RLC_DIG);
		bn_rand_mod(k, n);
		BENCH_ADD(ep_mul_dig(p, q, k->dp[0]));
	}
	BENCH_END;

	for (int i = 0; i < RLC_EP_TABLE; i++) {
		ep_new(t[i]);
	}

	BENCH_RUN("ep_mul_pre") {
		ep_rand(p);
		BENCH_ADD(ep_mul_pre(t, p));
	} BENCH_END;

	BENCH_RUN("ep_mul_fix") {
		bn_rand_mod(k, n);
		ep_rand(p);
		ep_mul_pre(t, p);
		BENCH_ADD(ep_mul_fix(q, (const ep_t *)t, k));
	} BENCH_END;

	for (int i = 0; i < RLC_EP_TABLE; i++) {
		ep_free(t[i]);
	}

#if EP_FIX == BASIC || !defined(STRIP)
	for (int i = 0; i < RLC_EP_TABLE_BASIC; i++) {
		ep_new(t[i]);
	}
	BENCH_RUN("ep_mul_pre_basic") {
		ep_rand(p);
		BENCH_ADD(ep_mul_pre_basic(t, p));
	} BENCH_END;

	BENCH_RUN("ep_mul_fix_basic") {
		bn_rand_mod(k, n);
		ep_rand(p);
		ep_mul_pre_basic(t, p);
		BENCH_ADD(ep_mul_fix_basic(q, (const ep_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RLC_EP_TABLE_BASIC; i++) {
		ep_free(t[i]);
	}
#endif

#if EP_FIX == COMBS || !defined(STRIP)
	for (int i = 0; i < RLC_EP_TABLE_COMBS; i++) {
		ep_new(t[i]);
	}
	BENCH_RUN("ep_mul_pre_combs") {
		ep_rand(p);
		BENCH_ADD(ep_mul_pre_combs(t, p));
	} BENCH_END;

	BENCH_RUN("ep_mul_fix_combs") {
		bn_rand_mod(k, n);
		ep_rand(p);
		ep_mul_pre_combs(t, p);
		BENCH_ADD(ep_mul_fix_combs(q, (const ep_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RLC_EP_TABLE_COMBS; i++) {
		ep_free(t[i]);
	}
#endif

#if EP_FIX == COMBD || !defined(STRIP)
	for (int i = 0; i < RLC_EP_TABLE_COMBD; i++) {
		ep_new(t[i]);
	}
	BENCH_RUN("ep_mul_pre_combd") {
		BENCH_ADD(ep_mul_pre_combd(t, p));
	} BENCH_END;

	BENCH_RUN("ep_mul_fix_combd") {
		bn_rand_mod(k, n);
		ep_mul_pre_combd(t, p);
		BENCH_ADD(ep_mul_fix_combd(q, (const ep_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RLC_EP_TABLE_COMBD; i++) {
		ep_free(t[i]);
	}
#endif

#if EP_FIX == LWNAF || !defined(STRIP)
	for (int i = 0; i < RLC_EP_TABLE_LWNAF; i++) {
		ep_new(t[i]);
	}
	BENCH_RUN("ep_mul_pre_lwnaf") {
		ep_rand(p);
		BENCH_ADD(ep_mul_pre_lwnaf(t, p));
	} BENCH_END;

	BENCH_RUN("ep_mul_fix_lwnaf") {
		bn_rand_mod(k, n);
		ep_rand(p);
		ep_mul_pre_lwnaf(t, p);
		BENCH_ADD(ep_mul_fix_lwnaf(q, (const ep_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RLC_EP_TABLE_LWNAF; i++) {
		ep_free(t[i]);
	}
#endif
	BENCH_RUN("ep_mul_sim") {
		bn_rand_mod(l[0], n);
		bn_rand_mod(l[1], n);
		ep_rand(p);
		ep_rand(q);
		BENCH_ADD(ep_mul_sim(r, p, l[0], q, l[1]));
	} BENCH_END;

#if EP_SIM == BASIC || !defined(STRIP)
	BENCH_RUN("ep_mul_sim_basic") {
		bn_rand_mod(l[0], n);
		bn_rand_mod(l[1], n);
		ep_rand(p);
		ep_rand(q);
		BENCH_ADD(ep_mul_sim_basic(r, p, l[0], q, l[1]));
	} BENCH_END;
#endif

#if EP_SIM == TRICK || !defined(STRIP)
	BENCH_RUN("ep_mul_sim_trick") {
		bn_rand_mod(l[0], n);
		bn_rand_mod(l[1], n);
		ep_rand(p);
		ep_rand(q);
		BENCH_ADD(ep_mul_sim_trick(r, p, l[0], q, l[1]));
	} BENCH_END;
#endif

#if EP_SIM == INTER || !defined(STRIP)
	BENCH_RUN("ep_mul_sim_inter") {
		bn_rand_mod(l[0], n);
		bn_rand_mod(l[1], n);
		ep_rand(p);
		ep_rand(q);
		BENCH_ADD(ep_mul_sim_inter(r, p, l[0], q, l[1]));
	} BENCH_END;
#endif

#if EP_SIM == JOINT || !defined(STRIP)
	BENCH_RUN("ep_mul_sim_joint") {
		bn_rand_mod(l[0], n);
		bn_rand_mod(l[1], n);
		ep_rand(p);
		ep_rand(q);
		BENCH_ADD(ep_mul_sim_joint(r, p, l[0], q, l[1]));
	} BENCH_END;
#endif

	BENCH_RUN("ep_mul_sim_gen") {
		bn_rand_mod(l[0], n);
		bn_rand_mod(l[1], n);
		ep_rand(q);
		BENCH_ADD(ep_mul_sim_gen(r, l[0], q, l[1]));
	} BENCH_END;

	for (int i = 0; i < 2; i++) {
		ep_new(t[i]);
	}

	BENCH_RUN("ep_mul_sim_lot (2)") {
		bn_rand_mod(l[0], n);
		bn_rand_mod(l[1], n);
		ep_rand(t[0]);
		ep_rand(t[1]);
		BENCH_ADD(ep_mul_sim_lot(r, t, l, 2));
	} BENCH_END;

	for (int i = 0; i < 2; i++) {
		ep_free(t[i]);
	}

	BENCH_RUN("ep_map") {
		uint8_t msg[5];
		rand_bytes(msg, 5);
		BENCH_ADD(ep_map(p, msg, 5));
	} BENCH_END;

	BENCH_RUN("ep_pck") {
		ep_rand(p);
		BENCH_ADD(ep_pck(q, p));
	} BENCH_END;

	BENCH_RUN("ep_upk") {
		ep_rand(p);
		BENCH_ADD(ep_upk(q, p));
	} BENCH_END;

	ep_free(p);
	ep_free(q);
	ep_free(r);
	bn_free(k);
	bn_free(l[0]);
	bn_free(l[1]);
	bn_free(n);
}

static void bench(void) {
	ep_param_print();
	util_banner("Utilities:", 1);
	memory();
	util();
	util_banner("Arithmetic:", 1);
	arith();
}

int main(void) {
	int r0 = RLC_ERR, r1 = RLC_ERR, r2 = RLC_ERR, r3 = RLC_ERR;;

	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	conf_print();
	util_banner("Benchmarks for the EP module:", 0);

#if defined(EP_PLAIN)
	r0 = ep_param_set_any_plain();
	if (r0 == RLC_OK) {
		bench();
	}
#endif

#if defined(EP_ENDOM)
	r1 = ep_param_set_any_endom();
	if (r1 == RLC_OK) {
		bench();
	}
#endif

	r2 = ep_param_set_any_pairf();
	if (r2 == RLC_OK) {
		bench();
	}

#if defined(EP_SUPER)
	r3 = ep_param_set_any_super();
	if (r3 == RLC_OK) {
		bench();
	}
#endif

	if (r0 == RLC_ERR && r1 == RLC_ERR && r2 == RLC_ERR && r3 == RLC_ERR) {
		if (ep_param_set_any() == RLC_ERR) {
			RLC_THROW(ERR_NO_CURVE);
		}
	}

	core_clean();
	return 0;
}
