
    //reads a[i]
    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    //r7: even, low
    mov.b fb_srt_table_evens(r4),r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r5: odd, low
    mov.b fb_srt_table_odds(r4),r8


    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    mov.b fb_srt_table_evens(r4),r5
    //r5: even, high
    swpb r5
    //r7: even
    bis r5,r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, high
    swpb r5
    //r8: odd
    bis r5,r8
    
    mov r7,2*0(r1)

    mov r8,2*(0+3)(r1)
    
    rrc r9
    rlc r8
    rlc r9
    
    mov r8,2*(0+11)(r1)

    //reads a[i]
    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    //r7: even, low
    mov.b fb_srt_table_evens(r4),r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r5: odd, low
    mov.b fb_srt_table_odds(r4),r8


    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    mov.b fb_srt_table_evens(r4),r5
    //r5: even, high
    swpb r5
    //r7: even
    bis r5,r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, high
    swpb r5
    //r8: odd
    bis r5,r8
    
    mov r7,2*1(r1)

    mov r8,2*(1+3)(r1)
    
    rrc r9
    rlc r8
    rlc r9
    
    mov r8,2*(1+11)(r1)

    //reads a[i]
    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    //r7: even, low
    mov.b fb_srt_table_evens(r4),r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r5: odd, low
    mov.b fb_srt_table_odds(r4),r8


    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    mov.b fb_srt_table_evens(r4),r5
    //r5: even, high
    swpb r5
    //r7: even
    bis r5,r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, high
    swpb r5
    //r8: odd
    bis r5,r8
    
    mov r7,2*2(r1)

    mov r8,2*(2+3)(r1)
    
    rrc r9
    rlc r8
    rlc r9
    
    mov r8,2*(2+11)(r1)

    //reads a[i]
    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    //r7: even, low
    mov.b fb_srt_table_evens(r4),r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r5: odd, low
    mov.b fb_srt_table_odds(r4),r8


    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    mov.b fb_srt_table_evens(r4),r5
    //r5: even, high
    swpb r5
    //r7: even
    bis r5,r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, high
    swpb r5
    //r8: odd
    bis r5,r8
    
    xor r7,2*3(r1)

    mov r8,2*(3+3)(r1)
    
    rrc r9
    rlc r8
    rlc r9
    
    mov r8,2*(3+11)(r1)

    //reads a[i]
    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    //r7: even, low
    mov.b fb_srt_table_evens(r4),r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r5: odd, low
    mov.b fb_srt_table_odds(r4),r8


    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    mov.b fb_srt_table_evens(r4),r5
    //r5: even, high
    swpb r5
    //r7: even
    bis r5,r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, high
    swpb r5
    //r8: odd
    bis r5,r8
    
    xor r7,2*4(r1)

    mov r8,2*(4+3)(r1)
    
    rrc r9
    rlc r8
    rlc r9
    
    mov r8,2*(4+11)(r1)

    //reads a[i]
    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    //r7: even, low
    mov.b fb_srt_table_evens(r4),r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r5: odd, low
    mov.b fb_srt_table_odds(r4),r8


    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    mov.b fb_srt_table_evens(r4),r5
    //r5: even, high
    swpb r5
    //r7: even
    bis r5,r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, high
    swpb r5
    //r8: odd
    bis r5,r8
    
    xor r7,2*5(r1)

    mov r8,2*(5+3)(r1)
    
    rrc r9
    rlc r8
    rlc r9
    
    mov r8,2*(5+11)(r1)

    //reads a[i]
    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    //r7: even, low
    mov.b fb_srt_table_evens(r4),r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r5: odd, low
    mov.b fb_srt_table_odds(r4),r8


    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    mov.b fb_srt_table_evens(r4),r5
    //r5: even, high
    swpb r5
    //r7: even
    bis r5,r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, high
    swpb r5
    //r8: odd
    bis r5,r8
    
    xor r7,2*6(r1)

    mov r8,2*(6+3)(r1)
    
    rrc r9
    rlc r8
    rlc r9
    
    mov r8,2*(6+11)(r1)

    //reads a[i]
    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    //r7: even, low
    mov.b fb_srt_table_evens(r4),r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r5: odd, low
    mov.b fb_srt_table_odds(r4),r8


    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    mov.b fb_srt_table_evens(r4),r5
    //r5: even, high
    swpb r5
    //r7: even
    bis r5,r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, high
    swpb r5
    //r8: odd
    bis r5,r8
    
    xor r7,2*7(r1)

    mov r8,2*(7+3)(r1)
    
    rrc r9
    rlc r8
    rlc r9
    
    mov r8,2*(7+11)(r1)

    //reads a[i]
    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    //r7: even, low
    mov.b fb_srt_table_evens(r4),r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r5: odd, low
    mov.b fb_srt_table_odds(r4),r8


    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    mov.b fb_srt_table_evens(r4),r5
    //r5: even, high
    swpb r5
    //r7: even
    bis r5,r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, high
    swpb r5
    //r8: odd
    bis r5,r8
    
    xor r7,2*8(r1)

    xor r8,2*(8+3)(r1)
    
    rrc r9
    rlc r8
    rlc r9
    
    mov r8,2*(8+11)(r1)

    //reads a[i]
    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    //r7: even, low
    mov.b fb_srt_table_evens(r4),r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r5: odd, low
    mov.b fb_srt_table_odds(r4),r8


    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    mov.b fb_srt_table_evens(r4),r5
    //r5: even, high
    swpb r5
    //r7: even
    bis r5,r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, high
    swpb r5
    //r8: odd
    bis r5,r8
    
    xor r7,2*9(r1)

    xor r8,2*(9+3)(r1)
    
    rrc r9
    rlc r8
    rlc r9
    
    mov r8,2*(9+11)(r1)

    //reads a[i]
    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    //r7: even, low
    mov.b fb_srt_table_evens(r4),r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r5: odd, low
    mov.b fb_srt_table_odds(r4),r8


    mov @r14+,r4

    mov r4,r5
    mov r4,r6
    and #0x55,r4
    swpb r5
    rla r5
    and #0xAA,r5
    bis r5,r4
    mov.b fb_srt_table_evens(r4),r5
    //r5: even, high
    swpb r5
    //r7: even
    bis r5,r7

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, high
    swpb r5
    //r8: odd
    bis r5,r8
    
    xor r7,2*10(r1)

    xor r8,2*(10+3)(r1)
    
    rrc r9
    rlc r8
    rlc r9
    
    mov r8,2*(10+11)(r1)

    //reads a[i]
    mov @r14+,r4

    xor r4,2*11(r1)
    
    mov r9,2*(11+11)(r1)
