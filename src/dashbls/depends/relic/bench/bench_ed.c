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
 * Benchmarks for arithmetic on Edwards elliptic curves.
 *
 * @ingroup bench
 */

#include <stdio.h>

#include "relic.h"
#include "relic_bench.h"

static void memory(void) {
	ed_t a[BENCH];

	BENCH_FEW("ed_null", ed_null(a[i]), 1);

	BENCH_FEW("ed_new", ed_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		ed_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		ed_new(a[i]);
	}
	BENCH_FEW("ed_free", ed_free(a[i]), 1);

	(void)a;
}

static void util(void) {
	ed_t p, q, t[4];
	uint8_t bin[2 * RLC_FP_BYTES + 1];
	int l;

	ed_null(p);
	ed_null(q);
	for (int j = 0; j < 4; j++) {
		ed_null(t[j]);
	}

	ed_new(p);
	ed_new(q);
	for (int j = 0; j < 4; j++) {
		ed_new(t[j]);
	}

	BENCH_RUN("ed_is_infty") {
		ed_rand(p);
		BENCH_ADD(ed_is_infty(p));
	} BENCH_END;

	BENCH_RUN("ed_set_infty") {
		ed_rand(p);
		BENCH_ADD(ed_set_infty(p));
	} BENCH_END;

	BENCH_RUN("ed_copy") {
		ed_rand(p);
		ed_rand(q);
		BENCH_ADD(ed_copy(p, q));
	} BENCH_END;

	BENCH_RUN("ed_cmp") {
		ed_rand(p);
		ed_dbl(p, p);
		ed_rand(q);
		ed_dbl(q, q);
		BENCH_ADD(ed_cmp(p, q));
	} BENCH_END;

	BENCH_RUN("ed_cmp (1 norm)") {
		ed_rand(p);
		ed_dbl(p, p);
		ed_rand(q);
		BENCH_ADD(ed_cmp(p, q));
	} BENCH_END;

	BENCH_RUN("ed_cmp (2 norm)") {
		ed_rand(p);
		ed_rand(q);
		BENCH_ADD(ed_cmp(p, q));
	} BENCH_END;

	BENCH_RUN("ed_rand") {
		BENCH_ADD(ed_rand(p));
	} BENCH_END;

	BENCH_RUN("ed_blind") {
		BENCH_ADD(ed_blind(p, p));
	} BENCH_END;

	BENCH_RUN("ed_rhs") {
		ed_rand(p);
		BENCH_ADD(ed_rhs(q->x, p));
	} BENCH_END;

	BENCH_RUN("ed_tab (4)") {
		ed_rand(p);
		BENCH_ADD(ed_tab(t, p, 4));
	} BENCH_END;

	BENCH_RUN("ed_on_curve") {
		ed_rand(p);
		BENCH_ADD(ed_on_curve(p));
	} BENCH_END;

	BENCH_RUN("ed_size_bin (0)") {
		ed_rand(p);
		BENCH_ADD(ed_size_bin(p, 0));
	} BENCH_END;

	BENCH_RUN("ed_size_bin (1)") {
		ed_rand(p);
		BENCH_ADD(ed_size_bin(p, 1));
	} BENCH_END;

	BENCH_RUN("ed_write_bin (0)") {
		ed_rand(p);
		l = ed_size_bin(p, 0);
		BENCH_ADD(ed_write_bin(bin, l, p, 0));
	} BENCH_END;

	BENCH_RUN("ed_write_bin (1)") {
		ed_rand(p);
		l = ed_size_bin(p, 1);
		BENCH_ADD(ed_write_bin(bin, l, p, 1));
	} BENCH_END;

	BENCH_RUN("ed_read_bin (0)") {
		ed_rand(p);
		l = ed_size_bin(p, 0);
		ed_write_bin(bin, l, p, 0);
		BENCH_ADD(ed_read_bin(p, bin, l));
	} BENCH_END;

	BENCH_RUN("ed_read_bin (1)") {
		ed_rand(p);
		l = ed_size_bin(p, 1);
		ed_write_bin(bin, l, p, 1);
		BENCH_ADD(ed_read_bin(p, bin, l));
	} BENCH_END;

	ed_free(p);
	ed_free(q);
	for (int j = 0; j < 4; j++) {
		ed_free(t[j]);
	}
}

