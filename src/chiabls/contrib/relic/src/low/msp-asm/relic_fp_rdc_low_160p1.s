fp_rdcs_low:
    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11

    /* create temp area */
    sub #2*4,r1

    /* shift right q0 */
    clrc
    mov 2*19(r14),r13
    rrc r13
    mov r13,2*3(r1)
    mov 2*18(r14),r13
    rrc r13
    mov r13,2*2(r1)
    mov 2*17(r14),r13
    rrc r13
    mov r13,2*1(r1)
    mov 2*16(r14),r13
    rrc r13
    mov r13,2*0(r1)
    mov 2*15(r14),r13
    rrc r13
    mov r13,r11
    mov 2*14(r14),r13
    rrc r13
    mov r13,r10
    mov 2*13(r14),r13
    rrc r13
    mov r13,r9
    mov 2*12(r14),r13
    rrc r13
    mov r13,r8
    mov 2*11(r14),r13
    rrc r13
    mov r13,r7
    mov 2*10(r14),r13
    rrc r13
    mov r13,r6
    clr r13
    rrc r13
    mov r13,r5
    clr r4

    /* compute q0.c: add q0 to q0 << 31 */
    add #2*10,r14
    add @r14+,r4
    addc @r14+,r5
    addc @r14+,r6
    addc @r14+,r7
    addc @r14+,r8
    addc @r14+,r9
    addc @r14+,r10
    addc @r14+,r11
    addc @r14+,2*0(r1)
    addc @r14+,2*1(r1)
    adc 2*2(r1)
    adc 2*3(r1)
    sub #2*20,r14

    /* r = r + low(q0.c) */
    mov @r14+,r13
    add r4,r13
    mov r13,2*0(r15)
    mov @r14+,r13
    addc r5,r13
    mov r13,2*1(r15)
    mov @r14+,r13
    addc r6,r13
    mov r13,2*2(r15)
    mov @r14+,r13
    addc r7,r13
    mov r13,2*3(r15)
    mov @r14+,r13
    addc r8,r13
    mov r13,2*4(r15)
    mov @r14+,r13
    addc r9,r13
    mov r13,2*5(r15)
    mov @r14+,r13
    addc r10,r13
    mov r13,2*6(r15)
    mov @r14+,r13
    addc r11,r13
    mov r13,2*7(r15)
    mov @r14+,r13
    addc 2*0(r1),r13
    mov r13,2*8(r15)
    mov @r14+,r13
    addc 2*1(r1),r13
    mov r13,2*9(r15)
    clr r12
    adc r12
    sub #2*10,r14

    /* TODO: check if q1 is 0 */

    /* q1 = high(q0.c) */
    /* shift right q1 */
    clrc
    mov 2*3(r1),r13
    rrc r13
    mov r13,r7
    mov 2*2(r1),r13
    rrc r13
    mov r13,r6
    clr r13
    rrc r13
    mov r13,r5
    clr r4

    /* compute q1.c: add q1 to q1 << 31 */
    add 2*2(r1),r4
    addc 2*3(r1),r5
    adc r6
    adc r7

    /* r = r + low(q1.c) */
    add r4,2*0(r15)
    addc r5,2*1(r15)
    addc r6,2*2(r15)
    addc r7,2*3(r15)
    adc 2*4(r15)
    adc 2*5(r15)
    adc 2*6(r15)
    adc 2*7(r15)
    adc 2*8(r15)
    adc 2*9(r15)
    adc r12

    /* q2 = high(q1.c) = 0 */

subtract1:
    tst r12
    jz skip1
    sub #0xFFFF,0*2(r15)
    subc #0x7FFF,1*2(r15)
    subc #0xFFFF,2*2(r15)
    subc #0xFFFF,3*2(r15)
    subc #0xFFFF,4*2(r15)
    subc #0xFFFF,5*2(r15)
    subc #0xFFFF,6*2(r15)
    subc #0xFFFF,7*2(r15)
    subc #0xFFFF,8*2(r15)
    subc #0xFFFF,9*2(r15)
    sbc r12
    jmp subtract1
skip1:

    cmp #0xFFFF,2*9(r15)
    jlo skip2 /* m > z, jmp to DO NOT subtract */
    jne subtract2 /* m < z, jmp to subtract */
    cmp #0xFFFF,2*8(r15)
    jlo skip2
    jne subtract2
    cmp #0xFFFF,2*7(r15)
    jlo skip2
    jne subtract2
    cmp #0xFFFF,2*6(r15)
    jlo skip2
    jne subtract2
    cmp #0xFFFF,2*5(r15)
    jlo skip2
    jne subtract2
    cmp #0xFFFF,2*4(r15)
    jlo skip2
    jne subtract2
    cmp #0xFFFF,2*3(r15)
    jlo skip2
    jne subtract2
    cmp #0xFFFF,2*2(r15)
    jlo skip2
    jne subtract2
    cmp #0x7FFF,2*1(r15)
    jlo skip2
    jne subtract2
    cmp #0xFFFF,2*0(r15)
    jlo skip2
    jne subtract2
    /* subtract */
subtract2:
    /*** DECREMENT ***/
    sub #0xFFFF,0*2(r15)
    subc #0x7FFF,1*2(r15)
    subc #0xFFFF,2*2(r15)
    subc #0xFFFF,3*2(r15)
    subc #0xFFFF,4*2(r15)
    subc #0xFFFF,5*2(r15)
    subc #0xFFFF,6*2(r15)
    subc #0xFFFF,7*2(r15)
    subc #0xFFFF,8*2(r15)
    subc #0xFFFF,9*2(r15)
skip2:
    /* delete temp area */
    add #2*4,r1
    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
    ret
