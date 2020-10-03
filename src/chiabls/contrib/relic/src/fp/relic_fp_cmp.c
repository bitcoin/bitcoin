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
 * Implementation of the prime field comparison functions.
 *
 * @ingroup fp
 */

#include "relic_core.h"
#include "relic_fp_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int fp_cmp_dig(const fp_t a, dig_t b) {
#if FP_RDC == MONTY
	fp_t t;
	int r = CMP_EQ;

	fp_null(t);

	TRY {
		fp_new(t);

		fp_prime_conv_dig(t, b);
		r = fp_cmp(a, t);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp_free(t);
	}

	return r;
#else
	for (int i = 1; i < FP_DIGS; i++) {
		if (a[i] > 0) {
			return CMP_GT;
		}
	}

	return fp_cmp1_low(a[0], b);
#endif
}

int fp_cmp(const fp_t a, const fp_t b) {
	return fp_cmpn_low(a, b);
}
