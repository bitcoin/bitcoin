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
 * Implementation of point normalization on binary elliptic curves.
 *
 * @ingroup eb
 */

#include "string.h"

#include "relic_core.h"
#include "relic_eb.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if EB_ADD == PROJC || !defined(STRIP)

/**
 * Normalizes a point represented in projective coordinates.
 *
 * @param[out] r		- the result.
 * @param[in] p			- the point to normalize.
 * @param inv			- the flag to indicate if z is already inverted.
 */
static void eb_norm_imp(eb_t r, const eb_t p, int inv) {
	if (p->coord != BASIC) {
		if (inv) {
			fb_copy(r->z, p->z);
		} else {
			fb_inv(r->z, p->z);
		}
		fb_mul(r->x, p->x, r->z);
		fb_sqr(r->z, r->z);
		fb_mul(r->y, p->y, r->z);
		fb_set_dig(r->z, 1);
	}

	r->coord = BASIC;
}

#endif /* EB_ADD == PROJC */

/**
 * Normalizes a point represented in lambda-coordinates.
 *
 * @param[out] r		- the result.
 * @param[in] p			- the point to normalize.
 */
static void eb_norm_hlv(eb_t r, const eb_t p) {
	fb_add(r->y, p->x, p->y);
	fb_mul(r->y, r->y, p->x);
	fb_copy(r->x, p->x);
	r->coord = BASIC;
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void eb_norm(eb_t r, const eb_t p) {
	if (eb_is_infty(p)) {
		eb_set_infty(r);
		return;
	}

	if (p->coord == BASIC) {
		/* If the point is represented in affine coordinates, just copy it. */
		eb_copy(r, p);
		return;
	}

	if (p->coord == HALVE) {
		eb_norm_hlv(r, p);
		return;
	}

#if EB_ADD == PROJC || !defined(STRIP)
	eb_norm_imp(r, p, 0);
#endif /* EB_ADD == PROJC */
}

void eb_norm_sim(eb_t *r, const eb_t *t, int n) {
	fb_t *a = RLC_ALLOCA(fb_t, n);

	if (n == 1) {
		eb_norm(r[0], t[0]);
		return;
	}

	RLC_TRY {
		if (a == NULL) {
			RLC_THROW(ERR_NO_MEMORY);
		}
		for (int i = 0; i < n; i++) {
			fb_null(a[i]);
			fb_new(a[i]);
			if (!eb_is_infty(t[i])) {
				fb_copy(a[i], t[i]->z);
			} else {
				fb_set_dig(a[i], 1);
			}
		}

		fb_inv_sim(a, (const fb_t *)a, n);

		for (int i = 0; i < n; i++) {
			fb_copy(r[i]->x, t[i]->x);
			fb_copy(r[i]->y, t[i]->y);
			if (!eb_is_infty(t[i])) {
				fb_copy(r[i]->z, a[i]);
			}
		}

		for (int i = 0; i < n; i++) {
			eb_norm_imp(r[i], r[i], 1);
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		for (int i = 0; i < n; i++) {
			fb_free(a[i]);
		}
		RLC_FREE(a);
	}
}
