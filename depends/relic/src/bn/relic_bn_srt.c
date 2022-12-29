/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2015 RELIC Authors
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
 * Implementation of the multiple precision integer square root extraction.
 *
 * @ingroup bn
 */

#include "relic_core.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void bn_srt(bn_t c, bn_t a) {
	bn_t h, l, m, t;
	int bits, cmp;

	if (bn_sign(a) == RLC_NEG) {
		RLC_THROW(ERR_NO_VALID);
	}

	bits = bn_bits(a);
	bits += (bits % 2);

	bn_null(h);
	bn_null(l);
	bn_null(m);
	bn_null(t);

	RLC_TRY {
		bn_new(h);
		bn_new(l);
		bn_new(m);
		bn_new(t);

		bn_set_2b(h, bits >> 1);
		bn_set_2b(l, (bits >> 1) - 1);

		/* Trivial binary search approach. */
		do {
			bn_add(m, h, l);
			bn_hlv(m, m);
			bn_sqr(t, m);
			cmp = bn_cmp(t, a);
			bn_sub(t, h, l);

			if (cmp == RLC_GT) {
				bn_copy(h, m);
			} else if (cmp == RLC_LT) {
				bn_copy(l, m);
			}
		} while (bn_cmp_dig(t, 1) == RLC_GT && cmp != RLC_EQ);

		bn_copy(c, m);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(h);
		bn_free(l);
		bn_free(m);
		bn_free(t);
	}
}
