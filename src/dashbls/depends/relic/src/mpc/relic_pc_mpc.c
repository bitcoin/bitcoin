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
 * Implementation of pairing triples for MPC applications.
 *
 * @ingroup pp
 */

#include "relic_core.h"
#include "relic_mpc.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void g1_mul_lcl(bn_t d, g1_t q, bn_t x, g1_t p, mt_t tri) {
	bn_t n;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		/* Compute public values for transmission. */

		/* [d] = [x] - [a]. */
		g1_get_ord(n);
		bn_sub(d, x, tri->a);
		if (bn_sign(d) == RLC_NEG) {
			bn_add(d, d, n);
		}
		bn_mod(d, d, n);

		/* [Q] = [P] - [B] = [b]G. */
		/* Copy first to avoid overwriting P. */
		g1_copy(q, p);
		g1_sub(q, q, *tri->b1);
		g1_norm(q, q);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(n);
	}
}

void g1_mul_bct(bn_t d[2], g1_t q[2]) {
	bn_t n;

	bn_null(n);

	RLC_TRY {
		bn_new(n);
		/* Open values d and Q. */
		g1_get_ord(n);
		bn_add(d[0], d[0], d[1]);
		bn_mod(d[0], d[0], n);
		bn_copy(d[1], d[0]);
		g1_add(q[0], q[0], q[1]);
		g1_norm(q[0], q[0]);
		g1_copy(q[1], q[0]);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(n);
	}
}

void g1_mul_mpc(g1_t r, bn_t d, g1_t q, mt_t tri, int party) {
	g1_t t;

	g1_null(t);

	RLC_TRY {
		g1_new(t);

		if (party == 0) {
			/* For one party, compute [B] + Q. */
			g1_add(t, *tri->b1, q);
			g1_norm(t, t);
		} else {
			g1_copy(t, *tri->b1);
		}
		/* Compute [a]Q + d([B] + Q) or d[B]. */
		g1_mul_sim(r, q, tri->a, t, d);
		/* R = [a]Q + d[B] + dQ + [C] = [xy] */
		g1_add(r, r, *tri->c1);
		g1_norm(r, r);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		g1_free(t);
	}
}

void g2_mul_lcl(bn_t d, g2_t q, bn_t x, g2_t p, mt_t tri) {
	bn_t n;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		/* Compute public values for transmission. */

		/* [d] = [x] - [a]. */
		g2_get_ord(n);
		bn_sub(d, x, tri->a);
		if (bn_sign(d) == RLC_NEG) {
			bn_add(d, d, n);
		}
		bn_mod(d, d, n);

		/* [Q] = [P] - [B] = [b]G. */
		/* Copy first to avoid overwriting P. */
		g2_copy(q, p);
		g2_sub(q, q, *tri->b2);
		g2_norm(q, q);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(n);
	}
}

void g2_mul_bct(bn_t d[2], g2_t q[2]) {
	bn_t n;

	bn_null(n);

	RLC_TRY {
		bn_new(n);
		/* Open values d and Q. */
		g2_get_ord(n);
		bn_add(d[0], d[0], d[1]);
		bn_mod(d[0], d[0], n);
		bn_copy(d[1], d[0]);
		g2_add(q[0], q[0], q[1]);
		g2_norm(q[0], q[0]);
		g2_copy(q[1], q[0]);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(n);
	}
}

void g2_mul_mpc(g2_t r, bn_t d, g2_t q, mt_t tri, int party) {
	g2_t t;

	g2_null(t);

	RLC_TRY {
		g2_new(t);

		if (party == 0) {
			/* For one party, compute [B] + Q. */
			g2_add(t, *tri->b2, q);
			g2_norm(t, t);
		} else {
			g2_copy(t, *tri->b2);
		}
		/* Compute [a]Q + d([B] + Q) or d[B]. */
		g2_mul_sim(r, q, tri->a, t, d);
		/* R = [a]Q + d[B] + dQ + [C] = [xy] */
		g2_add(r, r, *tri->c2);
		g2_norm(r, r);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		g2_free(t);
	}
}

