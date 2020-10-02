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
 * Implementation of the basic functions to temporary double precision digit
 * vectors.
 *
 * @ingroup dv
 */

#include <inttypes.h>

#include "relic_core.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void dv_print(dig_t *a, int digits) {
	int i;

	/* Suppress possible unused parameter warning. */
	(void)a;
	for (i = digits - 1; i >= 0; i--) {
		util_print_dig(a[i], 1);
	}
	util_print("\n");

	return;
}

void dv_zero(dig_t *a, int digits) {
	int i;

	if (digits > DV_DIGS) {
		THROW(ERR_NO_PRECI);
	}

	for (i = 0; i < digits; i++, a++)
		(*a) = 0;

	return;
}

void dv_copy(dig_t *c, const dig_t *a, int digits) {
	const dig_t *tmp = a;

	for (int i = 0; i < digits; i++, c++, tmp++) {
		*c = *tmp;
	}
}

void dv_copy_cond(dig_t *c, const dig_t *a, int digits, dig_t cond) {
	dig_t mask, t;

	mask = -cond;
	for (int i = 0; i < digits; i++) {
		t = (a[i] ^ c[i]) & mask;
		c[i] ^= t;
	}
}

void dv_swap_cond(dig_t *c, dig_t *a, int digits, dig_t cond) {
	dig_t mask, t;

	mask = -cond;
	for (int i = 0; i < digits; i++) {
		t = (a[i] ^ c[i]) & mask;
		a[i] ^= t;
		c[i] ^= t;
	}
}

int dv_cmp_const(const dig_t *a, const dig_t *b, int size) {
	int result = 0;

	for (int i = 0; i < size; i++) {
		result |= a[i] ^ b[i];
	}

	return (result == 0 ? CMP_EQ : CMP_NE);
}
