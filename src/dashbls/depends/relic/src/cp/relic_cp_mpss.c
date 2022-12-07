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
 * Implementation of the multi-party Pointcheval-Sanders signature protocols.
 *
 * @ingroup cp
 */

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_mpss_gen(bn_t r[2], bn_t s[2], g2_t h, g2_t x[2], g2_t y[2]) {
	bn_t n;
	int result = RLC_OK;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		/* Generate keys for PS and secret share them. */
		g2_rand(h);
		g2_get_ord(n);

		bn_rand_mod(r[0], n);
		bn_rand_mod(r[1], n);
		g2_mul(x[0], h, r[0]);
		g2_mul(x[1], h, r[1]);

		bn_rand_mod(s[0], n);
		bn_rand_mod(s[1], n);
		g2_mul(y[0], h, s[0]);
		g2_mul(y[1], h, s[1]);
	} RLC_CATCH_ANY {
		result = RLC_ERR;
	} RLC_FINALLY {
		bn_free(n);
	}
	return result;
}

int cp_mpss_bct(g2_t x[2], g2_t y[2]) {
	/* Add public values and replicate. */
	g2_add(x[0], x[0], x[1]);
	g2_norm(x[0], x[0]);
	g2_copy(x[1], x[0]);
	g2_add(y[0], y[0], y[1]);
	g2_norm(y[0], y[0]);
	g2_copy(y[1], y[0]);
	return RLC_OK;
}

int cp_mpss_sig(g1_t a, g1_t b[2], bn_t m[2], bn_t r[2], bn_t s[2], mt_t mul_tri[2], mt_t sm_tri[2]) {
	int result = RLC_OK;
	bn_t n, d[2], e[2];

	bn_null(n);

	RLC_TRY {
		bn_new(n);
		for (int i = 0; i < 2; i++) {
			bn_null(d[i]);
			bn_null(e[i]);
			bn_new(d[i]);
			bn_new(e[i]);
		}
		/* Compute d = (xm + y) in MPC. */
		g1_get_ord(n);
		mt_mul_lcl(d[0], e[0], m[0], s[0], n, mul_tri[0]);
		mt_mul_lcl(d[1], e[1], m[1], s[1], n, mul_tri[1]);
		mt_mul_bct(d, e, n);
		mt_mul_mpc(d[0], d[0], e[0], n, mul_tri[0], 0);
		mt_mul_mpc(d[1], d[1], e[1], n, mul_tri[1], 1);
		bn_add(d[0], d[0], r[0]);
		bn_mod(d[0], d[0], n);
		bn_add(d[1], d[1], r[1]);
		bn_mod(d[1], d[1], n);
		/* Compute signature in MPC. */
		g1_rand(b[0]);
		g1_rand(b[1]);
		g1_add(a, b[0], b[1]);
		g1_norm(a, a);
		g1_mul(b[0], a, d[0]);
		g1_mul(b[1], a, d[1]);
	} RLC_CATCH_ANY {
		result = RLC_ERR;
	} RLC_FINALLY {
		bn_free(n);
		for (int i = 0; i < 2; i++) {
			bn_free(d[i]);
			bn_free(e[i]);
		}
	}
	return result;
}

int cp_mpss_ver(gt_t e, g1_t a, g1_t b[2], bn_t m[2], g2_t h, g2_t x, g2_t y,
		mt_t sm_tri[2], pt_t pc_tri[2]) {
	int result = 0;
	bn_t n, d[2], r[2];
	g1_t p[2], q[2];
	g2_t z[2], w[2];
	gt_t alpha[2], beta[2];

	bn_null(n);

	RLC_TRY {
		bn_new(n);
		for (int i = 0; i < 2; i++) {
			bn_null(d[i]);
			bn_null(r[i]);
			g1_null(p[i]);
			g1_null(q[i]);
			g2_null(z[i]);
			g2_null(w[i]);
			gt_null(alpha[i]);
			gt_null(beta[i]);
			bn_new(d[i]);
			bn_new(r[i]);
			g1_new(p[i]);
			g1_new(q[i]);
			g2_new(z[i]);
			g2_new(w[i]);
			gt_new(alpha[i]);
			gt_new(beta[i]);
		}
		g1_get_ord(n);
		/* Compute Z = X + [m] * Y. */
		g2_mul(z[0], y, m[0]);
		g2_mul(z[1], y, m[1]);
		g2_add(z[0], z[0], x);
		g2_norm(z[0], z[0]);

		/* Compute [P] = [sigma_1'] = [r] * [sigma_1] in MPC. */

		bn_rand_mod(r[0], n);
		bn_rand_mod(r[1], n);

		g1_neg(p[0], a);
		g1_copy(p[1], b[0]);
		g2_copy(w[0], z[0]);
		g2_copy(w[1], h);
		pc_map_sim(beta[0], p, w, 2);

		g1_neg(q[0], a);
		g1_copy(q[1], b[1]);
		g2_copy(w[0], z[1]);
		g2_copy(w[1], h);
		pc_map_sim(beta[1], q, w, 2);

		/* Compute [alpha] = e([r] * [sigma_2, H]).  */
		gt_exp_lcl(d[0], alpha[0], r[0], beta[0], sm_tri[0]);
		gt_exp_lcl(d[1], alpha[1], r[1], beta[1], sm_tri[1]);
		/* Broadcast public values. */
		gt_exp_bct(d, alpha);
		gt_exp_mpc(beta[0], d[0], alpha[0], sm_tri[0], 0);
		gt_exp_mpc(beta[1], d[1], alpha[1], sm_tri[1], 1);

		if (g1_is_infty(a)) {
			gt_rand(e);
		} else {
			/* Now combine shares and multiply. */
			gt_mul(e, beta[0], beta[1]);
		}
	} RLC_CATCH_ANY {
		result = RLC_ERR;
	} RLC_FINALLY {
		bn_free(n);
		for (int i = 0; i < 2; i++) {
			bn_free(d[i]);
			bn_free(r[i]);
			g1_free(p[i]);
			g1_free(q[i]);
			g2_free(z[i]);
			g2_free(w[i]);
			gt_free(alpha[i]);
			gt_free(beta[i]);
		}
	}
	return result;
}

