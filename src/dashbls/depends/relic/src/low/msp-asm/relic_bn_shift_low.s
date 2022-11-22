#include "relic_conf.h"

.text
.align 2
.global bn_rsh1_low


bn_rsh1_low:
    push r10
    push r11
    dec r13
    rla r13
    add r13,r15
    add r13,r14
    rra r13
    inc r13
    clr r10
back:
    tst r13
    jz skip
    mov @r14,r12
    clrc
    rrc r12
    clr r11
    rrc r11
    bis r12,r10
    mov r10,@r15
    mov r11,r10
    decd r15
    decd r14
    dec r13
    jmp back
skip:
    rla r10
    rlc r10
    pop r11
    pop r10
    ret
