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
 * Binary field multiplication functions.
 *
 * @ingroup fb
 */

#include "relic_fb_low.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#define KARATSUBA

/*
 * Pointer register which points to the parameter a.
 */
#define A_PTR		r14

/*
 * Pointer register which points to the parameter b.
 */
#define B_PTR		r13

/*
 * Pointer register which points to the parameter c.
 */
#define C_PTR		r15

/*
 * Temporary register.
 */
#define RT			r4

/*
 * Pointer to the table.
 */
#define T_PTR		r12

/*
 * Size in bits of the multiplication of a binary field element by a polynomial
 * with degree lower than 4.
 */
#define POLY_M4		(FB_POLYN + 4)

/*
 * Size in digits of a precomputation table line.
 */
#ifdef KARATSUBA
#define T_LINE 16
#elif (POLY_M4 % WSIZE) > 0
#define T_LINE	(POLY_M4/WSIZE + 1)
#else
#define T_LINE	(POLY_M4/WSIZE)
#endif

/*
 * Size of the multiplication precomputation table in digits.
 */
#define T_DIGS	(16 * T_LINE)

.macro PROLOGUE
    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11
.endm

.macro RELIC_MULN_TABLE
    mov     r1, T_PTR
    RELIC_FILL_TABLE 0, 16 /* fill 17 columns */
    PREP_LAST   r6, r7, r8, r9 /* fill last column */
    FILL_COLUMN r6, r7, r8, r9
    sub #2*T_LINE, T_PTR /* restore T_PTR */
    sub #2*T_LINE, B_PTR /* restore B_PTR */
.endm

.macro RELIC_MULN_TABLE_KARATSUBA
    mov     r1, T_PTR
    RELIC_FILL_TABLE 0, 11 /* fill 12 columns */
    sub #2*T_LINE, T_PTR /* restore T_PTR */
    sub #2*T_LINE, B_PTR /* restore B_PTR */
.endm

.macro RELIC_FILL_TABLE i, j
    .if \i == 0
        PREP_FIRST  r6, r7, r8, r9
        FILL_COLUMN r6, r7, r8, r9
    .else
        PREP_COLUMN \i + \i, r6, r7, r8, r9
        FILL_COLUMN r6, r7, r8, r9
    .endif
    incd T_PTR
    .if \i < \j
        RELIC_FILL_TABLE \i + 1, \j
    .endif
.endm

/*
 * Prepares the first column values.
 */
.macro PREP_FIRST r1, r2, r4, r8
	mov		@B_PTR+, \r1	; r1 = b[0]
	mov		\r1, \r2
	rla		\r2				; r2 = r1 << 1
	mov		\r2, \r4
	rla		\r4				; r4 = r1 << 2
	mov 	\r4, \r8
	rla		\r8				; r8 = r1 << 3
.endm

/*
 * Prepares the values for column i.
 */
.macro PREP_COLUMN i, r1, r2, r4, r8
	mov		\r1, RT

	mov		@B_PTR+, \r1
	mov		\r1, \r2
	rla		RT
	rlc		\r2
	mov		\r2, \r4
	rla 	RT
	rlc		\r4
	mov		\r4, \r8
	rla		RT
	rlc		\r8
.endm

/*
 * Prepares the values for the last column.
 */
.macro PREP_LAST r1, r2, r4, r8
    mov     \r1, RT

    clr     \r1
    mov     \r1, \r2
    rla     RT
    rlc     \r2
    mov     \r2, \r4
    rla     RT
    rlc     \r4
    mov     \r4, \r8
    rla     RT
    rlc     \r8
.endm

/*
 * Fills a column of the precomputation table.
 */