static void arith(void) {
	ed_t p, q, r, t[RLC_ED_TABLE_MAX];
	bn_t k, l, n;

	ed_null(p);
	ed_null(q);
	ed_null(r);
	for (int i = 0; i < RLC_ED_TABLE_MAX; i++) {
		ed_null(t[i]);
	}

	ed_new(p);
	ed_new(q);
	ed_new(r);
	bn_new(k);
	bn_new(n);
	bn_new(l);

	ed_curve_get_ord(n);

	BENCH_RUN("ed_add") {
		ed_rand(p);
		ed_rand(q);
		ed_add(p, p, q);
		ed_rand(q);
		ed_rand(p);
		ed_add(q, q, p);
		BENCH_ADD(ed_add(r, p, q));
	} BENCH_END;

#if ED_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("ed_add_basic") {
		ed_rand(p);
		ed_rand(q);
		BENCH_ADD(ed_add_basic(r, p, q));
	} BENCH_END;
#endif

#if ED_ADD == PROJC || !defined(STRIP)
	BENCH_RUN("ed_add_projc") {
		ed_rand(p);
		ed_rand(q);
		ed_add_projc(p, p, q);
		ed_rand(q);
		ed_rand(p);
		ed_add_projc(q, q, p);
		BENCH_ADD(ed_add_projc(r, p, q));
	} BENCH_END;

	BENCH_RUN("ed_add_projc (z2 = 1)") {
		ed_rand(p);
		ed_rand(q);
		ed_add_projc(p, p, q);
		ed_rand(q);
		ed_norm(q, q);
		BENCH_ADD(ed_add_projc(r, p, q));
	} BENCH_END;

	BENCH_RUN("ed_add_projc (z1,z2 = 1)") {
		ed_rand(p);
		ed_norm(p, p);
		ed_rand(q);
		ed_norm(q, q);
		BENCH_ADD(ed_add_projc(r, p, q));
	} BENCH_END;
#endif

#if ED_ADD == EXTND || !defined(STRIP)
	BENCH_RUN("ed_add_extnd") {
		ed_rand(p);
		ed_rand(q);
		ed_add_extnd(p, p, q);
		ed_rand(q);
		ed_rand(p);
		ed_add_extnd(q, q, p);
		BENCH_ADD(ed_add_extnd(r, p, q));
	} BENCH_END;

	BENCH_RUN("ed_add_extnd (z2 = 1)") {
		ed_rand(p);
		ed_rand(q);
		ed_add_extnd(p, p, q);
		ed_rand(q);
		ed_norm(q, q);
		BENCH_ADD(ed_add_extnd(r, p, q));
	} BENCH_END;

	BENCH_RUN("ed_add_extnd (z1,z2 = 1)") {
		ed_rand(p);
		ed_norm(p, p);
		ed_rand(q);
		ed_norm(q, q);
		BENCH_ADD(ed_add_extnd(r, p, q));
	} BENCH_END;
#endif

	BENCH_RUN("ed_sub") {
		ed_rand(p);
		ed_rand(q);
		ed_add(p, p, q);
		ed_rand(q);
		ed_rand(p);
		ed_add(q, q, p);
		BENCH_ADD(ed_sub(r, p, q));
	} BENCH_END;

#if ED_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("ed_sub_basic") {
		ed_rand(p);
		ed_rand(q);
		BENCH_ADD(ed_sub_basic(r, p, q));
	} BENCH_END;
#endif

