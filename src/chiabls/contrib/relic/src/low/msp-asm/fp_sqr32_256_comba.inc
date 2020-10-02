
    push r2
    dint
    nop
    bis #MPYDLYWRTEN,&MPY32CTL0
    bic #MPYDLY32,&MPY32CTL0
    mov #SUMEXT,r13
    clr r12

    mov @r15+,r4
    mov @r15+,r5

    mov @r15+,r6
    mov @r15+,r7

    mov @r15+,r8
    mov @r15+,r9

    mov @r15+,r10
    mov @r15+,r11

    sub #2*8,r15

    /* SELF_STEP_FIRST */
    mov r4,&MPY32L
    mov r5,&MPY32H
    mov r4,&OP2L
    mov r5,&OP2H

    /* COLUMN_END */
    mov &RES0,2*0(r14)
    mov &RES1,2*(0+1)(r14)
    mov &RES2,&RES0
    mov &RES3,&RES1
    mov r12,&RES2
    clr &RES3

    /* STEP_1 */
    mov r4,&MAC32L
    mov r5,&MAC32H
    mov r6,&OP2L
    mov r7,&OP2H
    add &SUMEXT,r12
    mov r6,&OP2L
    mov r7,&OP2H

    /* COLUMN_END */
    mov &RES0,2*2(r14)
    add @r13,r12
    mov &RES1,2*(2+1)(r14)
    mov &RES2,&RES0
    mov &RES3,&RES1
    mov r12,&RES2
    clr &RES3
    clr r12

    /* STEP_1 */
    mov r4,&MAC32L
    mov r5,&MAC32H
    mov r8,&OP2L
    mov r9,&OP2H
    add &SUMEXT,r12
    mov r8,&OP2L
    mov r9,&OP2H

    /* SELF_STEP */
    mov r6,&MAC32L
    mov r7,&MAC32H
    add @r13,r12
    mov r6,&OP2L
    mov r7,&OP2H

    /* COLUMN_END */
    mov &RES0,2*4(r14)
    add @r13,r12
    mov &RES1,2*(4+1)(r14)
    mov &RES2,&RES0
    mov &RES3,&RES1
    mov r12,&RES2
    clr &RES3
    clr r12

    /* STEP_1 */
    mov r4,&MAC32L
    mov r5,&MAC32H
    mov r10,&OP2L
    mov r11,&OP2H
    add &SUMEXT,r12
    mov r10,&OP2L
    mov r11,&OP2H

    /* STEP_2MORE */
    mov r6,&MAC32L
    mov r7,&MAC32H
    add @r13,r12
    mov r8,&OP2L
    mov r9,&OP2H
    add &SUMEXT,r12
    mov r8,&OP2L
    mov r9,&OP2H

    /* COLUMN_END */
    mov &RES0,2*6(r14)
    add @r13,r12
    mov &RES1,2*(6+1)(r14)
    mov &RES2,&RES0
    mov &RES3,&RES1
    mov r12,&RES2
    clr &RES3
    clr r12

    /* STEP_1 */
    mov r4,&MAC32L
    mov r5,&MAC32H
    mov 2*8(r15),&OP2L
    mov 2*9(r15),&OP2H
    add &SUMEXT,r12
    mov 2*8(r15),&OP2L
    mov 2*9(r15),&OP2H

    /* STEP_2MORE */
    mov r6,&MAC32L
    mov r7,&MAC32H
    add @r13,r12
    mov r10,&OP2L
    mov r11,&OP2H
    add &SUMEXT,r12
    mov r10,&OP2L
    mov r11,&OP2H

    /* SELF_STEP */
    mov r8,&MAC32L
    mov r9,&MAC32H
    add @r13,r12
    mov r8,&OP2L
    mov r9,&OP2H

    /* COLUMN_END */
    mov &RES0,2*8(r14)
    add @r13,r12
    mov &RES1,2*(8+1)(r14)
    mov &RES2,&RES0
    mov &RES3,&RES1
    mov r12,&RES2
    clr &RES3
    clr r12

    /* STEP_1 */
    mov r4,&MAC32L
    mov r5,&MAC32H
    mov 2*10(r15),&OP2L
    mov 2*11(r15),&OP2H
    add &SUMEXT,r12
    mov 2*10(r15),&OP2L
    mov 2*11(r15),&OP2H

    /* STEP_2MORE */
    mov r6,&MAC32L
    mov r7,&MAC32H
    add @r13,r12
    mov 2*8(r15),&OP2L
    mov 2*9(r15),&OP2H
    add &SUMEXT,r12
    mov 2*8(r15),&OP2L
    mov 2*9(r15),&OP2H

    /* STEP_2MORE */
    mov r8,&MAC32L
    mov r9,&MAC32H
    add @r13,r12
    mov r10,&OP2L
    mov r11,&OP2H
    add &SUMEXT,r12
    mov r10,&OP2L
    mov r11,&OP2H

    /* COLUMN_END */
    mov &RES0,2*10(r14)
    add @r13,r12
    mov &RES1,2*(10+1)(r14)
    mov &RES2,&RES0
    mov &RES3,&RES1
    mov r12,&RES2
    clr &RES3
    clr r12

    /* STEP_1 */
    mov r4,&MAC32L
    mov r5,&MAC32H
    mov 2*12(r15),&OP2L
    mov 2*13(r15),&OP2H
    add &SUMEXT,r12
    mov 2*12(r15),&OP2L
    mov 2*13(r15),&OP2H

    /* STEP_2MORE */
    mov r6,&MAC32L
    mov r7,&MAC32H
    add @r13,r12
    mov 2*10(r15),&OP2L
    mov 2*11(r15),&OP2H
    add &SUMEXT,r12
    mov 2*10(r15),&OP2L
    mov 2*11(r15),&OP2H

    /* STEP_2MORE */
    mov r8,&MAC32L
    mov r9,&MAC32H
    add @r13,r12
    mov 2*8(r15),&OP2L
    mov 2*9(r15),&OP2H
    add &SUMEXT,r12
    mov 2*8(r15),&OP2L
    mov 2*9(r15),&OP2H

    /* SELF_STEP */
    mov r10,&MAC32L
    mov r11,&MAC32H
    add @r13,r12
    mov r10,&OP2L
    mov r11,&OP2H

    /* COLUMN_END */
    mov &RES0,2*12(r14)
    add @r13,r12
    mov &RES1,2*(12+1)(r14)
    mov &RES2,&RES0
    mov &RES3,&RES1
    mov r12,&RES2
    clr &RES3
    clr r12

    /* STEP_1 */
    mov r4,&MAC32L
    mov r5,&MAC32H
    mov 2*14(r15),&OP2L
    mov 2*15(r15),&OP2H
    add &SUMEXT,r12
    mov 2*14(r15),&OP2L
    mov 2*15(r15),&OP2H

    /* STEP_2MORE */
    mov r6,&MAC32L
    mov r7,&MAC32H
    add @r13,r12
    mov 2*12(r15),&OP2L
    mov 2*13(r15),&OP2H
    add &SUMEXT,r12
    mov 2*12(r15),&OP2L
    mov 2*13(r15),&OP2H

    /* STEP_2MORE */
    mov r8,&MAC32L
    mov r9,&MAC32H
    add @r13,r12
    mov 2*10(r15),&OP2L
    mov 2*11(r15),&OP2H
    add &SUMEXT,r12
    mov 2*10(r15),&OP2L
    mov 2*11(r15),&OP2H

    /* STEP_2MORE */
    mov r10,&MAC32L
    mov r11,&MAC32H
    add @r13,r12
    mov 2*8(r15),&OP2L
    mov 2*9(r15),&OP2H
    add &SUMEXT,r12
    mov 2*8(r15),&OP2L
    mov 2*9(r15),&OP2H

    /* COLUMN_END */
    mov &RES0,2*14(r14)
    add @r13,r12
    mov &RES1,2*(14+1)(r14)
    mov &RES2,&RES0
    mov &RES3,&RES1
    mov r12,&RES2
    clr &RES3
    clr r12

    mov 2*14(r15),r4
    mov 2*(14+1)(r15),r5

    /* STEP_1 */
    mov r6,&MAC32L
    mov r7,&MAC32H
    mov r4,&OP2L
    mov r5,&OP2H
    add &SUMEXT,r12
    mov r4,&OP2L
    mov r5,&OP2H

    /* STEP_2MORE */
    mov r8,&MAC32L
    mov r9,&MAC32H
    add @r13,r12
    mov 2*12(r15),&OP2L
    mov 2*13(r15),&OP2H
    add &SUMEXT,r12
    mov 2*12(r15),&OP2L
    mov 2*13(r15),&OP2H

    /* STEP_2MORE */
    mov r10,&MAC32L
    mov r11,&MAC32H
    add @r13,r12
    mov 2*10(r15),&OP2L
    mov 2*11(r15),&OP2H
    add &SUMEXT,r12
    mov 2*10(r15),&OP2L
    mov 2*11(r15),&OP2H

    /* SELF_STEP */
    mov 2*8(r15),&MAC32L
    mov 2*9(r15),&MAC32H
    add @r13,r12
    mov 2*8(r15),&OP2L
    mov 2*9(r15),&OP2H

    /* COLUMN_END */
    mov &RES0,2*16(r14)
    add @r13,r12
    mov &RES1,2*(16+1)(r14)
    mov &RES2,&RES0
    mov &RES3,&RES1
    mov r12,&RES2
    clr &RES3
    clr r12

    /* STEP_1 */
    mov r8,&MAC32L
    mov r9,&MAC32H
    mov r4,&OP2L
    mov r5,&OP2H
    add &SUMEXT,r12
    mov r4,&OP2L
    mov r5,&OP2H

    mov 2*12(r15),r6
    mov 2*(12+1)(r15),r7

    /* STEP_2MORE */
    mov r10,&MAC32L
    mov r11,&MAC32H
    add @r13,r12
    mov r6,&OP2L
    mov r7,&OP2H
    add &SUMEXT,r12
    mov r6,&OP2L
    mov r7,&OP2H

    /* STEP_2MORE */
    mov 2*8(r15),&MAC32L
    mov 2*9(r15),&MAC32H
    add @r13,r12
    mov 2*10(r15),&OP2L
    mov 2*11(r15),&OP2H
    add &SUMEXT,r12
    mov 2*10(r15),&OP2L
    mov 2*11(r15),&OP2H

    /* COLUMN_END */
    mov &RES0,2*18(r14)
    add @r13,r12
    mov &RES1,2*(18+1)(r14)
    mov &RES2,&RES0
    mov &RES3,&RES1
    mov r12,&RES2
    clr &RES3
    clr r12

    /* STEP_1 */
    mov r10,&MAC32L
    mov r11,&MAC32H
    mov r4,&OP2L
    mov r5,&OP2H
    add &SUMEXT,r12
    mov r4,&OP2L
    mov r5,&OP2H

    /* STEP_2MORE */
    mov 2*8(r15),&MAC32L
    mov 2*9(r15),&MAC32H
    add @r13,r12
    mov r6,&OP2L
    mov r7,&OP2H
    add &SUMEXT,r12
    mov r6,&OP2L
    mov r7,&OP2H

    mov 2*10(r15),r8
    mov 2*(10+1)(r15),r9

    /* SELF_STEP */
    mov r8,&MAC32L
    mov r9,&MAC32H
    add @r13,r12
    mov r8,&OP2L
    mov r9,&OP2H

    /* COLUMN_END */
    mov &RES0,2*20(r14)
    add @r13,r12
    mov &RES1,2*(20+1)(r14)
    mov &RES2,&RES0
    mov &RES3,&RES1
    mov r12,&RES2
    clr &RES3
    clr r12

    mov 2*8(r15),r10
    mov 2*(8+1)(r15),r11

    /* STEP_1 */
    mov r10,&MAC32L
    mov r11,&MAC32H
    mov r4,&OP2L
    mov r5,&OP2H
    add &SUMEXT,r12
    mov r4,&OP2L
    mov r5,&OP2H

    /* STEP_2MORE */
    mov r8,&MAC32L
    mov r9,&MAC32H
    add @r13,r12
    mov r6,&OP2L
    mov r7,&OP2H
    add &SUMEXT,r12
    mov r6,&OP2L
    mov r7,&OP2H

    /* COLUMN_END */
    mov &RES0,2*22(r14)
    add @r13,r12
    mov &RES1,2*(22+1)(r14)
    mov &RES2,&RES0
    mov &RES3,&RES1
    mov r12,&RES2
    clr &RES3
    clr r12

    /* STEP_1 */
    mov r8,&MAC32L
    mov r9,&MAC32H
    mov r4,&OP2L
    mov r5,&OP2H
    add &SUMEXT,r12
    mov r4,&OP2L
    mov r5,&OP2H

    /* SELF_STEP */
    mov r6,&MAC32L
    mov r7,&MAC32H
    add @r13,r12
    mov r6,&OP2L
    mov r7,&OP2H

    /* COLUMN_END */
    mov &RES0,2*24(r14)
    add @r13,r12
    mov &RES1,2*(24+1)(r14)
    mov &RES2,&RES0
    mov &RES3,&RES1
    mov r12,&RES2
    clr &RES3
    clr r12

    /* STEP_1 */
    mov r6,&MAC32L
    mov r7,&MAC32H
    mov r4,&OP2L
    mov r5,&OP2H
    add &SUMEXT,r12
    mov r4,&OP2L
    mov r5,&OP2H

    /* COLUMN_END */
    mov &RES0,2*26(r14)
    add @r13,r12
    mov &RES1,2*(26+1)(r14)
    mov &RES2,&RES0
    mov &RES3,&RES1
    mov r12,&RES2
    clr &RES3
    clr r12

    /* SELF_STEP_1 */
    mov r4,&MAC32L
    mov r5,&MAC32H
    mov r4,&OP2L
    mov r5,&OP2H

    /* COLUMN_END */
    mov &RES0,2*28(r14)
    add @r13,r12
    mov &RES1,2*(28+1)(r14)
    mov &RES2,&RES0
    mov &RES3,&RES1
    mov r12,&RES2
    clr &RES3
    clr r12

    /* END */
    mov &RES0,2*30(r14)
    mov &RES1,2*(30+1)(r14)
    pop r2
    eint
