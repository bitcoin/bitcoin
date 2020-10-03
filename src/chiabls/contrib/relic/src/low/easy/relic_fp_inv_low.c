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

#include "relic_bn.h"
#include "relic_bn_low.h"
#include "relic_fp.h"
#include "relic_fp_low.h"
#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp_invn_low(dig_t *c, const dig_t *a) {
	bn_st e;

	bn_init(&e, FP_DIGS);

	e.used = FP_DIGS;
	dv_copy(e.dp, fp_prime_get(), FP_DIGS);
	bn_sub1_low(e.dp, e.dp, 2, FP_DIGS);
#if AUTO == ALLOC
	fp_exp(c, a, &e);
#else
	fp_exp(c, (const fp_t)a, &e);
#endif

	bn_clean(&e);
}