#if ED_ADD == PROJC || !defined(STRIP)
	BENCH_RUN("ed_sub_projc") {
		ed_rand(p);
		ed_rand(q);
		ed_add_projc(p, p, q);
		ed_rand(q);
		ed_rand(p);
		ed_add_projc(q, q, p);
		BENCH_ADD(ed_sub_projc(r, p, q));
	} BENCH_END;

	BENCH_RUN("ed_sub_projc (z2 = 1)") {
		ed_rand(p);
		ed_rand(q);
		ed_add_projc(p, p, q);
		ed_rand(q);
		ed_norm(q, q);
		BENCH_ADD(ed_sub_projc(r, p, q));
	} BENCH_END;

	BENCH_RUN("ed_sub_projc (z1,z2 = 1)") {
		ed_rand(p);
		ed_norm(p, p);
		ed_rand(q);
		ed_norm(q, q);
		BENCH_ADD(ed_sub_projc(r, p, q));
	} BENCH_END;
#endif

#if ED_ADD == EXTND || !defined(STRIP)
	BENCH_RUN("ed_sub_extnd") {
		ed_rand(p);
		ed_rand(q);
		ed_add_extnd(p, p, q);
		ed_rand(q);
		ed_rand(p);
		ed_add_extnd(q, q, p);
		BENCH_ADD(ed_sub_extnd(r, p, q));
	} BENCH_END;

	BENCH_RUN("ed_sub_projc (z2 = 1)") {
		ed_rand(p);
		ed_rand(q);
		ed_add_extnd(p, p, q);
		ed_rand(q);
		ed_norm(q, q);
		BENCH_ADD(ed_sub_extnd(r, p, q));
	} BENCH_END;

	BENCH_RUN("ed_sub_projc (z1,z2 = 1)") {
		ed_rand(p);
		ed_norm(p, p);
		ed_rand(q);
		ed_norm(q, q);
		BENCH_ADD(ed_sub_projc(r, p, q));
	} BENCH_END;
#endif

	BENCH_RUN("ed_dbl") {
		ed_rand(p);
		ed_rand(q);
		ed_add(p, p, q);
		BENCH_ADD(ed_dbl(r, p));
	} BENCH_END;

#if ED_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("ed_dbl_basic") {
		ed_rand(p);
		BENCH_ADD(ed_dbl_basic(r, p));
	} BENCH_END;
#endif

#if ED_ADD == PROJC || !defined(STRIP)
	BENCH_RUN("ed_dbl_projc") {
		ed_rand(p);
		ed_rand(q);
		ed_add_projc(p, p, q);
		BENCH_ADD(ed_dbl_projc(r, p));
	} BENCH_END;

	BENCH_RUN("ed_dbl_projc (z1 = 1)") {
		ed_rand(p);
		ed_norm(p, p);
		BENCH_ADD(ed_dbl_projc(r, p));
	} BENCH_END;
#endif

#if ED_ADD == PROJC || !defined(STRIP)
	BENCH_RUN("ed_dbl_extnd") {
		ed_rand(p);
		ed_rand(q);
		ed_add_extnd(p, p, q);
		BENCH_ADD(ed_dbl_extnd(r, p));
	} BENCH_END;

	BENCH_RUN("ed_dbl_extnd (z1 = 1)") {
		ed_rand(p);
		ed_norm(p, p);
		BENCH_ADD(ed_dbl_extnd(r, p));
	} BENCH_END;
#endif

	BENCH_RUN("ed_neg") {
		ed_rand(p);
		ed_rand(q);
		ed_add(p, p, q);
		BENCH_ADD(ed_neg(r, p));
	} BENCH_END;

#if ED_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("ed_neg_basic") {
		ed_rand(p);
		BENCH_ADD(ed_neg_basic(r, p));
	} BENCH_END;
#endif

#if ED_ADD == PROJC || !defined(STRIP)
	BENCH_RUN("ed_neg_projc") {
		ed_rand(p);
		ed_rand(q);
		ed_add_projc(p, p, q);
		BENCH_ADD(ed_neg_projc(r, p));
	} BENCH_END;
#endif

	BENCH_RUN("ed_mul") {
		bn_rand_mod(k, n);
		ed_rand(p);
		BENCH_ADD(ed_mul(q, p, k));
	} BENCH_END;

