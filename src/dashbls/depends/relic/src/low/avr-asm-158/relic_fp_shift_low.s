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

#include "relic_fp_low.h"

//.arch atmega128

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

.text

.global fp_rsh1_low
.global fp_lsh1_low

.macro RSH1_STEP i, j
	ld		r18, -x
	ror		r18
	st		-z, r18
	.if \i < \j
		RSH1_STEP \i + 1, \j
	.endif
.endm

fp_rsh1_low:
	movw 	r30, r24
	movw 	r26, r22
	ldi		r20, RLC_FP_DIGS
	add     r30, r20
	adc     r31, r1
	add		r26, r20
	adc		r27, r1

	clc
	RSH1_STEP 0, RLC_FP_DIGS - 1

	clr		r24
	adc		r24, r1
	ret

.macro LSH1_STEP i, j
	ld		r18, x+
	rol		r18
	st		z+, r18
	.if \i < \j
		LSH1_STEP \i + 1, \j
	.endif
.endm

fp_lsh1_low:
	movw 	r30, r24
	movw 	r26, r22

	clc
	LSH1_STEP 0, RLC_FP_DIGS - 1

	clr		r24
	adc		r24, r1
	ret
