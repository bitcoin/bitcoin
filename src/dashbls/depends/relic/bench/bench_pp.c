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

	BENCH_RUN("pp_add_k2") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k2(e, p[0], p[0], q[0]));
	}
	BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("pp_add_k2_basic") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k2_basic(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)

	BENCH_RUN("pp_add_k2_projc") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k2_projc(e, p[0], p[0], q[0]));
	}
	BENCH_END;

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_RUN("pp_add_k2_projc_basic") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k2_projc_basic(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_RUN("pp_add_k2_projc_lazyr") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k2_projc_lazyr(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#endif

	BENCH_RUN("pp_dbl_k2") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k2(e, p[0], p[0], q[0]));
	}
	BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("pp_dbl_k2_basic") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k2_basic(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)

	BENCH_RUN("pp_dbl_k2_projc") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k2_projc(e, p[0], p[0], q[0]));
	}
	BENCH_END;

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_RUN("pp_dbl_k2_projc_basic") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k2_projc_basic(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_RUN("pp_dbl_k2_projc_lazyr") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k2_projc_lazyr(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#endif
	BENCH_RUN("pp_exp_k2") {
		fp2_rand(e);
		BENCH_ADD(pp_exp_k2(e, e));
	}
	BENCH_END;

	BENCH_RUN("pp_map_k2") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_map_k2(e, q[0], p[0]));
	}
	BENCH_END;

#if PP_MAP == TATEP || PP_MAP == OATEP || !defined(STRIP)
	BENCH_RUN("pp_map_tatep_k2") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_map_tatep_k2(e, q[0], p[0]));
	}
	BENCH_END;

	BENCH_RUN("pp_map_tatep_sim_k12 (2)") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		ep_rand(p[1]);
		ep_rand(q[1]);
		BENCH_ADD(pp_map_sim_tatep_k2(e, q, p, 2));
	}
	BENCH_END;
#endif

#if PP_MAP == WEILP || !defined(STRIP)
	BENCH_RUN("pp_map_weilp_k2") {
		ep_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_map_weilp_k2(e, q[0], p[0]));
	}
	BENCH_END;

	BENCH_RUN("pp_map_weilp_sim_k12 (2)") {
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

static void pairing8(void) {
	bn_t k, n, l;
	ep2_t p[2], r;
	ep_t q[2];
	fp8_t e;
	int j;

	bn_null(k);
	bn_null(n);
	bn_null(l);
	ep2_null(r);
	fp8_null(e);

	bn_new(k);
	bn_new(n);
	bn_new(l);
	ep2_new(r);
	fp8_new(e);

	for (j = 0; j < 2; j++) {
		ep2_null(p[j]);
		ep_null(q[j]);
		ep2_new(p[0]);
		ep_new(q[0]);
	}

	ep2_curve_get_ord(n);

	BENCH_RUN("pp_add_k8") {
		ep2_rand(p[0]);
		ep2_dbl(r, p[0]);
		ep2_norm(r, r);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k8(e, r, p[0], q[0]));
	}
	BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("pp_add_k8_basic") {
		ep2_rand(p[0]);
		ep2_dbl(r, p[0]);
		ep2_norm(r, r);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k8_basic(e, r, p[0], q[0]));
	}
	BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)
	BENCH_RUN("pp_add_k8_projc") {
		ep2_rand(p[0]);
		ep2_dbl(r, p[0]);
		ep2_norm(r, r);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k8_projc(e, r, p[0], q[0]));
	}
	BENCH_END;
