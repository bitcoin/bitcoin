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
 * Benchmarks for arithmetic on binary elliptic curves.
 *
 * @ingroup bench
 */

#include <stdio.h>

#include "relic.h"
#include "relic_bench.h"

static void memory(void) {
	eb_t a[BENCH];

	BENCH_FEW("eb_null", eb_null(a[i]), 1);

	BENCH_FEW("eb_new", eb_new(a[i]), 1);
	for (int i = 0; i < BENCH; i++) {
		eb_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		eb_new(a[i]);
	}
	BENCH_FEW("eb_free", eb_free(a[i]), 1);

	(void)a;
}

static void util(void) {
	eb_t p, q, t[4];
	uint8_t bin[2 * RLC_FB_BYTES + 1];
	int l;

	eb_null(p);
	eb_null(q);
	for (int j= 0; j < 4; j++) {
		eb_null(t[j]);
	}

	eb_new(p);
	eb_new(q);
	for (int j= 0; j < 4; j++) {
		eb_new(t[j]);
	}

	BENCH_RUN("eb_is_infty") {
		eb_rand(p);
		BENCH_ADD(eb_is_infty(p));
	} BENCH_END;

	BENCH_RUN("eb_set_infty") {
		eb_rand(p);
		BENCH_ADD(eb_set_infty(p));
	} BENCH_END;

	BENCH_RUN("eb_copy") {
		eb_rand(p);
		eb_rand(q);
		BENCH_ADD(eb_copy(p, q));
	} BENCH_END;

	BENCH_RUN("eb_cmp") {
		eb_rand(p);
		eb_dbl(p, p);
		eb_rand(q);
		eb_dbl(q, q);
		BENCH_ADD(eb_cmp(p, q));
	} BENCH_END;

	BENCH_RUN("eb_cmp (1 norm)") {
		eb_rand(p);
		eb_dbl(p, p);
		eb_rand(q);
		BENCH_ADD(eb_cmp(p, q));
	} BENCH_END;

	BENCH_RUN("eb_cmp (2 norm)") {
		eb_rand(p);
		eb_rand(q);
		BENCH_ADD(eb_cmp(p, q));
	} BENCH_END;

	BENCH_RUN("eb_rand") {
		BENCH_ADD(eb_rand(p));
	} BENCH_END;

	BENCH_RUN("eb_blind") {
		BENCH_ADD(eb_blind(p, p));
	} BENCH_END;

	BENCH_RUN("eb_rhs") {
		eb_rand(p);
		BENCH_ADD(eb_rhs(q->x, p));
	} BENCH_END;

	BENCH_RUN("eb_tab (4)") {
		eb_rand(p);
		BENCH_ADD(eb_tab(t, p, 4));
	} BENCH_END;

	BENCH_RUN("eb_on_curve") {
		eb_rand(p);
		BENCH_ADD(eb_on_curve(p));
	} BENCH_END;

	BENCH_RUN("eb_size_bin (0)") {
		eb_rand(p);
		BENCH_ADD(eb_size_bin(p, 0));
	} BENCH_END;

	BENCH_RUN("eb_size_bin (1)") {
		eb_rand(p);
		BENCH_ADD(eb_size_bin(p, 1));
	} BENCH_END;

	BENCH_RUN("eb_write_bin (0)") {
		eb_rand(p);
		l = eb_size_bin(p, 0);
		BENCH_ADD(eb_write_bin(bin, l, p, 0));
	} BENCH_END;

	BENCH_RUN("eb_write_bin (1)") {
		eb_rand(p);
		l = eb_size_bin(p, 1);
		BENCH_ADD(eb_write_bin(bin, l, p, 1));
	} BENCH_END;

	BENCH_RUN("eb_read_bin (0)") {
		eb_rand(p);
		l = eb_size_bin(p, 0);
		eb_write_bin(bin, l, p, 0);
		BENCH_ADD(eb_read_bin(p, bin, l));
	} BENCH_END;

	BENCH_RUN("eb_read_bin (1)") {
		eb_rand(p);
		l = eb_size_bin(p, 1);
		eb_write_bin(bin, l, p, 1);
		BENCH_ADD(eb_read_bin(p, bin, l));
	} BENCH_END;

	eb_free(p);
	eb_free(q);
	for (int j = 0; j < 4; j++) {
		eb_free(t[j]);
	}
}

