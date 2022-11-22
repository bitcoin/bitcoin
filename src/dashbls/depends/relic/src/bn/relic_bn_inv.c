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
 * Implementation of the multiple precision integer modular inversion.
 *
 * @ingroup bn
 */

#include "relic_core.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void bn_mod_inv(bn_t c, const bn_t a, const bn_t b) {
	bn_t t, u;

	bn_null(t);
	bn_null(u);

	RLC_TRY {
		bn_new(t);
		bn_new(u);

		bn_copy(u, b);
		bn_gcd_ext(t, c, NULL, a, b);

		if (bn_sign(c) == RLC_NEG) {
			bn_add(c, c, u);
		}

		if (bn_cmp_dig(t, 1) != RLC_EQ) {
			RLC_THROW(ERR_NO_VALID);
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(t);
		bn_free(u);
	}
}
