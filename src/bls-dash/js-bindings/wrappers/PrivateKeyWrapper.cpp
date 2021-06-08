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

#include "PrivateKeyWrapper.h"

namespace js_wrappers {
PrivateKeyWrapper::PrivateKeyWrapper(const PrivateKey &privateKey) : JSWrapper(privateKey) {}

const size_t PrivateKeyWrapper::PRIVATE_KEY_SIZE = PrivateKey::PRIVATE_KEY_SIZE;

std::vector <PrivateKey> PrivateKeyWrapper::Unwrap(std::vector <PrivateKeyWrapper> wrappers) {
    std::vector <PrivateKey> unwrapped;
    for (auto &wrapper : wrappers) {
        unwrapped.push_back(wrapper.GetWrappedInstance());
    }
    return unwrapped;
}

PrivateKeyWrapper PrivateKeyWrapper::FromSeed(val buffer) {
    std::vector <uint8_t> bytes = helpers::toVector(buffer);
    PrivateKey sk = PrivateKey::FromSeed(bytes.data(), bytes.size());
    return PrivateKeyWrapper(sk);
}

PrivateKeyWrapper PrivateKeyWrapper::Aggregate(val privateKeysArray, val publicKeysArray) {
    std::vector <PublicKey> pubKeys = PublicKeyWrapper::Unwrap(
            helpers::toVectorFromJSArray<PublicKeyWrapper>(publicKeysArray));
    std::vector <PrivateKey> privateKeys = PrivateKeyWrapper::Unwrap(
            helpers::toVectorFromJSArray<PrivateKeyWrapper>(privateKeysArray));

    PrivateKey aggregatedSk = PrivateKey::Aggregate(privateKeys, pubKeys);
    return PrivateKeyWrapper(aggregatedSk);
}

PrivateKeyWrapper PrivateKeyWrapper::AggregateInsecure(val privateKeysArray) {
    std::vector <PrivateKey> privateKeys = PrivateKeyWrapper::Unwrap(
            helpers::toVectorFromJSArray<PrivateKeyWrapper>(privateKeysArray));
    PrivateKey aggregatedSk = PrivateKey::AggregateInsecure(privateKeys);
    return PrivateKeyWrapper(aggregatedSk);
}

PrivateKeyWrapper PrivateKeyWrapper::FromBytes(val buffer, bool modOrder) {
    std::vector <uint8_t> bytes = helpers::toVector(buffer);
    PrivateKey pk = PrivateKey::FromBytes(bytes.data(), modOrder);
    return PrivateKeyWrapper(pk);
}

val PrivateKeyWrapper::Serialize() const {
    return helpers::toUint8Array(wrapped.Serialize());
}

SignatureWrapper PrivateKeyWrapper::Sign(val messageBuffer) const {
    std::vector <uint8_t> message = helpers::toVector(messageBuffer);
    Signature signature = wrapped.Sign(message.data(), message.size());
    return SignatureWrapper::FromSignature(signature);
}

InsecureSignatureWrapper PrivateKeyWrapper::SignInsecure(val messageBuffer) const {
    std::vector <uint8_t> message = helpers::toVector(messageBuffer);
    InsecureSignature signature = wrapped.SignInsecure(message.data(), message.size());
    return InsecureSignatureWrapper(signature);
}

SignatureWrapper PrivateKeyWrapper::SignPrehashed(val messageHashBuffer) const {
    std::vector <uint8_t> hash = helpers::toVector(messageHashBuffer);
    Signature signature = wrapped.SignPrehashed(hash.data());
    return SignatureWrapper::FromSignature(signature);
}

PublicKeyWrapper PrivateKeyWrapper::GetPublicKey() const {
    PublicKey pk = wrapped.GetPublicKey();
    return PublicKeyWrapper(pk);
}
}  // namespace js_wrappers
