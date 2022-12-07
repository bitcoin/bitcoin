/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2020 RELIC Authors
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

int ep2_cmp(ep2_t p, ep2_t q) {
    ep2_t r, s;
    int result = RLC_NE;

	if (ep2_is_infty(p) && ep2_is_infty(q)) {
		return RLC_EQ;
	}

    ep2_null(r);
    ep2_null(s);

    RLC_TRY {
        ep2_new(r);
        ep2_new(s);

        if ((p->coord != BASIC) && (q->coord != BASIC)) {
            /* If the two points are not normalized, it is faster to compare
             * x1 * z2^2 == x2 * z1^2 and y1 * z2^3 == y2 * z1^3. */
            fp2_sqr(r->z, p->z);
            fp2_sqr(s->z, q->z);
            fp2_mul(r->x, p->x, s->z);
            fp2_mul(s->x, q->x, r->z);
            fp2_mul(r->z, r->z, p->z);
            fp2_mul(s->z, s->z, q->z);
            fp2_mul(r->y, p->y, s->z);
            fp2_mul(s->y, q->y, r->z);
        } else {
			ep2_norm(r, p);
            ep2_norm(s, q);
        }

        if ((fp2_cmp(r->x, s->x) == RLC_EQ) &&
				(fp2_cmp(r->y, s->y) == RLC_EQ)) {
            result = RLC_EQ;
        }
    } RLC_CATCH_ANY {
        RLC_THROW(ERR_CAUGHT);
    } RLC_FINALLY {
        ep2_free(r);
        ep2_free(s);
    }

    return result;
}
