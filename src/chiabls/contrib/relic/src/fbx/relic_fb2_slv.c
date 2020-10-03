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
 * Implementation of quadratic equation solution on binary extension fields.
 *
 * @ingroup fb
 */

#include "relic_core.h"
#include "relic_fbx.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb2_slv(fb2_t c, fb2_t a) {
	/* Compute c_0 = a_0 + a_1. */
	fb_add(c[0], a[0], a[1]);
	/* Compute c_1^2 + c_1 = a_1. */
	fb_slv(c[1], a[1]);
	/* Compute c_0 = a_0 + a_1 + c_1 + Tr(c_1). */
	fb_add(c[0], c[0], c[1]);
	fb_add_dig(c[0], c[0], fb_trc(c[1]));
	/* Make Tr(c_0) = 0. */
	fb_slv(c[0], c[0]);
	/* Compute c_0^2 + c_0 = c_0. */
	fb_add_dig(c[1], c[1], fb_trc(c[1]));
}
