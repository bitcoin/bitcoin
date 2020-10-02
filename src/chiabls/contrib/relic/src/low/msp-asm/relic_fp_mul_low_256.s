#ifdef MSP430X2

fp_muln_low:
    mov r13,r12
    mov r15,r13
    mov r12,r15
    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11

#include "fp_mul32_256_comba.inc"

    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
    ret

#else

.global fp_muln_low_half

fp_muln_low_half:
#include "fp_mul_128_comba.inc"
    ret

fp_muln_low:
    /* r15 = x
     * r14 = y
     * r13 = z
     * r12 = t
     */

    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11
    mov r13,r12
    mov r15,r13
    mov r12,r15
    /* temp area */
    sub #4*16,r1
    mov r1,r12


    /* t2 = x0 + x1
     * read: r14, r15
     * write: r4,r8-11
     * output: r4
     */

    /* a = r11
     * b = r10
     * c = r9
     * carry = r4
     */
    mov r15, r11
    mov r15, r10
    add #16,r10
    mov r12, r9
    add #16*2,r9
    /* sum */
    mov @r11+,r8
    add @r10+,r8
    mov r8,2*0(r9)
    mov @r11+,r8
    addc @r10+,r8
    mov r8,2*1(r9)
    mov @r11+,r8
    addc @r10+,r8
    mov r8,2*2(r9)
    mov @r11+,r8
    addc @r10+,r8
    mov r8,2*3(r9)
    mov @r11+,r8
    addc @r10+,r8
    mov r8,2*4(r9)
    mov @r11+,r8
    addc @r10+,r8
    mov r8,2*5(r9)
    mov @r11+,r8
    addc @r10+,r8
    mov r8,2*6(r9)
    mov @r11+,r8
    addc @r10+,r8
    mov r8,2*7(r9)
    clr r4
    adc r4

    /* t3 = y0 + y1 */
    /* read: r14, r15
     * write: r4,r8-11
     * output: r4
     */

    /* a = r11
     * b = r10
     * c = r9
     * carry = r4
     */
    mov r14, r11
    mov r14, r10
    add #16,r10
    mov r12, r9
    add #16*3,r9
    /* sum */
    mov @r11+,r8
    add @r10+,r8
    mov r8,2*0(r9)
    mov @r11+,r8
    addc @r10+,r8
    mov r8,2*1(r9)
    mov @r11+,r8
    addc @r10+,r8
    mov r8,2*2(r9)
    mov @r11+,r8
    addc @r10+,r8
    mov r8,2*3(r9)
    mov @r11+,r8
    addc @r10+,r8
    mov r8,2*4(r9)
    mov @r11+,r8
    addc @r10+,r8
    mov r8,2*5(r9)
    mov @r11+,r8
    addc @r10+,r8
    mov r8,2*6(r9)
    mov @r11+,r8
    addc @r10+,r8
    mov r8,2*7(r9)
    swpb r4
    adc r4

    /* z0z1 = x0.y0 */
    /* read: r13-15
     * write: all
     * stack end: c z
     */

    push r4
    push r13
    call #fp_muln_low_half

    /* z2z3 = x1.y1 */
    /* read: r13-15
     * write: all
     */

    add #16,r15
    add #16,r14
    add #16*2,r13
    call #fp_muln_low_half

    /* t0t1 = t2.t3 */
    /* read: r13-15
     * write: all
     * stack end:
     */

    /* we need t, the stack has two ints, so t = sp + 2*2 */
    mov r1,r15
    add #2*2,r15 /* t */
    mov r15,r14
    mov r15,r13
    add #16*2,r15
    add #16*3,r14
    call #fp_muln_low_half
    mov r13,r15 /* t */
    pop r14 /* z */
    pop r13 /* c */

    /* move t1 to registers */

    mov r15,r12
    add #16,r12
    mov @r12+,r4
    mov @r12+,r5
    mov @r12+,r6
    mov @r12+,r7
    mov @r12+,r8
    mov @r12+,r9
    mov @r12+,r10
    mov @r12+,r11

    /* t1 += t2 */

    clr r12 /* r12 holds the carry now */
    bit #1,r13
    jz skip1
    add #16*2,r15
    /* inc */
    add @r15+,r4
    addc @r15+,r5
    addc @r15+,r6
    addc @r15+,r7
    addc @r15+,r8
    addc @r15+,r9
    addc @r15+,r10
    addc @r15+,r11
    adc r12
    sub #16*3,r15 /* recover t */
skip1:

    /* t1 += t3 */

    bit #0x100,r13
    jz skip2
    /* inc */
    add #16*3,r15
    add @r15+,r4
    addc @r15+,r5
    addc @r15+,r6
    addc @r15+,r7
    addc @r15+,r8
    addc @r15+,r9
    addc @r15+,r10
    addc @r15+,r11
    adc r12
    sub #16*4,r15 /* recover t */
skip2:

    /* if (c1&c2) c++;
     *
     */

    cmp #0x101,r13
    jne skip3
    inc r12
skip3:

    /* t0t1 -= z0z1 */

    /* t0 -= z0 */
    sub @r14+,2*0(r15)
    subc @r14+,2*1(r15)
    subc @r14+,2*2(r15)
    subc @r14+,2*3(r15)
    subc @r14+,2*4(r15)
    subc @r14+,2*5(r15)
    subc @r14+,2*6(r15)
    subc @r14+,2*7(r15)
    /* t1 -= z1 */
    subc @r14+,r4
    subc @r14+,r5
    subc @r14+,r6
    subc @r14+,r7
    subc @r14+,r8
    subc @r14+,r9
    subc @r14+,r10
    subc @r14+,r11
    /* sub carry */
    sbc r12

    /* t0t1 -= z2z3 */

    /* t0 -= z2 */
    sub @r14+,2*0(r15)
    subc @r14+,2*1(r15)
    subc @r14+,2*2(r15)
    subc @r14+,2*3(r15)
    subc @r14+,2*4(r15)
    subc @r14+,2*5(r15)
    subc @r14+,2*6(r15)
    subc @r14+,2*7(r15)
    /* t1 -= z3 */
    subc @r14+,r4
    subc @r14+,r5
    subc @r14+,r6
    subc @r14+,r7
    subc @r14+,r8
    subc @r14+,r9
    subc @r14+,r10
    subc @r14+,r11
    /* sub carry */
    sbc r12

    /* z1z2 += t0t1 */

    sub #16*3,r14 /* recover z */
    /* z1 += t0 */
    add @r15+,2*0(r14)
    addc @r15+,2*1(r14)
    addc @r15+,2*2(r14)
    addc @r15+,2*3(r14)
    addc @r15+,2*4(r14)
    addc @r15+,2*5(r14)
    addc @r15+,2*6(r14)
    addc @r15+,2*7(r14)
    /* z2 += t1 */
    addc r4,2*8(r14)
    addc r5,2*9(r14)
    addc r6,2*10(r14)
    addc r7,2*11(r14)
    addc r8,2*12(r14)
    addc r9,2*13(r14)
    addc r10,2*14(r14)
    addc r11,2*15(r14)
    /* add carry */
    adc r12

    /* add carry */

    add r12,2*16(r14)
    jnc skip4
    adc 2*17(r14)
    adc 2*18(r14)
    adc 2*19(r14)
    adc 2*20(r14)
    adc 2*21(r14)
    adc 2*22(r14)
    adc 2*23(r14)
skip4:

    /* delete temp area */
    add #16*4,r1
    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4

    ret

#endif
