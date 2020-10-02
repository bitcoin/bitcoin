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
 * Benchmarks for pairings defined over prime elliptic curves.
 *
 * @ingroup bench
 */

#include <stdio.h>

#include "relic.h"
#include "relic_bench.h"

static void pairing2(void) {
	bn_t k, n, l;
	ep_t p[2], q[2];
	fp2_t e;
	int j;

	bn_null(k);
	bn_null(n);
	bn_null(l);
	fp2_null(e);

	bn_new(k);
	bn_new(n);
	bn_new(l);
	fp2_new(e);

	for (j = 0; j < 2; j++) {
		ep_null(p[j]);
		ep_null(q[j]);
		ep_new(p[0]);
		ep_new(q[0]);
	}

	ep_curve_get_ord(n);

	BENCH_BEGIN("pp_add_k2") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k2(e, p[0], p[0], q[0]));
	}
	BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_BEGIN("pp_add_k2_basic") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k2_basic(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)

	BENCH_BEGIN("pp_add_k2_projc") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k2_projc(e, p[0], p[0], q[0]));
	}
	BENCH_END;

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_BEGIN("pp_add_k2_projc_basic") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k2_projc_basic(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_BEGIN("pp_add_k2_projc_lazyr") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k2_projc_lazyr(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#endif

	BENCH_BEGIN("pp_dbl_k2") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k2(e, p[0], p[0], q[0]));
	}
	BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_BEGIN("pp_dbl_k2_basic") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k2_basic(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)

	BENCH_BEGIN("pp_dbl_k2_projc") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k2_projc(e, p[0], p[0], q[0]));
	}
	BENCH_END;

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_BEGIN("pp_dbl_k2_projc_basic") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k2_projc_basic(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_BEGIN("pp_dbl_k2_projc_lazyr") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k2_projc_lazyr(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#endif
	BENCH_BEGIN("pp_exp_k2") {
		fp2_rand(e);
		BENCH_ADD(pp_exp_k2(e, e));
	}
	BENCH_END;

	BENCH_BEGIN("pp_map_k2") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_map_k2(e, q[0], p[0]));
	}
	BENCH_END;

#if PP_MAP == TATEP || PP_MAP == OATEP || !defined(STRIP)
	BENCH_BEGIN("pp_map_tatep_k2") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_map_tatep_k2(e, q[0], p[0]));
	}
	BENCH_END;

	BENCH_BEGIN("pp_map_tatep_sim_k12 (2)") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		ep_rand(p[1]);
		ep_rand(q[1]);
		BENCH_ADD(pp_map_sim_tatep_k2(e, q, p, 2));
	}
	BENCH_END;
#endif

#if PP_MAP == WEILP || !defined(STRIP)
	BENCH_BEGIN("pp_map_weilp_k2") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_map_weilp_k2(e, q[0], p[0]));
	}
	BENCH_END;

	BENCH_BEGIN("pp_map_weilp_sim_k12 (2)") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		ep_rand(p[1]);
		ep_rand(q[1]);
		BENCH_ADD(pp_map_sim_weilp_k2(e, q, p, 2));
	}
	BENCH_END;
#endif

	bn_free(k);
	bn_free(n);
	bn_free(l);
	fp2_free(e);
	for (j = 0; j < 2; j++) {
		ep_free(p[j]);
		ep_free(q[j]);
	}
}

