#ifdef MSP430X2

fp_sqrn_low:
    /* r15 = x
    * r14 = z
    * r13 = t
    */
    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11
    mov r15,r12
    mov r14,r15
    mov r12,r14

#include "fp_sqr32_256_comba.inc"

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

fp_sqrn_low_half:
#include "fp_sqr_128_comba.inc"
    ret

fp_sqrn_low:
    /* r15 = x
    * r14 = z
    * r13 = t
    */
    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11
    mov r15,r12
    mov r14,r15
    mov r12,r14
    /* temp area */
    sub #4*16,r1
    mov r1,r13

    /* z0z1 = x0.x0 */
    push r14
    push r13
    call #fp_sqrn_low_half

    /* z2z3 = x1.x1 */
    add #16,r15
    add #2*16,r14
    call #fp_sqrn_low_half

    /* t0t1 = x0.x1 */
    pop r13
    mov r15,r14
    sub #16,r15
    call #fp_muln_low_half
    pop r14

    /* z1z2 += t0t1 */
    /* r15: t
    * r14: z
    */
    mov r13,r15
    add #16,r14
    /* z1 += t0 */
    mov @r15+,r12
    rla r12
    mov @r15+,r11
    rlc r11
    mov @r15+,r10
    rlc r10
    mov @r15+,r9
    rlc r9
    mov @r15+,r8
    rlc r8
    mov @r15+,r7
    rlc r7
    mov @r15+,r6
    rlc r6
    mov @r15+,r5
    rlc r5
    clr r4
    adc r4

    add r12,2*0(r14)
    addc r11,2*1(r14)
    addc r10,2*2(r14)
    addc r9,2*3(r14)
    addc r8,2*4(r14)
    addc r7,2*5(r14)
    addc r6,2*6(r14)
    addc r5,2*7(r14)
    clr r13
    adc r13

    /* z2 += t1 */
    bis r4,r2 /* carry = r4 */

    mov @r15+,r12
    rlc r12
    mov @r15+,r11
    rlc r11
    mov @r15+,r10
    rlc r10
    mov @r15+,r9
    rlc r9
    mov @r15+,r8
    rlc r8
    mov @r15+,r7
    rlc r7
    mov @r15+,r6
    rlc r6
    mov @r15+,r5
    rlc r5
    clr r4
    adc r4

    bis r13,r2 /* carry = r13 */

    addc r12,2*8(r14)
    addc r11,2*9(r14)
    addc r10,2*10(r14)
    addc r9,2*11(r14)
    addc r8,2*12(r14)
    addc r7,2*13(r14)
    addc r6,2*14(r14)
    addc r5,2*15(r14)
    adc r4

    /* add carry */
    add r4,2*16(r14)
    jnc skip1
    adc 2*17(r14)
    adc 2*18(r14)
    adc 2*19(r14)
    adc 2*20(r14)
    adc 2*21(r14)
    adc 2*22(r14)
    adc 2*23(r14)
skip1:

    /* delete temp area */
    add #4*16,r1
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
