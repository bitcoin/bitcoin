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
 * Implementation of the low-level binary field bit shifting functions.
 *
 * @ingroup fb
 */

#include "relic_fb_low.h"

.arch atmega128

/*============================================================================*/
/* Private definitions                                                         */
/*============================================================================*/

.macro RSH1_STEP i, j
	ld		r18, -x
	ror		r18
	st		-z, r18
	.if \i < \j
		RSH1_STEP \i + 1, \j
	.endif
.endm

.macro LSH1_STEP i, j
	ld		r18, x+
	rol		r18
	st		z+, r18
	.if \i < \j
		LSH1_STEP \i + 1, \j
	.endif
.endm

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

.text

.global fb_rsh1_low
.global fb_lsh1_low

fb_rsh1_low:
	movw	r30, r24
	movw	r26, r22
	adiw	r30, FB_DIGS
	adiw	r26, FB_DIGS

	clc
	RSH1_STEP 0, FB_DIGS - 1

	clr		r24
	adc		r24, r1
	ret

fb_lsh1_low:
	movw	r30, r24
	movw	r26, r22

	clc
	LSH1_STEP 0, FB_DIGS - 1

	clr		r24
	adc		r24, r1
	ret

#if FB_INV == EXGCD || !defined(STRIP)

.global fb_lsha1_low
.global fb_lsha2_low
.global fb_lsha3_low
.global fb_lsha4_low
.global fb_lsha5_low
.global fb_lsha6_low
.global fb_lsha7_low

fb_lsha1_low:
	movw	r30, r24
	movw	r26, r22

	clc
fb_lsha1_loop:
	ld		r25, z
	ld		r21, x+
	rol		r21
	eor		r25, r21
	st		z+, r25
	dec		r20
	brne	fb_lsha1_loop

	clr		r24
	rol		r24
	ret

fb_lsha2_low:
	movw	r30, r24
	movw	r26, r22

	ld		r25, z
	ld		r21, x+
	clr		r24
	lsl		r21
	rol		r24
	lsl		r21
	rol		r24
	eor		r25, r21
	st		z+, r25
	dec		r20
	breq	fb_lsha2_end
fb_lsha2_loop:
	ld		r25, z
	ld		r21, x+
	eor		r25, r24
	clr		r24
	lsl		r21
	rol		r24
	lsl		r21
	rol		r24
	eor		r25, r21
	st		z+, r25
	dec		r20
	brne	fb_lsha2_loop

fb_lsha2_end:
	ret

fb_lsha3_low:
	movw	r30, r24
	movw	r26, r22

	ld		r25, z
	ld		r21, x+
	clr		r24
	lsl		r21
	rol		r24
	lsl		r21
	rol		r24
	lsl		r21
	rol		r24
	eor		r25, r21
	st		z+, r25
	dec		r20
	breq	fb_lsha3_end
fb_lsha3_loop:
	ld		r25, z
	ld		r21, x+
	eor		r25, r24
	clr		r24
	lsl		r21
	rol		r24
	lsl		r21
	rol		r24
	lsl		r21
	rol		r24
	eor		r25, r21
	st		z+, r25
	dec		r20
	brne	fb_lsha3_loop

fb_lsha3_end:
	ret

fb_lsha4_low:
	movw	r30, r24
	movw	r26, r22

	ld		r25, z
	ld		r21, x+
	swap	r21
	mov		r24, r21
	andi	r21, 0xF0
	andi	r24, 0x0F
	eor		r25, r21
	st		z+, r25
	dec		r20
	breq	fb_lsha4_end
fb_lsha4_loop:
	ld		r25, z
	ld		r21, x+
	eor		r25, r24
	swap	r21
	mov		r24, r21
	andi	r21, 0xF0
	andi	r24, 0x0F
	eor		r25, r21
	st		z+, r25
	dec		r20
	brne	fb_lsha4_loop

fb_lsha4_end:
	ret

fb_lsha5_low:
	movw	r30, r24
	movw	r26, r22

	ld		r25, z
	ld		r21, x+
	swap	r21
	mov		r24, r21
	andi	r21, 0xF0
	andi	r24, 0x0F
	lsl		r21
	rol		r24
	eor		r25, r21
	st		z+, r25
	dec		r20
	breq	fb_lsha5_end
fb_lsha5_loop:
	ld		r25, z
	ld		r21, x+
	eor		r25, r24
	swap	r21
	mov		r24, r21
	andi	r21, 0xF0
	andi	r24, 0x0F
	lsl		r21
	rol		r24
	eor		r25, r21
	st		z+, r25
	dec		r20
	brne	fb_lsha5_loop

fb_lsha5_end:
	ret

fb_lsha6_low:
	movw	r30, r24
	movw	r26, r22

	ld		r25, z
	ld		r24, x+
	clr		r23
	bst		r24, 0
	bld		r23, 6
	bst		r24, 1
	bld		r23, 7
	lsr		r24
	lsr		r24
	eor		r25, r23
	st		z+, r25
	dec		r20
	breq	fb_lsha6_end
fb_lsha6_loop:
	ld		r25, z
	eor		r25, r24
	ld		r24, x+
	bst		r24, 0
	bld		r23, 6
	bst		r24, 1
	bld		r23, 7
	eor		r25, r23
	lsr		r24
	lsr		r24
	st		z+, r25
	dec		r20
	brne	fb_lsha6_loop

fb_lsha6_end:
	ret

fb_lsha7_low:
	movw	r30, r24
	movw	r26, r22

	ld		r25, z
	ld		r24, x+
	clr		r23
	bst		r24, 0
	bld		r23, 7
	lsr		r24
	eor		r25, r23
	st		z+, r25
	dec		r20
	breq	fb_lsha7_end
fb_lsha7_loop:
	ld		r25, z
	eor		r25, r24
	ld		r24, x+
	bst		r24, 0
	bld		r23, 7
	eor		r25, r23
	lsr		r24
	st		z+, r25
	dec		r20
	brne	fb_lsha7_loop

fb_lsha7_end:
	ret

#endif
