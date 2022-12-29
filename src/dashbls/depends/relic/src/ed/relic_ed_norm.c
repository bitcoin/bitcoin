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
 * Implementation of point normalization on Edwards elliptic curves.
 *
 * @ingroup ep
 */

#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if ED_ADD == PROJC || ED_ADD == EXTND || !defined(STRIP)

#include "assert.h"

/**
 * Normalizes a point represented in projective coordinates.
 *
 * @param r			- the result.
 * @param p			- the point to normalize.
 */
static void ed_norm_imp(ed_t r, const ed_t p, int inverted) {
	if (p->coord != BASIC) {
		if (inverted) {
			fp_copy(r->z, p->z);
		} else {
			fp_inv(r->z, p->z);
		}
		fp_mul(r->x, p->x, r->z);
		fp_mul(r->y, p->y, r->z);
#if ED_ADD == EXTND
		fp_mul(r->t, p->t, r->z);
#endif
		fp_set_dig(r->z, 1);

		r->coord = BASIC;
	}
}

#endif /* ED_ADD == PROJC */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ed_norm(ed_t r, const ed_t p) {
	if (ed_is_infty(p)) {
		ed_set_infty(r);
		return;
	}

	if (p->coord == BASIC) {
		/* If the point is represented in affine coordinates, just copy it. */
		ed_copy(r, p);
		return;
	}
#if ED_ADD == PROJC || ED_ADD == EXTND || !defined(STRIP)
	ed_norm_imp(r, p, 0);
#endif /* ED_ADD != BASIC*/
}

void ed_norm_sim(ed_t *r, const ed_t *t, int n) {
	fp_t* a = RLC_ALLOCA(fp_t, n);

	RLC_TRY {
		if (a == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}
		for (int i = 0; i < n; i++) {
			fp_null(a[i]);
			fp_new(a[i]);
			fp_copy(a[i], t[i]->z);
		}

		fp_inv_sim(a, (const fp_t *)a, n);

		for (int i = 0; i < n; i++) {
			fp_copy(r[i]->x, t[i]->x);
			fp_copy(r[i]->y, t[i]->y);
#if ED_ADD == EXTND
			fp_copy(r[i]->t, t[i]->t);
#endif
			if (!ed_is_infty(t[i])) {
				fp_copy(r[i]->z, a[i]);
			}
		}

		for (int i = 0; i < n; i++) {
			ed_norm_imp(r[i], r[i], 1);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		for (int i = 0; i < n; i++) {
			fp_free(a[i]);
		}
		RLC_FREE(a);
	}
}
