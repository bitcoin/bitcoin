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
 * Implementation of the multiple precision integer arithmetic squaring
 * functions.
 *
 * @ingroup fp
 */

#include "relic_core.h"
#include "relic_fp_low.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if FP_KARAT > 0 || !defined(STRIP)

/**
 * Computes the square of a multiple precision integer using recursive Karatsuba
 * squaring.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the prime field element to square.
 * @param[in] size			- the number of digits to square.
 * @param[in] level			- the number of Karatsuba steps to apply.
 */
static void fp_sqr_karat_imp(dv_t c, const fp_t a, int size, int level) {
	int i, h, h1;
	dv_t t0, t1, a0a0, a1a1;
	dig_t carry;

	/* Compute half the digits of a or b. */
	h = size >> 1;
	h1 = size - h;

	dv_null(t0);
	dv_null(t1);
	dv_null(a0a0);
	dv_null(a1a1);

	RLC_TRY {
		/* Allocate the temp variables. */
		dv_new(t0);
		dv_new(t1);
		dv_new(a0a0);
		dv_new(a1a1);
		dv_zero(t0, 2 * h1);
		dv_zero(t1, 2 * (h1 + 1));
		dv_zero(a0a0, 2 * h);
		dv_zero(a1a1, 2 * h1);

		if (level <= 1) {
			/* a0a0 = a0 * a0 and a1a1 = a1 * a1 */
#if FP_SQR == BASIC
			for (i = 0; i < h - 1; i++) {
				a0a0[h + i + 1] = bn_sqra_low(a0a0 + 2 * i, a + i, h - i);
			}
			bn_sqra_low(a0a0 + 2 * i, a + i, 1);
			for (i = 0; i < h1 - 1; i++) {
				a1a1[h1 + i + 1] = bn_sqra_low(a1a1 + 2 * i, a + h + i, h1 - i);
			}
			bn_sqra_low(a1a1 + 2 * i, a + h + i, 1);
#elif FP_SQR == COMBA || FP_SQR == INTEG
			bn_sqrn_low(a0a0, a, h);
			bn_sqrn_low(a1a1, a + h, h1);
#elif FP_SQR == MULTP
			bn_muln_low(a0a0, a, a, h);
			bn_muln_low(a1a1, a + h, a + h, h1);
#endif
		} else {
			fp_sqr_karat_imp(a0a0, a, h, level - 1);
			fp_sqr_karat_imp(a1a1, a + h, h1, level - 1);
		}

		/* t2 = a1 * a1 << 2*h digits + a0 * a0. */
		for (i = 0; i < 2 * h; i++) {
			c[i] = a0a0[i];
		}
		for (i = 0; i < 2 * h1; i++) {
			c[2 * h + i] = a1a1[i];
		}

		/* t = (a1 + a0) */
		carry = bn_addn_low(t0, a, a + h, h);
		carry = bn_add1_low(t0 + h, t0 + h, carry, 2);
		if (h1 > h) {
			carry = bn_add1_low(t0 + h, t0 + h, *(a + 2 * h), 2);
		}

		if (level <= 1) {
			/* a1a1 = (a1 + a0)*(a1 + a0) */
#if FP_SQR == BASIC
			for (i = 0; i < h1; i++) {
				t1[h1 + i + 2] = bn_sqra_low(t1 + (2 * i), t0 + i, h1 + 1 - i);
			}
			bn_sqra_low(t1 + (2 * i), t0 + i, 1);
#elif FP_SQR == COMBA || FP_SQR == INTEG
			bn_sqrn_low(t1, t0, h1 + 1);
#elif FP_SQR == MULTP
			bn_muln_low(t1, t0, t0, h1 + 1);
#endif
		} else {
			fp_sqr_karat_imp(t1, t0, h1 + 1, level - 1);
		}

		/* t = t - (a0*a0 << h digits) */
		carry = bn_subn_low(t1, t1, a0a0, 2 * h);
		bn_sub1_low(t1 + 2 * h, t1 + 2 * h, carry, 2 * (h1 + 1) - 2 * h);

		/* t = t - (a1*a1 << h digits) */
		carry = bn_subn_low(t1, t1, a1a1, 2 * h1);
		bn_sub1_low(t1 + 2 * h, t1 + 2 * h, carry, 2 * (h1 + 1) - 2 * h);

		/* c = c + [(a1 + a0)*(a1 + a0) << digits] */
		c += h;
		carry = bn_addn_low(c, c, t1, 2 * (h1 + 1));
		c += 2 * (h1 + 1);
		if (2 * size > h + 2 * (h1 + 1)) {
			carry = bn_add1_low(c, c, carry, 2 * size - h - 2 * (h1 + 1));
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		dv_free(t0);
		dv_free(t1);
		dv_free(a0a0);
		dv_free(a1a1);
	}
}
#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if FP_SQR == BASIC || !defined(STRIP)

void fp_sqr_basic(fp_t c, const fp_t a) {
	int i;
	dv_t t;

	dv_null(t);

	RLC_TRY {
		dv_new(t);
		dv_zero(t, 2 * RLC_FP_DIGS);

		for (i = 0; i < RLC_FP_DIGS - 1; i++) {
			t[RLC_FP_DIGS + i + 1] =
					bn_sqra_low(t + 2 * i, a + i, RLC_FP_DIGS - i);
		}
		bn_sqra_low(t + 2 * i, a + i, 1);

		fp_rdc(c, t);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp_free(t);
	}
}

#endif

#if FP_SQR == COMBA || !defined(STRIP)

void fp_sqr_comba(fp_t c, const fp_t a) {
	dv_t t;

	dv_null(t);

	RLC_TRY {
		dv_new(t);

		fp_sqrn_low(t, a);

		fp_rdc(c, t);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp_free(t);
	}
}

#endif

#if FP_SQR == INTEG || !defined(STRIP)

void fp_sqr_integ(fp_t c, const fp_t a) {
	fp_sqrm_low(c, a);
}

#endif

#if FP_KARAT > 0 || !defined(STRIP)

void fp_sqr_karat(fp_t c, const fp_t a) {
	dv_t t;

	dv_null(t);

	RLC_TRY {
		dv_new(t);
		dv_zero(t, 2 * RLC_FP_DIGS);

		if (RLC_FP_DIGS > 1) {
			fp_sqr_karat_imp(t, a, RLC_FP_DIGS, FP_KARAT);
		} else {
			fp_sqrn_low(t, a);
		}


		fp_rdc(c, t);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		dv_free(t);
	}
}

#endif
