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
 * Implementation of the low-level binary field square root.
 *
 * @ingroup fb
 */

#include "relic_fb_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

.global fb_srtn_low

.text
.align 2

#if FB_POLYN == 271

fb_srtn_low:
    /* r15: c
       r14: a
    */
    push r4
    push r5
    push r6
    push r7
    push r8
    sub #2*17,r1

    clr r8

#include "fb_srt_271_penta.inc"

    .irp i, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
        mov @r1+,(2 * \i)(r15)
    .endr

    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
	ret

#elif FB_POLYN == 353

fb_srtn_low:
    /* r15: c
       r14: a
    */
    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    sub #2*23,r1

    clr r9

#include "fb_srt_353_trino.inc"

    .irp i, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22
        mov @r1+,(2 * \i)(r15)
    .endr

    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
    ret

#endif
