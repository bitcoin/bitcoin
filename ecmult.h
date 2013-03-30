#ifndef _SECP256K1_ECMULT_
#define _SECP256K1_ECMULT_

#include "group.h"
#include "num.h"

namespace secp256k1 {

void ECMultBase(GroupElemJac &out, const secp256k1_num_t &gn);
void ECMult(GroupElemJac &out, const GroupElemJac &a, const secp256k1_num_t &an, const secp256k1_num_t &gn);

}

#endif
