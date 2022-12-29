
    /* INDEX_READ_3 */
    mov @r14+,r4
    swpb r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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
    rla r4
    rla r4
    rla r4
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
    rla r4
    rla r4
    rla r4
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
    rla r4
    rla r4
    rla r4
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
    rla r4
    rla r4
    rla r4
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
    rla r4
    rla r4
    rla r4
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
    rla r4
    rla r4
    rla r4
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
    rla r4
    rla r4
    rla r4
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

    /* SUB */
    sub #16,r14

    /* SHIFT_HIGH */
    add #2*7,r15
    mov @r15+,r4
    
    
    
    rla r4
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13

    mov r6,2*0(r15)
    mov r7,2*1(r15)
    mov r8,2*2(r15)
    mov r9,2*3(r15)
    mov r10,2*4(r15)
    mov r11,2*5(r15)
    mov r12,2*6(r15)
    mov r13,2*7(r15)
    
    sub #2*8,r15

    /* SHIFT_LOW */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    mov @r15+,r12
    
    rla r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12

    rla r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12

    rla r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12

    rla r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12

    mov r5,-2*8(r15)
    mov r6,-2*7(r15)
    mov r7,-2*6(r15)
    mov r8,-2*5(r15)
    mov r9,-2*4(r15)
    mov r10,-2*3(r15)
    mov r11,-2*2(r15)
    mov r12,-2*1(r15)

    /* SUB */
    sub #16,r15

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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

    /* INDEX_READ_2 */
    mov @r14+,r4
    swpb r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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

    /* SUB */
    sub #16,r14

    /* SHIFT_HIGH */
    add #2*7,r15
    mov @r15+,r4
    
    
    /* SHIFT_HIGH_XORS */
    xor @r15+,r6
    xor @r15+,r7
    xor @r15+,r8
    xor @r15+,r9
    xor @r15+,r10
    xor @r15+,r11
    xor @r15+,r12
    xor @r15+,r13
    sub #2*8,r15

    
    rla r4
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13

    mov r6,2*0(r15)
    mov r7,2*1(r15)
    mov r8,2*2(r15)
    mov r9,2*3(r15)
    mov r10,2*4(r15)
    mov r11,2*5(r15)
    mov r12,2*6(r15)
    mov r13,2*7(r15)
    
    sub #2*8,r15

    /* SHIFT_LOW */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    mov @r15+,r12
    
    rla r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12

    rla r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12

    rla r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12

    rla r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12

    mov r5,-2*8(r15)
    mov r6,-2*7(r15)
    mov r7,-2*6(r15)
    mov r8,-2*5(r15)
    mov r9,-2*4(r15)
    mov r10,-2*3(r15)
    mov r11,-2*2(r15)
    mov r12,-2*1(r15)

    /* SUB */
    sub #16,r15

    /* INDEX_READ_1 */
    mov @r14+,r4
    rra r4
    rra r4
    rra r4
    rra r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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
    rla r4
    rla r4
    rla r4
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
    rla r4
    rla r4
    rla r4
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
    rla r4
    rla r4
    rla r4
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
    rla r4
    rla r4
    rla r4
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
    rla r4
    rla r4
    rla r4
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
    rla r4
    rla r4
    rla r4
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
    rla r4
    rla r4
    rla r4
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

    /* SUB */
    sub #16,r14

    /* SHIFT_HIGH */
    add #2*7,r15
    mov @r15+,r4
    
    
    /* SHIFT_HIGH_XORS */
    xor @r15+,r6
    xor @r15+,r7
    xor @r15+,r8
    xor @r15+,r9
    xor @r15+,r10
    xor @r15+,r11
    xor @r15+,r12
    xor @r15+,r13
    sub #2*8,r15

    
    rla r4
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13
    rlc r4

    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12
    rlc r13

    mov r6,2*0(r15)
    mov r7,2*1(r15)
    mov r8,2*2(r15)
    mov r9,2*3(r15)
    mov r10,2*4(r15)
    mov r11,2*5(r15)
    mov r12,2*6(r15)
    mov r13,2*7(r15)
    
    sub #2*8,r15

    /* SHIFT_LOW */
    mov @r15+,r5
    mov @r15+,r6
    mov @r15+,r7
    mov @r15+,r8
    mov @r15+,r9
    mov @r15+,r10
    mov @r15+,r11
    mov @r15+,r12
    
    rla r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12

    rla r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12

    rla r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12

    rla r5
    rlc r6
    rlc r7
    rlc r8
    rlc r9
    rlc r10
    rlc r11
    rlc r12

    mov r5,-2*8(r15)
    mov r6,-2*7(r15)
    mov r7,-2*6(r15)
    mov r8,-2*5(r15)
    mov r9,-2*4(r15)
    mov r10,-2*3(r15)
    mov r11,-2*2(r15)
    mov r12,-2*1(r15)

    /* SUB */
    sub #16,r15

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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

    /* INDEX_READ_0 */
    mov @r14+,r4
    and #0xF,r4

    /* INDEX_MULT */
    rla r4
    rla r4
    rla r4
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

    /* COL_WRITE_XOR */
    xor r6,16(r15)

    /* COL_WRITE_XOR */
    xor r7,18(r15)

    /* COL_WRITE_XOR */
    xor r8,20(r15)

    /* COL_WRITE_XOR */
    xor r9,22(r15)

    /* COL_WRITE_XOR */
    xor r10,24(r15)

    /* COL_WRITE_XOR */
    xor r11,26(r15)

    /* COL_WRITE_XOR */
    xor r12,28(r15)

    /* COL_WRITE_XOR */
    xor r13,30(r15)

    /* SUB */
    sub #16,r14