int cp_mpsb_gen(bn_t r[2], bn_t s[][2], g2_t h, g2_t x[2], g2_t y[][2], int l) {
	bn_t n;
	int result = RLC_OK;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		/* Generate keys for PS and secret share them. */
		g2_rand(h);
		g2_get_ord(n);

		bn_rand_mod(r[0], n);
		bn_rand_mod(r[1], n);
		g2_mul(x[0], h, r[0]);
		g2_mul(x[1], h, r[1]);

		for (int i = 0; i < l; i++) {
			bn_rand_mod(s[i][0], n);
			bn_rand_mod(s[i][1], n);
			g2_mul(y[i][0], h, s[i][0]);
			g2_mul(y[i][1], h, s[i][1]);
		}
	} RLC_CATCH_ANY {
		result = RLC_ERR;
	} RLC_FINALLY {
		bn_free(n);
	}
	return result;
}

int cp_mpsb_bct(g2_t x[2], g2_t y[][2], int l) {
	/* Add public values and replicate. */
	g2_add(x[0], x[0], x[1]);
	g2_norm(x[0], x[0]);
	g2_copy(x[1], x[0]);
	for (int i = 0; i < l; i++) {
		g2_add(y[i][0], y[i][0], y[i][1]);
		g2_norm(y[i][0], y[i][0]);
		g2_copy(y[i][1], y[i][0]);
	}
	return RLC_OK;
}

int cp_mpsb_sig(g1_t a, g1_t b[2], bn_t m[][2], bn_t r[2], bn_t s[][2],
		mt_t mul_tri[2], mt_t sm_tri[2], int l) {
	int result = RLC_OK;
	bn_t n, d[2], e[2], t[2];

	bn_null(n);

	RLC_TRY {
		bn_new(n);
		for (int i = 0; i < 2; i++) {
			bn_null(d[i]);
			bn_null(e[i]);
			bn_null(t[i]);
			bn_new(d[i]);
			bn_new(e[i]);
			bn_new(t[i]);
		}
		/* Compute d = (y_im_i + x) in MPC. */
		g1_get_ord(n);
		bn_zero(t[0]);
		for (int i = 0; i < l; i++) {
			bn_add(d[0], s[i][0], s[i][1]);
			bn_add(d[1], m[i][0], m[i][1]);
			bn_mul(t[1], d[0], d[1]);
			bn_mod(t[1], t[1], n);
			bn_add(t[0], t[0], t[1]);
			bn_mod(t[0], t[0], n);
		}
		bn_rand_mod(t[1], n);
		bn_sub(t[0], t[0], t[1]);
		if (bn_sign(t[0]) == RLC_NEG) {
			bn_add(t[0], t[0], n);
		}
		bn_add(d[0], t[0], r[0]);
		bn_mod(d[0], d[0], n);
		bn_add(d[1], t[1], r[1]);
		bn_mod(d[1], d[1], n);
		/* Compute signature in MPC. */
		g1_rand(b[0]);
		g1_rand(b[1]);
		g1_add(a, b[0], b[1]);
		g1_norm(a, a);
		g1_mul(b[0], a, d[0]);
		g1_mul(b[1], a, d[1]);
	} RLC_CATCH_ANY {
		result = RLC_ERR;
	} RLC_FINALLY {
		bn_free(n);
		for (int i = 0; i < 2; i++) {
			bn_free(d[i]);
			bn_free(e[i]);
			bn_free(t[i]);
		}
	}
	return result;
}

