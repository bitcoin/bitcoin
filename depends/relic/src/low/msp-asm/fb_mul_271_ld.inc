
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

    /* COL_WRITE_MOV */
    mov r7,34(r15)

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

    /* SUB */
    sub #34,r14

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

    mov @r4+,r7

    /* COL_WRITE_XOR */
    xor r7,18(r15)

    mov @r4+,r8

    mov @r4+,r9

    mov @r4+,r10

    mov @r4+,r11

    mov @r4+,r12

    mov @r4+,r13

    mov @r4+,r6

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
    xor r8,20(r15)

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
    xor r9,22(r15)

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
    xor r10,24(r15)

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
    xor r11,26(r15)

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
    xor r12,28(r15)

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
    xor r13,30(r15)

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
    xor r6,32(r15)

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
    xor r7,34(r15)

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
    xor r8,36(r15)

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
    xor r9,38(r15)

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
    xor r10,40(r15)

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
    xor r11,42(r15)

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
    xor r12,44(r15)

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
    xor r13,46(r15)

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
    xor r6,48(r15)

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

    /* COL_WRITE_MOV */
    mov r7,50(r15)

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    mov @r4+,r7

    /* COL_WRITE_MOV */
    mov r8,52(r15)

    /* COL_WRITE_MOV */
    mov r9,54(r15)

    /* COL_WRITE_MOV */
    mov r10,56(r15)

    /* COL_WRITE_MOV */
    mov r11,58(r15)

    /* COL_WRITE_MOV */
    mov r12,60(r15)

    /* COL_WRITE_MOV */
    mov r13,62(r15)

    /* COL_WRITE_MOV */
    mov r6,64(r15)

    /* COL_WRITE_MOV */
    mov r7,66(r15)

    /* SUB */
    sub #34,r14

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

    /* SHIFT_FAST_LAST_7 */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    
    rla r4
    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11

    mov r5,-2*7(r15)
    mov r6,-2*6(r15)
    mov r7,-2*5(r15)
    mov r8,-2*4(r15)
    mov r9,-2*3(r15)
    mov r10,-2*2(r15)
    mov r11,-2*1(r15)
    
    sub #2*34,r15

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

    mov @r4+,r8

    /* COL_WRITE_XOR */
    xor r8,0(r15)

    mov @r4+,r9

    mov @r4+,r10

    mov @r4+,r11

    mov @r4+,r12

    mov @r4+,r13

    mov @r4+,r6

    mov @r4+,r7

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
    xor r9,2(r15)

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
    xor r10,4(r15)

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
    xor r11,6(r15)

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
    xor r12,8(r15)

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
    xor r13,10(r15)

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
    xor r6,12(r15)

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
    xor r7,14(r15)

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
    xor r8,16(r15)

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
    xor r9,18(r15)

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
    xor r10,20(r15)

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
    xor r11,22(r15)

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
    xor r12,24(r15)

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
    xor r13,26(r15)

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
    xor r6,28(r15)

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
    xor r7,30(r15)

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
    xor r8,32(r15)

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    mov @r4+,r8

    /* COL_WRITE_XOR */
    xor r9,34(r15)

    /* COL_WRITE_XOR */
    xor r10,36(r15)

    /* COL_WRITE_XOR */
    xor r11,38(r15)

    /* COL_WRITE_XOR */
    xor r12,40(r15)

    /* COL_WRITE_XOR */
    xor r13,42(r15)

    /* COL_WRITE_XOR */
    xor r6,44(r15)

    /* COL_WRITE_XOR */
    xor r7,46(r15)

    /* COL_WRITE_XOR */
    xor r8,48(r15)

    /* SUB */
    sub #34,r14

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

    mov @r4+,r9

    /* COL_WRITE_XOR */
    xor r9,18(r15)

    mov @r4+,r10

    mov @r4+,r11

    mov @r4+,r12

    mov @r4+,r13

    mov @r4+,r6

    mov @r4+,r7

    mov @r4+,r8

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
    xor r10,20(r15)

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
    xor r11,22(r15)

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
    xor r12,24(r15)

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
    xor r13,26(r15)

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
    xor r6,28(r15)

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
    xor r7,30(r15)

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
    xor r8,32(r15)

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
    xor r9,34(r15)

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
    xor r10,36(r15)

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
    xor r11,38(r15)

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
    xor r12,40(r15)

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
    xor r13,42(r15)

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
    xor r6,44(r15)

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
    xor r7,46(r15)

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
    xor r8,48(r15)

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
    xor r9,50(r15)

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    mov @r4+,r9

    /* COL_WRITE_XOR */
    xor r10,52(r15)

    /* COL_WRITE_XOR */
    xor r11,54(r15)

    /* COL_WRITE_XOR */
    xor r12,56(r15)

    /* COL_WRITE_XOR */
    xor r13,58(r15)

    /* COL_WRITE_XOR */
    xor r6,60(r15)

    /* COL_WRITE_XOR */
    xor r7,62(r15)

    /* COL_WRITE_XOR */
    xor r8,64(r15)

    /* COL_WRITE_XOR */
    xor r9,66(r15)

    /* SUB */
    sub #34,r14

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

    /* SHIFT_FAST_LAST_7 */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    
    rla r4
    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11

    mov r5,-2*7(r15)
    mov r6,-2*6(r15)
    mov r7,-2*5(r15)
    mov r8,-2*4(r15)
    mov r9,-2*3(r15)
    mov r10,-2*2(r15)
    mov r11,-2*1(r15)
    
    sub #2*34,r15

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
    xor r11,2(r15)

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
    xor r12,4(r15)

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
    xor r13,6(r15)

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
    xor r6,8(r15)

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
    xor r7,10(r15)

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
    xor r8,12(r15)

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
    xor r9,14(r15)

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
    xor r10,16(r15)

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
    xor r11,18(r15)

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
    xor r12,20(r15)

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
    xor r13,22(r15)

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
    xor r6,24(r15)

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
    xor r7,26(r15)

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
    xor r8,28(r15)

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
    xor r9,30(r15)

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
    xor r10,32(r15)

    xor @r4+,r11

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    mov @r4+,r10

    /* COL_WRITE_XOR */
    xor r11,34(r15)

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

    /* SUB */
    sub #34,r14

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

    mov @r4+,r11

    /* COL_WRITE_XOR */
    xor r11,18(r15)

    mov @r4+,r12

    mov @r4+,r13

    mov @r4+,r6

    mov @r4+,r7

    mov @r4+,r8

    mov @r4+,r9

    mov @r4+,r10

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
    xor r12,20(r15)

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
    xor r13,22(r15)

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
    xor r6,24(r15)

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
    xor r7,26(r15)

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
    xor r8,28(r15)

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
    xor r9,30(r15)

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
    xor r10,32(r15)

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
    xor r11,34(r15)

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
    xor r12,36(r15)

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
    xor r13,38(r15)

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
    xor r6,40(r15)

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
    xor r7,42(r15)

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
    xor r8,44(r15)

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
    xor r9,46(r15)

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
    xor r10,48(r15)

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
    xor r11,50(r15)

    xor @r4+,r12

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    mov @r4+,r11

    /* COL_WRITE_XOR */
    xor r12,52(r15)

    /* COL_WRITE_XOR */
    xor r13,54(r15)

    /* COL_WRITE_XOR */
    xor r6,56(r15)

    /* COL_WRITE_XOR */
    xor r7,58(r15)

    /* COL_WRITE_XOR */
    xor r8,60(r15)

    /* COL_WRITE_XOR */
    xor r9,62(r15)

    /* COL_WRITE_XOR */
    xor r10,64(r15)

    /* COL_WRITE_XOR */
    xor r11,66(r15)

    /* SUB */
    sub #34,r14

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

    /* SHIFT_FAST_LAST_7 */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    
    rla r4
    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r4

    rlc r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11

    mov r5,-2*7(r15)
    mov r6,-2*6(r15)
    mov r7,-2*5(r15)
    mov r8,-2*4(r15)
    mov r9,-2*3(r15)
    mov r10,-2*2(r15)
    mov r11,-2*1(r15)
    
    sub #2*34,r15

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

    mov @r4+,r12

    /* COL_WRITE_XOR */
    xor r12,0(r15)

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
    add #0,r4

    xor @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,2(r15)

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
    xor r6,4(r15)

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
    xor r7,6(r15)

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
    xor r8,8(r15)

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
    xor r9,10(r15)

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
    xor r10,12(r15)

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
    xor r11,14(r15)

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
    xor r12,16(r15)

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
    xor r13,18(r15)

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
    xor r6,20(r15)

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
    xor r7,22(r15)

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
    xor r8,24(r15)

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
    xor r9,26(r15)

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
    xor r10,28(r15)

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
    xor r11,30(r15)

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
    xor r12,32(r15)

    xor @r4+,r13

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    mov @r4+,r12

    /* COL_WRITE_XOR */
    xor r13,34(r15)

    /* COL_WRITE_XOR */
    xor r6,36(r15)

    /* COL_WRITE_XOR */
    xor r7,38(r15)

    /* COL_WRITE_XOR */
    xor r8,40(r15)

    /* COL_WRITE_XOR */
    xor r9,42(r15)

    /* COL_WRITE_XOR */
    xor r10,44(r15)

    /* COL_WRITE_XOR */
    xor r11,46(r15)

    /* COL_WRITE_XOR */
    xor r12,48(r15)

    /* SUB */
    sub #34,r14

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

    mov @r4+,r13

    /* COL_WRITE_XOR */
    xor r13,18(r15)

    mov @r4+,r6

    mov @r4+,r7

    mov @r4+,r8

    mov @r4+,r9

    mov @r4+,r10

    mov @r4+,r11

    mov @r4+,r12

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
    xor r6,20(r15)

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
    xor r7,22(r15)

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
    xor r8,24(r15)

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
    xor r9,26(r15)

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
    xor r10,28(r15)

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
    xor r11,30(r15)

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
    xor r12,32(r15)

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
    xor r13,34(r15)

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
    xor r6,36(r15)

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
    xor r7,38(r15)

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
    xor r8,40(r15)

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
    xor r9,42(r15)

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
    xor r10,44(r15)

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
    xor r11,46(r15)

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
    xor r12,48(r15)

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
    xor r13,50(r15)

    xor @r4+,r6

    xor @r4+,r7

    xor @r4+,r8

    xor @r4+,r9

    xor @r4+,r10

    xor @r4+,r11

    xor @r4+,r12

    mov @r4+,r13

    /* COL_WRITE_XOR */
    xor r6,52(r15)

    /* COL_WRITE_XOR */
    xor r7,54(r15)

    /* COL_WRITE_XOR */
    xor r8,56(r15)

    /* COL_WRITE_XOR */
    xor r9,58(r15)

    /* COL_WRITE_XOR */
    xor r10,60(r15)

    /* COL_WRITE_XOR */
    xor r11,62(r15)

    /* COL_WRITE_XOR */
    xor r12,64(r15)

    /* COL_WRITE_XOR */
    xor r13,66(r15)

    /* SUB */
    sub #34,r14
