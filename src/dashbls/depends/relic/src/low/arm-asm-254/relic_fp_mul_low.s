/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2015 RELIC Authors
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

#include "relic_fp_low.h"

.syntax unified
.arch armv7-a

.global fp_muln_low

/*============================================================================*/
/* Uso dos registradores                                                      */
/* r0 = C                                                                      */
/* r1 = A                                                                        */
/* r2 = B                                                                        */
/* r3 = ACC_2                                                                    */
/* r4 = ACC_1                                                                    */
/* r5 = ACC_0                                                                    */
/* r6 = Ra[0]                                                                    */
/* r7 = Ra[1]                                                                    */
/* r8 = Ra[2]                                                                    */
/* r9 = Rb[0]                                                                    */
/* r10 = Rb[1]                                                                    */
/* r11 = Rb[2]                                                                    */
/* r12 = aux                                                                    */
/* r13 = sp                                                                        */
/* r14 = lr                                                                        */
/* r15 = pc                                                                        */
/*============================================================================*/

fp_muln_low:
    STMDB sp!, {r4-r12, r14}

    PLD    [r1, #32]
    PLD    [r2, #32]

/********************************************************** B_init ***********************************************************************/

    LDR r6,  [r1, #(4*6)]
    LDR r9,  [r2, #(4*0)]
        MOV r5, #0

    /* COMBA_STEP_1 r6 r9 */
    UMULL r5, r4, r6, r9
        MOV r3, #0
        LDR r10, [r2, #(4*1)]
    /* COMBA_MFIN_1 6 */
    STR r5, [r0, #(4*6)]
    MOV r5, #0

    /* COMBA_STEP_2 r6 r10 */
    UMLAL r4, r5, r6, r10
        LDR r7,  [r1, #(4*7)]
        MOV r12, #0
    ADDS r3, r3, r5
    ADC r5, r12, #0
    /* COMBA_STEP_2 r7 r9 */
    UMLAL r4, r12, r7, r9
        MOV r14, #0
        LDR r6,  [r1, #(4*3)]
        LDR r9,  [r2, #(4*0)]
    ADDS r3, r3, r12
/* ADC r5, r5, #0 */
    /* COMBA_MFIN_2 7 */

    /* COMBA_STEP_3 r7 r10 */
    UMLAL r3, r14, r7, r10
        STR r4, [r0, #(4*7)]
        MOV r4, #0
        MOV r12, #0
    ADCS r5, r5, r14
/* ADC r4, r4, #0 */
    /* COMBA_MFIN_3 8 */

    STR r5, [r0, #(4*9)]
    MOV r5, #0
/********************************************************** r0 - Part 1 ***********************************************************************/

    /* COMBA_STEP_1 r6 r9 */
    UMULL r5, r12, r6, r9
        STR r3, [r0, #(4*8)]
        MOV r3, #0
        LDR r7,  [r1, #(4*4)]
        LDR r10, [r2, #(4*1)]
        MOV r14, #0
    ADCS r4, r4, r12
/* ADC r3, r3, #0 */
    /* COMBA_MFIN_1 3 */

    /* COMBA_STEP_2 r7 r9 */
    UMLAL r4, r14, r7, r9
        STR r5, [r0, #(4*3)]
        MOV r5, #0
        MOV r12, #0
    ADCS r3, r3, r14
    ADC r5, r5, #0
    /* COMBA_STEP_2 r6 r10 */
    UMLAL r4, r12, r6, r10
        MOV r14, #0
        LDR r8,  [r1, #(4*5)]
        LDR r11, [r2, #(4*2)]
    ADDS r3, r3, r12
/* ADC r5, r5, #0 */
    /* COMBA_MFIN_2 4 */

    /* COMBA_STEP_3 r8 r9 */
    UMLAL r3, r14, r8, r9
        STR r4, [r0, #(4*4)]
        MOV r4, #0
        MOV r12, #0
    ADCS r5, r5, r14
    ADC r4, r4, #0
    /* COMBA_STEP_3 r7 r10 */
    UMLAL r3, r12, r7, r10
        MOV r14, #0
    ADDS r5, r5, r12
    ADC r4, r4, #0
    /* COMBA_STEP_3 r6 r11 */
    UMLAL r3, r14, r6, r11
        MOV r12, #0
    ADDS r5, r5, r14
/* ADC r4, r4, #0 */
    /* COMBA_MFIN_3 5 */

/********************************************************** r0 - Part 2 ***********************************************************************/

    /* COMBA_STEP_1 r8 r10 */
    UMLAL r5, r12, r8, r10
        STR r3, [r0, #(4*5)]
        MOV r3, #0
        MOV r14, #0
    ADCS r4, r4, r12
    ADC r3, r3, #0
    /* COMBA_STEP_1 r7 r11 */
    UMLAL r5, r14, r7, r11
        LDR r9,  [r2, #(4*3)]
        MOV r12, #0
    ADDS r4, r4, r14
    ADC r3, r3, #0
    /* COMBA_STEP_1 r6 r9 */
    UMLAL r5, r12, r6, r9
        MOV r14, #0
        LDR  r10, [r0, #(4*6)]
/*
ADDS r4, r4, r12
ADC r3, r3, #0
*/
    /* COMBA_ADD_1 6 */
    ADDS r5, r5, r10
    ADCS r4, r4, r12
/* ADC  r3, r3, #0 */
    /* COMBA_MFIN_1 6 */
    STR r5, [r0, #(4*6)]

    /* COMBA_STEP_2 r8 r11 */
    UMLAL r4, r14, r8, r11
        MOV r5, #0
        LDR r10,  [r2, #(4*4)]
        MOV r12, #0
    ADCS r3, r3, r14
    ADC r5, r5, #0
    /* COMBA_STEP_2 r7 r9 */
    UMLAL r4, r12, r7, r9
        MOV r14, #0
    ADDS r3, r3, r12
    ADC r5, r5, #0
    /* COMBA_STEP_2 r6 r10 */
    UMLAL r4, r14, r6, r10
        MOV r12, #0
        LDR  r6, [r0, #(4*7)]
/*
    ADDS r3, r3, r14
    ADC r5, r5, #0
*/
    /* COMBA_ADD_2 7 */
    ADDS r4, r4, r6
    ADCS r3, r3, r14
/* ADC  r5, r5, #0 */
    /* COMBA_MFIN_2 7 */
    STR r4, [r0, #(4*7)]

/********************************************************** r0 - Part 3 ***********************************************************************/

    /* COMBA_STEP_3 r8 r9 */
    UMLAL r3, r12, r8, r9
        MOV r4, #0
        LDR r6,  [r1, #(4*6)]
        MOV r14, #0
    ADCS r5, r5, r12
    ADC r4, r4, #0
    /* COMBA_STEP_3 r6 r11 */
    UMLAL r3, r14, r6, r11
        MOV r12, #0
    ADDS r5, r5, r14
    ADC r4, r4, #0
    /* COMBA_STEP_3 r7 r10 */
    UMLAL r3, r12, r7, r10
        MOV r14, #0
        LDR  r7, [r0, #(4*8)]
/*
    ADDS r5, r5, r12
    ADC r4, r4, #0
*/
    /* COMBA_ADD_3 8 */
    ADDS r3, r3, r7
    ADCS r5, r5, r12
/* ADC  r4, r4, #0 */

    /* COMBA_MFIN_3 8 */
    STR r3, [r0, #(4*8)]

    /* COMBA_STEP_1 r6 r9 */
    UMLAL r5, r14, r6, r9
        MOV r3, #0
        LDR r7,  [r1, #(4*7)]
        MOV r12, #0
    ADCS r4, r4, r14
    ADC r3, r3, #0
    /* COMBA_STEP_1 r7 r11 */
    UMLAL r5, r12, r7, r11
        MOV r14, #0
    ADDS r4, r4, r12
    ADC r3, r3, #0
    /* COMBA_STEP_1 r8 r10 */
    UMLAL r5, r14, r8, r10
        MOV r12, #0
    ADDS r4, r4, r14
    ADC r3, r3, #0
    /* COMBA_ADD_1 9 */
    LDR  r14, [r0, #(4*9)]
    ADDS r5, r5, r14
    ADCS r4, r4, #0
/* ADC  r3, r3, #0 */
    /* COMBA_MFIN_1 9 */
    STR r5, [r0, #(4*9)]

/********************************************************** r0 - Part 4 ***********************************************************************/

    /* COMBA_STEP_2 r7 r9 */
    UMLAL r4, r12, r7, r9
        MOV r5, #0
        MOV r14, #0
    ADCS r3, r3, r12
    ADC r5, r5, #0
    /* COMBA_STEP_2 r6 r10 */
    UMLAL r4, r14, r6, r10
        MOV r12, #0
    ADDS r3, r3, r14
/* ADC r5, r5, #0 */
    /* COMBA_MFIN_2 10 */
    STR r4, [r0, #(4*10)]

    /* COMBA_STEP_3 r7 r10 */
    UMLAL r3, r12, r7, r10
        MOV r4, #0
        MOV r14, #0
        LDR r6,  [r1, #(4*0)]
        LDR r9,  [r2, #(4*0)]
    ADCS r5, r5, r12
/* ADC r4, r4, #0 */
    /* COMBA_MFIN_3 11 */
    STR r5, [r0, #(4*12)]
    MOV r5, #0

/********************************************************** r1 - Part 1 ***********************************************************************/

    /* COMBA_STEP_1 r6 r9 */
    UMULL r5, r14, r6, r9
        STR r3, [r0, #(4*11)]
        MOV r3, #0
        LDR r7,  [r1, #(4*1)]
        MOV r12, #0
    ADCS r4, r4, r14
/* ADC r3, r3, #0 */
    /* COMBA_MFIN_1 0 */
    STR r5, [r0, #(4*0)]

    /* COMBA_STEP_2 r7 r9 */
    UMLAL r4, r12, r7, r9
        MOV r5, #0
        LDR r10, [r2, #(4*1)]
        MOV r14, #0
    ADCS r3, r3, r12
    ADC r5, r5, #0
    /* COMBA_STEP_2 r6 r10 */
    UMLAL r4, r14, r6, r10
        MOV r12, #0
        LDR r8,  [r1, #(4*2)]
        LDR r11, [r2, #(4*2)]
    ADDS r3, r3, r14
/* ADC r5, r5, #0 */

    /* COMBA_MFIN_2 1 */
    STR r4, [r0, #(4*1)]

    /* COMBA_STEP_3 r8 r9 */
    UMLAL r3, r12, r8, r9
        MOV r4, #0
        MOV r14, #0
    ADCS r5, r5, r12
    ADC r4, r4, #0
    /* COMBA_STEP_3 r7 r10 */
    UMLAL r3, r14, r7, r10
        MOV r12, #0
    ADDS r5, r5, r14
    ADC r4, r4, #0
    /* COMBA_STEP_3 r6 r11 */
    UMLAL r3, r12, r6, r11
        MOV r14, #0
    ADDS r5, r5, r12
/* ADC r4, r4, #0 */
    /* COMBA_MFIN_3 2 */
    STR r3, [r0, #(4*2)]

/********************************************************** r1 - Part 2 ***********************************************************************/

    /* COMBA_STEP_1 r8 r10 */
    UMLAL r5, r14, r8, r10
        MOV r3, #0
        LDR r9,  [r2, #(4*3)]
        MOV r12, #0
    ADCS r4, r4, r14
    ADC r3, r3, #0
    /* COMBA_STEP_1 r7 r11 */
    UMLAL r5, r12, r7, r11
        MOV r14, #0
    ADDS r4, r4, r12
    ADC r3, r3, #0
    /* COMBA_STEP_1 r6 r9 */
    UMLAL r5, r14, r6, r9
        MOV r12, #0
        LDR  r10, [r0, #(4*3)]
/*
    ADDS r4, r4, r14
    ADC r3, r3, #0
*/
    /* COMBA_ADD_1 3 */
    ADDS r5, r5, r10
    ADCS r4, r4, r14
/* ADC  r3, r3, #0 */
    /* COMBA_MFIN_1 3 */
    STR r5, [r0, #(4*3)]

    /* COMBA_STEP_2 r8 r11 */
    UMLAL r4, r12, r8, r11
        MOV r5, #0
        LDR r10,  [r2, #(4*4)]
        MOV r14, #0
    ADCS r3, r3, r12
    ADC r5, r5, #0
    /* COMBA_STEP_2 r7 r9 */
    UMLAL r4, r14, r7, r9
        MOV r12, #0
    ADDS r3, r3, r14
    ADC r5, r5, #0
    /* COMBA_STEP_2 r6 r10 */
    UMLAL r4, r12, r6, r10
        MOV r14, #0
        LDR  r11, [r0, #(4*4)]
/*
    ADDS r3, r3, r12
    ADC r5, r5, #0
*/
    /* COMBA_ADD_2 4 */
    ADDS r4, r4, r11
    ADCS r3, r3, r12
/* ADC  r5, r5, #0 */
    /* COMBA_MFIN_2 4 */
    STR r4, [r0, #(4*4)]

    /* COMBA_STEP_3 r8 r9 */
    UMLAL r3, r14, r8, r9
        MOV r4, #0
        LDR r11,  [r2, #(4*5)]
        MOV r12, #0
    ADCS r5, r5, r14
    ADC r4, r4, #0
    /* COMBA_STEP_3 r7 r10 */
    UMLAL r3, r12, r7, r10
        MOV r14, #0
    ADDS r5, r5, r12
    ADC r4, r4, #0
    /* COMBA_STEP_3 r6 r11 */
    UMLAL r3, r14, r6, r11
        MOV r12, #0
        LDR  r9, [r0, #(4*5)]
/*
    ADDS r5, r5, r14
    ADC r4, r4, #0
*/
    /* COMBA_ADD_3 5 */
    ADDS r3, r3, r9
    ADCS r5, r5, r14
/* ADC  r4, r4, #0 */
    /* COMBA_MFIN_3 5 */
    STR r3, [r0, #(4*5)]

    /* COMBA_STEP_1 r8 r10 */
    UMLAL r5, r12, r8, r10
        MOV r3, #0
        LDR r9,  [r2, #(4*6)]
        MOV r14, #0
    ADCS r4, r4, r12
    ADC r3, r3, #0
    /* COMBA_STEP_1 r7 r11 */
    UMLAL r5, r14, r7, r11
        MOV r12, #0
    ADDS r4, r4, r14
    ADC r3, r3, #0
    /* COMBA_STEP_1 r6 r9 */
    UMLAL r5, r12, r6, r9
        MOV r14, #0
        LDR  r10, [r0, #(4*6)]
/*
    ADDS r4, r4, r12
    ADC r3, r3, #0
*/
    /* COMBA_ADD_1 6 */
    ADDS r5, r5, r10
    ADCS r4, r4, r12
/* ADC  r3, r3, #0 */
    /* COMBA_MFIN_1 6 */
    STR r5, [r0, #(4*6)]

    /* COMBA_STEP_2 r8 r11 */
    UMLAL r4, r14, r8, r11
        MOV r5, #0
        LDR r10,  [r2, #(4*7)]
        MOV r12, #0
    ADCS r3, r3, r14
    ADC r5, r5, #0
    /* COMBA_STEP_2 r7 r9 */
    UMLAL r4, r12, r7, r9
        MOV r14, #0
    ADDS r3, r3, r12
    ADC r5, r5, #0
    /* COMBA_STEP_2 r6 r10 */
    UMLAL r4, r14, r6, r10
        MOV r12, #0
        LDR  r6, [r0, #(4*7)]
/*
    ADDS r3, r3, r14
    ADC r5, r5, #0
*/
    /* COMBA_ADD_2 7 */
    ADDS r4, r4, r6
    ADCS r3, r3, r14
/* ADC  r5, r5, #0 */
    /* COMBA_MFIN_2 7 */
    STR r4, [r0, #(4*7)]

/********************************************************** r1 - Part 3 ***********************************************************************/

    /* COMBA_STEP_3 r8 r9 */
    UMLAL r3, r12, r8, r9
        MOV r4, #0
        LDR r6,  [r1, #(4*3)]
        MOV r14, #0
    ADCS r5, r5, r12
    ADC r4, r4, #0
    /* COMBA_STEP_3 r6 r11 */
    UMLAL r3, r14, r6, r11
        MOV r12, #0
    ADDS r5, r5, r14
    ADC r4, r4, #0
    /* COMBA_STEP_3 r7 r10 */
    UMLAL r3, r12, r7, r10
        MOV r14, #0
        LDR  r7, [r0, #(4*8)]
/*
    ADDS r5, r5, r12
    ADC r4, r4, #0
*/
    /* COMBA_ADD_3 8 */
    ADDS r3, r3, r7
    ADCS r5, r5, r12
/* ADC  r4, r4, #0 */
    /* COMBA_MFIN_3 8 */
    STR r3, [r0, #(4*8)]

    /* COMBA_STEP_1 r6 r9 */
    UMLAL r5, r14, r6, r9
        MOV r3, #0
        LDR r7,  [r1, #(4*4)]
        MOV r12, #0
    ADCS r4, r4, r14
    ADC r3, r3, #0
    /* COMBA_STEP_1 r7 r11 */
    UMLAL r5, r12, r7, r11
        MOV r14, #0
    ADDS r4, r4, r12
    ADC r3, r3, #0
    /* COMBA_STEP_1 r8 r10 */
    UMLAL r5, r14, r8, r10
        MOV r12, #0
        LDR  r8, [r0, #(4*9)]
/*
    ADDS r4, r4, r14
    ADC r3, r3, #0
*/
    /* COMBA_ADD_1 9 */
    ADDS r5, r5, r8
    ADCS r4, r4, r14
/* ADC  r3, r3, #0 */
    /* COMBA_MFIN_1 9 */
    STR r5, [r0, #(4*9)]

    /* COMBA_STEP_2 r7 r9 */
    UMLAL r4, r12, r7, r9
        MOV r5, #0
        LDR r8,  [r1, #(4*5)]
        MOV r14, #0
    ADCS r3, r3, r12
    ADC r5, r5, #0
    /* COMBA_STEP_2 r8 r11 */
    UMLAL r4, r14, r8, r11
        MOV r12, #0
    ADDS r3, r3, r14
    ADC r5, r5, #0
    /* COMBA_STEP_2 r6 r10 */
    UMLAL r4, r12, r6, r10
        MOV r14, #0
        LDR  r6, [r0, #(4*10)]
/*
    ADDS r3, r3, r12
    ADC r5, r5, #0
*/
    /* COMBA_ADD_2 10 */
    ADDS r4, r4, r6
    ADCS r3, r3, r12
/* ADC  r5, r5, #0 */
    /* COMBA_MFIN_2 10 */
    STR r4, [r0, #(4*10)]

    /* COMBA_STEP_3 r8 r9 */
    UMLAL r3, r14, r8, r9
        MOV r4, #0
        LDR r6,  [r1, #(4*6)]
        MOV r12, #0
    ADCS r5, r5, r14
    ADC r4, r4, #0
    /* COMBA_STEP_3 r6 r11 */
    UMLAL r3, r12, r6, r11
        MOV r14, #0
    ADDS r5, r5, r12
    ADC r4, r4, #0
    /* COMBA_STEP_3 r7 r10 */
    UMLAL r3, r14, r7, r10
        MOV r12, #0
        LDR  r7, [r0, #(4*11)]
/*
    ADDS r5, r5, r14
    ADC r4, r4, #0
*/
    /* COMBA_ADD_3 11 */
    ADDS r3, r3, r7
    ADCS r5, r5, r14
/* ADC  r4, r4, #0 */
    /* COMBA_MFIN_3 11 */
    STR r3, [r0, #(4*11)]

    /* COMBA_STEP_1 r6 r9 */
    UMLAL r5, r12, r6, r9
        MOV r3, #0
        LDR r7,  [r1, #(4*7)]
        MOV r14, #0
    ADCS r4, r4, r12
    ADC r3, r3, #0
    /* COMBA_STEP_1 r7 r11 */
    UMLAL r5, r14, r7, r11
        MOV r12, #0
    ADDS r4, r4, r14
    ADC r3, r3, #0
    /* COMBA_STEP_1 r8 r10 */
    UMLAL r5, r12, r8, r10
        MOV r14, #0
    ADDS r4, r4, r12
    ADC r3, r3, #0
    /* COMBA_ADD_1 12 */
    LDR  r12, [r0, #(4*12)]
    ADDS r5, r5, r12
    ADCS r4, r4, #0
/* ADC  r3, r3, #0 */
    /* COMBA_MFIN_1 12 */
    STR r5, [r0, #(4*12)]

/********************************************************** r1 - Part 4 ***********************************************************************/

    /* COMBA_STEP_2 r7 r9 */
    UMLAL r4, r14, r7, r9
        MOV r5, #0
        MOV r12, #0
    ADCS r3, r3, r14
    ADC r5, r5, #0
    /* COMBA_STEP_2 r6 r10 */
    UMLAL r4, r12, r6, r10
        MOV r14, #0
    ADDS r3, r3, r12
/* ADC r5, r5, #0 */
    /* COMBA_MFIN_2 13 */
    STR r4, [r0, #(4*13)]

    /* COMBA_STEP_3 r7 r10 */
    UMLAL r3, r14, r7, r10
        MOV r4, #0
    ADCS r5, r5, r14
    ADC r4, r4, #0
    /* COMBA_MFIN_3 14 */
    STR r3, [r0, #(4*14)]

    STR r5, [r0, #(4*15)]

    LDMIA sp!, {r4-r12, r14}
    MOV pc, lr