#endif

	BENCH_RUN("pp_dbl_k8") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k8(e, p[0], p[0], q[0]));
	}
	BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("pp_dbl_k8_basic") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k8_basic(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)
	BENCH_RUN("pp_dbl_k8_projc") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k8_projc(e, p[0], p[0], q[0]));
	}
	BENCH_END;

	#if PP_EXT == BASIC || !defined(STRIP)
		BENCH_RUN("pp_dbl_k8_projc_basic") {
			ep2_rand(p[0]);
			ep_rand(q[0]);
			BENCH_ADD(pp_dbl_k8_projc_basic(e, p[0], p[0], q[0]));
		}
		BENCH_END;
	#endif

	#if PP_EXT == LAZYR || !defined(STRIP)
		BENCH_RUN("pp_dbl_k8_projc_lazyr") {
			ep2_rand(p[0]);
			ep_rand(q[0]);
			BENCH_ADD(pp_dbl_k8_projc_lazyr(e, p[0], p[0], q[0]));
		}
		BENCH_END;
	#endif
#endif

	BENCH_RUN("pp_exp_k8") {
		fp8_rand(e);
		BENCH_ADD(pp_exp_k8(e, e));
	}
	BENCH_END;

	BENCH_RUN("pp_map_oate_k8") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_map_oatep_k8(e, q[0], p[0]));
	}
	BENCH_END;

	bn_free(k);
	bn_free(n);
	bn_free(l);
	fp8_free(e);
	ep2_free(r);
	for (j = 0; j < 2; j++) {
		ep2_free(p[j]);
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
		ep2_new(p[j]);
		ep_new(q[j]);
	}

	ep2_curve_get_ord(n);

	BENCH_RUN("pp_add_k12") {
		ep2_rand(p[0]);
		ep2_dbl(r, p[0]);
		ep2_norm(r, r);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k12(e, r, p[0], q[0]));
	}
	BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("pp_add_k12_basic") {
		ep2_rand(p[0]);
		ep2_dbl(r, p[0]);
		ep2_norm(r, r);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k12_basic(e, r, p[0], q[0]));
	}
	BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)

	BENCH_RUN("pp_add_k12_projc") {
		ep2_rand(p[0]);
		ep2_dbl(r, p[0]);
		ep2_norm(r, r);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k12_projc(e, r, p[0], q[0]));
	}
	BENCH_END;

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_RUN("pp_add_k12_projc_basic") {
		ep2_rand(p[0]);
		ep2_dbl(r, p[0]);
		ep2_norm(r, r);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k12_projc_basic(e, r, p[0], q[0]));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_RUN("pp_add_k12_projc_lazyr") {
		ep2_rand(p[0]);
		ep2_dbl(r, p[0]);
		ep2_norm(r, r);
		ep_rand(q[0]);
		BENCH_ADD(pp_add_k12_projc_lazyr(e, r, p[0], q[0]));
	}
	BENCH_END;
#endif

#endif

	BENCH_RUN("pp_dbl_k12") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k12(e, p[0], p[0], q[0]));
	}
	BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("pp_dbl_k12_basic") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k12_basic(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)

	BENCH_RUN("pp_dbl_k12_projc") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k12_projc(e, p[0], p[0], q[0]));
	}
	BENCH_END;

#if PP_EXT == BASIC || !defined(STRIP)
	BENCH_RUN("pp_dbl_k12_projc_basic") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k12_projc_basic(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
	BENCH_RUN("pp_dbl_k12_projc_lazyr") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_dbl_k12_projc_lazyr(e, p[0], p[0], q[0]));
	}
	BENCH_END;
#endif

#endif

	BENCH_RUN("pp_exp_k12") {
		fp12_rand(e);
		BENCH_ADD(pp_exp_k12(e, e));
	}
	BENCH_END;

	BENCH_RUN("pp_map_k12") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_map_k12(e, q[0], p[0]));
	}
	BENCH_END;

	BENCH_RUN("pp_map_sim_k12 (2)") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		ep2_rand(p[1]);
		ep_rand(q[1]);
		BENCH_ADD(pp_map_sim_k12(e, q, p, 2));
	}
	BENCH_END;

