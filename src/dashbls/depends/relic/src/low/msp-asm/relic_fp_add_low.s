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
 * Implementation of the low-level prime field addition and subtraction
 * functions.
 *
 * @ingroup fp
 */

#include "relic_conf.h"

.text
.align 2
.global fp_add1_low
.global fp_addn_low
.global fp_addd_low
.global fp_sub1_low
.global fp_subn_low
.global fp_subd_low
.global fp_dbln_low


fp_add1_low:
	add @r14+,r13
	mov r13,@r15
	mov @r14+,r12
	adc r12
	mov r12,2*1(r15)
	mov @r14+,r12
	adc r12
	mov r12,2*2(r15)
	mov @r14+,r12
	adc r12
	mov r12,2*3(r15)
	mov @r14+,r12
	adc r12
	mov r12,2*4(r15)
	mov @r14+,r12
	adc r12
	mov r12,2*5(r15)
	mov @r14+,r12
	adc r12
	mov r12,2*6(r15)
	mov @r14+,r12
	adc r12
	mov r12,2*7(r15)
	mov @r14+,r12
	adc r12
	mov r12,2*8(r15)
	mov @r14+,r12
	adc r12
	mov r12,2*9(r15)
#if (FP_PRIME > 160)
	mov @r14+,r12
	adc r12
	mov r12,2*10(r15)
	mov @r14+,r12
	adc r12
	mov r12,2*11(r15)
	mov @r14+,r12
	adc r12
	mov r12,2*12(r15)
	mov @r14+,r12
	adc r12
	mov r12,2*13(r15)
	mov @r14+,r12
	adc r12
	mov r12,2*14(r15)
	mov @r14+,r12
	adc r12
	mov r12,2*15(r15)
#endif
	clr r15
	adc r15
	ret


fp_addn_low:
	mov @r14+,r12
	add @r13+,r12
	mov r12,@r15
	mov @r14+,r12
	addc @r13+,r12
	mov r12,2*1(r15)
	mov @r14+,r12
	addc @r13+,r12
	mov r12,2*2(r15)
	mov @r14+,r12
	addc @r13+,r12
	mov r12,2*3(r15)
	mov @r14+,r12
	addc @r13+,r12
	mov r12,2*4(r15)
	mov @r14+,r12
	addc @r13+,r12
	mov r12,2*5(r15)
	mov @r14+,r12
	addc @r13+,r12
	mov r12,2*6(r15)
	mov @r14+,r12
	addc @r13+,r12
	mov r12,2*7(r15)
	mov @r14+,r12
	addc @r13+,r12
	mov r12,2*8(r15)
	mov @r14+,r12
	addc @r13+,r12
	mov r12,2*9(r15)
#if (FP_PRIME > 160)
	mov @r14+,r12
	addc @r13+,r12
	mov r12,2*10(r15)
	mov @r14+,r12
	addc @r13+,r12
	mov r12,2*11(r15)
	mov @r14+,r12
	addc @r13+,r12
	mov r12,2*12(r15)
	mov @r14+,r12
	addc @r13+,r12
	mov r12,2*13(r15)
	mov @r14+,r12
	addc @r13+,r12
	mov r12,2*14(r15)
	mov @r14+,r12
	addc @r13+,r12
	mov r12,2*15(r15)
#endif
	clr r15
	adc r15
	ret

fp_dbln_low:
    mov r14,r13
    br #fp_addn_low

fp_addd_low:
    mov @r14+,r12
    add @r13+,r12
    mov r12,@r15
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*1(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*2(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*3(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*4(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*5(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*6(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*7(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*8(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*9(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*10(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*11(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*12(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*13(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*14(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*15(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*16(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*17(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*18(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*19(r15)
#if (FP_PRIME > 160)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*20(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*21(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*22(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*23(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*24(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*25(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*26(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*27(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*28(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*29(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*30(r15)
    mov @r14+,r12
    addc @r13+,r12
    mov r12,2*31(r15)
#endif
    clr r15
    adc r15
    ret

fp_sub1_low:
	mov @r14+,r12
	sub r13,r12
	mov r12,2*0(r15)
	mov @r14+,r12
	sbc r12
	mov r12,2*1(r15)
	mov @r14+,r12
	sbc r12
	mov r12,2*2(r15)
	mov @r14+,r12
	sbc r12
	mov r12,2*3(r15)
	mov @r14+,r12
	sbc r12
	mov r12,2*4(r15)
	mov @r14+,r12
	sbc r12
	mov r12,2*5(r15)
	mov @r14+,r12
	sbc r12
	mov r12,2*6(r15)
	mov @r14+,r12
	sbc r12
	mov r12,2*7(r15)
	mov @r14+,r12
	sbc r12
	mov r12,2*8(r15)
	mov @r14+,r12
	sbc r12
	mov r12,2*9(r15)
#if (FP_PRIME > 160)
	mov @r14+,r12
	sbc r12
	mov r12,2*10(r15)
	mov @r14+,r12
	sbc r12
	mov r12,2*11(r15)
	mov @r14+,r12
	sbc r12
	mov r12,2*12(r15)
	mov @r14+,r12
	sbc r12
	mov r12,2*13(r15)
	mov @r14+,r12
	sbc r12
	mov r12,2*14(r15)
	mov @r14+,r12
	sbc r12
	mov r12,2*15(r15)
#endif
	clr r15
	adc r15
	xor #1,r15
	ret

fp_subn_low:
	mov @r14+,r12
	sub @r13+,r12
	mov r12,@r15
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*1(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*2(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*3(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*4(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*5(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*6(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*7(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*8(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*9(r15)
#if (FP_PRIME > 160)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*10(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*11(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*12(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*13(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*14(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*15(r15)
#endif
	clr r15
	adc r15
	xor #1,r15
	ret

fp_subd_low:
	mov @r14+,r12
	sub @r13+,r12
	mov r12,@r15
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*1(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*2(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*3(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*4(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*5(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*6(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*7(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*8(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*9(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*10(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*11(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*12(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*13(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*14(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*15(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*16(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*17(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*18(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*19(r15)
#if (FP_PRIME > 160)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*20(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*21(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*22(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*23(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*24(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*25(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*26(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*27(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*28(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*29(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*30(r15)
	mov @r14+,r12
	subc @r13+,r12
	mov r12,2*31(r15)
#endif
	clr r15
	adc r15
	xor #1,r15
	ret
