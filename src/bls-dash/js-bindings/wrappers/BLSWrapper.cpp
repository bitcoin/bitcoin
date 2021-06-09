//
// Created by anton on 17.03.19.
//

#include "BLSWrapper.h"

namespace js_wrappers {
PublicKeyWrapper BLSWrapper::DHKeyExchange(const PrivateKeyWrapper &privateKeyWrapper, const PublicKeyWrapper &publicKeyWrapper) {
    PrivateKey sk = privateKeyWrapper.GetWrappedInstance();
    PublicKey pk = publicKeyWrapper.GetWrappedInstance();
    PublicKey ret = BLS::DHKeyExchange(sk, pk);
    return PublicKeyWrapper(ret);
}
}  // namespace js_wrappers
