/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2021 RELIC Authors
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
 * Implementation of utilities for prime elliptic curves over quadratic
 * extensions.
 *
 * @ingroup epx
 */

#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int ep4_cmp(ep4_t p, ep4_t q) {
    ep4_t r, s;
    int result = RLC_NE;

	if (ep4_is_infty(p) && ep4_is_infty(q)) {
		return RLC_EQ;
	}

    ep4_null(r);
    ep4_null(s);

    RLC_TRY {
        ep4_new(r);
        ep4_new(s);

        if ((p->coord != BASIC) && (q->coord != BASIC)) {
            /* If the two points are not normalized, it is faster to compare
             * x1 * z2^2 == x2 * z1^2 and y1 * z2^3 == y2 * z1^3. */
            fp4_sqr(r->z, p->z);
            fp4_sqr(s->z, q->z);
            fp4_mul(r->x, p->x, s->z);
            fp4_mul(s->x, q->x, r->z);
            fp4_mul(r->z, r->z, p->z);
            fp4_mul(s->z, s->z, q->z);
            fp4_mul(r->y, p->y, s->z);
            fp4_mul(s->y, q->y, r->z);
        } else {
			ep4_norm(r, p);
            ep4_norm(s, q);
        }

        if ((fp4_cmp(r->x, s->x) == RLC_EQ) &&
				(fp4_cmp(r->y, s->y) == RLC_EQ)) {
            result = RLC_EQ;
        }
    } RLC_CATCH_ANY {
        RLC_THROW(ERR_CAUGHT);
    } RLC_FINALLY {
        ep4_free(r);
        ep4_free(s);
    }

    return result;
}
