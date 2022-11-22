#include "relic_conf.h"

.text
.align 2
.global bn_addn_low
.global bn_subn_low


bn_addn_low:
    push r9
    push r10
    push r11
    clr r11
back1:
    tst r12
    jz skip1
    mov @r14+,r9
    add @r13+,r9
    clr r10
    adc r10
    add r11,r9
    clr r11
    adc r11
    mov r9,@r15
    bis r10,r11
    incd r15
    dec r12
    jmp back1
skip1:
    mov r11,r15
    pop r11
    pop r10
    pop r9
    ret

bn_subn_low:
    push r9
    push r10
    push r11
    clr r11
back2:
    tst r12
    jz skip3
    mov @r14+,r9
    sub @r13+,r9
    clr r10
    sbc r10
    tst r11
    jz skip2
    add r11,r9
    clr r11
    sbc r11
    bis r11,r10
skip2:
    mov r10,r11
    mov r9,@r15
    incd r15
    dec r12
    jmp back2
skip3:
    mov r11,r15
    and #1,r15
    pop r11
    pop r10
    pop r9
    ret
