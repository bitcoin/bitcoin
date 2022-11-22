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
 * Implementation of the Frobenius map on binary elliptic curves.
 *
 * @ingroup eb
 */

#include "relic_core.h"
#include "relic_eb.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if defined(EB_KBLTZ)

void eb_frb(eb_t r, const eb_t p) {
	if (eb_is_infty(p)) {
		eb_set_infty(r);
		return;
	}

	fb_sqr(r->x, p->x);
	fb_sqr(r->y, p->y);
	if (p->coord != BASIC) {
		fb_sqr(r->z, p->z);
	} else {
		fb_set_dig(r->z, 1);
	}

	r->coord = p->coord;
}

#endif
