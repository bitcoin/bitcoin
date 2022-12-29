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
 * Implementation of the low-level prime field multiplication functions.
 *
 * @ingroup bn
 */

#include <gmp.h>

#include "relic_fp.h"
#include "relic_fp_low.h"
#include "relic_alloc.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

dig_t fp_mula_low(dig_t *c, const dig_t *a, dig_t digit) {
	return mpn_addmul_1(c, a, RLC_FP_DIGS, digit);
}

dig_t fp_mul1_low(dig_t *c, const dig_t *a, dig_t digit) {
	return mpn_mul_1(c, a, RLC_FP_DIGS, digit);
}

void fp_muln_low(dig_t *c, const dig_t *a, const dig_t *b) {
	dig_t *t = RLC_ALLOCA(dig_t, mpn_sec_mul_itch(RLC_FP_DIGS, RLC_FP_DIGS));
	mpn_sec_mul(c, a, RLC_FP_DIGS, b, RLC_FP_DIGS, t);
	RLC_FREE(t);
}

void fp_mulm_low(dig_t *c, const dig_t *a, const dig_t *b) {
	rlc_align dig_t t[2 * RLC_FP_DIGS];
	fp_muln_low(t, a, b);
	fp_rdc(c, t);
}
