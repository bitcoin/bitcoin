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

#ifndef JS_BINDINGS_WRAPPERS_G2ELEMENTWRAPPER_H_
#define JS_BINDINGS_WRAPPERS_G2ELEMENTWRAPPER_H_

#include <helpers.h>
#include "JSWrapper.h"
#include "BignumWrapper.h"

namespace js_wrappers {
class G2ElementWrapper : public JSWrapper<G2Element> {
 public:
    explicit G2ElementWrapper(const G2Element &signature);

    G2ElementWrapper();

    static const size_t SIZE;

    static std::vector <G2Element> Unwrap(std::vector <G2ElementWrapper> sigWrappers);

    static G2ElementWrapper FromG2Element(const G2Element &signature);

    static G2ElementWrapper FromBytes(val buffer);

    static G2ElementWrapper AggregateSigs(val signatureWrappers);

    static G2ElementWrapper Generator();

    G2ElementWrapper Deepcopy();

    G2ElementWrapper Negate();

    val Serialize() const;

    G2ElementWrapper Add(const G2ElementWrapper &other);

    G2ElementWrapper Mul(const BignumWrapper &other);

    bool EqualTo(const G2ElementWrapper &others);
};
}  // namespace js_wrappers

#endif  // JS_BINDINGS_WRAPPERS_G2ELEMENTWRAPPER_H_