void gt_exp_lcl(bn_t d, gt_t q, bn_t x, gt_t p, mt_t tri) {
	bn_t n;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		/* Compute public values for transmission. */

		/* [d] = [x] - [a]. */
		gt_get_ord(n);
		bn_sub(d, x, tri->a);
		if (bn_sign(d) == RLC_NEG) {
			bn_add(d, d, n);
		}
		bn_mod(d, d, n);

		/* [Q] = [P] - [B] = [b]G. */
		/* Copy first to avoid overwriting P. */
		gt_copy(q, p);
		gt_inv(*tri->bt, *tri->bt);
		gt_mul(q, q, *tri->bt);
		gt_inv(*tri->bt, *tri->bt);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(n);
	}
}

void gt_exp_bct(bn_t d[2], gt_t q[2]) {
	bn_t n;

	bn_null(n);

	RLC_TRY {
		bn_new(n);
		/* Open values d and Q. */
		gt_get_ord(n);
		bn_add(d[0], d[0], d[1]);
		bn_mod(d[0], d[0], n);
		bn_copy(d[1], d[0]);
		gt_mul(q[0], q[0], q[1]);
		gt_copy(q[1], q[0]);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(n);
	}
}

void gt_exp_mpc(gt_t r, bn_t d, gt_t q, mt_t tri, int party) {
	gt_t t;

	gt_null(t);

	RLC_TRY {
		gt_new(t);

		if (party == 0) {
			/* For one party, compute [B] + Q. */
			gt_mul(t, *tri->bt, q);
		} else {
			gt_copy(t, *tri->bt);
		}
		/* Compute [a]Q + d([B] + Q) or d[B]. */
		gt_exp_sim(r, q, tri->a, t, d);
		/* R = [a]Q + d[B] + dQ + [C] = [xy] */
		gt_mul(r, r, *tri->ct);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		gt_free(t);
	}
}

void pc_map_tri(pt_t t[2]) {
	bn_t n;
	mt_t tri[2];

	bn_null(n);
	mt_null(tri[0]);
	mt_null(tri[1]);

	RLC_TRY {
		bn_new(n);
		mt_new(tri[0]);
		mt_new(tri[1]);

		g1_get_ord(n);
		mt_gen(tri, n);

		for (int i = 0; i < 2; i++) {
			g1_mul_gen(t[i]->a, tri[i]->a);
			g2_mul_gen(t[i]->b, tri[i]->b);
			gt_exp_gen(t[i]->c, tri[i]->c);
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(n);
		mt_free(tri[0]);
		mt_free(tri[1]);
	}
}

void pc_map_lcl(g1_t d, g2_t e, g1_t p, g2_t q, pt_t t) {
	/* Compute public values for transmission. */
	g1_sub(d, p, t->a);
	g1_norm(d, d);
	g2_sub(e, q, t->b);
	g2_norm(e, e);
}

void pc_map_bct(g1_t d[2], g2_t e[2]) {
	/* Add public values and replicate. */
	g1_add(d[0], d[0], d[1]);
	g1_norm(d[0], d[0]);
	g1_copy(d[1], d[0]);
	g2_add(e[0], e[0], e[1]);
	g2_norm(e[0], e[0]);
	g2_copy(e[1], e[0]);
}

void pc_map_mpc(gt_t r, g1_t d1, g2_t d2, pt_t triple, int party) {
	gt_t t;
	g1_t _p[2];
	g2_t _q[2];

	gt_null(t);

	RLC_TRY {
		gt_new(t);
		for (int i = 0; i < 2; i++) {
			g1_null(_p[i]);
			g2_null(_q[i]);
			g1_new(_p[i]);
			g2_new(_q[i]);
		}

		/* Compute the pairing in MPC. */
		if (party == 0) {
			g1_add(_p[0], triple->a, d1);
			g1_norm(_p[0], _p[0]);
			g2_copy(_q[0], d2);
		} else {
			g1_copy(_p[0], triple->a);
			g2_copy(_q[0], d2);
		}
		g1_copy(_p[1], d1);
		g2_copy(_q[1], triple->b);
		pc_map_sim(t, _p, _q, 2);
		gt_mul(r, triple->c, t);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		gt_free(t);
		for (int i = 0; i < 2; i++) {
			g1_free(_p[i]);
			g2_free(_q[i]);
			g1_free(_p[i]);
			g2_free(_q[i]);
		}
	}
}
