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
 * Binary field multiplication functions.
 *
 * @ingroup fb
 */

#include "relic_fb_low.h"

.arch atmega128

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/*
 * Pointer register which points to the parameter a.
 */
#define A_PTR		X

/*
 * Pointer register which points to the parameter b.
 */
#define B_PTR		Z

/*
 * Pointer register which points to the parameter c.
 */
#define C_PTR		Z

/*
 * Temporary register.
 */
#define RT			r23

/*
 * Carry register for shifting.
 */
#define RC			r22

/*
 * Size in bits of the multiplication of a binary field element by a polynomial
 * with degree lower than 4.
 */
#define POLY_M4		(FB_POLYN + 4)

/*
 * Size in bytes of a precomputation table line.
 */
#if (POLY_M4 % WSIZE) > 0
#define T_LINE	(POLY_M4/WSIZE + 1)
#else
#define T_LINE	(POLY_M4/WSIZE)
#endif

/*
 * Size of the multiplication precomputation table in bytes.
 */
#define T_DIGS	(16 * T_LINE)

/*
 * Size of the precomputation table in blocks of 256 bytes.
 */
#if (T_DIGS % 256) > 0
#define T_BLOCKS	(T_DIGS/256 + 1)
#else
#define T_BLOCKS	(T_DIGS/256)
#endif

/*
 * Precomputation table stored on a custom section (address must be terminated
 * in 0x00).
 */
.data
.byte
/*
 * This table will be loaded on address 0x100.
 */
fb_muln_table0: .space 256, 0
fb_muln_table1: .space 256, 0
fb_sqrt_table:
.byte 0x00, 0x01, 0x04, 0x05, 0x10, 0x11, 0x14, 0x15, 0x40, 0x41, 0x44, 0x45, 0x50, 0x51, 0x54, 0x55
fb_sqrm_table0:
.byte 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07
fb_sqrm_table1:
.byte 0x00, 0x19, 0x64, 0x7d, 0x92, 0x8b, 0xf6, 0xef, 0x48, 0x51, 0x2c, 0x35, 0xda, 0xc3, 0xbe, 0xa7
fb_sqrm_table2:
.byte 0x00, 0x20, 0x80, 0xa0, 0x00, 0x20, 0x80, 0xa0, 0x00, 0x20, 0x80, 0xa0, 0x00, 0x20, 0x80, 0xa0

.global fb_sqrt_table
.global fb_sqrm_table0
.global fb_sqrm_table1
.global fb_sqrm_table2
.global fb_rdcn_table0
.global fb_rdcn_table1

/*
 * Loads the pointer Y with the 16-bit address composed by i and j.
 */
.macro LOAD_T_PTR i j
	ldi		r28, \i
	ldi		r29, \j
.endm

/*
 * Prepares the first column values.
 */
.macro PREP_FIRST r1, r2, r4, r8
	ldd		\r1, B_PTR + 0	; r18 = r1 = b[0]
	mov		\r2, \r1
	lsl		\r2				; r19 = r2 = r1 << 1
	mov		\r4, \r2
	lsl		\r4				; r20 = r4 = r1 << 2
	mov 	\r8, \r4
	lsl		\r8				; r21 = r8 = r1 << 3
.endm

/*
 * Prepares the values for column i.
 */
.macro PREP_COLUMN i, r1, r2, r4, r8
	mov		RT, \r1

	ldd		\r1, B_PTR+\i	; r1 = b[i]
	mov		\r2, \r1
	lsl		RT
	rol		\r2
	mov		\r4, \r2
	lsl 	RT
	rol		\r4
	mov		\r8, \r4
	lsl		RT
	rol		\r8
.endm

/*
 * Prepares the values for the last column.
 */
.macro PREP_LAST i, r1, r2, r4, r8
	swap	\r1
	andi	\r1, 0x0f
	lsr		\r1			; t = b[i] >> 5

	mov		\r8, \r1
	lsr		\r1
	mov		\r4, \r1
	lsr		\r1
	mov		\r2, \r1
	clr		\r1
