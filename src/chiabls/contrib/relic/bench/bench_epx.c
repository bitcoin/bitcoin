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
 * Benchmarks for elliptic curves defined over extensions of prime fields.
 *
 * @ingroup bench
 */

#include <stdio.h>

#include "relic.h"
#include "relic_bench.h"

static void memory(void) {
	ep2_t a[BENCH];

	BENCH_SMALL("ep2_null", ep2_null(a[i]));

	BENCH_SMALL("ep2_new", ep2_new(a[i]));
	for (int i = 0; i < BENCH; i++) {
		ep2_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		ep2_new(a[i]);
	}
	BENCH_SMALL("ep2_free", ep2_free(a[i]));

	(void)a;
}

static void util(void) {
	ep2_t p, q;
	uint8_t bin[4 * FP_BYTES + 1];
	int l;

	ep2_null(p);
	ep2_null(q);

	ep2_new(p);
	ep2_new(q);

	BENCH_BEGIN("ep2_is_infty") {
		ep2_rand(p);
		BENCH_ADD(ep2_is_infty(p));
	}
	BENCH_END;

	BENCH_BEGIN("ep2_set_infty") {
		ep2_rand(p);
		BENCH_ADD(ep2_set_infty(p));
	}
	BENCH_END;

	BENCH_BEGIN("ep2_copy") {
		ep2_rand(p);
		ep2_rand(q);
		BENCH_ADD(ep2_copy(p, q));
	}
	BENCH_END;

	BENCH_BEGIN("ep2_cmp") {
		ep2_rand(p);
		ep2_dbl(p, p);
		ep2_rand(q);
		ep2_dbl(q, q);
		BENCH_ADD(ep2_cmp(p, q));
	} BENCH_END;

	BENCH_BEGIN("ep2_cmp (1 norm)") {
		ep2_rand(p);
		ep2_dbl(p, p);
		ep2_rand(q);
		BENCH_ADD(ep2_cmp(p, q));
	} BENCH_END;

	BENCH_BEGIN("ep2_cmp (2 norm)") {
		ep2_rand(p);
		ep2_rand(q);
		BENCH_ADD(ep2_cmp(p, q));
	} BENCH_END;

	BENCH_BEGIN("ep2_rand") {
		BENCH_ADD(ep2_rand(p));
	}
	BENCH_END;

	BENCH_BEGIN("ep2_is_valid") {
		ep2_rand(p);
		BENCH_ADD(ep2_is_valid(p));
	} BENCH_END;

	BENCH_BEGIN("ep2_size_bin (0)") {
		ep2_rand(p);
		BENCH_ADD(ep2_size_bin(p, 0));
	} BENCH_END;

	BENCH_BEGIN("ep2_size_bin (1)") {
		ep2_rand(p);
		BENCH_ADD(ep2_size_bin(p, 1));
	} BENCH_END;

	BENCH_BEGIN("ep2_write_bin (0)") {
		ep2_rand(p);
		l = ep2_size_bin(p, 0);
		BENCH_ADD(ep2_write_bin(bin, l, p, 0));
	} BENCH_END;

	BENCH_BEGIN("ep2_write_bin (1)") {
		ep2_rand(p);
		l = ep2_size_bin(p, 1);
		BENCH_ADD(ep2_write_bin(bin, l, p, 1));
	} BENCH_END;

	BENCH_BEGIN("ep2_read_bin (0)") {
		ep2_rand(p);
		l = ep2_size_bin(p, 0);
		ep2_write_bin(bin, l, p, 0);
		BENCH_ADD(ep2_read_bin(p, bin, l));
	} BENCH_END;

	BENCH_BEGIN("ep2_read_bin (1)") {
		ep2_rand(p);
		l = ep2_size_bin(p, 1);
		ep2_write_bin(bin, l, p, 1);
		BENCH_ADD(ep2_read_bin(p, bin, l));
	} BENCH_END;

	ep2_free(p);
	ep2_free(q);
}