#if ED_MUL == BASIC || !defined(STRIP)
	BENCH_RUN("ed_mul_basic") {
		bn_rand_mod(k, n);
		BENCH_ADD(ed_mul_basic(q, p, k));
	} BENCH_END;
#endif

#if ED_MUL == SLIDE || !defined(STRIP)
	BENCH_RUN("ed_mul_slide") {
		bn_rand_mod(k, n);
		ed_rand(p);
		BENCH_ADD(ed_mul_slide(q, p, k));
	} BENCH_END;
#endif

#if ED_MUL == MONTY || !defined(STRIP)
	BENCH_RUN("ed_mul_monty") {
		bn_rand_mod(k, n);
		ed_rand(p);
		BENCH_ADD(ed_mul_monty(q, p, k));
	} BENCH_END;
#endif

#if ED_MUL == LWNAF || !defined(STRIP)
	BENCH_RUN("ed_mul_lwnaf") {
		bn_rand_mod(k, n);
		ed_rand(p);
		BENCH_ADD(ed_mul_lwnaf(q, p, k));
	} BENCH_END;
#endif

#if ED_MUL == LWREG || !defined(STRIP)
	BENCH_RUN("ed_mul_lwreg") {
		bn_rand_mod(k, n);
		ed_rand(p);
		BENCH_ADD(ed_mul_lwreg(q, p, k));
	} BENCH_END;
#endif

	BENCH_RUN("ed_mul_gen") {
		bn_rand_mod(k, n);
		BENCH_ADD(ed_mul_gen(q, k));
	} BENCH_END;

	BENCH_RUN("ed_mul_dig") {
		bn_rand(k, RLC_POS, RLC_DIG);
		bn_rand_mod(k, n);
		BENCH_ADD(ed_mul_dig(p, q, k->dp[0]));
	}
	BENCH_END;

	for (int i = 0; i < RLC_ED_TABLE; i++) {
		ed_new(t[i]);
	}

	BENCH_RUN("ed_mul_pre") {
		ed_rand(p);
		BENCH_ADD(ed_mul_pre(t, p));
	} BENCH_END;

	BENCH_RUN("ed_mul_fix") {
		bn_rand_mod(k, n);
		ed_rand(p);
		ed_mul_pre(t, p);
		BENCH_ADD(ed_mul_fix(q, (const ed_t *)t, k));
	} BENCH_END;

	for (int i = 0; i < RLC_ED_TABLE; i++) {
		ed_free(t[i]);
	}

