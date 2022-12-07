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

#include "./helpers.h"

namespace helpers {
    val toUint8Array(std::vector<uint8_t> vec) {
        val arr = helpers::toJSArray<uint8_t>(vec);
        return val::global("Uint8Array").call<val>("from", arr);
    }

    val toUint8Array(uint8_t *pointer, size_t data_size) {
        std::vector<uint8_t> vec = toVector(pointer, data_size);
        val buffer = toUint8Array(vec);
        return buffer;
    }

    val toUint8Array(bn_t bn) {
        std::vector<uint8_t> vec = toVector(bn);
        val buffer = toUint8Array(vec);
        return buffer;
    }

    std::vector<uint8_t> toVector(uint8_t *pointer, size_t data_size) {
        std::vector<uint8_t> data;
        data.reserve(data_size);
        std::copy(pointer, pointer + data_size, std::back_inserter(data));
        return data;
    }

    std::vector<uint8_t> toVector(val jsUint8Array) {
        auto l = jsUint8Array["length"].as<unsigned>();
        std::vector<uint8_t> vec;
        for (unsigned i = 0; i < l; ++i) {
            vec.push_back(jsUint8Array[i].as<uint8_t>());
        }
        return vec;
    }

    std::vector<uint8_t> toVector(bn_t bn) {
        uint8_t buf[bn_size_bin(bn)];
        bn_write_bin(buf, bn_size_bin(bn), bn);
        std::vector<uint8_t> vec = helpers::toVector(buf, bn_size_bin(bn));
        return vec;
    }

    std::vector<std::vector<uint8_t>> jsBuffersArrayToVector(val buffersArray) {
        auto l = buffersArray["length"].as<unsigned>();
        std::vector<std::vector<uint8_t>> vec;
        for (unsigned i = 0; i < l; ++i) {
            vec.push_back(toVector(buffersArray[i].as<val>()));
        }
        return vec;
    }

    std::vector<bn_t *> jsBuffersArrayToBnVector(val buffersArray) {
        auto l = buffersArray["length"].as<unsigned>();
        std::vector<bn_t *> vec;
        for (unsigned i = 0; i < l; ++i) {
            bn_t data;
            bn_new(data);
            std::vector<uint8_t> bnVec = toVector(buffersArray[i]);
            bn_read_bin(data, bnVec.data(), static_cast<int>(bnVec.size()));
            bn_t *point = &data;
            vec.push_back(point);
        }
        return vec;
    }

    val byteArraysVectorToJsBuffersArray(std::vector<uint8_t *> arraysVector, size_t element_size) {
        auto vecSize = arraysVector.size();
        std::vector<val> valVector;
        for (unsigned i = 0; i < vecSize; ++i) {
            valVector.push_back(toUint8Array(arraysVector[i], element_size));
        }
        val arr = helpers::toJSArray<val>(valVector);
        return arr;
    }
}  // namespace helpers
