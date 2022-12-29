/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2019 RELIC Authors
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
 * Implementation of the Pointcheval-Sanders signature protocols.
 *
 * @ingroup cp
 */

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_pss_gen(bn_t r, bn_t s, g2_t g, g2_t x, g2_t y) {
	bn_t n;
	int result = RLC_OK;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		pc_get_ord(n);
		bn_rand_mod(r, n);
		bn_rand_mod(s, n);
		g2_rand(g);
		g2_mul(x, g, r);
		g2_mul(y, g, s);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
	}
	return result;
}

int cp_pss_sig(g1_t a, g1_t b, bn_t m, bn_t r, bn_t s) {
	bn_t n, t;
	int result = RLC_OK;

	bn_null(n);
	bn_null(t);

	RLC_TRY {
		bn_new(n);
		bn_new(t);

		g1_get_ord(n);
		bn_mul(t, m, s);
		bn_mod(t, t, n);
		bn_add(t, t, r);
		bn_mod(t, t, n);
		g1_rand(a);
		g1_mul(b, a, t);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	} RLC_FINALLY {
		bn_free(n);
		bn_free(t);
	}
	return result;
}

int cp_pss_ver(g1_t a, g1_t b, bn_t m, g2_t g, g2_t x, g2_t y) {
	g1_t p[2];
	g2_t r[2];
	gt_t e;
	int result = 0;

	g1_null(p[0]);
	g1_null(p[1]);
	g2_null(r[0]);
	g2_null(r[1]);
	gt_null(e);

	RLC_TRY {
		g1_new(p[0]);
		g1_new(p[1]);
		g2_new(r[0]);
		g2_new(r[1]);
		gt_new(e);

		g1_copy(p[0], a);
		g1_copy(p[1], b);
		g2_copy(r[1], g);
		g2_neg(r[1], r[1]);

		g2_mul(r[0], y, m);
		g2_add(r[0], r[0], x);
		g2_norm(r[0], r[0]);

		pc_map_sim(e, p, r, 2);
		if (gt_is_unity(e) && !g1_is_infty(a)) {
			result = 1;
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		g1_free(p[0]);
		g1_free(p[1]);
		g2_free(r[0]);
		g2_free(r[1]);
		gt_free(e);
	}
	return result;
}

int cp_psb_gen(bn_t r, bn_t s[], g2_t g, g2_t x, g2_t y[], int l) {
	bn_t n;
	int i, result = RLC_OK;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		pc_get_ord(n);
		bn_rand_mod(r, n);
		g2_rand(g);
		g2_mul(x, g, r);
		for (i = 0; i < l; i++) {
			bn_rand_mod(s[i], n);
			g2_mul(y[i], g, s[i]);
		}
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
	}
	return result;
}

int cp_psb_sig(g1_t a, g1_t b, bn_t ms[], bn_t r, bn_t s[], int l) {
	bn_t m, n, t;
	int i, result = RLC_OK;

	bn_null(m);
	bn_null(n);
	bn_null(t);

	RLC_TRY {
		bn_new(m);
		bn_new(n);
		bn_new(t);

		/* Choose random a in G1. */
		g1_rand(a);
		/* Compute b = a^x+\sum y_im_i. */
		pc_get_ord(n);

		bn_copy(t, r);
		for (i = 0; i < l; i++) {
			bn_mod(m, ms[i], n);
			bn_mul(m, m, s[i]);
			bn_mod(m, m, n);
			bn_add(t, t, m);
			bn_mod(t, t, n);
		}
		g1_mul(b, a, t);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(m);
		bn_free(n);
		bn_free(t);
	}
	return result;
}

int cp_psb_ver(g1_t a, g1_t b, bn_t ms[], g2_t g, g2_t x, g2_t y[], int l) {
	g1_t p[2];
	g2_t q[2];
	gt_t e;
	bn_t m, n;
	int result = 0;

	g1_null(p[0]);
	g1_null(p[1]);
	g2_null(q[0]);
	g2_null(q[1]);
	gt_null(e);
	bn_null(m);
	bn_null(n);

	RLC_TRY {
		g1_new(p[0]);
		g1_new(p[1]);
		g2_new(q[0]);
		g2_new(q[1]);
		gt_new(e);
		bn_new(m);
		bn_new(n);

		/* Check that e(a, x \prod y_i^m_i) = e(b, g). */
		g1_copy(p[0], a);
		g1_copy(p[1], b);

		pc_get_ord(n);
		g2_mul_sim_lot(q[0], y, ms, l);
		g2_add(q[0], q[0], x);
		g2_norm(q[0], q[0]);
		g2_copy(q[1], g);
		g2_neg(q[1], q[1]);
		pc_map_sim(e, p, q, 2);
		if (!g1_is_infty(a) && gt_is_unity(e)) {
			result = 1;
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		g1_free(p[0]);
		g1_free(p[1]);
		g2_free(q[0]);
		g2_free(q[1]);
		gt_free(e);
		bn_free(m);
		bn_free(n);
	}
	return result;
}
