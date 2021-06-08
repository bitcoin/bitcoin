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

#ifndef JS_BINDINGS_WRAPPERS_EXTENDEDPUBLICKEYWRAPPER_H_
#define JS_BINDINGS_WRAPPERS_EXTENDEDPUBLICKEYWRAPPER_H_

#include "../helpers.h"
#include "PublicKeyWrapper.h"
#include "ChainCodeWrapper.h"

namespace js_wrappers {
class ExtendedPublicKeyWrapper : public JSWrapper<ExtendedPublicKey> {
 public:
    explicit ExtendedPublicKeyWrapper(const ExtendedPublicKey &extendedPublicKey);

    static const size_t VERSION;

    static const size_t EXTENDED_PUBLIC_KEY_SIZE;

    static ExtendedPublicKeyWrapper FromBytes(val serializedBuffer);

    ExtendedPublicKeyWrapper PublicChild(uint32_t i) const;

    uint32_t GetVersion() const;

    uint8_t GetDepth() const;

    uint32_t GetParentFingerprint() const;

    uint32_t GetChildNumber() const;

    ChainCodeWrapper GetChainCode() const;

    PublicKeyWrapper GetPublicKey() const;

    val Serialize() const;
};
}  // namespace js_wrappers


#endif  // JS_BINDINGS_WRAPPERS_EXTENDEDPUBLICKEYWRAPPER_H_
