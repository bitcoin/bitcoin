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
 * Implementation of the low-level prime field addition and subtraction
 * functions.
 *
 * @ingroup fp
 */

#include <gmp.h>

#include "relic_fp.h"
#include "relic_fp_low.h"
#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

dig_t fp_add1_low(dig_t *c, const dig_t *a, const dig_t digit) {
	dig_t *t = RLC_ALLOCA(dig_t, mpn_sec_add_1_itch(RLC_FP_DIGS));
	dig_t r = mpn_sec_add_1(c, a, RLC_FP_DIGS, digit, t);
	RLC_FREE(t);
	return r;
}

dig_t fp_addn_low(dig_t *c, const dig_t *a, const dig_t *b) {
	return mpn_add_n(c, a, b, RLC_FP_DIGS);
}

void fp_addm_low(dig_t *c, const dig_t *a, const dig_t *b) {
	dig_t carry = mpn_add_n(c, a, b, RLC_FP_DIGS);
	dig_t cond = dv_cmp(c, fp_prime_get(), RLC_FP_DIGS) != RLC_LT;
	mpn_cnd_sub_n(carry | cond, c, c, fp_prime_get(), RLC_FP_DIGS);
}

dig_t fp_addd_low(dig_t *c, const dig_t *a, const dig_t *b) {
	return mpn_add_n(c, a, b, 2 * RLC_FP_DIGS);
}

void fp_addc_low(dig_t *c, const dig_t *a, const dig_t *b) {
	dig_t carry = mpn_add_n(c, a, b, 2 * RLC_FP_DIGS);
	dig_t cond = dv_cmp(c + RLC_FP_DIGS, fp_prime_get(), RLC_FP_DIGS) != RLC_LT;
	mpn_cnd_sub_n(carry | cond, c + RLC_FP_DIGS, c + RLC_FP_DIGS,
		fp_prime_get(), RLC_FP_DIGS);
}

dig_t fp_sub1_low(dig_t *c, const dig_t *a, const dig_t digit) {
	dig_t *t = RLC_ALLOCA(dig_t, mpn_sec_sub_1_itch(RLC_FP_DIGS));
	dig_t r = mpn_sec_sub_1(c, a, RLC_FP_DIGS, digit, t);
	RLC_FREE(t);
	return r;
}

dig_t fp_subn_low(dig_t *c, const dig_t *a, const dig_t *b) {
	return mpn_sub_n(c, a, b, RLC_FP_DIGS);
}

void fp_subm_low(dig_t *c, const dig_t *a, const dig_t *b) {
	dig_t carry = mpn_sub_n(c, a, b, RLC_FP_DIGS);
	mpn_cnd_add_n(carry, c, c, fp_prime_get(), RLC_FP_DIGS);
}

dig_t fp_subd_low(dig_t *c, const dig_t *a, const dig_t *b) {
	return mpn_sub_n(c, a, b, 2 * RLC_FP_DIGS);
}

void fp_subc_low(dig_t *c, const dig_t *a, const dig_t *b) {
	dig_t carry = mpn_sub_n(c, a, b, 2 * RLC_FP_DIGS);
	mpn_cnd_sub_n(carry, c + RLC_FP_DIGS, c + RLC_FP_DIGS,
		fp_prime_get(), RLC_FP_DIGS);
}

void fp_negm_low(dig_t *c, const dig_t *a) {
	if (fp_is_zero(a)) {
		fp_zero(c);
	} else {
		mpn_sub_n(c, fp_prime_get(), a, RLC_FP_DIGS);
	}
}

dig_t fp_dbln_low(dig_t *c, const dig_t *a) {
	return mpn_add_n(c, a, a, RLC_FP_DIGS);
}

void fp_dblm_low(dig_t *c, const dig_t *a) {
	dig_t carry = mpn_add_n(c, a, a, RLC_FP_DIGS);
	dig_t cond = dv_cmp(c, fp_prime_get(), RLC_FP_DIGS) != RLC_LT;
	mpn_cnd_sub_n(carry | cond, c, c, fp_prime_get(), RLC_FP_DIGS);
}

void fp_hlvm_low(dig_t *c, const dig_t *a) {
    dig_t carry = mpn_cnd_add_n(a[0] & 1, c, a, fp_prime_get(), RLC_FP_DIGS);
    mpn_rshift(c, c, RLC_FP_DIGS, 1);
    c[RLC_FP_DIGS - 1] ^= ((dig_t)carry << (RLC_DIG - 1));
}

void fp_hlvd_low(dig_t *c, const dig_t *a) {
	dig_t *t = RLC_ALLOCA(dig_t, mpn_sec_add_1_itch(RLC_FP_DIGS));
	dig_t carry = mpn_cnd_add_n(a[0] & 1, c, a, fp_prime_get(), RLC_FP_DIGS);
	mpn_sec_add_1(c + RLC_FP_DIGS, a + RLC_FP_DIGS, RLC_FP_DIGS, carry, t);
	carry = mpn_rshift(c + RLC_FP_DIGS, c + RLC_FP_DIGS, RLC_FP_DIGS, 1);
	mpn_rshift(c, c, RLC_FP_DIGS, 1);
	c[RLC_FP_DIGS - 1] ^= ((dig_t)carry << (RLC_DIG - 1));
	RLC_FREE(t);
}
