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
 * Implementation of pairing computation for curves with embedding degree 48.
 *
 * @ingroup pp
 */

#include "relic_core.h"
#include "relic_pp.h"
#include "relic_util.h"

/*============================================================================*/
/* Private definitions                                                         */
/*============================================================================*/

static void pp_mil_k48(fp48_t r, fp8_t qx, fp8_t qy, ep_t p, bn_t a) {
	fp48_t l;
	ep_t _p;
	fp8_t rx, ry, rz;
	int i, len = bn_bits(a) + 1;
	int8_t s[RLC_FP_BITS + 1];

	fp48_null(l);
	ep_null(_p);
	fp8_null(rx);
	fp8_null(ry);
	fp8_null(rz);

	RLC_TRY {
		fp48_new(l);
		ep_new(_p);
		fp8_new(rx);
		fp8_new(ry);
		fp8_new(rz);

		fp48_zero(l);
		fp8_copy(rx, qx);
		fp8_copy(ry, qy);
		fp8_set_dig(rz, 1);
#if EP_ADD == BASIC
		ep_neg(_p, p);
#else
		fp_add(_p->x, p->x, p->x);
		fp_add(_p->x, _p->x, p->x);
		fp_neg(_p->y, p->y);
#endif

		bn_rec_naf(s, &len, a, 2);
		for (i = len - 2; i >= 0; i--) {
			fp48_sqr(r, r);
			pp_dbl_k48(l, rx, ry, rz, _p);
			fp48_mul_dxs(r, r, l);
			if (s[i] > 0) {
				pp_add_k48(l, rx, ry, rz, qx, qy, p);
				fp48_mul_dxs(r, r, l);
			}
			if (s[i] < 0) {
				fp8_neg(qy, qy);
				pp_add_k48(l, rx, ry, rz, qx, qy, p);
				fp8_neg(qy, qy);
				fp48_mul_dxs(r, r, l);
			}
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp48_free(l);
		ep_free(_p);
		fp8_free(rx);
		fp8_free(ry);
		fp8_free(rz);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void pp_map_k48(fp48_t r, ep_t p, fp8_t qx, fp8_t qy) {
	bn_t a;

	bn_null(a);

	RLC_TRY {
		bn_new(a);

		fp_prime_get_par(a);
		fp48_set_dig(r, 1);

		if (!ep_is_infty(p) && !(fp8_is_zero(qx) && fp8_is_zero(qy))) {
			switch (ep_curve_is_pairf()) {
				case EP_B48:
					/* r = f_{|a|,Q}(P). */
					pp_mil_k48(r, qx, qy, p, a);
					if (bn_sign(a) == RLC_NEG) {
						fp48_inv_cyc(r, r);
					}
					pp_exp_k48(r, r);
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
