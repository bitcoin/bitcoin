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

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

.text

.global fp_rdcn_low

/********************************************************** FP_RDCN_LOW ***********************************************************************/

#define P71 0x2523
#define P70 0x6482
#define P6 0x40000001
#define P51 0xBA34
#define P50 0x4D80
#define P4 0x00000008
#define P31 0x6121
#define P30 0x0000
#define P2 0x00000013
#define P1 0xA7000000
#define P0 0x00000013

#define CONST1 0xD794
#define CONST0 0x35E5


fp_rdcn_low:
	STMDB sp!, {r4-r12, r14}

	PLD    [r1, #32]

	MOV r3, #0
	MOV r4, #0
	MOV r5, #0

	/* COMBA_MFIN_LO_1 0 */
	LDR  r6, [r1, #(4*0)]
		MOV r8, #P0
		MOVW r9, #CONST0
		MOVT r9, #CONST1
	ADDS r5, r5, r6
/*
	ADCS r4, r4, #0
	ADC  r3, r3, #0
*/
	UMULL r6, r10, r5, r9
   		MOV r12, #0
   		MOV r14, #0
   	STR r6, [r0, #(4*0)]


	UMLAL r5, r12, r6, r8
		LDR r6, [r0, #(4*0)]
    	MOV r7, #P1
	ADCS r4, r4, r12
	ADC r3, r3, #0

	/* COMBA_STEP_LO_2 0 P1 */
	UMLAL r4, r14, r6, r7
		LDR  r6, [r1, #(4*1)]
		MOV r12, #0
	ADDS r3, r3, r14
	ADC r5, r5, #0
	/* COMBA_MFIN_LO_2 1 */
	ADDS r4, r4, r6
/*
	ADCS r3, r3, #0
	ADC  r5, r5, #0
*/
	UMULL r6, r10, r4, r9
		MOV r14, #0
    STR r6, [r0, #(4*1)]

	UMLAL r4, r12, r6, r8
		LDR r6, [r0, #(4*0)]
    	MOV r7, #P2
	ADCS r3, r3, r12
	ADC r5, r5, #0

	/* COMBA_STEP_LO_3 0 P2 */

	UMLAL r3, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*1)]
		MOV r7, #P1
	ADDS r5, r5, r14
	ADC r4, r4, #0
	/* COMBA_STEP_LO_3 1 P1 */

	UMLAL r3, r12, r6, r7
		LDR  r6, [r1, #(4*2)]
		MOV r14, #0
	ADDS r5, r5, r12
	ADC r4, r4, #0
	/* COMBA_MFIN_LO_3 2 */
	ADDS r3, r3, r6
/*
	ADCS r5, r5, #0
	ADC  r4, r4, #0
*/
	UMULL r6, r10, r3, r9
    STR r6, [r0, #(4*2)]

	UMLAL r3, r14, r6, r8
		MOV r12, #0
		LDR r6, [r0, #(4*0)]
		MOVW r7, #P30
    	MOVT r7, #P31
	ADCS r5, r5, r14
	ADC r4, r4, #0

	/* _COMBA_STEP_LO_1 0 P31 P30 */
	UMLAL r5, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*1)]
   		MOV r7, #P2
	ADDS r4, r4, r12
	ADC r3, r3, #0
	/* COMBA_STEP_LO_1 1 P2 */
	UMLAL r5, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*2)]
    	MOV r7, #P1
	ADDS r4, r4, r14
	ADC r3, r3, #0
	/* COMBA_STEP_LO_1 2 P1 */
	UMLAL r5, r12, r6, r7
		MOV r14, #0
		LDR  r6, [r1, #(4*3)]
	ADDS r4, r4, r12
	ADC r3, r3, #0
	/* COMBA_MFIN_LO_1 3 */
	ADDS r5, r5, r6
/*
	ADCS r4, r4, #0
	ADC  r3, r3, #0
*/
	UMULL r6, r10, r5, r9
   	STR r6, [r0, #(4*3)]

	UMLAL r5, r14, r6, r8
		MOV r12, #0
		LDR r6, [r0, #(4*0)]
   	 	MOV r7, #P4
	ADCS r4, r4, r14
	ADC r3, r3, #0

	/* COMBA_STEP_LO_2 0 P4 */

	UMLAL r4, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*1)]
		MOVW r7, #P30
    	MOVT r7, #P31
	ADDS r3, r3, r12
	ADC r5, r5, #0
	/* _COMBA_STEP_LO_2 1 P31 P30 */

	UMLAL r4, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*2)]
    	MOV r7, #P2
	ADDS r3, r3, r14
	ADC r5, r5, #0
	/* COMBA_STEP_LO_2 2 P2 */

	UMLAL r4, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*3)]
    	MOV r7, #P1
	ADDS r3, r3, r12
	ADC r5, r5, #0
	/* COMBA_STEP_LO_2 3 P1 */

	UMLAL r4, r14, r6, r7
		MOV r12, #0
		LDR  r6, [r1, #(4*4)]
	ADDS r3, r3, r14
	ADC r5, r5, #0
	/* COMBA_MFIN_LO_2 4 */

	ADDS r4, r4, r6
/*
	ADCS r3, r3, #0
	ADC  r5, r5, #0
*/
	UMULL r6, r10, r4, r9
    STR r6, [r0, #(4*4)]

	UMLAL r4, r12, r6, r8
		MOV r14, #0
		LDR r6, [r0, #(4*0)]
		MOVW r7, #P50
    	MOVT r7, #P51
	ADCS r3, r3, r12
	ADC r5, r5, #0

	/* _COMBA_STEP_LO_3 0 P51 P50 */

	UMLAL r3, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*1)]
    	MOV r7, #P4
	ADDS r5, r5, r14
	ADC r4, r4, #0
	/* COMBA_STEP_LO_3 1 P4 */

	UMLAL r3, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*2)]
		MOVW r7, #P30
    	MOVT r7, #P31
	ADDS r5, r5, r12
	ADC r4, r4, #0
	/* _COMBA_STEP_LO_3 2 P31 P30 */

	UMLAL r3, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*3)]
    	MOV r7, #P2
	ADDS r5, r5, r14
	ADC r4, r4, #0
	/* COMBA_STEP_LO_3 3 P2 */

	UMLAL r3, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*4)]
    	MOV r7, #P1
	ADDS r5, r5, r12
	ADC r4, r4, #0
	/* COMBA_STEP_LO_3 4 P1 */

	UMLAL r3, r14, r6, r7
		MOV r12, #0
		LDR  r6, [r1, #(4*5)]
	ADDS r5, r5, r14
	ADC r4, r4, #0
	/* COMBA_MFIN_LO_3 5 */
	ADDS r3, r3, r6
/*
	ADCS r5, r5, #0
	ADC  r4, r4, #0
*/
	UMULL r6, r10, r3, r9
    STR r6, [r0, #(4*5)]

	UMLAL r3, r12, r6, r8
		MOV r14, #0
		LDR r6, [r0, #(4*0)]
    	MOVW r7, #0x0001
    	MOVT r7, #0x4000
	ADCS r5, r5, r12
	ADC r4, r4, #0

	/* COMBA_STEP_LO_1 0 P6 */

	UMLAL r5, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*1)]
		MOVW r7, #P50
    	MOVT r7, #P51
	ADDS r4, r4, r14
	ADC r3, r3, #0
	/*_COMBA_STEP_LO_1 1 P51 P50 */

	UMLAL r5, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*2)]
    	MOV r7, #P4
	ADDS r4, r4, r12
	ADC r3, r3, #0
	/* COMBA_STEP_LO_1 2 P4 */

	UMLAL r5, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*3)]
		MOVW r7, #P30
    	MOVT r7, #P31
	ADDS r4, r4, r14
	ADC r3, r3, #0
	/* _COMBA_STEP_LO_1 3 P31 P30 */

	UMLAL r5, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*4)]
    	MOV r7, #P2
	ADDS r4, r4, r12
	ADC r3, r3, #0
	/* COMBA_STEP_LO_1 4 P2 */

	UMLAL r5, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*5)]
    	MOV r7, #P1
	ADDS r4, r4, r14
	ADC r3, r3, #0
	/* COMBA_STEP_LO_1 5 P1 */

	UMLAL r5, r12, r6, r7
		MOV r14, #0
		LDR  r6, [r1, #(4*6)]
	ADDS r4, r4, r12
	ADC r3, r3, #0
	/* COMBA_MFIN_LO_1 6 */
	ADDS r5, r5, r6
/*
	ADCS r4, r4, #0
	ADC  r3, r3, #0
*/
	UMULL r6, r10, r5, r9
   	STR r6, [r0, #(4*6)]

	UMLAL r5, r14, r6, r8
		MOV r12, #0
		LDR r6, [r0, #(4*0)]
		MOVW r7, #P70
    	MOVT r7, #P71
	ADCS r4, r4, r14
	ADC r3, r3, #0

	/* _COMBA_STEP_LO_2 0 P71 P70 */

	UMLAL r4, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*1)]
    	MOVW r7, #0x0001
    	MOVT r7, #0x4000
	ADDS r3, r3, r12
	ADC r5, r5, #0
	/* COMBA_STEP_LO_2 1 P6 */

	UMLAL r4, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*2)]
		MOVW r7, #P50
    	MOVT r7, #P51
	ADDS r3, r3, r14
	ADC r5, r5, #0
	/* _COMBA_STEP_LO_2 2 P51 P50 */

	UMLAL r4, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*3)]
    	MOV r7, #P4
	ADDS r3, r3, r12
	ADC r5, r5, #0
	/* COMBA_STEP_LO_2 3 P4 */

	UMLAL r4, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*4)]
		MOVW r7, #P30
    	MOVT r7, #P31
	ADDS r3, r3, r14
	ADC r5, r5, #0
	/* _COMBA_STEP_LO_2 4 P31 P30 */

	UMLAL r4, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*5)]
    	MOV r7, #P2
	ADDS r3, r3, r12
	ADC r5, r5, #0
	/* COMBA_STEP_LO_2 5 P2 */

	UMLAL r4, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*6)]
    	MOV r7, #P1
	ADDS r3, r3, r14
	ADC r5, r5, #0
	/* COMBA_STEP_LO_2 6 P1 */

	UMLAL r4, r12, r6, r7
		MOV r14, #0
		LDR  r6, [r1, #(4*7)]
	ADDS r3, r3, r12
	ADC r5, r5, #0
	/* COMBA_MFIN_LO_2 7 */
	ADDS r4, r4, r6
/*
	ADCS r3, r3, #0
	ADC  r5, r5, #0
*/
	UMULL r6, r10, r4, r9
    STR r6, [r0, #(4*7)]

	UMLAL r4, r14, r6, r8
		MOV r12, #0
		LDR r6, [r0, #(4*1)]
		MOVW r7, #P70
    	MOVT r7, #P71
	ADCS r3, r3, r14
	ADC r5, r5, #0

	/* _COMBA_STEP_HI_3 1 P71 P70 */

	UMLAL r3, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*2)]
    	MOVW r7, #0x0001
    	MOVT r7, #0x4000
	ADDS r5, r5, r12
	ADC r4, r4, #0
	/* COMBA_STEP_HI_3 2 P6 */

	UMLAL r3, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*3)]
		MOVW r7, #P50
    	MOVT r7, #P51
	ADDS r5, r5, r14
	ADC r4, r4, #0
	/* _COMBA_STEP_HI_3 3 P51 P50 */

	UMLAL r3, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*4)]
    	MOV r7, #P4
	ADDS r5, r5, r12
	ADC r4, r4, #0
	/* COMBA_STEP_HI_3 4 P4 */

	UMLAL r3, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*5)]
		MOVW r7, #P30
    	MOVT r7, #P31
	ADDS r5, r5, r14
	ADC r4, r4, #0
	/* _COMBA_STEP_HI_3 5 P31 P30 */

	UMLAL r3, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*6)]
    	MOV r7, #P2
	ADDS r5, r5, r12
	ADC r4, r4, #0
	/* COMBA_STEP_HI_3 6 P2 */

	UMLAL r3, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*7)]
    	MOV r7, #P1
	ADDS r5, r5, r14
	ADC r4, r4, #0
	/* COMBA_STEP_HI_3 7 P1 */

	UMLAL r3, r12, r6, r7
		MOV r14, #0
		LDR  r6, [r1, #(4*8)]
		MOVW r7, #P70
    	MOVT r7, #P71
/*
	ADDS r5, r5, r12
	ADC r4, r4, #0
*/
	/* COMBA_MFIN_HI_3 8 */
	ADDS r3, r3, r6
	ADCS r5, r5, r12
	ADC  r4, r4, #0
	LDR r6, [r0, #(4*2)]
	STR r3, [r0, #(4*(8-8))]
	MOV r3, #0

	/* _COMBA_STEP_HI_1 2 P71 P70 */

	UMLAL r5, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*3)]
    	MOVW r7, #0x0001
    	MOVT r7, #0x4000
	ADDS r4, r4, r14
	ADC r3, r3, #0
	/* COMBA_STEP_HI_1 3 P6 */

	UMLAL r5, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*4)]
		MOVW r7, #P50
    	MOVT r7, #P51
	ADDS r4, r4, r12
	ADC r3, r3, #0
	/* _COMBA_STEP_HI_1 4 P51 P50 */

	UMLAL r5, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*5)]
    	MOV r7, #P4
	ADDS r4, r4, r14
	ADC r3, r3, #0
	/* COMBA_STEP_HI_1 5 P4 */

	UMLAL r5, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*6)]
		MOVW r7, #P30
    	MOVT r7, #P31
	ADDS r4, r4, r12
	ADC r3, r3, #0
	/* _COMBA_STEP_HI_1 6 P31 P30 */

	UMLAL r5, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*7)]
    	MOV r7, #P2
	ADDS r4, r4, r14
	ADC r3, r3, #0
	/* COMBA_STEP_HI_1 7 P2 */

	UMLAL r5, r12, r6, r7
		MOV r14, #0
		LDR  r6, [r1, #(4*9)]
		MOVW r7, #P70
    	MOVT r7, #P71
/*
	ADDS r4, r4, r12
	ADC r3, r3, #0
*/
	/* COMBA_MFIN_HI_1 9 */
	ADDS r5, r5, r6
	ADCS r4, r4, r12
	ADC  r3, r3, #0
	LDR r6, [r0, #(4*3)]
	STR r5, [r0, #(4*(9-8))]
	MOV r5, #0

	/* _COMBA_STEP_HI_2 3 P71 P70 */

	UMLAL r4, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*4)]
    	MOVW r7, #0x0001
    	MOVT r7, #0x4000
	ADDS r3, r3, r14
	ADC r5, r5, #0
	/* COMBA_STEP_HI_2 4 P6 */

	UMLAL r4, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*5)]
		MOVW r7, #P50
    	MOVT r7, #P51
	ADDS r3, r3, r12
	ADC r5, r5, #0
	/* _COMBA_STEP_HI_2 5 P51 P50 */

	UMLAL r4, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*6)]
    	MOV r7, #P4
	ADDS r3, r3, r14
	ADC r5, r5, #0
	/* COMBA_STEP_HI_2 6 P4 */

	UMLAL r4, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*7)]
		MOVW r7, #P30
    	MOVT r7, #P31
	ADDS r3, r3, r12
	ADC r5, r5, #0
	/* _COMBA_STEP_HI_2 7 P31 P30 */

	UMLAL r4, r14, r6, r7
		MOV r12, #0
		LDR  r6, [r1, #(4*10)]
		MOVW r7, #P70
    	MOVT r7, #P71
/*
	ADDS r3, r3, r14
	ADC r5, r5, #0
*/
	/* COMBA_MFIN_HI_2 10 */
	ADDS r4, r4, r6
	ADCS r3, r3, r14
	ADC  r5, r5, #0
	LDR r6, [r0, #(4*4)]
	STR r4, [r0, #(4*(10-8))]
	MOV r4, #0

	/* _COMBA_STEP_HI_3 4 P71 P70 */
	UMLAL r3, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*5)]
    	MOVW r7, #0x0001
    	MOVT r7, #0x4000
	ADDS r5, r5, r12
	ADC r4, r4, #0
	/* COMBA_STEP_HI_3 5 P6 */
	UMLAL r3, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*6)]
		MOVW r7, #P50
  	  MOVT r7, #P51
	ADDS r5, r5, r14
	ADC r4, r4, #0
	/* _COMBA_STEP_HI_3 6 P51 P50 */

	UMLAL r3, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*7)]
    	MOV r7, #P4
	ADDS r5, r5, r12
	ADC r4, r4, #0
	/* COMBA_STEP_HI_3 7 P4 */

	UMLAL r3, r14, r6, r7
		MOV r12, #0
		LDR  r6, [r1, #(4*11)]
		MOVW r7, #P70
    	MOVT r7, #P71
/*
	ADDS r5, r5, r14
	ADC r4, r4, #0
*/
	/* COMBA_MFIN_HI_3 11 */
	ADDS r3, r3, r6
	ADCS r5, r5, r14
	ADC  r4, r4, #0
	LDR r6, [r0, #(4*5)]
	STR r3, [r0, #(4*(11-8))]
	MOV r3, #0

	/* _COMBA_STEP_HI_1 5 P71 P70 */
	UMLAL r5, r12, r6, r7
		MOV r14, #0
		LDR r6, [r0, #(4*6)]
    	MOVW r7, #0x0001
    	MOVT r7, #0x4000
	ADDS r4, r4, r12
	ADC r3, r3, #0
	/* COMBA_STEP_HI_1 6 P6 */

	UMLAL r5, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*7)]
		MOVW r7, #P50
    	MOVT r7, #P51
	ADDS r4, r4, r14
	ADC r3, r3, #0
	/* _COMBA_STEP_HI_1 7 P51 P50 */

	UMLAL r5, r12, r6, r7
		MOV r14, #0
		LDR  r6, [r1, #(4*12)]
		MOVW r7, #P70
    	MOVT r7, #P71
/*
	ADDS r4, r4, r12
	ADC r3, r3, #0
*/
	/* COMBA_MFIN_HI_1 12 */
	ADDS r5, r5, r6
	ADCS r4, r4, r12
	ADC  r3, r3, #0
	LDR r6, [r0, #(4*6)]
	STR r5, [r0, #(4*(12-8))]
	MOV r5, #0
	/* _COMBA_STEP_HI_2 6 P71 P70 */

	UMLAL r4, r14, r6, r7
		MOV r12, #0
		LDR r6, [r0, #(4*7)]
    	MOVW r7, #0x0001
    	MOVT r7, #0x4000
	ADDS r3, r3, r14
	ADC r5, r5, #0
	/* COMBA_STEP_HI_2 7 P6 */

	UMLAL r4, r12, r6, r7
		MOV r14, #0
		LDR  r6, [r1, #(4*13)]
		MOVW r7, #P70
    	MOVT r7, #P71
 /*
	ADDS r3, r3, r12
	ADC r5, r5, #0
*/
	/* COMBA_MFIN_HI_2 13 */
	ADDS r4, r4, r6
	ADCS r3, r3, r12
	ADC  r5, r5, #0
	LDR r6, [r0, #(4*7)]
	STR r4, [r0, #(4*(13-8))]
	MOV r4, #0

	/* _COMBA_STEP_HI_3 7 P71 P70 */
	UMLAL r3, r14, r6, r7
		MOV r12, #0
		LDR  r6, [r1, #(4*14)]
/*
	ADDS r5, r5, r14
	ADC r4, r4, #0
*/
	/* COMBA_MFIN_HI_3 14 */
	ADDS r3, r3, r6
	ADCS r5, r5, r14
	ADC  r4, r4, #0
	LDR  r6, [r1, #(4*15)]
	STR r3, [r0, #(4*(14-8))]
	MOV r3, #0

	/* COMBA_ADD_1 15 */
	ADD r5, r5, r6
/*
	ADCS r4, r4, #0
	ADC  r3, r3, #0
*/
	STR r5, [r0, #(4*7)]
	/* STR r4, [r0, #(4*8)]  Necessario? */

/* CMP_STEP: */
	LDR r5, [r0, #0]
	LDR r6, [r0, #4]
	LDR r7, [r0, #8]
	LDR r8, [r0, #12]
	LDR r9, [r0, #16]
	LDR r10, [r0, #20]
	LDR r11, [r0, #24]
	LDR r12, [r0, #28]

	/**** Primeira iteracao ****/
	MOVW r3, #0x6482
	MOVT r3, #0x2523
	CMP r12, r3
	BHI GREATER_THAN_OR_EQUAL
	BLO LESS_THAN

	/**** Segunda iteracao ****/
    	MOVW r3, #0x0001
    	MOVT r3, #0x4000
	CMP r11, r3
	BHI GREATER_THAN_OR_EQUAL
	BLO LESS_THAN

	/**** Terceira iteracao ****/
	MOVW r3, #0x4D80
	MOVT r3, #0xBA34
	CMP r10, r3
	BHI GREATER_THAN_OR_EQUAL
	BLO LESS_THAN

	/**** Quarta iteracao ****/
	MOV r3, #P4
	CMP r9, r3
	BHI GREATER_THAN_OR_EQUAL
	BLO LESS_THAN

	/**** Quinta iteracao ****/
	MOVW r3, #0x0000
	MOVT r3, #0x6121
	CMP r8, r3
	BHI GREATER_THAN_OR_EQUAL
	BLO LESS_THAN

	/**** Sexta iteracao ****/
	MOV r3, #P2
	CMP r7, r3
	BHI GREATER_THAN_OR_EQUAL
	BLO LESS_THAN

	/**** Setima iteracao ****/
	MOV r3, #P1
	CMP r6, r3
	BHI GREATER_THAN_OR_EQUAL
	BLO LESS_THAN

	/**** Oitava iteracao ****/
	MOV r3, #P0
	CMP r5, r3
	BHI GREATER_THAN_OR_EQUAL
	BLO LESS_THAN

GREATER_THAN_OR_EQUAL:

/*Se for maior ou igual subtrair 'p' e escrever em memoria */

/* SUB_STEP: */
	/**** Primeira iteracao ****/
	MOV r3, #P0
	SUBS r5, r5, r3
	STR r5, [r0, #0]

	/**** Segunda iteracao ****/
	MOV r3, #P1
	SBCS r6, r6, r3
	STR r6, [r0, #4]

	/**** Terceira iteracao ****/
	MOV r3, #P2
	SBCS r7, r7, r3
	STR r7, [r0, #8]

	/**** Quarta iteracao ****/
	MOVW r3, #0x0000
	MOVT r3, #0x6121
	SBCS r8, r8, r3
	STR r8, [r0, #12]

	/**** Quinta iteracao ****/
	MOV r3, #P4
	SBCS r9, r9, r3
	STR r9, [r0, #16]

	/**** Sexta iteracao ****/
	MOVW r3, #0x4D80
	MOVT r3, #0xBA34
	SBCS r10, r10, r3
	STR r10, [r0, #20]

	/**** Setima iteracao ****/
    	MOVW r3, #0x0001
    	MOVT r3, #0x4000
	SBCS r11, r11, r3
	STR r11, [r0, #24]

	/**** Oitava iteracao ****/
	MOVW r3, #0x6482
	MOVT r3, #0x2523
	SBCS r12, r12, r3
	STR r12, [r0, #28]

LESS_THAN:

	LDMIA sp!, {r4-r12, r14}
	MOV pc, lr

.end
