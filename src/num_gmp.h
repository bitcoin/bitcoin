#ifndef _SECP256K1_NUM_REPR_
#define _SECP256K1_NUM_REPR_

#include <gmp.h>

typedef struct {
    mpz_t bn;
} secp256k1_num_t;

#endif