static void arith(void) {
	eb_t p, q, r, t[RLC_EB_TABLE_MAX];
	bn_t k, l, n;

	eb_null(p);
	eb_null(q);
	eb_null(r);
	for (int i = 0; i < RLC_EB_TABLE_MAX; i++) {
		eb_null(t[i]);
	} bn_null(k);
	bn_null(l);
	bn_null(n);

	eb_new(p);
	eb_new(q);
	eb_new(r);
	bn_new(k);
	bn_new(n);
	bn_new(l);

	eb_curve_get_ord(n);

	BENCH_RUN("eb_add") {
		eb_rand(p);
		eb_rand(q);
		eb_add(p, p, q);
		eb_rand(q);
		eb_rand(p);
		eb_add(q, q, p);
		BENCH_ADD(eb_add(r, p, q));
	} BENCH_END;

#if EB_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("eb_add_basic") {
		eb_rand(p);
		eb_rand(q);
		BENCH_ADD(eb_add_basic(r, p, q));
	} BENCH_END;
#endif

#if EB_ADD == PROJC || !defined(STRIP)
	BENCH_RUN("eb_add_projc") {
		eb_rand(p);
		eb_rand(q);
		eb_add_projc(p, p, q);
		eb_rand(q);
		eb_rand(p);
		eb_add_projc(q, q, p);
		BENCH_ADD(eb_add_projc(r, p, q));
	} BENCH_END;

	BENCH_RUN("eb_add_projc (z2 = 1)") {
		eb_rand(p);
		eb_rand(q);
		eb_add_projc(p, p, q);
		eb_rand(q);
		eb_norm(q, q);
		BENCH_ADD(eb_add_projc(r, p, q));
	} BENCH_END;

	BENCH_RUN("eb_add_projc (z1,z2 = 1)") {
		eb_rand(p);
		eb_norm(p, p);
		eb_rand(q);
		eb_norm(q, q);
		BENCH_ADD(eb_add_projc(r, p, q));
	} BENCH_END;
#endif

	BENCH_RUN("eb_sub") {
		eb_rand(p);
		eb_rand(q);
		eb_add(p, p, q);
		eb_rand(q);
		eb_rand(p);
		eb_add(q, q, p);
		BENCH_ADD(eb_sub(r, p, q));
	} BENCH_END;

#if EB_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("eb_sub_basic") {
		eb_rand(p);
		eb_rand(q);
		BENCH_ADD(eb_sub_basic(r, p, q));
	} BENCH_END;
#endif

#if EB_ADD == PROJC || !defined(STRIP)
	BENCH_RUN("eb_sub_projc") {
		eb_rand(p);
		eb_rand(q);
		eb_add_projc(p, p, q);
		eb_rand(q);
		eb_rand(p);
		eb_add_projc(q, q, p);
		BENCH_ADD(eb_sub_projc(r, p, q));
	} BENCH_END;

	BENCH_RUN("eb_sub_projc (z2 = 1)") {
		eb_rand(p);
		eb_rand(q);
		eb_add_projc(p, p, q);
		eb_rand(q);
		eb_norm(q, q);
		BENCH_ADD(eb_sub_projc(r, p, q));
	} BENCH_END;

	BENCH_RUN("eb_sub_projc (z1,z2 = 1)") {
		eb_rand(p);
		eb_norm(p, p);
		eb_rand(q);
		eb_norm(q, q);
		BENCH_ADD(eb_sub_projc(r, p, q));
	} BENCH_END;
#endif

	BENCH_RUN("eb_dbl") {
		eb_rand(p);
		eb_rand(q);
		eb_add(p, p, q);
		BENCH_ADD(eb_dbl(r, p));
	} BENCH_END;

#if EB_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("eb_dbl_basic") {
		eb_rand(p);
		BENCH_ADD(eb_dbl_basic(r, p));
	} BENCH_END;
#endif

#if EB_ADD == PROJC || !defined(STRIP)
	BENCH_RUN("eb_dbl_projc") {
		eb_rand(p);
		eb_rand(q);
		eb_add_projc(p, p, q);
		BENCH_ADD(eb_dbl_projc(r, p));
	} BENCH_END;

	BENCH_RUN("eb_dbl_projc (z1 = 1)") {
		eb_rand(p);
		eb_norm(p, p);
		BENCH_ADD(eb_dbl_projc(r, p));
	} BENCH_END;
#endif

	BENCH_RUN("eb_hlv") {
		eb_rand(p);
		BENCH_ADD(eb_hlv(r, p));
	}
	BENCH_END;
#if defined(EB_KBLTZ)
	if (eb_curve_is_kbltz()) {
		BENCH_RUN("eb_frb") {
			eb_rand(p);
			eb_rand(q);
			eb_add_projc(p, p, q);
			BENCH_ADD(eb_frb(r, p));
		}
		BENCH_END;
	}

#if EB_ADD == BASIC || !defined(STRIP)
	if (eb_curve_is_kbltz()) {
		BENCH_RUN("eb_frb (z = 1)") {
			eb_rand(p);
			BENCH_ADD(eb_frb(r, p));
		}
		BENCH_END;
	}
