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
#define T_LINE 16

/*
 * Size of the multiplication precomputation table in digits.
 */
#define T_DIGS	(16 * T_LINE)

/*
 * Precomputation table stored on a custom section (address must be terminated
 * in 0x00).
 */
.data
.align 2

.macro PROLOGUE
    .irp i, 4, 5, 6, 7, 8, 9, 10, 11
        push    r\i
    .endr
.endm

.macro RELIC_MULN_TABLE
    mov     r1, T_PTR
    RELIC_FILL_TABLE 0, 7 /* fill 8 columns */
    PREP_LAST   r6, r7, r8, r9 /* fill last column */
    FILL_COLUMN r6, r7, r8, r9
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
	clr     @T_PTR
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
     * r1 for the last digit is zero.
     */
    clr     @T_PTR
    mov     \r1, 1*2*T_LINE(T_PTR)  ; tab[1][i] = r1
    mov     \r2, 2*2*T_LINE(T_PTR)  ; tab[2][i] = r2
    mov     \r2, RT
    mov     RT, 3*2*T_LINE(T_PTR)   ; tab[3][i] = r1^r2
    mov     \r4, 4*2*T_LINE(T_PTR)  ; tab[4][i] = r4
    xor     \r4, RT
    mov     RT, 7*2*T_LINE(T_PTR)   ; tab[7][i] = r1^r2^r4
    mov     RT, 6*2*T_LINE(T_PTR)   ; tab[6][i] = r2^r4
    mov     \r4, 5*2*T_LINE(T_PTR)   ; tab[5][i] = r1^r4
    mov     \r8, 8*2*T_LINE(T_PTR)  ; tab[8][i] = r8
    mov     \r8, RT
    mov     RT, 9*2*T_LINE(T_PTR)   ; tab[9][i] = r1^r8
    xor     \r2, RT
    mov     RT, 11*2*T_LINE(T_PTR)  ; tab[11][i] = r1^r2^r8
    mov     RT, 10*2*T_LINE(T_PTR)  ; tab[10][i] = r2^r8
    xor     \r8, \r4
    mov     \r4, 12*2*T_LINE(T_PTR); tab[12][i] = r4^r8
    mov     \r4, 13*2*T_LINE(T_PTR); tab[13][i] = r1^r4^r8
    xor     \r2, \r4
    mov     \r4, 15*2*T_LINE(T_PTR); tab[15][i] = r1^r2^r4^r8
    mov     \r4, 14*2*T_LINE(T_PTR); tab[14][i] = r2^r4^r8
.endm

.macro EPILOGUE
	.irp i, 11, 10, 9, 8, 7, 6, 5, 4
		pop 	r\i
	.endr
	ret
.endm

.text
.align 2

.global fb_muln_low

#define TEMP_SIZE 192
#define RELIC_TABLE_SIZE 512

