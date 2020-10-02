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
 * Implementation of the low-level extension field addition functions.
 *
 * @ingroup fpx
 */

#include "relic_core.h"
#include "relic_fp_low.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp2_addn_low(fp2_t c, fp2_t a, fp2_t b) {
	fp_addn_low(c[0], a[0], b[0]);
	fp_addn_low(c[1], a[1], b[1]);
}

void fp2_addm_low(fp2_t c, fp2_t a, fp2_t b) {
	fp_addm_low(c[0], a[0], b[0]);
	fp_addm_low(c[1], a[1], b[1]);
}

void fp2_addd_low(dv2_t c, dv2_t a, dv2_t b) {
	fp_addd_low(c[0], a[0], b[0]);
	fp_addd_low(c[1], a[1], b[1]);
}

void fp2_addc_low(dv2_t c, dv2_t a, dv2_t b) {
	fp_addc_low(c[0], a[0], b[0]);
	fp_addc_low(c[1], a[1], b[1]);
}

void fp2_subn_low(fp2_t c, fp2_t a, fp2_t b) {
	fp_subn_low(c[0], a[0], b[0]);
	fp_subn_low(c[1], a[1], b[1]);
}

void fp2_subm_low(fp2_t c, fp2_t a, fp2_t b) {
	fp_subm_low(c[0], a[0], b[0]);
	fp_subm_low(c[1], a[1], b[1]);
}

void fp2_dbln_low(fp2_t c, fp2_t a) {
	/* 2 * (a0 + a1 * u) = 2 * a0 + 2 * a1 * u. */
	fp_dbln_low(c[0], a[0]);
	fp_dbln_low(c[1], a[1]);
}

void fp2_subd_low(dv2_t c, dv2_t a, dv2_t b) {
	fp_subd_low(c[0], a[0], b[0]);
	fp_subd_low(c[1], a[1], b[1]);
}

void fp2_subc_low(dv2_t c, dv2_t a, dv2_t b) {
	fp_subc_low(c[0], a[0], b[0]);
	fp_subc_low(c[1], a[1], b[1]);
}

void fp2_dblm_low(fp2_t c, fp2_t a) {
	/* 2 * (a0 + a1 * u) = 2 * a0 + 2 * a1 * u. */
	fp_dblm_low(c[0], a[0]);
	fp_dblm_low(c[1], a[1]);
}

void fp2_norm_low(fp2_t c, fp2_t a) {
	fp2_t t;
	bn_t b;

	fp2_null(t);
	bn_null(b);

	TRY {
		fp2_new(t);
		bn_new(b);

#ifdef FP_QNRES
		/* If p = 3 mod 8, (1 + i) is a QNR/CNR. */
		fp_neg(t[0], a[1]);
		fp_add(c[1], a[0], a[1]);
		fp_add(c[0], t[0], a[0]);
#else
		switch (fp_prime_get_mod8()) {
			case 3:
				/* If p = 3 mod 8, (1 + u) is a QNR/CNR. */
				fp_neg(t[0], a[1]);
				fp_add(c[1], a[0], a[1]);
				fp_add(c[0], t[0], a[0]);
				break;
			case 1:
			case 5:
				/* If p = 1,5 mod 8, (u) is a QNR/CNR. */
				fp2_mul_art(c, a);
				break;
			case 7:
				/* If p = 7 mod 8, we choose (2 + u) as QNR/CNR. */
				fp2_mul_art(t, a);
				fp2_dbl(c, a);
				fp2_dbl(c, c);
				fp2_add(c, c, t);
				break;
			default:
				THROW(ERR_NO_VALID);
				break;
		}
#endif
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp2_free(t);
		bn_free(b);
	}
}

void fp2_nord_low(dv2_t c, dv2_t a) {
	dv2_t t;
	bn_t b;

	dv2_null(t);
	bn_null(b);

	TRY {
		dv2_new(t);
		bn_new(b);

#ifdef FP_QNRES
		/* If p = 3 mod 8, (1 + i) is a QNR/CNR. */
		/* (a_0 + a_1 * i) * (1 + i) = (a_0 - a_1) + (a_0 + a_1) * u. */
		dv_copy(t[0], a[1], 2 * FP_DIGS);
		fp_addc_low(c[1], a[0], a[1]);
		fp_subc_low(c[0], a[0], t[0]);
#else
		switch (fp_prime_get_mod8()) {
			case 3:
				/* If p = 3 mod 8, (1 + u) is a QNR, u^2 = -1. */
				/* (a_0 + a_1 * u) * (1 + u) = (a_0 - a_1) + (a_0 + a_1) * u. */
				dv_copy(t[0], a[1], 2 * FP_DIGS);
				fp_addc_low(c[1], a[0], a[1]);
				fp_subc_low(c[0], a[0], t[0]);
				break;
			case 1:
			case 5:
				/* If p = 1,5 mod 8, (u) is a QNR. */
				dv_copy(t[0], a[0], 2 * FP_DIGS);
				dv_zero(t[1], FP_DIGS);
				dv_copy(t[1] + FP_DIGS, fp_prime_get(), FP_DIGS);
				fp_subc_low(c[0], t[1], a[1]);
				for (int i = -1; i > fp_prime_get_qnr(); i--) {
					fp_subc_low(c[0], c[0], a[1]);
				}
				dv_copy(c[1], t[0], 2 * FP_DIGS);
				break;
			case 7:
				/* If p = 7 mod 8, (4 + u) is a QNR/CNR.   */
				fp2_addc_low(t, a, a);
				fp2_addc_low(t, t, t);
				fp_subc_low(c[0], t[0], a[1]);
				fp_addc_low(c[1], t[1], a[0]);
				break;
			default:
				THROW(ERR_NO_VALID);
				break;
		}
#endif
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv2_free(t);
		bn_free(b);
	}
}

