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
 * Implementation of the low-level inversion functions.
 *
 * @&version $Id$
 * @ingroup fp
 */

#include <gmp.h>

#include "relic_fp.h"
#include "relic_fp_low.h"
#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp_invn_low(dig_t *c, const dig_t *a) {
	mp_size_t cn;
	relic_align dig_t s[FP_DIGS], t[2 * FP_DIGS], u[FP_DIGS + 1];

#if FP_RDC == MONTY
	dv_zero(t + FP_DIGS, FP_DIGS);
	dv_copy(t, a, FP_DIGS);
	fp_rdcn_low(u, t);
#else
	fp_copy(u, a);
#endif

	dv_copy(s, fp_prime_get(), FP_DIGS);

	mpn_gcdext(t, c, &cn, u, FP_DIGS, s, FP_DIGS);
	if (cn < 0) {
		dv_zero(c - cn, FP_DIGS + cn);
		mpn_sub_n(c, fp_prime_get(), c, FP_DIGS);
	} else {
		dv_zero(c + cn, FP_DIGS - cn);
	}

#if FP_RDC == MONTY
	dv_zero(t, FP_DIGS);
	dv_copy(t + FP_DIGS, c, FP_DIGS);
	mpn_tdiv_qr(u, c, 0, t, 2 * FP_DIGS, fp_prime_get(), FP_DIGS);
#endif
}