fb_muln_low:
    /* r15: c
       r14: a
       r13: b
    */
    //pushes
	PROLOGUE

	clr 2*23(r14)
	clr 2*23(r13)

    //allocate temp area
	sub #TEMP_SIZE,r1

	//compute t0(8) = a0(8) + a1(8)
	//compute t1(8) = a1(8) + a2(8)
	//compute t2(8) = a0(8) + a1(8) + a2(8)
	mov r14,r11
	mov r14,r10
	add #2*8,r11
	add #2*16,r10
	//r14: a0 / r11: a1 / r10: a2
	.irp i, 0, 1, 2, 3, 4, 5, 6, 7
    	mov @r14+,r12 //a0
    	mov r12,r9
    	xor @r11+,r12 //a0 + a1
    	mov r12,(2 * \i)(r1)
    	xor @r10+,r12 //a0 + a1 + a2
    	mov r12,(2 * (\i + 16))(r1)
    	xor r9,r12 //a1 + a2
    	mov r12,(2 * (\i + 8))(r1)
    .endr

	//compute t3(8) = b0(8) + b1(8)
	//compute t4(8) = b1(8) + b2(8)
	//compute t5(8) = b0(8) + b1(8) + b2(8)
	mov r13,r11
	mov r13,r10
	add #2*8,r11
	add #2*16,r10
	//r14: a0 / r11: a1 / r10: a2
	.irp i, 0, 1, 2, 3, 4, 5, 6, 7
    	mov @r13+,r12 //a0
    	mov r12,r9
    	xor @r11+,r12 //a0 + a1
    	mov r12,(2 * (\i + 24))(r1)
    	xor @r10+,r12 //a0 + a1 + a2
    	mov r12,(2 * (\i + 40))(r1)
    	xor r9,r12 //a1 + a2
    	mov r12,(2 * (\i + 32))(r1)
    .endr

    //restore pointers
    sub #2*8,r14
    sub #2*8,r13

    //compute c01 = a0b0 (v0)
    push r13
    call #fb_muln_low_karatsuba

    //compute c45 = a2b2 (v2)
    add #2*16,r14
    mov @r1,r13
    add #2*16,r13
    add #2*32,r15
    call #fb_muln_low_karatsuba

    //compute t67 = a1b1 (v1)
    pop r13
    //restore and push c
    sub #2*(4*8),r15
    push r15
    sub #2*8,r14
    add #2*8,r13
    mov r1,r15
    add #2*(6*8+1),r15
    call #fb_muln_low_karatsuba

    //compute t89 = t0t3 (v3)
    add #2*(2*8),r15
    mov r15,r14
    sub #2*(8*8),r14
    mov r15,r13
    sub #2*(5*8),r13
    call #fb_muln_low_karatsuba

    //compute t10,11 = t1t4 (v4)
    add #2*(2*8),r15
    mov r15,r14
    sub #2*(9*8),r14
    mov r15,r13
    sub #2*(6*8),r13
    call #fb_muln_low_karatsuba

    //compute c23 = t2t5 (v5)
    mov r15,r14
    sub #2*(8*8),r14
    mov r15,r13
    sub #2*(5*8),r13
    pop r15
    add #2*(2*8),r15
    call #fb_muln_low_karatsuba

    //restore c to c1
    sub #2*(1*8),r15

    //add what's left to c2
    //move c1 (v0H) to regs
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        mov @r15+,\i
    .endr
    //c2' += t7 (v1H)
    mov r1,r14
    add #2*(7*8),r14
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        xor @r14+,\i
    .endr
    //c2' += t8 (v3L)
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        xor @r14+,\i
    .endr
    //c2' += t9 (v3H)
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        xor @r14+,\i
    .endr
    //c2' += t10 (v4L)
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        xor @r14+,\i
    .endr
    //c2 += c2'
    xor r4,(2*0)(r15)
    xor r5,(2*1)(r15)
    xor r6,(2*2)(r15)
    xor r7,(2*3)(r15)
    xor r8,(2*4)(r15)
    xor r9,(2*5)(r15)
    xor r10,(2*6)(r15)
    xor r11,(2*7)(r15)

    //add what's left to c1
    //move c0 (v0L) to regs
    sub #2*(2*8),r15
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        mov @r15+,\i
    .endr
    //c1' += t6 (v1L)
    sub #2*(5*8),r14
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        xor @r14+,\i
    .endr
    //c1' += t8 (v3L)
    add #2*(1*8),r14
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        xor @r14+,\i
    .endr
    //c1 += c1'
    xor r4,(2*0)(r15)
    xor r5,(2*1)(r15)
    xor r6,(2*2)(r15)
    xor r7,(2*3)(r15)
    xor r8,(2*4)(r15)
    xor r9,(2*5)(r15)
    xor r10,(2*6)(r15)
    xor r11,(2*7)(r15)

    //add what's left to c3
    //move t9 (v3H) to regs
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        mov @r14+,\i
    .endr
    //c3' += t11 (v4H)
    add #2*(1*8),r14
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        xor @r14+,\i
    .endr
    sub #2*(6*8),r14
    //c3' += t6 (v1L)
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        xor @r14+,\i
    .endr
    //c3' += c4 (v2L)
    add #2*(3*8),r15
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        xor @r15+,\i
    .endr
    add #2*(3*8),r14
    //c3' += t10 (v4L)
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        xor @r14+,\i
    .endr
    //c3 += c3'
    sub #2*(2*8),r15
    xor r4,(2*0)(r15)
    xor r5,(2*1)(r15)
    xor r6,(2*2)(r15)
    xor r7,(2*3)(r15)
    xor r8,(2*4)(r15)
    xor r9,(2*5)(r15)
    xor r10,(2*6)(r15)
    xor r11,(2*7)(r15)

    //add what's left to c4
    //move t7 (v1H) to regs
    sub #2*(4*8),r14
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        mov @r14+,\i
    .endr
    //c4' += c5 (v2H)
    add #2*(2*8),r15
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        xor @r15+,\i
    .endr
    //c4' += t11 (v4H)
    add #2*(3*8),r14
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        xor @r14+,\i
    .endr
    //c4 += c4'
    sub #2*(2*8),r15
    xor r4,(2*0)(r15)
    xor r5,(2*1)(r15)
    xor r6,(2*2)(r15)
    xor r7,(2*3)(r15)
    xor r8,(2*4)(r15)
    xor r9,(2*5)(r15)
    xor r10,(2*6)(r15)
    xor r11,(2*7)(r15)

    skip:

    add #TEMP_SIZE,r1
    //pops
	EPILOGUE

fb_muln_low_karatsuba:

    sub #RELIC_TABLE_SIZE,r1

    RELIC_MULN_TABLE

#include "fb_mul_353_ld_k3.inc"

    add #RELIC_TABLE_SIZE,r1

    ret

