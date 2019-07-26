#ifndef WELL512a_h
#define WELL512a_h

#include <stdio.h>
#include <stdint.h>

void InitWELLRNG512a( const uint32_t *init);
uint32_t WELLRNG512a( void);

#define R 16

static uint32_t state_i = 0;
static uint32_t STATE[R];

#endif
