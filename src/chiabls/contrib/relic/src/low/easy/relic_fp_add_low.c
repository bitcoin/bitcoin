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
 * Implementation of the low-level prime field addition and subtraction
 * functions.
 *
 * @ingroup fp
 */

#include "relic_fp.h"
#include "relic_fp_low.h"
#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

dig_t fp_add1_low(dig_t *c, const dig_t *a, dig_t digit) {
	int i;
	dig_t carry, r0;

	carry = digit;
	for (i = 0; i < FP_DIGS && carry; i++, a++, c++) {
		r0 = (*a) + carry;
		carry = (r0 < carry);
		(*c) = r0;
	}
	for (; i < FP_DIGS; i++, a++, c++) {
		(*c) = (*a);
	}
	return carry;
}

dig_t fp_addn_low(dig_t *c, const dig_t *a, const dig_t *b) {
	int i;
	dig_t carry, c0, c1, r0, r1;

	carry = 0;
	for (i = 0; i < FP_DIGS; i++, a++, b++, c++) {
		r0 = (*a) + (*b);
		c0 = (r0 < (*a));
		r1 = r0 + carry;
		c1 = (r1 < r0);
		carry = c0 | c1;
		(*c) = r1;
	}
	return carry;
}

void fp_addm_low(dig_t *c, const dig_t *a, const dig_t *b) {
	int i;
	dig_t carry, c0, c1, r0, r1;

	carry = 0;
	for (i = 0; i < FP_DIGS; i++, a++, b++) {
		r0 = (*a) + (*b);
		c0 = (r0 < (*a));
		r1 = r0 + carry;
		c1 = (r1 < r0);
		carry = c0 | c1;
		c[i] = r1;
	}
	if (carry || (fp_cmpn_low(c, fp_prime_get()) != CMP_LT)) {
		carry = fp_subn_low(c, c, fp_prime_get());
	}
}

dig_t fp_addd_low(dig_t *c, const dig_t *a, const dig_t *b) {
	int i;
	dig_t carry, c0, c1, r0, r1;

	carry = 0;
	for (i = 0; i < 2 * FP_DIGS; i++, a++, b++) {
		r0 = (*a) + (*b);
		c0 = (r0 < (*a));
		r1 = r0 + carry;
		c1 = (r1 < r0);
		carry = c0 | c1;
		c[i] = r1;
	}
	return carry;
}

void fp_addc_low(dig_t *c, const dig_t *a, const dig_t *b) {
	dig_t carry = fp_addd_low(c, a, b);

	if (carry || (fp_cmpn_low(c + FP_DIGS, fp_prime_get()) != CMP_LT)) {
		carry = fp_subn_low(c + FP_DIGS, c + FP_DIGS, fp_prime_get());
	}
}

dig_t fp_sub1_low(dig_t *c, const dig_t *a, dig_t digit) {
	int i;
	dig_t carry, r0;

	carry = digit;
	for (i = 0; i < FP_DIGS; i++, c++, a++) {
		r0 = (*a) - carry;
		carry = (r0 > (*a));
		(*c) = r0;
	}
	return carry;
}

dig_t fp_subn_low(dig_t *c, const dig_t *a, const dig_t *b) {
	int i;
	dig_t carry, r0, diff;

	/* Zero the carry. */
	carry = 0;
	for (i = 0; i < FP_DIGS; i++, a++, b++, c++) {
		diff = (*a) - (*b);
		r0 = diff - carry;
		carry = ((*a) < (*b)) || (carry && !diff);
		(*c) = r0;
	}
	return carry;
}

void fp_subm_low(dig_t *c, const dig_t *a, const dig_t *b) {
	int i;
	dig_t carry, r0, diff;

	/* Zero the carry. */
	carry = 0;
	for (i = 0; i < FP_DIGS; i++, a++, b++) {
		diff = (*a) - (*b);
		r0 = diff - carry;
		carry = ((*a) < (*b)) || (carry && !diff);
		c[i] = r0;
	}
	if (carry) {
		fp_addn_low(c, c, fp_prime_get());
	}
}

void fp_subc_low(dig_t *c, const dig_t *a, const dig_t *b) {
	dig_t carry = fp_subd_low(c, a, b);

	if (carry) {
		fp_addn_low(c + FP_DIGS, c + FP_DIGS, fp_prime_get());
	}
}

dig_t fp_subd_low(dig_t *c, const dig_t *a, const dig_t *b) {
	int i;
	dig_t carry, r0, diff;

	/* Zero the carry. */
	carry = 0;
	for (i = 0; i < 2 * FP_DIGS; i++, a++, b++) {
		diff = (*a) - (*b);
		r0 = diff - carry;
		carry = ((*a) < (*b)) || (carry && !diff);
		c[i] = r0;
	}
	return carry;
}

void fp_negm_low(dig_t *c, const dig_t *a) {
	fp_subn_low(c, fp_prime_get(), a);
}

dig_t fp_dbln_low(dig_t *c, const dig_t *a) {
	int i;
	dig_t carry, c0, c1, r0, r1;

	carry = 0;
	for (i = 0; i < FP_DIGS; i++, a++, c++) {
		r0 = (*a) + (*a);
		c0 = (r0 < (*a));
		r1 = r0 + carry;
		c1 = (r1 < r0);
		carry = c0 | c1;
		(*c) = r1;
	}
	return carry;
}

void fp_dblm_low(dig_t *c, const dig_t *a) {
	int i;
	dig_t carry, c0, c1, r0, r1;

	carry = 0;
	for (i = 0; i < FP_DIGS; i++, a++) {
		r0 = (*a) + (*a);
		c0 = (r0 < (*a));
		r1 = r0 + carry;
		c1 = (r1 < r0);
		carry = c0 | c1;
		c[i] = r1;
	}
	if (carry || (fp_cmpn_low(c, fp_prime_get()) != CMP_LT)) {
		carry = fp_subn_low(c, c, fp_prime_get());
	}
}

void fp_hlvm_low(dig_t *c, const dig_t *a) {
	dig_t carry = 0;

	if (a[0] & 1) {
		carry = fp_addn_low(c, a, fp_prime_get());
	} else {
		dv_copy(c, a, FP_DIGS);
	}
	fp_rsh1_low(c, c);
	if (carry) {
		c[FP_DIGS - 1] ^= ((dig_t)1 << (FP_DIGIT - 1));
	}
}

void fp_hlvd_low(dig_t *c, const dig_t *a) {
	dig_t carry = 0;

	if (a[0] & 1) {
		carry = fp_addn_low(c, a, fp_prime_get());
	} else {
		dv_copy(c, a, FP_DIGS);
	}

	fp_add1_low(c + FP_DIGS, a + FP_DIGS, carry);

	carry = fp_rsh1_low(c + FP_DIGS, c + FP_DIGS);
	fp_rsh1_low(c, c);
	if (carry) {
		c[FP_DIGS - 1] ^= ((dig_t)1 << (FP_DIGIT - 1));
	}
}