#if PP_MAP == TATEP || !defined(STRIP)
	BENCH_RUN("pp_map_tatep_k12") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_map_tatep_k12(e, q[0], p[0]));
	}
	BENCH_END;

	BENCH_RUN("pp_map_tatep_sim_k12 (2)") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		ep2_rand(p[1]);
		ep_rand(q[1]);
		BENCH_ADD(pp_map_sim_tatep_k12(e, q, p, 2));
	}
	BENCH_END;
#endif

#if PP_MAP == WEILP || !defined(STRIP)
	BENCH_RUN("pp_map_weilp_k12") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_map_weilp_k12(e, q[0], p[0]));
	}
	BENCH_END;

	BENCH_RUN("pp_map_weilp_sim_k12 (2)") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		ep2_rand(p[1]);
		ep_rand(q[1]);
		BENCH_ADD(pp_map_sim_weilp_k12(e, q, p, 2));
	}
	BENCH_END;
#endif

#if PP_MAP == OATEP || !defined(STRIP)
	BENCH_RUN("pp_map_oatep_k12") {
		ep2_rand(p[0]);
		ep_rand(q[0]);
		BENCH_ADD(pp_map_oatep_k12(e, q[0], p[0]));
	}
	BENCH_END;
#endif

#if PP_MAP == OATEP || !defined(STRIP)
	BENCH_RUN("pp_map_sim_oatep_k12 (2)") {
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

static void pairing48(void) {
	ep_t p;
	fp8_t qx, qy, qz;
	fp48_t e;

	ep_null(p);
	fp8_null(qx);
	fp8_null(qy);
	fp8_null(qz);
	fp48_null(e);

	ep_new(p);
	fp8_new(qx);
	fp8_new(qy);
	fp8_new(qz);
	fp48_new(e);

	BENCH_RUN("pp_add_k48") {
		fp8_rand(qx);
		fp8_rand(qy);
		fp8_rand(qz);
		ep_rand(p);
		BENCH_ADD(pp_add_k48(e, qx, qy, qz, qy, qx, p));
	}
	BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("pp_add_k48_basic") {
		fp8_rand(qx);
		fp8_rand(qy);
		fp8_rand(qz);
		ep_rand(p);
		BENCH_ADD(pp_add_k48_basic(e, qx, qy, qy, qx, p));
	}
	BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)
	BENCH_RUN("pp_add_k48_projc") {
		fp8_rand(qx);
		fp8_rand(qy);
		fp8_rand(qz);
		ep_rand(p);
		BENCH_ADD(pp_add_k48_projc(e, qx, qy, qz, qx, qy, p));
	}
	BENCH_END;
#endif

	BENCH_RUN("pp_dbl_k48") {
		fp8_rand(qx);
		fp8_rand(qy);
		fp8_rand(qz);
		ep_rand(p);
		BENCH_ADD(pp_dbl_k48(e, qx, qy, qz, p));
	}
	BENCH_END;

	#if EP_ADD == BASIC || !defined(STRIP)
		BENCH_RUN("pp_dbl_k48_basic") {
			fp8_rand(qx);
			fp8_rand(qy);
			ep_rand(p);
			BENCH_ADD(pp_dbl_k48_basic(e, qx, qy, p));
		}
		BENCH_END;
	#endif

	#if EP_ADD == PROJC || !defined(STRIP)
		BENCH_RUN("pp_dbl_k48_projc") {
			fp8_rand(qx);
			fp8_rand(qy);
			fp8_rand(qz);
			ep_rand(p);
			BENCH_ADD(pp_dbl_k48_projc(e, qx, qy, qz, p));
		}
		BENCH_END;
	#endif

	BENCH_RUN("pp_exp_k48") {
		fp48_rand(e);
		BENCH_ADD(pp_exp_k48(e, e));
	}
	BENCH_END;

	BENCH_RUN("pp_map_k48") {
		fp8_rand(qx);
		fp8_rand(qy);
		fp8_rand(qz);
		ep_rand(p);
		BENCH_ADD(pp_map_k48(e, p, qx, qy));
	}
	BENCH_END;

	ep_free(p);
	fp8_free(qx);
	fp8_free(qy);
	fp8_free(qz);
	fp48_free(e);
}

