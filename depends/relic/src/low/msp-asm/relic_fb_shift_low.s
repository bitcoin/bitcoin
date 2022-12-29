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

.global fb_lshadd1_low
.global fb_lshadd2_low
.global fb_lshadd3_low
.global fb_lshadd4_low
.global fb_lshadd5_low
.global fb_lshadd6_low
.global fb_lshadd7_low
.global fb_lshadd8_low

.text
.align 2

fb_lshadd1_low:
    /* r15: c
       r14: a
       r13: size
    */
    push	r11
    clr		r11
fb_lshadd1_loop:
    mov 	@r14+,r12
    rrc		r11
    rlc		r12
    rlc		r11
    xor 	@r15+,r12
    mov 	r12,-2(r15)
    dec		r13
    jnz		fb_lshadd1_loop

	mov		r11,r15
	pop		r11
	ret

fb_lshadd2_low:
    /* r15: c
       r14: a
       r13: size
    */
    push	r10
    push	r11
    clr		r10
fb_lshadd2_loop:
    mov 	@r14+,r12
    clr		r11
    rla		r12
    rlc		r11
    rla		r12
    rlc		r11
    xor 	r10,r12
    mov		r11,r10
    xor 	@r15+,r12
    mov 	r12,-2(r15)
    dec		r13
    jnz		fb_lshadd2_loop

	mov		r11, r15
	pop		r11
	pop		r10
	ret

fb_lshadd3_low:
    /* r15: c
       r14: a
       r13: size
    */
    push	r10
    push	r11
    clr		r10
fb_lshadd3_loop:
    mov 	@r14+,r12
    clr		r11
    rla		r12
    rlc		r11
    rla		r12
    rlc		r11
    rla		r12
    rlc		r11
    xor 	r10,r12
    mov		r11,r10
    xor 	@r15+,r12
    mov 	r12,-2(r15)
    dec		r13
    jnz		fb_lshadd3_loop

	mov		r11, r15
	pop		r11
	pop		r10
	ret

fb_lshadd4_low:
    /* r15: c
       r14: a
       r13: size
    */
    push	r10
    push	r11
    clr		r10
fb_lshadd4_loop:
    mov 	@r14+,r12
    clr		r11
    rla		r12
    rlc		r11
    rla		r12
    rlc		r11
    rla		r12
    rlc		r11
    rla		r12
    rlc		r11
    xor 	r10,r12
    mov		r11,r10
    xor 	@r15+,r12
    mov 	r12,-2(r15)
    dec		r13
    jnz		fb_lshadd4_loop

	mov		r11, r15
	pop		r11
	pop		r10
	ret

fb_lshadd5_low:
    /* r15: c
       r14: a
       r13: size
    */
    push	r10
    push	r11
    clr		r10
fb_lshadd5_loop:
    mov 	@r14+,r12
    clr		r11
    rla		r12
    rlc		r11
    rla		r12
    rlc		r11
    rla		r12
    rlc		r11
    rla		r12
    rlc		r11
    rla		r12
    rlc		r11
    xor 	r10,r12
    mov		r11,r10
    xor 	@r15+,r12
    mov 	r12,-2(r15)
    dec		r13
    jnz		fb_lshadd5_loop

	mov		r11, r15
	pop		r11
	pop		r10
	ret

fb_lshadd6_low:
    /* r15: c
       r14: a
       r13: size
    */
    push	r10
    push	r11
    clr		r10
fb_lshadd6_loop:
    mov 	@r14+,r12
    clr		r11
    rla		r12
    rlc		r11
    rla		r12
    rlc		r11
    rla		r12
    rlc		r11
    rla		r12
    rlc		r11
    rla		r12
    rlc		r11
    rla		r12
    rlc		r11
    xor 	r10,r12
    mov		r11,r10
    xor 	@r15+,r12
    mov 	r12,-2(r15)
    dec		r13
    jnz		fb_lshadd6_loop

	mov		r11, r15
	pop		r11
	pop		r10
	ret

fb_lshadd7_low:
    /* r15: c
       r14: a
       r13: size
    */
    push	r10
    push	r11
    clr		r10
fb_lshadd7_loop:
    mov 	@r14+,r12
    mov		r12, r11
    and		#0xFF,r12
    and		#0xFF00,r11
    swpb	r12
    swpb	r11
    clrc
    rrc		r11
    rrc		r12
    xor 	r10,r12
    mov		r11,r10
    xor 	@r15+,r12
    mov 	r12,-2(r15)
    dec		r13
    jnz		fb_lshadd7_loop

	mov		r11, r15
	pop		r11
	pop		r10
	ret

fb_lshadd8_low:
    /* r15: c
       r14: a
       r13: size
    */
    push	r10
    push	r11
    clr		r10
fb_lshadd8_loop:
    mov 	@r14+,r12
    mov		r12,r11
    and		#0xFF,r12
    and		#0xFF00,r11
    swpb	r12
    swpb	r11
    xor 	r10,r12
    mov		r11,r10
    xor 	@r15+,r12
    mov 	r12,-2(r15)
    dec		r13
    jnz		fb_lshadd8_loop

	mov		r11, r15
	pop		r11
	pop		r10
	ret
