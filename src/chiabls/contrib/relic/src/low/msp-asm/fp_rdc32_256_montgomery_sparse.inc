
  
  clr r9
  clr r10
  clr r11
  mov @r15+,r5
  mov @r15+,r6
  sub #2*2,r15
  mov #RES0,r4
  mov #SUMEXT,r8
  push r2
  dint
  nop
  bis #MPYDLYWRTEN,&MPY32CTL0
  bic #MPYDLY32,&MPY32CTL0

  /* COLEND_STAGE1_A_0STEP */
  mov r5,&MPY32L
  mov r6,&MPY32H
  mov #13797,&OP2L
  mov #55188,&OP2H
  mov #19,&MPY32L
  mov #0,&MPY32H
  mov @r4+,0(r15)
  mov @r4+,2(r15)
  mov @r15+,&OP2L
  mov @r15+,&OP2H
  sub #2*2,r4
  add @r4+,r5
  addc @r4+,r6
  addc @r4+,r9
  addc @r4+,r10
  adc r11
  sub #2*4,r4

  /* COLEND_STAGE1_B */
  add @r15+,r9
  addc @r15+,r10
  adc r11

    /* DEC */
    sub #2*4,r15

  /* STEP */
  
  mov @r15+,&MPY32L
  mov @r15+,&MPY32H

  
  mov #0,&OP2L
  mov #42752,&OP2H

  /* COLEND_STAGE1_A_1STEP */
  nop
  add @r4+,r9
  addc @r4+,r10
  addc @r4+,r11
  addc @r4+,r5
  adc r6
  sub #2*4,r4
  mov r9,&MPY32L
  mov r10,&MPY32H
  mov #13797,&OP2L
  mov #55188,&OP2H
  mov #19,&MPY32L
  mov #0,&MPY32H
  mov @r4+,0(r15)
  mov @r4+,2(r15)
  mov @r15+,&OP2L
  mov @r15+,&OP2H
  sub #2*2,r4
  add @r4+,r9
  addc @r4+,r10
  addc @r4+,r11
  addc @r4+,r5
  adc r6
  sub #2*4,r4

  /* COLEND_STAGE1_B */
  add @r15+,r11
  addc @r15+,r5
  adc r6

    /* DEC */
    sub #2*6,r15

  /* STEP */
  
  mov @r15+,&MPY32L
  mov @r15+,&MPY32H

  
  mov #19,&OP2

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  mov #0,&OP2L
  mov #42752,&OP2H

  /* COLEND_STAGE1_A_2MORE_STEPS */
  nop
  add @r4+,r11
  addc @r4+,r5
  addc @r4+,r6
  addc @r4+,r9
  adc r10
  add @r8,r10
  sub #2*4,r4
  mov r11,&MPY32L
  mov r5,&MPY32H
  mov #13797,&OP2L
  mov #55188,&OP2H
  mov #19,&MPY32L
  mov #0,&MPY32H
  mov @r4+,0(r15)
  mov @r4+,2(r15)
  mov @r15+,&OP2L
  mov @r15+,&OP2H
  sub #2*2,r4
  add @r4+,r11
  addc @r4+,r5
  addc @r4+,r6
  addc @r4+,r9
  adc r10
  sub #2*4,r4

  /* COLEND_STAGE1_B */
  add @r15+,r6
  addc @r15+,r9
  adc r10

    /* DEC */
    sub #2*8,r15

  /* STEP */
  
  mov @r15+,&MPY32L
  mov @r15+,&MPY32H

  
  mov #0,&OP2L
  mov #24865,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  mov #19,&OP2

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r5

  mov #0,&OP2L
  mov #42752,&OP2H

  /* COLEND_STAGE1_A_2MORE_STEPS */
  nop
  add @r4+,r6
  addc @r4+,r9
  addc @r4+,r10
  addc @r4+,r11
  adc r5
  add @r8,r5
  sub #2*4,r4
  mov r6,&MPY32L
  mov r9,&MPY32H
  mov #13797,&OP2L
  mov #55188,&OP2H
  mov #19,&MPY32L
  mov #0,&MPY32H
  mov @r4+,0(r15)
  mov @r4+,2(r15)
  mov @r15+,&OP2L
  mov @r15+,&OP2H
  sub #2*2,r4
  add @r4+,r6
  addc @r4+,r9
  addc @r4+,r10
  addc @r4+,r11
  adc r5
  sub #2*4,r4

  /* COLEND_STAGE1_B */
  add @r15+,r10
  addc @r15+,r11
  adc r5

    /* DEC */
    sub #2*10,r15

  /* STEP */
  
  mov @r15+,&MPY32L
  mov @r15+,&MPY32H

  
  mov #8,&OP2

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  mov #0,&OP2L
  mov #24865,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r9

  mov #19,&OP2

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r9

  mov #0,&OP2L
  mov #42752,&OP2H

  /* COLEND_STAGE1_A_2MORE_STEPS */
  nop
  add @r4+,r10
  addc @r4+,r11
  addc @r4+,r5
  addc @r4+,r6
  adc r9
  add @r8,r9
  sub #2*4,r4
  mov r10,&MPY32L
  mov r11,&MPY32H
  mov #13797,&OP2L
  mov #55188,&OP2H
  mov #19,&MPY32L
  mov #0,&MPY32H
  mov @r4+,0(r15)
  mov @r4+,2(r15)
  mov @r15+,&OP2L
  mov @r15+,&OP2H
  sub #2*2,r4
  add @r4+,r10
  addc @r4+,r11
  addc @r4+,r5
  addc @r4+,r6
  adc r9
  sub #2*4,r4

  /* COLEND_STAGE1_B */
  add @r15+,r5
  addc @r15+,r6
  adc r9

    /* DEC */
    sub #2*12,r15

  /* STEP */
  
  mov @r15+,&MPY32L
  mov @r15+,&MPY32H

  
  mov #19840,&OP2L
  mov #47668,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  mov #8,&OP2

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r11

  mov #0,&OP2L
  mov #24865,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r11

  mov #19,&OP2

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r11

  mov #0,&OP2L
  mov #42752,&OP2H

  /* COLEND_STAGE1_A_2MORE_STEPS */
  nop
  add @r4+,r5
  addc @r4+,r6
  addc @r4+,r9
  addc @r4+,r10
  adc r11
  add @r8,r11
  sub #2*4,r4
  mov r5,&MPY32L
  mov r6,&MPY32H
  mov #13797,&OP2L
  mov #55188,&OP2H
  mov #19,&MPY32L
  mov #0,&MPY32H
  mov @r4+,0(r15)
  mov @r4+,2(r15)
  mov @r15+,&OP2L
  mov @r15+,&OP2H
  sub #2*2,r4
  add @r4+,r5
  addc @r4+,r6
  addc @r4+,r9
  addc @r4+,r10
  adc r11
  sub #2*4,r4

  /* COLEND_STAGE1_B */
  add @r15+,r9
  addc @r15+,r10
  adc r11

    /* DEC */
    sub #2*14,r15

  /* STEP */
  
  mov @r15+,&MPY32L
  mov @r15+,&MPY32H

  
  mov #1,&OP2L
  mov #16384,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  mov #19840,&OP2L
  mov #47668,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r6

  mov #8,&OP2

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r6

  mov #0,&OP2L
  mov #24865,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r6

  mov #19,&OP2

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r6

  mov #0,&OP2L
  mov #42752,&OP2H

  /* COLEND_STAGE1_A_2MORE_STEPS */
  nop
  add @r4+,r9
  addc @r4+,r10
  addc @r4+,r11
  addc @r4+,r5
  adc r6
  add @r8,r6
  sub #2*4,r4
  mov r9,&MPY32L
  mov r10,&MPY32H
  mov #13797,&OP2L
  mov #55188,&OP2H
  mov #19,&MPY32L
  mov #0,&MPY32H
  mov @r4+,0(r15)
  mov @r4+,2(r15)
  mov @r15+,&OP2L
  mov @r15+,&OP2H
  sub #2*2,r4
  add @r4+,r9
  addc @r4+,r10
  addc @r4+,r11
  addc @r4+,r5
  adc r6
  sub #2*4,r4

  /* COLEND_STAGE1_B */
  add @r15+,r11
  addc @r15+,r5
  adc r6

    /* DEC */
    sub #2*16,r15

  /* STEP */
  
  mov @r15+,&MPY32L
  mov @r15+,&MPY32H

  
  mov #25730,&OP2L
  mov #9507,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  mov #1,&OP2L
  mov #16384,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r10

  mov #19840,&OP2L
  mov #47668,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r10

  mov #8,&OP2

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r10

  mov #0,&OP2L
  mov #24865,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r10

  mov #19,&OP2

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r10

  mov #0,&OP2L
  mov #42752,&OP2H

  /* COLEND_STAGE1_A_2MORE_STEPS */
  nop
  add @r4+,r11
  addc @r4+,r5
  addc @r4+,r6
  addc @r4+,r9
  adc r10
  add @r8,r10
  sub #2*4,r4
  mov r11,&MPY32L
  mov r5,&MPY32H
  mov #13797,&OP2L
  mov #55188,&OP2H
  mov #19,&MPY32L
  mov #0,&MPY32H
  mov @r4+,0(r15)
  mov @r4+,2(r15)
  mov @r15+,&OP2L
  mov @r15+,&OP2H
  sub #2*2,r4
  add @r4+,r11
  addc @r4+,r5
  addc @r4+,r6
  addc @r4+,r9
  adc r10
  sub #2*4,r4

  /* COLEND_STAGE1_B */
  add @r15+,r6
  addc @r15+,r9
  adc r10


  /* STATE CHANGE */


    /* DEC */
    sub #2*16,r15

  /* STEP */
  
  mov @r15+,&MPY32L
  mov @r15+,&MPY32H

  
  mov #25730,&OP2L
  mov #9507,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  mov #1,&OP2L
  mov #16384,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r5

  mov #19840,&OP2L
  mov #47668,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r5

  mov #8,&OP2

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r5

  mov #0,&OP2L
  mov #24865,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r5

  mov #19,&OP2

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r5

  mov #0,&OP2L
  mov #42752,&OP2H

  /* COLEND_STAGE2_A_2MORE_STEPS */
  nop
  add @r4+,r6
  addc @r4+,r9
  addc @r4+,r10
  addc @r4+,r11
  adc r5
  add @r8,r5
  sub #2*4,r4
  mov r6,2*0(r14)
  mov r9,2*(0+1)(r14)

  /* COLEND_STAGE2_B */
  clr r6
  clr r9
  add 2*2(r15),r10
  addc 2*(2+1)(r15),r11
  adc r5

    /* DEC */
    sub #2*12,r15

  /* STEP */
  
  mov @r15+,&MPY32L
  mov @r15+,&MPY32H

  
  mov #25730,&OP2L
  mov #9507,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  mov #1,&OP2L
  mov #16384,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r9

  mov #19840,&OP2L
  mov #47668,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r9

  mov #8,&OP2

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r9

  mov #0,&OP2L
  mov #24865,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r9

  mov #19,&OP2

  /* COLEND_STAGE2_A_2MORE_STEPS */
  nop
  add @r4+,r10
  addc @r4+,r11
  addc @r4+,r5
  addc @r4+,r6
  adc r9
  add @r8,r9
  sub #2*4,r4
  mov r10,2*2(r14)
  mov r11,2*(2+1)(r14)

  /* COLEND_STAGE2_B */
  clr r10
  clr r11
  add 2*4(r15),r5
  addc 2*(4+1)(r15),r6
  adc r9

    /* DEC */
    sub #2*10,r15

  /* STEP */
  
  mov @r15+,&MPY32L
  mov @r15+,&MPY32H

  
  mov #25730,&OP2L
  mov #9507,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  mov #1,&OP2L
  mov #16384,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r11

  mov #19840,&OP2L
  mov #47668,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r11

  mov #8,&OP2

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r11

  mov #0,&OP2L
  mov #24865,&OP2H

  /* COLEND_STAGE2_A_2MORE_STEPS */
  nop
  add @r4+,r5
  addc @r4+,r6
  addc @r4+,r9
  addc @r4+,r10
  adc r11
  add @r8,r11
  sub #2*4,r4
  mov r5,2*4(r14)
  mov r6,2*(4+1)(r14)

  /* COLEND_STAGE2_B */
  clr r5
  clr r6
  add 2*6(r15),r9
  addc 2*(6+1)(r15),r10
  adc r11

    /* DEC */
    sub #2*8,r15

  /* STEP */
  
  mov @r15+,&MPY32L
  mov @r15+,&MPY32H

  
  mov #25730,&OP2L
  mov #9507,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  mov #1,&OP2L
  mov #16384,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r6

  mov #19840,&OP2L
  mov #47668,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r6

  mov #8,&OP2

  /* COLEND_STAGE2_A_2MORE_STEPS */
  nop
  add @r4+,r9
  addc @r4+,r10
  addc @r4+,r11
  addc @r4+,r5
  adc r6
  add @r8,r6
  sub #2*4,r4
  mov r9,2*6(r14)
  mov r10,2*(6+1)(r14)

  /* COLEND_STAGE2_B */
  clr r9
  clr r10
  add 2*8(r15),r11
  addc 2*(8+1)(r15),r5
  adc r6

    /* DEC */
    sub #2*6,r15

  /* STEP */
  
  mov @r15+,&MPY32L
  mov @r15+,&MPY32H

  
  mov #25730,&OP2L
  mov #9507,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  mov #1,&OP2L
  mov #16384,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  add @r8,r10

  mov #19840,&OP2L
  mov #47668,&OP2H

  /* COLEND_STAGE2_A_2MORE_STEPS */
  nop
  add @r4+,r11
  addc @r4+,r5
  addc @r4+,r6
  addc @r4+,r9
  adc r10
  add @r8,r10
  sub #2*4,r4
  mov r11,2*8(r14)
  mov r5,2*(8+1)(r14)

  /* COLEND_STAGE2_B */
  clr r11
  clr r5
  add 2*10(r15),r6
  addc 2*(10+1)(r15),r9
  adc r10

    /* DEC */
    sub #2*4,r15

  /* STEP */
  
  mov @r15+,&MPY32L
  mov @r15+,&MPY32H

  
  mov #25730,&OP2L
  mov #9507,&OP2H

  /* STEP */
  
  mov @r15+,&MAC32L
  mov @r15+,&MAC32H

  
  mov #1,&OP2L
  mov #16384,&OP2H

  /* COLEND_STAGE2_A_2MORE_STEPS */
  nop
  add @r4+,r6
  addc @r4+,r9
  addc @r4+,r10
  addc @r4+,r11
  adc r5
  add @r8,r5
  sub #2*4,r4
  mov r6,2*10(r14)
  mov r9,2*(10+1)(r14)

  /* COLEND_STAGE2_B */
  clr r6
  clr r9
  add 2*12(r15),r10
  addc 2*(12+1)(r15),r11
  adc r5

    /* DEC */
    sub #2*2,r15

  /* STEP */
  
  mov @r15+,&MPY32L
  mov @r15+,&MPY32H

  
  mov #25730,&OP2L
  mov #9507,&OP2H

  /* COLEND_STAGE2_A_1STEP */
  nop
  add @r4+,r10
  addc @r4+,r11
  addc @r4+,r5
  addc @r4+,r6
  adc r9
  sub #2*4,r4
  mov r10,2*12(r14)
  mov r11,2*(12+1)(r14)

  /* COLEND_STAGE2_B */
  clr r10
  clr r11
  add 2*14(r15),r5
  addc 2*(14+1)(r15),r6
  adc r9

    /* DEC */
    sub #2*16,r15

  mov r5,2*14(r14)
  mov r6,2*(14+1)(r14)
  mov r9,r11
  pop r2
  eint
  
