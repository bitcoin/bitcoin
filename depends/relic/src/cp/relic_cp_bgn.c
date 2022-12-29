/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2015 RELIC Authors
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
 * Implementation of Freeman's prime-order version of the Boneh-Goh-Nissim
 * cryptosystem.
 *
 * @version $Id$
 * @ingroup cp
 */

#include <limits.h>

#include "relic.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_bgn_gen(bgn_t pub, bgn_t prv) {
	bn_t n;
	int result = RLC_OK;

	bn_null(n);

	RLC_TRY {
		bn_new(n);

		pc_get_ord(n);

		bn_rand_mod(prv->x, n);
		bn_rand_mod(prv->y, n);
		bn_rand_mod(prv->z, n);

		g1_mul_gen(pub->gx, prv->x);
		g1_mul_gen(pub->gy, prv->y);
		g1_mul_gen(pub->gz, prv->z);

		g2_mul_gen(pub->hx, prv->x);
		g2_mul_gen(pub->hy, prv->y);
		g2_mul_gen(pub->hz, prv->z);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
	}

	return result;
}

int cp_bgn_enc1(g1_t out[2], dig_t in, bgn_t pub) {
	bn_t r, n;
	g1_t t;
	int result = RLC_OK;

	bn_null(n);
	bn_null(r);
	g1_null(t);

	RLC_TRY {
		bn_new(n);
		bn_new(r);
		g1_new(t);

		pc_get_ord(n);
		bn_rand_mod(r, n);

		/* Compute c0 = (ym + r)G. */
		g1_mul_dig(out[0], pub->gy, in);

		g1_mul_gen(t, r);
		g1_add(out[0], out[0], t);
		g1_norm(out[0], out[0]);

		/* Compute c1 = (zm + xr)G. */
		g1_mul_dig(out[1], pub->gz, in);
		g1_mul(t, pub->gx, r);
		g1_add(out[1], out[1], t);
		g1_norm(out[1], out[1]);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(r);
		g1_free(t);
	}

	return result;
}

int cp_bgn_dec1(dig_t *out, g1_t in[2], bgn_t prv) {
	bn_t r, n;
	g1_t s, t, u;
	int i, result = RLC_ERR;

	bn_null(n);
	bn_null(r);
	g1_null(s);
	g1_null(t);
	g1_null(u);

	RLC_TRY {
		bn_new(n);
		bn_new(r);
		g1_new(s);
		g1_new(t);
		g1_new(u);

		pc_get_ord(n);
		/* Compute T = x(ym + r)G - (zm + xr)G = m(xy - z)G. */
		g1_mul(t, in[0], prv->x);
		g1_sub(t, t, in[1]);
		g1_norm(t, t);
		/* Compute U = (xy - z)G and find m. */
		bn_mul(r, prv->x, prv->y);
		bn_sub(r, r, prv->z);
		bn_mod(r, r, n);
		g1_mul_gen(s, r);
		g1_copy(u, s);

		if (g1_is_infty(t) == 1){
			*out = 0;
			result = RLC_OK;
		} else {
			for (i = 0; i < INT_MAX; i++) {
				if (g1_cmp(t, u) == RLC_EQ) {
					*out = i + 1;
					result = RLC_OK;
					break;
				}
				g1_add(u, u, s);
				g1_norm(u, u);
			}
		}
	} RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(r);
		g1_free(s);
		g1_free(t);
		g1_free(u);
	}

	return result;
}

int cp_bgn_enc2(g2_t out[2], dig_t in, bgn_t pub) {
	bn_t r, n;
	g2_t t;
	int result = RLC_OK;

	bn_null(n);
	bn_null(r);
	g2_null(t);

	RLC_TRY {
		bn_new(n);
		bn_new(r);
		g2_new(t);

		pc_get_ord(n);
		bn_rand_mod(r, n);

		/* Compute c0 = (ym + r)G. */
		g2_mul_dig(out[0], pub->hy, in);
		g2_mul_gen(t, r);
		g2_add(out[0], out[0], t);
		g2_norm(out[0], out[0]);

		/* Compute c1 = (zm + xr)G. */
		g2_mul_dig(out[1], pub->hz, in);
		g2_mul(t, pub->hx, r);
		g2_add(out[1], out[1], t);
		g2_norm(out[1], out[1]);
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(r);
		g2_free(t);
	}

	return result;
}