#if ED_FIX == BASIC || !defined(STRIP)
	for (int i = 0; i < RLC_ED_TABLE_BASIC; i++) {
		ed_new(t[i]);
	}
	BENCH_RUN("ed_mul_pre_basic") {
		ed_rand(p);
		BENCH_ADD(ed_mul_pre_basic(t, p));
	} BENCH_END;

	BENCH_RUN("ed_mul_fix_basic") {
		bn_rand_mod(k, n);
		ed_rand(p);
		ed_mul_pre_basic(t, p);
		BENCH_ADD(ed_mul_fix_basic(q, (const ed_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RLC_ED_TABLE_BASIC; i++) {
		ed_free(t[i]);
	}
#endif

#if ED_FIX == COMBS || !defined(STRIP)
	for (int i = 0; i < RLC_ED_TABLE_COMBS; i++) {
		ed_new(t[i]);
	}
	BENCH_RUN("ed_mul_pre_combs") {
		ed_rand(p);
		BENCH_ADD(ed_mul_pre_combs(t, p));
	} BENCH_END;

	BENCH_RUN("ed_mul_fix_combs") {
		bn_rand_mod(k, n);
		ed_rand(p);
		ed_mul_pre_combs(t, p);
		BENCH_ADD(ed_mul_fix_combs(q, (const ed_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RLC_ED_TABLE_COMBS; i++) {
		ed_free(t[i]);
	}
#endif

#if ED_FIX == COMBD || !defined(STRIP)
	for (int i = 0; i < RLC_ED_TABLE_COMBD; i++) {
		ed_new(t[i]);
	}
	BENCH_RUN("ed_mul_pre_combd") {
		BENCH_ADD(ed_mul_pre_combd(t, p));
	} BENCH_END;

	BENCH_RUN("ed_mul_fix_combd") {
		bn_rand_mod(k, n);
		ed_mul_pre_combd(t, p);
		BENCH_ADD(ed_mul_fix_combd(q, (const ed_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RLC_ED_TABLE_COMBD; i++) {
		ed_free(t[i]);
	}
#endif

#if ED_FIX == LWNAF || !defined(STRIP)
	for (int i = 0; i < RLC_ED_TABLE_LWNAF; i++) {
		ed_new(t[i]);
	}
	BENCH_RUN("ed_mul_pre_lwnaf") {
		ed_rand(p);
		BENCH_ADD(ed_mul_pre_lwnaf(t, p));
	} BENCH_END;

	BENCH_RUN("ed_mul_fix_lwnaf") {
		bn_rand_mod(k, n);
		ed_rand(p);
		ed_mul_pre_lwnaf(t, p);
		BENCH_ADD(ed_mul_fix_lwnaf(q, (const ed_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RLC_ED_TABLE_LWNAF; i++) {
		ed_free(t[i]);
	}
#endif
	BENCH_RUN("ed_mul_sim") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		ed_rand(p);
		ed_rand(q);
		BENCH_ADD(ed_mul_sim(r, p, k, q, l));
	} BENCH_END;

#if ED_SIM == BASIC || !defined(STRIP)
	BENCH_RUN("ed_mul_sim_basic") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		ed_rand(p);
		ed_rand(q);
		BENCH_ADD(ed_mul_sim_basic(r, p, k, q, l));
	} BENCH_END;
#endif

#if ED_SIM == TRICK || !defined(STRIP)
	BENCH_RUN("ed_mul_sim_trick") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		ed_rand(p);
		ed_rand(q);
		BENCH_ADD(ed_mul_sim_trick(r, p, k, q, l));
	} BENCH_END;
#endif

#if ED_SIM == INTER || !defined(STRIP)
	BENCH_RUN("ed_mul_sim_inter") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		ed_rand(p);
		ed_rand(q);
		BENCH_ADD(ed_mul_sim_inter(r, p, k, q, l));
	} BENCH_END;
#endif

#if ED_SIM == JOINT || !defined(STRIP)
	BENCH_RUN("ed_mul_sim_joint") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		ed_rand(p);
		ed_rand(q);
		BENCH_ADD(ed_mul_sim_joint(r, p, k, q, l));
	} BENCH_END;
#endif

	BENCH_RUN("ed_mul_sim_gen") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		ed_rand(q);
		BENCH_ADD(ed_mul_sim_gen(r, k, q, l));
	} BENCH_END;

	BENCH_RUN("ed_map") {
		uint8_t msg[5];
		rand_bytes(msg, 5);
		BENCH_ADD(ed_map(p, msg, 5));
	} BENCH_END;

	BENCH_RUN("ed_pck") {
		ed_rand(p);
		BENCH_ADD(ed_pck(q, p));
	} BENCH_END;

	BENCH_RUN("ed_upk") {
		ed_rand(p);
		BENCH_ADD(ed_upk(q, p));
	} BENCH_END;

	ed_free(p);
	ed_free(q);
	ed_free(r);
	bn_free(k);
	bn_free(l);
	bn_free(n);
}

static void bench(void) {
	ed_param_print();
	util_banner("Utilities:", 1);
	memory();
	util();
	util_banner("Arithmetic:", 1);
	arith();
}

int main(void) {
	int r0 = RLC_ERR;

	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	conf_print();
	util_banner("Benchmarks for the ED module:", 0);
	r0 = ed_param_set_any();
	if (r0 == RLC_OK) {
		bench();
	}

	if (r0 == RLC_ERR) {
		if (ed_param_set_any() == RLC_ERR) {
			RLC_THROW(ERR_NO_CURVE);
		}
	}

	core_clean();
	return 0;
}
