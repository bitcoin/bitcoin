#ifndef _SECP256K1_NUM_GMP_
#define _SECP256K1_NUM_GMP_

#include <gmp.h>

extern "C" {

typedef struct {
    mpz_t bn;
} secp256k1_num_t;

}

#endif
