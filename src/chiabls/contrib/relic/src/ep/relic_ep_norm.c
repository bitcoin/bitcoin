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
 * Implementation of the point normalization on prime elliptic curves.
 *
 * @ingroup ep
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
static void ep_norm_imp(ep_t r, const ep_t p, int inverted) {
	if (!p->norm) {
		fp_t t0, t1;

		fp_null(t0);
		fp_null(t1);

		TRY {

			fp_new(t0);
			fp_new(t1);

			if (inverted) {
				fp_copy(t1, p->z);
			} else {
				fp_inv(t1, p->z);
			}
			fp_sqr(t0, t1);
			fp_mul(r->x, p->x, t0);
			fp_mul(t0, t0, t1);
			fp_mul(r->y, p->y, t0);
			fp_set_dig(r->z, 1);
		}
		CATCH_ANY {
			THROW(ERR_CAUGHT);
		}
		FINALLY {
			fp_free(t0);
			fp_free(t1);
		}
	}

	r->norm = 1;
}

#endif /* EP_ADD == PROJC */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ep_norm(ep_t r, const ep_t p) {
	if (ep_is_infty(p)) {
		ep_set_infty(r);
		return;
	}

	if (p->norm) {
		/* If the point is represented in affine coordinates, we just copy it. */
		ep_copy(r, p);
		return;
	}
#if EP_ADD == PROJC || !defined(STRIP)
	ep_norm_imp(r, p, 0);
#endif /* EP_ADD == PROJC */
}

void ep_norm_sim(ep_t *r, const ep_t *t, int n) {
	int i;
	fp_t a[n];

	for (i = 0; i < n; i++) {
		fp_null(a[i]);
	}

	TRY {
		for (i = 0; i < n; i++) {
			fp_new(a[i]);
			fp_copy(a[i], t[i]->z);
		}

		fp_inv_sim(a, (const fp_t *)a, n);

		for (i = 0; i < n; i++) {
			fp_copy(r[i]->x, t[i]->x);
			fp_copy(r[i]->y, t[i]->y);
			fp_copy(r[i]->z, a[i]);
		}

		for (i = 0; i < n; i++) {
			ep_norm_imp(r[i], r[i], 1);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		for (i = 0; i < n; i++) {
			fp_free(a[i]);
		}
	}
}
