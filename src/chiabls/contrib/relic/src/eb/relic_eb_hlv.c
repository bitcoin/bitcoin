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
 * Implementation of point doubling on binary elliptic curves.
 *
 * @ingroup eb
 */

#include "string.h"

#include "relic_core.h"
#include "relic_eb.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void eb_hlv(eb_t r, const eb_t p) {
	fb_t l, t;

	fb_null(l);
	fb_null(t);

	TRY {
		fb_new(l);
		fb_new(t);

		/* Solve l^2 + l = u + a. */
		switch (eb_curve_opt_a()) {
			case OPT_ZERO:
				fb_copy(t, p->x);
				break;
			case OPT_ONE:
				fb_add_dig(t, p->x, (dig_t)1);
				break;
			case OPT_DIGIT:
				fb_add_dig(t, p->x, eb_curve_get_a()[0]);
				break;
			default:
				fb_add(t, p->x, eb_curve_get_a());
				break;
		}

		fb_slv(l, t);

		if (p->norm == 1) {
			/* Compute t = v + u * lambda. */
			fb_mul(t, l, p->x);
			fb_add(t, t, p->y);
		} else {
			/* Compute t = u * (u + lambda_P + lambda). */
			fb_add(t, l, p->y);
			fb_add(t, t, p->x);
			fb_mul(t, t, p->x);
		}

		/* If Tr(t) = 0 then lambda_P = lambda, u = sqrt(t + u). */
		if (fb_trc(t) == 0) {
			fb_copy(r->y, l);
			fb_add(t, t, p->x);
			fb_srt(r->x, t);
		} else {
			/* Else lambda_P = lambda + 1, u = sqrt(t). */
			fb_add_dig(r->y, l, 1);
			fb_srt(r->x, t);
		}
		fb_set_dig(r->z, 1);
		r->norm = 2;
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fb_free(l);
		fb_free(t);
	}
}
