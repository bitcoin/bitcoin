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
 * Benchmarks for cryptographic protocols.
 *
 * @version $Id$
 * @ingroup bench
 */

#include <stdio.h>
#include <assert.h>

#include "relic.h"
#include "relic_test.h"
#include "relic_bench.h"

#define MSGS	10000

static int test_pss(void) {
	int i, code = RLC_ERR;
	bn_t ms[MSGS], n, u, v, _v[MSGS];
	g1_t a, b;
	g2_t g, x, y, _y[MSGS];

	bn_null(n);
	bn_null(u);
	bn_null(v);
	g1_null(a);
	g1_null(b);
	g2_null(g);
	g2_null(x);
	g2_null(y);

	RLC_TRY {
		bn_new(n);
		bn_new(u);
		bn_new(v);
		g1_new(a);
		g1_new(b);
		g2_new(g);
		g2_new(x);
		g2_new(y);

		g1_get_ord(n);

		for (i = 0; i < MSGS; i++) {
			bn_null(ms[i]);
			bn_null(_v[i]);
			g2_null(_y[i]);
			bn_new(ms[i]);
			bn_rand_mod(ms[i], n);
			bn_new(_v[i]);
			g2_new(_y[i]);
		}

		TEST_CASE("pointcheval-sanders simple signature is correct") {
			TEST_ASSERT(cp_pss_gen(u, v, g, x, y) == RLC_OK, end);
			TEST_ASSERT(cp_pss_sig(a, b, ms[0], u, v) == RLC_OK, end);
			TEST_ASSERT(cp_pss_ver(a, b, ms[0], g, x, y) == 1, end);
			/* Check adversarial signature. */
			g1_set_infty(a);
			g1_set_infty(b);
			TEST_ASSERT(cp_pss_ver(a, b, ms[0], g, x, y) == 0, end);
		}
		TEST_END;

		TEST_CASE("pointcheval-sanders block signature is correct") {
			TEST_ASSERT(cp_psb_gen(u, _v, g, x, _y, MSGS) == RLC_OK, end);
			TEST_ASSERT(cp_psb_sig(a, b, ms, u, _v, MSGS) == RLC_OK, end);
			TEST_ASSERT(cp_psb_ver(a, b, ms, g, x, _y, MSGS) == 1, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	bn_free(n);
	bn_free(u);
	bn_free(v);
	g1_free(a);
	g1_free(b);
	g2_free(g);
	g2_free(x);
	g2_free(y);
	for (i = 0; i < MSGS; i++) {
		bn_free(ms[i]);
		bn_free(_v[i]);
		g2_free(_y[i]);
	}
  	return code;
}

static int test_mpss(void) {
	int i, j, code = RLC_ERR;
	bn_t m[2], n, u[2], v[2], ms[MSGS][2], _v[MSGS][2];
	g1_t g, s[2];
	g2_t h, x[2], y[2], _y[MSGS][2];
	gt_t e;
	mt_t tri[3][2];
	pt_t t[2];

	bn_null(n);
	g1_null(g);
	g2_null(h);
	gt_null(e);

	RLC_TRY {
		bn_new(n);
		g1_new(g);
		g2_new(h);
		gt_new(e);
		g1_get_ord(n);
		for (i = 0; i < 2; i++) {
			bn_null(m[i]);
			bn_null(u[i]);
			bn_null(v[i]);
			g1_null(s[i]);
			g2_null(x[i]);
			g2_null(y[i]);
			mt_null(tri[0][i]);
			mt_null(tri[1][i]);
			mt_null(tri[2][i]);
			pt_null(t[i]);
			bn_new(m[i]);
			bn_rand_mod(m[i], n);
			bn_new(u[i]);
			bn_new(v[i]);
			g1_new(s[i]);
			g2_new(x[i]);
			g2_new(y[i]);
			mt_new(tri[0][i]);
			mt_new(tri[1][i]);
			mt_new(tri[2][i]);
			pt_new(t[i]);
			for (j = 0; j < MSGS; j++) {
				bn_null(ms[j][i]);
				bn_null(_v[j][i]);
				g2_null(_y[j][i]);
				bn_new(ms[j][i]);
				bn_rand_mod(ms[j][i], n);
				bn_new(_v[j][i]);
				g2_new(_y[j][i]);
			}
		}

		TEST_CASE("multi-party pointcheval-sanders simple signature is correct") {
			pc_map_tri(t);
			mt_gen(tri[0], n);
			mt_gen(tri[1], n);
			mt_gen(tri[2], n);
			TEST_ASSERT(cp_mpss_gen(u, v, h, x, y) == RLC_OK, end);
			TEST_ASSERT(cp_mpss_bct(x, y) == RLC_OK, end);
			/* Compute signature in MPC. */
			TEST_ASSERT(cp_mpss_sig(g, s, m, u, v, tri[0], tri[1]) == RLC_OK, end);
			/* Verify signature in MPC. */
			cp_mpss_ver(e, g, s, m, h, x[0], y[0], tri[2], t);
			TEST_ASSERT(gt_is_unity(e) == 1, end);
			/* Check that signature is also valid for conventional scheme. */
			bn_add(m[0], m[0], m[1]);
			bn_mod(m[0], m[0], n);
			g1_add(s[0], s[0], s[1]);
			g1_norm(s[0], s[0]);
			TEST_ASSERT(cp_pss_ver(g, s[0], m[0], h, x[0], y[0]) == 1, end);
		}
		TEST_END;

		TEST_CASE("multi-party pointcheval-sanders block signature is correct") {
			g1_get_ord(n);
			pc_map_tri(t);
			mt_gen(tri[0], n);
			mt_gen(tri[1], n);
			mt_gen(tri[2], n);
			TEST_ASSERT(cp_mpsb_gen(u, _v, h, x, _y, MSGS) == RLC_OK, end);
			TEST_ASSERT(cp_mpsb_bct(x, _y, MSGS) == RLC_OK, end);
			/* Compute signature in MPC. */
			TEST_ASSERT(cp_mpsb_sig(g, s, ms, u, _v, tri[0], tri[1], MSGS) == RLC_OK, end);
			/* Verify signature in MPC. */
			cp_mpsb_ver(e, g, s, ms, h, x[0], _y, NULL, tri[2], t, MSGS);
			TEST_ASSERT(gt_is_unity(e) == 1, end);
			cp_mpsb_ver(e, g, s, ms, h, x[0], _y, _v, tri[2], t, MSGS);
			TEST_ASSERT(gt_is_unity(e) == 1, end);
			bn_sub_dig(ms[0][0], ms[0][0], 1);
			cp_mpsb_ver(e, g, s, ms, h, x[0], _y, _v, tri[2], t, MSGS);
			TEST_ASSERT(gt_is_unity(e) == 0, end);
		}
		TEST_END;
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
  	bn_free(n);
	g1_free(g);
	g2_free(h);
	gt_free(e);
	for (i = 0; i < 2; i++) {
		bn_free(m[i]);
		bn_free(u[i]);
		bn_free(v[i]);
		g1_free(s[i]);
		g2_free(x[i]);
		g2_free(y[i]);
		mt_free(tri[0][i]);
		mt_free(tri[1][i]);
		mt_free(tri[2][i]);
		pt_free(t[i]);
		for (j = 0; j < MSGS; j++) {
			bn_free(ms[j][i]);
			bn_free(_v[j][i]);
			g2_free(_y[j][i]);
		}
	}
  	return code;
}

static void bench_pss(void) {
	bn_t ms[MSGS], n, u, v, _v[MSGS];
	g1_t a, b;
	g2_t g, x, y, _y[MSGS];

	bn_null(n);
	bn_null(u);
	bn_null(v);
	g1_null(a);
	g1_null(b);
	g2_null(g);
	g2_null(x);
	g2_null(y);
	bn_new(n);
	bn_new(u);
	bn_new(v);
	g1_new(a);
	g1_new(b);
	g2_new(g);
	g2_new(x);
	g2_new(y);

	g1_get_ord(n);
	for (int i = 0; i < MSGS; i++) {
		bn_null(ms[i]);
		bn_null(_v[i]);
		g2_null(_y[i]);
		bn_new(ms[i]);
		bn_rand_mod(ms[i], n);
		bn_new(_v[i]);
		g2_new(_y[i]);
	}

	BENCH_RUN("cp_pss_gen") {
		BENCH_ADD(cp_pss_gen(u, v, g, x, y));
	} BENCH_END;

	BENCH_RUN("cp_pss_sig") {
		BENCH_ADD(cp_pss_sig(a, b, ms[0], u, v));
	} BENCH_END;

	BENCH_RUN("cp_pss_ver") {
		BENCH_ADD(cp_pss_ver(a, b, ms[0], g, x, y));
	} BENCH_END;

	BENCH_RUN("cp_psb_gen") {
		BENCH_ADD(cp_psb_gen(u, _v, g, x, _y, MSGS));
	} BENCH_END;

	BENCH_RUN("cp_psb_sig") {
		BENCH_ADD(cp_psb_sig(a, b, ms, u, _v, MSGS));
	} BENCH_END;

	BENCH_RUN("cp_psb_ver") {
		BENCH_ADD(cp_psb_ver(a, b, ms, g, x, _y, MSGS));
	} BENCH_END;

	bn_free(u);
	bn_free(v);
	g1_free(a);
	g1_free(b);
	g2_free(g);
	g2_free(x);
	g2_free(y);
	for (int i = 0; i < MSGS; i++) {
		bn_free(ms[i]);
		bn_free(_v[i]);
		g1_free(_y[i]);
	}
}

static void bench_mpss(void) {
	bn_t m[2], n, u[2], v[2], ms[MSGS][2], _v[MSGS][2];
	g1_t g, s[2];
	g2_t h, x[2], y[2], _y[MSGS][2];
	gt_t r;
	mt_t tri[3][2];
	pt_t t[2];

	bn_null(n);
	g1_null(g);
	g2_null(h);
	gt_null(r);

	bn_new(n);
	g1_new(g);
	g2_new(h);
	gt_new(r);
	for (int i = 0; i < 2; i++) {
		bn_null(m[i]);
		bn_null(u[i]);
		bn_null(v[i]);
		g1_null(s[i]);
		g2_null(x[i]);
		g2_null(y[i]);
		mt_null(tri[0][i]);
		mt_null(tri[1][i]);
		mt_null(tri[2][i]);
		pt_null(t[i]);
		bn_new(m[i]);
		bn_new(u[i]);
		bn_new(v[i]);
		g1_new(s[i]);
		g2_new(x[i]);
		g2_new(y[i]);
		mt_new(tri[0][i]);
		mt_new(tri[1][i]);
		mt_new(tri[2][i]);
		pt_new(t[i]);

		g1_get_ord(n);
		for (int j = 0; j < MSGS; j++) {
			bn_null(ms[j][i]);
			bn_null(_v[j][i]);
			g2_null(_y[j][i]);
			bn_new(ms[j][i]);
			bn_rand_mod(ms[j][i], n);
			bn_new(_v[j][i]);
			g2_new(_y[j][i]);
		}
	}

	pc_map_tri(t);
	mt_gen(tri[0], n);
	mt_gen(tri[1], n);
	mt_gen(tri[2], n);

	bn_rand_mod(m[0], n);
	bn_rand_mod(m[1], n);
	bn_sub(m[0], m[1], m[0]);
	if (bn_sign(m[0]) == RLC_NEG) {
		bn_add(m[0], m[0], n);
	}

	BENCH_RUN("cp_mpss_gen") {
		BENCH_ADD(cp_mpss_gen(u, v, h, x, y));
	} BENCH_END;

	BENCH_RUN("cp_mpss_bct") {
		BENCH_ADD(cp_mpss_bct(x, y));
	} BENCH_END;

	BENCH_RUN("cp_mpss_sig") {
		BENCH_ADD(cp_mpss_sig(g, s, m, u, v, tri[0], tri[1]));
	} BENCH_DIV(2);

	BENCH_RUN("cp_mpss_ver") {
		BENCH_ADD(cp_mpss_ver(r, g, s, m, h, x[0], y[0], tri[2], t));
	} BENCH_DIV(2);

	g1_get_ord(n);
	pc_map_tri(t);
	mt_gen(tri[0], n);
	mt_gen(tri[1], n);
	mt_gen(tri[2], n);

	BENCH_RUN("cp_mpsb_gen") {
		BENCH_ADD(cp_mpsb_gen(u, _v, h, x, _y, MSGS));
	} BENCH_END;

	BENCH_RUN("cp_mpsb_bct") {
		BENCH_ADD(cp_mpsb_bct(x, _y, MSGS));
	} BENCH_END;

	BENCH_RUN("cp_mpsb_sig") {
		BENCH_ADD(cp_mpsb_sig(g, s, ms, u, _v, tri[0], tri[1], MSGS));
	} BENCH_DIV(2);

	BENCH_RUN("cp_mpsb_ver") {
		BENCH_ADD(cp_mpsb_ver(r, g, s, ms, h, x[0], _y, NULL, tri[2], t, MSGS));
	} BENCH_DIV(2);

	BENCH_RUN("cp_mpsb_ver (sk)") {
		BENCH_ADD(cp_mpsb_ver(r, g, s, ms, h, x[0], _y, _v, tri[2], t, MSGS));
	} BENCH_DIV(2);

  	bn_free(n);
	g1_free(g);
	g2_free(h);
	gt_free(r);
	for (int i = 0; i < 2; i++) {
		bn_free(m[i]);
		bn_free(u[i]);
		bn_free(v[i]);
		g1_free(s[i]);
		g2_free(x[i]);
		g2_free(y[i]);
		mt_free(tri[0][i]);
		mt_free(tri[1][i]);
		mt_free(tri[2][i]);
		pt_free(t[i]);
		for (int j = 0; j < MSGS; j++) {
			bn_free(ms[j][i]);
			bn_free(_v[j][i]);
			g2_free(_y[j][i]);
		}
	}
}


int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	conf_print();

	util_banner("Note: If you get a SIGSEGV, try to increase the stack size with ulimit.", 1);

#if defined(WITH_PC)
	if (pc_param_set_any() == RLC_OK) {
#if defined(WITH_MPC)
		util_banner("Tests for the PS signature and two-party PS protocol:\n", 0);
		test_pss();
		test_mpss();
		util_banner("Benchmarks for the PS signature and two-party PS protocol:\n", 0);
		bench_pss();
		bench_mpss();
#endif
	} else {
		RLC_THROW(ERR_NO_CURVE);
	}
#endif

	core_clean();
	return 0;
}