static void pairing54(void) {
	ep_t p;
	fp9_t qx, qy, qz;
	fp54_t e;

	ep_null(p);
	fp9_null(qx);
	fp9_null(qy);
	fp9_null(qz);
	fp54_null(e);

	ep_new(p);
	fp9_new(qx);
	fp9_new(qy);
	fp9_new(qz);
	fp54_new(e);

	BENCH_RUN("pp_add_k54") {
		fp9_rand(qx);
		fp9_rand(qy);
		fp9_rand(qz);
		ep_rand(p);
		BENCH_ADD(pp_add_k54(e, qx, qy, qz, qy, qx, p));
	}
	BENCH_END;

#if EP_ADD == BASIC || !defined(STRIP)
	BENCH_RUN("pp_add_k54_basic") {
		fp9_rand(qx);
		fp9_rand(qy);
		fp9_rand(qz);
		ep_rand(p);
		BENCH_ADD(pp_add_k54_basic(e, qx, qy, qy, qx, p));
	}
	BENCH_END;
#endif

#if EP_ADD == PROJC || !defined(STRIP)
	BENCH_RUN("pp_add_k54_projc") {
		fp9_rand(qx);
		fp9_rand(qy);
		fp9_rand(qz);
		ep_rand(p);
		BENCH_ADD(pp_add_k54_projc(e, qx, qy, qz, qx, qy, p));
	}
	BENCH_END;
#endif

	BENCH_RUN("pp_dbl_k54") {
		fp9_rand(qx);
		fp9_rand(qy);
		fp9_rand(qz);
		ep_rand(p);
		BENCH_ADD(pp_dbl_k54(e, qx, qy, qz, p));
	}
	BENCH_END;

	#if EP_ADD == BASIC || !defined(STRIP)
		BENCH_RUN("pp_dbl_k54_basic") {
			fp9_rand(qx);
			fp9_rand(qy);
			ep_rand(p);
			BENCH_ADD(pp_dbl_k54_basic(e, qx, qy, p));
		}
		BENCH_END;
	#endif

	#if EP_ADD == PROJC || !defined(STRIP)
		BENCH_RUN("pp_dbl_k54_projc") {
			fp9_rand(qx);
			fp9_rand(qy);
			fp9_rand(qz);
			ep_rand(p);
			BENCH_ADD(pp_dbl_k54_projc(e, qx, qy, qz, p));
		}
		BENCH_END;
	#endif

	BENCH_RUN("pp_exp_k54") {
		fp54_rand(e);
		BENCH_ADD(pp_exp_k54(e, e));
	}
	BENCH_END;

	BENCH_RUN("pp_map_k54") {
		fp9_rand(qx);
		fp9_rand(qy);
		fp9_rand(qz);
		ep_rand(p);
		BENCH_ADD(pp_map_k54(e, p, qx, qy));
	}
	BENCH_END;

	ep_free(p);
	fp9_free(qx);
	fp9_free(qy);
	fp9_free(qz);
	fp54_free(e);
}

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	conf_print();

	util_banner("Benchmarks for the PP module:", 0);

	if (ep_param_set_any_pairf() != RLC_OK) {
		RLC_THROW(ERR_NO_CURVE);
		core_clean();
		return 0;
	}

	ep_param_print();
	util_banner("Arithmetic:", 1);

	if (ep_param_embed() == 2) {
		pairing2();
	}

	if (ep_param_embed() == 8) {
		pairing8();
	}

	if (ep_param_embed() == 12) {
		pairing12();
	}

	if (ep_param_embed() == 48) {
		pairing48();
	}

	if (ep_param_embed() == 54) {
		pairing54();
	}

	core_clean();
	return 0;
}