int cp_bgn_dec2(dig_t *out, g2_t in[2], bgn_t prv) {
	bn_t r, n;
	g2_t s, t, u;
	int i, result = RLC_ERR;

	bn_null(n);
	bn_null(r);
	g2_null(s);
	g2_null(t);
	g2_null(u);

	RLC_TRY {
		bn_new(n);
		bn_new(r);
		g2_new(s);
		g2_new(t);
		g2_new(u);

		pc_get_ord(n);
		/* Compute T = x(ym + r)G - (zm + xr)G = m(xy - z)G. */
		g2_mul(t, in[0], prv->x);
		g2_sub(t, t, in[1]);
		g2_norm(t, t);
		/* Compute U = (xy - z)G and find m. */
		bn_mul(r, prv->x, prv->y);
		bn_sub(r, r, prv->z);
		bn_mod(r, r, n);
		g2_mul_gen(s, r);
		g2_copy(u, s);

		if (g2_is_infty(t) == 1) {
			*out = 0;
			result = RLC_OK;
		} else {
			for (i = 0; i < INT_MAX; i++) {
				if (g2_cmp(t, u) == RLC_EQ) {
					*out = i + 1;
					result = RLC_OK;
					break;
				}
				g2_add(u, u, s);
				g2_norm(u, u);
			}
		}
	} RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(n);
		bn_free(r);
		g2_free(s);
		g2_free(t);
		g2_free(u);
	}

	return result;
}

int cp_bgn_add(gt_t e[4], gt_t c[4], gt_t d[4]) {
	for (int i = 0; i < 4; i++) {
		gt_mul(e[i], c[i], d[i]);
	}
	return RLC_OK;
}

int cp_bgn_mul(gt_t e[4], g1_t c[2], g2_t d[2]) {
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			pc_map(e[2*i + j], c[i], d[j]);
		}
	}
	return RLC_OK;
}

int cp_bgn_dec(dig_t *out, gt_t in[4], bgn_t prv) {
	int i, result = RLC_ERR;
	g1_t g;
	g2_t h;
	gt_t t[4];
	bn_t n, r, s;

	bn_null(n);
	bn_null(r);
	bn_null(s);
	g1_null(g);
	g2_null(h);

	RLC_TRY {
		bn_new(n);
		bn_new(r);
		bn_new(s);
		g1_new(g);
		g2_new(h);
		for (i = 0; i < 4; i++) {
			gt_null(t[i]);
			gt_new(t[i]);
		}

		gt_exp(t[0], in[0], prv->x);
		gt_exp(t[0], t[0], prv->x);

		gt_mul(t[1], in[1], in[2]);
		gt_exp(t[1], t[1], prv->x);
		gt_inv(t[1], t[1]);

		gt_mul(t[3], in[3], t[1]);
		gt_mul(t[3], t[3], t[0]);

		pc_get_ord(n);
		g1_get_gen(g);
		g2_get_gen(h);

		bn_mul(r, prv->x, prv->y);
		bn_sqr(r, r);

		bn_mul(s, prv->x, prv->y);
		bn_mul(s, s, prv->z);
		bn_sub(r, r, s);
		bn_sub(r, r, s);

		bn_sqr(s, prv->z);
		bn_add(r, r, s);
		bn_mod(r, r, n);
		pc_map(t[1], g, h);
		gt_exp(t[1], t[1], r);

		gt_copy(t[2], t[1]);

		if (gt_is_unity(t[3]) == 1) {
			*out = 0;
			result = RLC_OK;
		} else {
			for (i = 0; i < INT_MAX; i++) {
				if (gt_cmp(t[2], t[3]) == RLC_EQ) {
					*out = i + 1;
					result = RLC_OK;
					break;
				}
				gt_mul(t[2], t[2], t[1]);
			}
		}
	} RLC_CATCH_ANY {
		result = RLC_ERR;
	} RLC_FINALLY {
		bn_free(n);
		bn_free(r);
		bn_free(s);
		g1_free(g);
		g2_free(h);
		for (i = 0; i < 4; i++) {
			gt_free(t[i]);
		}
	}

	return result;
}