int cp_mpsb_ver(gt_t e, g1_t a, g1_t b[2], bn_t m[][2], g2_t h, g2_t x,
		g2_t y[][2], bn_t v[][2], mt_t sm_tri[2], pt_t pc_tri[2], int l) {
	int result = 0;
	bn_t n, _t, t[2], d[2], r[2], *_m = RLC_ALLOCA(bn_t, 2 * l);
	g1_t p[2], q[2];
	g2_t w[2], z[2], *_y = RLC_ALLOCA(g2_t, 2 * l);
	gt_t alpha[2], beta[2];

	bn_null(n);
	bn_null(_t);

	RLC_TRY {
		bn_new(n);
		bn_new(_t);
		for (int i = 0; i < 2; i++) {
			bn_null(d[i]);
			bn_null(r[i]);
			bn_null(t[i]);
			g1_null(p[i]);
			g1_null(q[i]);
			g2_null(w[i]);
			gt_null(alpha[i]);
			gt_null(beta[i]);
			bn_new(d[i]);
			bn_new(r[i]);
			bn_new(t[i]);
			g1_new(p[i]);
			g1_new(q[i]);
			g2_new(w[i]);
			g2_new(z[i]);
			gt_new(alpha[i]);
			gt_new(beta[i]);
			for (int j = 0; j < l; j++) {
				g2_null(_y[l*i + j]);
				g2_new(_y[l*i + j]);
				bn_null(_m[l*i + j]);
				bn_new(_m[l*i + j]);
			}
		}
		g1_get_ord(n);
		if (v == NULL) {
			for (int i = 0; i < 2; i++) {
				for (int j = 0; j < l; j++) {
					bn_copy(_m[l*i + j], m[j][i]);
					g2_copy(_y[l*i + j], y[j][i]);
				}
			}
			/* Compute Z = X + [m] * Y. */
			for (int i = 0; i < 2; i++) {
				g2_mul_sim_lot(z[i], &_y[l*i], &_m[l*i], l);
			}
		} else {
			/* Compute Z = X + [m] * [y_i] * G. */
			bn_zero(t[0]);
			for (int i = 0; i < l; i++) {
				bn_add(d[0], v[i][0], v[i][1]);
				bn_add(d[1], m[i][0], m[i][1]);
				bn_mul(t[1], d[0], d[1]);
				bn_mod(t[1], t[1], n);
				bn_add(t[0], t[0], t[1]);
				bn_mod(t[0], t[0], n);
			}
			bn_rand_mod(t[1], n);
			bn_sub(t[0], t[0], t[1]);
			if (bn_sign(t[0]) == RLC_NEG) {
				bn_add(t[0], t[0], n);
			}
			g2_mul(z[0], h, t[0]);
			g2_mul(z[1], h, t[1]);
		}
		g2_add(z[0], z[0], x);
		g2_norm(z[0], z[0]);

		/* Compute [P] = [sigma_1'] = [r] * [sigma_1] in MPC. */
		bn_rand_mod(r[0], n);
		bn_rand_mod(r[1], n);

		g1_neg(p[0], a);
		g1_copy(p[1], b[0]);
		g2_copy(w[0], z[0]);
		g2_copy(w[1], h);
		pc_map_sim(beta[0], p, w, 2);

		g1_neg(q[0], a);
		g1_copy(q[1], b[1]);
		g2_copy(w[0], z[1]);
		g2_copy(w[1], h);
		pc_map_sim(beta[1], q, w, 2);

		/* Compute [alpha] = e([r] * [sigma_2, H]).  */
		gt_exp_lcl(d[0], alpha[0], r[0], beta[0], sm_tri[0]);
		gt_exp_lcl(d[1], alpha[1], r[1], beta[1], sm_tri[1]);
		/* Broadcast public values. */
		gt_exp_bct(d, alpha);
		gt_exp_mpc(beta[0], d[0], alpha[0], sm_tri[0], 0);
		gt_exp_mpc(beta[1], d[1], alpha[1], sm_tri[1], 1);

		if (g1_is_infty(a)) {
			gt_rand(e);
		} else {
			/* Now combine shares and multiply. */
			gt_mul(e, beta[0], beta[1]);
		}
	} RLC_CATCH_ANY {
		result = RLC_ERR;
	} RLC_FINALLY {
		bn_free(n);
		bn_free(_t);
		for (int i = 0; i < 2; i++) {
			bn_free(d[i]);
			bn_free(r[i]);
			bn_free(t[i]);
			g1_free(p[i]);
			g1_free(q[i]);
			g2_free(w[i]);
			g2_free(z[i]);
			gt_free(alpha[i]);
			gt_free(beta[i]);
			for (int j = 0; j < l; j++) {
				g2_free(_y[l*i + j]);
				bn_free(_m[l*i + j]);
			}
		}
		RLC_FREE(_y);
		RLC_FREE(_m);
	}
	return result;
}
