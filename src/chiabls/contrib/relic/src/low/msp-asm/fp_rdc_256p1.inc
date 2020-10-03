
    clr r11

    mov r11,r12
    clr r11
    tst r12
    jge sk0
    mov #-1,r11
    sk0:

    /* noncached */
    add 2*0(r14),r12
    adc r11

    /* noncached */
    add 2*16(r14),r12
    adc r11

    /* noncached */
    add 2*18(r14),r12
    adc r11

    /* noncached */
    sub 2*22(r14),r12
    adc r11

    /* save cache */
    mov 2*24(r14),r13
    sub r13,r12
    adc r11

    /* save cache */
    mov 2*26(r14),r9
    sub r9,r12
    adc r11

    /* save cache */
    mov 2*28(r14),r7
    sub r7,r12
    adc r11

    sub #4,r11
    mov r12,2*0(r15)

    mov r11,r12
    clr r11
    tst r12
    jge sk1
    mov #-1,r11
    sk1:

    /* noncached */
    add 2*1(r14),r12
    adc r11

    /* noncached */
    add 2*17(r14),r12
    adc r11

    /* noncached */
    add 2*19(r14),r12
    adc r11

    /* noncached */
    sub 2*23(r14),r12
    adc r11

    /* save cache */
    mov 2*25(r14),r10
    sub r10,r12
    adc r11

    /* save cache */
    mov 2*27(r14),r8
    sub r8,r12
    adc r11

    /* save cache */
    mov 2*29(r14),r6
    sub r6,r12
    adc r11

    sub #4,r11
    mov r12,2*1(r15)

    mov r11,r12
    clr r11
    tst r12
    jge sk2
    mov #-1,r11
    sk2:

    /* noncached */
    add 2*2(r14),r12
    adc r11

    /* noncached */
    add 2*18(r14),r12
    adc r11

    /* noncached */
    add 2*20(r14),r12
    adc r11

    /* cached */
    sub r13,r12
    adc r11

    /* cached */
    sub r9,r12
    adc r11

    /* cached */
    sub r7,r12
    adc r11

    /* save cache */
    mov 2*30(r14),r5
    sub r5,r12
    adc r11

    sub #4,r11
    mov r12,2*2(r15)

    mov r11,r12
    clr r11
    tst r12
    jge sk3
    mov #-1,r11
    sk3:

    /* noncached */
    add 2*3(r14),r12
    adc r11

    /* noncached */
    add 2*19(r14),r12
    adc r11

    /* noncached */
    add 2*21(r14),r12
    adc r11

    /* cached */
    sub r10,r12
    adc r11

    /* cached */
    sub r8,r12
    adc r11

    /* cached */
    sub r6,r12
    adc r11

    /* save cache */
    mov 2*31(r14),r4
    sub r4,r12
    adc r11

    sub #4,r11
    mov r12,2*3(r15)

    mov r11,r12
    clr r11
    tst r12
    jge sk4
    mov #-1,r11
    sk4:

    /* noncached */
    add 2*4(r14),r12
    adc r11

    /* noncached */
    add 2*20(r14),r12
    adc r11

    /* noncached */
    add 2*22(r14),r12
    adc r11

    /* cached */
    sub r9,r12
    adc r11

    /* cached */
    sub r7,r12
    adc r11

    /* cached */
    sub r5,r12
    adc r11

    sub #3,r11
    mov r12,2*4(r15)

    mov r11,r12
    clr r11
    tst r12
    jge sk5
    mov #-1,r11
    sk5:

    /* noncached */
    add 2*5(r14),r12
    adc r11

    /* noncached */
    add 2*21(r14),r12
    adc r11

    /* noncached */
    add 2*23(r14),r12
    adc r11

    /* cached */
    sub r8,r12
    adc r11

    /* cached */
    sub r6,r12
    adc r11

    /* cached */
    sub r4,r12
    adc r11

    sub #3,r11
    mov r12,2*5(r15)

    mov r11,r12
    clr r11
    tst r12
    jge sk6
    mov #-1,r11
    sk6:

    /* noncached */
    add 2*6(r14),r12
    adc r11

    /* noncached */
    add 2*22(r14),r12
    adc r11

    /* noncached */
    add 2*22(r14),r12
    adc r11

    /* cached */
    add r13,r12
    adc r11

    /* cached */
    add r13,r12
    adc r11

    /* cached */
    add r9,r12
    adc r11

    /* cached */
    sub r5,r12
    adc r11

    /* noncached */
    sub 2*16(r14),r12
    adc r11

    /* noncached */
    sub 2*18(r14),r12
    adc r11

    sub #3,r11
    mov r12,2*6(r15)

    mov r11,r12
    clr r11
    tst r12
    jge sk7
    mov #-1,r11
    sk7:

    /* noncached */
    add 2*7(r14),r12
    adc r11

    /* noncached */
    add 2*23(r14),r12
    adc r11

    /* noncached */
    add 2*23(r14),r12
    adc r11

    /* cached */
    add r10,r12
    adc r11

    /* cached */
    add r10,r12
    adc r11

    /* cached */
    add r8,r12
    adc r11

    /* cached */
    sub r4,r12
    adc r11

    /* noncached */
    sub 2*17(r14),r12
    adc r11

    /* noncached */
    sub 2*19(r14),r12
    adc r11

    sub #3,r11
    mov r12,2*7(r15)

    mov r11,r12
    clr r11
    tst r12
    jge sk8
    mov #-1,r11
    sk8:

    /* noncached */
    add 2*8(r14),r12
    adc r11

    /* cached */
    add r13,r12
    adc r11

    /* cached */
    add r13,r12
    adc r11

    /* cached */
    add r9,r12
    adc r11

    /* cached */
    add r9,r12
    adc r11

    /* cached */
    add r7,r12
    adc r11

    /* noncached */
    sub 2*18(r14),r12
    adc r11

    /* noncached */
    sub 2*20(r14),r12
    adc r11

    sub #2,r11
    mov r12,2*8(r15)

    mov r11,r12
    clr r11
    tst r12
    jge sk9
    mov #-1,r11
    sk9:

    /* noncached */
    add 2*9(r14),r12
    adc r11

    /* cached */
    add r10,r12
    adc r11

    /* cached */
    add r10,r12
    adc r11

    /* cached */
    add r8,r12
    adc r11

    /* cached */
    add r8,r12
    adc r11

    /* cached */
    add r6,r12
    adc r11

    /* noncached */
    sub 2*19(r14),r12
    adc r11

    /* noncached */
    sub 2*21(r14),r12
    adc r11

    sub #2,r11
    mov r12,2*9(r15)

    mov r11,r12
    clr r11
    tst r12
    jge sk10
    mov #-1,r11
    sk10:

    /* noncached */
    add 2*10(r14),r12
    adc r11

    /* cached */
    add r9,r12
    adc r11

    /* cached */
    add r9,r12
    adc r11

    /* cached */
    add r7,r12
    adc r11

    /* cached */
    add r7,r12
    adc r11

    /* cached */
    add r5,r12
    adc r11

    /* noncached */
    sub 2*20(r14),r12
    adc r11

    /* noncached */
    sub 2*22(r14),r12
    adc r11

    sub #2,r11
    mov r12,2*10(r15)

    mov r11,r12
    clr r11
    tst r12
    jge sk11
    mov #-1,r11
    sk11:

    /* noncached */
    add 2*11(r14),r12
    adc r11

    /* cached */
    add r8,r12
    adc r11

    /* cached */
    add r8,r12
    adc r11

    /* cached */
    add r6,r12
    adc r11

    /* cached */
    add r6,r12
    adc r11

    /* cached */
    add r4,r12
    adc r11

    /* noncached */
    sub 2*21(r14),r12
    adc r11

    /* noncached */
    sub 2*23(r14),r12
    adc r11

    sub #2,r11
    mov r12,2*11(r15)

    mov r11,r12
    clr r11
    tst r12
    jge sk12
    mov #-1,r11
    sk12:

    /* noncached */
    add 2*12(r14),r12
    adc r11

    /* cached */
    add r7,r12
    adc r11

    /* cached */
    add r7,r12
    adc r11

    /* cached */
    add r5,r12
    adc r11

    /* cached */
    add r5,r12
    adc r11

    /* cached */
    add r7,r12
    adc r11

    /* cached */
    add r9,r12
    adc r11

    /* noncached */
    sub 2*16(r14),r12
    adc r11

    /* noncached */
    sub 2*18(r14),r12
    adc r11

    sub #2,r11
    mov r12,2*12(r15)

    mov r11,r12
    clr r11
    tst r12
    jge sk13
    mov #-1,r11
    sk13:

    /* noncached */
    add 2*13(r14),r12
    adc r11

    /* cached */
    add r6,r12
    adc r11

    /* cached */
    add r6,r12
    adc r11

    /* cached */
    add r4,r12
    adc r11

    /* cached */
    add r4,r12
    adc r11

    /* cached */
    add r6,r12
    adc r11

    /* cached */
    add r8,r12
    adc r11

    /* noncached */
    sub 2*17(r14),r12
    adc r11

    /* noncached */
    sub 2*19(r14),r12
    adc r11

    sub #2,r11
    mov r12,2*13(r15)

    mov r11,r12
    clr r11
    tst r12
    jge sk14
    mov #-1,r11
    sk14:

    /* noncached */
    add 2*14(r14),r12
    adc r11

    /* cached */
    add r5,r12
    adc r11

    /* cached */
    add r5,r12
    adc r11

    /* cached */
    add r5,r12
    adc r11

    /* noncached */
    add 2*16(r14),r12
    adc r11

    /* noncached */
    sub 2*20(r14),r12
    adc r11

    /* noncached */
    sub 2*22(r14),r12
    adc r11

    /* cached */
    sub r13,r12
    adc r11

    /* cached */
    sub r9,r12
    adc r11

    sub #4,r11
    mov r12,2*14(r15)

    mov r11,r12
    clr r11
    tst r12
    jge sk15
    mov #-1,r11
    sk15:

    /* noncached */
    add 2*15(r14),r12
    adc r11

    /* cached */
    add r4,r12
    adc r11

    /* cached */
    add r4,r12
    adc r11

    /* cached */
    add r4,r12
    adc r11

    /* noncached */
    add 2*17(r14),r12
    adc r11

    /* noncached */
    sub 2*21(r14),r12
    adc r11

    /* noncached */
    sub 2*23(r14),r12
    adc r11

    /* cached */
    sub r10,r12
    adc r11

    /* cached */
    sub r8,r12
    adc r11

    sub #4,r11
    mov r12,2*15(r15)

