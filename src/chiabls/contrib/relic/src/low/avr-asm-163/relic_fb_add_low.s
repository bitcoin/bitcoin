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
 * Implementation of the low-level binary field addition functions.
 *
 * @ingroup fb
 */

#include "relic_fb_low.h"

.arch atmega128

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

.macro ADD1_STEP i, j
	ld		r18, x+
	st		z+, r18
	.if \i < \j
		ADD1_STEP \i + 1, \j
	.endif
.endm

/*
 * Shifts the vector pointed by x by 1 and stores the result in z.
 */
.macro ADDN_STEP i, j
	ld		r18, x+
	ld		r19, y+
	eor		r18, r19
	st		z+, r18
	.if \i < \j
		ADDN_STEP \i + 1, \j
	.endif
.endm

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

.text

.global fb_add1_low
.global fb_addn_low
.global fb_addd_low

fb_add1_low:
	movw	r30, r24
	movw	r26, r22

	ld		r18,x+
	eor		r18,r20
	st		z+, r18

	ADD1_STEP 1, FB_DIGS - 1

	ret

fb_addn_low:
	push	r28
	push	r29
	movw	r30, r24
	movw	r28, r22
	movw	r26, r20

	ADDN_STEP 0, FB_DIGS - 1

	pop		r29
	pop		r28
	ret

fb_addd_low:
	push	r28
	push	r29
	movw 	r30, r24
	movw 	r28, r22
	movw	r26, r20

fb_addd_loop:
	ld		r20, x+
	ld		r21, y+
	eor		r21, r20
	st		z+, r21
	dec		r18
	brne	fb_addd_loop

	pop		r29
	pop		r28
	ret
