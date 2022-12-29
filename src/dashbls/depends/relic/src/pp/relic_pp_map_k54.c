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
 * Implementation of pairing computation for curves with embedding degree 54.
 *
 * @ingroup pp
 */

#include "relic_core.h"
#include "relic_pp.h"
#include "relic_util.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

static void pp_mil_k54(fp54_t r, fp9_t qx, fp9_t qy, ep_t p, bn_t a) {
	fp54_t l;
	ep_t _p;
	fp9_t rx, ry, rz, sx, sy, sz;
	int i, len = bn_bits(a) + 1;
	int8_t s[RLC_FP_BITS + 1];

	fp54_null(l);
	ep_null(_p);
	fp9_null(rx);
	fp9_null(ry);
	fp9_null(rz);
	fp9_null(sx);
	fp9_null(sy);
	fp9_null(sz);

	RLC_TRY {
		fp54_new(l);
		ep_new(_p);
		fp9_new(rx);
		fp9_new(ry);
		fp9_new(rz);
		fp9_new(sx);
		fp9_new(sy);
		fp9_new(sz);

		fp54_zero(l);
		fp9_copy(rx, qx);
		fp9_copy(ry, qy);
		fp9_set_dig(rz, 1);
#if EP_ADD == BASIC
		ep_neg(_p, p);
#else
		fp_add(_p->x, p->x, p->x);
		fp_add(_p->x, _p->x, p->x);
		fp_neg(_p->y, p->y);
#endif

		bn_rec_naf(s, &len, a, 2);
		for (i = len - 2; i >= 0; i--) {
			fp54_sqr(r, r);
			pp_dbl_k54(l, rx, ry, rz, _p);
			fp54_mul_dxs(r, r, l);
			if (s[i] > 0) {
				pp_add_k54(l, rx, ry, rz, qx, qy, p);
				fp54_mul_dxs(r, r, l);
			}
			if (s[i] < 0) {
				fp9_neg(qy, qy);
				pp_add_k54(l, rx, ry, rz, qx, qy, p);
				fp9_neg(qy, qy);
				fp54_mul_dxs(r, r, l);
			}
		}
		/* Compute f^3. */
		fp54_sqr(l, r);
		fp54_mul(r, r, l);

		fp54_zero(l);
		fp9_copy(sx, rx);
		fp9_copy(sy, ry);
		fp9_copy(sz, rz);
		pp_dbl_k54(l, sx, sy, sz, _p);
		fp54_mul_dxs(r, r, l);
#if EP_ADD == PROJC
		fp9_inv(sz, sz);
		fp9_mul(sx, sx, sz);
		fp9_mul(sy, sy, sz);
#endif
		pp_add_k54(l, rx, ry, rz, sx, sy, p);
		fp54_mul_dxs(r, r, l);
		fp9_frb(rx, qx, 1);
		fp9_frb(ry, qy, 1);
		fp9_zero(sz);
		fp3_set_dig(sz[1], 1);
		fp9_inv(sz, sz);
		fp_copy(sz[0][0], sz[2][2]);
		fp_mul(sz[0][0], sz[0][0], core_get()->fp3_p0[1]);
		fp_mul(sz[0][0], sz[0][0], core_get()->fp3_p1[3]);
		fp_mul(sz[0][0], sz[0][0], core_get()->fp3_p1[0]);
		fp3_mul_nor(sz[0], sz[0]);
		fp3_mul_nor(sz[0], sz[0]);
		fp3_mul_nor(sz[0], sz[0]);
		fp_mul(sz[1][0], sz[0][0], core_get()->fp3_p2[1]);

		for (int i = 0; i < 3; i++) {
			fp3_mul(ry[i], ry[i], sz[0]);
			fp3_mul(rx[i], rx[i], sz[1]);
		}

		fp9_frb(sx, qx, 10);
		fp9_frb(sy, qy, 10);
		for (int j = 0; j < 10; j++) {
			for (int i = 0; i < 3; i++) {
				fp3_mul(sy[i], sy[i], sz[0]);
				fp3_mul(sx[i], sx[i], sz[1]);
			}
		}
		fp9_set_dig(sz, 1);

		pp_add_k54(l, sx, sy, sz, rx, ry, p);
		fp54_mul_dxs(r, r, l);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp54_free(l);
		ep_free(_p);
		fp9_free(rx);
		fp9_free(ry);
		fp9_free(rz);
		fp9_free(sx);
		fp9_free(sy);
		fp9_free(sz);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void pp_map_k54(fp54_t r, ep_t p, fp9_t qx, fp9_t qy) {
	bn_t a;

	bn_null(a);

	RLC_TRY {
		bn_new(a);

		fp_prime_get_par(a);
		fp54_set_dig(r, 1);

		if (!ep_is_infty(p) && !(fp9_is_zero(qx) && fp9_is_zero(qy))) {
			switch (ep_curve_is_pairf()) {
				case EP_K54:
					/* r = f_{|a|,Q}(P). */
					pp_mil_k54(r, qx, qy, p, a);
					if (bn_sign(a) == RLC_NEG) {
						fp54_inv_cyc(r, r);
					}
					pp_exp_k54(r, r);
					break;
			}
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(a);
	}
}
