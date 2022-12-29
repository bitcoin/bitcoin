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
 * Implementation of the Edwards elliptic curve point comparison.
 *
 * @version $Id$
 * @ingroup ed
 */

#include <assert.h>

#include "relic_core.h"
#include "relic_label.h"
#include "relic_md.h"


/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int ed_cmp(const ed_t p, const ed_t q) {
    ed_t r, s;
    int result = RLC_EQ;

    ed_null(r);
    ed_null(s);

    RLC_TRY {
        ed_new(r);
        ed_new(s);

        if ((p->coord != BASIC) && (q->coord != BASIC)) {
            /* If the two points are not normalized, it is faster to compare
             * x1 * z2 == x2 * z1 and y1 * z2 == y2 * z1. */
            fp_mul(r->x, p->x, q->z);
            fp_mul(s->x, q->x, p->z);
            fp_mul(r->y, p->y, q->z);
            fp_mul(s->y, q->y, p->z);
#if ED_ADD == EXTND
			fp_mul(r->t, p->t, q->z);
			fp_mul(s->t, q->t, p->z);
			if (fp_cmp(r->t, s->t) != RLC_EQ) {
	            result = RLC_NE;
	        }
#endif
        } else {
			ed_copy(r, p);
            ed_copy(s, q);
            if (p->coord != BASIC) {
                ed_norm(r, p);
            }
            if (q->coord != BASIC) {
                ed_norm(s, q);
            }
        }

        if (fp_cmp(r->x, s->x) != RLC_EQ) {
            result = RLC_NE;
        }
        if (fp_cmp(r->y, s->y) != RLC_EQ) {
            result = RLC_NE;
        }
    } RLC_CATCH_ANY {
        RLC_THROW(ERR_CAUGHT);
    } RLC_FINALLY {
        ep_free(r);
        ep_free(s);
    }

    return result;
}
