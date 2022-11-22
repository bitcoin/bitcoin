
    push r2
    dint
    nop
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
    mov r4,&MPY
    mov r4,&OP2

    /* COLUMN_END */
    mov &RESLO,2*0(r14)
    mov &RESHI,&RESLO
    mov r12,&RESHI

    /* STEP_1 */
    mov r4,&MAC
    mov r5,&OP2
    add &SUMEXT,r12
    mov r5,&OP2

    /* COLUMN_END */
    mov &RESLO,2*1(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* STEP_1 */
    mov r4,&MAC
    mov r6,&OP2
    add &SUMEXT,r12
    mov r6,&OP2

    /* SELF_STEP */
    mov r5,&MAC
    add @r13,r12
    mov r5,&OP2

    /* COLUMN_END */
    mov &RESLO,2*2(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* STEP_1 */
    mov r4,&MAC
    mov r7,&OP2
    add &SUMEXT,r12
    mov r7,&OP2

    /* STEP_2MORE */
    mov r5,&MAC
    add @r13,r12
    mov r6,&OP2
    add &SUMEXT,r12
    mov r6,&OP2

    /* COLUMN_END */
    mov &RESLO,2*3(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* STEP_1 */
    mov r4,&MAC
    mov r8,&OP2
    add &SUMEXT,r12
    mov r8,&OP2

    /* STEP_2MORE */
    mov r5,&MAC
    add @r13,r12
    mov r7,&OP2
    add &SUMEXT,r12
    mov r7,&OP2

    /* SELF_STEP */
    mov r6,&MAC
    add @r13,r12
    mov r6,&OP2

    /* COLUMN_END */
    mov &RESLO,2*4(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* STEP_1 */
    mov r4,&MAC
    mov r9,&OP2
    add &SUMEXT,r12
    mov r9,&OP2

    /* STEP_2MORE */
    mov r5,&MAC
    add @r13,r12
    mov r8,&OP2
    add &SUMEXT,r12
    mov r8,&OP2

    /* STEP_2MORE */
    mov r6,&MAC
    add @r13,r12
    mov r7,&OP2
    add &SUMEXT,r12
    mov r7,&OP2

    /* COLUMN_END */
    mov &RESLO,2*5(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* STEP_1 */
    mov r4,&MAC
    mov r10,&OP2
    add &SUMEXT,r12
    mov r10,&OP2

    /* STEP_2MORE */
    mov r5,&MAC
    add @r13,r12
    mov r9,&OP2
    add &SUMEXT,r12
    mov r9,&OP2

    /* STEP_2MORE */
    mov r6,&MAC
    add @r13,r12
    mov r8,&OP2
    add &SUMEXT,r12
    mov r8,&OP2

    /* SELF_STEP */
    mov r7,&MAC
    add @r13,r12
    mov r7,&OP2

    /* COLUMN_END */
    mov &RESLO,2*6(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* STEP_1 */
    mov r4,&MAC
    mov r11,&OP2
    add &SUMEXT,r12
    mov r11,&OP2

    /* STEP_2MORE */
    mov r5,&MAC
    add @r13,r12
    mov r10,&OP2
    add &SUMEXT,r12
    mov r10,&OP2

    /* STEP_2MORE */
    mov r6,&MAC
    add @r13,r12
    mov r9,&OP2
    add &SUMEXT,r12
    mov r9,&OP2

    /* STEP_2MORE */
    mov r7,&MAC
    add @r13,r12
    mov r8,&OP2
    add &SUMEXT,r12
    mov r8,&OP2

    /* COLUMN_END */
    mov &RESLO,2*7(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* STEP_1 */
    mov r4,&MAC
    mov 2*8(r15),&OP2
    add &SUMEXT,r12
    mov 2*8(r15),&OP2

    /* STEP_2MORE */
    mov r5,&MAC
    add @r13,r12
    mov r11,&OP2
    add &SUMEXT,r12
    mov r11,&OP2

    /* STEP_2MORE */
    mov r6,&MAC
    add @r13,r12
    mov r10,&OP2
    add &SUMEXT,r12
    mov r10,&OP2

    /* STEP_2MORE */
    mov r7,&MAC
    add @r13,r12
    mov r9,&OP2
    add &SUMEXT,r12
    mov r9,&OP2

    /* SELF_STEP */
    mov r8,&MAC
    add @r13,r12
    mov r8,&OP2

    /* COLUMN_END */
    mov &RESLO,2*8(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* STEP_1 */
    mov r4,&MAC
    mov 2*9(r15),&OP2
    add &SUMEXT,r12
    mov 2*9(r15),&OP2

    /* STEP_2MORE */
    mov r5,&MAC
    add @r13,r12
    mov 2*8(r15),&OP2
    add &SUMEXT,r12
    mov 2*8(r15),&OP2

    /* STEP_2MORE */
    mov r6,&MAC
    add @r13,r12
    mov r11,&OP2
    add &SUMEXT,r12
    mov r11,&OP2

    /* STEP_2MORE */
    mov r7,&MAC
    add @r13,r12
    mov r10,&OP2
    add &SUMEXT,r12
    mov r10,&OP2

    /* STEP_2MORE */
    mov r8,&MAC
    add @r13,r12
    mov r9,&OP2
    add &SUMEXT,r12
    mov r9,&OP2

    /* COLUMN_END */
    mov &RESLO,2*9(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    mov 2*9(r15),r4

    /* STEP_1 */
    mov r5,&MAC
    mov r4,&OP2
    add &SUMEXT,r12
    mov r4,&OP2

    /* STEP_2MORE */
    mov r6,&MAC
    add @r13,r12
    mov 2*8(r15),&OP2
    add &SUMEXT,r12
    mov 2*8(r15),&OP2

    /* STEP_2MORE */
    mov r7,&MAC
    add @r13,r12
    mov r11,&OP2
    add &SUMEXT,r12
    mov r11,&OP2

    /* STEP_2MORE */
    mov r8,&MAC
    add @r13,r12
    mov r10,&OP2
    add &SUMEXT,r12
    mov r10,&OP2

    /* SELF_STEP */
    mov r9,&MAC
    add @r13,r12
    mov r9,&OP2

    /* COLUMN_END */
    mov &RESLO,2*10(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* STEP_1 */
    mov r6,&MAC
    mov r4,&OP2
    add &SUMEXT,r12
    mov r4,&OP2

    mov 2*8(r15),r5

    /* STEP_2MORE */
    mov r7,&MAC
    add @r13,r12
    mov r5,&OP2
    add &SUMEXT,r12
    mov r5,&OP2

    /* STEP_2MORE */
    mov r8,&MAC
    add @r13,r12
    mov r11,&OP2
    add &SUMEXT,r12
    mov r11,&OP2

    /* STEP_2MORE */
    mov r9,&MAC
    add @r13,r12
    mov r10,&OP2
    add &SUMEXT,r12
    mov r10,&OP2

    /* COLUMN_END */
    mov &RESLO,2*11(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* STEP_1 */
    mov r7,&MAC
    mov r4,&OP2
    add &SUMEXT,r12
    mov r4,&OP2

    /* STEP_2MORE */
    mov r8,&MAC
    add @r13,r12
    mov r5,&OP2
    add &SUMEXT,r12
    mov r5,&OP2

    /* STEP_2MORE */
    mov r9,&MAC
    add @r13,r12
    mov r11,&OP2
    add &SUMEXT,r12
    mov r11,&OP2

    /* SELF_STEP */
    mov r10,&MAC
    add @r13,r12
    mov r10,&OP2

    /* COLUMN_END */
    mov &RESLO,2*12(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* STEP_1 */
    mov r8,&MAC
    mov r4,&OP2
    add &SUMEXT,r12
    mov r4,&OP2

    /* STEP_2MORE */
    mov r9,&MAC
    add @r13,r12
    mov r5,&OP2
    add &SUMEXT,r12
    mov r5,&OP2

    /* STEP_2MORE */
    mov r10,&MAC
    add @r13,r12
    mov r11,&OP2
    add &SUMEXT,r12
    mov r11,&OP2

    /* COLUMN_END */
    mov &RESLO,2*13(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* STEP_1 */
    mov r9,&MAC
    mov r4,&OP2
    add &SUMEXT,r12
    mov r4,&OP2

    /* STEP_2MORE */
    mov r10,&MAC
    add @r13,r12
    mov r5,&OP2
    add &SUMEXT,r12
    mov r5,&OP2

    /* SELF_STEP */
    mov r11,&MAC
    add @r13,r12
    mov r11,&OP2

    /* COLUMN_END */
    mov &RESLO,2*14(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* STEP_1 */
    mov r10,&MAC
    mov r4,&OP2
    add &SUMEXT,r12
    mov r4,&OP2

    /* STEP_2MORE */
    mov r11,&MAC
    add @r13,r12
    mov r5,&OP2
    add &SUMEXT,r12
    mov r5,&OP2

    /* COLUMN_END */
    mov &RESLO,2*15(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* STEP_1 */
    mov r11,&MAC
    mov r4,&OP2
    add &SUMEXT,r12
    mov r4,&OP2

    /* SELF_STEP */
    mov r5,&MAC
    add @r13,r12
    mov r5,&OP2

    /* COLUMN_END */
    mov &RESLO,2*16(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* STEP_1 */
    mov r5,&MAC
    mov r4,&OP2
    add &SUMEXT,r12
    mov r4,&OP2

    /* COLUMN_END */
    mov &RESLO,2*17(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* SELF_STEP_1 */
    mov r4,&MAC
    mov r4,&OP2

    /* COLUMN_END */
    mov &RESLO,2*18(r14)
    add @r13,r12
    mov &RESHI,&RESLO
    mov r12,&RESHI
    clr r12

    /* END */
    mov &RESLO,2*19(r14)
    pop r2
    eint
