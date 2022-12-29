
    push r2
    dint
    nop
    bis #MPYDLYWRTEN,&MPY32CTL0
    bic #MPYDLY32,&MPY32CTL0
    mov #SUMEXT,r12
    mov #RES0,r10
    clr r11

    /* STEP_FIRST */
    mov @r15+,&MPY32L
    mov @r15+,&MPY32H
    
    mov 2*0(r14),&OP2L
    mov 2*(0+1)(r14),&OP2H


    /* COLUMN_END_FIRST */
    nop
    mov @r10+,2*0(r13)
    mov @r10+,2*(0+1)(r13)
    mov @r10+,&RES0
    mov @r10+,&RES1
    sub #2*4,r10
    mov r11,&RES2
    clr &RES3

    /* DEC */
    sub #2*2,r15

    /* STEP_SECOND / STEP_THIRD_MORE_1 */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    
    mov 2*2(r14),r4
    mov 2*(2+1)(r14),r5

    mov r4,&OP2L
    mov r5,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*0(r14),&OP2L
    mov 2*(0+1)(r14),&OP2H


    /* COLUMN_END_SECOND_MORE */
    nop
    mov @r10+,2*2(r13)
    add @r12,r11
    mov @r10+,2*(2+1)(r13)
    mov @r10+,&RES0
    mov @r10+,&RES1
    sub #2*4,r10
    mov r11,&RES2
    clr r11
    clr &RES3

    /* DEC */
    sub #2*4,r15

    /* STEP_SECOND / STEP_THIRD_MORE_1 */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    
    mov 2*4(r14),r6
    mov 2*(4+1)(r14),r7

    mov r6,&OP2L
    mov r7,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r4,&OP2L
    mov r5,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*0(r14),&OP2L
    mov 2*(0+1)(r14),&OP2H


    /* COLUMN_END_SECOND_MORE */
    nop
    mov @r10+,2*4(r13)
    add @r12,r11
    mov @r10+,2*(4+1)(r13)
    mov @r10+,&RES0
    mov @r10+,&RES1
    sub #2*4,r10
    mov r11,&RES2
    clr r11
    clr &RES3

    /* DEC */
    sub #2*6,r15

    /* STEP_SECOND / STEP_THIRD_MORE_1 */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    
    mov 2*6(r14),r8
    mov 2*(6+1)(r14),r9

    mov r8,&OP2L
    mov r9,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r6,&OP2L
    mov r7,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r4,&OP2L
    mov r5,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*0(r14),&OP2L
    mov 2*(0+1)(r14),&OP2H


    /* COLUMN_END_SECOND_MORE */
    nop
    mov @r10+,2*6(r13)
    add @r12,r11
    mov @r10+,2*(6+1)(r13)
    mov @r10+,&RES0
    mov @r10+,&RES1
    sub #2*4,r10
    mov r11,&RES2
    clr r11
    clr &RES3

    /* DEC */
    sub #2*8,r15

    /* STEP_SECOND / STEP_THIRD_MORE_1 */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    
    mov 2*8(r14),&OP2L
    mov 2*(8+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r8,&OP2L
    mov r9,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r6,&OP2L
    mov r7,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r4,&OP2L
    mov r5,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*0(r14),&OP2L
    mov 2*(0+1)(r14),&OP2H


    /* COLUMN_END_SECOND_MORE */
    nop
    mov @r10+,2*8(r13)
    add @r12,r11
    mov @r10+,2*(8+1)(r13)
    mov @r10+,&RES0
    mov @r10+,&RES1
    sub #2*4,r10
    mov r11,&RES2
    clr r11
    clr &RES3

    /* DEC */
    sub #2*10,r15

    /* STEP_SECOND / STEP_THIRD_MORE_1 */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    
    mov 2*10(r14),&OP2L
    mov 2*(10+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*8(r14),&OP2L
    mov 2*(8+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r8,&OP2L
    mov r9,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r6,&OP2L
    mov r7,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r4,&OP2L
    mov r5,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*0(r14),&OP2L
    mov 2*(0+1)(r14),&OP2H


    /* COLUMN_END_SECOND_MORE */
    nop
    mov @r10+,2*10(r13)
    add @r12,r11
    mov @r10+,2*(10+1)(r13)
    mov @r10+,&RES0
    mov @r10+,&RES1
    sub #2*4,r10
    mov r11,&RES2
    clr r11
    clr &RES3

    /* DEC */
    sub #2*12,r15

    /* STEP_SECOND / STEP_THIRD_MORE_1 */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    
    mov 2*12(r14),&OP2L
    mov 2*(12+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*10(r14),&OP2L
    mov 2*(10+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*8(r14),&OP2L
    mov 2*(8+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r8,&OP2L
    mov r9,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r6,&OP2L
    mov r7,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r4,&OP2L
    mov r5,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*0(r14),&OP2L
    mov 2*(0+1)(r14),&OP2H


    /* COLUMN_END_SECOND_MORE */
    nop
    mov @r10+,2*12(r13)
    add @r12,r11
    mov @r10+,2*(12+1)(r13)
    mov @r10+,&RES0
    mov @r10+,&RES1
    sub #2*4,r10
    mov r11,&RES2
    clr r11
    clr &RES3

    /* DEC */
    sub #2*14,r15

    /* STEP_SECOND / STEP_THIRD_MORE_1 */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    
    mov 2*14(r14),&OP2L
    mov 2*(14+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*12(r14),&OP2L
    mov 2*(12+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*10(r14),&OP2L
    mov 2*(10+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*8(r14),&OP2L
    mov 2*(8+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r8,&OP2L
    mov r9,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r6,&OP2L
    mov r7,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r4,&OP2L
    mov r5,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*0(r14),&OP2L
    mov 2*(0+1)(r14),&OP2H


    /* COLUMN_END_SECOND_MORE */
    nop
    mov @r10+,2*14(r13)
    add @r12,r11
    mov @r10+,2*(14+1)(r13)
    mov @r10+,&RES0
    mov @r10+,&RES1
    sub #2*4,r10
    mov r11,&RES2
    clr r11
    clr &RES3

    /* DEC */
    sub #2*14,r15

    /* STEP_SECOND / STEP_THIRD_MORE_1 */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    
    mov 2*14(r14),&OP2L
    mov 2*(14+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*12(r14),&OP2L
    mov 2*(12+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*10(r14),&OP2L
    mov 2*(10+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*8(r14),&OP2L
    mov 2*(8+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r8,&OP2L
    mov r9,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r6,&OP2L
    mov r7,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r4,&OP2L
    mov r5,&OP2H


    /* COLUMN_END_SECOND_MORE */
    nop
    mov @r10+,2*16(r13)
    add @r12,r11
    mov @r10+,2*(16+1)(r13)
    mov @r10+,&RES0
    mov @r10+,&RES1
    sub #2*4,r10
    mov r11,&RES2
    clr r11
    clr &RES3

    /* DEC */
    sub #2*12,r15

    /* STEP_SECOND / STEP_THIRD_MORE_1 */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    
    mov 2*14(r14),r4
    mov 2*(14+1)(r14),r5

    mov r4,&OP2L
    mov r5,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*12(r14),&OP2L
    mov 2*(12+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*10(r14),&OP2L
    mov 2*(10+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*8(r14),&OP2L
    mov 2*(8+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r8,&OP2L
    mov r9,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r6,&OP2L
    mov r7,&OP2H


    /* COLUMN_END_SECOND_MORE */
    nop
    mov @r10+,2*18(r13)
    add @r12,r11
    mov @r10+,2*(18+1)(r13)
    mov @r10+,&RES0
    mov @r10+,&RES1
    sub #2*4,r10
    mov r11,&RES2
    clr r11
    clr &RES3

    /* DEC */
    sub #2*10,r15

    /* STEP_SECOND / STEP_THIRD_MORE_1 */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    
    mov r4,&OP2L
    mov r5,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*12(r14),r6
    mov 2*(12+1)(r14),r7

    mov r6,&OP2L
    mov r7,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*10(r14),&OP2L
    mov 2*(10+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*8(r14),&OP2L
    mov 2*(8+1)(r14),&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r8,&OP2L
    mov r9,&OP2H


    /* COLUMN_END_SECOND_MORE */
    nop
    mov @r10+,2*20(r13)
    add @r12,r11
    mov @r10+,2*(20+1)(r13)
    mov @r10+,&RES0
    mov @r10+,&RES1
    sub #2*4,r10
    mov r11,&RES2
    clr r11
    clr &RES3

    /* DEC */
    sub #2*8,r15

    /* STEP_SECOND / STEP_THIRD_MORE_1 */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    
    mov r4,&OP2L
    mov r5,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r6,&OP2L
    mov r7,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*10(r14),r8
    mov 2*(10+1)(r14),r9

    mov r8,&OP2L
    mov r9,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov 2*8(r14),&OP2L
    mov 2*(8+1)(r14),&OP2H


    /* COLUMN_END_SECOND_MORE */
    nop
    mov @r10+,2*22(r13)
    add @r12,r11
    mov @r10+,2*(22+1)(r13)
    mov @r10+,&RES0
    mov @r10+,&RES1
    sub #2*4,r10
    mov r11,&RES2
    clr r11
    clr &RES3

    /* DEC */
    sub #2*6,r15

    /* STEP_SECOND / STEP_THIRD_MORE_1 */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    
    mov r4,&OP2L
    mov r5,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r6,&OP2L
    mov r7,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r8,&OP2L
    mov r9,&OP2H


    /* COLUMN_END_SECOND_MORE */
    nop
    mov @r10+,2*24(r13)
    add @r12,r11
    mov @r10+,2*(24+1)(r13)
    mov @r10+,&RES0
    mov @r10+,&RES1
    sub #2*4,r10
    mov r11,&RES2
    clr r11
    clr &RES3

    /* DEC */
    sub #2*4,r15

    /* STEP_SECOND / STEP_THIRD_MORE_1 */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    
    mov r4,&OP2L
    mov r5,&OP2H


    /* STEP_THIRD_MORE */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    add @r12,r11
    
    mov r6,&OP2L
    mov r7,&OP2H


    /* COLUMN_END_SECOND_MORE */
    nop
    mov @r10+,2*26(r13)
    add @r12,r11
    mov @r10+,2*(26+1)(r13)
    mov @r10+,&RES0
    mov @r10+,&RES1
    sub #2*4,r10
    mov r11,&RES2
    clr r11
    clr &RES3

    /* DEC */
    sub #2*2,r15

    /* STEP_SECOND / STEP_THIRD_MORE_1 */
    mov @r15+,&MAC32L
    mov @r15+,&MAC32H
    
    mov r4,&OP2L
    mov r5,&OP2H


    /* COLUMN_END_SECOND_MORE */
    nop
    mov @r10+,2*28(r13)
    add @r12,r11
    mov @r10+,2*(28+1)(r13)
    mov @r10+,&RES0
    mov @r10+,&RES1
    sub #2*4,r10
    mov r11,&RES2
    clr r11
    clr &RES3

    /* DEC */
    sub #2*16,r15

    /* END */
    mov @r10+,2*30(r13)
    mov @r10,2*(30+1)(r13)
    pop r2
    eint
