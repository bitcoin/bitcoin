// Copyright 2020 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ExtendedPrivateKeyWrapper.h"

namespace js_wrappers {
ExtendedPrivateKeyWrapper::ExtendedPrivateKeyWrapper(const ExtendedPrivateKey &extendedPrivateKey) : JSWrapper(
        extendedPrivateKey) {}

const size_t ExtendedPrivateKeyWrapper::EXTENDED_PRIVATE_KEY_SIZE = ExtendedPrivateKey::EXTENDED_PRIVATE_KEY_SIZE;

ExtendedPrivateKeyWrapper ExtendedPrivateKeyWrapper::FromSeed(val seedBuffer) {
    std::vector <uint8_t> seed = helpers::toVector(seedBuffer);
    ExtendedPrivateKey esk = ExtendedPrivateKey::FromSeed(seed.data(), seed.size());
    return ExtendedPrivateKeyWrapper(esk);
}

ExtendedPrivateKeyWrapper ExtendedPrivateKeyWrapper::FromBytes(val serializedBuffer) {
    std::vector <uint8_t> serialized = helpers::toVector(serializedBuffer);
    ExtendedPrivateKey esk = ExtendedPrivateKey::FromBytes(serialized.data());
    return ExtendedPrivateKeyWrapper(esk);
}

ExtendedPrivateKeyWrapper ExtendedPrivateKeyWrapper::PrivateChild(uint32_t i) const {
    ExtendedPrivateKey esk = wrapped.PrivateChild(i);
    return ExtendedPrivateKeyWrapper(esk);
}

ExtendedPublicKeyWrapper ExtendedPrivateKeyWrapper::PublicChild(uint32_t i) const {
    ExtendedPublicKey epk = wrapped.PublicChild(i);
    return ExtendedPublicKeyWrapper(epk);
}

uint32_t ExtendedPrivateKeyWrapper::GetVersion() const {
    return wrapped.GetVersion();
}

uint8_t ExtendedPrivateKeyWrapper::GetDepth() const {
    return wrapped.GetDepth();
}

uint32_t ExtendedPrivateKeyWrapper::GetParentFingerprint() const {
    return wrapped.GetParentFingerprint();
}

uint32_t ExtendedPrivateKeyWrapper::GetChildNumber() const {
    return wrapped.GetChildNumber();
}

ChainCodeWrapper ExtendedPrivateKeyWrapper::GetChainCode() const {
    ChainCode chainCode = wrapped.GetChainCode();
    return ChainCodeWrapper(chainCode);
}

PrivateKeyWrapper ExtendedPrivateKeyWrapper::GetPrivateKey() const {
    PrivateKey sk = wrapped.GetPrivateKey();
    return PrivateKeyWrapper(sk);
}

PublicKeyWrapper ExtendedPrivateKeyWrapper::GetPublicKey() const {
    PublicKey pk = wrapped.GetPublicKey();
    return PublicKeyWrapper(pk);
}

ExtendedPublicKeyWrapper ExtendedPrivateKeyWrapper::GetExtendedPublicKey() const {
    ExtendedPublicKey pk = wrapped.GetExtendedPublicKey();
    return ExtendedPublicKeyWrapper(pk);
}

val ExtendedPrivateKeyWrapper::Serialize() const {
    return helpers::toUint8Array(wrapped.Serialize());
}
}  // namespace js_wrappers
