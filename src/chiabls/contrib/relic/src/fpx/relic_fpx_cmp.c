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
 * Implementation of comparison in extensions defined over prime fields.
 *
 * @ingroup fpx
 */

#include "relic_core.h"
#include "relic_fpx_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int fp2_cmp(fp2_t a, fp2_t b) {
	return (fp_cmp(a[0], b[0]) == CMP_EQ) && (fp_cmp(a[1], b[1]) == CMP_EQ) ?
			CMP_EQ : CMP_NE;
}

int fp2_cmp_dig(fp2_t a, dig_t b) {
	return (fp_cmp_dig(a[0], b) == CMP_EQ) && fp_is_zero(a[1]) ?
			CMP_EQ : CMP_NE;
}

int fp3_cmp(fp3_t a, fp3_t b) {
	return (fp_cmp(a[0], b[0]) == CMP_EQ) && (fp_cmp(a[1], b[1]) == CMP_EQ) &&
			(fp_cmp(a[2], b[2]) == CMP_EQ) ? CMP_EQ : CMP_NE;
}

int fp3_cmp_dig(fp3_t a, dig_t b) {
	return (fp_cmp_dig(a[0], b) == CMP_EQ) && fp_is_zero(a[1]) &&
			fp_is_zero(a[2]) ? CMP_EQ : CMP_NE;
}

int fp6_cmp(fp6_t a, fp6_t b) {
	return (fp2_cmp(a[0], b[0]) == CMP_EQ) && (fp2_cmp(a[1], b[1]) == CMP_EQ) &&
			(fp2_cmp(a[2], b[2]) == CMP_EQ) ? CMP_EQ : CMP_NE;
}

int fp6_cmp_dig(fp6_t a, dig_t b) {
	return (fp2_cmp_dig(a[0], b) == CMP_EQ) && fp2_is_zero(a[1]) &&
			fp2_is_zero(a[2]) ?	CMP_EQ : CMP_NE;
}

int fp12_cmp(fp12_t a, fp12_t b) {
	return (fp6_cmp(a[0], b[0]) == CMP_EQ) && (fp6_cmp(a[1], b[1]) == CMP_EQ) ?
			CMP_EQ : CMP_NE;
}

int fp12_cmp_dig(fp12_t a, dig_t b) {
	return (fp6_cmp_dig(a[0], b) == CMP_EQ) && fp6_is_zero(a[1]) ?
			CMP_EQ : CMP_NE;
}

int fp18_cmp(fp18_t a, fp18_t b) {
	return (fp6_cmp(a[0], b[0]) == CMP_EQ) && (fp6_cmp(a[1], b[1]) == CMP_EQ) &&
			(fp6_cmp(a[2], b[2]) == CMP_EQ ? CMP_EQ : CMP_NE);
}

int fp18_cmp_dig(fp18_t a, dig_t b) {
	return (fp6_cmp_dig(a[0], b) == CMP_EQ) && fp6_is_zero(a[1]) &&
			fp6_is_zero(a[2]) ? CMP_EQ : CMP_NE;
}
