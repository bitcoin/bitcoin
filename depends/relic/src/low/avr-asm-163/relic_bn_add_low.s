/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2011 RELIC Authors
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
 * Implementation of the low-level multiple precision addition functions.
 *
 * @ingroup bn
 */

#include "relic_bn_low.h"

.arch atmega128

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

.text

.global bn_add1_low
.global bn_addn_low
.global bn_sub1_low
.global bn_subn_low

bn_add1_low:
	movw	r30, r24	; copy c to z
	movw	r26, r22	; copy a to x

	;  b is in r20
	; size is in r18

	clc
	ld		r23, x+
	adc		r23, r20
	st		z+, r23
	dec		r18
	breq	bn_add1_end

bn_add1_loop:
	ld		r23, x+
	adc		r23, r1
	st		z+, r23
	dec		r18
	brne	bn_add1_loop
bn_add1_end:
	clr		r24
	adc		r24, r1
	ret

bn_addn_low:
	push	r28
	push	r29

	movw	r30, r24	; copy c to z
	movw	r26, r22	; copy a to x
	movw	r28, r20	; copy b to y

	; size is in r18

	clc
	tst		r18
	breq	bn_addn_end

bn_addn_loop:
	ld		r23, x+
	ld		r24, y+
	adc		r23, r24
	st		z+, r23
	dec		r18
	brne	bn_addn_loop
bn_addn_end:
	clr		r24
	adc		r24, r1

	pop		r29
	pop		r28
	ret

bn_sub1_low:
	movw	r30, r24	; copy c to z
	movw	r26, r22	; copy a to x

	;  b is in r20
	; size is in r18

	clc
	ld		r23, x+
	sbc		r23, r20
	st		z+, r23
	dec		r18
	breq	bn_sub1_end

bn_sub1_loop:
	ld		r23, x+
	sbc		r23, r1
	st		z+, r23
	dec		r18
	brne	bn_sub1_loop
bn_sub1_end:
	clr		r24
	adc		r24, r1
	ret

bn_subn_low:
	push	r28
	push	r29

	movw	r30, r24	; copy c to z
	movw	r26, r22	; copy a to x
	movw	r28, r20	; copy b to y

	; size is in r18

	clc
	tst		r18
	breq	bn_subn_end

bn_subn_loop:
	ld		r23, x+
	ld		r24, y+
	sbc		r23, r24
	st		z+, r23
	dec		r18
	brne	bn_subn_loop
bn_subn_end:
	clr		r24
	adc		r24, r1

	pop		r29
	pop		r28
	ret
