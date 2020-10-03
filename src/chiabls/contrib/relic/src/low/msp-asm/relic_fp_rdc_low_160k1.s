fp_rdcs_low:

    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11

    /* compute q0.c, add lo(qo.c) to r0 */
    mov #0x538D,&MPY

    add #2*10,r14

    mov @r14+,r13
    mov r13,&OP2
    mov &RESLO,r4
    mov &RESHI,r5
    mov r13,r6
    clr r7

    mov @r14+,r13
    mov r13,&OP2
    add &RESLO,r5
    addc &RESHI,r6
    addc r13,r7
    clr r8
    adc r8

    mov @r14+,r13
    mov r13,&OP2
    add &RESLO,r6
    addc &RESHI,r7
    addc r13,r8
    clr r9
    adc r9

    mov @r14+,r13
    mov r13,&OP2
    add &RESLO,r7
    addc &RESHI,r8
    addc r13,r9
    clr r10
    adc r10

    mov @r14+,r13
    mov r13,&OP2
    add &RESLO,r8
    addc &RESHI,r9
    addc r13,r10
    clr r11
    adc r11

    add r4,-2*15(r14)
    addc r5,-2*14(r14)
    addc r6,-2*13(r14)
    addc r7,-2*12(r14)
    addc r8,-2*11(r14)
    clr r12
    adc r12

    mov @r14+,r13
    mov r13,&OP2
    add &RESLO,r9
    addc &RESHI,r10
    addc r13,r11
    clr r4
    adc r4

    mov @r14+,r13
    mov r13,&OP2
    add &RESLO,r10
    addc &RESHI,r11
    addc r13,r4
    clr r5
    adc r5

    mov @r14+,r13
    mov r13,&OP2
    add &RESLO,r11
    addc &RESHI,r4
    addc r13,r5
    clr r6
    adc r6

    mov @r14+,r13
    mov r13,&OP2
    add &RESLO,r4
    addc &RESHI,r5
    addc r13,r6
    clr r7
    adc r7

    mov @r14+,r13
    mov r13,&OP2
    add &RESLO,r5
    addc &RESHI,r6
    addc r13,r7
    clr r8
    adc r8

    rrc r12
    addc r9,-2*15(r14)
    addc r10,-2*14(r14)
    addc r11,-2*13(r14)
    addc r4,-2*12(r14)
    addc r5,-2*11(r14)
    clr r12
    adc r12

    sub #2*20,r14


    /* q1 = high(q0.c) */
    /* compute q1.c, add lo(q1.c) to r1 */

    mov r6,&OP2
    mov &RESLO,r4
    mov &RESHI,r5

    mov r7,&OP2
    add &RESLO,r5
    addc &RESHI,r6
    adc r7
    clr r9
    adc r9

    mov r8,r13
    mov r13,&OP2
    add &RESLO,r6
    addc &RESHI,r7
    addc r9,r8
    clr r9
    adc r9

    add @r14+,r4
    mov r4,2*0(r15)
    addc @r14+,r5
    mov r5,2*1(r15)
    addc @r14+,r6
    mov r6,2*2(r15)
    addc @r14+,r7
    mov r7,2*3(r15)
    addc @r14+,r8
    mov r8,2*4(r15)
    addc @r14+,r9
    mov r9,2*5(r15)
    clr r10
    addc @r14+,r10
    mov r10,2*6(r15)
    clr r10
    addc @r14+,r10
    mov r10,2*7(r15)
    clr r10
    addc @r14+,r10
    mov r10,2*8(r15)
    clr r10
    addc @r14+,r10
    mov r10,2*9(r15)
    adc r12

    /* q2 = high(q1.c) = 0 */
subtract1:
    tst r12
    jz skip1
    sub #0xAC73,0*2(r15)
    subc #0xFFFF,1*2(r15)
    subc #0xFFFE,2*2(r15)
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
    cmp #0xFFFE,2*2(r15)
    jlo skip2
    jne subtract2
    cmp #0xFFFF,2*1(r15)
    jlo skip2
    jne subtract2
    cmp #0xAC73,2*0(r15)
    jlo skip2
    jne subtract2
    /* subtract */
subtract2:
    /*** DECREMENT ***/
    sub #0xAC73,0*2(r15)
    subc #0xFFFF,1*2(r15)
    subc #0xFFFE,2*2(r15)
    subc #0xFFFF,3*2(r15)
    subc #0xFFFF,4*2(r15)
    subc #0xFFFF,5*2(r15)
    subc #0xFFFF,6*2(r15)
    subc #0xFFFF,7*2(r15)
    subc #0xFFFF,8*2(r15)
    subc #0xFFFF,9*2(r15)
skip2:
    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
    ret
