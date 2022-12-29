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

#include "relic_fb_low.h"

#define RT		r24

#define RT2		r25

#define R0		r21

#define R1		r22

#define R2		r23

.arch atmega128

.data

.text

#if FB_SQR == INTEG || !defined(STRIP)

#if FB_POLYN == 163

.global fb_sqri_low

.macro ACC0 r0, r1, r2
	ld		\r0, y
	ldd		\r1, y + 16
	ldd		\r2, y + 32
.endm

.macro ACC r0, r1, r2
	ld		RT2, y
	eor		\r0, RT2
	ldd		RT2, y + 16
	eor		\r1, RT2
	ldd		\r2, y + 32
.endm

.macro SQRM_HALF i
	ldd		RT, z + \i
	mov		r28, RT
	andi	r28, 0x0f
	ld		\i + \i,y
.endm

.macro SQRM_STEP i, j
	ldd		RT, z + \i
	mov		r28, RT
	andi	r28, 0x0f
	ld		\i + \i, y
	swap	RT
	mov		r28, RT
	andi	r28, 0x0f
	ld		\i + \i + 1, y
	.if \i < \j
		SQRM_STEP \i+1, \j
	.endif
.endm

fb_sqri_low:
	.irp i, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 28, 29
		push	\i
	.endr

	movw	r30, r20				; Copy a to z
	; Ignore t.
	movw	r26, r24				; Copy c to x

	ldi		r28,lo8(fb_sqrt_table)
	ldi		r29,hi8(fb_sqrt_table)

	SQRM_STEP 0, 9
	SQRM_HALF 10

	ldd		RT, z + 20
	ldi		r28, lo8(fb_sqrm_table0)
	add		r28, RT
	ACC0 	R1, R2, R0

	push	R1				; Store backup of c[21] on stack.

	ldd		RT, z + 19
	mov		RT2, RT
	ldi		r28, lo8(fb_sqrm_table0)
	swap	RT2
	andi	RT2, 0x0F
	add		r28, RT2
	ACC 	R2, R0, R1
	eor		r20, R2
	andi	RT, 0x0F
	ldi		r28, lo8(fb_sqrm_table0)
	add		r28, RT
	ACC 	R0, R1, R2
	eor		r19, R0

	ldd		RT, z + 18
	mov		RT2, RT
	ldi		r28, lo8(fb_sqrm_table0)
	swap	RT2
	andi	RT2, 0x0F
	add		r28, RT2
	ACC		R1, R2, R0
	eor		r18, R1
	andi	RT, 0x0F
	ldi		r28, lo8(fb_sqrm_table0)
	add		r28, RT
	ACC 	R2, R0, R1
	eor		r17, R2

	ldd		RT, z + 17
	mov		RT2, RT
	ldi		r28, lo8(fb_sqrm_table0)
	swap	RT2
	andi	RT2, 0x0F
	add		r28, RT2
	ACC		R0, R1, R2
	eor		r16, R0
	andi	RT, 0x0F
	ldi		r28, lo8(fb_sqrm_table0)
	add		r28, RT
	ACC 	R1, R2, R0
	eor		r15, R1

	ldd		RT, z + 16
	mov		RT2, RT
	ldi		r28, lo8(fb_sqrm_table0)
	swap	RT2
	andi	RT2, 0x0F
	add		r28, RT2
	ACC 	R2, R0, R1
	eor		r14, R2
	andi	RT, 0x0F
	ldi		r28, lo8(fb_sqrm_table0)
	add		r28, RT
	ACC 	R0, R1, R2
	eor		r13, R0

	ldd		RT, z + 15
	mov		RT2, RT
	ldi		r28, lo8(fb_sqrm_table0)
	swap	RT2
	andi	RT2, 0x0F
	add		r28, RT2
	ACC 	R1, R2, R0
	eor		r12, R1
	andi	RT, 0x0F
	ldi		r28, lo8(fb_sqrm_table0)
	add		r28, RT
	ACC		R2, R0, R1
	eor		r11, R2

	ldd		RT, z + 14
	mov		RT2, RT
	ldi		r28, lo8(fb_sqrm_table0)
	swap	RT2
	andi	RT2, 0x0F
	add		r28, RT2
	ACC		R0, R1, R2
	eor		r10, R0
	andi	RT, 0x0F
	ldi		r28, lo8(fb_sqrm_table0)
	add		r28, RT
	ACC 	R1, R2, R0
	eor		r9, R1

	ldd		RT, z + 13
	mov		RT2, RT
	ldi		r28, lo8(fb_sqrm_table0)
	swap	RT2
	andi	RT2, 0x0F
	add		r28, RT2
	ACC 	R2, R0, R1
	eor		r8, R2
	andi	RT, 0x0F
	ldi		r28, lo8(fb_sqrm_table0)
	add		r28, RT
	ACC		R0, R1, R2
	eor		r7, R0

	ldd		RT, z + 12
	mov		RT2, RT
	ldi		r28, lo8(fb_sqrm_table0)
	swap	RT2
	andi	RT2, 0x0F
	add		r28, RT2
	ACC 	R1, R2, R0
	eor		r6, R1
	andi	RT, 0x0F
	ldi		r28, lo8(fb_sqrm_table0)
	add		r28, RT
	ACC		R2, R0, R1
	eor		r5, R2

	ldd		RT, z + 11
	mov		RT2, RT
	ldi		r28, lo8(fb_sqrm_table0)
	swap	RT2
	andi	RT2, 0x0F
	add		r28, RT2
	ACC 	R0, R1, R2
	eor		r4, R0
	andi	RT, 0x0F
	ldi		r28, lo8(fb_sqrm_table0)
	add		r28, RT
	ACC		R1, R2, R0
	eor		r3, R1

	ldd		RT, z + 10
	mov		RT2, RT
	ldi		r28, lo8(fb_sqrm_table0)
	swap	RT2
	andi	RT2, 0x0F
	add		r28, RT2
	ACC 	R2, R0, R1
	eor		r2, R2
	eor		r1, R0
	eor		r0, R1

	movw	r28, r26	; y = c[0]

	.irp i, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
		std	y + \i, \i
	.endr

	pop		RT			; Read c[21].

	eor		r1, RT
	mov		RT2, RT
	lsl		RT2
	lsl		RT2
	lsl		RT2
	eor		r1, RT2
	lsl		RT2
	eor		r1, RT2
	swap	RT
	andi	RT, 0x0F
	lsr		RT
	eor		r1, RT
	lsl		RT2
	eor		r0, RT2

	mov		RT, r20
	lsr		RT
	lsr		RT
	lsr		RT
	mov		RT2, RT
	lsl		RT2
	lsl		RT2
	lsl		RT2
	eor		r0, RT
	eor		r0, RT2
	mov		RT2, RT
	swap	RT2
	andi	RT2, 0xF0
	lsl		RT2
	lsl		RT2
	eor		r0, RT2
	lsl		RT2
	eor		r0, RT2
	std		y + 0, r0

	mov		RT2, RT
	lsr		RT2
	eor		RT, RT2
	lsr		RT
	eor		r1, RT
	std		y + 1, r1

	andi	r20, 0x07
	std		y + 20, r20

	.irp i, 29, 28, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 0
		pop		\i
	.endr

	clr		r1

	ret

#endif

#endif
