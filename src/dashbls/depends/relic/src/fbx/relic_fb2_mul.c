/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2012 RELIC Authors
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
 * Implementation of multiplication in quadratic extensions of binary fields.
 *
 * @ingroup fbx
 */

#include "relic_core.h"
#include "relic_fbx.h"
#include "relic_fb_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb2_mul(fb2_t c, fb2_t a, fb2_t b) {
	fb_t t0, t1, t2;

	fb_null(t0);
	fb_null(t1);
	fb_null(t2);

	RLC_TRY {
		fb_new(t0);
		fb_new(t1);
		fb_new(t2);

		fb_add(t0, a[0], a[1]);
		fb_add(t1, b[0], b[1]);

		fb_mul(t0, t0, t1);
		fb_mul(t1, a[0], b[0]);
		fb_mul(t2, a[1], b[1]);

		fb_add(c[0], t1, t2);
		fb_add(c[1], t0, t1);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fb_free(t0);
		fb_free(t1);
		fb_free(t2);
	}
}

void fb2_mul_nor(fb2_t c, fb2_t a) {
	fb_t t;

	fb_null(t);

	RLC_TRY {
		fb_new(t);

		fb_copy(t, a[1]);
		fb_add(c[1], a[0], a[1]);
		fb_copy(c[0], t);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fb_free(t);
	}
}

