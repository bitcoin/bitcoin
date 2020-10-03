/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007, 2008, 2009 RELIC Authors
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
 * Implementation of the low-level binary field shifting.
 *
 * @ingroup fb
 */

#include "relic_fb_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

.global fb_sqrl_low

.text
.align 2

.macro STEP i
    mov @r14+,r13
    mov r13,r12
    //clear high byte
    mov.b r13,r13
    rla r13
    mov fb_sqrl_table(r13),2*(2 * \i)(r15)
    swpb r12
    //clear high byte
    mov.b r12,r12
    rla r12
    mov fb_sqrl_table(r12),2*(2 * \i + 1)(r15)
.endm

fb_sqrl_low:
    /* r15: c
       r14: a
    */
    STEP 0
    STEP 1
    STEP 2
    STEP 3
    STEP 4
    STEP 5
    STEP 6
    STEP 7
    STEP 8
    STEP 9
    STEP 10
#if (FB_POLYN > 176)
    STEP 11
    STEP 12
    STEP 13
    STEP 14
    STEP 15
    STEP 16
#if (FB_POLYN > 272)
    STEP 17
#if (FB_POLYN > 288)
    STEP 18
    STEP 19
    STEP 20
    STEP 21
    STEP 22
#endif
#endif
#endif

	ret
