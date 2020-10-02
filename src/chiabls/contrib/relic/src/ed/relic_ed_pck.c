/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2017 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * RELIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RELIC. If not, see <http://www.gnu.org/licenses/>.
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
/* Private definitions                                                        */
/*============================================================================*/

static void ed_recover_x(fp_t x, const fp_t y, const fp_t d, const fp_t a) {
	fp_t tmpFP1;

	fp_null(tmpFP1);
	fp_new(tmpFP1);

	// x = +/- sqrt((y^2 - 1) / (dy^2 - a))
	fp_sqr(x, y);
	fp_copy(tmpFP1, x);
	fp_sub_dig(x, x, 1);
	fp_mul(tmpFP1, tmpFP1, d);
	fp_sub(tmpFP1, tmpFP1, a);
	fp_inv(tmpFP1, tmpFP1);
	fp_mul(x, x, tmpFP1);
	fp_srt(x, x);

	fp_free(tmpFP1);
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ed_pck(ed_t r, const ed_t p) {
	fp_copy(r->y, p->y);
	int b = fp_get_bit(p->x, 0);
	fp_zero(r->x);
	fp_set_bit(r->x, 0, b);
	fp_set_dig(r->z, 1);
	r->norm = 1;
}

int ed_upk(ed_t r, const ed_t p) {
	int result = 1;
	fp_t t;

	TRY {
		fp_new(t);

		fp_copy(r->y, p->y);
		ed_recover_x(t, p->y, core_get()->ed_d, core_get()->ed_a);

		if (fp_get_bit(t, 0) != fp_get_bit(p->x, 0)) {
			fp_neg(t, t);
		}
		fp_copy(r->x, t);

#if ED_ADD == EXTND
		fp_mul(r->t, r->x, r->y);
#endif

		fp_set_dig(r->z, 1);
		r->norm = 1;
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp_free(t);
	}
	return result;
}