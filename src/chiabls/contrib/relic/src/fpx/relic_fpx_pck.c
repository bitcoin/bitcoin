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
 * Implementation of finite field compression in extension fields.
 *
 * @ingroup fpx
 */

#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp2_pck(fp2_t c, fp2_t a) {
	int b = fp_get_bit(a[1], 0);
	fp2_copy(c, a);
	if (fp2_test_uni(a)) {
		fp_copy(c[0], a[0]);
		fp_zero(c[1]);
		fp_set_bit(c[1], 0, b);
	}
}

int fp2_upk(fp2_t c, fp2_t a) {
	if (fp_bits(a[1]) <= 1) {
		int result, b = fp_get_bit(a[1], 0);
		fp_t t;

		fp_null(t);

		TRY {
			fp_new(t);

			/* a_0^2 + a_1^2 = 1, thus a_1^2 = 1 - a_0^2. */
			fp_sqr(t, a[0]);
			fp_sub_dig(t, t, 1);
			fp_neg(t, t);

			/* a1 = sqrt(a_0^2). */
			result = fp_srt(t, t);

			if (result) {
				/* Verify if least significant bit of the result matches the
				 * compressed second coordinate. */
				if (fp_get_bit(t, 0) != b) {
					fp_neg(t, t);
				}
				fp_copy(c[0], a[0]);
				fp_copy(c[1], t);
			}
		} CATCH_ANY {
			THROW(ERR_CAUGHT);
		} FINALLY {
			fp_free(t);
		}
		return result;
	} else {
		fp2_copy(c, a);
		return 1;
	}
}

void fp12_pck(fp12_t c, fp12_t a) {
	fp12_copy(c, a);
	if (fp12_test_cyc(c)) {
		fp2_zero(c[0][0]);
		fp2_zero(c[1][1]);
	}
}

int fp12_upk(fp12_t c, fp12_t a) {
	if (fp2_is_zero(a[0][0]) && fp2_is_zero(a[1][1])) {
		fp12_back_cyc(c, a);
		if (fp12_test_cyc(c)) {
			return 1;
		} else {
			return 0;
		}
	} else {
		fp12_copy(c, a);
		return 1;
	}
}
