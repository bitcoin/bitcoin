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

#ifndef JS_BINDINGS_WRAPPERS_THRESHOLDWRAPPER_H_
#define JS_BINDINGS_WRAPPERS_THRESHOLDWRAPPER_H_

#include "../helpers.h"
#include "PrivateKeyWrapper.h"

namespace js_wrappers {
class ThresholdWrapper {
 public:
    static PrivateKeyWrapper
    Create(val pubKeyWrappersArray, val privateKeyWrappers, size_t threshold, size_t playersCount);

    static InsecureSignatureWrapper
    SignWithCoefficient(const PrivateKeyWrapper &privateKeyWrapper, val msgBuffer, size_t playerIndex, val players);

    static InsecureSignatureWrapper AggregateUnitSigs(val insecureSignatures, val messageBuffer, val playersArray);

    static bool
    VerifySecretFragment(size_t playerIndex, const PrivateKeyWrapper &secretFragment, val publicKeyWrappers,
                         size_t threshold);
};
}  // namespace js_wrappers

#endif  // JS_BINDINGS_WRAPPERS_THRESHOLDWRAPPER_H_
