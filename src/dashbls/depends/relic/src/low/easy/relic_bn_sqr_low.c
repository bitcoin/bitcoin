/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2009 RELIC Authors
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
 * Implementation of the multiple precision integer arithmetic multiplication
 * functions.
 *
 * @ingroup bn
 */

#include "relic_bn.h"
#include "relic_bn_low.h"
#include "relic_util.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

dig_t bn_sqra_low(dig_t *c, const dig_t *a, int size) {
	int i;
	dig_t t, c0, c1;

	t = a[0];

#ifdef RLC_CONF_NODBL
	dig_t r0, r1, _r0, _r1, s0, s1, t0, t1;
	/* Accumulate this column with the square of a->dp[i]. */
	RLC_MUL_DIG(_r1, _r0, t, t);
	r0 = _r0 + c[0];
	r1 = _r1 + (r0 < _r0);
	c[0] = r0;

	/* Update the carry. */
	c0 = r1;
	c1 = 0;

	/* Version of the main loop not using double-precision types. */
	for (i = 1; i < size; i++) {
		RLC_MUL_DIG(_r1, _r0, t, a[i]);
		r0 = _r0 + _r0;
		r1 = _r1 + _r1 + (r0 < _r0);

		s0 = r0 + c0;
		s1 = r1 + (s0 < r0);

		t0 = s0 + c[i];
		t1 = s1 + (t0 < s0);
		c[i] = t0;

		/* Accumulate the old delayed carry. */
		c0 = t1 + c1;
		/* Compute the new delayed carry. */
		c1 = (t1 < s1) || (s1 < r1) || (r1 < _r1) || (c0 < c1);
	}
#else
	dbl_t r, r0, r1;

	/* Accumulate this column with the square of a->dp[i]. */
	r = (dbl_t)(*c) + (dbl_t)(a[0]) * (dbl_t)(a[0]);
	c[0] = (dig_t)r;

	/* Update the carry. */
	c0 = (dig_t)(r >> (dbl_t)RLC_DIG);
	c1 = 0;

	/* Version using double-precision types is hopefully faster. */
	for (i = 1; i < size; i++) {
		r = (dbl_t)(a[0]) * (dbl_t)(a[i]);
		r0 = r + r;
		r1 = r0 + (dbl_t)(c[i]) + (dbl_t)(c0);
		c[i] = (dig_t)r1;

		/* Accumulate the old delayed carry. */
		c0 = (dig_t)((r1 >> (dbl_t)RLC_DIG) + c1);
		/* Compute the new delayed carry. */
		c1 = (r0 < r) || (r1 < r0) || (c0 < c1);
	}
#endif

	c[size] += c0;
	c1 += (c[size] < c0);
	return c1;
}

void bn_sqrn_low(dig_t *c, const dig_t *a, int size) {
	int i, j;
	const dig_t *tmpa, *tmpb;
	dig_t r0, r1, r2;

	/* Zero the accumulator. */
	r0 = r1 = r2 = 0;

	/* Comba squaring produces one column of the result per iteration. */
	for (i = 0; i < size; i++, c++) {
		tmpa = a;
		tmpb = a + i;

		/* Compute the number of additions in this column. */
		j = (i + 1);
		for (j = 0; j < (i + 1) / 2; j++, tmpa++, tmpb--) {
			RLC_COMBA_STEP_SQR(r2, r1, r0, *tmpa, *tmpb);
		}
		if (!(i & 0x01)) {
			RLC_COMBA_STEP_MUL(r2, r1, r0, *tmpa, *tmpa);
		}
		*c = r0;
		r0 = r1;
		r1 = r2;
		r2 = 0;
	}
	for (i = 0; i < size; i++, c++) {
		tmpa = a + (i + 1);
		tmpb = a + (size - 1);

		/* Compute the number of additions in this column. */
		for (j = 0; j < (size - 1 - i) / 2; j++, tmpa++, tmpb--) {
			RLC_COMBA_STEP_SQR(r2, r1, r0, *tmpa, *tmpb);
		}
		if (!((size - i) & 0x01)) {
			RLC_COMBA_STEP_MUL(r2, r1, r0, *tmpa, *tmpa);
		}
		*c = r0;
		r0 = r1;
		r1 = r2;
		r2 = 0;
	}
}
