/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2012 RELIC Authors
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
 * Implementation of the low-level binary field addition and subtraction
 * functions.
 *
 * @ingroup fb
 */

#include "relic_conf.h"

.text
.align 2

.global fb_addn_low

fb_addn_low:
    mov @r14+,r12
    xor @r13+,r12
    mov r12,@r15
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*1(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*2(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*3(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*4(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*5(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*6(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*7(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*8(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*9(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*10(r15)
#if (FB_POLYN > 176)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*11(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*12(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*13(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*14(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*15(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*16(r15)
#if (FB_POLYN > 272)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*17(r15)
#if (FB_POLYN > 288)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*18(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*19(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*20(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*21(r15)
    mov @r14+,r12
    xor @r13+,r12
    mov r12,2*22(r15)
#endif
#endif
#endif
    ret
