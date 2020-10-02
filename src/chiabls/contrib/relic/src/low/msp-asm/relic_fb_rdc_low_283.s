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
 * @ingroup fb
 */

#include "relic_fb_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

.global fb_rdcn_low

.text
.align 2

.macro REDC_COL i
    //reduce into a[i]
    //r8: carry
    //r9 pending a[i+1] write

    .if \i > 0
        mov (2 * (17 + \i))(r14),r10
    .else
        mov (2 * (17 + \i))(r15),r10
        and #0x7FF,(2 * (17 + \i))(r15)
        and #0xF800,r10
    .endif

    mov r10,r13
    clr r12
    rla r13
    rlc r12

    .if \i < 18
        xor r12,r9
        //write a[i+1]
        .if \i == 17
            mov r9,(2 * (\i + 1))(r14)
        .else
            mov r9,(2 * (\i + 1))(r15)
        .endif
    .endif
    mov r13,r9

    //r9: a[i]
    xor (2 * \i)(r14),r9

    //"carry"
    .if \i < 18
        xor r8,r9
    .endif

    .if \i > 0
        mov r10,r11
        //clear high byte
        mov.b r10,r10
        swpb r10
        swpb r11
        mov.b r11,r11

        mov r10,r12
        mov r11,r13

        clrc
        rrc r11
        rrc r10
        rrc r11
        rrc r10
        rrc r11
        rrc r10
        // << 8 >> 3 (<< 5)
        xor r11,r9
        mov r10,r8

        rla r12
        rlc r13
        rla r12
        rlc r13
        // << 8 << 2 (<< 10)
        xor r13,r9
        xor r12,r8

        rla r12
        rlc r13
        rla r12
        rlc r13
        // << 8 << 4 (<< 12)
        xor r13,r9
        xor r12,r8
    .else
        //clear high byte
        swpb r10
        mov.b r10,r10

        mov r10,r13

        clrc
        rrc r10
        rrc r10
        rrc r10
        // << 8 >> 3 (<< 5)
        xor r10,r9

        rla r13
        rla r13
        // << 8 << 2 (<< 10)
        xor r13,r9

        rla r13
        rla r13
        // << 8 << 4 (<< 12)
        xor r13,r9
    .endif
.endm

fb_rdcn_low:
    /* r15: c
       r14: a
    */
    push r8
    push r9
    push r10
    push r11

    REDC_COL 18
    REDC_COL 17
    REDC_COL 16
    REDC_COL 15
    REDC_COL 14
    REDC_COL 13
    REDC_COL 12
    REDC_COL 11
    REDC_COL 10
    REDC_COL 9
    REDC_COL 8
    REDC_COL 7
    REDC_COL 6
    REDC_COL 5
    REDC_COL 4
    REDC_COL 3
    REDC_COL 2
    REDC_COL 1
    REDC_COL 0

    mov r9,@r15

    pop r11
    pop r10
    pop r9
    pop r8
	ret
