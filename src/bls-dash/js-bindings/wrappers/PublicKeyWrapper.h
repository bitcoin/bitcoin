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

#ifndef JS_BINDINGS_WRAPPERS_PUBLICKEYWRAPPER_H_
#define JS_BINDINGS_WRAPPERS_PUBLICKEYWRAPPER_H_

#include "../helpers.h"
#include "JSWrapper.h"

namespace js_wrappers {
class PublicKeyWrapper : public JSWrapper<PublicKey> {
 public:
    explicit PublicKeyWrapper(const PublicKey &publicKey);

    static const size_t PUBLIC_KEY_SIZE;

    static std::vector <PublicKey> Unwrap(std::vector <PublicKeyWrapper> wrappers);

    static PublicKeyWrapper FromBytes(val buffer);

    static PublicKeyWrapper Aggregate(val pubKeysWrappers);

    static PublicKeyWrapper AggregateInsecure(val pubKeysWrappers);

    val Serialize() const;

    uint32_t GetFingerprint() const;
};
}  // namespace js_wrappers

#endif  // JS_BINDINGS_WRAPPERS_PUBLICKEYWRAPPER_H_
