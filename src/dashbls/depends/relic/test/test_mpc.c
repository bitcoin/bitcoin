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
 * Tests for the error-management routines.
 *
 * @ingroup test
 */

#include <stdio.h>

#include "relic.h"
#include "relic_err.h"
#include "relic_test.h"

static int triple(void) {
	int code = RLC_ERR;
	bn_t n, t, u;
	bn_t d[2], e[2], x[2], y[2];
	mt_t tri[2];

	bn_null(n);
	bn_null(t);
	bn_null(u);
	mt_null(tri[0]);
	mt_null(tri[1]);

	RLC_TRY {
		bn_new(n);
		bn_new(t);
		bn_new(u);
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

		bn_gen_prime(n, RLC_BN_BITS);

		TEST_CASE("multiplication triples are generated correctly") {
			mt_gen(tri, n);
			bn_add(t, tri[0]->a, tri[1]->a);
			bn_mod(t, t, n);
			bn_add(u, tri[0]->b, tri[1]->b);
			bn_mod(u, u, n);
			bn_mul(t, t, u);
			bn_mod(t, t, n);
			bn_add(u, tri[0]->c, tri[1]->c);
			bn_mod(u, u, n);
			TEST_ASSERT(bn_cmp(t, u) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("multiplication triples are consistent") {
			mt_gen(tri, n);
			/* Generate random inputs. */
			bn_rand_mod(x[0], n);
			bn_rand_mod(y[0], n);
			bn_mul(t, x[0], y[0]);
			bn_mod(t, t, n);
			/* Secret share inputs. */
			bn_rand_mod(x[1], n);
			bn_sub(x[0], x[0], x[1]);
			if (bn_sign(x[0]) == RLC_NEG) {
				bn_add(x[0], x[0], n);
			}
			bn_mod(x[0], x[0], n);
			bn_rand_mod(y[1], n);
			bn_sub(y[0], y[0], y[1]);
			if (bn_sign(y[0]) == RLC_NEG) {
				bn_add(y[0], y[0], n);
			}
			bn_mod(y[0], y[0], n);

			/* Compute public values locally. */
			mt_mul_lcl(d[0], e[0], x[0], y[0], n, tri[0]);
			mt_mul_lcl(d[1], e[1], x[1], y[1], n, tri[1]);
			/* Broadcast public values. */
			mt_mul_bct(d, e, n);
			TEST_ASSERT(bn_cmp(d[0], d[1]) == RLC_EQ, end);
			TEST_ASSERT(bn_cmp(e[0], e[1]) == RLC_EQ, end);
			mt_mul_mpc(d[0], d[0], e[0], n, tri[0], 0);
			mt_mul_mpc(d[1], d[1], e[1], n, tri[1], 1);
			bn_add(d[0], d[0], d[1]);
			bn_mod(d[0], d[0], n);
			TEST_ASSERT(bn_cmp(t, d[0]) == RLC_EQ, end);
		} TEST_END;
	} RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}

	code = RLC_OK;
  end:
	bn_free(n);
	bn_free(t);
	bn_free(u);
	mt_free(tri[0]);
	mt_free(tri[1]);
	for (int j = 0; j < 2; j++) {
	  bn_free(d[j]);
	  bn_free(e[j]);
	  bn_free(x[j]);
	  bn_free(y[j]);
	}
	return code;
}

static int pairing(void) {
	int j, code = RLC_ERR;
	g1_t b1[2], c1[2], d[2], p[2], _p;
	g2_t b2[2], c2[2], e[2], q[2], _q;
	gt_t bt[2], ct[2], f[2], r[2], _r;
	bn_t k[2], l[2], n;
	mt_t tri[2];
	pt_t t[2];

	g1_null(_p);
	g2_null(_q);
	gt_null(_r);
	bn_null(n);

	RLC_TRY {
		g1_new(_p);
		g2_new(_q);
		gt_new(_r);
		bn_new(n);

		for (j = 0; j < 2; j++) {
			g1_null(b1[j]);
			g2_null(b2[j]);
			gt_null(bt[j]);
			g1_null(c1[j]);
			g2_null(c2[j]);
			gt_null(ct[j]);
			g1_null(d[j]);
			g2_null(e[j]);
			bn_null(k[j]);
			bn_null(l[j]);
			g1_null(p[j]);
			g2_null(q[j]);
			gt_null(r[j]);
			gt_null(f[j]);
			mt_null(tri[j]);
			pt_null(t[j]);
			g1_new(b1[j]);
			g2_new(b2[j]);
			gt_new(bt[j]);
			g1_new(c1[j]);
			g2_new(c2[j]);
			gt_new(ct[j]);
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

		TEST_CASE("scalar multiplication triples in g1 are consistent") {
			mt_gen(tri, n);
			/* Generate random inputs. */
			g1_rand(p[0]);
			bn_rand_mod(k[0], n);
			g1_mul(_p, p[0], k[0]);
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

			g1_mul_gen(b1[0], tri[0]->b);
			g1_mul_gen(b1[1], tri[1]->b);
			g1_mul_gen(c1[0], tri[0]->c);
			g1_mul_gen(c1[1], tri[1]->c);
			tri[0]->b1 = &b1[0];
			tri[1]->b1 = &b1[1];
			tri[0]->c1 = &c1[0];
			tri[1]->c1 = &c1[1];

			/* Compute public values locally. */
			g1_mul_lcl(l[0], d[0], k[0], p[0], tri[0]);
			g1_mul_lcl(l[1], d[1], k[1], p[1], tri[1]);
			/* Broadcast public values. */
			g1_mul_bct(l, d);
			TEST_ASSERT(bn_cmp(l[0], l[1]) == RLC_EQ, end);
			TEST_ASSERT(g1_cmp(d[0], d[1]) == RLC_EQ, end);
			g1_mul_mpc(d[0], l[0], d[0], tri[0], 0);
			g1_mul_mpc(d[1], l[1], d[1], tri[1], 1);
			g1_add(d[0], d[0], d[1]);
			g1_norm(d[0], d[0]);
			TEST_ASSERT(g1_cmp(_p, d[0]) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("scalar multiplication triples in g2 are consistent") {
			/* Generate random inputs. */
			g2_rand(q[0]);
			bn_rand_mod(k[0], n);
			g2_mul(_q, q[0], k[0]);
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

			g2_mul_gen(b2[0], tri[0]->b);
			g2_mul_gen(b2[1], tri[1]->b);
			g2_mul_gen(c2[0], tri[0]->c);
			g2_mul_gen(c2[1], tri[1]->c);
			tri[0]->b2 = &b2[0];
			tri[1]->b2 = &b2[1];
			tri[0]->c2 = &c2[0];
			tri[1]->c2 = &c2[1];

			/* Compute public values locally. */
			g2_mul_lcl(l[0], e[0], k[0], q[0], tri[0]);
			g2_mul_lcl(l[1], e[1], k[1], q[1], tri[1]);
			/* Broadcast public values. */
			g2_mul_bct(l, e);
			TEST_ASSERT(bn_cmp(l[0], l[1]) == RLC_EQ, end);
			TEST_ASSERT(g2_cmp(e[0], e[1]) == RLC_EQ, end);
			g2_mul_gen(c2[0], tri[0]->c);
			g2_mul_gen(c2[1], tri[1]->c);
			g2_mul_mpc(e[0], l[0], e[0], tri[0], 0);
			g2_mul_mpc(e[1], l[1], e[1], tri[1], 1);
			g2_add(e[0], e[0], e[1]);
			g2_norm(e[0], e[0]);
			TEST_ASSERT(g2_cmp(_q, e[0]) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("exponentiation triples in target group are consistent") {
			/* Generate random inputs. */
			gt_rand(r[0]);
			bn_rand_mod(k[0], n);
			gt_exp(_r, r[0], k[0]);
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

			gt_exp_gen(bt[0], tri[0]->b);
			gt_exp_gen(bt[1], tri[1]->b);
			gt_exp_gen(ct[0], tri[0]->c);
			gt_exp_gen(ct[1], tri[1]->c);
			tri[0]->bt = &bt[0];
			tri[1]->bt = &bt[1];
			tri[0]->ct = &ct[0];
			tri[1]->ct = &ct[1];

			/* Compute public values locally. */
			gt_exp_lcl(l[0], f[0], k[0], r[0], tri[0]);
			gt_exp_lcl(l[1], f[1], k[1], r[1], tri[1]);
			/* Broadcast public values. */
			gt_exp_bct(l, f);
			TEST_ASSERT(bn_cmp(l[0], l[1]) == RLC_EQ, end);
			TEST_ASSERT(gt_cmp(f[0], f[1]) == RLC_EQ, end);
			gt_exp_mpc(f[0], l[0], f[0], tri[0], 0);
			gt_exp_mpc(f[1], l[1], f[1], tri[1], 1);
			gt_mul(f[0], f[0], f[1]);
			TEST_ASSERT(gt_cmp(_r, f[0]) == RLC_EQ, end);
		} TEST_END;

		TEST_CASE("pairing triples are consistent") {
			pc_map_tri(t);
			g1_add(t[0]->a, t[0]->a, t[1]->a);
			g1_norm(t[0]->a, t[0]->a);
			g2_add(t[0]->b, t[0]->b, t[1]->b);
			g2_norm(t[0]->b, t[0]->b);
			gt_mul(t[1]->c, t[0]->c, t[1]->c);
			pc_map(t[0]->c, t[0]->a, t[0]->b);
			TEST_ASSERT(gt_cmp(t[0]->c, t[1]->c) == RLC_EQ, end);
			/* Regenerate triple. */
			pc_map_tri(t);
			/* Generate random inputs. */
			g1_rand(p[0]);
			g2_rand(q[0]);
			pc_map(f[0], p[0], q[0]);
			/* Secret share inputs. */
			g1_rand(p[1]);
			g1_sub(p[0], p[0], p[1]);
			g1_norm(p[0], p[0]);
			g2_rand(q[1]);
			g2_sub(q[0], q[0], q[1]);
			g2_norm(q[0], q[0]);
			/* Compute public values locally. */
			pc_map_lcl(d[0], e[0], p[0], q[0], t[0]);
			pc_map_lcl(d[1], e[1], p[1], q[1], t[1]);
			/* Broadcast public values. */
			pc_map_bct(d, e);
			TEST_ASSERT(g1_cmp(d[0], d[1]) == RLC_EQ, end);
			TEST_ASSERT(g2_cmp(e[0], e[1]) == RLC_EQ, end);
			pc_map_mpc(r[0], d[0], e[0], t[0], 0);
			pc_map_mpc(r[1], d[1], e[1], t[1], 1);
			gt_mul(f[1], r[0], r[1]);
			TEST_ASSERT(gt_cmp(f[0], f[1]) == RLC_EQ, end);
		} TEST_END;
	}
	RLC_CATCH_ANY {
		util_print("FATAL ERROR!\n");
		RLC_ERROR(end);
	}
	code = RLC_OK;
  end:
	g1_free(_p);
	g2_free(_q);
	gt_free(_r);
	bn_free(n);
	for (j = 0; j < 2; j++) {
		g1_free(b1[j]);
		g2_free(b2[j]);
		gt_free(bt[j]);
		g1_free(c1[j]);
		g2_free(c2[j]);
		gt_free(ct[j]);
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
	return code;
}

int main(void) {

	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	util_banner("Tests for the MPC module:\n", 0);
	util_banner("Utilities:", 1);

#if defined(WITH_BN)
	if (triple()) {
		core_clean();
		return 1;
	}
#endif

#if defined(WITH_PC)
	if (pc_param_set_any() != RLC_OK) {
		RLC_THROW(ERR_NO_CURVE);
		core_clean();
		return 0;
	}

	pc_param_print();

	util_banner("Arithmetic:", 1);

	if (pairing()) {
		core_clean();
		return 1;
	}
#endif

	util_banner("All tests have passed.\n", 0);

	core_clean();
	return 0;
}
