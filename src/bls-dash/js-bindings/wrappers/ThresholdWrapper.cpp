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

#include "ThresholdWrapper.h"

namespace js_wrappers {
PrivateKeyWrapper ThresholdWrapper::Create(val pubKeyWrappersArray, val privateKeyWrappers, size_t threshold,
                                           size_t playersCount) {
    std::vector <PublicKey> commitment = PublicKeyWrapper::Unwrap(
            helpers::toVectorFromJSArray<PublicKeyWrapper>(pubKeyWrappersArray));
    std::vector <PrivateKey> secretFragment = PrivateKeyWrapper::Unwrap(
            helpers::toVectorFromJSArray<PrivateKeyWrapper>(privateKeyWrappers));
    PrivateKey result = Threshold::Create(commitment, secretFragment, threshold, playersCount);
    auto cl = commitment.size();
    for (unsigned i = 0; i < cl; ++i) {
        pubKeyWrappersArray.set(i, PublicKeyWrapper(commitment.at(i)));
    }
    auto fl = secretFragment.size();
    for (unsigned i = 0; i < fl; ++i) {
        privateKeyWrappers.set(i, PrivateKeyWrapper(secretFragment.at(i)));
    }
    return PrivateKeyWrapper(result);
}

InsecureSignatureWrapper
ThresholdWrapper::SignWithCoefficient(const PrivateKeyWrapper &privateKeyWrapper, val msgBuffer,
                                      size_t playerIndex, val players) {
    PrivateKey sk = privateKeyWrapper.GetWrappedInstance();
    std::vector <uint8_t> message = helpers::toVector(msgBuffer);
    std::vector <size_t> playersVector = helpers::toVectorFromJSArray<size_t>(players);
    InsecureSignature sig = Threshold::SignWithCoefficient(sk, message.data(), message.size(), playerIndex,
                                                           playersVector.data(), playersVector.size());
    return InsecureSignatureWrapper(sig);
}

InsecureSignatureWrapper ThresholdWrapper::AggregateUnitSigs(val insecureSignatures, val messageBuffer,
                                                             val playersArray) {
    std::vector <InsecureSignature> sigs = InsecureSignatureWrapper::Unwrap(
            helpers::toVectorFromJSArray<InsecureSignatureWrapper>(insecureSignatures));
    std::vector <uint8_t> message = helpers::toVector(messageBuffer);
    std::vector <size_t> players = helpers::toVectorFromJSArray<size_t>(playersArray);
    InsecureSignature aggregatedSig = Threshold::AggregateUnitSigs(sigs, message.data(), message.size(),
                                                                   players.data(), players.size());
    return InsecureSignatureWrapper(aggregatedSig);
}

bool ThresholdWrapper::VerifySecretFragment(size_t playerIndex, const PrivateKeyWrapper &secretFragment,
                                            val publicKeyWrappers, size_t threshold) {
    std::vector <PublicKey> commitment = PublicKeyWrapper::Unwrap(
            helpers::toVectorFromJSArray<PublicKeyWrapper>(publicKeyWrappers));
    return Threshold::VerifySecretFragment(playerIndex, secretFragment.GetWrappedInstance(), commitment, threshold);
}
}  // namespace js_wrappers
