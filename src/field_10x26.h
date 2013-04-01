#ifndef _SECP256K1_FIELD_10x26_
#define _SECP256K1_FIELD_10x26_

#include <stdint.h>

typedef struct {
    // X = sum(i=0..9, elem[i]*2^26) mod n
    uint32_t n[10];
#ifdef VERIFY
    int magnitude;
    int normalized;
#endif
} secp256k1_fe_t;

#endif
