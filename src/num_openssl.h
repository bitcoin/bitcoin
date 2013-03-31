#ifndef _SECP256K1_NUM_OPENSSL_
#define _SECP256K1_NUM_OPENSSL_

#include <openssl/bn.h>

typedef struct {
    BIGNUM bn;
} secp256k1_num_t;

#endif
