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
 * Implementation of multiplication in a quadratic extension of a prime field.
 *
 * @ingroup fpx
 */

#include "relic_core.h"
#include "relic_fp_low.h"
#include "relic_fpx_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if FPX_QDR == BASIC || !defined(STRIP)

void fp2_mul_basic(fp2_t c, fp2_t a, fp2_t b) {
	dv_t t0, t1, t2, t3, t4;

	dv_null(t0);
	dv_null(t1);
	dv_null(t2);
	dv_null(t3);
	dv_null(t4);

	RLC_TRY {
		dv_new(t0);
		dv_new(t1);
		dv_new(t2);
		dv_new(t3);
		dv_new(t4);

		/* Karatsuba algorithm. */

		/* t2 = a_0 + a_1, t1 = b_0 + b_1. */
		fp_add(t2, a[0], a[1]);
		fp_add(t1, b[0], b[1]);

		/* t3 = (a_0 + a_1) * (b_0 + b_1). */
		fp_muln_low(t3, t2, t1);

		/* t0 = a_0 * b_0, t4 = a_1 * b_1. */
		fp_muln_low(t0, a[0], b[0]);
		fp_muln_low(t4, a[1], b[1]);

		/* t2 = (a_0 * b_0) + (a_1 * b_1). */
		fp_addc_low(t2, t0, t4);

		/* t1 = (a_0 * b_0) + i^2 * (a_1 * b_1). */
		fp_subc_low(t1, t0, t4);

		/* t1 = u^2 * (a_1 * b_1). */
		for (int i = -1; i > fp_prime_get_qnr(); i--) {
			fp_subc_low(t1, t1, t4);
		}
		for (int i = 1; i < fp_prime_get_qnr(); i++) {
			fp_addc_low(t1, t1, t4);
		}

		/* c_0 = t1 mod p. */
		fp_rdc(c[0], t1);

		/* t4 = t3 - t2. */
		fp_subc_low(t4, t3, t2);

		/* c_1 = t4 mod p. */
		fp_rdc(c[1], t4);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		dv_free(t0);
		dv_free(t1);
		dv_free(t2);
		dv_free(t3);
		dv_free(t4);
	}
}

void fp2_mul_nor_basic(fp2_t c, fp2_t a) {
	fp2_t t;
	bn_t b;

	fp2_null(t);
	bn_null(b);

	RLC_TRY {
		fp2_new(t);
		bn_new(b);

#ifdef FP_QNRES
		/* If p = 3 mod 8, (1 + i) is a QNR/CNR. */
		fp_neg(t[0], a[1]);
		fp_add(c[1], a[0], a[1]);
		fp_add(c[0], t[0], a[0]);
#else
		int qnr = fp2_field_get_qnr();

		switch (fp_prime_get_mod8()) {
			case 3:
				/* If p = 3 mod 8, (1 + i) is a QNR/CNR. */
				fp_neg(t[0], a[1]);
				fp_add(c[1], a[0], a[1]);
				fp_add(c[0], t[0], a[0]);
				break;
			case 1:
			case 5:
				/* If p = 1,5 mod 8, (i) is a QNR/CNR. */
				fp2_mul_art(c, a);
				break;
			case 7:
				/* If p = 7 mod 8, we choose (2^k + i) as a QNR/CNR. */
				fp2_mul_art(t, a);
				fp2_copy(c, a);
				while (qnr > 1) {
					fp2_dbl(c, c);
					qnr = qnr >> 1;
				}
				fp2_add(c, c, t);
				break;
			default:
				RLC_THROW(ERR_NO_VALID);
				break;
		}
#endif
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp2_free(t);
		bn_free(b);
	}
}

#endif

#if FPX_QDR == INTEG || !defined(STRIP)

void fp2_mul_integ(fp2_t c, fp2_t a, fp2_t b) {
	fp2_mulm_low(c, a, b);
}

void fp2_mul_nor_integ(fp2_t c, fp2_t a) {
	fp2_norm_low(c, a);
}

#endif

void fp2_mul_art(fp2_t c, fp2_t a) {
	fp_t t;

	fp_null(t);

	RLC_TRY {
		fp_new(t);

#ifdef FP_QNRES
		/* (a_0 + a_1 * i) * i = -a_1 + a_0 * i. */
		fp_copy(t, a[0]);
		fp_neg(c[0], a[1]);
		fp_copy(c[1], t);
#else
		/* (a_0 + a_1 * i) * i = (a_1 * i^2) + a_0 * i. */
		fp_copy(t, a[0]);
		fp_neg(c[0], a[1]);
		for (int i = -1; i > fp_prime_get_qnr(); i--) {
			fp_sub(c[0], c[0], a[1]);
		}
		for (int i = 0; i <= fp_prime_get_qnr(); i++) {
			fp_add(c[0], c[0], a[1]);
		}
		fp_copy(c[1], t);
#endif
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp_free(t);
	}
}

void fp2_mul_frb(fp2_t c, fp2_t a, int i, int j) {
	ctx_t *ctx = core_get();

#if ALLOC == AUTO
	switch(i) {
		case 1:
			fp2_mul(c, a, ctx->fp2_p1[j - 1]);
			break;
		case 2:
			fp2_mul(c, a, ctx->fp2_p2[j - 1]);
			break;
	}
#else
	fp2_t t;

	fp2_null(t);

	RLC_TRY {
		fp2_new(t);

		switch(i) {
			case 1:
				fp_copy(t[0], ctx->fp2_p1[j - 1][0]);
				fp_copy(t[1], ctx->fp2_p1[j - 1][1]);
				break;
			case 2:
				fp_copy(t[0], ctx->fp2_p2[j - 1][0]);
				fp_copy(t[1], ctx->fp2_p2[j - 1][1]);
				break;
		}

		fp2_mul(c, a, t);
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp2_free(t);
	}
#endif
}

void fp2_mul_dig(fp2_t c, const fp2_t a, dig_t b) {
	fp_mul_dig(c[0], a[0], b);
	fp_mul_dig(c[1], a[1], b);
}
