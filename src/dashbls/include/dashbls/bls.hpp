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

#ifndef SRC_BLS_HPP_
#define SRC_BLS_HPP_

#include "privatekey.hpp"
#include "util.hpp"
#include "schemes.hpp"
#include "chaincode.hpp"
#include "elements.hpp"
#include "extendedprivatekey.hpp"
#include "extendedpublickey.hpp"
#include "hkdf.hpp"
#include "hdkeys.hpp"
#include "threshold.hpp"

namespace bls {

/*
 * Principal class for verification and signature aggregation.
 * Include this file to use the library.
 */
class BLS {
 public:
    static const size_t MESSAGE_HASH_LEN = 32;

    // Initializes the BLS library (called automatically)
    static bool Init();

    static void SetSecureAllocator(Util::SecureAllocCallback allocCb, Util::SecureFreeCallback freeCb);

    static void CheckRelicErrors(bool should_throw = true);
};
} // end namespace bls

#endif  // SRC_BLS_HPP_
