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

#include "PublicKeyWrapper.h"

namespace js_wrappers {
PublicKeyWrapper::PublicKeyWrapper(const PublicKey &publicKey) : JSWrapper(publicKey) {}

const size_t PublicKeyWrapper::PUBLIC_KEY_SIZE = PublicKey::PUBLIC_KEY_SIZE;

std::vector <PublicKey> PublicKeyWrapper::Unwrap(std::vector <PublicKeyWrapper> wrappers) {
    std::vector <PublicKey> unwrapped;
    for (auto &wrapper : wrappers) {
        unwrapped.push_back(wrapper.GetWrappedInstance());
    }
    return unwrapped;
}

PublicKeyWrapper PublicKeyWrapper::FromBytes(val buffer) {
    std::vector <uint8_t> bytes = helpers::toVector(buffer);
    PublicKey pk = PublicKey::FromBytes(bytes.data());
    return PublicKeyWrapper(pk);
}

PublicKeyWrapper PublicKeyWrapper::Aggregate(val pubKeysWrappers) {
    std::vector <PublicKey> pubKeys = PublicKeyWrapper::Unwrap(
            helpers::toVectorFromJSArray<PublicKeyWrapper>(pubKeysWrappers));
    PublicKey aggregatedPk = PublicKey::Aggregate(pubKeys);
    return PublicKeyWrapper(aggregatedPk);
}

PublicKeyWrapper PublicKeyWrapper::AggregateInsecure(val pubKeysWrappers) {
    std::vector <PublicKey> pubKeys = PublicKeyWrapper::Unwrap(
            helpers::toVectorFromJSArray<PublicKeyWrapper>(pubKeysWrappers));
    PublicKey aggregatedPk = PublicKey::AggregateInsecure(pubKeys);
    return PublicKeyWrapper(aggregatedPk);
}

val PublicKeyWrapper::Serialize() const {
    return helpers::toUint8Array(wrapped.Serialize());
}

uint32_t PublicKeyWrapper::GetFingerprint() const {
    return wrapped.GetFingerprint();
}
}  // namespace js_wrappers
