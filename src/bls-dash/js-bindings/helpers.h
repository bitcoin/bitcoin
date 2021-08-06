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

#ifndef JS_BINDINGS_HELPERS_H_
#define JS_BINDINGS_HELPERS_H_

#include <algorithm>
#include <vector>
#include "emscripten/val.h"
#include <relic.h>
#include "../src/bls.hpp"

using namespace emscripten;
using namespace bls;

namespace helpers {
    val toUint8Array(uint8_t *pointer, size_t data_size);

    val toUint8Array(std::vector<uint8_t> vec);

    val toUint8Array(bn_t bn);

    std::vector<uint8_t> toVector(uint8_t *pointer, size_t data_size);

    std::vector<uint8_t> toVector(val jsBuffer);

    std::vector<uint8_t> toVector(bn_t bn);

    template<typename T>
    inline std::vector<T> toVectorFromJSArray(val jsArray) {
        auto l = jsArray["length"].as<unsigned>();
        std::vector<T> vec;
        for (unsigned i = 0; i < l; ++i) {
            vec.push_back(jsArray[i].as<T>());
        }
        return vec;
    }

    template<typename T>
    inline val toJSArray(std::vector<T> vec) {
        val Array = val::global("Array");
        val arr = Array.new_();
        auto l = vec.size();
        for (unsigned i = 0; i < l; ++i) {
            arr.call<void>("push", vec[i]);
        }
        return arr;
    }

    std::vector<std::vector<uint8_t>> jsBuffersArrayToVector(val buffersArray);

    std::vector<bn_t *> jsBuffersArrayToBnVector(val buffersArray);

    val byteArraysVectorToJsBuffersArray(std::vector<uint8_t *> arraysVector, size_t element_size);
}  // namespace helpers

#endif  // JS_BINDINGS_HELPERS_H_