#endif

#endif /* EB_KBLTZ */

	BENCH_RUN("eb_neg") {
		eb_rand(p);
		eb_rand(q);
		eb_add(p, p, q);
		BENCH_ADD(eb_neg(r, p));
	}
	BENCH_END;

#if EB_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("eb_neg_basic") {
		eb_rand(p);
		BENCH_ADD(eb_neg_basic(r, p));
	}
	BENCH_END;
#endif

#if EB_ADD == PROJC || !defined(STRIP)
	BENCH_RUN("eb_neg_projc") {
		eb_rand(p);
		eb_rand(q);
		eb_add_projc(p, p, q);
		BENCH_ADD(eb_neg_projc(r, p));
	}
	BENCH_END;
#endif

	BENCH_RUN("eb_mul") {
		bn_rand_mod(k, n);
		eb_rand(p);
		eb_rand(q);
		BENCH_ADD(eb_mul(q, p, k));
	}
	BENCH_END;

#if EB_MUL == BASIC || !defined(STRIP)
	BENCH_RUN("eb_mul_basic") {
		bn_rand_mod(k, n);
		eb_rand(p);
		BENCH_ADD(eb_mul_basic(q, p, k));
	}
	BENCH_END;
#endif

#if EB_MUL == LODAH || !defined(STRIP)
	BENCH_RUN("eb_mul_lodah") {
		bn_rand_mod(k, n);
		eb_rand(p);
		BENCH_ADD(eb_mul_lodah(q, p, k));
	}
	BENCH_END;
#endif

#if EB_MUL == LWNAF || !defined(STRIP)
	BENCH_RUN("eb_mul_lwnaf") {
		bn_rand_mod(k, n);
		eb_rand(p);
		BENCH_ADD(eb_mul_lwnaf(q, p, k));
	}
	BENCH_END;
#endif

#if EB_MUL == RWNAF || !defined(STRIP)
	BENCH_RUN("eb_mul_rwnaf") {
		bn_rand_mod(k, n);
		eb_rand(p);
		BENCH_ADD(eb_mul_rwnaf(q, p, k));
	}
	BENCH_END;
#endif

#if EB_MUL == HALVE || !defined(STRIP)
		BENCH_RUN("eb_mul_halve") {
			bn_rand_mod(k, n);
			eb_rand(p);
			BENCH_ADD(eb_mul_halve(q, p, k));
		}
		BENCH_END;
#endif

	BENCH_RUN("eb_mul_gen") {
		bn_rand_mod(k, n);
		BENCH_ADD(eb_mul_gen(q, k));
	}
	BENCH_END;

	BENCH_RUN("eb_mul_dig") {
		bn_rand(k, RLC_POS, RLC_DIG);
		bn_rand_mod(k, n);
		BENCH_ADD(eb_mul_dig(p, q, k->dp[0]));
	}
	BENCH_END;

	for (int i = 0; i < RLC_EB_TABLE; i++) {
		eb_new(t[i]);
	}

	BENCH_RUN("eb_mul_pre") {
		eb_rand(p);
		BENCH_ADD(eb_mul_pre(t, p));
	} BENCH_END;

	BENCH_RUN("eb_mul_fix") {
		bn_rand_mod(k, n);
		eb_rand(p);
		eb_mul_pre(t, p);
		BENCH_ADD(eb_mul_fix(q, (const eb_t *)t, k));
	} BENCH_END;

	for (int i = 0; i < RLC_EB_TABLE; i++) {
		eb_free(t[i]);
	}