.macro FILL_COLUMN r1, r2, r4, r8
	clr     0(T_PTR)
	mov     \r1, 1*2*T_LINE(T_PTR)	; tab[1][i] = r1
	mov     \r2, 2*2*T_LINE(T_PTR)	; tab[2][i] = r2
	mov     \r1, RT
	xor     \r2, RT
	mov		RT, 3*2*T_LINE(T_PTR)	; tab[3][i] = r1^r2
	mov		\r4, 4*2*T_LINE(T_PTR)	; tab[4][i] = r4
	xor		\r4, RT
	mov		RT, 7*2*T_LINE(T_PTR)	; tab[7][i] = r1^r2^r4
	xor		\r1, RT
	mov		RT, 6*2*T_LINE(T_PTR)	; tab[6][i] = r2^r4
	mov		\r1, RT
	xor		\r4, RT
	mov		RT, 5*2*T_LINE(T_PTR)	; tab[5][i] = r1^r4
	mov		\r8, 8*2*T_LINE(T_PTR)	; tab[8][i] = r8
	mov		\r1, RT
	xor		\r8, RT
	mov		RT, 9*2*T_LINE(T_PTR)	; tab[9][i] = r1^r8
	xor		\r2, RT
	mov		RT, 11*2*T_LINE(T_PTR)	; tab[11][i] = r1^r2^r8
	xor		\r1, RT
	mov		RT, 10*2*T_LINE(T_PTR)	; tab[10][i] = r2^r8
	xor		\r8, \r4
	mov		\r4, 12*2*T_LINE(T_PTR); tab[12][i] = r4^r8
	xor		\r1, \r4
	mov		\r4, 13*2*T_LINE(T_PTR); tab[13][i] = r1^r4^r8
	xor		\r2, \r4
	mov		\r4, 15*2*T_LINE(T_PTR); tab[15][i] = r1^r2^r4^r8
	xor		\r1, \r4
	mov		\r4, 14*2*T_LINE(T_PTR); tab[14][i] = r2^r4^r8
.endm

.macro FILL_LAST r1, r2, r4, r8
    /*
     * Since the last bit of the 17th digit of a number is 0, then
     * r1 and r2 for the 18th digit are zero.
     */
    clr     @T_PTR
    clr     1*2*T_LINE(T_PTR)  ; tab[1][i] = r1
    clr     2*2*T_LINE(T_PTR)  ; tab[2][i] = r2
    clr     3*2*T_LINE(T_PTR)   ; tab[3][i] = r1^r2
    mov     \r4, 4*2*T_LINE(T_PTR)  ; tab[4][i] = r4
    mov     \r4, 7*2*T_LINE(T_PTR)   ; tab[7][i] = r1^r2^r4
    mov     \r4, 6*2*T_LINE(T_PTR)   ; tab[6][i] = r2^r4
    mov     \r4, 5*2*T_LINE(T_PTR)   ; tab[5][i] = r1^r4
    mov     \r8, 8*2*T_LINE(T_PTR)  ; tab[8][i] = r8
    mov     \r8, 9*2*T_LINE(T_PTR)   ; tab[9][i] = r1^r8
    mov     \r8, 11*2*T_LINE(T_PTR)  ; tab[11][i] = r1^r2^r8
    mov     \r8, 10*2*T_LINE(T_PTR)  ; tab[10][i] = r2^r8
    xor     \r8, \r4
    mov     \r4, 12*2*T_LINE(T_PTR); tab[12][i] = r4^r8
    mov     \r4, 13*2*T_LINE(T_PTR); tab[13][i] = r1^r4^r8
    mov     \r4, 15*2*T_LINE(T_PTR); tab[15][i] = r1^r2^r4^r8
    mov     \r4, 14*2*T_LINE(T_PTR); tab[14][i] = r2^r4^r8
.endm

.macro EPILOGUE
    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
	ret
.endm

.text
.align 2

.global fb_muln_low

#ifndef KARATSUBA

fb_muln_low:
    /* r15: c
       r14: a
       r13: b
    */
    /* pushes */
	PROLOGUE

	sub #576,r1

	RELIC_MULN_TABLE

#include "ld_mult_271.inc"

    add #576,r1

    /* pops */
	EPILOGUE

