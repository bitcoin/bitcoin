#ifndef _SECP256K1_SCALAR_
#define _SECP256K1_SCALAR_

#include "num.h"
#include "consts.h"

namespace secp256k1 {
/** Implements arithmetic modulo FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE BAAEDCE6 AF48A03B BFD25E8C D0364141,
 *  using OpenSSL's BIGNUM
 */
class Scalar : private Number {
public:
};

}

#endif
