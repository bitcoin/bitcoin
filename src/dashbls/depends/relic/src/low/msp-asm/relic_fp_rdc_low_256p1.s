fp_rdcs_low:
    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11

#include "fp_rdc_256p1.inc"

add1:
    tst r11
    jge skip1
    add #0xFFFF,0*2(r15)
    addc #0xFFFF,1*2(r15)
    addc #0xFFFF,2*2(r15)
    addc #0xFFFF,3*2(r15)
    addc #0xFFFF,4*2(r15)
    addc #0xFFFF,5*2(r15)
    adc 6*2(r15)
    adc 7*2(r15)
    adc 8*2(r15)
    adc 9*2(r15)
    adc 10*2(r15)
    adc 11*2(r15)
    addc #1,12*2(r15)
    adc 13*2(r15)
    addc #0xFFFF,14*2(r15)
    addc #0xFFFF,15*2(r15)
    adc r11
    jmp add1
skip1:

subtract2:
    tst r11
    jz skip2
    sub #0xFFFF,0*2(r15)
    subc #0xFFFF,1*2(r15)
    subc #0xFFFF,2*2(r15)
    subc #0xFFFF,3*2(r15)
    subc #0xFFFF,4*2(r15)
    subc #0xFFFF,5*2(r15)
    sbc 6*2(r15)
    sbc 7*2(r15)
    sbc 8*2(r15)
    sbc 9*2(r15)
    sbc 10*2(r15)
    sbc 11*2(r15)
    subc #1,12*2(r15)
    sbc 13*2(r15)
    subc #0xFFFF,14*2(r15)
    subc #0xFFFF,15*2(r15)
    sbc r11
    jmp subtract2
skip2:

    cmp #0xFFFF,2*15(r15)
    jlo skip3 /* m > z, jmp to DO NOT subtract */
    jne subtract3 /* m < z, jmp to subtract */
    cmp #0xFFFF,2*14(r15)
    jlo skip3
    jne subtract3
    cmp #0,2*13(r15)
    jlo skip3
    jne subtract3
    cmp #1,2*12(r15)
    jlo skip3
    jne subtract3
    cmp #0,2*11(r15)
    jlo skip3
    jne subtract3
    cmp #0,2*10(r15)
    jlo skip3
    jne subtract3
    cmp #0,2*9(r15)
    jlo skip3
    jne subtract3
    cmp #0,2*8(r15)
    jlo skip3
    jne subtract3
    cmp #0,2*7(r15)
    jlo skip3
    jne subtract3
    cmp #0,2*6(r15)
    jlo skip3
    jne subtract3
    cmp #0xFFFF,2*5(r15)
    jlo skip3
    jne subtract3
    cmp #0xFFFF,2*4(r15)
    jlo skip3
    jne subtract3
    cmp #0xFFFF,2*3(r15)
    jlo skip3
    jne subtract3
    cmp #0xFFFF,2*2(r15)
    jlo skip3
    jne subtract3
    cmp #0xFFFF,2*1(r15)
    jlo skip3
    jne subtract3
    cmp #0xFFFF,2*0(r15)
    jlo skip3
    /* subtract */
subtract3:
    /*** DECREMENT ***/
    sub #0xFFFF,0*2(r15)
    subc #0xFFFF,1*2(r15)
    subc #0xFFFF,2*2(r15)
    subc #0xFFFF,3*2(r15)
    subc #0xFFFF,4*2(r15)
    subc #0xFFFF,5*2(r15)
    sbc 6*2(r15)
    sbc 7*2(r15)
    sbc 8*2(r15)
    sbc 9*2(r15)
    sbc 10*2(r15)
    sbc 11*2(r15)
    subc #1,12*2(r15)
    sbc 13*2(r15)
    subc #0xFFFF,14*2(r15)
    subc #0xFFFF,15*2(r15)
skip3:
    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
    ret
