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
* Benchmarks for arithmetic on prime elliptic twisted Edwards curves.
*
* @ingroup bench
*/

#include <stdio.h>

#include "relic.h"
#include "relic_bench.h"

static void memory(void) {
	ed_t a[BENCH];

	BENCH_SMALL("ed_null", ed_null(a[i]));

	BENCH_SMALL("ed_new", ed_new(a[i]));
	for (int i = 0; i < BENCH; i++) {
		ed_free(a[i]);
	}

	for (int i = 0; i < BENCH; i++) {
		ed_new(a[i]);
	}
	BENCH_SMALL("ed_free", ed_free(a[i]));

	(void)a;
}

static void util(void) {
	ed_t p, q, t[4];
	uint8_t bin[2 * FP_BYTES + 1];
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

	BENCH_BEGIN("ed_is_infty") {
		ed_rand(p);
		BENCH_ADD(ed_is_infty(p));
	} BENCH_END;

	BENCH_BEGIN("ed_set_infty") {
		ed_rand(p);
		BENCH_ADD(ed_set_infty(p));
	} BENCH_END;

	BENCH_BEGIN("ed_copy") {
		ed_rand(p);
		ed_rand(q);
		BENCH_ADD(ed_copy(p, q));
	} BENCH_END;

	BENCH_BEGIN("ed_cmp") {
		ed_rand(p);
		ed_rand(q);
		BENCH_ADD(ed_cmp(p, q));
	} BENCH_END;

	BENCH_BEGIN("ed_rand") {
		BENCH_ADD(ed_rand(p));
	} BENCH_END;

	BENCH_BEGIN("ed_tab (4)") {
		ed_rand(p);
		BENCH_ADD(ed_tab(t, p, 4));
	} BENCH_END;

	BENCH_BEGIN("ed_is_valid") {
		ed_rand(p);
		BENCH_ADD(ed_is_valid(p));
	} BENCH_END;

	BENCH_BEGIN("ed_size_bin (0)") {
		ed_rand(p);
		BENCH_ADD(ed_size_bin(p, 0));
	} BENCH_END;

	BENCH_BEGIN("ed_size_bin (1)") {
		ed_rand(p);
		BENCH_ADD(ed_size_bin(p, 1));
	} BENCH_END;

	BENCH_BEGIN("ed_write_bin (0)") {
		ed_rand(p);
		l = ed_size_bin(p, 0);
		BENCH_ADD(ed_write_bin(bin, l, p, 0));
	} BENCH_END;

	BENCH_BEGIN("ed_write_bin (1)") {
		ed_rand(p);
		l = ed_size_bin(p, 1);
		BENCH_ADD(ed_write_bin(bin, l, p, 1));
	} BENCH_END;

	BENCH_BEGIN("ed_read_bin (0)") {
		ed_rand(p);
		l = ed_size_bin(p, 0);
		ed_write_bin(bin, l, p, 0);
		BENCH_ADD(ed_read_bin(p, bin, l));
	} BENCH_END;

	BENCH_BEGIN("ed_read_bin (1)") {
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
	ed_t p, q, r, t[RELIC_ED_TABLE_MAX];
	bn_t k, l, n;

	ed_null(p);
	ed_null(q);
	ed_null(r);
	for (int i = 0; i < RELIC_ED_TABLE_MAX; i++) {
		ed_null(t[i]);
	}

	ed_new(p);
	ed_new(q);
	ed_new(r);
	bn_new(k);
	bn_new(n);
	bn_new(l);

	ed_curve_get_ord(n);

	BENCH_BEGIN("ed_add") {
		ed_rand(p);
		ed_rand(q);
		ed_add(p, p, q);
		ed_rand(q);
		ed_rand(p);
		ed_add(q, q, p);
		BENCH_ADD(ed_add(r, p, q));
	} BENCH_END;

	BENCH_BEGIN("ed_sub") {
		ed_rand(p);
		ed_rand(q);
		ed_add(p, p, q);
		ed_rand(q);
		ed_rand(p);
		ed_add(q, q, p);
		BENCH_ADD(ed_sub(r, p, q));
	} BENCH_END;

	BENCH_BEGIN("ed_dbl") {
		ed_rand(p);
		ed_rand(q);
		ed_add(p, p, q);
		BENCH_ADD(ed_dbl(r, p));
	} BENCH_END;

	BENCH_BEGIN("ed_neg") {
		ed_rand(p);
		ed_rand(q);
		ed_add(p, p, q);
		BENCH_ADD(ed_neg(r, p));
	} BENCH_END;

	BENCH_BEGIN("ed_mul") {
		bn_rand_mod(k, n);
		ed_rand(p);
		BENCH_ADD(ed_mul(q, p, k));
	} BENCH_END;

#if ED_MUL == BASIC || !defined(STRIP)
	BENCH_BEGIN("ed_mul_basic") {
		bn_rand_mod(k, n);
		BENCH_ADD(ed_mul_basic(q, p, k));
	} BENCH_END;
#endif

#if ED_MUL == SLIDE || !defined(STRIP)
	BENCH_BEGIN("ed_mul_slide") {
		bn_rand_mod(k, n);
		ed_rand(p);
		BENCH_ADD(ed_mul_slide(q, p, k));
	} BENCH_END;
#endif

#if ED_MUL == MONTY || !defined(STRIP)
	BENCH_BEGIN("ed_mul_monty") {
		bn_rand_mod(k, n);
		ed_rand(p);
		BENCH_ADD(ed_mul_monty(q, p, k));
	} BENCH_END;
#endif

#if ED_MUL == MONTY || !defined(STRIP)
	BENCH_BEGIN("ed_mul_fixed") {
		bn_rand_mod(k, n);
		ed_rand(p);
		BENCH_ADD(ed_mul_fixed(q, p, k));
	} BENCH_END;
#endif

#if ED_MUL == LWNAF || !defined(STRIP)
	BENCH_BEGIN("ed_mul_lwnaf") {
		bn_rand_mod(k, n);
		ed_rand(p);
		BENCH_ADD(ed_mul_lwnaf(q, p, k));
	} BENCH_END;
#endif

	BENCH_BEGIN("ed_mul_gen") {
		bn_rand_mod(k, n);
		BENCH_ADD(ed_mul_gen(q, k));
	} BENCH_END;

	BENCH_BEGIN("ed_mul_dig") {
		bn_rand(k, BN_POS, BN_DIGIT);
		bn_rand_mod(k, n);
		BENCH_ADD(ed_mul_dig(p, q, k->dp[0]));
	}
	BENCH_END;

	for (int i = 0; i < RELIC_ED_TABLE; i++) {
		ed_new(t[i]);
	}

	BENCH_BEGIN("ed_mul_pre") {
		ed_rand(p);
		BENCH_ADD(ed_mul_pre(t, p));
	} BENCH_END;

	BENCH_BEGIN("ed_mul_fix") {
		bn_rand_mod(k, n);
		ed_rand(p);
		ed_mul_pre(t, p);
		BENCH_ADD(ed_mul_fix(q, (const ed_t *)t, k));
	} BENCH_END;

	for (int i = 0; i < RELIC_ED_TABLE; i++) {
		ed_free(t[i]);
	}

#if ED_FIX == BASIC || !defined(STRIP)
	for (int i = 0; i < RELIC_ED_TABLE_BASIC; i++) {
		ed_new(t[i]);
	}
	BENCH_BEGIN("ed_mul_pre_basic") {
		ed_rand(p);
		BENCH_ADD(ed_mul_pre_basic(t, p));
	} BENCH_END;

	BENCH_BEGIN("ed_mul_fix_basic") {
		bn_rand_mod(k, n);
		ed_rand(p);
		ed_mul_pre_basic(t, p);
		BENCH_ADD(ed_mul_fix_basic(q, (const ed_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RELIC_ED_TABLE_BASIC; i++) {
		ed_free(t[i]);
	}
#endif

#if ED_FIX == YAOWI || !defined(STRIP)
	for (int i = 0; i < RELIC_ED_TABLE_YAOWI; i++) {
		ed_new(t[i]);
	}
	BENCH_BEGIN("ed_mul_pre_yaowi") {
		ed_rand(p);
		BENCH_ADD(ed_mul_pre_yaowi(t, p));
	} BENCH_END;

	BENCH_BEGIN("ed_mul_fix_yaowi") {
		bn_rand_mod(k, n);
		ed_rand(p);
		ed_mul_pre_yaowi(t, p);
		BENCH_ADD(ed_mul_fix_yaowi(q, (const ed_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RELIC_ED_TABLE_YAOWI; i++) {
		ed_free(t[i]);
	}
#endif

#if ED_FIX == NAFWI || !defined(STRIP)
	for (int i = 0; i < RELIC_ED_TABLE_NAFWI; i++) {
		ed_new(t[i]);
	}
	BENCH_BEGIN("ed_mul_pre_nafwi") {
		ed_rand(p);
		BENCH_ADD(ed_mul_pre_nafwi(t, p));
	} BENCH_END;

	BENCH_BEGIN("ed_mul_fix_nafwi") {
		bn_rand_mod(k, n);
		ed_rand(p);
		ed_mul_pre_nafwi(t, p);
		BENCH_ADD(ed_mul_fix_nafwi(q, (const ed_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RELIC_ED_TABLE_NAFWI; i++) {
		ed_free(t[i]);
	}
#endif

#if ED_FIX == COMBS || !defined(STRIP)
	for (int i = 0; i < RELIC_ED_TABLE_COMBS; i++) {
		ed_new(t[i]);
	}
	BENCH_BEGIN("ed_mul_pre_combs") {
		ed_rand(p);
		BENCH_ADD(ed_mul_pre_combs(t, p));
	} BENCH_END;

	BENCH_BEGIN("ed_mul_fix_combs") {
		bn_rand_mod(k, n);
		ed_rand(p);
		ed_mul_pre_combs(t, p);
		BENCH_ADD(ed_mul_fix_combs(q, (const ed_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RELIC_ED_TABLE_COMBS; i++) {
		ed_free(t[i]);
	}
#endif

#if ED_FIX == COMBD || !defined(STRIP)
	for (int i = 0; i < RELIC_ED_TABLE_COMBD; i++) {
		ed_new(t[i]);
	}
	BENCH_BEGIN("ed_mul_pre_combd") {
		BENCH_ADD(ed_mul_pre_combd(t, p));
	} BENCH_END;

	BENCH_BEGIN("ed_mul_fix_combd") {
		bn_rand_mod(k, n);
		ed_mul_pre_combd(t, p);
		BENCH_ADD(ed_mul_fix_combd(q, (const ed_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RELIC_ED_TABLE_COMBD; i++) {
		ed_free(t[i]);
	}
#endif

#if ED_FIX == LWNAF || !defined(STRIP)
	for (int i = 0; i < RELIC_ED_TABLE_LWNAF; i++) {
		ed_new(t[i]);
	}
	BENCH_BEGIN("ed_mul_pre_lwnaf") {
		ed_rand(p);
		BENCH_ADD(ed_mul_pre_lwnaf(t, p));
	} BENCH_END;

	BENCH_BEGIN("ed_mul_fix_lwnaf") {
		bn_rand_mod(k, n);
		ed_rand(p);
		ed_mul_pre_lwnaf(t, p);
		BENCH_ADD(ed_mul_fix_lwnaf(q, (const ed_t *)t, k));
	} BENCH_END;
	for (int i = 0; i < RELIC_ED_TABLE_LWNAF; i++) {
		ed_free(t[i]);
	}
#endif
	BENCH_BEGIN("ed_mul_sim") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		ed_rand(p);
		ed_rand(q);
		BENCH_ADD(ed_mul_sim(r, p, k, q, l));
	} BENCH_END;

	BENCH_BEGIN("ed_mul_sim_gen") {
		bn_rand_mod(k, n);
		bn_rand_mod(l, n);
		ed_rand(q);
		BENCH_ADD(ed_mul_sim_gen(r, k, q, l));
	} BENCH_END;

	BENCH_BEGIN("ed_map") {
		uint8_t msg[5];
		rand_bytes(msg, 5);
		BENCH_ADD(ed_map(p, msg, 5));
	} BENCH_END;

	BENCH_BEGIN("ed_pck") {
		ed_rand(p);
		BENCH_ADD(ed_pck(q, p));
	} BENCH_END;

	BENCH_BEGIN("ed_upk") {
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
	int r0 = STS_ERR;

	if (core_init() != STS_OK) {
		core_clean();
		return 1;
	}

	conf_print();
	util_banner("Benchmarks for the EP module:", 0);
	r0 = ed_param_set_any();
	if (r0 == STS_OK) {
		bench();
	}

	if (r0 == STS_ERR) {
		if (ed_param_set_any() == STS_ERR) {
			THROW(ERR_NO_CURVE);
		}
	}

	core_clean();
	return 0;
}
