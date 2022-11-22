/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2014 RELIC Authors
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
 * Implementation of the low-level quadratic extension field multiplication
 * functions.
 *
 * @ingroup fpx
 */

#include "relic_fp.h"
#include "relic_core.h"
#include "relic_fp_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp2_addn_low(fp2_t c, fp2_t a, fp2_t b) {
	fp_addn_low(c[0], a[0], b[0]);
	fp_addn_low(c[1], a[1], b[1]);
}

void fp2_addm_low(fp2_t c, fp2_t a, fp2_t b) {
	fp_add(c[0], a[0], b[0]);
	fp_add(c[1], a[1], b[1]);
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
	fp_sub(c[0], a[0], b[0]);
	fp_sub(c[1], a[1], b[1]);
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
	fp_dbl(c[0], a[0]);
	fp_dbl(c[1], a[1]);
}

void fp2_norm_low(fp2_t c, fp2_t a) {
	fp2_t t;
	bn_t b;

	fp2_null(t);
	bn_null(b);

	RLC_TRY {
		fp2_new(t);
		bn_new(b);

#if FP_PRIME == 158
		fp_dbl(t[0], a[0]);
		fp_dbl(t[0], t[0]);
		fp_sub(t[0], t[0], a[1]);
		fp_dbl(t[1], a[1]);
		fp_dbl(t[1], t[1]);
		fp_add(c[1], a[0], t[1]);
		fp_copy(c[0], t[0]);
#elif defined(FP_QNRES)
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
			case 5:
				/* If p = 5 mod 8, (u) is a QNR/CNR. */
				fp2_mul_art(c, a);
				break;
			case 7:
				/* If p = 7 mod 8, we choose (2^(lg_4(b-1)) + u) as QNR/CNR. */
				fp2_mul_art(t, a);
				fp2_dbl(c, a);
				fp_prime_back(b, ep_curve_get_b());
				for (int i = 1; i < bn_bits(b) / 2; i++) {
					fp2_dbl(c, c);
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

void fp2_nord_low(dv2_t c, dv2_t a) {
	dv2_t t;
	bn_t b;

	dv2_null(t);
	bn_null(b);

	RLC_TRY {
		dv2_new(t);
		bn_new(b);

#if FP_PRIME == 158
		fp_addc_low(t[0], a[0], a[0]);
		fp_addc_low(t[0], t[0], t[0]);
		fp_subc_low(t[0], t[0], a[1]);
		fp_addc_low(t[1], a[1], a[1]);
		fp_addc_low(t[1], t[1], t[1]);
		fp_addc_low(c[1], a[0], t[1]);
		dv_copy(c[0], t[0], 2 * RLC_FP_DIGS);
#elif defined(FP_QNRES)
		/* If p = 3 mod 8, (1 + i) is a QNR/CNR. */
		/* (a_0 + a_1 * i) * (1 + i) = (a_0 - a_1) + (a_0 + a_1) * u. */
		dv_copy(t[0], a[1], 2 * RLC_FP_DIGS);
		fp_addc_low(c[1], a[0], a[1]);
		fp_subc_low(c[0], a[0], t[0]);
#else
		switch (fp_prime_get_mod8()) {
			case 3:
				/* If p = 3 mod 8, (1 + u) is a QNR, u^2 = -1. */
				/* (a_0 + a_1 * u) * (1 + u) = (a_0 - a_1) + (a_0 + a_1) * u. */
				dv_copy(t[0], a[1], 2 * RLC_FP_DIGS);
				fp_addc_low(c[1], a[0], a[1]);
				fp_subc_low(c[0], a[0], t[0]);
				break;
			case 5:
				/* If p = 5 mod 8, (u) is a QNR. */
				dv_copy(t[0], a[0], 2 * RLC_FP_DIGS);
				dv_zero(t[1], RLC_FP_DIGS);
				dv_copy(t[1] + RLC_FP_DIGS, fp_prime_get(), RLC_FP_DIGS);
				fp_subc_low(c[0], t[1], a[1]);
				for (int i = -1; i > fp_prime_get_qnr(); i--) {
					fp_subc_low(c[0], c[0], a[1]);
				}
				dv_copy(c[1], t[0], 2 * RLC_FP_DIGS);
				break;
			case 7:
				/* If p = 7 mod 8, (2^lg_4(b-1) + u) is a QNR/CNR.   */
				/* (a_0 + a_1 * u)(2^lg_4(b-1) + u) =
				 * (2^lg_4(b-1)a_0 - a_1) + (a_0 + 2^lg_4(b-1)a_1 * u. */
				fp2_addc_low(t, a, a);
				fp_prime_back(b, ep_curve_get_b());
				for (int i = 1; i < bn_bits(b) / 2; i++) {
					fp2_addc_low(t, t, t);
				}
				fp_subc_low(c[0], t[0], a[1]);
				fp_addc_low(c[1], t[1], a[0]);
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
		dv2_free(t);
		bn_free(b);
	}
}
