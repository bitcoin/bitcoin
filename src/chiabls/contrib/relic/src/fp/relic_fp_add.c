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
 * Implementation of the prime field addition and subtraction functions.
 *
 * @ingroup fp
 */

#include "relic_core.h"
#include "relic_fp_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if FP_ADD == BASIC || !defined(STRIP)

void fp_add_basic(fp_t c, const fp_t a, const fp_t b) {
	dig_t carry;

	carry = fp_addn_low(c, a, b);
	if (carry || (fp_cmpn_low(c, fp_prime_get()) != CMP_LT)) {
		carry = fp_subn_low(c, c, fp_prime_get());
	}
}

#endif

#if FP_ADD == INTEG || !defined(STRIP)

void fp_add_integ(fp_t c, const fp_t a, const fp_t b) {
	fp_addm_low(c, a, b);
}

#endif

void fp_add_dig(fp_t c, const fp_t a, dig_t b) {
#if FP_RDC == MONTY
	fp_t t;

	fp_null(t);

	TRY {
		fp_new(t);

		fp_set_dig(t, b);
		fp_add(c, a, t);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp_free(t);
	}
#else
	dig_t carry;

	carry = fp_add1_low(c, a, b);
	if (carry || fp_cmpn_low(c, fp_prime_get()) != CMP_LT) {
		carry = fp_subn_low(c, c, fp_prime_get());
	}
#endif
}

#if FP_ADD == BASIC || !defined(STRIP)

void fp_sub_basic(fp_t c, const fp_t a, const fp_t b) {
	dig_t carry;

	carry = fp_subn_low(c, a, b);
	if (carry) {
		fp_addn_low(c, c, fp_prime_get());
	}
}

#endif

#if FP_ADD == INTEG || !defined(STRIP)

void fp_sub_integ(fp_t c, const fp_t a, const fp_t b) {
	fp_subm_low(c, a, b);
}

#endif

void fp_sub_dig(fp_t c, const fp_t a, dig_t b) {
#if FP_RDC == MONTY
	fp_t t;

	fp_null(t);

	TRY {
		fp_new(t);

		fp_set_dig(t, b);
		fp_sub(c, a, t);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp_free(t);
	}
#else
	dig_t carry;

	carry = fp_sub1_low(c, a, b);
	if (carry) {
		fp_addn_low(c, c, fp_prime_get());
	}
#endif
}

#if FP_ADD == BASIC || !defined(STRIP)

void fp_neg_basic(fp_t c, const fp_t a) {
	fp_subn_low(c, fp_prime_get(), a);
}

#endif

#if FP_ADD == INTEG || !defined(STRIP)

void fp_neg_integ(fp_t c, const fp_t a) {
	fp_negm_low(c, a);
}

#endif

#if FP_ADD == BASIC || !defined(STRIP)

void fp_dbl_basic(fp_t c, const fp_t a) {
	dig_t carry;

	carry = fp_lsh1_low(c, a);
	if (carry || (fp_cmpn_low(c, fp_prime_get()) != CMP_LT)) {
		carry = fp_subn_low(c, c, fp_prime_get());
	}
}

#endif

#if FP_ADD == INTEG || !defined(STRIP)

void fp_dbl_integ(fp_t c, const fp_t a) {
	fp_dblm_low(c, a);
}

#endif

#if FP_ADD == BASIC || !defined(STRIP)

void fp_hlv_basic(fp_t c, const fp_t a) {
	dig_t carry = 0;

	if (a[0] & 1) {
		carry = fp_addn_low(c, a, fp_prime_get());
	} else {
		fp_copy(c, a);
	}
	fp_rsh1_low(c, c);
	if (carry) {
		c[FP_DIGS - 1] ^= ((dig_t)1 << (FP_DIGIT - 1));
	}
}

#endif

#if FP_ADD == INTEG || !defined(STRIP)

void fp_hlv_integ(fp_t c, const fp_t a) {
	fp_hlvm_low(c, a);
}

#endif
