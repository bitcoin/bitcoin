fp_sqrn_low:
    mov r15,r12
    mov r14,r15
    mov r12,r14
    push r4
    push r5
    push r6
    push r7
    push r8
    push r9
    push r10
    push r11

#ifdef MSP430X2

#include "fp_sqr32_160_comba.inc"

#else

#include "fp_sqr_160_comba.inc"

#endif

    pop r11
    pop r10
    pop r9
    pop r8
    pop r7
    pop r6
    pop r5
    pop r4
    ret

