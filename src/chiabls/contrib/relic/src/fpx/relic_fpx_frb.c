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
 * Implementation of frobenius action in extensions defined over prime fields.
 *
 * @ingroup fpx
 */

#include "relic_core.h"
#include "relic_fpx_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp2_frb(fp2_t c, fp2_t a, int i) {
	switch (i % 2) {
		case 0:
			fp2_copy(c, a);
			break;
		case 1:
			/* (a_0 + a_1 * u)^p = a_0 - a_1 * u. */
			fp_copy(c[0], a[0]);
			fp_neg(c[1], a[1]);
			break;
	}
}


void fp3_frb(fp3_t c, fp3_t a, int i) {
	switch (i % 3) {
		case 0:
			fp3_copy(c, a);
			break;
		case 1:
			fp3_mul_frb(c, a, 0, 1, 1);
			break;
		case 2:
			fp3_mul_frb(c, a, 0, 2, 1);
			break;
	}
}

void fp6_frb(fp6_t c, fp6_t a, int i) {
	switch (i) {
		case 0:
			fp6_copy(c, a);
			break;
		case 1:
			fp2_frb(c[0], a[0], 1);
			fp2_frb(c[1], a[1], 1);
			fp2_frb(c[2], a[2], 1);
			fp2_mul_frb(c[1], c[1], 1, 2);
			fp2_mul_frb(c[2], c[2], 1, 4);
			break;
		case 2:
			fp2_copy(c[0], a[0]);
			fp2_mul_frb(c[1], a[1], 2, 2);
			fp2_mul_frb(c[2], a[2], 2, 1);
			fp2_neg(c[2], c[2]);
			break;
	}
}

void fp12_frb(fp12_t c, fp12_t a, int i) {
	switch (i) {
		case 0:
			fp12_copy(c, a);
			break;
		case 1:
			fp2_frb(c[0][0], a[0][0], 1);
			fp2_frb(c[1][0], a[1][0], 1);
			fp2_frb(c[0][1], a[0][1], 1);
			fp2_frb(c[1][1], a[1][1], 1);
			fp2_frb(c[0][2], a[0][2], 1);
			fp2_frb(c[1][2], a[1][2], 1);
			fp2_mul_frb(c[1][0], c[1][0], 1, 1);
			fp2_mul_frb(c[0][1], c[0][1], 1, 2);
			fp2_mul_frb(c[1][1], c[1][1], 1, 3);
			fp2_mul_frb(c[0][2], c[0][2], 1, 4);
			fp2_mul_frb(c[1][2], c[1][2], 1, 5);
			break;
		case 2:
			fp2_copy(c[0][0], a[0][0]);
			fp2_mul_frb(c[0][2], a[0][2], 2, 1);
			fp2_mul_frb(c[0][1], a[0][1], 2, 2);
			fp2_neg(c[0][2], c[0][2]);
			fp2_mul_frb(c[1][0], a[1][0], 2, 1);
			fp2_mul_frb(c[1][2], a[1][2], 2, 2);
			fp2_mul_frb(c[1][1], a[1][1], 2, 3);
			fp2_neg(c[1][2], c[1][2]);
			break;
		case 3:
			fp2_frb(c[0][0], a[0][0], 1);
			fp2_frb(c[1][0], a[1][0], 1);
			fp2_frb(c[0][1], a[0][1], 1);
			fp2_frb(c[1][1], a[1][1], 1);
			fp2_frb(c[0][2], a[0][2], 1);
			fp2_frb(c[1][2], a[1][2], 1);
			fp2_mul_frb(c[0][1], c[0][1], 3, 2);
			fp2_mul_frb(c[0][2], c[0][2], 3, 4);
			fp2_neg(c[0][2], c[0][2]);
			fp2_mul_frb(c[1][0], c[1][0], 3, 1);
			fp2_mul_frb(c[1][1], c[1][1], 3, 3);
			fp2_mul_frb(c[1][2], c[1][2], 3, 5);
			fp2_neg(c[1][2], c[1][2]);
			break;
	}
}

void fp18_frb(fp18_t c, fp18_t a, int i) {
	fp3_t t;

	fp3_null(t);

	TRY {
		fp3_new(t);

		fp18_copy(c, a);
		for (int j = 0; j < 3; j++) {
			fp_copy(t[0], a[j][0][0]);
			fp_copy(t[1], a[j][2][0]);
			fp_copy(t[2], a[j][1][1]);
			fp3_frb(t, t, i % 3);
			if (j != 0) {
				fp3_mul_frb(t, t, 1, i, j);
			}
			fp_copy(c[j][0][0], t[0]);
			fp_copy(c[j][2][0], t[1]);
			fp_copy(c[j][1][1], t[2]);

			fp_copy(t[0], a[j][1][0]);
			fp_copy(t[1], a[j][0][1]);
			fp_copy(t[2], a[j][2][1]);
			fp3_frb(t, t, i % 3);
			fp3_mul_frb(t, t, 1, i, j + 3);
			fp_copy(c[j][1][0], t[0]);
			fp_copy(c[j][0][1], t[1]);
			fp_copy(c[j][2][1], t[2]);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp3_free(t);
	}
}
