/*
 * Copyright 2007-2009 RELIC Project
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
 * Implementation of the low-level prime field modular reduction functions.
 *
 * @ingroup fp
 */

#include "relic_conf.h"

#ifdef MSP430X
#include <msp430.h>
#else

/* For some reason these are not defined in asm */
#define MPY                0x0130  /* Multiply Unsigned/Operand 1 */
#define MPYS               0x0132  /* Multiply Signed/Operand 1 */
#define MAC                0x0134  /* Multiply Unsigned and Accumulate/Operand 1 */
#define MACS               0x0136  /* Multiply Signed and Accumulate/Operand 1 */
#define OP2                0x0138  /* Operand 2 */
#define RESLO              0x013A  /* Result Low Word */
#define RESHI              0x013C  /* Result High Word */
#define SUMEXT             0x013E  /* Sum Extend */

#endif

.text
.align 2
#if FP_RDC == MONTY
.global fp_rdcn_low


/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

fp_rdcn_low:
    /*
     * r15: t
     * r14: z
     * r13: m
     * r12: n'
     */
    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11
    mov r15,r11
    call #fp_prime_get
    mov r15,r13
#ifndef MSP430X2
    call #fp_prime_get_rdc
    mov @r15,r15
    mov r15,r12
#endif
    mov r14,r15
    mov r11,r14

#if FP_PRIME <= 160 && FP_PRIME > (160-16)

#ifdef MSP430X2
#include "fp_rdc32_160_montgomery_sparse.inc"
#else
#include "fp_rdc_160_montgomery_sparse.inc"
#endif

#elif FP_PRIME <= 256 && FP_PRIME > (256 - 16)

#ifdef MSP430X2
#include "fp_rdc32_256_montgomery_sparse.inc"
#else
#include "fp_rdc_256_montgomery_sparse.inc"
#endif

#else
#error "Unsupported prime field size"
#endif

    /* r14: z
     * r13: m
     */
    /* subtract? */
    tst r11
    jnz subtract /* jmp to subtract */
#if FP_PRIME <= 256 && FP_PRIME > (256 - 16)
    cmp 2*15(r13),2*15(r14)
    jlo dont_subtract /* m > z, jmp to DO NOT subtract */
    jne subtract /* m < z, jmp to subtract */
    cmp 2*14(r13),2*14(r14)
    jlo dont_subtract
    jne subtract
    cmp 2*13(r13),2*13(r14)
    jlo dont_subtract
    jne subtract
    cmp 2*12(r13),2*12(r14)
    jlo dont_subtract
    jne subtract
    cmp 2*11(r13),2*11(r14)
    jlo dont_subtract
    jne subtract
    cmp 2*10(r13),2*10(r14)
    jlo dont_subtract
    jne subtract
#endif
    cmp 2*9(r13),2*9(r14)
    jlo dont_subtract
    jne subtract
    cmp 2*8(r13),2*8(r14)
    jlo dont_subtract
    jne subtract
    cmp 2*7(r13),2*7(r14)
    jlo dont_subtract
    jne subtract
    cmp 2*6(r13),2*6(r14)
    jlo dont_subtract
    jne subtract
    cmp 2*5(r13),2*5(r14)
    jlo dont_subtract
    jne subtract
    cmp 2*4(r13),2*4(r14)
    jlo dont_subtract
    jne subtract
    cmp 2*3(r13),2*3(r14)
    jlo dont_subtract
    jne subtract
    cmp 2*2(r13),2*2(r14)
    jlo dont_subtract
    jne subtract
    cmp 2*1(r13),2*1(r14)
    jlo dont_subtract
    jne subtract
    cmp 2*0(r13),2*0(r14)
    jlo dont_subtract
    /* subtract */
subtract:
    sub @r13+,0*2(r14)
    subc @r13+,1*2(r14)
    subc @r13+,2*2(r14)
    subc @r13+,3*2(r14)
    subc @r13+,4*2(r14)
    subc @r13+,5*2(r14)
    subc @r13+,6*2(r14)
    subc @r13+,7*2(r14)
    subc @r13+,8*2(r14)
    subc @r13+,9*2(r14)
#if FP_PRIME <= 256 && FP_PRIME > (256 - 16)
    subc @r13+,10*2(r14)
    subc @r13+,11*2(r14)
    subc @r13+,12*2(r14)
    subc @r13+,13*2(r14)
    subc @r13+,14*2(r14)
    subc @r13+,15*2(r14)
#endif
dont_subtract:

    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
    ret

#else
.global fp_rdcs_low

#if FP_PRIME == 160

#ifdef EP_ENDOM
#include "relic_fp_rdc_low_160k1.s"
#else
#include "relic_fp_rdc_low_160p1.s"
#endif

#elif FP_PRIME == 256

#ifdef EP_ENDOM
#include "relic_fp_rdc_low_256k1.s"
#else
#include "relic_fp_rdc_low_256p1.s"
#endif

#else

#error Unsupported field size

#endif
#endif
