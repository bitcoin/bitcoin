#ifndef _SECP256K1_FIELD_5x52_
#define _SECP256K1_FIELD_5x52_

#include <stdint.h>

typedef struct {
    // X = sum(i=0..4, elem[i]*2^52) mod n
    uint64_t n[5];
#ifdef VERIFY
    int magnitude;
    int normalized;
#endif
} secp256k1_fe_t;

#endif
