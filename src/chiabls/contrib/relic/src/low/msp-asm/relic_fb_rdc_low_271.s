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

    mov 2*33(r14),r13
    mov r13,r11
    mov r13,r7

    mov 2*32(r14),r12
    mov r12,r10
    mov r12,r6

    mov 2*31(r14),r4
    mov r4,r9
    mov r4,r5
    xor r4,r11

    mov 2*30(r14),r4
    mov r4,r8
    mov r4,r4
    xor r4,r10

    xor 2*29(r14),r13
    mov r13,2*29(r14)
    xor r13,r9
    xor r13,r7

    xor 2*28(r14),r12
    mov r12,2*28(r14)
    xor r12,r8
    xor r12,r6

    xor 2*27(r14),r11
    mov r11,2*27(r14)
    xor r11,r7
    xor r11,r5

    xor 2*26(r14),r10
    mov r10,2*26(r14)
    xor r10,r6
    xor r10,r4

    xor 2*25(r14),r9
    mov r9,2*25(r14)
    xor r9,r5
    xor r9,r13

    xor 2*24(r14),r8
    mov r8,2*24(r14)
    xor r8,r4
    xor r8,r12

    xor 2*23(r14),r7
    mov r7,2*23(r14)
    xor r7,r13
    xor r7,r11

    xor 2*22(r14),r6
    mov r6,2*22(r14)
    xor r6,r12
    xor r6,r10

    xor 2*21(r14),r5
    mov r5,2*21(r14)
    xor r5,r11
    xor r5,r9

    xor 2*20(r14),r4
    mov r4,2*20(r14)
    xor r4,r10
    xor r4,r8

    xor 2*19(r14),r13
    mov r13,2*19(r14)
    xor r13,r9
    xor r13,r7

    xor 2*18(r14),r12
    mov r12,2*18(r14)
    xor r12,r8
    xor r12,r6

    xor 2*17(r14),r11
    mov r11,2*17(r14)
    xor r11,r7
    xor r11,r5

    xor r10,2*16(r14)
    xor r9,2*15(r14)
    xor r8,2*14(r14)
    xor r7,2*13(r14)
    xor r6,2*12(r14)
    xor r5,2*11(r14)
    xor r4,2*10(r14)
    xor r13,2*9(r14)
    xor r12,2*8(r14)
    xor r11,2*7(r14)

    //Add 1-shifted part

    //Digits 0-8

    add #2*17,r14
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11, r12
        mov @r14+,\i
    .endr
    sub #2*26,r14

    rla r4
    .irp i, r5, r6, r7, r8, r9, r10, r11, r12
        rlc \i
    .endr
    clr r13
    adc r13

    xor @r14+,r4
    mov r4,2*0(r15)
    xor @r14+,r5
    mov r5,2*1(r15)
    xor @r14+,r6
    mov r6,2*2(r15)
    xor @r14+,r7
    mov r7,2*3(r15)
    xor @r14+,r8
    mov r8,2*4(r15)
    xor @r14+,r9
    mov r9,2*5(r15)
    xor @r14+,r10
    mov r10,2*6(r15)
    xor @r14+,r11
    mov r11,2*7(r15)
    xor @r14+,r12
    mov r12,2*8(r15)

    //Digits 9-16

    add #2*17,r14
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        mov @r14+,\i
    .endr
    sub #2*25,r14

    rrc r13
    .irp i, r4, r5, r6, r7, r8, r9, r10, r11
        rlc \i
    .endr

    xor @r14+,r4
    mov r4,2*9(r15)
    xor @r14+,r5
    mov r5,2*10(r15)
    xor @r14+,r6
    mov r6,2*11(r15)
    xor @r14+,r7
    mov r7,2*12(r15)
    xor @r14+,r8
    mov r8,2*13(r15)
    xor @r14+,r9
    mov r9,2*14(r15)
    xor @r14+,r10
    mov r10,2*15(r15)

    xor @r14+,r11
    mov r11,r12
    and #0x7FFF,r11
    mov r11,(2 * 16)(r15)

    and #0x8000,r12
    xor r12,2*(16 - 4)(r15)
    xor r12,2*(16 - 6)(r15)
    xor r12,2*(16 - 10)(r15)
    rla r12
    clr r13
    adc r13
    xor r13,0(r15)

    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
    ret