.endm

/*
 * Fills a column of the precomputation table.
 */
.macro FILL_COLUMN offset, r1, r2, r4, r8
	std		y + 00, \r1		; tab[1][i] = r1
	std		y + 16, \r2		; tab[2][i] = r2;
	mov		RT, \r1
	eor		RT, \r2
	std		y + 32, RT		; tab[3][i] = r1^r2
	std		y + 48, \r4		; tab[4][i] = r4
	add		r28, RC			; &tab[5][i]
	eor		RT, \r4
	std		y + 32, RT		; tab[7][i] = r1^r2^r4
	eor		RT, \r1
	std		y + 16, RT		; tab[6][i] = r2^r4
	mov		RT, \r1
	eor		RT, \r4
	std		y + 00, RT		; tab[5][i] = r1^r4;
	std		y + 48, \r8		; tab[8][i] = r8
	add		r28, RC			; &tab[9][i]
	mov		RT, \r1
	eor		RT, \r8			;
	std		y + 00, RT		; tab[9][i] = r1^r8
	eor		RT, \r2
	std		y + 32, RT		; tab[11][i] = r1^r2^r8
	eor		RT, \r1
	std		y + 16, RT		; tab[10][i] = r2^r8
	eor		\r4, \r8
	std		y + 48, \r4		; tab[12][i] = r4^r8
	add		r28, RC			; &tab[13][i]
	eor		\r4, \r1
	std		y + 00, \r4		; tab[13][i] = r1^r4^r8
	eor		\r4, \r2
	std		y + 32, \r4		; tab[15][i] = r1^r2^r4^r8
	eor		\r4, \r1
	std		y + 16, \r4		; tab[14][i] = r2^r4^r8
.endm

.macro FILL_LAST offset r1, r2, r4, r8
	;std		y + 00, \r1		; tab[1][i] = r1
	std		y + 16, \r2		; tab[2][i] = r2
	std		y + 32, \r2		; tab[3][i] = r2
	std		y + 48, \r4		; tab[4][i] = r4
	add		r28, RC			; &tab[5][i]
	std		y + 00,	\r4		; tab[5][i] = r1^r4
	mov		RT, \r2
	eor		RT, \r4			; r6 = r2^r4
	std		y + 16, RT		; tab[6][i] = r6
	std		y + 32, RT		; tab[7][i] = r1^r6;
	std		y + 48, \r8		; tab[8][i] = r8
	add		r28, RC			; &tab[9][i]
	std		y + 00,\r8		; tab[9][i] = r9
	eor		\r2, \r8
	std		y + 16,\r2		; tab[10][i] = r2^r8
	std		y + 32,\r2		; tab[11][i] = r2^r9
	eor		\r4, \r8
	std		y + 48,\r4		; tab[12][i] = r4^r8
	add		r28, RC			; &tab[13][i];
	std		y + 0, \r4		; tab[13][i] = r4^r9
	eor		RT, \r8
	std		y + 16, RT		; tab[14][i] = r6^r8
	std		y + 32, RT		; tab[15][i] = r6^r9
.endm

.macro RELIC_FILL_TABLE i, j
	/*
	 * We do not need to initialize the first row with zeroes, as the .data
	 * section is already initialized with null bytes.
	 */
	.if \i == 0
		ldi		r28, 16
		PREP_FIRST	r18, r19, r20, r21
		FILL_COLUMN	16, r18, r19, r20, r21
	.else
		.if \i >= 16
			ldi		r28, \i
			PREP_COLUMN	\i, r18, r19, r20, r21
			FILL_COLUMN	\i, r18, r19, r20, r21
		.else
			ldi		r28, \i + 16
			PREP_COLUMN	\i, r18, r19, r20, r21
			FILL_COLUMN	\i + 16, r18, r19, r20, r21
		.endif
	.endif
	.if \i < \j
		RELIC_FILL_TABLE \i + 1, \j
	.endif
.endm

