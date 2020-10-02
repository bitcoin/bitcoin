/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007, 2008, 2009 RELIC Authors
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
	return mpn_add_1(c, a, FP_DIGS, digit);
}

dig_t fp_addn_low(dig_t *c, const dig_t *a, const dig_t *b) {
	return mpn_add_n(c, a, b, FP_DIGS);
}

void fp_addm_low(dig_t *c, const dig_t *a, const dig_t *b) {
	dig_t carry;

	carry = mpn_add_n(c, a, b, FP_DIGS);

	if (carry || (fp_cmpn_low(c, fp_prime_get()) != CMP_LT)) {
		carry = mpn_sub_n(c, c, fp_prime_get(), FP_DIGS);
	}
}

dig_t fp_addd_low(dig_t *c, const dig_t *a, const dig_t *b) {
	return mpn_add_n(c, a, b, 2 * FP_DIGS);
}

void fp_addc_low(dig_t *c, const dig_t *a, const dig_t *b) {
	dig_t carry;

	carry = mpn_add_n(c, a, b, 2 * FP_DIGS);

	if (carry || fp_cmpn_low(c + FP_DIGS, fp_prime_get()) != CMP_LT) {
		mpn_sub_n(c + FP_DIGS, c + FP_DIGS, fp_prime_get(), FP_DIGS);
	}
}

dig_t fp_sub1_low(dig_t *c, const dig_t *a, const dig_t digit) {
	return mpn_sub_1(c, a, FP_DIGS, digit);
}

dig_t fp_subn_low(dig_t *c, const dig_t *a, const dig_t *b) {
	return mpn_sub_n(c, a, b, FP_DIGS);
}

void fp_subm_low(dig_t *c, const dig_t *a, const dig_t *b) {
	if (mpn_sub_n(c, a, b, FP_DIGS)) {
		mpn_add_n(c, c, fp_prime_get(), FP_DIGS);
	}
}

dig_t fp_subd_low(dig_t *c, const dig_t *a, const dig_t *b) {
	return mpn_sub_n(c, a, b, 2 * FP_DIGS);
}

void fp_subc_low(dig_t *c, const dig_t *a, const dig_t *b) {
	if (mpn_sub_n(c, a, b, 2 * FP_DIGS)) {
		mpn_add_n(c + FP_DIGS, c + FP_DIGS, fp_prime_get(), FP_DIGS);
	}
}

void fp_negm_low(dig_t *c, const dig_t *a) {
	mpn_sub_n(c, fp_prime_get(), a, FP_DIGS);
}

dig_t fp_dbln_low(dig_t *c, const dig_t *a) {
	return mpn_add_n(c, a, a, FP_DIGS);
}

void fp_dblm_low(dig_t *c, const dig_t *a) {
	dig_t carry = mpn_add_n(c, a, a, FP_DIGS);
	if (carry || (fp_cmpn_low(c, fp_prime_get()) != CMP_LT)) {
		carry = fp_subn_low(c, c, fp_prime_get());
	}
}

void fp_hlvm_low(dig_t *c, const dig_t *a) {
    dig_t carry = 0;

    if (a[0] & 1) {
            carry = mpn_add_n(c, a, fp_prime_get(), FP_DIGS);
    } else {
            dv_copy(c, a, FP_DIGS);
    }
    mpn_rshift(c, c, FP_DIGS, 1);
    if (carry) {
            c[FP_DIGS - 1] ^= ((dig_t)1 << (FP_DIGIT - 1));
    }
}

void fp_hlvd_low(dig_t *c, const dig_t *a) {
	dig_t carry = 0;

	if (a[0] & 1) {
		carry = mpn_add_n(c, a, fp_prime_get(), FP_DIGS);
	} else {
		dv_copy(c, a, FP_DIGS);
	}

	mpn_add_1(c + FP_DIGS, a + FP_DIGS, FP_DIGS, carry);

	carry = mpn_rshift(c + FP_DIGS, c + FP_DIGS, FP_DIGS, 1);
	mpn_rshift(c, c, FP_DIGS, 1);
	if (carry) {
		c[FP_DIGS - 1] ^= ((dig_t)1 << (FP_DIGIT - 1));
	}
}
