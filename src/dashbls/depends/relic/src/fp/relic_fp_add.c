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
	if (carry || (dv_cmp(c, fp_prime_get(), RLC_FP_DIGS) != RLC_LT)) {
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
	if (b == 1) {
		fp_add(c, a, core_get()->one.dp);
	} else {
		fp_t t;

		fp_null(t);

		RLC_TRY {
			fp_new(t);

			fp_set_dig(t, b);
			fp_add(c, a, t);
		} RLC_CATCH_ANY {
			RLC_THROW(ERR_CAUGHT);
		} RLC_FINALLY {
			fp_free(t);
		}
	}
#else
	dig_t carry;

	carry = fp_add1_low(c, a, b);
	if (carry || dv_cmp(c, fp_prime_get(), RLC_FP_DIGS) != RLC_LT) {
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
	if (b == 1) {
		fp_sub(c, a, core_get()->one.dp);
	} else {
		fp_t t;

		fp_null(t);

		RLC_TRY {
			fp_new(t);

			fp_set_dig(t, b);
			fp_sub(c, a, t);
		} RLC_CATCH_ANY {
			RLC_THROW(ERR_CAUGHT);
		} RLC_FINALLY {
			fp_free(t);
		}
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
	if (fp_is_zero(a)) {
		fp_zero(c);
	} else {
		fp_subn_low(c, fp_prime_get(), a);
	}
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
	if (carry || (dv_cmp(c, fp_prime_get(), RLC_FP_DIGS) != RLC_LT)) {
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
		c[RLC_FP_DIGS - 1] ^= ((dig_t)1 << (RLC_DIG - 1));
	}
}

#endif

#if FP_ADD == INTEG || !defined(STRIP)

void fp_hlv_integ(fp_t c, const fp_t a) {
	fp_hlvm_low(c, a);
}

#endif
