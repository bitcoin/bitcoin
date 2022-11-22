/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2017 RELIC Authors
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
 * Implementation of point compression on Edwards elliptic curves.
 *
 * @ingroup ep
 */

#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ed_pck(ed_t r, const ed_t p) {
	fp_copy(r->y, p->y);
	int b = fp_get_bit(p->x, 0);
	fp_zero(r->x);
	fp_set_bit(r->x, 0, b);
	fp_set_dig(r->z, 1);
	r->coord = BASIC;
}

int ed_upk(ed_t r, const ed_t p) {
	int result = 1;
	fp_t t, u;

	fp_null(t);
	fp_null(u);

	RLC_TRY {
		fp_new(t);
		fp_new(u);

		fp_copy(r->y, p->y);

		/* x = +/- sqrt((y^2 - 1) / (dy^2 - a)). */
		fp_sqr(t, p->y);
		fp_sub_dig(u, t, 1);
		fp_mul(t, t, core_get()->ed_d);
		fp_sub(t, t, core_get()->ed_a);
		fp_inv(t, t);
		fp_mul(u, u, t);
		fp_srt(u, u);

		if (fp_get_bit(u, 0) != fp_get_bit(p->x, 0)) {
			fp_neg(u, u);
		}
		fp_copy(r->x, u);

#if ED_ADD == EXTND
		fp_mul(r->t, r->x, r->y);
#endif
		fp_set_dig(r->z, 1);
		r->coord = BASIC;
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp_free(t);
	}
	return result;
}
