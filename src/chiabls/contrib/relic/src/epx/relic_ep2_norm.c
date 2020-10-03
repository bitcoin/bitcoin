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
 * Implementation of point normalization on prime elliptic curves over quadratic
 * extensions.
 *
 * @ingroup epx
 */

#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if EP_ADD == PROJC || !defined(STRIP)

/**
 * Normalizes a point represented in projective coordinates.
 *
 * @param r			- the result.
 * @param p			- the point to normalize.
 */
static void ep2_norm_imp(ep2_t r, ep2_t p, int inverted) {
	if (!p->norm) {
		fp2_t t0, t1;

		fp2_null(t0);
		fp2_null(t1);

		TRY {

			fp2_new(t0);
			fp2_new(t1);

			if (inverted) {
				fp2_copy(t1, p->z);
			} else {
				fp2_inv(t1, p->z);
			}
			fp2_sqr(t0, t1);
			fp2_mul(r->x, p->x, t0);
			fp2_mul(t0, t0, t1);
			fp2_mul(r->y, p->y, t0);
			fp2_set_dig(r->z, 1);
		}
		CATCH_ANY {
			THROW(ERR_CAUGHT);
		}
		FINALLY {
			fp2_free(t0);
			fp2_free(t1);
		}
	}

	r->norm = 1;
}

#endif /* EP_ADD == PROJC */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ep2_norm(ep2_t r, ep2_t p) {
	if (ep2_is_infty(p)) {
		ep2_set_infty(r);
		return;
	}

	if (p->norm) {
		/* If the point is represented in affine coordinates, we just copy it. */
		ep2_copy(r, p);
	}
#if EP_ADD == PROJC || !defined(STRIP)
	ep2_norm_imp(r, p, 0);
#endif
}

void ep2_norm_sim(ep2_t *r, ep2_t *t, int n) {
	int i;
	fp2_t a[n];

	for (i = 0; i < n; i++) {
		fp2_null(a[i]);
	}

	TRY {
		for (i = 0; i < n; i++) {
			fp2_new(a[i]);
			fp2_copy(a[i], t[i]->z);
		}

		fp2_inv_sim(a, a, n);

		for (i = 0; i < n; i++) {
			fp2_copy(r[i]->x, t[i]->x);
			fp2_copy(r[i]->y, t[i]->y);
			fp2_copy(r[i]->z, a[i]);
		}

		for (i = 0; i < n; i++) {
			ep2_norm_imp(r[i], r[i], 1);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		for (i = 0; i < n; i++) {
			fp2_free(a[i]);
		}
	}
}