#if EB_FIX == BASIC || !defined(STRIP)
	for (int i = 0; i < RLC_EB_TABLE_BASIC; i++) {
		eb_new(t[i]);
	}
	BENCH_RUN("eb_mul_pre_basic") {
		eb_rand(p);
		BENCH_ADD(eb_mul_pre_basic(t, p));
	} BENCH_END;

	BENCH_RUN("eb_mul_fix_basic") {
		bn_rand_mod(k, n);
		eb_rand(p);
		eb_mul_pre_basic(t, p);
		BENCH_ADD(eb_mul_fix_basic(q, (const eb_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RLC_EB_TABLE_BASIC; i++) {
		eb_free(t[i]);
	}
#endif

#if EB_FIX == COMBS || !defined(STRIP)
	for (int i = 0; i < RLC_EB_TABLE_COMBS; i++) {
		eb_new(t[i]);
	}
	BENCH_RUN("eb_mul_pre_combs") {
		eb_rand(p);
		BENCH_ADD(eb_mul_pre_combs(t, p));
	} BENCH_END;

	BENCH_RUN("eb_mul_fix_combs") {
		bn_rand_mod(k, n);
		eb_rand(p);
		eb_mul_pre_combs(t, p);
		BENCH_ADD(eb_mul_fix_combs(q, (const eb_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RLC_EB_TABLE_COMBS; i++) {
		eb_free(t[i]);
	}
#endif

#if EB_FIX == COMBD || !defined(STRIP)
	for (int i = 0; i < RLC_EB_TABLE_COMBD; i++) {
		eb_new(t[i]);
	}
	BENCH_RUN("eb_mul_pre_combd") {
		eb_rand(p);
		BENCH_ADD(eb_mul_pre_combd(t, p));
	} BENCH_END;

	BENCH_RUN("eb_mul_fix_combd") {
		bn_rand_mod(k, n);
		eb_rand(p);
		eb_mul_pre_combd(t, p);
		BENCH_ADD(eb_mul_fix_combd(q, (const eb_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RLC_EB_TABLE_COMBD; i++) {
		eb_free(t[i]);
	}
#endif

#if EB_FIX == LWNAF || !defined(STRIP)
	for (int i = 0; i < RLC_EB_TABLE_LWNAF; i++) {
		eb_new(t[i]);
	}
	BENCH_RUN("eb_mul_pre_lwnaf") {
		eb_rand(p);
		BENCH_ADD(eb_mul_pre_lwnaf(t, p));
	} BENCH_END;

	BENCH_RUN("eb_mul_fix_lwnaf") {
		bn_rand_mod(k, n);
		eb_rand(p);
		eb_mul_pre_lwnaf(t, p);
		BENCH_ADD(eb_mul_fix_lwnaf(q, (const eb_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RLC_EB_TABLE_LWNAF; i++) {
		eb_free(t[i]);
	}
#endif

	BENCH_RUN("eb_mul_sim") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		eb_rand(p);
		eb_rand(q);
		BENCH_ADD(eb_mul_sim(r, p, k, q, l));
	} BENCH_END;

#if EB_SIM == BASIC || !defined(STRIP)
	BENCH_RUN("eb_mul_sim_basic") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		eb_rand(p);
		eb_rand(q);
		BENCH_ADD(eb_mul_sim_basic(r, p, k, q, l));
	} BENCH_END;
#endif

#if EB_SIM == TRICK || !defined(STRIP)
	BENCH_RUN("eb_mul_sim_trick") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		eb_rand(p);
		eb_rand(q);
		BENCH_ADD(eb_mul_sim_trick(r, p, k, q, l));
	} BENCH_END;
#endif

#if EB_SIM == INTER || !defined(STRIP)
	BENCH_RUN("eb_mul_sim_inter") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		eb_rand(p);
		eb_rand(q);
		BENCH_ADD(eb_mul_sim_inter(r, p, k, q, l));
	} BENCH_END;
#endif

#if EB_SIM == JOINT || !defined(STRIP)
	BENCH_RUN("eb_mul_sim_joint") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		eb_rand(p);
		eb_rand(q);
		BENCH_ADD(eb_mul_sim_joint(r, p, k, q, l));
	} BENCH_END;
#endif

	BENCH_RUN("eb_mul_sim_gen") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		eb_rand(q);
		BENCH_ADD(eb_mul_sim_gen(r, k, q, l));
	} BENCH_END;

	BENCH_RUN("eb_map") {
		uint8_t msg[5];
		rand_bytes(msg, 5);
		BENCH_ADD(eb_map(p, msg, 5));
	} BENCH_END;

	BENCH_RUN("eb_pck") {
		eb_rand(p);
		BENCH_ADD(eb_pck(q, p));
	} BENCH_END;

	BENCH_RUN("eb_upk") {
		eb_rand(p);
		BENCH_ADD(eb_upk(q, p));
	} BENCH_END;

	eb_free(p);
	eb_free(q);
	bn_free(k);
	bn_free(l);
	bn_free(n);
}

static void bench(void) {
	eb_param_print();
	util_banner("Utilities:", 1);
	memory();
	util();
	util_banner("Arithmetic:", 1);
	arith();
}

int main(void) {
	int r0 = RLC_ERR, r1 = RLC_ERR;

	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	conf_print();
	util_banner("Benchmarks for the EB module:", 0);

#if defined(EB_PLAIN)
	r0 = eb_param_set_any_plain();
	if (r0 == RLC_OK) {
		bench();
	}
#endif

#if defined(EB_KBLTZ)
	r1 = eb_param_set_any_kbltz();
	if (r1 == RLC_OK) {
		bench();
	}
#endif

	if (r0 == RLC_ERR && r1 == RLC_ERR) {
		if (eb_param_set_any() == RLC_ERR) {
			RLC_THROW(ERR_NO_CURVE);
		}
	}

	core_clean();
	return 0;
}
