
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
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, low, in high byte
    swpb r5
    //add carry
    //r5: odd
    bis r8,r5

    mov r5,2*(0+3)(r1)
    mov r5,2*(0+5)(r1)
    mov r5,2*(0+6)(r1)
    mov r5,2*(0+8)(r1)

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

    mov r7,2*0(r1)

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r8: odd, high, in low byte (carry)
    mov.b fb_srt_table_odds(r4),r8

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
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, low, in high byte
    swpb r5
    //add carry
    //r5: odd
    bis r8,r5

    mov r5,2*(1+3)(r1)
    xor r5,2*(1+5)(r1)
    mov r5,2*(1+6)(r1)
    mov r5,2*(1+8)(r1)

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

    mov r7,2*1(r1)

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r8: odd, high, in low byte (carry)
    mov.b fb_srt_table_odds(r4),r8

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
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, low, in high byte
    swpb r5
    //add carry
    //r5: odd
    bis r8,r5

    xor r5,2*(2+3)(r1)
    xor r5,2*(2+5)(r1)
    xor r5,2*(2+6)(r1)
    mov r5,2*(2+8)(r1)

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

    mov r7,2*2(r1)

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r8: odd, high, in low byte (carry)
    mov.b fb_srt_table_odds(r4),r8

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
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, low, in high byte
    swpb r5
    //add carry
    //r5: odd
    bis r8,r5

    xor r5,2*(3+3)(r1)
    xor r5,2*(3+5)(r1)
    xor r5,2*(3+6)(r1)
    mov r5,2*(3+8)(r1)

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

    xor r7,2*3(r1)

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r8: odd, high, in low byte (carry)
    mov.b fb_srt_table_odds(r4),r8

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
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, low, in high byte
    swpb r5
    //add carry
    //r5: odd
    bis r8,r5

    xor r5,2*(4+3)(r1)
    xor r5,2*(4+5)(r1)
    xor r5,2*(4+6)(r1)
    mov r5,2*(4+8)(r1)

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

    xor r7,2*4(r1)

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r8: odd, high, in low byte (carry)
    mov.b fb_srt_table_odds(r4),r8

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
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, low, in high byte
    swpb r5
    //add carry
    //r5: odd
    bis r8,r5

    xor r5,2*(5+3)(r1)
    xor r5,2*(5+5)(r1)
    xor r5,2*(5+6)(r1)
    mov r5,2*(5+8)(r1)

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

    xor r7,2*5(r1)

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r8: odd, high, in low byte (carry)
    mov.b fb_srt_table_odds(r4),r8

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
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, low, in high byte
    swpb r5
    //add carry
    //r5: odd
    bis r8,r5

    xor r5,2*(6+3)(r1)
    xor r5,2*(6+5)(r1)
    xor r5,2*(6+6)(r1)
    mov r5,2*(6+8)(r1)

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

    xor r7,2*6(r1)

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r8: odd, high, in low byte (carry)
    mov.b fb_srt_table_odds(r4),r8

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
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, low, in high byte
    swpb r5
    //add carry
    //r5: odd
    bis r8,r5

    xor r5,2*(7+3)(r1)
    xor r5,2*(7+5)(r1)
    xor r5,2*(7+6)(r1)
    mov r5,2*(7+8)(r1)

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

    xor r7,2*7(r1)

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r8: odd, high, in low byte (carry)
    mov.b fb_srt_table_odds(r4),r8

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

    xor r7,2*8(r1)

    mov r6,r4
    and #0xAA,r4
    swpb r6
    rra r6
    and #0x55,r6
    bis r6,r4
    //r5: odd, low
    mov.b fb_srt_table_odds(r4),r5
    //r5: odd, low, in high byte
    swpb r5
    //add carry
    //r5: odd
    bis r8,r5

    xor r5,2*(8+3)(r1)
    xor r5,2*(8+5)(r1)
    xor r5,2*(8+6)(r1)
    mov r5,2*(8+8)(r1)
