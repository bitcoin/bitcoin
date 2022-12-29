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

#ifndef JS_BINDINGS_WRAPPERS_G1ELEMENTWRAPPER_H_
#define JS_BINDINGS_WRAPPERS_G1ELEMENTWRAPPER_H_

#include <helpers.h>
#include "JSWrapper.h"
#include "BignumWrapper.h"

namespace js_wrappers {
class G1ElementWrapper : public JSWrapper<G1Element> {
public:
    explicit G1ElementWrapper(const G1Element &publicKey);

    G1ElementWrapper();

    static const size_t SIZE;

    static std::vector <G1Element> Unwrap(std::vector <G1ElementWrapper> wrappers);

    static G1ElementWrapper FromBytes(val buffer);

    static G1ElementWrapper Generator();

    val Serialize() const;

    G1ElementWrapper Add(const G1ElementWrapper &other);

    G1ElementWrapper Mul(const BignumWrapper &other);

    bool EqualTo(const G1ElementWrapper &others);

    G1ElementWrapper Negate();

    G1ElementWrapper Deepcopy();

    uint32_t GetFingerprint() const;
};
}  // namespace js_wrappers

#endif  // JS_BINDINGS_WRAPPERS_G1ELEMENTWRAPPER_H_
