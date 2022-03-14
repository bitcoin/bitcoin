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

#include "../helpers.h"
#include "BignumWrapper.h"

namespace js_wrappers {
Bignum::Bignum() {
    bn_make(&content, 1);
}

Bignum::~Bignum() {
    bn_clean(&content);
}

Bignum::Bignum(const Bignum &other) {
    bn_copy(&content, &other.content);
}

bn_st *Bignum::operator &() { return &content; }

BignumWrapper::BignumWrapper(const Bignum &bn) : JSWrapper(bn) {}

BignumWrapper BignumWrapper::FromString(const std::string &s, int radix) {
    Bignum result;
    bn_read_str(&result, s.c_str(), s.size(), radix);
    return BignumWrapper(result);
}

std::string BignumWrapper::ToString(int radix) {
    auto wrapper = GetWrappedInstance();
    int strsize = bn_size_str(&wrapper, radix);
    std::vector<char> strholder(strsize);
    bn_write_str(&strholder[0], strsize, &wrapper, radix);
    return std::string(&strholder[0], strsize - 1);
}
}  // namespace js_wrappers
