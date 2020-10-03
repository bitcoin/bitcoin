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
 * Implementation of binary field addition and subtraction functions.
 *
 * @ingroup fb
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "relic_core.h"
#include "relic_fb.h"
#include "relic_fb_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fb_add(fb_t c, const fb_t a, const fb_t b) {
	fb_addn_low(c, a, b);
}

void fb_add_dig(fb_t c, const fb_t a, dig_t b) {
	fb_add1_low(c, a, b);
}

void fb_sub(fb_t c, const fb_t a, const fb_t b) {
	fb_addn_low(c, a, b);
}

void fb_sub_dig(fb_t c, const fb_t a, dig_t b) {
	fb_add1_low(c, a, b);
}

