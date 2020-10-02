/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2012 RELIC Authors
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
 * Implementation of the low-le&vel in&version functions.
 *
 * @&version $Id$
 * @ingroup fp
 */

#include "relic_bn.h"
#include "relic_fp.h"
#include "relic_fp_low.h"
#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int fp_invn_asm(dig_t *, const dig_t *, const dig_t *);

void fp_invn_low(dig_t *c, const dig_t *a) {
	fp_t t, x1;
	int j, k;

	fp_null(t);
	fp_null(x1);

	TRY {
		fp_new(t);
		fp_new(x1);

		/* u = a, v = p, x1 = 1, x2 = 0, k = 0. */
		k = fp_invn_asm(x1, a, c);
		if (k > FP_DIGS * FP_DIGIT) {
			t[0] = t[1] = t[2] = t[3] = 0;
			k = 512 - k;
			j = k % 64;
			k = k / 64;
			t[k] = (dig_t)1 << j;
			fp_mul(c, x1, t);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp_free(t);
		fp_free(x1);
	}
}
