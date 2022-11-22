
  
    push r13

  clr r10
  clr r11
  mov @r15,r9
  
  mov #RESLO,r4
  mov #RESHI,r5

  mov #SUMEXT,r8
  push r2
  dint
  nop

  /* COLEND_STAGE1_A_0STEP */
  mov r9,&MPY
  mov r12,&OP2
  mov @r4,@r15
  mov #13767,&MPY
  mov @r15+,&OP2
  add @r4,r9
  addc @r5,r10
  adc r11

  /* COLEND_STAGE1_B */
  add @r15,r10
  adc r11

    /* DEC */
    sub #2*1,r15

  /* STEP */
  
  mov @r15+,r6

  mov r6,&MPY

  
  mov #3232,&OP2

  /* COLEND_STAGE1_A_1STEP */
  add @r4,r10
  addc @r5,r11
  adc r9
  mov r10,&MPY
  mov r12,&OP2
  mov @r4,@r15
  mov #13767,&MPY
  mov @r15+,&OP2
  add @r4,r10
  addc @r5,r11
  adc r9

  /* COLEND_STAGE1_B */
  add @r15,r11
  adc r9

  /* STEP */
  
  mov r6,&MPY

  
  mov #60544,&OP2

    /* DEC */
    sub #2*1,r15

  /* STEP */
  
  mov @r15+,r7

  mov r7,&MAC

  
  mov #3232,&OP2

  /* COLEND_STAGE1_A_2MORE_STEPS */
  add @r4,r11
  addc @r5,r9
  adc r10
  add @r8,r10
  mov r11,&MPY
  mov r12,&OP2
  mov @r4,@r15
  mov #13767,&MPY
  mov @r15+,&OP2
  add @r4,r11
  addc @r5,r9
  adc r10

  /* COLEND_STAGE1_B */
  add @r15,r9
  adc r10

  /* STEP */
  
  mov r6,&MPY

  
  mov #16799,&OP2

  /* STEP */
  
  mov r7,&MAC

  
  mov #60544,&OP2

    /* DEC */
    sub #2*1,r15

  /* STEP */
  
  mov @r15+,r13

  mov r13,&MAC

  
  add @r8,r11

  mov #3232,&OP2

  /* COLEND_STAGE1_A_2MORE_STEPS */
  add @r4,r9
  addc @r5,r10
  adc r11
  add @r8,r11
  mov r9,&MPY
  mov r12,&OP2
  mov @r4,@r15
  mov #13767,&MPY
  mov @r15+,&OP2
  add @r4,r9
  addc @r5,r10
  adc r11

  /* COLEND_STAGE1_B */
  add @r15,r10
  adc r11

  /* STEP */
  
  mov r6,&MPY

  
  mov #49152,&OP2

  /* STEP */
  
  mov r7,&MAC

  
  mov #16799,&OP2

  /* STEP */
  
  mov r13,&MAC

  
  add @r8,r9

  mov #60544,&OP2

    /* DEC */
    sub #2*1,r15

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r9

  mov #3232,&OP2

  /* COLEND_STAGE1_A_2MORE_STEPS */
  add @r4,r10
  addc @r5,r11
  adc r9
  add @r8,r9
  mov r10,&MPY
  mov r12,&OP2
  mov @r4,@r15
  mov #13767,&MPY
  mov @r15+,&OP2
  add @r4,r10
  addc @r5,r11
  adc r9

  /* COLEND_STAGE1_B */
  add @r15,r11
  adc r9

  /* STEP */
  
  mov r6,&MPY

  
  mov #32745,&OP2

  /* STEP */
  
  mov r7,&MAC

  
  mov #49152,&OP2

  /* STEP */
  
  mov r13,&MAC

  
  add @r8,r10

  mov #16799,&OP2

    /* DEC */
    sub #2*2,r15

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r10

  mov #60544,&OP2

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r10

  mov #3232,&OP2

  /* COLEND_STAGE1_A_2MORE_STEPS */
  add @r4,r11
  addc @r5,r9
  adc r10
  add @r8,r10
  mov r11,&MPY
  mov r12,&OP2
  mov @r4,@r15
  mov #13767,&MPY
  mov @r15+,&OP2
  add @r4,r11
  addc @r5,r9
  adc r10

  /* COLEND_STAGE1_B */
  add @r15,r9
  adc r10

  /* STEP */
  
  mov r7,&MPY

  
  mov #32745,&OP2

  /* STEP */
  
  mov r13,&MAC

  
  mov #49152,&OP2

    /* DEC */
    sub #2*3,r15

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r11

  mov #16799,&OP2

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r11

  mov #60544,&OP2

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r11

  mov #3232,&OP2

  /* COLEND_STAGE1_A_2MORE_STEPS */
  add @r4,r9
  addc @r5,r10
  adc r11
  add @r8,r11
  mov r9,&MPY
  mov r12,&OP2
  mov @r4,@r15
  mov #13767,&MPY
  mov @r15+,&OP2
  add @r4,r9
  addc @r5,r10
  adc r11

  /* COLEND_STAGE1_B */
  add @r15,r10
  adc r11

  /* STEP */
  
  mov r6,&MPY

  
  mov #28368,&OP2

  /* STEP */
  
  mov r13,&MAC

  
  mov #32745,&OP2

    /* DEC */
    sub #2*4,r15

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r9

  mov #49152,&OP2

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r9

  mov #16799,&OP2

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r9

  mov #60544,&OP2

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r9

  mov #3232,&OP2

  /* COLEND_STAGE1_A_2MORE_STEPS */
  add @r4,r10
  addc @r5,r11
  adc r9
  add @r8,r9
  mov r10,&MPY
  mov r12,&OP2
  mov @r4,@r15
  mov #13767,&MPY
  mov @r15+,&OP2
  add @r4,r10
  addc @r5,r11
  adc r9

  /* COLEND_STAGE1_B */
  add @r15,r11
  adc r9

  /* STEP */
  
  mov r7,&MPY

  
  mov #28368,&OP2

    /* DEC */
    sub #2*5,r15

  /* STEP */
  
  mov @r15+,&MAC

  
  mov #32745,&OP2

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r10

  mov #49152,&OP2

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r10

  mov #16799,&OP2

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r10

  mov #60544,&OP2

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r10

  mov #3232,&OP2

  /* COLEND_STAGE1_A_2MORE_STEPS */
  add @r4,r11
  addc @r5,r9
  adc r10
  add @r8,r10
  mov r11,&MPY
  mov r12,&OP2
  mov @r4,@r15
  mov #13767,&MPY
  mov @r15+,&OP2
  add @r4,r11
  addc @r5,r9
  adc r10

  /* COLEND_STAGE1_B */
  add @r15,r9
  adc r10

  /* STEP */
  
  mov r6,&MPY

  
  mov #9216,&OP2

  /* STEP */
  
  mov r13,&MAC

  
  mov #28368,&OP2

    /* DEC */
    sub #2*5,r15

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r11

  mov #32745,&OP2

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r11

  mov #49152,&OP2

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r11

  mov #16799,&OP2

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r11

  mov #60544,&OP2

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r11

  mov #3232,&OP2

  /* COLEND_STAGE1_A_2MORE_STEPS */
  add @r4,r9
  addc @r5,r10
  adc r11
  add @r8,r11
  mov r9,&MPY
  mov r12,&OP2
  mov @r4,@r15
  mov #13767,&MPY
  mov @r15+,&OP2
  add @r4,r9
  addc @r5,r10
  adc r11

  /* COLEND_STAGE1_B */
  add @r15,r10
  adc r11


  /* STATE CHANGE */


    /* DEC */
    sub #2*9,r15

  /* STEP */
  
  mov @r15+,&MPY

  
  mov #9216,&OP2

    /* INC */
    add #2*1,r15

  /* STEP */
  
  mov @r15+,&MAC

  
  mov #28368,&OP2

    /* INC */
    add #2*1,r15

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r9

  mov #32745,&OP2

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r9

  mov #49152,&OP2

  /* STEP */
  
  mov @r15+,r6

  mov r6,&MAC

  
  add @r8,r9

  mov #16799,&OP2

  /* STEP */
  
  mov @r15+,r7

  mov r7,&MAC

  
  add @r8,r9

  mov #60544,&OP2

  /* STEP */
  
  mov @r15+,r13

  mov r13,&MAC

  
  add @r8,r9

  mov #3232,&OP2

  /* COLEND_STAGE2_A_2MORE_STEPS */
  add @r4,r10
  addc @r5,r11
  adc r9
  add @r8,r9
  mov r10,2*0(r14)

  /* COLEND_STAGE2_B */
  clr r10
  add 2*1(r15),r11
  adc r9

    /* DEC */
    sub #2*8,r15

  /* STEP */
  
  mov @r15+,&MPY

  
  mov #9216,&OP2

    /* INC */
    add #2*1,r15

  /* STEP */
  
  mov @r15+,&MAC

  
  mov #28368,&OP2

    /* INC */
    add #2*1,r15

  /* STEP */
  
  mov @r15+,&MAC

  
  add @r8,r10

  mov #32745,&OP2

  /* STEP */
  
  mov r6,&MAC

  
  add @r8,r10

  mov #49152,&OP2

  /* STEP */
  
  mov r7,&MAC

  
  add @r8,r10

  mov #16799,&OP2

  /* STEP */
  
  mov r13,&MAC

  
  add @r8,r10

  mov #60544,&OP2

  /* COLEND_STAGE2_A_2MORE_STEPS */
  add @r4,r11
  addc @r5,r9
  adc r10
  add @r8,r10
  mov r11,2*1(r14)

  /* COLEND_STAGE2_B */
  clr r11
  add 2*5(r15),r9
  adc r10

    /* DEC */
    sub #2*4,r15

  /* STEP */
  
  mov @r15+,&MPY

  
  mov #9216,&OP2

    /* INC */
    add #2*1,r15

  /* STEP */
  
  mov @r15+,&MAC

  
  mov #28368,&OP2

  /* STEP */
  
  mov r6,&MAC

  
  add @r8,r11

  mov #32745,&OP2

  /* STEP */
  
  mov r7,&MAC

  
  add @r8,r11

  mov #49152,&OP2

  /* STEP */
  
  mov r13,&MAC

  
  add @r8,r11

  mov #16799,&OP2

  /* COLEND_STAGE2_A_2MORE_STEPS */
  add @r4,r9
  addc @r5,r10
  adc r11
  add @r8,r11
  mov r9,2*2(r14)

  /* COLEND_STAGE2_B */
  clr r9
  add 2*7(r15),r10
  adc r11

    /* DEC */
    sub #2*2,r15

  /* STEP */
  
  mov @r15+,&MPY

  
  mov #9216,&OP2

    /* INC */
    add #2*1,r15

  /* STEP */
  
  mov @r15+,&MAC

  
  mov #28368,&OP2

  /* STEP */
  
  mov r7,&MAC

  
  add @r8,r9

  mov #32745,&OP2

  /* STEP */
  
  mov r13,&MAC

  
  add @r8,r9

  mov #49152,&OP2

  /* COLEND_STAGE2_A_2MORE_STEPS */
  add @r4,r10
  addc @r5,r11
  adc r9
  add @r8,r9
  mov r10,2*3(r14)

  /* COLEND_STAGE2_B */
  clr r10
  add 2*7(r15),r11
  adc r9

    /* DEC */
    sub #2*2,r15

  /* STEP */
  
  mov @r15+,&MPY

  
  mov #9216,&OP2

  /* STEP */
  
  mov r6,&MAC

  
  mov #28368,&OP2

  /* STEP */
  
  mov r13,&MAC

  
  add @r8,r10

  mov #32745,&OP2

  /* COLEND_STAGE2_A_2MORE_STEPS */
  add @r4,r11
  addc @r5,r9
  adc r10
  add @r8,r10
  mov r11,2*4(r14)

  /* COLEND_STAGE2_B */
  clr r11
  add 2*9(r15),r9
  adc r10

  /* STEP */
  
  mov @r15+,&MPY

  
  mov #9216,&OP2

  /* STEP */
  
  mov r7,&MAC

  
  mov #28368,&OP2

  /* COLEND_STAGE2_A_2MORE_STEPS */
  add @r4,r9
  addc @r5,r10
  adc r11
  add @r8,r11
  mov r9,2*5(r14)

  /* COLEND_STAGE2_B */
  clr r9
  add 2*9(r15),r10
  adc r11

  /* STEP */
  
  mov r6,&MPY

  
  mov #9216,&OP2

  /* STEP */
  
  mov r13,&MAC

  
  mov #28368,&OP2

  /* COLEND_STAGE2_A_2MORE_STEPS */
  add @r4,r10
  addc @r5,r11
  adc r9
  add @r8,r9
  mov r10,2*6(r14)

  /* COLEND_STAGE2_B */
  clr r10
  add 2*10(r15),r11
  adc r9

  /* STEP */
  
  mov r7,&MPY

  
  mov #9216,&OP2

  /* COLEND_STAGE2_A_1STEP */
  add @r4,r11
  addc @r5,r9
  adc r10
  mov r11,2*7(r14)

  /* COLEND_STAGE2_B */
  clr r11
  add 2*11(r15),r9
  adc r10

  /* STEP */
  
  mov r13,&MPY

  
  mov #9216,&OP2

  /* COLEND_STAGE2_A_1STEP */
  add @r4,r9
  addc @r5,r10
  adc r11
  mov r9,2*8(r14)

  /* COLEND_STAGE2_B */
  clr r9
  add 2*12(r15),r10
  adc r11

    /* DEC */
    sub #2*7,r15

  mov r10,2*9(r14)
  pop r2
  eint
  
    pop r13

