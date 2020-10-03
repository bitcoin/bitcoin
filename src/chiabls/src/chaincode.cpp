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

#include "chaincode.hpp"
#include "bls.hpp"
namespace bls {

ChainCode ChainCode::FromBytes(const uint8_t* bytes) {
    ChainCode c = ChainCode();
    bn_new(c.chainCode);
    bn_read_bin(c.chainCode, bytes, ChainCode::CHAIN_CODE_SIZE);
    return c;
}

ChainCode::ChainCode(const ChainCode &cc) {
    uint8_t bytes[ChainCode::CHAIN_CODE_SIZE];
    cc.Serialize(bytes);
    bn_new(chainCode);
    bn_read_bin(chainCode, bytes, ChainCode::CHAIN_CODE_SIZE);
}

// Comparator implementation.
bool operator==(ChainCode const &a,  ChainCode const &b) {
    return bn_cmp(a.chainCode, b.chainCode) == CMP_EQ;
}

bool operator!=(ChainCode const &a,  ChainCode const &b) {
    return !(a == b);
}

std::ostream &operator<<(std::ostream &os, ChainCode const &s) {
    uint8_t buffer[ChainCode::CHAIN_CODE_SIZE];
    s.Serialize(buffer);
    return os << Util::HexStr(buffer, ChainCode::CHAIN_CODE_SIZE);
}

void ChainCode::Serialize(uint8_t *buffer) const {
    bn_write_bin(buffer, ChainCode::CHAIN_CODE_SIZE, chainCode);
}

std::vector<uint8_t> ChainCode::Serialize() const {
    std::vector<uint8_t> data(CHAIN_CODE_SIZE);
    Serialize(data.data());
    return data;
}
} // end namespace bls
