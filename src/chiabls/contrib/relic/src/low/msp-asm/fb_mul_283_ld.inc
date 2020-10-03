
    clr 2*(2*18 - 1)(r15)

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    mov @r4+,r6

    /* COL_WRITE_MOV */
    mov r6,0(r15)

    mov @r4+,r7

    mov @r4+,r8

    mov @r4+,r9

    mov @r4+,r10

    mov @r4+,r11

    mov @r4+,r12

    mov @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r7

    /* COL_WRITE_MOV */
    mov r7,2(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r8

    /* COL_WRITE_MOV */
    mov r8,4(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r9

    /* COL_WRITE_MOV */
    mov r9,6(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r10

    /* COL_WRITE_MOV */
    mov r10,8(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r11

    /* COL_WRITE_MOV */
    mov r11,10(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r12

    /* COL_WRITE_MOV */
    mov r12,12(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r13

    /* COL_WRITE_MOV */
    mov r13,14(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r6

    /* COL_WRITE_MOV */
    mov r6,16(r15)

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r7

    /* COL_WRITE_MOV */
    mov r7,18(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r8

    /* COL_WRITE_MOV */
    mov r8,20(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r9

    /* COL_WRITE_MOV */
    mov r9,22(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r10

    /* COL_WRITE_MOV */
    mov r10,24(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r11

    /* COL_WRITE_MOV */
    mov r11,26(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r12

    /* COL_WRITE_MOV */
    mov r12,28(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r13

    /* COL_WRITE_MOV */
    mov r13,30(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r6

    /* COL_WRITE_MOV */
    mov r6,32(r15)

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r7

    /* COL_WRITE_MOV */
    mov r7,34(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* COL_WRITE_MOV */
    mov r8,36(r15)

    /* COL_WRITE_MOV */
    mov r9,38(r15)

    /* COL_WRITE_MOV */
    mov r10,40(r15)

    /* COL_WRITE_MOV */
    mov r11,42(r15)

    /* COL_WRITE_MOV */
    mov r12,44(r15)

    /* COL_WRITE_MOV */
    mov r13,46(r15)

    /* COL_WRITE_MOV */
    mov r6,48(r15)

    /* COL_WRITE_MOV */
    mov r7,50(r15)

    /* SUB */
    sub #36,r14

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    mov @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,18(r15)

    mov @r4+,r9

    mov @r4+,r10

    mov @r4+,r11

    mov @r4+,r12

    mov @r4+,r13

    mov @r4+,r6

    mov @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r9

    /* COL_WRITE_XOR */
    xor r9,20(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r10

    /* COL_WRITE_XOR */
    xor r10,22(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,24(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,26(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,28(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r6

    /* COL_WRITE_XOR */
    xor r6,30(r15)

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r7

    /* COL_WRITE_XOR */
    xor r7,32(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,34(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r9

    /* COL_WRITE_XOR */
    xor r9,36(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r10

    /* COL_WRITE_XOR */
    xor r10,38(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,40(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,42(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,44(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r6

    /* COL_WRITE_XOR */
    xor r6,46(r15)

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r7

    /* COL_WRITE_XOR */
    xor r7,48(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,50(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r9

    /* COL_WRITE_MOV */
    mov r9,52(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* COL_WRITE_MOV */
    mov r10,54(r15)

    /* COL_WRITE_MOV */
    mov r11,56(r15)

    /* COL_WRITE_MOV */
    mov r12,58(r15)

    /* COL_WRITE_MOV */
    mov r13,60(r15)

    /* COL_WRITE_MOV */
    mov r6,62(r15)

    /* COL_WRITE_MOV */
    mov r7,64(r15)

    /* COL_WRITE_MOV */
    mov r8,66(r15)

    /* COL_WRITE_MOV */
    mov r9,68(r15)

    /* SUB */
    sub #36,r14

    clr r4

    /* SHIFT_FAST */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    mov @r15+,r12
    mov @r15+,r13
    
    rla r4
    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    mov r5,-2*9(r15)
    mov r6,-2*8(r15)
    mov r7,-2*7(r15)
    mov r8,-2*6(r15)
    mov r9,-2*5(r15)
    mov r10,-2*4(r15)
    mov r11,-2*3(r15)
    mov r12,-2*2(r15)
    mov r13,-2*1(r15)

    /* SHIFT_FAST_CARRY */
    rla r4
    rla r4
    rla r4
    rla r4
    swpb r4

    /* SHIFT_FAST */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    mov @r15+,r12
    mov @r15+,r13
    
    rla r4
    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    mov r5,-2*9(r15)
    mov r6,-2*8(r15)
    mov r7,-2*7(r15)
    mov r8,-2*6(r15)
    mov r9,-2*5(r15)
    mov r10,-2*4(r15)
    mov r11,-2*3(r15)
    mov r12,-2*2(r15)
    mov r13,-2*1(r15)

    /* SHIFT_FAST_CARRY */
    rla r4
    rla r4
    rla r4
    rla r4
    swpb r4

    /* SHIFT_FAST */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    mov @r15+,r12
    mov @r15+,r13
    
    rla r4
    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    mov r5,-2*9(r15)
    mov r6,-2*8(r15)
    mov r7,-2*7(r15)
    mov r8,-2*6(r15)
    mov r9,-2*5(r15)
    mov r10,-2*4(r15)
    mov r11,-2*3(r15)
    mov r12,-2*2(r15)
    mov r13,-2*1(r15)

    /* SHIFT_FAST_CARRY */
    rla r4
    rla r4
    rla r4
    rla r4
    swpb r4

    /* SHIFT_FAST */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    mov @r15+,r12
    mov @r15+,r13
    
    rla r4
    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    mov r5,-2*9(r15)
    mov r6,-2*8(r15)
    mov r7,-2*7(r15)
    mov r8,-2*6(r15)
    mov r9,-2*5(r15)
    mov r10,-2*4(r15)
    mov r11,-2*3(r15)
    mov r12,-2*2(r15)
    mov r13,-2*1(r15)

    /* SUB */
    sub #72,r15

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    mov @r4+,r10

    /* COL_WRITE_XOR */
    xor r10,0(r15)

    mov @r4+,r11

    mov @r4+,r12

    mov @r4+,r13

    mov @r4+,r6

    mov @r4+,r7

    mov @r4+,r8

    mov @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,2(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,4(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,6(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r6

    /* COL_WRITE_XOR */
    xor r6,8(r15)

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r7

    /* COL_WRITE_XOR */
    xor r7,10(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,12(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r9

    /* COL_WRITE_XOR */
    xor r9,14(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r10

    /* COL_WRITE_XOR */
    xor r10,16(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,18(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,20(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,22(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r6

    /* COL_WRITE_XOR */
    xor r6,24(r15)

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r7

    /* COL_WRITE_XOR */
    xor r7,26(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,28(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r9

    /* COL_WRITE_XOR */
    xor r9,30(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r10

    /* COL_WRITE_XOR */
    xor r10,32(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,34(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* COL_WRITE_XOR */
    xor r12,36(r15)

    /* COL_WRITE_XOR */
    xor r13,38(r15)

    /* COL_WRITE_XOR */
    xor r6,40(r15)

    /* COL_WRITE_XOR */
    xor r7,42(r15)

    /* COL_WRITE_XOR */
    xor r8,44(r15)

    /* COL_WRITE_XOR */
    xor r9,46(r15)

    /* COL_WRITE_XOR */
    xor r10,48(r15)

    /* COL_WRITE_XOR */
    xor r11,50(r15)

    /* SUB */
    sub #36,r14

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    mov @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,18(r15)

    mov @r4+,r13

    mov @r4+,r6

    mov @r4+,r7

    mov @r4+,r8

    mov @r4+,r9

    mov @r4+,r10

    mov @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,20(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r6

    /* COL_WRITE_XOR */
    xor r6,22(r15)

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r7

    /* COL_WRITE_XOR */
    xor r7,24(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,26(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r9

    /* COL_WRITE_XOR */
    xor r9,28(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r10

    /* COL_WRITE_XOR */
    xor r10,30(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,32(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,34(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,36(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r6

    /* COL_WRITE_XOR */
    xor r6,38(r15)

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r7

    /* COL_WRITE_XOR */
    xor r7,40(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,42(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r9

    /* COL_WRITE_XOR */
    xor r9,44(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r10

    /* COL_WRITE_XOR */
    xor r10,46(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,48(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,50(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,52(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* COL_WRITE_XOR */
    xor r6,54(r15)

    /* COL_WRITE_XOR */
    xor r7,56(r15)

    /* COL_WRITE_XOR */
    xor r8,58(r15)

    /* COL_WRITE_XOR */
    xor r9,60(r15)

    /* COL_WRITE_XOR */
    xor r10,62(r15)

    /* COL_WRITE_XOR */
    xor r11,64(r15)

    /* COL_WRITE_XOR */
    xor r12,66(r15)

    /* COL_WRITE_XOR */
    xor r13,68(r15)

    /* SUB */
    sub #36,r14

    clr r4

    /* SHIFT_FAST */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    mov @r15+,r12
    mov @r15+,r13
    
    rla r4
    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    mov r5,-2*9(r15)
    mov r6,-2*8(r15)
    mov r7,-2*7(r15)
    mov r8,-2*6(r15)
    mov r9,-2*5(r15)
    mov r10,-2*4(r15)
    mov r11,-2*3(r15)
    mov r12,-2*2(r15)
    mov r13,-2*1(r15)

    /* SHIFT_FAST_CARRY */
    rla r4
    rla r4
    rla r4
    rla r4
    swpb r4

    /* SHIFT_FAST */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    mov @r15+,r12
    mov @r15+,r13
    
    rla r4
    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    mov r5,-2*9(r15)
    mov r6,-2*8(r15)
    mov r7,-2*7(r15)
    mov r8,-2*6(r15)
    mov r9,-2*5(r15)
    mov r10,-2*4(r15)
    mov r11,-2*3(r15)
    mov r12,-2*2(r15)
    mov r13,-2*1(r15)

    /* SHIFT_FAST_CARRY */
    rla r4
    rla r4
    rla r4
    rla r4
    swpb r4

    /* SHIFT_FAST */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    mov @r15+,r12
    mov @r15+,r13
    
    rla r4
    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    mov r5,-2*9(r15)
    mov r6,-2*8(r15)
    mov r7,-2*7(r15)
    mov r8,-2*6(r15)
    mov r9,-2*5(r15)
    mov r10,-2*4(r15)
    mov r11,-2*3(r15)
    mov r12,-2*2(r15)
    mov r13,-2*1(r15)

    /* SHIFT_FAST_CARRY */
    rla r4
    rla r4
    rla r4
    rla r4
    swpb r4

    /* SHIFT_FAST */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    mov @r15+,r12
    mov @r15+,r13
    
    rla r4
    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    mov r5,-2*9(r15)
    mov r6,-2*8(r15)
    mov r7,-2*7(r15)
    mov r8,-2*6(r15)
    mov r9,-2*5(r15)
    mov r10,-2*4(r15)
    mov r11,-2*3(r15)
    mov r12,-2*2(r15)
    mov r13,-2*1(r15)

    /* SUB */
    sub #72,r15

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    mov @r4+,r6

    /* COL_WRITE_XOR */
    xor r6,0(r15)

    mov @r4+,r7

    mov @r4+,r8

    mov @r4+,r9

    mov @r4+,r10

    mov @r4+,r11

    mov @r4+,r12

    mov @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r7

    /* COL_WRITE_XOR */
    xor r7,2(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,4(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r9

    /* COL_WRITE_XOR */
    xor r9,6(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r10

    /* COL_WRITE_XOR */
    xor r10,8(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,10(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,12(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,14(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r6

    /* COL_WRITE_XOR */
    xor r6,16(r15)

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r7

    /* COL_WRITE_XOR */
    xor r7,18(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,20(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r9

    /* COL_WRITE_XOR */
    xor r9,22(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r10

    /* COL_WRITE_XOR */
    xor r10,24(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,26(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,28(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,30(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r6

    /* COL_WRITE_XOR */
    xor r6,32(r15)

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r7

    /* COL_WRITE_XOR */
    xor r7,34(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* COL_WRITE_XOR */
    xor r8,36(r15)

    /* COL_WRITE_XOR */
    xor r9,38(r15)

    /* COL_WRITE_XOR */
    xor r10,40(r15)

    /* COL_WRITE_XOR */
    xor r11,42(r15)

    /* COL_WRITE_XOR */
    xor r12,44(r15)

    /* COL_WRITE_XOR */
    xor r13,46(r15)

    /* COL_WRITE_XOR */
    xor r6,48(r15)

    /* COL_WRITE_XOR */
    xor r7,50(r15)

    /* SUB */
    sub #36,r14

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    mov @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,18(r15)

    mov @r4+,r9

    mov @r4+,r10

    mov @r4+,r11

    mov @r4+,r12

    mov @r4+,r13

    mov @r4+,r6

    mov @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r9

    /* COL_WRITE_XOR */
    xor r9,20(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r10

    /* COL_WRITE_XOR */
    xor r10,22(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,24(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,26(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,28(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r6

    /* COL_WRITE_XOR */
    xor r6,30(r15)

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r7

    /* COL_WRITE_XOR */
    xor r7,32(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,34(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r9

    /* COL_WRITE_XOR */
    xor r9,36(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r10

    /* COL_WRITE_XOR */
    xor r10,38(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,40(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,42(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,44(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r6

    /* COL_WRITE_XOR */
    xor r6,46(r15)

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r7

    /* COL_WRITE_XOR */
    xor r7,48(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,50(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r9

    /* COL_WRITE_XOR */
    xor r9,52(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* COL_WRITE_XOR */
    xor r10,54(r15)

    /* COL_WRITE_XOR */
    xor r11,56(r15)

    /* COL_WRITE_XOR */
    xor r12,58(r15)

    /* COL_WRITE_XOR */
    xor r13,60(r15)

    /* COL_WRITE_XOR */
    xor r6,62(r15)

    /* COL_WRITE_XOR */
    xor r7,64(r15)

    /* COL_WRITE_XOR */
    xor r8,66(r15)

    /* COL_WRITE_XOR */
    xor r9,68(r15)

    /* SUB */
    sub #36,r14

    clr r4

    /* SHIFT_FAST */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    mov @r15+,r12
    mov @r15+,r13
    
    rla r4
    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    mov r5,-2*9(r15)
    mov r6,-2*8(r15)
    mov r7,-2*7(r15)
    mov r8,-2*6(r15)
    mov r9,-2*5(r15)
    mov r10,-2*4(r15)
    mov r11,-2*3(r15)
    mov r12,-2*2(r15)
    mov r13,-2*1(r15)

    /* SHIFT_FAST_CARRY */
    rla r4
    rla r4
    rla r4
    rla r4
    swpb r4

    /* SHIFT_FAST */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    mov @r15+,r12
    mov @r15+,r13
    
    rla r4
    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    mov r5,-2*9(r15)
    mov r6,-2*8(r15)
    mov r7,-2*7(r15)
    mov r8,-2*6(r15)
    mov r9,-2*5(r15)
    mov r10,-2*4(r15)
    mov r11,-2*3(r15)
    mov r12,-2*2(r15)
    mov r13,-2*1(r15)

    /* SHIFT_FAST_CARRY */
    rla r4
    rla r4
    rla r4
    rla r4
    swpb r4

    /* SHIFT_FAST */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    mov @r15+,r12
    mov @r15+,r13
    
    rla r4
    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    mov r5,-2*9(r15)
    mov r6,-2*8(r15)
    mov r7,-2*7(r15)
    mov r8,-2*6(r15)
    mov r9,-2*5(r15)
    mov r10,-2*4(r15)
    mov r11,-2*3(r15)
    mov r12,-2*2(r15)
    mov r13,-2*1(r15)

    /* SHIFT_FAST_CARRY */
    rla r4
    rla r4
    rla r4
    rla r4
    swpb r4

    /* SHIFT_FAST */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    mov @r15+,r12
    mov @r15+,r13
    
    rla r4
    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    mov r5,-2*9(r15)
    mov r6,-2*8(r15)
    mov r7,-2*7(r15)
    mov r8,-2*6(r15)
    mov r9,-2*5(r15)
    mov r10,-2*4(r15)
    mov r11,-2*3(r15)
    mov r12,-2*2(r15)
    mov r13,-2*1(r15)

    /* SUB */
    sub #72,r15

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    mov @r4+,r10

    /* COL_WRITE_XOR */
    xor r10,0(r15)

    mov @r4+,r11

    mov @r4+,r12

    mov @r4+,r13

    mov @r4+,r6

    mov @r4+,r7

    mov @r4+,r8

    mov @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,2(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,4(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,6(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r6

    /* COL_WRITE_XOR */
    xor r6,8(r15)

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r7

    /* COL_WRITE_XOR */
    xor r7,10(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,12(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r9

    /* COL_WRITE_XOR */
    xor r9,14(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r10

    /* COL_WRITE_XOR */
    xor r10,16(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,18(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,20(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,22(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r6

    /* COL_WRITE_XOR */
    xor r6,24(r15)

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r7

    /* COL_WRITE_XOR */
    xor r7,26(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,28(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r9

    /* COL_WRITE_XOR */
    xor r9,30(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r10

    /* COL_WRITE_XOR */
    xor r10,32(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #0,r4

    xor @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,34(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* COL_WRITE_XOR */
    xor r12,36(r15)

    /* COL_WRITE_XOR */
    xor r13,38(r15)

    /* COL_WRITE_XOR */
    xor r6,40(r15)

    /* COL_WRITE_XOR */
    xor r7,42(r15)

    /* COL_WRITE_XOR */
    xor r8,44(r15)

    /* COL_WRITE_XOR */
    xor r9,46(r15)

    /* COL_WRITE_XOR */
    xor r10,48(r15)

    /* COL_WRITE_XOR */
    xor r11,50(r15)

    /* SUB */
    sub #36,r14

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    mov @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,18(r15)

    mov @r4+,r13

    mov @r4+,r6

    mov @r4+,r7

    mov @r4+,r8

    mov @r4+,r9

    mov @r4+,r10

    mov @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,20(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r6

    /* COL_WRITE_XOR */
    xor r6,22(r15)

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r7

    /* COL_WRITE_XOR */
    xor r7,24(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,26(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r9

    /* COL_WRITE_XOR */
    xor r9,28(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r10

    /* COL_WRITE_XOR */
    xor r10,30(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,32(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,34(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,36(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r6

    /* COL_WRITE_XOR */
    xor r6,38(r15)

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    mov @r4+,r6

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r7

    /* COL_WRITE_XOR */
    xor r7,40(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,42(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r9

    /* COL_WRITE_XOR */
    xor r9,44(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r10

    /* COL_WRITE_XOR */
    xor r10,46(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,48(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,50(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    mov r4,r5
    rla r4
    rla r4
    rla r4
    add r5,r4
    rla r4
    rla r4
    
    add r1,r4
    add #18,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,52(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* COL_WRITE_XOR */
    xor r6,54(r15)

    /* COL_WRITE_XOR */
    xor r7,56(r15)

    /* COL_WRITE_XOR */
    xor r8,58(r15)

    /* COL_WRITE_XOR */
    xor r9,60(r15)

    /* COL_WRITE_XOR */
    xor r10,62(r15)

    /* COL_WRITE_XOR */
    xor r11,64(r15)

    /* COL_WRITE_XOR */
    xor r12,66(r15)

    /* COL_WRITE_XOR */
    xor r13,68(r15)

    /* SUB */
    sub #36,r14