.macro SHIFT i, offset
	swap	\i
	mov		RT, \i
	andi	RT, 0x0F
	eor		\i, RT
	eor		\i + 1, RT
	std		C_PTR + (\i + \offset + 1), \i + 1
.endm

.macro MUL0_STEP i
	.if \i == 16
		inc		r29
	.endif
	.if \i > 15
		ldd		\i, y + (\i - 16)
	.else
		ldd		\i, y + \i
	.endif
	.if \i < T_LINE - 1
		MUL0_STEP \i + 1
	.endif
.endm

.macro MUL_STEP i, j
	.if \j == 16
		inc		r29
	.endif
	.if \j == (T_LINE - 1)
		.if \j > 15
			ldd		\i, y + (\j - 16)
		.else
			ldd		\i, y + \j
		.endif
	.else
		.if \j > 15
			ldd		RT, y + (\j - 16)
		.else
			ldd		RT, y + \j
		.endif
		eor		\i, RT
	.endif
	.if \j < (T_LINE - 1)
		.if \i == (T_LINE - 1)
			MUL_STEP 0, \j + 1
		.else
			MUL_STEP \i + 1, \j + 1
		.endif
	.endif
.endm

.macro MULH i
	ld 		r28, A_PTR+
	andi	r28, 0xF0
	ldi		r29, hi8(fb_muln_table0)

	.if \i == 0
		MUL0_STEP 0

		swap	r0
		mov		RT, r0
		andi	RT, 0x0F
		mov		RC, RT
		eor		r0, RT

		std		C_PTR + 0, r0
	.else
		MUL_STEP \i, 0

		swap	\i
		mov		RT, \i
		andi	RT, 0x0F
		eor		\i, RT
		eor		\i, RC
		mov		RC, RT

		std		C_PTR + \i, \i
	.endif
.endm

.macro MULL i
	ld 		r28, A_PTR+
	andi	r28, 0x0F
	swap	r28
	ldi		r29, hi8(fb_muln_table0)

	.if \i == 0
		MUL0_STEP 0
	.else
		MUL_STEP \i, 0
	.endif

	ldd		RT, C_PTR + \i
	eor		\i, RT
	std		C_PTR + \i, \i
.endm

.macro PROLOGUE
	.irp i, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 28, 29
		push 	\i
	.endr
.endm

.macro EPILOGUE
	clr		r1
	.irp i, 29, 28, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 0
		pop 	\i
	.endr
	ret
.endm

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

.text

.global	fb_muln_low
.global fb_muli_low

#if FB_POLYN == 163

.macro RELIC_MULN_TABLE
	ldi		RC, 0x40
	ldi		r29, hi8(fb_muln_table0)
	RELIC_FILL_TABLE 0, 15
	ldi		r29, hi8(fb_muln_table1)
	RELIC_FILL_TABLE 16, 20
.endm

.macro MULN_DO
	.irp i, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
		MULH \i
	.endr

	swap	r18
	mov		RT, r18
	andi	RT, 0x0F
	eor		r18, RT
	std		C_PTR + (18 + T_LINE + 1), RT

	.irp i, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
		SHIFT \i, T_LINE
	.endr

	swap	r20
	mov		RT,r20
	andi	RT,0x0F
	eor		r20, RT
	eor		r0, RT
	std		C_PTR + 21, r0

	eor		r20, RC
	std		C_PTR + 20, r20

	sbiw	r26, 20		; x = &a

	.irp i, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20
		MULL \i
	.endr
.endm

#if FB_MUL == LODAH || !defined(STRIP)

fb_muln_low:
	PROLOGUE

	; C is stored on r25:r24
	movw	r26, r22		; copy a to x
	movw	r30, r20		; copy b to z

	RELIC_MULN_TABLE
	movw	r30, r24		; z = &c
	MULN_DO

	.irp i, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40
		ldd		RT, C_PTR + \i
		eor		RT, \i % T_LINE
		std		C_PTR + \i, RT
	.endr

	EPILOGUE
#endif

