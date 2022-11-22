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
 * Implementation of the low-level binary field bit shifting functions.
 *
 * @ingroup bn
 */

#include "relic_bn_low.h"

//.arch atmega128

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

.text

.global bn_rsh1_low
.global bn_lsh1_low
.global bn_lsh2_low
.global bn_lsh4_low

bn_rsh1_low:
	movw 	r30, r24
	movw 	r26, r22
	add     r30, r20
	adc     r31, r1
	add		r26, r20
	adc		r27, r1

	clc
bn_rsh1_loop:
	ld		r18, -x
	ror		r18
	st		-z, r18
	dec		r20
	brne	bn_rsh1_loop

	clr		r24
	adc		r24, r1
	ret

bn_lsh1_low:
	movw 	r30, r24
	movw 	r26, r22

	clc
bn_lsh1_loop:
	ld		r18, x+
	rol		r18
	st		z+, r18
	dec		r20
	brne	bn_lsh1_loop

	clr		r24
	adc		r24, r1
	ret
