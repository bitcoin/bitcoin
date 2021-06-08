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

#ifndef JS_BINDINGS_WRAPPERS_PRIVATEKEYWRAPPER_H_
#define JS_BINDINGS_WRAPPERS_PRIVATEKEYWRAPPER_H_

#include "../helpers.h"
#include "JSWrapper.h"
#include "SignatureWrapper.h"
#include "PublicKeyWrapper.h"

namespace js_wrappers {
class PrivateKeyWrapper : public JSWrapper<PrivateKey> {
 public:
    explicit PrivateKeyWrapper(const PrivateKey &privateKey);

    static const size_t PRIVATE_KEY_SIZE;

    static std::vector<PrivateKey> Unwrap(std::vector<PrivateKeyWrapper> wrappers);

    static PrivateKeyWrapper FromSeed(val buffer);

    static PrivateKeyWrapper FromBytes(val buffer, bool modOrder);

    static PrivateKeyWrapper Aggregate(val privateKeysArray, val publicKeysArray);

    static PrivateKeyWrapper AggregateInsecure(val privateKeysArray);

    val Serialize() const;

    SignatureWrapper Sign(val messageBuffer) const;

    InsecureSignatureWrapper SignInsecure(val messageBuffer) const;

    SignatureWrapper SignPrehashed(val messageHashBuffer) const;

    PublicKeyWrapper GetPublicKey() const;
};
}  // namespace js_wrappers


#endif  // JS_BINDINGS_WRAPPERS_PRIVATEKEYWRAPPER_H_