static void pairing12(void) {
	bn_t k, n, l;
	ep2_t p[2], r;
	ep_t q[2];
	fp12_t e;
	int j;

	bn_null(k);
	bn_null(n);
	bn_null(l);
	ep2_null(r);
	fp12_null(e);

	bn_new(k);
	bn_new(n);
	bn_new(l);
	ep2_new(r);
	fp12_new(e);

	for (j = 0; j < 2; j++) {
		ep2_null(p[j]);
		ep_null(q[j]);
		ep2_new(p[0]);
		ep_new(q[0]);
	}

	ep2_curve_get_ord(n);

	BENCH_BEGIN("pp_add_k12") {
		ep2_rand(p[0]);
		ep2_dbl(r, p[0]);
		ep2_norm(r, r);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k12(e, r, p[0], q[0]));
	}
	BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_BEGIN("pp_add_k12_basic") {
		ep2_rand(p[0]);
		ep2_dbl(r, p[0]);
		ep2_norm(r, r);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k12_basic(e, r, p[0], q[0]));
	}
	BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)

	BENCH_BEGIN("pp_add_k12_projc") {
		ep2_rand(p[0]);
		ep2_dbl(r, p[0]);
		ep2_norm(r, r);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k12_projc(e, r, p[0], q[0]));
	}
	BENCH_END;

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_BEGIN("pp_add_k12_projc_basic") {
		ep2_rand(p[0]);
		ep2_dbl(r, p[0]);
		ep2_norm(r, r);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k12_projc_basic(e, r, p[0], q[0]));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_BEGIN("pp_add_k12_projc_lazyr") {
		ep2_rand(p[0]);
		ep2_dbl(r, p[0]);
		ep2_norm(r, r);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k12_projc_lazyr(e, r, p[0], q[0]));
	}
	BENCH_END;
#endif

#endif

	BENCH_BEGIN("pp_dbl_k12") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k12(e, p[0], p[0], q[0]));
	}
	BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_BEGIN("pp_dbl_k12_basic") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k12_basic(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_BEGIN("pp_dbl_k12_projc_basic") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k12_projc_basic(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_BEGIN("pp_dbl_k12_projc_lazyr") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k12_projc_lazyr(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#endif

	BENCH_BEGIN("pp_exp_k12") {
		fp12_rand(e);
		BENCH_ADD(pp_exp_k12(e, e));
	}
	BENCH_END;

	BENCH_BEGIN("pp_map_k12") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_map_k12(e, q[0], p[0]));
	}
	BENCH_END;

	BENCH_BEGIN("pp_map_sim_k12 (2)") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		ep2_rand(p[1]);
		ep_rand(q[1]);
		BENCH_ADD(pp_map_sim_k12(e, q, p, 2));
	}
	BENCH_END;

#if PP_MAP == TATEP || !defined(STRIP)
	BENCH_BEGIN("pp_map_tatep_k12") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_map_tatep_k12(e, q[0], p[0]));
	}
	BENCH_END;

	BENCH_BEGIN("pp_map_tatep_sim_k12 (2)") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		ep2_rand(p[1]);
		ep_rand(q[1]);
		BENCH_ADD(pp_map_sim_tatep_k12(e, q, p, 2));
	}
	BENCH_END;
#endif

#if PP_MAP == WEILP || !defined(STRIP)
	BENCH_BEGIN("pp_map_weilp_k12") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_map_weilp_k12(e, q[0], p[0]));
	}
	BENCH_END;

	BENCH_BEGIN("pp_map_weilp_sim_k12 (2)") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		ep2_rand(p[1]);
		ep_rand(q[1]);
		BENCH_ADD(pp_map_sim_weilp_k12(e, q, p, 2));
	}
	BENCH_END;
#endif

#if PP_MAP == OATEP || !defined(STRIP)
	BENCH_BEGIN("pp_map_oatep_k12") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_map_oatep_k12(e, q[0], p[0]));
	}
	BENCH_END;
#endif

#if PP_MAP == OATEP || !defined(STRIP)
	BENCH_BEGIN("pp_map_sim_oatep_k12 (2)") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		ep2_rand(p[1]);
		ep_rand(q[1]);
		BENCH_ADD(pp_map_sim_oatep_k12(e, q, p, 2));
	}
	BENCH_END;
#endif

	bn_free(k);
	bn_free(n);
	bn_free(l);
	fp12_free(e);
	ep2_free(r);
	for (j = 0; j < 2; j++) {
		ep2_free(p[j]);
		ep_free(q[j]);
	}
}

int main(void) {
	if (core_init() != STS_OK) {
		core_clean();
		return 1;
	}

	conf_print();

	util_banner("Benchmarks for the PP module:", 0);

	if (ep_param_set_any_pairf() != STS_OK) {
		THROW(ERR_NO_CURVE);
		core_clean();
		return 0;
	}

	ep_param_print();
	util_banner("Arithmetic:", 1);

	if (ep_param_embed() == 2) {
		pairing2();
	}

	if (ep_param_embed() == 12) {
		pairing12();
	}

	core_clean();
	return 0;
}
