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

#include "ChainCodeWrapper.h"

namespace js_wrappers {
ChainCodeWrapper::ChainCodeWrapper(const ChainCode &chainCode) : JSWrapper(chainCode) {}

const size_t ChainCodeWrapper::CHAIN_CODE_SIZE = ChainCode::CHAIN_CODE_SIZE;

ChainCodeWrapper ChainCodeWrapper::FromBytes(val jsBuffer) {
    std::vector <uint8_t> bytes = helpers::toVector(jsBuffer);
    ChainCode chainCode = ChainCode::FromBytes(bytes.data());
    return ChainCodeWrapper(chainCode);
}

val ChainCodeWrapper::Serialize() const {
    return helpers::toUint8Array(wrapped.Serialize());
}
}  // namespace js_wrappers
