// Copyright 2018 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_CHAINCODE_HPP_
#define SRC_CHAINCODE_HPP_

#include <iostream>
#include <vector>

#include <relic_conf.h>

#if defined GMP && ARITH == GMP
#include <gmp.h>
#endif


#include <relic.h>
#include <relic_test.h>

#include "util.hpp"

namespace bls {
class ChainCode {
 public:
    static const size_t SIZE = 32;

    static ChainCode FromBytes(const Bytes& bytes);

    ChainCode(const ChainCode &cc);

    // Comparator implementation.
    friend bool operator==(ChainCode const &a,  ChainCode const &b);
    friend bool operator!=(ChainCode const &a,  ChainCode const &b);
    friend std::ostream &operator<<(std::ostream &os, ChainCode const &s);

    void Serialize(uint8_t *buffer) const;
    std::vector<uint8_t> Serialize() const;

    // Prevent direct construction, use static constructor
    ChainCode() {}
private:

    bn_t chainCode;
};
} // end namespace bls

#endif  // SRC_CHAINCODE_HPP_

