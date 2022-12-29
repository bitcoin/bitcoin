/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2012 RELIC Authors
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

void fp_invm_low(dig_t *c, const dig_t *a) {
	bn_t bnc;
	bn_t bna;
	bn_t gcd;
	bn_t bnp;
	const dig_t *p;

	bn_null(bnc);
	bn_null(bna);
	bn_null(gcd);
	bn_null(bnp);

	bn_new(bnc);
	bn_new(bna);
	bn_new(gcd);
	bn_new(bnp);

	p = fp_prime_get();

#if FP_RDC == MONTY
	fp_prime_back(bna, a);
#else
	bna->used = RLC_FP_DIGS;
	dv_copy(bna->dp, a, RLC_FP_DIGS);
#endif
	bnp->used = RLC_FP_DIGS;
	dv_copy(bnp->dp, p, RLC_FP_DIGS);

	bn_gcd_ext(gcd, bnc, NULL, bna, bnp);

	while (bn_sign(bnc) == RLC_NEG) {
		bn_add(bnc, bnc, bnp);
	}
	while (bn_cmp(bnc, bnp) != RLC_LT) {
		bn_sub(bnc, bnc, bnp);
	}

#if FP_RDC == MONTY
	fp_prime_conv(c, bnc);
#else
	dv_copy(c, bnc->dp, bnc->used);
	for (int i = bnc->used; i < RLC_FP_DIGS; i++) {
		c[i] = 0;
	}
#endif
}
