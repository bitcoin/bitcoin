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
 * Implementation of low-level prime field squaring functions.
 *
 * @ingroup fp
 */

#include "relic_fp.h"
#include "relic_fp_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp_sqrn_low(dig_t *c, const dig_t *a) {
	int i, j;
	const dig_t *tmpa, *tmpb;
	dig_t r0, r1, r2;

	/* Zero the accumulator. */
	r0 = r1 = r2 = 0;

	/* Comba squaring produces one column of the result per iteration. */
	for (i = 0; i < RLC_FP_DIGS; i++, c++) {
		tmpa = a;
		tmpb = a + i;

		/* Compute the number of additions in this column. */
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
	for (i = 0; i < RLC_FP_DIGS; i++, c++) {
		tmpa = a + (i + 1);
		tmpb = a + (RLC_FP_DIGS - 1);

		/* Compute the number of additions in this column. */
		for (j = 0; j < (RLC_FP_DIGS - 1 - i) / 2; j++, tmpa++, tmpb--) {
			RLC_COMBA_STEP_SQR(r2, r1, r0, *tmpa, *tmpb);
		}
		if (!((RLC_FP_DIGS - i) & 0x01)) {
			RLC_COMBA_STEP_MUL(r2, r1, r0, *tmpa, *tmpa);
		}
		*c = r0;
		r0 = r1;
		r1 = r2;
		r2 = 0;
	}
}

void fp_sqrm_low(dig_t *c, const dig_t *a) {
	rlc_align dig_t t[2 * RLC_FP_DIGS];

	fp_sqrn_low(t, a);
	fp_rdc(c, t);
}
