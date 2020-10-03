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
 * Implementation of point negation on binary elliptic curves.
 *
 * @ingroup eb
 */

#include "string.h"

#include "relic_core.h"
#include "relic_eb.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if EB_ADD == BASIC || !defined(STRIP)

void eb_neg_basic(eb_t r, const eb_t p) {
	if (eb_is_infty(p)) {
		eb_set_infty(r);
		return;
	}

	if (r != p) {
		fb_copy(r->x, p->x);
		fb_copy(r->z, p->z);
	}

	fb_add(r->y, p->x, p->y);

	r->norm = 1;
}

#endif

#if EB_ADD == PROJC || !defined(STRIP)

void eb_neg_projc(eb_t r, const eb_t p) {
	fb_t t;

	fb_null(t);

	if (eb_is_infty(p)) {
		eb_set_infty(r);
		return;
	}

	if (p->norm) {
		if (r != p) {
			fb_copy(r->x, p->x);
			fb_copy(r->z, p->z);
		}

		fb_add(r->y, p->x, p->y);
		r->norm = 1;
		return;
	}

	TRY {
		fb_new(t);

		fb_mul(t, p->x, p->z);
		fb_add(r->y, p->y, t);
		if (r != p) {
			fb_copy(r->z, p->z);
			fb_copy(r->x, p->x);
		}

		r->norm = 0;
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fb_free(t);
	}
}

#endif
