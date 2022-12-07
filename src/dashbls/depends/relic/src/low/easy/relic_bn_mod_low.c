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
 * Implementation of the low-level multiple precision integer modular reduction
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

void bn_modn_low(dig_t *c, const dig_t *a, int sa, const dig_t *m, int sm, dig_t u) {
	int i, j;
	dig_t t, r0, r1, r2;
	dig_t *tmp, *tmpc;
	const dig_t *tmpm;

	tmpc = c;

	r0 = r1 = r2 = 0;
	for (i = 0; i < sm; i++, tmpc++, a++) {
		tmp = c;
		tmpm = m + i;
		for (j = 0; j < i; j++, tmp++, tmpm--) {
			RLC_COMBA_STEP_MUL(r2, r1, r0, *tmp, *tmpm);
		}
		if (i < sa) {
			RLC_COMBA_ADD(t, r2, r1, r0, *a);
		}
		*tmpc = (dig_t)(r0 * u);
		RLC_COMBA_STEP_MUL(r2, r1, r0, *tmpc, *m);
		r0 = r1;
		r1 = r2;
		r2 = 0;
	}
	for (i = sm; i < 2 * sm - 1; i++, a++) {
		tmp = c + (i - sm + 1);
		tmpm = m + sm - 1;
		for (j = i - sm + 1; j < sm; j++, tmp++, tmpm--) {
			RLC_COMBA_STEP_MUL(r2, r1, r0, *tmp, *tmpm);
		}
		if (i < sa) {
			RLC_COMBA_ADD(t, r2, r1, r0, *a);
		}
		c[i - sm] = r0;
		r0 = r1;
		r1 = r2;
		r2 = 0;
	}

	if (i < sa) {
		RLC_COMBA_ADD(t, r2, r1, r0, *a);
	}
	c[sm - 1] = r0;
	if (r1) {
		bn_subn_low(c, c, m, sm);
	}
}
