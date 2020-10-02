fb_rdcn_low:
    /* r15: c
       r14: a
    */
    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11

    add #2*37,r14
    .irp reg, r11, r10, r9, r8, r7, r6, r5, r4
        mov @r14+, \reg
    .endr
    sub #2*(37+8),r14
    /*
    mov 2*44(r14),r4
    mov 2*43(r14),r5
    mov 2*42(r14),r6
    mov 2*41(r14),r7
    mov 2*40(r14),r8
    mov 2*39(r14),r9
    mov 2*38(r14),r10
    mov 2*37(r14),r11
    */

    clrc
    .irp reg, r4, r5, r6, r7, r8, r9, r10, r11
        rrc \reg
    .endr
    clr r12
    rrc r12

    xor r4,2*22(r14)
    xor r5,2*21(r14)
    xor r6,2*20(r14)
    xor r7,2*19(r14)
    xor r8,2*18(r14)
    xor r9,2*17(r14)
    xor r10,2*16(r14)
    xor r11,2*15(r14)

    clrc
    .irp reg, r4, r5, r6, r7, r8, r9, r10, r11, r12
        rrc \reg
    .endr

    xor r4,2*28(r14)
    xor r5,2*27(r14)
    xor r6,2*26(r14)
    xor r7,2*25(r14)
    xor r8,2*24(r14)
    xor r9,2*23(r14)
    xor r10,2*22(r14)
    xor r11,2*21(r14)


    add #2*29,r14
    .irp reg, r11, r10, r9, r8, r7, r6, r5, r4
        mov @r14+, \reg
    .endr
    sub #2*(29+8),r14
    /*
    mov 2*36(r14),r4
    mov 2*35(r14),r5
    mov 2*34(r14),r6
    mov 2*33(r14),r7
    mov 2*32(r14),r8
    mov 2*31(r14),r9
    mov 2*30(r14),r10
    mov 2*29(r14),r11
    */

    rla r12
    rlc r12
    .irp reg, r4, r5, r6, r7, r8, r9, r10, r11
        rrc \reg
    .endr
    clr r13
    rrc r13

    xor r4,2*14(r14)
    xor r5,2*13(r14)
    xor r6,2*12(r14)
    xor r7,2*11(r14)
    xor r8,2*10(r14)
    xor r9,2*9(r14)
    xor r10,2*8(r14)
    xor r11,2*7(r14)

    rrc r12
    .irp reg, r4, r5, r6, r7, r8, r9, r10, r11, r13
        rrc \reg
    .endr
    mov r13,r12

    xor r4,2*20(r14)
    xor r5,2*19(r14)
    xor r6,2*18(r14)
    xor r7,2*17(r14)
    xor r8,2*16(r14)
    xor r9,2*15(r14)
    xor r10,2*14(r14)
    xor r11,2*13(r14)


    add #2*22,r14
    .irp reg, r10, r9, r8, r7, r6, r5, r4
        mov @r14+, \reg
    .endr
    sub #2*(22+7),r14
    /*
    mov 2*28(r14),r4
    mov 2*27(r14),r5
    mov 2*26(r14),r6
    mov 2*25(r14),r7
    mov 2*24(r14),r8
    mov 2*23(r14),r9
    mov 2*22(r14),r10
    */
    and #0x1,2*22(r14)
    and #0xFFFE,r10

    rla r12
    rlc r12
    .irp reg, r4, r5, r6, r7, r8, r9, r10
        rrc \reg
    .endr
    clr r13
    rrc r13

    xor r4,2*6(r14)
    xor r5,2*5(r14)
    xor r6,2*4(r14)
    xor r7,2*3(r14)
    xor r8,2*2(r14)
    xor r9,2*1(r14)
    xor r10,2*0(r14)

    clrc
    rrc r12
    .irp reg, r4, r5, r6, r7, r8, r9, r10, r13
        rrc \reg
    .endr

    xor r4,2*12(r14)
    xor r5,2*11(r14)
    xor r6,2*10(r14)
    xor r7,2*9(r14)
    xor r8,2*8(r14)
    xor r9,2*7(r14)
    xor r10,2*6(r14)
    xor r13,2*5(r14)

    .irp i, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22
        mov @r14+,(2 * \i)(r15)
    .endr

    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
    ret
