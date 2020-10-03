/*
 * Copyright 2007 Project RELIC
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file.
 *
 * RELIC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RELIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RELIC. If not, see <http://www.gnu.org/licenses/>.
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

.global fp_add1_low
.global fp_sub1_low
.global fp_addn_low
.global fp_subn_low

.macro ADD1_STEP i, j
	ld		r18, x+
	adc		r18, r1
	st		z+, r18
	.if \i < \j
		ADD1_STEP \i + 1, \j
	.endif
.endm

fp_add1_low:
	movw	r30, r24	; copy c to z
	movw	r26, r22	; copy a to x

	;  b is in r20
	; size is in r18

	clc
	ld		r18, x+
	adc		r18, r20
	st		z+, r18

	ADD1_STEP 1, FP_DIGS - 1

	clr		r24
	adc		r24, r1
	ret

.macro ADDN_STEP i, j
	ld		r18, x+
	ld		r19, y+
	adc		r18, r19
	st		z+, r18
	.if \i < \j
		ADDN_STEP \i + 1, \j
	.endif
.endm

fp_addn_low:
	push	r28
	push	r29
	movw	r30, r24	; copy c to z
	movw	r26, r22	; copy a to x
	movw	r28, r20	; copy b to x

	;  b is in r20
	; size is in r18

	clc
	ld		r18, x+
	ld		r19, y+
	adc		r18, r19
	st		z+, r18

	ADDN_STEP 1, FP_DIGS - 1

	clr		r24
	adc		r24, r1
	pop		r29
	pop		r28
	ret

.macro SUB1_STEP i, j
	ld		r18, x+
	sbc		r18, r1
	st		z+, r18
	.if \i < \j
		SUB1_STEP \i + 1, \j
	.endif
.endm

fp_sub1_low:
	movw	r30, r24	; copy c to z
	movw	r26, r22	; copy a to x

	;  b is in r20
	; size is in r18

	clc
	ld		r18, x+
	sbc		r18, r20
	st		z+, r18

	SUB1_STEP 1, FP_DIGS - 1

	clr		r24
	adc		r24, r1
	ret

.macro SUBN_STEP i, j
	ld		r18, x+
	ld		r19, y+
	sbc		r18, r19
	st		z+, r18
	.if \i < \j
		SUBN_STEP \i + 1, \j
	.endif
.endm

fp_subn_low:
	push	r28
	push	r29
	movw	r30, r24	; copy c to z
	movw	r26, r22	; copy a to x
	movw	r28, r20	; copy b to x

	;  b is in r20
	; size is in r18

	clc
	ld		r18, x+
	ld		r19, y+
	sbc		r18, r19
	st		z+, r18

	SUBN_STEP 1, FP_DIGS - 1

	clr		r24
	adc		r24, r1
	pop		r29
	pop		r28
	ret
