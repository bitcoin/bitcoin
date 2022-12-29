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
 * Implementation of the low-level prime field modular reduction functions.
 *
 * @ingroup fp
 */

#include <gmp.h>

#include "relic_core.h"
#include "relic_fp.h"
#include "relic_fp_low.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp_rdcs_low(dig_t *c, const dig_t *a, const dig_t *m) {
	rlc_align dig_t q[2 * RLC_FP_DIGS], _q[2 * RLC_FP_DIGS];
	rlc_align dig_t t[2 * RLC_FP_DIGS], r[RLC_FP_DIGS];
	const int *sform;
	int len, first, i, j, k, b0, d0, b1, d1;

	sform = fp_prime_get_sps(&len);

	RLC_RIP(b0, d0, sform[len - 1]);
	first = (d0) + (b0 == 0 ? 0 : 1);

	/* q = floor(a/b^k) */
	dv_rshd(q, a, 2 * RLC_FP_DIGS, d0);
	if (b0 > 0) {
		bn_rshb_low(q, q, 2 * RLC_FP_DIGS, b0);
	}

	/* r = a - qb^k. */
	dv_copy(r, a, first);
	if (b0 > 0) {
		r[first - 1] &= RLC_MASK(b0);
	}

	k = 0;
	while (!fp_is_zero(q)) {
		dv_zero(_q, 2 * RLC_FP_DIGS);
		for (i = len - 2; i > 0; i--) {
			j = (sform[i] < 0 ? -sform[i] : sform[i]);
			RLC_RIP(b1, d1, j);
			dv_zero(t, 2 * RLC_FP_DIGS);
			dv_lshd(t, q, 2 * RLC_FP_DIGS, d1);
			if (b1 > 0) {
				bn_lshb_low(t, t, 2 * RLC_FP_DIGS, b1);
			}
			/* Check if these two have the same sign. */
			if ((sform[len - 2] < 0) == (sform[i] < 0)) {
				bn_addn_low(_q, _q, t, 2 * RLC_FP_DIGS);
			} else {
				bn_subn_low(_q, _q, t, 2 * RLC_FP_DIGS);
			}
		}
		/* Check if these two have the same sign. */
		if ((sform[len - 2] < 0) == (sform[0] < 0)) {
			bn_addn_low(_q, _q, q, 2 * RLC_FP_DIGS);
		} else {
			bn_subn_low(_q, _q, q, 2 * RLC_FP_DIGS);
		}
		dv_rshd(q, _q, 2 * RLC_FP_DIGS, d0);
		if (b0 > 0) {
			bn_rshb_low(q, q, 2 * RLC_FP_DIGS, b0);
		}
		if (b0 > 0) {
			_q[first - 1] &= RLC_MASK(b0);
		}
		if (sform[len - 2] < 0) {
			fp_addm_low(r, r, _q);
		} else {
			if (k++ % 2 == 0) {
				if (fp_subn_low(r, r, _q)) {
					fp_addn_low(r, r, m);
				}
			} else {
				fp_addn_low(r, r, _q);
			}
		}
	}
	while (dv_cmp(r, m, RLC_FP_DIGS) != RLC_LT) {
		fp_subn_low(r, r, m);
	}
	fp_copy(c, r);
}

void fp_rdcn_low(dig_t *c, dig_t *a) {
	dig_t r, u = *(fp_prime_get_rdc());
	const dig_t *m = fp_prime_get();

	for (int i = 0; i < RLC_FP_DIGS; i++, a++) {
		r = (dig_t)(*a * u);
		*a = mpn_addmul_1(a, m, RLC_FP_DIGS, r);
	}
	fp_addm_low(c, a, a - RLC_FP_DIGS);
}
