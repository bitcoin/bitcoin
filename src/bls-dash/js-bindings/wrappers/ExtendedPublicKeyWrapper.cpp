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

#include "ExtendedPublicKeyWrapper.h"

namespace js_wrappers {
ExtendedPublicKeyWrapper::ExtendedPublicKeyWrapper(const ExtendedPublicKey &extendedPublicKey) : JSWrapper(
        extendedPublicKey) {}

const size_t ExtendedPublicKeyWrapper::VERSION = ExtendedPublicKey::VERSION;


const size_t ExtendedPublicKeyWrapper::EXTENDED_PUBLIC_KEY_SIZE = ExtendedPublicKey::EXTENDED_PUBLIC_KEY_SIZE;

ExtendedPublicKeyWrapper ExtendedPublicKeyWrapper::FromBytes(val serializedBuffer) {
    std::vector <uint8_t> serialized = helpers::toVector(serializedBuffer);
    ExtendedPublicKey pk = ExtendedPublicKey::FromBytes(serialized.data());
    return ExtendedPublicKeyWrapper(pk);
}

ExtendedPublicKeyWrapper ExtendedPublicKeyWrapper::PublicChild(uint32_t i) const {
    ExtendedPublicKey pk = wrapped.PublicChild(i);
    return ExtendedPublicKeyWrapper(pk);
}

uint32_t ExtendedPublicKeyWrapper::GetVersion() const {
    return wrapped.GetVersion();
}

uint8_t ExtendedPublicKeyWrapper::GetDepth() const {
    return wrapped.GetDepth();
}

uint32_t ExtendedPublicKeyWrapper::GetParentFingerprint() const {
    return wrapped.GetParentFingerprint();
}

uint32_t ExtendedPublicKeyWrapper::GetChildNumber() const {
    return wrapped.GetChildNumber();
}

ChainCodeWrapper ExtendedPublicKeyWrapper::GetChainCode() const {
    ChainCode chainCode = wrapped.GetChainCode();
    return ChainCodeWrapper(chainCode);
}

PublicKeyWrapper ExtendedPublicKeyWrapper::GetPublicKey() const {
    PublicKey pk = wrapped.GetPublicKey();
    return PublicKeyWrapper(pk);
}

val ExtendedPublicKeyWrapper::Serialize() const {
    return helpers::toUint8Array(wrapped.Serialize());
}
}  // namespace js_wrappers