static void arith(void) {
	ep2_t p, q, r, t[RELIC_EPX_TABLE_MAX];
	bn_t k, n, l;
	fp2_t s;

	ep2_null(p);
	ep2_null(q);
	ep2_null(r);
	bn_null(k);
	bn_null(n);
	fp2_null(s);
	for (int i = 0; i < RELIC_EPX_TABLE_MAX; i++) {
		ep2_null(t[i]);
	}

	ep2_new(p);
	ep2_new(q);
	ep2_new(r);
	bn_new(k);
	bn_new(n);
	bn_new(l);
	fp2_new(s);

	ep2_curve_get_ord(n);

	BENCH_BEGIN("ep2_add") {
		ep2_rand(p);
		ep2_rand(q);
		ep2_add(p, p, q);
		ep2_rand(q);
		ep2_rand(p);
		ep2_add(q, q, p);
		BENCH_ADD(ep2_add(r, p, q));
	}
	BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_BEGIN("ep2_add_basic") {
		ep2_rand(p);
		ep2_rand(q);
		BENCH_ADD(ep2_add_basic(r, p, q));
	}
	BENCH_END;

	BENCH_BEGIN("ep2_add_slp_basic") {
		ep2_rand(p);
		ep2_rand(q);
		BENCH_ADD(ep2_add_slp_basic(r, s, p, q));
	}
	BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)
	BENCH_BEGIN("ep2_add_projc") {
		ep2_rand(p);
		ep2_rand(q);
		ep2_add_projc(p, p, q);
		ep2_rand(q);
		ep2_rand(p);
		ep2_add_projc(q, q, p);
		BENCH_ADD(ep2_add_projc(r, p, q));
	}
	BENCH_END;

	BENCH_BEGIN("ep2_add_projc (z2 = 1)") {
		ep2_rand(p);
		ep2_rand(q);
		ep2_add_projc(p, p, q);
		ep2_rand(q);
		ep2_norm(q, q);
		BENCH_ADD(ep2_add_projc(r, p, q));
	}
	BENCH_END;

	BENCH_BEGIN("ep2_add_projc (z1,z2 = 1)") {
		ep2_rand(p);
		ep2_norm(p, p);
		ep2_rand(q);
		ep2_norm(q, q);
		BENCH_ADD(ep2_add_projc(r, p, q));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("ep2_sub") {
		ep2_rand(p);
		ep2_rand(q);
		ep2_add(p, p, q);
		ep2_rand(q);
		ep2_rand(p);
		ep2_add(q, q, p);
		BENCH_ADD(ep2_sub(r, p, q));
	}
	BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_BEGIN("ep2_sub_basic") {
		ep2_rand(p);
		ep2_rand(q);
		BENCH_ADD(ep2_sub_basic(r, p, q));
	}
	BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)
	BENCH_BEGIN("ep2_sub_projc") {
		ep2_rand(p);
		ep2_rand(q);
		ep2_add_projc(p, p, q);
		ep2_rand(q);
		ep2_rand(p);
		ep2_add_projc(q, q, p);
		BENCH_ADD(ep2_sub_projc(r, p, q));
	}
	BENCH_END;

	BENCH_BEGIN("ep2_sub_projc (z2 = 1)") {
		ep2_rand(p);
		ep2_rand(q);
		ep2_add_projc(p, p, q);
		ep2_rand(q);
		ep2_norm(q, q);
		BENCH_ADD(ep2_sub_projc(r, p, q));
	}
	BENCH_END;

	BENCH_BEGIN("ep2_sub_projc (z1,z2 = 1)") {
		ep2_rand(p);
		ep2_norm(p, p);
		ep2_rand(q);
		ep2_norm(q, q);
		BENCH_ADD(ep2_sub_projc(r, p, q));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("ep2_dbl") {
		ep2_rand(p);
		ep2_rand(q);
		ep2_add(p, p, q);
		BENCH_ADD(ep2_dbl(r, p));
	}
	BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_BEGIN("ep2_dbl_basic") {
		ep2_rand(p);
		BENCH_ADD(ep2_dbl_basic(r, p));
	}
	BENCH_END;

	BENCH_BEGIN("ep2_dbl_slp_basic") {
		ep2_rand(p);
		BENCH_ADD(ep2_dbl_slp_basic(r, s, p));
	}
	BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)
	BENCH_BEGIN("ep2_dbl_projc") {
		ep2_rand(p);
		ep2_rand(q);
		ep2_add_projc(p, p, q);
		BENCH_ADD(ep2_dbl_projc(r, p));
	}
	BENCH_END;

	BENCH_BEGIN("ep2_dbl_projc (z1 = 1)") {
		ep2_rand(p);
		ep2_norm(p, p);
		BENCH_ADD(ep2_dbl_projc(r, p));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("ep2_neg") {
		ep2_rand(p);
		ep2_rand(q);
		ep2_add(p, p, q);
		BENCH_ADD(ep2_neg(r, p));
	}
	BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_BEGIN("ep2_neg_basic") {
		ep2_rand(p);
		BENCH_ADD(ep2_neg_basic(r, p));
	}
	BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)
	BENCH_BEGIN("ep2_neg_projc") {
		ep2_rand(p);
		ep2_rand(q);
		ep2_add_projc(p, p, q);
		BENCH_ADD(ep2_neg_projc(r, p));
	}
	BENCH_END;
