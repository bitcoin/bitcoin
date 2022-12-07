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
 * Tests for pairings defined over prime elliptic curves.
 *
 * @ingroup test
 */

#include <stdio.h>

#include "relic.h"
#include "relic_test.h"
#include "relic_bench.h"

static int addition2(void) {
	int code = RLC_ERR;
	bn_t k, n;
	ep_t p, q, r, s;
	fp2_t e1, e2;

	bn_null(k);
	bn_null(n);
	ep_null(p);
	ep_null(q);
	ep_null(r);
	ep_null(s);
	fp2_null(e1);
	fp2_null(e2);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ep_new(p);
		ep_new(q);
		ep_new(r);
		ep_new(s);
		fp2_new(e1);
		fp2_new(e2);

		ep_curve_get_ord(n);

		TEST_CASE("miller addition is correct") {
			ep_rand(p);
			ep_rand(q);
			ep_rand(r);
			ep_copy(s, r);
			pp_add_k2(e1, r, q, p);
			pp_norm_k2(r, r);
			ep_add(s, s, q);
			ep_norm(s, s);
			TEST_ASSERT(ep_cmp(r, s) == RLC_EQ, end);
		} TEST_END;

#if EP_ADD == BASIC || !defined(STRIP)
		TEST_CASE("miller addition in affine coordinates is correct") {
			ep_rand(p);
			ep_rand(q);
			ep_rand(r);
			ep_copy(s, r);
			fp2_zero(e1);
			fp2_zero(e2);
			pp_add_k2(e1, r, q, p);
			pp_exp_k2(e1, e1);
			pp_add_k2_basic(e2, s, q, p);
			pp_exp_k2(e2, e2);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_ADD == PROJC || EP_ADD == JACOB || !defined(STRIP)
		TEST_CASE("miller addition in projective coordinates is correct") {
			ep_rand(p);
			ep_rand(q);
			ep_rand(r);
			ep_copy(s, r);
			fp2_zero(e1);
			fp2_zero(e2);
			pp_add_k2(e1, r, q, p);
			pp_exp_k2(e1, e1);
			pp_add_k2_projc(e2, s, q, p);
			pp_exp_k2(e2, e2);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;

#if PP_EXT == BASIC || !defined(STRIP)
		TEST_CASE("basic projective miller addition is consistent") {
			ep_rand(p);
			ep_rand(q);
			ep_rand(r);
			ep_copy(s, r);
			fp2_zero(e1);
			fp2_zero(e2);
			pp_add_k2_projc(e1, r, q, p);
			pp_add_k2_projc_basic(e2, s, q, p);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
		TEST_CASE("lazy-reduced projective miller addition is consistent") {
			ep_rand(p);
			ep_rand(q);
			ep_rand(r);
			ep_copy(s, r);
			fp2_zero(e1);
			fp2_zero(e2);
			pp_add_k2_projc(e1, r, q, p);
			pp_add_k2_projc_lazyr(e2, s, q, p);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif
#endif /* EP_ADD = PROJC */
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	ep_free(p);
	ep_free(q);
	ep_free(r);
	ep_free(s);
	fp2_free(e1);
	fp2_free(e2);
	return code;
}

static int doubling2(void) {
	int code = RLC_ERR;
	bn_t k, n;
	ep_t p, q, r, s;
	fp2_t e1, e2;

	bn_null(k);
	bn_null(n);
	ep_null(p);
	ep_null(q);
	ep_null(r);
	ep_null(s);
	fp2_null(e1);
	fp2_null(e2);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ep_new(p);
		ep_new(q);
		ep_new(r);
		ep_new(s);
		fp2_new(e1);
		fp2_new(e2);

		ep_curve_get_ord(n);

		TEST_CASE("miller doubling is correct") {
			ep_rand(p);
			ep_rand(q);
			ep_rand(r);
			pp_dbl_k2(e1, r, q, p);
			pp_norm_k2(r, r);
			ep_dbl(s, q);
			ep_norm(s, s);
			TEST_ASSERT(ep_cmp(r, s) == RLC_EQ, end);
		} TEST_END;

#if EP_ADD == BASIC || !defined(STRIP)
		TEST_CASE("miller doubling in affine coordinates is correct") {
			ep_rand(p);
			ep_rand(q);
			ep_rand(r);
			fp2_zero(e1);
			fp2_zero(e2);
			pp_dbl_k2(e1, r, q, p);
			pp_exp_k2(e1, e1);
			pp_dbl_k2_basic(e2, r, q, p);
			pp_exp_k2(e2, e2);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_ADD == PROJC || EP_ADD == JACOB || !defined(STRIP)
		TEST_CASE("miller doubling in projective coordinates is correct") {
			ep_rand(p);
			ep_rand(q);
			ep_rand(r);
			fp2_zero(e1);
			fp2_zero(e2);
			pp_dbl_k2(e1, r, q, p);
			pp_exp_k2(e1, e1);
			pp_dbl_k2_projc(e2, r, q, p);
			pp_exp_k2(e2, e2);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;

#if PP_EXT == BASIC || !defined(STRIP)
		TEST_CASE("basic projective miller doubling is correct") {
			ep_rand(p);
			ep_rand(q);
			ep_rand(r);
			fp2_zero(e1);
			fp2_zero(e2);
			pp_dbl_k2_projc(e1, r, q, p);
			pp_dbl_k2_projc_basic(e2, r, q, p);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
		TEST_CASE("lazy-reduced projective miller doubling is consistent") {
			ep_rand(p);
			ep_rand(q);
			ep_rand(r);
			fp2_zero(e1);
			fp2_zero(e2);
			pp_dbl_k2_projc(e1, r, q, p);
			pp_dbl_k2_projc_lazyr(e2, r, q, p);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif
#endif /* EP_ADD = PROJC */
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	ep_free(p);
	ep_free(q);
	ep_free(r);
	ep_free(s);
	fp2_free(e1);
	fp2_free(e2);
	return code;
}

static int pairing2(void) {
	int j, code = RLC_ERR;
	bn_t k, n;
	ep_t p[2], q[2], r;
	fp2_t e1, e2;

	bn_null(k);
	bn_null(n);
	ep_null(r);
	fp2_null(e1);
	fp2_null(e2);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ep_new(r);
		fp2_new(e1);
		fp2_new(e2);

		for (j = 0; j < 2; j++) {
			ep_null(p[j]);
			ep_null(q[j]);
			ep_new(p[j]);
			ep_new(q[j]);
		}

		ep_curve_get_ord(n);

		TEST_CASE("pairing non-degeneracy is correct") {
			ep_rand(p[0]);
			ep_rand(q[0]);
			pp_map_k2(e1, p[0], q[0]);
			TEST_ASSERT(fp2_cmp_dig(e1, 1) != RLC_EQ, end);
			ep_set_infty(p[0]);
			pp_map_k2(e1, p[0], q[0]);
			TEST_ASSERT(fp2_cmp_dig(e1, 1) == RLC_EQ, end);
			ep_rand(p[0]);
			ep_set_infty(q[0]);
			pp_map_k2(e1, p[0], q[0]);
			TEST_ASSERT(fp2_cmp_dig(e1, 1) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("pairing is bilinear") {
			ep_rand(p[0]);
			ep_rand(q[0]);
			bn_rand_mod(k, n);
			ep_mul(r, q[0], k);
			pp_map_k2(e1, p[0], r);
			pp_map_k2(e2, p[0], q[0]);
			fp2_exp(e2, e2, k);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
			ep_mul(p[0], p[0], k);
			pp_map_k2(e2, p[0], q[0]);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
			ep_dbl(p[0], p[0]);
			pp_map_k2(e2, p[0], q[0]);
			fp2_sqr(e1, e1);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
			ep_dbl(q[0], q[0]);
			pp_map_k2(e2, p[0], q[0]);
			fp2_sqr(e1, e1);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;

        TEST_CASE("multi-pairing is correct") {
                ep_rand(p[i % 2]);
                ep_rand(q[i % 2]);
                pp_map_k2(e1, p[i % 2], q[i % 2]);
                ep_rand(p[1 - (i % 2)]);
                ep_set_infty(q[1 - (i % 2)]);
                pp_map_sim_k2(e2, p, q, 2);
                TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
                ep_set_infty(p[1 - (i % 2)]);
                ep_rand(q[1 - (i % 2)]);
                pp_map_sim_k2(e2, p, q, 2);
                TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
                ep_set_infty(q[i % 2]);
                pp_map_sim_k2(e2, p, q, 2);
                TEST_ASSERT(fp2_cmp_dig(e2, 1) == RLC_EQ, end);
                ep_rand(p[0]);
                ep_rand(q[0]);
                pp_map_k2(e1, p[0], q[0]);
                ep_rand(p[1]);
                ep_rand(q[1]);
                pp_map_k2(e2, p[1], q[1]);
                fp2_mul(e1, e1, e2);
                pp_map_sim_k2(e2, p, q, 2);
                TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
        } TEST_END;

#if PP_MAP == TATEP || PP_MAP == OATEP || !defined(STRIP)
		TEST_CASE("tate pairing non-degeneracy is correct") {
			ep_rand(p[0]);
			ep_rand(q[0]);
			pp_map_tatep_k2(e1, p[0], q[0]);
			TEST_ASSERT(fp2_cmp_dig(e1, 1) != RLC_EQ, end);
			ep_set_infty(p[0]);
			pp_map_tatep_k2(e1, p[0], q[0]);
			TEST_ASSERT(fp2_cmp_dig(e1, 1) == RLC_EQ, end);
			ep_rand(p[0]);
			ep_set_infty(q[0]);
			pp_map_tatep_k2(e1, p[0], q[0]);
			TEST_ASSERT(fp2_cmp_dig(e1, 1) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("tate pairing is bilinear") {
			ep_rand(p[0]);
			ep_rand(q[0]);
			bn_rand_mod(k, n);
			ep_mul(r, q[0], k);
			pp_map_tatep_k2(e1, p[0], r);
			pp_map_tatep_k2(e2, p[0], q[0]);
			fp2_exp(e2, e2, k);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
			ep_mul(p[0], p[0], k);
			pp_map_tatep_k2(e2, p[0], q[0]);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
			ep_dbl(p[0], p[0]);
			pp_map_tatep_k2(e2, p[0], q[0]);
			fp2_sqr(e1, e1);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
			ep_dbl(q[0], q[0]);
			pp_map_tatep_k2(e2, p[0], q[0]);
			fp2_sqr(e1, e1);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("tate multi-pairing is correct") {
			ep_rand(p[i % 2]);
			ep_rand(q[i % 2]);
			pp_map_tatep_k2(e1, p[i % 2], q[i % 2]);
			ep_rand(p[1 - (i % 2)]);
			ep_set_infty(q[1 - (i % 2)]);
			pp_map_sim_tatep_k2(e2, p, q, 2);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
			ep_set_infty(p[1 - (i % 2)]);
			ep_rand(q[1 - (i % 2)]);
			pp_map_sim_tatep_k2(e2, p, q, 2);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
			ep_set_infty(q[i % 2]);
			pp_map_sim_tatep_k2(e2, p, q, 2);
			TEST_ASSERT(fp2_cmp_dig(e2, 1) == RLC_EQ, end);
			ep_rand(p[0]);
			ep_rand(q[0]);
			pp_map_tatep_k2(e1, p[0], q[0]);
			ep_rand(p[1]);
			ep_rand(q[1]);
			pp_map_tatep_k2(e2, p[1], q[1]);
			fp2_mul(e1, e1, e2);
			pp_map_sim_tatep_k2(e2, p, q, 2);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if PP_MAP == WEIL || !defined(STRIP)
		TEST_CASE("weil pairing non-degeneracy is correct") {
			ep_rand(p[0]);
			ep_rand(q[0]);
			pp_map_weilp_k2(e1, p[0], q[0]);
			TEST_ASSERT(fp2_cmp_dig(e1, 1) != RLC_EQ, end);
			ep_set_infty(p[0]);
			pp_map_weilp_k2(e1, p[0], q[0]);
			TEST_ASSERT(fp2_cmp_dig(e1, 1) == RLC_EQ, end);
			ep_rand(p[0]);
			ep_set_infty(q[0]);
			pp_map_weilp_k2(e1, p[0], q[0]);
			TEST_ASSERT(fp2_cmp_dig(e1, 1) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("weil pairing is bilinear") {
			ep_rand(p[0]);
			ep_rand(q[0]);
			bn_rand_mod(k, n);
			ep_mul(r, q[0], k);
			pp_map_weilp_k2(e1, p[0], r);
			pp_map_weilp_k2(e2, p[0], q[0]);
			fp2_exp(e2, e2, k);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
			ep_mul(p[0], p[0], k);
			pp_map_weilp_k2(e2, p[0], q[0]);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
			ep_dbl(p[0], p[0]);
			pp_map_weilp_k2(e2, p[0], q[0]);
			fp2_sqr(e1, e1);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
			ep_dbl(q[0], q[0]);
			pp_map_weilp_k2(e2, p[0], q[0]);
			fp2_sqr(e1, e1);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("weil multi-pairing is correct") {
			ep_rand(p[i % 2]);
			ep_rand(q[i % 2]);
			pp_map_weilp_k2(e1, p[i % 2], q[i % 2]);
			ep_rand(p[1 - (i % 2)]);
			ep_set_infty(q[1 - (i % 2)]);
			pp_map_sim_weilp_k2(e2, p, q, 2);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
			ep_set_infty(p[1 - (i % 2)]);
			ep_rand(q[1 - (i % 2)]);
			pp_map_sim_weilp_k2(e2, p, q, 2);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
			ep_set_infty(q[i % 2]);
			pp_map_sim_weilp_k2(e2, p, q, 2);
			TEST_ASSERT(fp2_cmp_dig(e2, 1) == RLC_EQ, end);
			ep_rand(p[0]);
			ep_rand(q[0]);
			pp_map_weilp_k2(e1, p[0], q[0]);
			ep_rand(p[1]);
			ep_rand(q[1]);
			pp_map_weilp_k2(e2, p[1], q[1]);
			fp2_mul(e1, e1, e2);
			pp_map_sim_weilp_k2(e2, p, q, 2);
			TEST_ASSERT(fp2_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	ep_free(r);
	fp2_free(e1);
	fp2_free(e2);

	for (j = 0; j < 2; j++) {
		ep_free(p[j]);
		ep2_free(q[j]);
	}

    return code;
}

static int doubling8(void) {
	int code = RLC_ERR;
	bn_t k, n;
	ep_t p;
	ep2_t q, r, s;
	fp8_t e1, e2;

	bn_null(k);
	bn_null(n);
	ep_null(p);
	ep2_null(q);
	ep2_null(r);
	ep2_null(s);
	fp8_null(e1);
	fp8_null(e2);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ep_new(p);
		ep2_new(q);
		ep2_new(r);
		ep2_new(s);
		fp8_new(e1);
		fp8_new(e2);

		ep_curve_get_ord(n);

		TEST_CASE("miller doubling is correct") {
			ep_rand(p);
			ep2_curve_get_gen(q);
			ep2_rand(r);
			pp_dbl_k8(e1, r, q, p);
			pp_norm_k8(r, r);
			ep2_dbl(s, q);
			ep2_norm(s, s);
			TEST_ASSERT(ep2_cmp(r, s) == RLC_EQ, end);
		} TEST_END;

#if EP_ADD == BASIC || !defined(STRIP)
		TEST_CASE("miller doubling in affine coordinates is correct") {
			ep_rand(p);
			ep2_curve_get_gen(q);
			ep2_rand(r);
			fp8_zero(e1);
			fp8_zero(e2);
			fp_neg(p->y, p->y);
			pp_dbl_k8_basic(e2, r, q, p);
			pp_exp_k8(e2, e2);
#if EP_ADD == PROJC
			/* Precompute. */
			fp_neg(p->y, p->y);
			fp_neg(p->x, p->x);
#endif
			pp_dbl_k8(e1, r, q, p);
			pp_exp_k8(e1, e1);
			TEST_ASSERT(fp8_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_ADD == PROJC || EP_ADD == JACOB || !defined(STRIP)
		TEST_CASE("miller doubling in projective coordinates is correct") {
			ep_rand(p);
			ep2_curve_get_gen(q);
			ep2_rand(r);
			fp8_zero(e1);
			fp8_zero(e2);
			/* Precompute. */
			fp_neg(p->x, p->x);
			pp_dbl_k8_projc(e2, r, q, p);
			pp_exp_k8(e2, e2);
#if EP_ADD == BASIC
			/* Revert and fix precomputing. */
			fp_neg(p->x, p->x);
			fp_neg(p->y, p->y);
#endif
			pp_dbl_k8(e1, r, q, p);
			pp_exp_k8(e1, e1);
			TEST_ASSERT(fp8_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;

#if PP_EXT == BASIC || !defined(STRIP)
		TEST_CASE("basic projective miller doubling is correct") {
			ep_rand(p);
			ep2_rand(q);
			ep2_rand(r);
			fp8_zero(e1);
			fp8_zero(e2);
			pp_dbl_k8_projc(e1, r, q, p);
			pp_dbl_k8_projc_basic(e2, r, q, p);
			TEST_ASSERT(fp8_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
		TEST_CASE("lazy-reduced projective miller doubling is consistent") {
			ep_rand(p);
			ep2_rand(q);
			ep2_rand(r);
			fp8_zero(e1);
			fp8_zero(e2);
			pp_dbl_k8_projc(e1, r, q, p);
			pp_dbl_k8_projc_lazyr(e2, r, q, p);
			TEST_ASSERT(fp8_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif
#endif /* EP_ADD = PROJC */
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	ep_free(p);
	ep2_free(q);
	ep2_free(r);
	ep2_free(s);
	fp8_free(e1);
	fp8_free(e2);
	return code;
}

static int addition8(void) {
	int code = RLC_ERR;
	bn_t k, n;
	ep_t p;
	ep2_t q, r, s;
	fp8_t e1, e2;

	bn_null(k);
	bn_null(n);
	ep_null(p);
	ep2_null(q);
	ep2_null(r);
	ep2_null(s);
	fp8_null(e1);
	fp8_null(e2);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ep_new(p);
		ep2_new(q);
		ep2_new(r);
		ep2_new(s);
		fp8_new(e1);
		fp8_new(e2);

		ep_curve_get_ord(n);

		TEST_CASE("miller addition is correct") {
			ep_rand(p);
			ep2_curve_get_gen(q);
			ep2_dbl(r, q);
			ep2_norm(r, r);
			ep2_copy(s, r);
			pp_add_k8(e1, r, q, p);
			pp_norm_k8(r, r);
			ep2_add(s, s, q);
			ep2_norm(s, s);
			TEST_ASSERT(ep2_cmp(r, s) == RLC_EQ, end);
		} TEST_END;

#if EP_ADD == BASIC || !defined(STRIP)
		TEST_CASE("miller addition in affine coordinates is correct") {
			ep_rand(p);
			ep2_curve_get_gen(q);
			ep2_dbl(r, q);
			ep2_norm(r, r);
			ep2_copy(s, r);
			fp8_zero(e1);
			fp8_zero(e2);
#if EP_ADD == PROJC
			/* Precompute. */
			fp_neg(p->x, p->x);
#else
			fp_neg(p->y, p->y);
#endif
			pp_add_k8(e1, r, q, p);
			pp_exp_k8(e1, e1);
#if EP_ADD == PROJC
			/* Revert precompute. */
			fp_neg(p->x, p->x);
			fp_neg(p->y, p->y);
#endif
			pp_add_k8_basic(e2, s, q, p);
			pp_exp_k8(e2, e2);
			TEST_ASSERT(fp8_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_ADD == PROJC || EP_ADD == JACOB || !defined(STRIP)
		TEST_CASE("miller addition in projective coordinates is correct") {
			ep_rand(p);
			ep2_curve_get_gen(q);
			ep2_dbl(r, q);
			ep2_norm(r, r);
			ep2_copy(s, r);
			fp8_zero(e1);
			fp8_zero(e2);
#if EP_ADD == PROJC
			/* Precompute. */
			fp_neg(p->x, p->x);
#else
			fp_neg(p->y, p->y);
#endif
			pp_add_k8(e1, r, q, p);
			pp_exp_k8(e1, e1);
#if EP_ADD == BASIC
			fp_neg(p->x, p->x);
			fp_neg(p->y, p->y);
#endif
			pp_add_k8_projc(e2, s, q, p);
			pp_exp_k8(e2, e2);
			TEST_ASSERT(fp8_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	ep_free(p);
	ep2_free(q);
	ep2_free(r);
	ep2_free(s);
	fp8_free(e1);
	fp8_free(e2);
	return code;
}

static int pairing8(void) {
	int j, code = RLC_ERR;
	bn_t k, n;
	ep_t p[2];
	ep2_t q[2], r;
	fp8_t e1, e2;

	bn_null(k);
	bn_null(n);
	fp8_null(e1);
	fp8_null(e2);
	ep2_null(r);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		fp8_new(e1);
		fp8_new(e2);
		ep2_new(r);

		for (j = 0; j < 2; j++) {
			ep_null(p[j]);
			ep2_null(q[j]);
			ep_new(p[j]);
			ep2_new(q[j]);
		}

		ep_curve_get_ord(n);

		TEST_CASE("pairing non-degeneracy is correct") {
			ep_rand(p[0]);
			ep2_curve_get_gen(q[0]);
			pp_map_oatep_k8(e1, p[0], q[0]);
			TEST_ASSERT(fp8_cmp_dig(e1, 1) != RLC_EQ, end);
			ep_set_infty(p[0]);
			pp_map_oatep_k8(e1, p[0], q[0]);
			TEST_ASSERT(fp8_cmp_dig(e1, 1) == RLC_EQ, end);
			ep_rand(p[0]);
			ep2_set_infty(q[0]);
			pp_map_oatep_k8(e1, p[0], q[0]);
			TEST_ASSERT(fp8_cmp_dig(e1, 1) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("pairing is bilinear") {
			ep_rand(p[0]);
			ep2_curve_get_gen(q[0]);
			bn_rand_mod(k, n);
			ep2_mul_basic(r, q[0], k);
			pp_map_oatep_k8(e1, p[0], r);
			pp_map_oatep_k8(e2, p[0], q[0]);
			fp8_exp(e2, e2, k);
			TEST_ASSERT(fp8_cmp(e1, e2) == RLC_EQ, end);
			ep_mul(p[0], p[0], k);
			pp_map_oatep_k8(e2, p[0], q[0]);
			TEST_ASSERT(fp8_cmp(e1, e2) == RLC_EQ, end);
			ep_dbl(p[0], p[0]);
			pp_map_oatep_k8(e2, p[0], q[0]);
			fp8_sqr(e1, e1);
			TEST_ASSERT(fp8_cmp(e1, e2) == RLC_EQ, end);
			ep2_dbl(q[0], q[0]);
			pp_map_oatep_k8(e2, p[0], q[0]);
			fp8_sqr(e1, e1);
			TEST_ASSERT(fp8_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	fp8_free(e1);
	fp8_free(e2);
	ep2_free(r);

	for (j = 0; j < 2; j++) {
		ep_free(p[j]);
		ep2_free(q[j]);
	}
	return code;
}

static int doubling12(void) {
	int code = RLC_ERR;
	bn_t k, n;
	ep_t p;
	ep2_t q, r, s;
	fp12_t e1, e2;

	bn_null(k);
	bn_null(n);
	ep_null(p);
	ep2_null(q);
	ep2_null(r);
	ep2_null(s);
	fp12_null(e1);
	fp12_null(e2);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ep_new(p);
		ep2_new(q);
		ep2_new(r);
		ep2_new(s);
		fp12_new(e1);
		fp12_new(e2);

		ep_curve_get_ord(n);

		TEST_CASE("miller doubling is correct") {
			ep_rand(p);
			ep2_rand(q);
			ep2_rand(r);
			pp_dbl_k12(e1, r, q, p);
			pp_norm_k12(r, r);
			ep2_dbl(s, q);
			ep2_norm(s, s);
			TEST_ASSERT(ep2_cmp(r, s) == RLC_EQ, end);
		} TEST_END;

#if EP_ADD == BASIC || !defined(STRIP)
		TEST_CASE("miller doubling in affine coordinates is correct") {
			ep_rand(p);
			ep2_rand(q);
			ep2_rand(r);
			fp12_zero(e1);
			fp12_zero(e2);
			fp_neg(p->y, p->y);
			pp_dbl_k12_basic(e2, r, q, p);
			pp_exp_k12(e2, e2);
#if EP_ADD == PROJC || EP_ADD == JACOB
			/* Precompute. */
			fp_dbl(p->z, p->x);
			fp_add(p->x, p->z, p->x);
#endif
			pp_dbl_k12(e1, r, q, p);
			pp_exp_k12(e1, e1);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_ADD == PROJC || EP_ADD == JACOB || !defined(STRIP)
		TEST_CASE("miller doubling in projective coordinates is correct") {
			ep_rand(p);
			ep2_rand(q);
			ep2_rand(r);
			fp12_zero(e1);
			fp12_zero(e2);
			/* Precompute. */
			fp_neg(p->y, p->y);
			fp_dbl(p->z, p->x);
			fp_add(p->x, p->z, p->x);
			pp_dbl_k12_projc(e2, r, q, p);
			pp_exp_k12(e2, e2);
#if EP_ADD == BASIC
			/* Revert precomputing. */
			fp_hlv(p->x, p->z);
#endif
			pp_dbl_k12(e1, r, q, p);
			pp_exp_k12(e1, e1);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;

#if PP_EXT == BASIC || !defined(STRIP)
		TEST_CASE("basic projective miller doubling is correct") {
			ep_rand(p);
			ep2_rand(q);
			ep2_rand(r);
			fp12_zero(e1);
			fp12_zero(e2);
			pp_dbl_k12_projc(e1, r, q, p);
			pp_dbl_k12_projc_basic(e2, r, q, p);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
		TEST_CASE("lazy-reduced projective miller doubling is consistent") {
			ep_rand(p);
			ep2_rand(q);
			ep2_rand(r);
			fp12_zero(e1);
			fp12_zero(e2);
			pp_dbl_k12_projc(e1, r, q, p);
			pp_dbl_k12_projc_lazyr(e2, r, q, p);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif
#endif /* EP_ADD = PROJC */
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	ep_free(p);
	ep2_free(q);
	ep2_free(r);
	ep2_free(s);
	fp12_free(e1);
	fp12_free(e2);
	return code;
}

static int addition12(void) {
	int code = RLC_ERR;
	bn_t k, n;
	ep_t p;
	ep2_t q, r, s;
	fp12_t e1, e2;

	bn_null(k);
	bn_null(n);
	ep_null(p);
	ep2_null(q);
	ep2_null(r);
	ep2_null(s);
	fp12_null(e1);
	fp12_null(e2);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ep_new(p);
		ep2_new(q);
		ep2_new(r);
		ep2_new(s);
		fp12_new(e1);
		fp12_new(e2);

		ep_curve_get_ord(n);

		TEST_CASE("miller addition is correct") {
			ep_rand(p);
			ep2_rand(q);
			ep2_rand(r);
			ep2_copy(s, r);
			pp_add_k12(e1, r, q, p);
			pp_norm_k12(r, r);
			ep2_add(s, s, q);
			ep2_norm(s, s);
			TEST_ASSERT(ep2_cmp(r, s) == RLC_EQ, end);
		} TEST_END;

#if EP_ADD == BASIC || !defined(STRIP)
		TEST_CASE("miller addition in affine coordinates is correct") {
			ep_rand(p);
			ep2_rand(q);
			ep2_rand(r);
			ep2_copy(s, r);
			fp12_zero(e1);
			fp12_zero(e2);
			pp_add_k12(e1, r, q, p);
			pp_exp_k12(e1, e1);
			pp_add_k12_basic(e2, s, q, p);
			pp_exp_k12(e2, e2);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_ADD == PROJC || EP_ADD == JACOB || !defined(STRIP)
		TEST_CASE("miller addition in projective coordinates is correct") {
			ep_rand(p);
			ep2_rand(q);
			ep2_rand(r);
			ep2_copy(s, r);
			fp12_zero(e1);
			fp12_zero(e2);
			pp_add_k12(e1, r, q, p);
			pp_exp_k12(e1, e1);
			pp_add_k12_projc(e2, s, q, p);
			pp_exp_k12(e2, e2);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;

#if PP_EXT == BASIC || !defined(STRIP)
		TEST_CASE("basic projective miller addition is consistent") {
			ep_rand(p);
			ep2_rand(q);
			ep2_rand(r);
			ep2_copy(s, r);
			fp12_zero(e1);
			fp12_zero(e2);
			pp_add_k12_projc(e1, r, q, p);
			pp_add_k12_projc_basic(e2, s, q, p);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if PP_EXT == LAZYR || !defined(STRIP)
		TEST_CASE("lazy-reduced projective miller addition is consistent") {
			ep_rand(p);
			ep2_rand(q);
			ep2_rand(r);
			ep2_copy(s, r);
			fp12_zero(e1);
			fp12_zero(e2);
			pp_add_k12_projc(e1, r, q, p);
			pp_add_k12_projc_lazyr(e2, s, q, p);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif
#endif /* EP_ADD = PROJC */
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	ep_free(p);
	ep2_free(q);
	ep2_free(r);
	ep2_free(s);
	fp12_free(e1);
	fp12_free(e2);
	return code;
}

static int pairing12(void) {
	int j, code = RLC_ERR;
	bn_t k, n;
	ep_t p[2];
	ep2_t q[2], r;
	fp12_t e1, e2;

	bn_null(k);
	bn_null(n);
	fp12_null(e1);
	fp12_null(e2);
	ep2_null(r);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		fp12_new(e1);
		fp12_new(e2);
		ep2_new(r);

		for (j = 0; j < 2; j++) {
			ep_null(p[j]);
			ep2_null(q[j]);
			ep_new(p[j]);
			ep2_new(q[j]);
		}

		ep_curve_get_ord(n);

		TEST_CASE("pairing non-degeneracy is correct") {
			ep_rand(p[0]);
			ep2_rand(q[0]);
			pp_map_k12(e1, p[0], q[0]);
			TEST_ASSERT(fp12_cmp_dig(e1, 1) != RLC_EQ, end);
			ep_set_infty(p[0]);
			pp_map_k12(e1, p[0], q[0]);
			TEST_ASSERT(fp12_cmp_dig(e1, 1) == RLC_EQ, end);
			ep_rand(p[0]);
			ep2_set_infty(q[0]);
			pp_map_k12(e1, p[0], q[0]);
			TEST_ASSERT(fp12_cmp_dig(e1, 1) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("pairing is bilinear") {
			ep_rand(p[0]);
			ep2_rand(q[0]);
			bn_rand_mod(k, n);
			ep2_mul(r, q[0], k);
			pp_map_k12(e1, p[0], r);
			pp_map_k12(e2, p[0], q[0]);
			fp12_exp(e2, e2, k);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep_mul(p[0], p[0], k);
			pp_map_k12(e2, p[0], q[0]);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep_dbl(p[0], p[0]);
			pp_map_k12(e2, p[0], q[0]);
			fp12_sqr(e1, e1);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep2_dbl(q[0], q[0]);
			pp_map_k12(e2, p[0], q[0]);
			fp12_sqr(e1, e1);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("multi-pairing is correct") {
			ep_rand(p[i % 2]);
			ep2_rand(q[i % 2]);
			pp_map_k12(e1, p[i % 2], q[i % 2]);
			ep_rand(p[1 - (i % 2)]);
			ep2_set_infty(q[1 - (i % 2)]);
			pp_map_sim_k12(e2, p, q, 2);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep_set_infty(p[1 - (i % 2)]);
			ep2_rand(q[1 - (i % 2)]);
			pp_map_sim_k12(e2, p, q, 2);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep2_set_infty(q[i % 2]);
			pp_map_sim_k12(e2, p, q, 2);
			TEST_ASSERT(fp12_cmp_dig(e2, 1) == RLC_EQ, end);
			ep_rand(p[0]);
			ep2_rand(q[0]);
			pp_map_k12(e1, p[0], q[0]);
			ep_rand(p[1]);
			ep2_rand(q[1]);
			pp_map_k12(e2, p[1], q[1]);
			fp12_mul(e1, e1, e2);
			pp_map_sim_k12(e2, p, q, 2);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;

#if PP_MAP == TATEP || !defined(STRIP)
		TEST_CASE("tate pairing non-degeneracy is correct") {
			ep_rand(p[0]);
			ep2_rand(q[0]);
			pp_map_tatep_k12(e1, p[0], q[0]);
			TEST_ASSERT(fp12_cmp_dig(e1, 1) != RLC_EQ, end);
			ep_set_infty(p[0]);
			pp_map_tatep_k12(e1, p[0], q[0]);
			TEST_ASSERT(fp12_cmp_dig(e1, 1) == RLC_EQ, end);
			ep_rand(p[0]);
			ep2_set_infty(q[0]);
			pp_map_tatep_k12(e1, p[0], q[0]);
			TEST_ASSERT(fp12_cmp_dig(e1, 1) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("tate pairing is bilinear") {
			ep_rand(p[0]);
			ep2_rand(q[0]);
			bn_rand_mod(k, n);
			ep2_mul(r, q[0], k);
			pp_map_tatep_k12(e1, p[0], r);
			pp_map_tatep_k12(e2, p[0], q[0]);
			fp12_exp(e2, e2, k);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep_mul(p[0], p[0], k);
			pp_map_tatep_k12(e2, p[0], q[0]);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep_dbl(p[0], p[0]);
			pp_map_tatep_k12(e2, p[0], q[0]);
			fp12_sqr(e1, e1);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep2_dbl(q[0], q[0]);
			pp_map_tatep_k12(e2, p[0], q[0]);
			fp12_sqr(e1, e1);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("tate multi-pairing is correct") {
			ep_rand(p[i % 2]);
			ep2_rand(q[i % 2]);
			pp_map_tatep_k12(e1, p[i % 2], q[i % 2]);
			ep_rand(p[1 - (i % 2)]);
			ep2_set_infty(q[1 - (i % 2)]);
			pp_map_sim_tatep_k12(e2, p, q, 2);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep_set_infty(p[1 - (i % 2)]);
			ep2_rand(q[1 - (i % 2)]);
			pp_map_sim_tatep_k12(e2, p, q, 2);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep2_set_infty(q[i % 2]);
			pp_map_sim_tatep_k12(e2, p, q, 2);
			TEST_ASSERT(fp12_cmp_dig(e2, 1) == RLC_EQ, end);
			ep_rand(p[0]);
			ep2_rand(q[0]);
			pp_map_tatep_k12(e1, p[0], q[0]);
			ep_rand(p[1]);
			ep2_rand(q[1]);
			pp_map_tatep_k12(e2, p[1], q[1]);
			fp12_mul(e1, e1, e2);
			pp_map_sim_tatep_k12(e2, p, q, 2);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if PP_MAP == WEIL || !defined(STRIP)
		TEST_CASE("weil pairing non-degeneracy is correct") {
			ep_rand(p[0]);
			ep2_rand(q[0]);
			pp_map_weilp_k12(e1, p[0], q[0]);
			TEST_ASSERT(fp12_cmp_dig(e1, 1) != RLC_EQ, end);
			ep_set_infty(p[0]);
			pp_map_weilp_k12(e1, p[0], q[0]);
			TEST_ASSERT(fp12_cmp_dig(e1, 1) == RLC_EQ, end);
			ep_rand(p[0]);
			ep2_set_infty(q[0]);
			pp_map_weilp_k12(e1, p[0], q[0]);
			TEST_ASSERT(fp12_cmp_dig(e1, 1) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("weil pairing is bilinear") {
			ep_rand(p[0]);
			ep2_rand(q[0]);
			bn_rand_mod(k, n);
			ep2_mul(r, q[0], k);
			pp_map_weilp_k12(e1, p[0], r);
			pp_map_weilp_k12(e2, p[0], q[0]);
			fp12_exp(e2, e2, k);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep_mul(p[0], p[0], k);
			pp_map_weilp_k12(e2, p[0], q[0]);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep_dbl(p[0], p[0]);
			pp_map_weilp_k12(e2, p[0], q[0]);
			fp12_sqr(e1, e1);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep2_dbl(q[0], q[0]);
			pp_map_weilp_k12(e2, p[0], q[0]);
			fp12_sqr(e1, e1);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("weil multi-pairing is correct") {
			ep_rand(p[i % 2]);
			ep2_rand(q[i % 2]);
			pp_map_weilp_k12(e1, p[i % 2], q[i % 2]);
			ep_rand(p[1 - (i % 2)]);
			ep2_set_infty(q[1 - (i % 2)]);
			pp_map_sim_weilp_k12(e2, p, q, 2);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep_set_infty(p[1 - (i % 2)]);
			ep2_rand(q[1 - (i % 2)]);
			pp_map_sim_weilp_k12(e2, p, q, 2);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep2_set_infty(q[i % 2]);
			pp_map_sim_weilp_k12(e2, p, q, 2);
			TEST_ASSERT(fp12_cmp_dig(e2, 1) == RLC_EQ, end);
			ep_rand(p[0]);
			ep2_rand(q[0]);
			pp_map_weilp_k12(e1, p[0], q[0]);
			ep_rand(p[1]);
			ep2_rand(q[1]);
			pp_map_weilp_k12(e2, p[1], q[1]);
			fp12_mul(e1, e1, e2);
			pp_map_sim_weilp_k12(e2, p, q, 2);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if PP_MAP == OATEP || !defined(STRIP)
		TEST_CASE("optimal ate pairing non-degeneracy is correct") {
			ep_rand(p[0]);
			ep2_rand(q[0]);
			pp_map_oatep_k12(e1, p[0], q[0]);
			TEST_ASSERT(fp12_cmp_dig(e1, 1) != RLC_EQ, end);
			ep_set_infty(p[0]);
			pp_map_oatep_k12(e1, p[0], q[0]);
			TEST_ASSERT(fp12_cmp_dig(e1, 1) == RLC_EQ, end);
			ep_rand(p[0]);
			ep2_set_infty(q[0]);
			pp_map_oatep_k12(e1, p[0], q[0]);
			TEST_ASSERT(fp12_cmp_dig(e1, 1) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("optimal ate pairing is bilinear") {
			ep_rand(p[0]);
			ep2_rand(q[0]);
			bn_rand_mod(k, n);
			ep2_mul(r, q[0], k);
			pp_map_oatep_k12(e1, p[0], r);
			ep_mul(p[0], p[0], k);
			pp_map_oatep_k12(e2, p[0], q[0]);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep_dbl(p[0], p[0]);
			pp_map_oatep_k12(e2, p[0], q[0]);
			fp12_sqr(e1, e1);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep2_dbl(q[0], q[0]);
			pp_map_oatep_k12(e2, p[0], q[0]);
			fp12_sqr(e1, e1);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("optimal ate multi-pairing is correct") {
			ep_rand(p[i % 2]);
			ep2_rand(q[i % 2]);
			pp_map_oatep_k12(e1, p[i % 2], q[i % 2]);
			ep_rand(p[1 - (i % 2)]);
			ep2_set_infty(q[1 - (i % 2)]);
			pp_map_sim_oatep_k12(e2, p, q, 2);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep_set_infty(p[1 - (i % 2)]);
			ep2_rand(q[1 - (i % 2)]);
			pp_map_sim_oatep_k12(e2, p, q, 2);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
			ep2_set_infty(q[i % 2]);
			pp_map_sim_oatep_k12(e2, p, q, 2);
			TEST_ASSERT(fp12_cmp_dig(e2, 1) == RLC_EQ, end);
			ep_rand(p[0]);
			ep2_rand(q[0]);
			pp_map_oatep_k12(e1, p[0], q[0]);
			ep_rand(p[1]);
			ep2_rand(q[1]);
			pp_map_oatep_k12(e2, p[1], q[1]);
			fp12_mul(e1, e1, e2);
			pp_map_sim_oatep_k12(e2, p, q, 2);
			TEST_ASSERT(fp12_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	fp12_free(e1);
	fp12_free(e2);
	ep2_free(r);

	for (j = 0; j < 2; j++) {
		ep_free(p[j]);
		ep2_free(q[j]);
	}
	return code;
}

static int doubling24(void) {
	int code = RLC_ERR;
	bn_t k, n;
	ep_t p;
	ep4_t q, r, s;
	fp24_t e1, e2;

	bn_null(k);
	bn_null(n);
	ep_null(p);
	ep4_null(q);
	ep4_null(r);
	ep4_null(s);
	fp24_null(e1);
	fp24_null(e2);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ep_new(p);
		ep4_new(q);
		ep4_new(r);
		ep4_new(s);
		fp24_new(e1);
		fp24_new(e2);

		ep_curve_get_ord(n);

		TEST_CASE("miller doubling is correct") {
			ep_rand(p);
			ep4_rand(q);
			ep4_rand(r);
			pp_dbl_k24(e1, r, q, p);
			pp_norm_k24(r, r);
			ep4_dbl(s, q);
			ep4_norm(s, s);
			TEST_ASSERT(ep4_cmp(r, s) == RLC_EQ, end);
		} TEST_END;

#if EP_ADD == BASIC || !defined(STRIP)
		TEST_CASE("miller doubling in affine coordinates is correct") {
			ep_rand(p);
			ep4_rand(q);
			fp24_zero(e1);
			fp24_zero(e2);
			fp_neg(p->y, p->y);
			pp_dbl_k24_basic(e2, r, q, p);
			pp_exp_k24(e2, e2);
#if EP_ADD == PROJC
			/* Precompute. */
			fp_dbl(p->z, p->x);
			fp_add(p->x, p->z, p->x);
#endif
			pp_dbl_k24(e1, r, q, p);
			pp_exp_k24(e1, e1);
			TEST_ASSERT(fp24_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_ADD == PROJC || EP_ADD == JACOB || !defined(STRIP)
		TEST_CASE("miller doubling in projective coordinates is correct") {
			ep_rand(p);
			ep4_rand(q);
			fp24_zero(e1);
			fp24_zero(e2);
			/* Precompute. */
			fp_neg(p->y, p->y);
			fp_dbl(p->z, p->x);
			fp_add(p->x, p->z, p->x);
			pp_dbl_k24_projc(e2, r, q, p);
			pp_exp_k24(e2, e2);
#if EP_ADD == BASIC
			/* Revert precomputing. */
			fp_hlv(p->x, p->z);
#endif
			pp_dbl_k24(e1, r, q, p);
			pp_exp_k24(e1, e1);
			TEST_ASSERT(fp24_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	ep_free(p);
	ep4_free(q);
	ep4_free(r);
	ep4_free(s);
	fp24_free(e1);
	fp24_free(e2);
	return code;
}

static int addition24(void) {
	int code = RLC_ERR;
	bn_t k, n;
	ep_t p;
	ep4_t q, r, s;
	fp24_t e1, e2;

	bn_null(k);
	bn_null(n);
	ep_null(p);
	ep4_null(q);
	ep4_null(r);
	ep4_null(s);
	fp24_null(e1);
	fp24_null(e2);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ep_new(p);
		ep4_new(q);
		ep4_new(r);
		ep4_new(s);
		fp24_new(e1);
		fp24_new(e2);

		ep_curve_get_ord(n);

		TEST_CASE("miller addition is correct") {
			ep_rand(p);
			ep4_rand(q);
			ep4_rand(r);
			ep4_copy(s, r);
			pp_add_k24(e1, r, q, p);
			pp_norm_k24(r, r);
			ep4_add(s, s, q);
			ep4_norm(s, s);
			TEST_ASSERT(ep4_cmp(r, s) == RLC_EQ, end);
		} TEST_END;

#if EP_ADD == BASIC || !defined(STRIP)
		TEST_CASE("miller addition in affine coordinates is correct") {
			ep_rand(p);
			ep4_rand(q);
			ep4_rand(r);
			ep4_copy(s, r);
			fp24_zero(e1);
			fp24_zero(e2);
			pp_add_k24(e1, r, q, p);
			pp_exp_k24(e1, e1);
			pp_add_k24_basic(e2, s, q, p);
			pp_exp_k24(e2, e2);
			TEST_ASSERT(fp24_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_ADD == PROJC || EP_ADD == JACOB || !defined(STRIP)
		TEST_CASE("miller addition in projective coordinates is correct") {
			ep_rand(p);
			ep4_rand(q);
			ep4_rand(r);
			ep4_copy(s, r);
			fp24_zero(e1);
			fp24_zero(e2);
			pp_add_k24(e1, r, q, p);
			pp_exp_k24(e1, e1);
			pp_add_k24_projc(e2, s, q, p);
			pp_exp_k24(e2, e2);
			TEST_ASSERT(fp24_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	ep_free(p);
	ep4_free(q);
	ep4_free(r);
	ep4_free(s);
	fp24_free(e1);
	fp24_free(e2);
	return code;
}

static int pairing24(void) {
	int j, code = RLC_ERR;
	bn_t k, n;
	ep_t p[2];
	ep4_t q[2], r;
	fp24_t e1, e2;

	bn_null(k);
	bn_null(n);
	fp24_null(e1);
	fp24_null(e2);
	ep4_null(r);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		fp24_new(e1);
		fp24_new(e2);
		ep4_new(r);

		for (j = 0; j < 2; j++) {
			ep_null(p[j]);
			ep4_null(q[j]);
			ep_new(p[j]);
			ep4_new(q[j]);
		}

		ep_curve_get_ord(n);

		TEST_CASE("pairing non-degeneracy is correct") {
			ep_rand(p[0]);
			ep4_rand(q[0]);
			pp_map_k24(e1, p[0], q[0]);
			TEST_ASSERT(fp24_cmp_dig(e1, 1) != RLC_EQ, end);
			ep_set_infty(p[0]);
			pp_map_k24(e1, p[0], q[0]);
			TEST_ASSERT(fp24_cmp_dig(e1, 1) == RLC_EQ, end);
			ep_rand(p[0]);
			ep4_set_infty(q[0]);
			pp_map_k24(e1, p[0], q[0]);
			TEST_ASSERT(fp24_cmp_dig(e1, 1) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("pairing is bilinear") {
			ep_rand(p[0]);
			ep4_rand(q[0]);
			bn_rand_mod(k, n);
			ep4_mul(r, q[0], k);
			pp_map_k24(e1, p[0], r);
			pp_map_k24(e2, p[0], q[0]);
			fp24_exp(e2, e2, k);
			TEST_ASSERT(fp24_cmp(e1, e2) == RLC_EQ, end);
			ep_mul(p[0], p[0], k);
			pp_map_k24(e2, p[0], q[0]);
			TEST_ASSERT(fp24_cmp(e1, e2) == RLC_EQ, end);
			ep_dbl(p[0], p[0]);
			pp_map_k24(e2, p[0], q[0]);
			fp24_sqr(e1, e1);
			TEST_ASSERT(fp24_cmp(e1, e2) == RLC_EQ, end);
			ep4_dbl(q[0], q[0]);
			pp_map_k24(e2, p[0], q[0]);
			fp24_sqr(e1, e1);
			TEST_ASSERT(fp24_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("multi-pairing is correct") {
			ep_rand(p[i % 2]);
			ep4_rand(q[i % 2]);
			pp_map_k24(e1, p[i % 2], q[i % 2]);
			ep_rand(p[1 - (i % 2)]);
			ep4_set_infty(q[1 - (i % 2)]);
			pp_map_sim_k24(e2, p, q, 2);
			TEST_ASSERT(fp24_cmp(e1, e2) == RLC_EQ, end);
			ep_set_infty(p[1 - (i % 2)]);
			ep4_rand(q[1 - (i % 2)]);
			pp_map_sim_k24(e2, p, q, 2);
			TEST_ASSERT(fp24_cmp(e1, e2) == RLC_EQ, end);
			ep4_set_infty(q[i % 2]);
			pp_map_sim_k24(e2, p, q, 2);
			TEST_ASSERT(fp24_cmp_dig(e2, 1) == RLC_EQ, end);
			ep_rand(p[0]);
			ep4_rand(q[0]);
			pp_map_k24(e1, p[0], q[0]);
			ep_rand(p[1]);
			ep4_rand(q[1]);
			pp_map_k24(e2, p[1], q[1]);
			fp24_mul(e1, e1, e2);
			pp_map_sim_k24(e2, p, q, 2);
			TEST_ASSERT(fp24_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	fp24_free(e1);
	fp24_free(e2);
	ep4_free(r);

	for (j = 0; j < 2; j++) {
		ep_free(p[j]);
		ep4_free(q[j]);
	}
	return code;
}

/* Put test vectors here until we implement E(Fp^8). */
#define QX000 "266A6ACAA4B8DDCFBF97F09DFBEB01999BFBFF872276FA7700114F761E8971C6C25A53CC77E96BCC9579F63D8A39D641B8070B07EF40E93C301A5B49CE87110CC30E044BEE5A2D43"
#define QX001 "5009EEB2A67C52B79D0727B408A193FFCE76B4F80C8DCF4D61ECEE5471601CD7A94341F697CE9D375DB5470EA055B73C256CCC0AC12F52EAD276C26E001DDCE02DE634BEFCB9CC7C"
#define QX010 "11A8DDB59724C01696BE52862B5AC2C7E1C0C42EFCAF456A80F6C6D9F95F89649D5575DE3BA8D28D1012E14308DE1D8F15CE1617611F961032B0B5DFA27EF3E3670B9B537ACC66B9"
#define QX011 "4E8BDED03587581A173AD008DFF600EB24054E4CDDCA8D7BFABA2898822DB5ED701BF59BD3F108AD7C714B6A6C7ECB11A1BC5DEEC1D49AE7FCA597C43943A66441B03164975D9BE0"
#define QX100 "29E2751CAC7D0FBA8E12CC818BDE6F2A7173D3C2ED74EC1991B936071DEB1AED1E07CDF71EA3501BEB4645C86BDC8A575898303FF6A058C7062F079F594E5B865626D0C031CF7E44"
#define QX101 "2F3A1BE54DFFB814DA4AE6311B9B1EEE6198CDB9F36CE92084272416462F4D0AC9ACAC025FDA6D3D0D1C239FE8CE4B7F22A1D0F65582DDA36EEB328843FDE5C0BDA02E871796CC8E"
#define QX110 "2FEE7B15EB22B05476A462374860140DCC9F00910E0555918D6357F6E32E623B88B893647AD07B615F364093D6F6D2A7B7614590A8833385B5A833563C0DD6C89AF89D06428E8AFE"
#define QX111 "005082322BB5E610DC0E61E3D01B8BFF23D195117F58B1FA68EC04A6769FEB754A58742C7F729E2A684386C40EFB871CB3D32A040966155649DC45C49E6FB5DEE58DB1586CBFD33E"

#define QY000 "3B603A4C408A402FB885B607C4A661BF92354D22F46945F222C6F51CCDEBF4006640346AE6BFD60F7E22240D4BF83EFDA1B575267A89597D7BC54FA4899BFBAAC4138E30C8DA55C0"
#define QY001 "35D3ECCC1F3C69A921E57CCDEA6C794A5ED01A53E19208854EA3B10D519CCCE64A30007CD7A57673567F2FFA070E5CE01C4E5C8BF1C61225DAC36A93C6524F4D0350C6358C67F85B"
#define QY010 "4228DA69A29E14E2CF00EC8FDD877CA9049DA161778A6ACE8DC275D4CE94C90AD9176280703AD9C6714A4865EF6160ED2FA7A5FC601025CF096AB6CF21B8FAA41421C7913DECF3B5"
#define QY011 "5273C1679E18D316C6988820E06335094FCCCD5E8FD870492EB96FCBC5B5494B2B9D0869C18309FF2D49CD80CF6E6FE1A660027A6E924831F8D5A070645A7B794BA7AE72507809AE"
#define QY100 "0ABD2F582F0D4C3C89770C13F02FF17CDEA5B22CDD661B6F82905ADCDC44E59900C5D09F8CAC90760CCC57D1987DE4BA21F34455E5B7394B68A7883E3F8D918AF308F0C3E6F98F4B"
#define QY101 "36FAE1DE9DD31FFE238526F618C14E5CB61EEA8E8E6D82235E43E45E306C5E60B4E5499BF4663516CE1202EF6CDE3B2E098E406B3186937483FC104A173707C6419F460A23ADF628"
#define QY110 "09E1BB455FCB47E98C5263B5098E2D148EC2EDEE5634B8F94F10AF9221D09BD60D28920342C11B1987A24B7F56AC4F5E290E7EA483727ED16FFC88C0F5EAD00892FDA66BA68FACE4"
#define QY111 "4F781C32F5CAFD446F299BE6BC600BF3482DF6ECAF4ABE3D410A7255B18A88DB77CC539CAC4A0D30A00690CCA8CC7661BCE042D0AC40FF8DC9171847A8E42696E4EF9DD8A5907A3D"

static int doubling48(void) {
	int code = RLC_ERR;
	bn_t k, n;
	ep_t p;
	fp8_t qx, qy, qz, rx, ry, rz;
	fp48_t e1, e2;

	bn_null(k);
	bn_null(n);
	ep_null(p);
	fp8_null(qx);
	fp8_null(qy);
	fp8_null(qz);
	fp8_null(rx);
	fp8_null(ry);
	fp8_null(rz);
	fp48_null(e1);
	fp48_null(e2);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ep_new(p);
		fp8_new(qx);
		fp8_new(qy);
		fp8_new(qz);
		fp8_new(rx);
		fp8_new(ry);
		fp8_new(rz);
		fp48_new(e1);
		fp48_new(e2);

		fp_read_str(qx[0][0][0], QX000, strlen(QX000), 16);
		fp_read_str(qx[0][0][1], QX001, strlen(QX001), 16);
		fp_read_str(qx[0][1][0], QX010, strlen(QX010), 16);
		fp_read_str(qx[0][1][1], QX011, strlen(QX011), 16);
		fp_read_str(qx[1][0][0], QX100, strlen(QX100), 16);
		fp_read_str(qx[1][0][1], QX101, strlen(QX101), 16);
		fp_read_str(qx[1][1][0], QX110, strlen(QX110), 16);
		fp_read_str(qx[1][1][1], QX111, strlen(QX111), 16);

		fp_read_str(qy[0][0][0], QY000, strlen(QY000), 16);
		fp_read_str(qy[0][0][1], QY001, strlen(QY001), 16);
		fp_read_str(qy[0][1][0], QY010, strlen(QY010), 16);
		fp_read_str(qy[0][1][1], QY011, strlen(QY011), 16);
		fp_read_str(qy[1][0][0], QY100, strlen(QY100), 16);
		fp_read_str(qy[1][0][1], QY101, strlen(QY101), 16);
		fp_read_str(qy[1][1][0], QY110, strlen(QY110), 16);
		fp_read_str(qy[1][1][1], QY111, strlen(QY111), 16);

		fp8_set_dig(qz, 1);

		ep_curve_get_ord(n);

		TEST_CASE("miller doubling is correct") {
			ep_rand(p);
			fp8_copy(rx, qx);
			fp8_copy(ry, qy);
			fp8_copy(rz, qz);
			pp_dbl_k48_projc(e1, rx, ry, rz, p);
			fp8_inv(rz, rz);
			fp8_mul(rx, rx, rz);
			fp8_mul(ry, ry, rz);
			pp_dbl_k48_basic(e2, qx, qy, p);
			TEST_ASSERT(fp8_cmp(rx, qx) == RLC_EQ && fp8_cmp(ry, qy) == RLC_EQ, end);
		} TEST_END;

#if EP_ADD == BASIC || !defined(STRIP)
		TEST_CASE("miller doubling in affine coordinates is correct") {
			ep_rand(p);
			fp8_copy(rx, qx);
			fp8_copy(ry, qy);
			fp8_copy(rz, qz);
			fp48_zero(e1);
			fp48_zero(e2);
			fp_neg(p->y, p->y);
			pp_dbl_k48_basic(e2, rx, ry, p);
			pp_exp_k48(e2, e2);
#if EP_ADD == PROJC
			/* Precompute. */
			fp_dbl(p->z, p->x);
			fp_add(p->x, p->z, p->x);
#endif
			fp8_copy(rx, qx);
			fp8_copy(ry, qy);
			fp8_copy(rz, qz);
			pp_dbl_k48(e1, rx, ry, rz, p);
			pp_exp_k48(e1, e1);
			TEST_ASSERT(fp48_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_ADD == PROJC || EP_ADD == JACOB || !defined(STRIP)
		TEST_CASE("miller doubling in projective coordinates is correct") {
			ep_rand(p);
			fp8_copy(rx, qx);
			fp8_copy(ry, qy);
			fp8_copy(rz, qz);
			fp48_zero(e1);
			fp48_zero(e2);
			/* Precompute. */
			fp_neg(p->y, p->y);
			fp_dbl(p->z, p->x);
			fp_add(p->x, p->z, p->x);
			pp_dbl_k48_projc(e2, rx, ry, rz, p);
			pp_exp_k48(e2, e2);
#if EP_ADD == BASIC
			/* Revert precomputing. */
			fp_hlv(p->x, p->z);
#endif
			fp8_copy(rx, qx);
			fp8_copy(ry, qy);
			fp8_copy(rz, qz);
			pp_dbl_k48(e1, rx, ry, rz, p);
			pp_exp_k48(e1, e1);
			TEST_ASSERT(fp48_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	ep_free(p);
	fp8_free(qx);
	fp8_free(qy);
	fp8_free(qz);
	fp8_free(rx);
	fp8_free(ry);
	fp8_free(rz);
	fp48_free(e1);
	fp48_free(e2);
	return code;
}

static int addition48(void) {
	int code = RLC_ERR;
	bn_t k, n;
	ep_t p;
	fp8_t qx, qy, qz, rx, ry, rz;
	fp48_t e1, e2;

	bn_null(k);
	bn_null(n);
	ep_null(p);
	fp8_null(qx);
	fp8_null(qy);
	fp8_null(qz);
	fp8_null(rx);
	fp8_null(ry);
	fp8_null(rz);
	fp48_null(e1);
	fp48_null(e2);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ep_new(p);
		fp8_new(qx);
		fp8_new(qy);
		fp8_new(qz);
		fp8_new(rx);
		fp8_new(ry);
		fp8_new(rz);
		fp48_new(e1);
		fp48_new(e2);

		fp_read_str(qx[0][0][0], QX000, strlen(QX000), 16);
		fp_read_str(qx[0][0][1], QX001, strlen(QX001), 16);
		fp_read_str(qx[0][1][0], QX010, strlen(QX010), 16);
		fp_read_str(qx[0][1][1], QX011, strlen(QX011), 16);
		fp_read_str(qx[1][0][0], QX100, strlen(QX100), 16);
		fp_read_str(qx[1][0][1], QX101, strlen(QX101), 16);
		fp_read_str(qx[1][1][0], QX110, strlen(QX110), 16);
		fp_read_str(qx[1][1][1], QX111, strlen(QX111), 16);

		fp_read_str(qy[0][0][0], QY000, strlen(QY000), 16);
		fp_read_str(qy[0][0][1], QY001, strlen(QY001), 16);
		fp_read_str(qy[0][1][0], QY010, strlen(QY010), 16);
		fp_read_str(qy[0][1][1], QY011, strlen(QY011), 16);
		fp_read_str(qy[1][0][0], QY100, strlen(QY100), 16);
		fp_read_str(qy[1][0][1], QY101, strlen(QY101), 16);
		fp_read_str(qy[1][1][0], QY110, strlen(QY110), 16);
		fp_read_str(qy[1][1][1], QY111, strlen(QY111), 16);

		fp8_set_dig(qz, 1);

		ep_curve_get_ord(n);

		TEST_CASE("miller addition is correct") {
			ep_rand(p);
			fp8_copy(rx, qx);
			fp8_copy(ry, qy);
			fp8_copy(rz, qz);
			pp_dbl_k48(e1, rx, ry, rz, p);
			pp_add_k48_projc(e1, rx, ry, rz, qx, qy, p);
			fp8_inv(rz, rz);
			fp8_mul(rx, rx, rz);
			fp8_mul(ry, ry, rz);
			fp8_copy(e1[0][0], rx);
			fp8_copy(e1[0][1], ry);
			fp8_copy(rx, qx);
			fp8_copy(ry, qy);
			fp8_copy(rz, qz);
			pp_dbl_k48(e2, rx, ry, rz, p);
#if EP_ADD == PROJC
			fp8_inv(rz, rz);
			fp8_mul(rx, rx, rz);
			fp8_mul(ry, ry, rz);
#endif
			pp_add_k48_basic(e2, rx, ry, qx, qy, p);
			TEST_ASSERT(fp8_cmp(rx, e1[0][0]) == RLC_EQ && fp8_cmp(ry, e1[0][1]) == RLC_EQ, end);
		} TEST_END;

#if EP_ADD == BASIC || !defined(STRIP)
		TEST_CASE("miller addition in affine coordinates is correct") {
			ep_rand(p);
			fp8_copy(rx, qx);
			fp8_copy(ry, qy);
			fp8_copy(rz, qz);
			fp48_zero(e1);
			fp48_zero(e2);
			pp_dbl_k48(e1, rx, ry, rz, p);
			pp_add_k48(e1, rx, ry, rz, qx, qy, p);
			pp_exp_k48(e1, e1);
			fp8_copy(rx, qx);
			fp8_copy(ry, qy);
			fp8_copy(rz, qz);
			pp_dbl_k48(e2, rx, ry, rz, p);
#if EP_ADD == PROJC
			fp8_inv(rz, rz);
			fp8_mul(rx, rx, rz);
			fp8_mul(ry, ry, rz);
#endif
			pp_add_k48_basic(e2, rx, ry, qx, qy, p);
			pp_exp_k48(e2, e2);
			TEST_ASSERT(fp48_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_ADD == BASIC || !defined(STRIP)
		TEST_CASE("miller addition in projective coordinates is correct") {
			ep_rand(p);
			fp8_copy(rx, qx);
			fp8_copy(ry, qy);
			fp8_copy(rz, qz);
			fp48_zero(e1);
			fp48_zero(e2);
			pp_dbl_k48(e1, rx, ry, rz, p);
			pp_add_k48(e1, rx, ry, rz, qx, qy, p);
			pp_exp_k48(e1, e1);
			fp8_copy(rx, qx);
			fp8_copy(ry, qy);
			fp8_copy(rz, qz);
			pp_dbl_k48(e2, rx, ry, rz, p);
			pp_add_k48_projc(e2, rx, ry, rz, qx, qy, p);
			pp_exp_k48(e2, e2);
			TEST_ASSERT(fp48_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	ep_free(p);
	fp8_free(qx);
	fp8_free(qy);
	fp8_free(qz);
	fp8_free(rx);
	fp8_free(ry);
	fp8_free(rz);
	fp48_free(e1);
	fp48_free(e2);
	return code;
}

static int pairing48(void) {
	int code = RLC_ERR;
	bn_t k, n;
	ep_t p;
	fp8_t qx, qy, qz;
	fp48_t e1, e2;

	bn_null(k);
	bn_null(n);
	ep_null(p);
	fp8_null(qx);
	fp8_null(qy);
	fp8_null(qz);
	fp48_null(e1);
	fp48_null(e2);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ep_new(p);
		fp8_new(qx);
		fp8_new(qy);
		fp8_new(qz);
		fp48_new(e1);
		fp48_new(e2);

		ep_curve_get_ord(n);

		fp_read_str(qx[0][0][0], QX000, strlen(QX000), 16);
		fp_read_str(qx[0][0][1], QX001, strlen(QX001), 16);
		fp_read_str(qx[0][1][0], QX010, strlen(QX010), 16);
		fp_read_str(qx[0][1][1], QX011, strlen(QX011), 16);
		fp_read_str(qx[1][0][0], QX100, strlen(QX100), 16);
		fp_read_str(qx[1][0][1], QX101, strlen(QX101), 16);
		fp_read_str(qx[1][1][0], QX110, strlen(QX110), 16);
		fp_read_str(qx[1][1][1], QX111, strlen(QX111), 16);

		fp_read_str(qy[0][0][0], QY000, strlen(QY000), 16);
		fp_read_str(qy[0][0][1], QY001, strlen(QY001), 16);
		fp_read_str(qy[0][1][0], QY010, strlen(QY010), 16);
		fp_read_str(qy[0][1][1], QY011, strlen(QY011), 16);
		fp_read_str(qy[1][0][0], QY100, strlen(QY100), 16);
		fp_read_str(qy[1][0][1], QY101, strlen(QY101), 16);
		fp_read_str(qy[1][1][0], QY110, strlen(QY110), 16);
		fp_read_str(qy[1][1][1], QY111, strlen(QY111), 16);

		TEST_CASE("pairing non-degeneracy is correct") {
			ep_rand(p);
			pp_map_k48(e1, p, qx, qy);
			TEST_ASSERT(fp48_cmp_dig(e1, 1) != RLC_EQ, end);
		} TEST_END;

		TEST_CASE("pairing is bilinear") {
			ep_rand(p);
			bn_rand_mod(k, n);
			pp_map_k48(e1, p, qx, qy);
			ep_mul(p, p, k);
			pp_map_k48(e2, p, qx, qy);
			fp48_exp(e1, e1, k);
			TEST_ASSERT(fp48_cmp(e1, e2) == RLC_EQ, end);
			fp8_set_dig(qz, 1);
			pp_dbl_k48(e2, qx, qy, qz, p);
			fp8_inv(qz, qz);
			fp8_mul(qx, qx, qz);
			fp8_mul(qy, qy, qz);
			fp8_set_dig(qz, 1);
			pp_map_k48(e2, p, qx, qy);
			fp48_sqr(e1, e1);
			TEST_ASSERT(fp48_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	ep_free(p);
	fp8_free(qx);
	fp8_free(qy);
	fp8_free(qz);
	fp48_free(e1);
	fp48_free(e2);
	return code;
}

/* Put test vectors here until we implement E(Fp^9). */
#define QX00 "1E389F332DF2021EA1184765A5F47349F170E9230ED98CF5F35AC4DD7491E0244A2CEA197FBF7527505DD4A8455DD23C042567F3E33393A01DB07985DC2DD27826D19522060E01"
#define QX01 "BAE3E7B243BE1100DAED0C041346351C0937F37E71E57A18B90311457F5AEEC704B9E6697FB246AEE22CF7A5BA48F6821ACDAE053CDF35A9A32FDF789673C8F15F837035F0FE52"
#define QX02 "194C03CC03E3D13996CD05B07F4026FC89017EC2B8B257FE567E164A418FD42958BF835F3395D2085F7B8FE92F0048F8FB5629F9D70EF56AB2FC7495999A8EA4FEE35F34A73075"
#define QX10 "B068EC27D441FC14583F57494CBF3AD7117A72CF8AFBFC3B5D046E0BD0849352CF23A0E5505F25A3B17936B789803AFA7AAE3D14BDFA9CB3C9804D7516CF7295E2656F921A08B4"
#define QX11 "0F3350C3168AB6A4A8F0D35D6F10CB4CB56440CEC4D3E826BB5492CD370712F0D13F73B197A3F7E9BECF46F8B0FB02B354ED86A4506A1AEBBF063A1FFC3BB519AAEBDFC96637EB"
#define QX12 "56881E19298D0DB323F4054BEF75803E917EA9332D2A5278713FCF68AF5BFBFCF0A07AAFFB3FAEFAB271BCA40A34C489DF9E23D3235FD3C15045AD797C1C10C9261EC6006ACE11"
#define QX20 "E75291962F8B6069888AE07BA7B8FBFF0F6C5D82CF1B240C983F3D2FC220B90D0C52E9194B55395FBE55BF66BF6178494F0B9CC16B9ED3A0924A4A225083086D850531FF47768C"
#define QX21 "4F0BE29A5B0B20CC998F84BE209F1007A56A5742BEDBC9EA5768131E15EC5316B97A5A41C7F118224933BF1C170A79739153EE1DEC5A084E13DF783E14EBF34096605E181EF35A"
#define QX22 "3B8F6AAF5067AF990974F8BEC58E3B8151DC61646C6E523CCFDD3A27B1588993607EF700D3F8A27DDA71E3CED9324282E9F6E394433921F655BECAB2E5AC08DBA9207A8DB99D39"

#define QY00 "FB32F805837AC53F50C4E9238B2808AA35A854DBF610BEF72A180B54E4BCDBFEC5706A41CF43258C6AC1DD857F3B94AE58C2F80CAB0E977B33145E481A1E8F848D15448006FF55"
#define QY01 "2864D2E200BCC3383F58BFFB73A81C177343F4661A1B8CE3723EDC1AB71C89AE240B587A654E7D555DD7BFB211A43C4C463312A8A82FEBCE6B1797FE047A84265C7FD70FC00D81"
#define QY02 "B1A5A2A4CD6CD5238E684B24DA6579ADC41A9EBE499524154B7F14DF2F05F08D0A296930488786F0F47523961D9AF607A924530A88F8842D420181FA8DF94EDDB5092A6B678119"
#define QY10 "51BE5155AC3DB6B20222CAA2F6F115798F6D56E7C294C64B8D182FEF03FCFAB57E791C464B38CFFA81905211FADE928914B888C6CB000040EAF53CF700ED4A405E3146D63335E4"
#define QY11 "08ABC2FAC0D87900C653C1279AAAD421D36571F0F1F49C126F4AA3610C72704E6E986C088350044CF9D715649015559147B3B6B80A5C45519B84AF4C302953302FE4445731F4C4"
#define QY12 "C1A69378BF81042E108345F93026D0452BD9EF72262A6CFD8BF28ECEF6C80242192CFFEF77737AA474D62E5FFF1B4BFE8F8378256A2854D0B7C8B199621C22D22F827729A50F52"
#define QY20 "53CBD42DF27B32598712C729913FC1E5ACDFE16063CCC1A8EDE887223B2891407917B37D811E1A93AB716693E9D04A6AD35819E5F383CC5A22B1848BADD0FEB18CBD44055858D4"
#define QY21 "16DC6C7F1A5FB6FDCFBAA0944CC98398245EABA7CA607DDA5EEC6A2FBDF6DA1D6EB29A0E6941704AAF932810FEF372FE0FF7AA55C1F7EDE06B317DDED4B53D7D7F6D9A09A099D9"
#define QY22 "86C9BA4E7DDA70BB3FC1CE8245638F06CABC723A31CEC1427E6627AB6BBC716C54C5B30DB951BF2C1D7E07E74CD9E15837E79BC354DA011517BEE5395BA38F32E0C2C75F089FDC"

static int doubling54(void) {
	int code = RLC_ERR;
	bn_t k, n;
	ep_t p;
	fp9_t qx, qy, qz, rx, ry, rz;
	fp54_t e1, e2;

	bn_null(k);
	bn_null(n);
	ep_null(p);
	fp9_null(qx);
	fp9_null(qy);
	fp9_null(qz);
	fp9_null(rx);
	fp9_null(ry);
	fp9_null(rz);
	fp54_null(e1);
	fp54_null(e2);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ep_new(p);
		fp9_new(qx);
		fp9_new(qy);
		fp9_new(qz);
		fp9_new(rx);
		fp9_new(ry);
		fp9_new(rz);
		fp54_new(e1);
		fp54_new(e2);

		fp_read_str(qx[0][0], QX00, strlen(QX00), 16);
		fp_read_str(qx[0][1], QX01, strlen(QX01), 16);
		fp_read_str(qx[0][2], QX02, strlen(QX02), 16);
		fp_read_str(qx[1][0], QX10, strlen(QX10), 16);
		fp_read_str(qx[1][1], QX11, strlen(QX11), 16);
		fp_read_str(qx[1][2], QX12, strlen(QX12), 16);
		fp_read_str(qx[2][0], QX20, strlen(QX20), 16);
		fp_read_str(qx[2][1], QX21, strlen(QX21), 16);
		fp_read_str(qx[2][2], QX22, strlen(QX22), 16);

		fp_read_str(qy[0][0], QY00, strlen(QY00), 16);
		fp_read_str(qy[0][1], QY01, strlen(QY01), 16);
		fp_read_str(qy[0][2], QY02, strlen(QY02), 16);
		fp_read_str(qy[1][0], QY10, strlen(QY10), 16);
		fp_read_str(qy[1][1], QY11, strlen(QY11), 16);
		fp_read_str(qy[1][2], QY12, strlen(QY12), 16);
		fp_read_str(qy[2][0], QY20, strlen(QY20), 16);
		fp_read_str(qy[2][1], QY21, strlen(QY21), 16);
		fp_read_str(qy[2][2], QY22, strlen(QY22), 16);

		fp9_set_dig(qz, 1);

		ep_curve_get_ord(n);

		TEST_CASE("miller doubling is correct") {
			ep_rand(p);
			fp9_copy(rx, qx);
			fp9_copy(ry, qy);
			fp9_copy(rz, qz);
			pp_dbl_k54_projc(e1, rx, ry, rz, p);
			fp9_inv(rz, rz);
			fp9_mul(rx, rx, rz);
			fp9_mul(ry, ry, rz);
			pp_dbl_k54_basic(e2, qx, qy, p);
			TEST_ASSERT(fp9_cmp(rx, qx) == RLC_EQ && fp9_cmp(ry, qy) == RLC_EQ, end);
		} TEST_END;

#if EP_ADD == BASIC || !defined(STRIP)
		TEST_CASE("miller doubling in affine coordinates is correct") {
			ep_rand(p);
			fp9_copy(rx, qx);
			fp9_copy(ry, qy);
			fp9_copy(rz, qz);
			fp54_zero(e1);
			fp54_zero(e2);
			fp_neg(p->y, p->y);
			pp_dbl_k54_basic(e2, rx, ry, p);
			pp_exp_k54(e2, e2);
#if EP_ADD == PROJC
			/* Precompute. */
			fp_dbl(p->z, p->x);
			fp_add(p->x, p->z, p->x);
#endif
			fp9_copy(rx, qx);
			fp9_copy(ry, qy);
			fp9_copy(rz, qz);
			pp_dbl_k54(e1, rx, ry, rz, p);
			pp_exp_k54(e1, e1);
			TEST_ASSERT(fp54_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_ADD == PROJC || EP_ADD == JACOB || !defined(STRIP)
		TEST_CASE("miller doubling in projective coordinates is correct") {
			ep_rand(p);
			fp9_copy(rx, qx);
			fp9_copy(ry, qy);
			fp9_copy(rz, qz);
			fp54_zero(e1);
			fp54_zero(e2);
			/* Precompute. */
			fp_neg(p->y, p->y);
			fp_dbl(p->z, p->x);
			fp_add(p->x, p->z, p->x);
			pp_dbl_k54_projc(e2, rx, ry, rz, p);
			pp_exp_k54(e2, e2);
#if EP_ADD == BASIC
			/* Revert precomputing. */
			fp_hlv(p->x, p->z);
#endif
			fp9_copy(rx, qx);
			fp9_copy(ry, qy);
			fp9_copy(rz, qz);
			pp_dbl_k54(e1, rx, ry, rz, p);
			pp_exp_k54(e1, e1);
			TEST_ASSERT(fp54_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	ep_free(p);
	fp9_free(qx);
	fp9_free(qy);
	fp9_free(qz);
	fp9_free(rx);
	fp9_free(ry);
	fp9_free(rz);
	fp54_free(e1);
	fp54_free(e2);
	return code;
}

static int addition54(void) {
	int code = RLC_ERR;
	bn_t k, n;
	ep_t p;
	fp9_t qx, qy, qz, rx, ry, rz;
	fp54_t e1, e2;

	bn_null(k);
	bn_null(n);
	ep_null(p);
	fp9_null(qx);
	fp9_null(qy);
	fp9_null(qz);
	fp9_null(rx);
	fp9_null(ry);
	fp9_null(rz);
	fp54_null(e1);
	fp54_null(e2);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ep_new(p);
		fp9_new(qx);
		fp9_new(qy);
		fp9_new(qz);
		fp9_new(rx);
		fp9_new(ry);
		fp9_new(rz);
		fp54_new(e1);
		fp54_new(e2);

		fp_read_str(qx[0][0], QX00, strlen(QX00), 16);
		fp_read_str(qx[0][1], QX01, strlen(QX01), 16);
		fp_read_str(qx[0][2], QX02, strlen(QX02), 16);
		fp_read_str(qx[1][0], QX10, strlen(QX10), 16);
		fp_read_str(qx[1][1], QX11, strlen(QX11), 16);
		fp_read_str(qx[1][2], QX12, strlen(QX12), 16);
		fp_read_str(qx[2][0], QX20, strlen(QX20), 16);
		fp_read_str(qx[2][1], QX21, strlen(QX21), 16);
		fp_read_str(qx[2][2], QX22, strlen(QX22), 16);

		fp_read_str(qy[0][0], QY00, strlen(QY00), 16);
		fp_read_str(qy[0][1], QY01, strlen(QY01), 16);
		fp_read_str(qy[0][2], QY02, strlen(QY02), 16);
		fp_read_str(qy[1][0], QY10, strlen(QY10), 16);
		fp_read_str(qy[1][1], QY11, strlen(QY11), 16);
		fp_read_str(qy[1][2], QY12, strlen(QY12), 16);
		fp_read_str(qy[2][0], QY20, strlen(QY20), 16);
		fp_read_str(qy[2][1], QY21, strlen(QY21), 16);
		fp_read_str(qy[2][2], QY22, strlen(QY22), 16);

		fp9_set_dig(qz, 1);

		ep_curve_get_ord(n);

		TEST_CASE("miller addition is correct") {
			ep_rand(p);
			fp9_copy(rx, qx);
			fp9_copy(ry, qy);
			fp9_copy(rz, qz);
			pp_dbl_k54(e1, rx, ry, rz, p);
			pp_add_k54_projc(e1, rx, ry, rz, qx, qy, p);
			fp9_inv(rz, rz);
			fp9_mul(rx, rx, rz);
			fp9_mul(ry, ry, rz);
			fp9_copy(e1[0][0], rx);
			fp9_copy(e1[0][1], ry);
			fp9_copy(rx, qx);
			fp9_copy(ry, qy);
			fp9_copy(rz, qz);
			pp_dbl_k54(e2, rx, ry, rz, p);
#if EP_ADD == PROJC
			fp9_inv(rz, rz);
			fp9_mul(rx, rx, rz);
			fp9_mul(ry, ry, rz);
#endif
			pp_add_k54_basic(e2, rx, ry, qx, qy, p);
			TEST_ASSERT(fp9_cmp(rx, e1[0][0]) == RLC_EQ && fp9_cmp(ry, e1[0][1]) == RLC_EQ, end);
		} TEST_END;

#if EP_ADD == BASIC || !defined(STRIP)
		TEST_CASE("miller addition in affine coordinates is correct") {
			ep_rand(p);
			fp9_copy(rx, qx);
			fp9_copy(ry, qy);
			fp9_copy(rz, qz);
			fp54_zero(e1);
			fp54_zero(e2);
			pp_dbl_k54(e1, rx, ry, rz, p);
			pp_add_k54(e1, rx, ry, rz, qx, qy, p);
			pp_exp_k54(e1, e1);
			fp9_copy(rx, qx);
			fp9_copy(ry, qy);
			fp9_copy(rz, qz);
			pp_dbl_k54(e2, rx, ry, rz, p);
#if EP_ADD == PROJC
			fp9_inv(rz, rz);
			fp9_mul(rx, rx, rz);
			fp9_mul(ry, ry, rz);
#endif
			pp_add_k54_basic(e2, rx, ry, qx, qy, p);
			pp_exp_k54(e2, e2);
			TEST_ASSERT(fp54_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif

#if EP_ADD == BASIC || !defined(STRIP)
		TEST_CASE("miller addition in projective coordinates is correct") {
			ep_rand(p);
			fp9_copy(rx, qx);
			fp9_copy(ry, qy);
			fp9_copy(rz, qz);
			fp54_zero(e1);
			fp54_zero(e2);
			pp_dbl_k54(e1, rx, ry, rz, p);
			pp_add_k54(e1, rx, ry, rz, qx, qy, p);
			pp_exp_k54(e1, e1);
			fp9_copy(rx, qx);
			fp9_copy(ry, qy);
			fp9_copy(rz, qz);
			pp_dbl_k54(e2, rx, ry, rz, p);
			pp_add_k54_projc(e2, rx, ry, rz, qx, qy, p);
			pp_exp_k54(e2, e2);
			TEST_ASSERT(fp54_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
#endif
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	ep_free(p);
	fp9_free(qx);
	fp9_free(qy);
	fp9_free(qz);
	fp9_free(rx);
	fp9_free(ry);
	fp9_free(rz);
	fp54_free(e1);
	fp54_free(e2);
	return code;
}

static int pairing54(void) {
	int code = RLC_ERR;
	bn_t k, n;
	ep_t p;
	fp9_t qx, qy, qz;
	fp54_t e1, e2;

	bn_null(k);
	bn_null(n);
	ep_null(p);
	fp9_null(qx);
	fp9_null(qy);
	fp9_null(qz);
	fp54_null(e1);
	fp54_null(e2);

	RLC_TRY {
		bn_new(n);
		bn_new(k);
		ep_new(p);
		fp9_new(qx);
		fp9_new(qy);
		fp9_new(qz);
		fp54_new(e1);
		fp54_new(e2);

		ep_curve_get_ord(n);

		fp_read_str(qx[0][0], QX00, strlen(QX00), 16);
		fp_read_str(qx[0][1], QX01, strlen(QX01), 16);
		fp_read_str(qx[0][2], QX02, strlen(QX02), 16);
		fp_read_str(qx[1][0], QX10, strlen(QX10), 16);
		fp_read_str(qx[1][1], QX11, strlen(QX11), 16);
		fp_read_str(qx[1][2], QX12, strlen(QX12), 16);
		fp_read_str(qx[2][0], QX20, strlen(QX20), 16);
		fp_read_str(qx[2][1], QX21, strlen(QX21), 16);
		fp_read_str(qx[2][2], QX22, strlen(QX22), 16);

		fp_read_str(qy[0][0], QY00, strlen(QY00), 16);
		fp_read_str(qy[0][1], QY01, strlen(QY01), 16);
		fp_read_str(qy[0][2], QY02, strlen(QY02), 16);
		fp_read_str(qy[1][0], QY10, strlen(QY10), 16);
		fp_read_str(qy[1][1], QY11, strlen(QY11), 16);
		fp_read_str(qy[1][2], QY12, strlen(QY12), 16);
		fp_read_str(qy[2][0], QY20, strlen(QY20), 16);
		fp_read_str(qy[2][1], QY21, strlen(QY21), 16);
		fp_read_str(qy[2][2], QY22, strlen(QY22), 16);

		TEST_CASE("pairing non-degeneracy is correct") {
			ep_rand(p);
			pp_map_k54(e1, p, qx, qy);
			TEST_ASSERT(fp54_cmp_dig(e1, 1) != RLC_EQ, end);
		} TEST_END;

		TEST_CASE("pairing is bilinear") {
			ep_rand(p);
			bn_rand_mod(k, n);
			pp_map_k54(e1, p, qx, qy);
			ep_mul(p, p, k);
			pp_map_k54(e2, p, qx, qy);
			fp54_exp(e1, e1, k);
			TEST_ASSERT(fp54_cmp(e1, e2) == RLC_EQ, end);
			fp9_set_dig(qz, 1);
			pp_dbl_k54(e2, qx, qy, qz, p);
			fp9_inv(qz, qz);
			fp9_mul(qx, qx, qz);
			fp9_mul(qy, qy, qz);
			fp9_set_dig(qz, 1);
			pp_map_k54(e2, p, qx, qy);
			fp54_sqr(e1, e1);
			TEST_ASSERT(fp54_cmp(e1, e2) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(k);
	ep_free(p);
	fp9_free(qx);
	fp9_free(qy);
	fp9_free(qz);
	fp54_free(e1);
	fp54_free(e2);
	return code;
}

int main(void) {
	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the PP module", 0);

	if (ep_param_set_any_pairf() == RLC_ERR) {
		RLC_THROW(ERR_NO_CURVE);
		core_clean();
		return 0;
	}

	ep_param_print();

	util_banner("Arithmetic", 1);

	if (ep_param_embed() == 2) {
		if (doubling2() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (addition2() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (pairing2() != RLC_OK) {
			core_clean();
			return 1;
		}
	}

	if (ep_param_embed() == 8) {
		if (doubling8() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (addition8() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (pairing8() != RLC_OK) {
			core_clean();
			return 1;
		}
	}

	if (ep_param_embed() == 24) {
		if (doubling24() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (addition24() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (pairing24() != RLC_OK) {
			core_clean();
			return 1;
		}
	}

	if (ep_param_embed() == 12) {
		if (doubling12() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (addition12() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (pairing12() != RLC_OK) {
			core_clean();
			return 1;
		}
	}

	if (ep_param_embed() == 48) {
		if (doubling48() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (addition48() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (pairing48() != RLC_OK) {
			core_clean();
			return 1;
		}
	}

	if (ep_param_embed() == 54) {
		if (doubling54() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (addition54() != RLC_OK) {
			core_clean();
			return 1;
		}

		if (pairing54() != RLC_OK) {
			core_clean();
			return 1;
		}
	}

	util_banner("All tests have passed.\n", 0);

	core_clean();
	return 0;
}