void fp2_norh_low(dv2_t c, dv2_t a) {
	dv2_t t;
	bn_t b;

	dv2_null(t);
	bn_null(b);

	TRY {
		dv2_new(t);
		bn_new(b);

#ifdef FP_QNRES
		/* If p = 3 mod 8, (1 + i) is a QNR/CNR. */
		/* (a_0 + a_1 * i) * (1 + i) = (a_0 - a_1) + (a_0 + a_1) * u. */
		dv_copy(t[1], a[1], 2 * FP_DIGS);
		fp_addd_low(c[1], a[0], a[1]);
		/* c_0 = c_0 + 2^N * p/2. */
		dv_copy(t[0], a[0], 2 * FP_DIGS);
		bn_lshb_low(t[0] + FP_DIGS - 1, t[0] + FP_DIGS - 1, FP_DIGS + 1, 1);
		fp_addn_low(t[0] + FP_DIGS, t[0] + FP_DIGS, fp_prime_get());
		bn_rshb_low(t[0] + FP_DIGS - 1, t[0] + FP_DIGS - 1, FP_DIGS + 1, 1);
		fp_subd_low(c[0], t[0], t[1]);
#else
		switch (fp_prime_get_mod8()) {
			case 3:
				/* If p = 3 mod 8, (1 + u) is a QNR, u^2 = -1. */
				/* (a_0 + a_1 * u) * (1 + u) = (a_0 - a_1) + (a_0 + a_1) * u. */
				dv_copy(t[0], a[1], 2 * FP_DIGS);
				fp_addc_low(c[1], a[0], a[1]);
				fp_subc_low(c[0], a[0], t[0]);
				break;
			case 1:
			case 5:
				/* If p = 1,5 mod 8, (u) is a QNR. */
				dv_copy(t[0], a[0], 2 * FP_DIGS);
				dv_zero(t[1], FP_DIGS);
				dv_copy(t[1] + FP_DIGS, fp_prime_get(), FP_DIGS);
				fp_subc_low(c[0], t[1], a[1]);
				for (int i = -1; i > fp_prime_get_qnr(); i--) {
					fp_subc_low(c[0], c[0], a[1]);
				}
				dv_copy(c[1], t[0], 2 * FP_DIGS);
				break;
			case 7:
				/* If p = 7 mod 8, (4 + u) is a QNR/CNR.   */
				fp2_addc_low(t, a, a);
				fp2_addc_low(t, t, t);
				fp_subc_low(c[0], t[0], a[1]);
				fp_addc_low(c[1], t[1], a[0]);
				break;
			default:
				THROW(ERR_NO_VALID);
				break;
		}
#endif
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv2_free(t);
		bn_free(b);
	}
}

void fp3_addn_low(fp3_t c, fp3_t a, fp3_t b) {
	fp_addn_low(c[0], a[0], b[0]);
	fp_addn_low(c[1], a[1], b[1]);
	fp_addn_low(c[2], a[2], b[2]);
}

void fp3_addm_low(fp3_t c, fp3_t a, fp3_t b) {
	fp_addm_low(c[0], a[0], b[0]);
	fp_addm_low(c[1], a[1], b[1]);
	fp_addm_low(c[2], a[2], b[2]);
}

void fp3_subn_low(fp3_t c, fp3_t a, fp3_t b) {
	fp_subn_low(c[0], a[0], b[0]);
	fp_subn_low(c[1], a[1], b[1]);
	fp_subn_low(c[2], a[2], b[2]);
}

void fp3_subm_low(fp3_t c, fp3_t a, fp3_t b) {
	fp_subm_low(c[0], a[0], b[0]);
	fp_subm_low(c[1], a[1], b[1]);
	fp_subm_low(c[2], a[2], b[2]);
}

void fp3_dbln_low(fp3_t c, fp3_t a) {
	fp_dbln_low(c[0], a[0]);
	fp_dbln_low(c[1], a[1]);
	fp_dbln_low(c[2], a[2]);
}

void fp3_subc_low(dv3_t c, dv3_t a, dv3_t b) {
	fp_subc_low(c[0], a[0], b[0]);
	fp_subc_low(c[1], a[1], b[1]);
	fp_subc_low(c[2], a[2], b[2]);
}

void fp3_dblm_low(fp3_t c, fp3_t a) {
	fp_dblm_low(c[0], a[0]);
	fp_dblm_low(c[1], a[1]);
	fp_dblm_low(c[2], a[2]);
}