#endif

	BENCH_BEGIN("ep2_mul") {
		bn_rand_mod(k, n);
		BENCH_ADD(ep2_mul(q, p, k));
	} BENCH_END;

#if EP_MUL == BASIC || !defined(STRIP)
	BENCH_BEGIN("ep2_mul_basic") {
		bn_rand_mod(k, n);
		BENCH_ADD(ep2_mul_basic(q, p, k));
	} BENCH_END;
#endif

#if EP_MUL == SLIDE || !defined(STRIP)
	BENCH_BEGIN("ep2_mul_slide") {
		bn_rand_mod(k, n);
		ep2_rand(p);
		BENCH_ADD(ep2_mul_slide(q, p, k));
	} BENCH_END;
#endif

#if EP_MUL == MONTY || !defined(STRIP)
	BENCH_BEGIN("ep2_mul_monty") {
		bn_rand_mod(k, n);
		ep2_rand(p);
		BENCH_ADD(ep2_mul_monty(q, p, k));
	} BENCH_END;
#endif

#if EP_MUL == LWNAF || !defined(STRIP)
	BENCH_BEGIN("ep2_mul_lwnaf") {
		bn_rand_mod(k, n);
		ep2_rand(p);
		BENCH_ADD(ep2_mul_lwnaf(q, p, k));
	} BENCH_END;
#endif

	BENCH_BEGIN("ep2_mul_gen") {
		bn_rand_mod(k, n);
		BENCH_ADD(ep2_mul_gen(q, k));
	} BENCH_END;

	BENCH_BEGIN("ep2_mul_dig") {
		bn_rand(k, BN_POS, BN_DIGIT);
		bn_rand_mod(k, n);
		BENCH_ADD(ep2_mul_dig(p, q, k->dp[0]));
	}
	BENCH_END;

	for (int i = 0; i < RELIC_EPX_TABLE_MAX; i++) {
		ep2_new(t[i]);
	}

	BENCH_BEGIN("ep2_mul_pre") {
		ep2_rand(p);
		BENCH_ADD(ep2_mul_pre(t, p));
	} BENCH_END;

	BENCH_BEGIN("ep2_mul_fix") {
		bn_rand_mod(k, n);
		ep2_rand(p);
		ep2_mul_pre(t, p);
		BENCH_ADD(ep2_mul_fix(q, t, k));
	} BENCH_END;

	for (int i = 0; i < RELIC_EPX_TABLE_MAX; i++) {
		ep2_free(t[i]);
	}

