/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2021 RELIC Authors
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
 * Implementation of the low-level inversion functions.
 *
 * @&version $Id$
 * @ingroup fp
 */

#include <gmp.h>

#include "relic_fp.h"
#include "relic_fp_low.h"
#include "relic_alloc.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp_invm_low(dig_t *c, const dig_t *a) {
	rlc_align dig_t u[RLC_FP_DIGS];
	dig_t *t = RLC_ALLOCA(dig_t, mpn_sec_invert_itch(RLC_FP_DIGS));
	mp_bitcnt_t cnt = 2 * RLC_FP_DIGS * GMP_NUMB_BITS;

#if FP_RDC == MONTY
	rlc_align dig_t s[2 * RLC_FP_DIGS];
	dv_copy(s, a, RLC_FP_DIGS);
	dv_zero(s + RLC_FP_DIGS, RLC_FP_DIGS);
	fp_rdcn_low(u, s);
#else
	fp_copy(u, a);
#endif

	mpn_sec_invert(c, u, fp_prime_get(), RLC_FP_DIGS, cnt, t);

#if FP_RDC == MONTY
	dv_zero(s, RLC_FP_DIGS);
	dv_copy(s + RLC_FP_DIGS, c, RLC_FP_DIGS);
	dig_t *v = RLC_ALLOCA(dig_t, mpn_sec_div_qr_itch(RLC_FP_DIGS, RLC_FP_DIGS));
	mpn_sec_div_r(s, 2 * RLC_FP_DIGS, fp_prime_get(), RLC_FP_DIGS, v);
	dv_copy(c, s, RLC_FP_DIGS);
	RLC_FREE(v);
#endif
	RLC_FREE(t);
}
