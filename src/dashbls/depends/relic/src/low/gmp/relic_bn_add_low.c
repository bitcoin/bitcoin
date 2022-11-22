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
 * Implementation of the low-level multiple precision addition and subtraction
 * functions.
 *
 * @ingroup bn
 */

#include <gmp.h>

#include "relic_bn.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

dig_t bn_add1_low(dig_t *c, const dig_t *a, dig_t digit, int size) {
	return mpn_add_1(c, a, size, digit);
}

dig_t bn_addn_low(dig_t *c, const dig_t *a, const dig_t *b, int size) {
	return mpn_add_n(c, a, b, size);
}

dig_t bn_sub1_low(dig_t *c, const dig_t *a, dig_t digit, int size) {
	return mpn_sub_1(c, a, size, digit);
}

dig_t bn_subn_low(dig_t *c, const dig_t *a, const dig_t *b, int size) {
	return mpn_sub_n(c, a, b, size);
}