#if EP_FIX == BASIC || !defined(STRIP)
	for (int i = 0; i < RELIC_EPX_TABLE_BASIC; i++) {
		ep2_new(t[i]);
	}
	BENCH_BEGIN("ep2_mul_pre_basic") {
		ep2_rand(p);
		BENCH_ADD(ep2_mul_pre_basic(t, p));
	} BENCH_END;

	BENCH_BEGIN("ep2_mul_fix_basic") {
		bn_rand_mod(k, n);
		ep2_rand(p);
		ep2_mul_pre_basic(t, p);
		BENCH_ADD(ep2_mul_fix_basic(q, t, k));
	} BENCH_END;
	for (int i = 0; i < RELIC_EPX_TABLE_BASIC; i++) {
		ep2_free(t[i]);
	}
#endif

#if EP_FIX == YAOWI || !defined(STRIP)
	for (int i = 0; i < RELIC_EPX_TABLE_YAOWI; i++) {
		ep2_new(t[i]);
	}
	BENCH_BEGIN("ep2_mul_pre_yaowi") {
		ep2_rand(p);
		BENCH_ADD(ep2_mul_pre_yaowi(t, p));
	} BENCH_END;

	BENCH_BEGIN("ep2_mul_fix_yaowi") {
		bn_rand_mod(k, n);
		ep2_rand(p);
		ep2_mul_pre_yaowi(t, p);
		BENCH_ADD(ep2_mul_fix_yaowi(q, t, k));
	} BENCH_END;
	for (int i = 0; i < RELIC_EPX_TABLE_YAOWI; i++) {
		ep2_free(t[i]);
	}
#endif

#if EP_FIX == NAFWI || !defined(STRIP)
	for (int i = 0; i < RELIC_EPX_TABLE_NAFWI; i++) {
		ep2_new(t[i]);
	}
	BENCH_BEGIN("ep2_mul_pre_nafwi") {
		ep2_rand(p);
		BENCH_ADD(ep2_mul_pre_nafwi(t, p));
	} BENCH_END;

	BENCH_BEGIN("ep2_mul_fix_nafwi") {
		bn_rand_mod(k, n);
		ep2_rand(p);
		ep2_mul_pre_nafwi(t, p);
		BENCH_ADD(ep2_mul_fix_nafwi(q, t, k));
	} BENCH_END;
	for (int i = 0; i < RELIC_EPX_TABLE_NAFWI; i++) {
		ep2_free(t[i]);
	}
#endif

#if EP_FIX == COMBS || !defined(STRIP)
	for (int i = 0; i < RELIC_EPX_TABLE_COMBS; i++) {
		ep2_new(t[i]);
	}
	BENCH_BEGIN("ep2_mul_pre_combs") {
		ep2_rand(p);
		BENCH_ADD(ep2_mul_pre_combs(t, p));
	} BENCH_END;

	BENCH_BEGIN("ep2_mul_fix_combs") {
		bn_rand_mod(k, n);
		ep2_rand(p);
		ep2_mul_pre_combs(t, p);
		BENCH_ADD(ep2_mul_fix_combs(q, t, k));
	} BENCH_END;
	for (int i = 0; i < RELIC_EPX_TABLE_COMBS; i++) {
		ep2_free(t[i]);
	}
#endif

#if EP_FIX == COMBD || !defined(STRIP)
	for (int i = 0; i < RELIC_EPX_TABLE_COMBD; i++) {
		ep2_new(t[i]);
	}
	BENCH_BEGIN("ep2_mul_pre_combd") {
		BENCH_ADD(ep2_mul_pre_combd(t, p));
	} BENCH_END;

	BENCH_BEGIN("ep2_mul_fix_combd") {
		bn_rand_mod(k, n);
		ep2_mul_pre_combd(t, p);
		BENCH_ADD(ep2_mul_fix_combd(q, t, k));
	} BENCH_END;
	for (int i = 0; i < RELIC_EPX_TABLE_COMBD; i++) {
		ep2_free(t[i]);
	}
