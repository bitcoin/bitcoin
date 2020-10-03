/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

.global fb_rdcn_low

.text
.align 2

.macro REDC_COL i
    //reduce into a[i]
    //r8: carry
    //r9 pending a[i+1] write

    .if \i > 0
        mov (2 * (10 + \i))(r14),r10
    .else
        mov (2 * (10 + \i))(r15),r10
        and #0x7,(2 * (10 + \i))(r15)
        and #0xFFF8,r10
    .endif

    mov r10,r13
    clr r12
    rla r13
    rlc r12
    rla r13
    rlc r12
    rla r13
    rlc r12
    mov r13,r11

    xor r12,r9
    rla r13
    rlc r12
    xor r12,r9
    .if \i < 10
        //write a[i+1]
        mov r9,(2 * (\i + 1))(r15)
    .endif
    mov r13,r9
    xor r11,r9

    //r9: a[i]
    xor (2 * \i)(r14),r9

    //"carry"
    .if \i < 10
        xor r8,r9
    .endif

    xor r10,r9

    .if \i > 0
        clr r8
        clrc
        rrc r10
        rrc r8
        rrc r10
        rrc r8
        rrc r10
        rrc r8

        xor r10,r9
    .else
        clrc
        rrc r10
        clrc
        rrc r10
        clrc
        rrc r10

        xor r10,r9
    .endif
.endm

fb_rdcn_low:
    /* r15: c
       r14: a
    */
    push r8
    push r9
    push r10
    push r11

    REDC_COL 10
    REDC_COL 9
    REDC_COL 8
    REDC_COL 7
    REDC_COL 6
    REDC_COL 5
    REDC_COL 4
    REDC_COL 3
    REDC_COL 2
    REDC_COL 1
    REDC_COL 0

    mov r9,@r15

    pop r11
    pop r10
    pop r9
    pop r8
    ret
