//
// Created by anton on 17.03.19.
//

#ifndef BLS_BLSWRAPPER_H
#define BLS_BLSWRAPPER_H

#include "../helpers.h"
#include "PublicKeyWrapper.h"
#include "PrivateKeyWrapper.h"

namespace js_wrappers {
class BLSWrapper {
 public:
    static PublicKeyWrapper DHKeyExchange(const PrivateKeyWrapper &privateKeyWrapper, const PublicKeyWrapper &publicKeyWrapper);
};
}  // namespace js_wrappers

#endif  // BLS_BLSWRAPPER_H