#endif

#if EP_FIX == LWNAF || !defined(STRIP)
	for (int i = 0; i < RELIC_EPX_TABLE_LWNAF; i++) {
		ep2_new(t[i]);
	}
	BENCH_BEGIN("ep2_mul_pre_lwnaf") {
		ep2_rand(p);
		BENCH_ADD(ep2_mul_pre_lwnaf(t, p));
	} BENCH_END;

	BENCH_BEGIN("ep2_mul_fix_lwnaf") {
		bn_rand_mod(k, n);
		ep2_rand(p);
		ep2_mul_pre_lwnaf(t, p);
		BENCH_ADD(ep2_mul_fix_lwnaf(q, t, k));
	} BENCH_END;
	for (int i = 0; i < RELIC_EPX_TABLE_LWNAF; i++) {
		ep2_free(t[i]);
	}
#endif

	BENCH_BEGIN("ep2_mul_sim") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		ep2_rand(p);
		ep2_rand(q);
		BENCH_ADD(ep2_mul_sim(r, p, k, q, l));
	} BENCH_END;

#if EP_SIM == BASIC || !defined(STRIP)
	BENCH_BEGIN("ep2_mul_sim_basic") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		ep2_rand(p);
		ep2_rand(q);
		BENCH_ADD(ep2_mul_sim_basic(r, p, k, q, l));
	} BENCH_END;
#endif

#if EP_SIM == TRICK || !defined(STRIP)
	BENCH_BEGIN("ep2_mul_sim_trick") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		ep2_rand(p);
		ep2_rand(q);
		BENCH_ADD(ep2_mul_sim_trick(r, p, k, q, l));
	} BENCH_END;
#endif

#if EP_SIM == INTER || !defined(STRIP)
	BENCH_BEGIN("ep2_mul_sim_inter") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		ep2_rand(p);
		ep2_rand(q);
		BENCH_ADD(ep2_mul_sim_inter(r, p, k, q, l));
	} BENCH_END;
#endif

#if EP_SIM == JOINT || !defined(STRIP)
	BENCH_BEGIN("ep2_mul_sim_joint") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		ep2_rand(p);
		ep2_rand(q);
		BENCH_ADD(ep2_mul_sim_joint(r, p, k, q, l));
	} BENCH_END;
#endif

	BENCH_BEGIN("ep2_mul_sim_gen") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		ep2_rand(q);
		BENCH_ADD(ep2_mul_sim_gen(r, k, q, l));
	} BENCH_END;

	BENCH_BEGIN("ep2_map") {
		uint8_t msg[5];
		rand_bytes(msg, 5);
		BENCH_ADD(ep2_map(p, msg, 5, 1));
	} BENCH_END;

	BENCH_BEGIN("ep2_pck") {
		ep2_rand(p);
		BENCH_ADD(ep2_pck(q, p));
	} BENCH_END;

	BENCH_BEGIN("ep2_upk") {
		ep2_rand(p);
		BENCH_ADD(ep2_upk(q, p));
	} BENCH_END;

	ep2_free(p);
	ep2_free(q);
	ep2_free(r);
	bn_free(k);
	bn_free(n);
	bn_free(l);
	fp2_free(s);
}

int main(void) {
	if (core_init() != STS_OK) {
		core_clean();
		return 1;
	}

	conf_print();

	util_banner("Benchmarks for the EPX module:", 0);

	if (ep_param_set_any_pairf() != STS_OK) {
		THROW(ERR_NO_CURVE);
		core_clean();
		return 0;
	}

	if (ep2_curve_is_twist() == 0) {
		THROW(ERR_NO_CURVE);
		core_clean();
		return 0;
	}

	ep_param_print();

	util_banner("Utilities:", 1);
	memory();
	util();

	util_banner("Arithmetic:", 1);
	arith();

	core_clean();
	return 0;
}
