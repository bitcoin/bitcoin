//
//  WELL512a.c
//  BitcoinEcc
//
//  Created by Harry Oh on 25/07/2019.
//  Copyright © 2019 Harry Oh. All rights reserved.
//

#include "WELL512a.h"

void InitWELLRNG512a( const uint32_t *init){
  int j;
  state_i = 0;
  for (j = 0; j < R; j++)
    STATE[j] = init[j];
}

uint32_t WELLRNG512a (void){
  uint32_t z1, z2, tval;
  uint32_t a, c;
  
  a = STATE[state_i];
  c = STATE[(state_i + 13) & 0xf];
  z1    = a ^ (a << 16) ^ c ^ (c << 15);
  a = STATE[(state_i + 9) & 0xf];
  z2 = a ^ (a >> 11);
  tval = STATE[state_i] = z1 ^ z2;
  state_i = (state_i + 15) & 0xfU;
  STATE[state_i] ^= (STATE[state_i] << 2) ^ (z1 << 18) ^ z2
  ^ (z2 << 28) ^ ((tval << 5) & 0xda442d24U);
  return (STATE[state_i]);
}