#else

#define TEMP_SIZE 96
#define RELIC_TABLE_SIZE 512

fb_muln_low:
    /* r15: c
       r14: a
       r13: b
    */
    //pushes
	PROLOGUE

    //allocate temp area
	sub #TEMP_SIZE,r1

	//compute t0(12) = a0(11) + a1(12)
	mov r14,r11
	add #2*11,r11
	mov @r14+,r12
	xor @r11+,r12
	mov r12,(2 * 0)(r1)
	mov @r14+,r12
	xor @r11+,r12
	mov r12,(2 * 1)(r1)
	mov @r14+,r12
	xor @r11+,r12
	mov r12,(2 * 2)(r1)
	mov @r14+,r12
	xor @r11+,r12
	mov r12,(2 * 3)(r1)
	mov @r14+,r12
	xor @r11+,r12
	mov r12,(2 * 4)(r1)
	mov @r14+,r12
	xor @r11+,r12
	mov r12,(2 * 5)(r1)
	mov @r14+,r12
	xor @r11+,r12
	mov r12,(2 * 6)(r1)
	mov @r14+,r12
	xor @r11+,r12
	mov r12,(2 * 7)(r1)
	mov @r14+,r12
	xor @r11+,r12
	mov r12,(2 * 8)(r1)
	mov @r14+,r12
	xor @r11+,r12
	mov r12,(2 * 9)(r1)
	mov @r14+,r12
	xor @r11+,r12
	mov r12,(2 * 10)(r1)
    mov @r11,(2 * 11)(r1)

    //compute t1(12) = b0(11) + b1(12)
    mov r13,r11
    add #2*11,r11
    mov @r13+,r12
    xor @r11+,r12
    mov r12,(2 * 12)(r1)
    mov @r13+,r12
    xor @r11+,r12
    mov r12,(2 * 13)(r1)
    mov @r13+,r12
    xor @r11+,r12
    mov r12,(2 * 14)(r1)
    mov @r13+,r12
    xor @r11+,r12
    mov r12,(2 * 15)(r1)
    mov @r13+,r12
    xor @r11+,r12
    mov r12,(2 * 16)(r1)
    mov @r13+,r12
    xor @r11+,r12
    mov r12,(2 * 17)(r1)
    mov @r13+,r12
    xor @r11+,r12
    mov r12,(2 * 18)(r1)
    mov @r13+,r12
    xor @r11+,r12
    mov r12,(2 * 19)(r1)
    mov @r13+,r12
    xor @r11+,r12
    mov r12,(2 * 20)(r1)
    mov @r13+,r12
    xor @r11+,r12
    mov r12,(2 * 21)(r1)
    mov @r13+,r12
    xor @r11+,r12
    mov r12,(2 * 22)(r1)
    mov @r11,(2 * 23)(r1)

    //restore pointers
    sub #2*11,r14
    sub #2*11,r13

    //compute c01 = a0b0
    push r13
    push 2*11(r14)
    push 2*11(r13)
    clr 2*11(r14)
    clr 2*11(r13)
    call #fb_muln_low_karatsuba
    mov 4(r1),r13
    pop 2*11(r13)
    pop 2*11(r14)

    //compute c23 = a1b1
    add #2*11,r14
    mov @r1,r13
    add #2*11,r13
    add #2*22,r15
    call #fb_muln_low_karatsuba
    //"pop r13"
    add #2,r1
    //restore and push c
    sub #2*22,r15
    push r15
    //compute t23 = t0t1
    mov r1,r15
    mov r1,r14
    mov r1,r13
    add #2*24+2,r15
    add #2,r14
    add #2*12+2,r13
    call #fb_muln_low_karatsuba

    //pop c
    pop r15

    //r14 = &t23
    mov r1,r14
    add #2*24,r14

    //compute t23 += c01 (a0b0)
    xor @r15+,(2 * 0)(r14)
    xor @r15+,(2 * 1)(r14)
    xor @r15+,(2 * 2)(r14)
    xor @r15+,(2 * 3)(r14)
    xor @r15+,(2 * 4)(r14)
    xor @r15+,(2 * 5)(r14)
    xor @r15+,(2 * 6)(r14)
    xor @r15+,(2 * 7)(r14)
    xor @r15+,(2 * 8)(r14)
    xor @r15+,(2 * 9)(r14)
    xor @r15+,(2 * 10)(r14)
    xor @r15+,(2 * 11)(r14)
    xor @r15+,(2 * 12)(r14)
    xor @r15+,(2 * 13)(r14)
    xor @r15+,(2 * 14)(r14)
    xor @r15+,(2 * 15)(r14)
    xor @r15+,(2 * 16)(r14)
    xor @r15+,(2 * 17)(r14)
    xor @r15+,(2 * 18)(r14)
    xor @r15+,(2 * 19)(r14)
    xor @r15+,(2 * 20)(r14)
    xor @r15+,(2 * 21)(r14)

    //compute t23 += c23 (a1b1)
    xor @r15+,(2 * 0)(r14)
    xor @r15+,(2 * 1)(r14)
    xor @r15+,(2 * 2)(r14)
    xor @r15+,(2 * 3)(r14)
    xor @r15+,(2 * 4)(r14)
    xor @r15+,(2 * 5)(r14)
    xor @r15+,(2 * 6)(r14)
    xor @r15+,(2 * 7)(r14)
    xor @r15+,(2 * 8)(r14)
    xor @r15+,(2 * 9)(r14)
    xor @r15+,(2 * 10)(r14)
    xor @r15+,(2 * 11)(r14)
    xor @r15+,(2 * 12)(r14)
    xor @r15+,(2 * 13)(r14)
    xor @r15+,(2 * 14)(r14)
    xor @r15+,(2 * 15)(r14)
    xor @r15+,(2 * 16)(r14)
    xor @r15+,(2 * 17)(r14)
    xor @r15+,(2 * 18)(r14)
    xor @r15+,(2 * 19)(r14)
    xor @r15+,(2 * 20)(r14)
    xor @r15+,(2 * 21)(r14)
    xor @r15+,(2 * 22)(r14)
    xor @r15+,(2 * 23)(r14)

    //set r15 from &c4 to &c1
    sub #2*(22+24-11),r15

    //compute c12 += t23
    xor @r14+,(2 * 0)(r15)
    xor @r14+,(2 * 1)(r15)
    xor @r14+,(2 * 2)(r15)
    xor @r14+,(2 * 3)(r15)
    xor @r14+,(2 * 4)(r15)
    xor @r14+,(2 * 5)(r15)
    xor @r14+,(2 * 6)(r15)
    xor @r14+,(2 * 7)(r15)
    xor @r14+,(2 * 8)(r15)
    xor @r14+,(2 * 9)(r15)
    xor @r14+,(2 * 10)(r15)
    xor @r14+,(2 * 11)(r15)
    xor @r14+,(2 * 12)(r15)
    xor @r14+,(2 * 13)(r15)
    xor @r14+,(2 * 14)(r15)
    xor @r14+,(2 * 15)(r15)
    xor @r14+,(2 * 16)(r15)
    xor @r14+,(2 * 17)(r15)
    xor @r14+,(2 * 18)(r15)
    xor @r14+,(2 * 19)(r15)
    xor @r14+,(2 * 20)(r15)
    xor @r14+,(2 * 21)(r15)
    xor @r14+,(2 * 22)(r15)
    xor @r14+,(2 * 23)(r15)

    skip:

    add #TEMP_SIZE,r1
    //pops
	EPILOGUE

fb_muln_low_karatsuba:

    sub #RELIC_TABLE_SIZE,r1

    RELIC_MULN_TABLE_KARATSUBA

#include "fb_mul_353_ld_k2.inc"

    add #RELIC_TABLE_SIZE,r1

    ret

#endif
