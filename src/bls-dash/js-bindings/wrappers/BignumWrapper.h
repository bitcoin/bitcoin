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

#ifndef JS_BINDINGS_WRAPPERS_BIGNUMWRAPPER_H_
#define JS_BINDINGS_WRAPPERS_BIGNUMWRAPPER_H_

#include "../helpers.h"
#include "JSWrapper.h"

namespace js_wrappers {
class Bignum {
public:
    Bignum();
    ~Bignum();
    Bignum(const Bignum &other);

    bn_st *operator &();

private:
    bn_st content;
};

class BignumWrapper : public JSWrapper<Bignum> {
public:
    explicit BignumWrapper(const Bignum &bn);
    static BignumWrapper FromString(const std::string &s, int radix);
    std::string ToString(int radix);
};
}  // namespace js_wrappers

#endif  // JS_BINDINGS_WRAPPERS_SIGNATUREWRAPPER_H_
