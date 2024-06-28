// Copyright (c) 2021 The Dash Core developers

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <vector>
#include "bls.hpp"
#include "privatekey.h"
#include "elements.h"

// helper functions
template <class T>
std::vector<T> toBLSVector(void** elems, const size_t len) {
    std::vector<T> vec;
    vec.reserve(len);
    for (int i = 0 ; i < (int)len; ++i) {
        const T* el = (T*)elems[i];
        vec.push_back(*el);
    }
    return vec;
}

std::vector<bls::Bytes> toVectorBytes(void** elems, const size_t len, const std::vector<size_t> vecElemsLens);
