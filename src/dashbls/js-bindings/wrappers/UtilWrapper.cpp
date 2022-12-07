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

#include "UtilWrapper.h"

namespace js_wrappers {
val UtilWrapper::Hash256(val msg) {
    std::vector<uint8_t> bytes = helpers::toVector(msg);
    std::vector<uint8_t> output(BLS::MESSAGE_HASH_LEN);
    Util::Hash256(&output[0], (const uint8_t *)bytes.data(), bytes.size());
    return helpers::toUint8Array(&output[0], output.size());
}
std::string UtilWrapper::HexStr(val msg) {
    std::vector<uint8_t> bytes = helpers::toVector(msg);
    return Util::HexStr(bytes);
}
}  // namespace js_wrappers