.macro R0 r0, r1, r2
	mov		RT, r19
	mov		\r1, RT
	lsl		RT
	lsl		RT
	lsl		RT
	eor		\r1, RT
	mov		RT, r19
	lsr		RT
	lsr		RT
	lsr		RT
	eor		\r1, RT

	swap	r19
	mov		RT, r19
	andi	RT, 0x0F
	mov		\r0, RT
	lsr		RT
	eor		\r0, RT

	andi	r19, 0xF0
	eor		\r1, r19

	lsl		r19
	mov		\r2, r19
.endm

.macro R r0, r1, r2
	mov		RT, r26
	eor		\r1, RT
	lsl		RT
	lsl		RT
	lsl		RT
	eor		\r1, RT
	mov		RT, r26
	lsr		RT
	lsr		RT
	lsr		RT
	eor		\r1, RT

	swap	r26
	mov		RT, r26
	andi	RT, 0x0F
	eor		\r0, RT
	lsr		RT
	eor		\r0, RT

	andi	r26, 0xF0
	eor		\r1, r26

	lsl		r26
	mov		\r2, r26
.endm

.macro STEP i, d1, d2
	mov		r26, \d1 - T_LINE
	.if \i == 1
		R r21, r22, r20
		ldd		RT, y + \d2
		eor		RT, r21
	.endif
	.if \i == 2
		R r22, r20, r21
		ldd		RT, y + \d2
		eor		RT, r22
	.endif
	.if \i == 3
		R r20, r21, r22
		ldd		RT, y + \d2
		eor		RT, r20
	.endif
	std		z + \d2, RT
.endm

#if FB_MUL == INTEG || !defined(STRIP)

fb_muli_low:
	PROLOGUE

	movw	r14, r22		; copy t to r15:r14
	movw	r26, r20		; copy a to x
	movw	r30, r18		; copy b to z

	RELIC_MULN_TABLE
	movw	r30, r14		; z = &t
	MULN_DO

	.irp i, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40
		ldd		RT, C_PTR + \i
		eor		\i % T_LINE, RT
	.endr

	movw	r28, r30			; y now points to t
	movw	r30, r24			; z now points to c

	R0		r21, r22, r20
	mov		RT, r0
	eor		RT, r21
	mov		r25, RT				; We cannot write to c[21].

	mov		r26, r18
	R		r22, r20, r21
	ldd		r18, y + 20
	eor		r18, r22

	STEP 3, 38, 19
	STEP 1, 37, 18
	STEP 2, 36, 17
	STEP 3, 35, 16
	STEP 1, 34, 15
	STEP 2, 33, 14
	STEP 3, 32, 13
	STEP 1, 31, 12
	STEP 2, 30, 11
	STEP 3, 29, 10
	STEP 1, 28, 9
	STEP 2, 27, 8
	STEP 3, 26, 7
	STEP 1, 25, 6
	STEP 2, 24, 5
	STEP 3, 23, 4
	STEP 1, 22, 3

	mov		r26, r25
	R		r22, r20, r21
	ldd		RT, y + 2
	eor		RT, r22
	std		z + 2, RT

	ldd 	r24, y + 1
	eor 	r24, r20		; r24 = m[1]

	ldd		r25, y + 0
	eor		r25, r21		; r25 = m[0]

	mov		r20, r18
	lsr		r20
	swap	r20
	andi	r20, 0xC0	; r20 = (t>>3)<<6

	mov		r19, r20
	lsl		r19			; r19 = (t>>3)<<7

	mov		r21, r18
	andi	r21, 0xF8		; r21 = (t>>3)<<3

	mov		r22, r18
	lsr		r22
	lsr		r22
	lsr		r22			; r22 = (t >> 3)

	eor		r22, r19
	eor		r22, r20
	eor		r22, r21

	eor		r25, r22
	std		z + 0, r25

	mov		r25, r18
	swap	r25
	andi	r25, 0x0F
	mov		RT, r25
	lsr		RT
	eor		RT, r25

	eor		r24, RT
	std		z + 1, r24

	andi	r18, 0x07
	std		z + 20, r18

	EPILOGUE
#endif

#endif
