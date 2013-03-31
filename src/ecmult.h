#ifndef _SECP256K1_ECMULT_
#define _SECP256K1_ECMULT_

#include "num.h"
#include "group.h"

namespace secp256k1 {

void ECMultBase(secp256k1_gej_t &out, const secp256k1_num_t &gn);
void ECMult(secp256k1_gej_t &out, const secp256k1_gej_t &a, const secp256k1_num_t &an, const secp256k1_num_t &gn);

}

#endif
