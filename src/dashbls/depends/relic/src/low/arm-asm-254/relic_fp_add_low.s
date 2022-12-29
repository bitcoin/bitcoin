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

#define RLC_LT	-1
#define RLC_EQ	0
#define RLC_GT	1

.syntax unified
.arch armv7-a

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

.text

.global fp_addn_low
.global fp_subn_low
.global fp_addm_low
.global fp_addd_low
.global fp_subd_low
.global fp_subm_low

/********************************************************** FP_ADDN_LOW ***********************************************************************/

fp_addn_low:
	STMDB sp!, {r4}

	/**** Primeira iteracao ****/
	LDR r3, [r1, #0]	/* r3 = *a */
	LDR r4, [r2, #0]	/* r4 = *b */
	ADDS r3, r3, r4		/* r3 = (*a) + (*b) */
	STR r3, [r0, #0]	/* (*c) = r3*/

	/**** Segunda iteracao ****/
	LDR r3, [r1, #4]	/* r3 = *a */
	LDR r4, [r2, #4]	/* r4 = *b */
	ADCS r3, r3, r4		/* r3 = (*a) + (*b) */
	STR r3, [r0, #4]	/* (*c) = r3*/

	/**** Terceira iteracao ****/
	LDR r3, [r1, #8]	/* r3 = *a */
	LDR r4, [r2, #8]	/* r4 = *b */
	ADCS r3, r3, r4		/* r3 = (*a) + (*b) */
	STR r3, [r0, #8]	/* (*c) = r3*/

	/**** Quarta iteracao ****/
	LDR r3, [r1, #12]	/* r3 = *a */
	LDR r4, [r2, #12]	/* r4 = *b */
	ADCS r3, r3, r4		/* r3 = (*a) + (*b) */
	STR r3, [r0, #12]	/* (*c) = r3*/

	/**** Quinta iteracao ****/
	LDR r3, [r1, #16]	/* r3 = *a */
	LDR r4, [r2, #16]	/* r4 = *b */
	ADCS r3, r3, r4		/* r3 = (*a) + (*b) */
	STR r3, [r0, #16]	/* (*c) = r3*/

	/**** Sexta iteracao ****/
	LDR r3, [r1, #20]	/* r3 = *a */
	LDR r4, [r2, #20]	/* r4 = *b */
	ADCS r3, r3, r4		/* r3 = (*a) + (*b) */
	STR r3, [r0, #20]	/* (*c) = r3*/

	/**** Setima iteracao ****/
	LDR r3, [r1, #24]	/* r3 = *a */
	LDR r4, [r2, #24]	/* r4 = *b */
	ADCS r3, r3, r4		/* r3 = (*a) + (*b) */
	STR r3, [r0, #24]	/* (*c) = r3*/

	/**** Oitava iteracao ****/
	LDR r3, [r1, #28]	/* r3 = *a */
	LDR r4, [r2, #28]	/* r4 = *b */
	ADCS r3, r3, r4		/* r3 = (*a) + (*b) */
	STR r3, [r0, #28]	/* (*c) = r3*/

	MOV r0, #0		/* r0 = carry = 0 */
	ADC r0, r0, r0 	/* Armazenando o resultado do carry no r0*/

	LDMIA sp!, {r4}
	MOV pc, lr		/* return carry*/

/********************************************************** FP_SUBN_LOW ***********************************************************************/
fp_subn_low:
	STMDB sp!, {r4}

	/**** Primeira iteracao ****/
	LDR r3, [r1, #0]	/* r3 = *a */
	LDR r4, [r2, #0]	/* r4 = *b */
	SUBS r3, r3, r4		/* r3 = (*a) - (*b) */
	STR r3, [r0, #0]	/* (*c) = r3*/

	/**** Segunda iteracao ****/
	LDR r3, [r1, #4]	/* r3 = *a */
	LDR r4, [r2, #4]	/* r4 = *b */
	SBCS r3, r3, r4		/* r3 = (*a) - (*b) */
	STR r3, [r0, #4]	/* (*c) = r3*/

	/**** Terceira iteracao ****/
	LDR r3, [r1, #8]	/* r3 = *a */
	LDR r4, [r2, #8]	/* r4 = *b */
	SBCS r3, r3, r4		/* r3 = (*a) - (*b) */
	STR r3, [r0, #8]	/* (*c) = r3*/

	/**** Quarta iteracao ****/
	LDR r3, [r1, #12]	/* r3 = *a */
	LDR r4, [r2, #12]	/* r4 = *b */
	SBCS r3, r3, r4		/* r3 = (*a) - (*b) */
	STR r3, [r0, #12]	/* (*c) = r3*/

	/**** Quinta iteracao ****/
	LDR r3, [r1, #16]	/* r3 = *a */
	LDR r4, [r2, #16]	/* r4 = *b */
	SBCS r3, r3, r4		/* r3 = (*a) - (*b) */
	STR r3, [r0, #16]	/* (*c) = r3*/

	/**** Sexta iteracao ****/
	LDR r3, [r1, #20]	/* r3 = *a */
	LDR r4, [r2, #20]	/* r4 = *b */
	SBCS r3, r3, r4		/* r3 = (*a) - (*b) */
	STR r3, [r0, #20]	/* (*c) = r3*/

	/**** Setima iteracao ****/
	LDR r3, [r1, #24]	/* r3 = *a */
	LDR r4, [r2, #24]	/* r4 = *b */
	SBCS r3, r3, r4		/* r3 = (*a) - (*b) */
	STR r3, [r0, #24]	/* (*c) = r3*/

	/**** Oitava iteracao ****/
	LDR r3, [r1, #28]	/* r3 = *a */
	LDR r4, [r2, #28]	/* r4 = *b */
	SBCS r3, r3, r4		/* r3 = (*a) - (*b) */
	STR r3, [r0, #28]	/* (*c) = r3*/

	MOV r0, #0		/* r0 = carry = 0 */
	SBC r0, r0, r0 	/* Armazenando o resultado do carry no r0*/

	LDMIA sp!, {r4}
	MOV pc, lr		/* return carry*/

/********************************************************** FP_ADDM_LOW ***********************************************************************/

fp_addd_low:
	STMDB sp!, {r4}

	/**** Primeira iteracao ****/
	LDR r3, [r1, #0]	/* r3 = *a */
	LDR r4, [r2, #0]	/* r4 = *b */
	ADDS r3, r3, r4		/* r3 = (*a) + (*b) */
	STR r3, [r0, #0]	/* (*c) = r3*/

	.irp i, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	LDR r3, [r1, #4*\i]	/* r3 = *a */
	LDR r4, [r2, #4*\i]	/* r4 = *b */
	ADCS r3, r3, r4		/* r3 = (*a) + (*b) */
	STR r3, [r0, #4*\i]	/* (*c) = r3*/
	.endr

	MOV r0, #0		/* r0 = carry = 0 */
	ADC r0, r0, r0 	/* Armazenando o resultado do carry no r0*/

	LDMIA sp!, {r4}
	MOV pc, lr		/* return carry*/

fp_subd_low:
	STMDB sp!, {r4}

	/**** Primeira iteracao ****/
	LDR r3, [r1, #0]	/* r3 = *a */
	LDR r4, [r2, #0]	/* r4 = *b */
	SUBS r3, r3, r4		/* r3 = (*a) + (*b) */
	STR r3, [r0, #0]	/* (*c) = r3*/

	.irp i, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	LDR r3, [r1, #4*\i]	/* r3 = *a */
	LDR r4, [r2, #4*\i]	/* r4 = *b */
	SBCS r3, r3, r4		/* r3 = (*a) + (*b) */
	STR r3, [r0, #4*\i]	/* (*c) = r3*/
	.endr

	MOV r0, #0		/* r0 = carry = 0 */
	SBCS r0, r0, r0 	/* Armazenando o resultado do carry no r0*/

	LDMIA sp!, {r4}
	MOV pc, lr		/* return carry*/

fp_addm_low2:
	STMDB sp!, {r4-r12}

/* ADD_STEP: */

/* Manter os resultados em registradores {r5-r12} */

	/**** Primeira iteracao ****/
	LDR r3, [r1, #0]	/* r3 = *a */
	LDR r4, [r2, #0]	/* r4 = *b */
	ADDS r5, r3, r4		/* r3 = (*a) + (*b) */

	/**** Segunda iteracao ****/
	LDR r3, [r1, #4]	/* r3 = *a */
	LDR r4, [r2, #4]	/* r4 = *b */
	ADCS r6, r3, r4		/* r3 = (*a) + (*b) */

	/**** Terceira iteracao ****/
	LDR r3, [r1, #8]	/* r3 = *a */
	LDR r4, [r2, #8]	/* r4 = *b */
	ADCS r7, r3, r4		/* r3 = (*a) + (*b) */

	/**** Quarta iteracao ****/
	LDR r3, [r1, #12]	/* r3 = *a */
	LDR r4, [r2, #12]	/* r4 = *b */
	ADCS r8, r3, r4		/* r3 = (*a) + (*b) */

	/**** Quinta iteracao ****/
	LDR r3, [r1, #16]	/* r3 = *a */
	LDR r4, [r2, #16]	/* r4 = *b */
	ADCS r9, r3, r4		/* r3 = (*a) + (*b) */

	/**** Sexta iteracao ****/
	LDR r3, [r1, #20]	/* r3 = *a */
	LDR r4, [r2, #20]	/* r4 = *b */
	ADCS r10, r3, r4		/* r3 = (*a) + (*b) */

	/**** Setima iteracao ****/
	LDR r3, [r1, #24]	/* r3 = *a */
	LDR r4, [r2, #24]	/* r4 = *b */
	ADCS r11, r3, r4		/* r3 = (*a) + (*b) */

	/**** Oitava iteracao ****/
	LDR r3, [r1, #28]	/* r3 = *a */
	LDR r4, [r2, #28]	/* r4 = *b */
	ADC r12, r3, r4		/* r3 = (*a) + (*b) */

/* SUB_STEP: */
	/**** Primeira iteracao ****/
	SUBS r5, r5, #0x13

	/**** Segunda iteracao ****/
	SBCS r6, r6, #0xA7000000

	/**** Terceira iteracao ****/
	SBCS r7, r7, #0x13

	/**** Quarta iteracao ****/
	MOVW r3, #0x6121
	SBCS r8, r8, r3, LSL#16

	/**** Quinta iteracao ****/
	SBCS r9, r9, #0x8

	/**** Sexta iteracao ****/
	MOVW r3, #0x4D80
	MOVT r3, #0xBA34
	SBCS r10, r10, r3

	/**** Setima iteracao ****/
	SBCS r11, r11, #0x40000001

	/**** Oitava iteracao ****/
	MOVW r3, #0x6482
	MOVT r3, #0x2523
	SBCS r12, r12, r3

	MOV r3, #0		/* r3 = carry = 0 */
	SBCS r3, r3, r3 /* Armazenando o resultado do carry no r3*/

/* Se houver carry, entao o resultado ficou menor, deve-se adicionar o primo 'p' novamente */
	/**** Primeira iteracao ****/
	AND  r4, r3, #0x13
	ADDS r5, r5, r4

	/**** Segunda iteracao ****/
	AND  r4, r3, #0xA7000000
	ADCS r6, r6, r4

	/**** Terceira iteracao ****/
	AND  r4, r3, #0x13
	ADCS r7, r7, r4

	/**** Quarta iteracao ****/
	MOVW r4, #0x6121
	AND  r4, r4, r3
	ADCS r8, r8, r4, LSL#16

	/**** Quinta iteracao ****/
	AND  r4, r3, #0x8
	ADCS r9, r9, r4

	/**** Sexta iteracao ****/
	MOVW r4, #0x4D80
	MOVT r4, #0xBA34
	AND  r4, r4, r3
	ADCS r10, r10, r4

	/**** Setima iteracao ****/
	AND  r4, r3, #0x40000001
	ADCS r11, r11, r4

	/**** Oitava iteracao ****/
	MOVW r4, #0x6482
	MOVT r4, #0x2523
	AND  r4, r4, r3
	ADCS r12, r12, r4

/* Senao, escreve o  resultado em memoria */
	STR r5, [r0, #0]	/* (*c) = r5*/
	STR r6, [r0, #4]	/* (*c) = r6*/
	STR r7, [r0, #8]	/* (*c) = r7*/
	STR r8, [r0, #12]	/* (*c) = r8*/
	STR r9, [r0, #16]	/* (*c) = r9*/
	STR r10, [r0, #20]	/* (*c) = r10*/
	STR r11, [r0, #24]	/* (*c) = r11*/
	STR r12, [r0, #28]	/* (*c) = r12*/

	LDMIA sp!, {r4-r12}
	MOV pc, lr		/* return r*/

fp_addm_low:
	STMDB sp!, {r4-r12}

/* ADD_STEP: */

/* Manter os resultados em registradores {r5-r12} */

	/**** Primeira iteracao ****/
	LDR r3, [r1, #0]	/* r3 = *a */
	LDR r4, [r2, #0]	/* r4 = *b */
	ADDS r5, r3, r4		/* r3 = (*a) + (*b) */

	/**** Segunda iteracao ****/
	LDR r3, [r1, #4]	/* r3 = *a */
	LDR r4, [r2, #4]	/* r4 = *b */
	ADCS r6, r3, r4		/* r3 = (*a) + (*b) */

	/**** Terceira iteracao ****/
	LDR r3, [r1, #8]	/* r3 = *a */
	LDR r4, [r2, #8]	/* r4 = *b */
	ADCS r7, r3, r4		/* r3 = (*a) + (*b) */

	/**** Quarta iteracao ****/
	LDR r3, [r1, #12]	/* r3 = *a */
	LDR r4, [r2, #12]	/* r4 = *b */
	ADCS r8, r3, r4		/* r3 = (*a) + (*b) */

	/**** Quinta iteracao ****/
	LDR r3, [r1, #16]	/* r3 = *a */
	LDR r4, [r2, #16]	/* r4 = *b */
	ADCS r9, r3, r4		/* r3 = (*a) + (*b) */

	/**** Sexta iteracao ****/
	LDR r3, [r1, #20]	/* r3 = *a */
	LDR r4, [r2, #20]	/* r4 = *b */
	ADCS r10, r3, r4		/* r3 = (*a) + (*b) */

	/**** Setima iteracao ****/
	LDR r3, [r1, #24]	/* r3 = *a */
	LDR r4, [r2, #24]	/* r4 = *b */
	ADCS r11, r3, r4		/* r3 = (*a) + (*b) */

	/**** Oitava iteracao ****/
	LDR r3, [r1, #28]	/* r3 = *a */
	LDR r4, [r2, #28]	/* r4 = *b */
	ADC r12, r3, r4		/* r3 = (*a) + (*b) */

/* CMP_STEP: */

	/**** Primeira iteracao ****/
	MOVW r3, #0x6482
	MOVT r3, #0x2523
	CMP r12, r3
	BHI GREATER_THAN_OR_EQUAL
	BLO LESS_THAN

	/**** Segunda iteracao ****/
	CMP r11, #0x40000001
	BHI GREATER_THAN_OR_EQUAL
	BLO LESS_THAN

	/**** Terceira iteracao ****/
	MOVW r3, #0x4D80
	MOVT r3, #0xBA34
	CMP r10, r3
	BHI GREATER_THAN_OR_EQUAL
	BLO LESS_THAN

	/**** Quarta iteracao ****/
	CMP r9, #0x8
	BHI GREATER_THAN_OR_EQUAL
	BLO LESS_THAN

	/**** Quinta iteracao ****/
	MOVW r3, #0x6121
	CMP r8, r3, LSL#16
	BHI GREATER_THAN_OR_EQUAL
	BLO LESS_THAN

	/**** Sexta iteracao ****/
	CMP r7, #0x13
	BHI GREATER_THAN_OR_EQUAL
	BLO LESS_THAN

	/**** Setima iteracao ****/
	CMP r6, #0xA7000000
	BHI GREATER_THAN_OR_EQUAL
	BLO LESS_THAN

	/**** Oitava iteracao ****/
	CMP r5, #0x13
	BHI GREATER_THAN_OR_EQUAL
	BLO LESS_THAN

GREATER_THAN_OR_EQUAL:

/*Se for maior ou igual subtrair 'p' e escrever em memoria */

/* SUB_STEP: */
	/**** Primeira iteracao ****/
	SUBS r5, r5, #0x13
	STR r5, [r0, #0]

	/**** Segunda iteracao ****/
	SBCS r6, r6, #0xA7000000
	STR r6, [r0, #4]

	/**** Terceira iteracao ****/
	SBCS r7, r7, #0x13
	STR r7, [r0, #8]

	/**** Quarta iteracao ****/
	MOVW r3, #0x6121
	SBCS r8, r8, r3, LSL#16
	STR r8, [r0, #12]

	/**** Quinta iteracao ****/
	SBCS r9, r9, #0x8
	STR r9, [r0, #16]

	/**** Sexta iteracao ****/
	MOVW r3, #0x4D80
	MOVT r3, #0xBA34
	SBCS r10, r10, r3
	STR r10, [r0, #20]

	/**** Setima iteracao ****/
	SBCS r11, r11, #0x40000001
	STR r11, [r0, #24]

	/**** Oitava iteracao ****/
	MOVW r3, #0x6482
	MOVT r3, #0x2523
	SBCS r12, r12, r3
	STR r12, [r0, #28]

	LDMIA sp!, {r4-r12}
	MOV pc, lr

LESS_THAN:

/* Se for menor escrever em memória */

	STR r5, [r0, #0]	/* (*c) = r5*/
	STR r6, [r0, #4]	/* (*c) = r6*/
	STR r7, [r0, #8]	/* (*c) = r7*/
	STR r8, [r0, #12]	/* (*c) = r8*/
	STR r9, [r0, #16]	/* (*c) = r9*/
	STR r10, [r0, #20]	/* (*c) = r10*/
	STR r11, [r0, #24]	/* (*c) = r11*/
	STR r12, [r0, #28]	/* (*c) = r12*/

	LDMIA sp!, {r4-r12}
	MOV pc, lr		/* return r*/

/********************************************************** FP_SUBM_LOW ***********************************************************************/
/*
void fp_subm_low(dig_t *c, dig_t *a, dig_t *b) {
	int i;
	dig_t carry, r0, diff;

	carry = 0;
	for (i = 0; i < RLC_FP_DIGS; i++, a++, b++) {
		diff = (*a) - (*b);
		r0 = diff - carry;
		carry = ((*a) < (*b)) || (carry && !diff);
		c[i] = r0;
	}
	if (carry) {
		fp_addn_low(c, c, fp_prime_get());
	}
}
*/

fp_subm_low2:
	STMDB sp!, {r4-r12}

/* SUB_STEP: */
	/**** Primeira iteracao ****/
	LDR r3, [r1, #0]	/* r3 = *a */
	LDR r4, [r2, #0]	/* r4 = *b */
	SUBS r5, r3, r4

	/**** Segunda iteracao ****/
	LDR r3, [r1, #4]	/* r3 = *a */
	LDR r4, [r2, #4]	/* r4 = *b */
	SBCS r6, r3, r4

	/**** Terceira iteracao ****/
	LDR r3, [r1, #8]	/* r3 = *a */
	LDR r4, [r2, #8]	/* r4 = *b */
	SBCS r7, r3, r4

	/**** Quarta iteracao ****/
	LDR r3, [r1, #12]	/* r3 = *a */
	LDR r4, [r2, #12]	/* r4 = *b */
	SBCS r8, r3, r4

	/**** Quinta iteracao ****/
	LDR r3, [r1, #16]	/* r3 = *a */
	LDR r4, [r2, #16]	/* r4 = *b */
	SBCS r9, r3, r4

	/**** Sexta iteracao ****/
	LDR r3, [r1, #20]	/* r3 = *a */
	LDR r4, [r2, #20]	/* r4 = *b */
	SBCS r10, r3, r4

	/**** Setima iteracao ****/
	LDR r3, [r1, #24]	/* r3 = *a */
	LDR r4, [r2, #24]	/* r4 = *b */
	SBCS r11, r3, r4

	/**** Oitava iteracao ****/
	LDR r3, [r1, #28]	/* r3 = *a */
	LDR r4, [r2, #28]	/* r4 = *b */
	SBCS r12, r3, r4

	MOV r3, #0		/* r3 = carry = 0 */
	SBCS r3, r3, r3 	/* Armazenando o resultado do carry no r3*/

	CMP r3, #0   		/* Verifica se o carry = 0 */

/* Se houver carry adicionar o primo 'p' e escrever em memória */

	/**** Primeira iteracao ****/
	ADDNES r5, r5, #0x13

	/**** Segunda iteracao ****/
	ADCNES r6, r6, #0xA7000000

	/**** Terceira iteracao ****/
	ADCNES r7, r7, #0x13

	/**** Quarta iteracao ****/
	MOVW r3, #0x6121
	ADCNES r8, r8, r3, LSL#16

	/**** Quinta iteracao ****/
	ADCNES r9, r9, #0x8

	/**** Sexta iteracao ****/
	MOVW r3, #0x4D80
	MOVT r3, #0xBA34
	ADCNES r10, r10, r3

	/**** Setima iteracao ****/
	ADCNES r11, r11, #0x40000001

	/**** Oitava iteracao ****/
	MOVW r3, #0x6482
	MOVT r3, #0x2523
	ADCNES r12, r12, r3

/* Se nao houver carry escrever em memória */

	STR r5, [r0, #0]	/* (*c) = r5*/
	STR r6, [r0, #4]	/* (*c) = r6*/
	STR r7, [r0, #8]	/* (*c) = r7*/
	STR r8, [r0, #12]	/* (*c) = r8*/
	STR r9, [r0, #16]	/* (*c) = r9*/
	STR r10, [r0, #20]	/* (*c) = r10*/
	STR r11, [r0, #24]	/* (*c) = r11*/
	STR r12, [r0, #28]	/* (*c) = r12*/

	LDMIA sp!, {r4-r12}
	MOV pc, lr

fp_subm_low:
	STMDB sp!, {r4-r12}

/* SUB_STEP: */
	/**** Primeira iteracao ****/
	LDR r3, [r1, #0]	/* r3 = *a */
	LDR r4, [r2, #0]	/* r4 = *b */
	SUBS r5, r3, r4

	/**** Segunda iteracao ****/
	LDR r3, [r1, #4]	/* r3 = *a */
	LDR r4, [r2, #4]	/* r4 = *b */
	SBCS r6, r3, r4

	/**** Terceira iteracao ****/
	LDR r3, [r1, #8]	/* r3 = *a */
	LDR r4, [r2, #8]	/* r4 = *b */
	SBCS r7, r3, r4

	/**** Quarta iteracao ****/
	LDR r3, [r1, #12]	/* r3 = *a */
	LDR r4, [r2, #12]	/* r4 = *b */
	SBCS r8, r3, r4

	/**** Quinta iteracao ****/
	LDR r3, [r1, #16]	/* r3 = *a */
	LDR r4, [r2, #16]	/* r4 = *b */
	SBCS r9, r3, r4

	/**** Sexta iteracao ****/
	LDR r3, [r1, #20]	/* r3 = *a */
	LDR r4, [r2, #20]	/* r4 = *b */
	SBCS r10, r3, r4

	/**** Setima iteracao ****/
	LDR r3, [r1, #24]	/* r3 = *a */
	LDR r4, [r2, #24]	/* r4 = *b */
	SBCS r11, r3, r4

	/**** Oitava iteracao ****/
	LDR r3, [r1, #28]	/* r3 = *a */
	LDR r4, [r2, #28]	/* r4 = *b */
	SBCS r12, r3, r4

	MOV r3, #0		/* r3 = carry = 0 */
	SBCS r3, r3, r3 	/* Armazenando o resultado do carry no r3*/

	CMP r3, #0   		/* Verifica se o carry = 0 */

	BNE IS_ONE

IS_ZERO:

/* Se nao houver carry escrever em memória */

	STR r5, [r0, #0]	/* (*c) = r5*/
	STR r6, [r0, #4]	/* (*c) = r6*/
	STR r7, [r0, #8]	/* (*c) = r7*/
	STR r8, [r0, #12]	/* (*c) = r8*/
	STR r9, [r0, #16]	/* (*c) = r9*/
	STR r10, [r0, #20]	/* (*c) = r10*/
	STR r11, [r0, #24]	/* (*c) = r11*/
	STR r12, [r0, #28]	/* (*c) = r12*/

	LDMIA sp!, {r4-r12}
	MOV pc, lr

IS_ONE:

/* Se houver carry adicionar o primo 'p' e escrever em memória */

	/**** Primeira iteracao ****/
	ADDS r5, r5, #0x13
	STR r5, [r0, #0]

	/**** Segunda iteracao ****/
	ADCS r6, r6, #0xA7000000
	STR r6, [r0, #4]

	/**** Terceira iteracao ****/
	ADCS r7, r7, #0x13
	STR r7, [r0, #8]

	/**** Quarta iteracao ****/
	MOVW r3, #0x6121
	ADCS r8, r8, r3, LSL#16
	STR r8, [r0, #12]

	/**** Quinta iteracao ****/
	ADCS r9, r9, #0x8
	STR r9, [r0, #16]

	/**** Sexta iteracao ****/
	MOVW r3, #0x4D80
	MOVT r3, #0xBA34
	ADCS r10, r10, r3
	STR r10, [r0, #20]

	/**** Setima iteracao ****/
	ADCS r11, r11, #0x40000001
	STR r11, [r0, #24]

	/**** Oitava iteracao ****/
	MOVW r3, #0x6482
	MOVT r3, #0x2523
	ADCS r12, r12, r3
	STR r12, [r0, #28]

	LDMIA sp!, {r4-r12}
	MOV pc, lr
