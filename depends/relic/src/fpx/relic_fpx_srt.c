/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2012 RELIC Authors
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
 * Implementation of square root in extensions defined over prime fields.
 *
 * @ingroup fpx
 */

#include "relic_core.h"
#include "relic_fpx_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int fp2_srt(fp2_t c, fp2_t a) {
	int r = 0;
	fp_t t0;
	fp_t t1;
	fp_t t2;

	fp_null(t0);
	fp_null(t1);
	fp_null(t2);

	if (fp2_is_zero(a)) {
		fp2_zero(c);
		return 1;
	}

	RLC_TRY {
		fp_new(t0);
		fp_new(t1);
		fp_new(t2);

		if (fp_is_zero(a[1])) {
			/* special case: either a[0] is square and sqrt is purely 'real'
			 * or a[0] is non-square and sqrt is purely 'imaginary' */
			r = 1;
			if (fp_srt(t0, a[0])) {
				fp_copy(c[0], t0);
				fp_zero(c[1]);
			} else {
				/* Compute a[0]/i^2. */
#ifdef FP_QNRES
				fp_copy(t0, a[0]);
#else
				if (fp_prime_get_qnr() == -2) {
					fp_hlv(t0, a[0]);
				} else {
					fp_set_dig(t0, -fp_prime_get_qnr());
					fp_inv(t0, t0);
					fp_mul(t0, t0, a[0]);
				}
#endif
				fp_neg(t0, t0);
				fp_zero(c[0]);
				if (!fp_srt(c[1], t0)) {
					/* should never happen! */
					RLC_THROW(ERR_NO_VALID);
				}
			}
		} else {
			/* t0 = a[0]^2 - i^2 * a[1]^2 */
			fp_sqr(t0, a[0]);
			fp_sqr(t1, a[1]);
			for (int i = -1; i > fp_prime_get_qnr(); i--) {
				fp_add(t0, t0, t1);
			}
			fp_add(t0, t0, t1);

			if (fp_srt(t1, t0)) {
				/* t0 = (a_0 + sqrt(t0)) / 2 */
				fp_add(t0, a[0], t1);
				fp_hlv(t0, t0);

				if (!fp_srt(t2, t0)) {
					/* t0 = (a_0 - sqrt(t0)) / 2 */
					fp_sub(t0, a[0], t1);
					fp_hlv(t0, t0);
					if (!fp_srt(t2, t0)) {
						/* should never happen! */
						RLC_THROW(ERR_NO_VALID);
					}
				}
				/* c_0 = sqrt(t0) */
				fp_copy(c[0], t2);
				/* c_1 = a_1 / (2 * sqrt(t0)) */
				fp_dbl(t2, t2);
				fp_inv(t2, t2);
				fp_mul(c[1], a[1], t2);
				r = 1;
			}
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp_free(t0);
		fp_free(t1);
		fp_free(t2);
	}
	return r;
}

