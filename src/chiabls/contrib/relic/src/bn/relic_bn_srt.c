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

	if (bn_sign(a) == BN_NEG) {
		THROW(ERR_NO_VALID);
	}

	bits = bn_bits(a);
	bits += (bits % 2);

	bn_null(h);
	bn_null(l);
	bn_null(m);
	bn_null(t);

	TRY{
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

			if (cmp == CMP_GT) {
				bn_copy(h, m);
			} else if (cmp == CMP_LT) {
				bn_copy(l, m);
			}
			bn_sub(t, h, l);
		} while (bn_cmp_dig(t, 1) == CMP_GT && cmp != CMP_EQ);

		bn_copy(c, m);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(h);
		bn_free(l);
		bn_free(m);
		bn_free(t);
	}
}
