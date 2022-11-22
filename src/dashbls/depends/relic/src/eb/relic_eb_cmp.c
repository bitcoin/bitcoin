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
 * Implementation of the binary elliptic curve point comparison.
 *
 * @ingroup eb
 */

#include "relic_core.h"
#include "relic_eb.h"
#include "relic_conf.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int eb_cmp(const eb_t p, const eb_t q) {
    eb_t r, s;
    int result = RLC_NE;

	if (eb_is_infty(p) && eb_is_infty(q)) {
		return RLC_EQ;
	}

    eb_null(r);
    eb_null(s);

    RLC_TRY {
        eb_new(r);
        eb_new(s);

        if ((p->coord == PROJC) && (q->coord == PROJC)) {
            /* If the two points are not normalized, it is faster to compare
             * x1 * z2 == x2 * z1 and y1 * z2^2 == y2 * z1^2. */
            fb_mul(r->x, p->x, q->z);
            fb_mul(s->x, q->x, p->z);
            fb_sqr(r->z, p->z);
            fb_sqr(s->z, q->z);
            fb_mul(r->y, p->y, s->z);
            fb_mul(s->y, q->y, r->z);
        } else {
            if (p->coord == BASIC) {
                eb_copy(r, p);
            } else {
                eb_norm(r, p);
            }

            if (q->coord == BASIC) {
                eb_copy(s, q);
            } else {
                eb_norm(s, q);
            }
        }

        if ((fb_cmp(r->x, s->x) == RLC_EQ) && (fb_cmp(r->y, s->y) == RLC_EQ)) {
            result = RLC_EQ;
        }
    } RLC_CATCH_ANY {
        RLC_THROW(ERR_CAUGHT);
    } RLC_FINALLY {
        eb_free(r);
        eb_free(s);
    }

    return result;
}