int fp3_srt(fp3_t c, fp3_t a) {
	int r = 0;
	fp3_t t0, t1, t2, t3;
	bn_t e;

	fp3_null(t0);
	fp3_null(t1);
	fp3_null(t2);
	fp3_null(t3);
	bn_null(e);

	if (fp3_is_zero(a)) {
		fp3_zero(c);
		return 1;
	}

	RLC_TRY {
		fp3_new(t0);
		fp3_new(t1);
		fp3_new(t2);
		fp3_new(t3);
		bn_new(e);

		switch (fp_prime_get_mod8()) {
			case 5:
				fp3_dbl(t3, a);
				fp3_frb(t0, t3, 1);

				fp3_sqr(t1, t0);
				fp3_mul(t2, t1, t0);
				fp3_mul(t1, t1, t2);

				fp3_frb(t0, t0, 1);
				fp3_mul(t3, t3, t1);
				fp3_mul(t0, t0, t3);

				e->used = RLC_FP_DIGS;
				dv_copy(e->dp, fp_prime_get(), RLC_FP_DIGS);
				bn_div_dig(e, e, 8);
				fp3_exp(t0, t0, e);

				fp3_mul(t0, t0, t2);
				fp3_sqr(t1, t0);
				fp3_mul(t1, t1, a);
				fp3_dbl(t1, t1);

				fp3_mul(t0, t0, a);
				fp_sub_dig(t1[0], t1[0], 1);
				fp3_mul(c, t0, t1);
				break;
			case 3:
			case 7:
				fp3_frb(t0, a, 1);
				fp3_sqr(t1, t0);
				fp3_mul(t2, t1, t0);
				fp3_frb(t0, t0, 1);
				fp3_mul(t3, t2, a);
				fp3_mul(t0, t0, t3);

				e->used = RLC_FP_DIGS;
				dv_copy(e->dp, fp_prime_get(), RLC_FP_DIGS);
				bn_div_dig(e, e, 4);
				fp3_exp(t0, t0, e);

				fp3_mul(t0, t0, a);
				fp3_mul(c, t0, t1);
				break;
			default:
				fp3_zero(c);
				break;
		}

		fp3_sqr(t0, c);
		if (fp3_cmp(t0, a) == RLC_EQ) {
			r = 1;
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp3_free(t0);
		fp3_free(t1);
		fp3_free(t2);
		fp3_free(t3);
		bn_free(e);
	}

	return r;
}

int fp4_srt(fp4_t c, fp4_t a) {
	int r = 0;
	fp2_t t0, t1, t2;

	fp2_null(t0);
	fp2_null(t1);
	fp2_null(t2);

	if (fp4_is_zero(a)) {
		fp4_zero(c);
		return 1;
	}

	RLC_TRY {
		fp2_new(t0);
		fp2_new(t1);
		fp2_new(t2);

		if (fp2_is_zero(a[1])) {
			/* special case: either a[0] is square and sqrt is purely 'real'
			 * or a[0] is non-square and sqrt is purely 'imaginary' */
			r = 1;
			if (fp2_srt(t0, a[0])) {
				fp2_copy(c[0], t0);
				fp2_zero(c[1]);
			} else {
				/* Compute a[0]/s^2. */
				fp2_set_dig(t0, 1);
				fp2_mul_nor(t0, t0);
				fp2_inv(t0, t0);
				fp2_mul(t0, a[0], t0);
				fp2_neg(t0, t0);
				fp2_zero(c[0]);
				if (!fp2_srt(c[1], t0)) {
					/* should never happen! */
					RLC_THROW(ERR_NO_VALID);
				}
				fp2_mul_art(c[1], c[1]);
			}
		} else {
			/* t0 = a[0]^2 - s^2 * a[1]^2 */
			fp2_sqr(t0, a[0]);
			fp2_sqr(t1, a[1]);
			fp2_mul_nor(t2, t1);
			fp2_sub(t0, t0, t2);

			if (fp2_srt(t1, t0)) {
				/* t0 = (a_0 + sqrt(t0)) / 2 */
				fp2_add(t0, a[0], t1);
				fp_hlv(t0[0], t0[0]);
				fp_hlv(t0[1], t0[1]);

				if (!fp2_srt(t2, t0)) {
					/* t0 = (a_0 - sqrt(t0)) / 2 */
					fp2_sub(t0, a[0], t1);
					fp_hlv(t0[0], t0[0]);
					fp_hlv(t0[1], t0[1]);
					if (!fp2_srt(t2, t0)) {
						/* should never happen! */
						RLC_THROW(ERR_NO_VALID);
					}
				}
				/* c_0 = sqrt(t0) */
				fp2_copy(c[0], t2);

				/* c_1 = a_1 / (2 * sqrt(t0)) */
				fp2_dbl(t2, t2);
				fp2_inv(t2, t2);
				fp2_mul(c[1], a[1], t2);
				r = 1;
			}
		}
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		fp2_free(t0);
		fp2_free(t1);
		fp2_free(t2);
	}
	return r;
}
